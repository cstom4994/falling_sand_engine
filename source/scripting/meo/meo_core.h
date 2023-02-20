// Metadot Code Copyright(c) 2022-2023, KaoruXun All rights reserved.
// https://github.com/pigpigyyy/Yuescript
// https://github.com/axilmar/parserlib

#ifndef _METADOT_MEO_CORE_H_
#define _METADOT_MEO_CORE_H_

#include <algorithm>
#include <cassert>
#include <codecvt>
#include <functional>
#include <list>
#include <locale>
#include <memory>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#pragma region ParserLib

namespace parserlib {

typedef std::basic_string<wchar_t> input;
typedef input::iterator input_it;
typedef std::wstring_convert<std::codecvt_utf8_utf16<input::value_type>> Converter;

class _private;
class _expr;
class _context;
class rule;

class pos {
public:
    input::iterator m_it;
    int m_line;
    int m_col;
    pos() : m_line(0), m_col(0) {}
    pos(input &i);
};
struct item_t {
    pos *begin;
    pos *end;
    void *user_data;
};
typedef std::function<bool(const item_t &)> user_handler;
class expr {
public:
    expr(char c);
    expr(const char *s);
    expr(rule &r);
    expr operator*() const;
    expr operator+() const;
    expr operator-() const;
    expr operator&() const;
    expr operator!() const;

private:
    _expr *m_expr;
    expr(_expr *e) : m_expr(e) {}
    expr &operator=(expr &);
    friend class _private;
};
typedef void (*parse_proc)(const pos &b, const pos &e, void *d);
class input_range {
public:
    virtual ~input_range() {}
    pos m_begin;
    pos m_end;
    input_range() {}
    input_range(const pos &b, const pos &e);
};
enum ERROR_TYPE { ERROR_SYNTAX_ERROR = 1, ERROR_INVALID_EOF, ERROR_USER = 100 };
class error : public input_range {
public:
    int m_type;
    error(const pos &b, const pos &e, int t);
    bool operator<(const error &e) const;
};
typedef std::list<error> error_list;
class rule {
public:
    rule();
    rule(char c);
    rule(const char *s);
    rule(const expr &e);
    rule(rule &r);
    rule(const rule &r);
    ~rule();
    expr operator*();
    expr operator+();
    expr operator-();
    expr operator&();
    expr operator!();
    void set_parse_proc(parse_proc p);
    rule *this_ptr() { return this; }
    rule &operator=(rule &);
    rule &operator=(const expr &);

private:
    enum _MODE { _PARSE, _REJECT, _ACCEPT };
    struct _state {
        size_t m_pos;
        _MODE m_mode;
        _state(size_t pos = -1, _MODE mode = _PARSE) : m_pos(pos), m_mode(mode) {}
    };
    _expr *m_expr;
    parse_proc m_parse_proc;
    _state m_state;
    friend class _private;
    friend class _context;
};
expr operator>>(const expr &left, const expr &right);
expr operator|(const expr &left, const expr &right);
expr term(const expr &e);
expr set(const char *s);
expr range(int min, int max);
expr nl(const expr &e);
expr eof();
expr not_(const expr &e);
expr and_(const expr &e);
expr any();
expr true_();
expr false_();
expr user(const expr &e, const user_handler &handler);
bool parse(input &i, rule &g, error_list &el, void *d, void *ud);
template <class T>
T &operator<<(T &stream, const input_range &ir) {
    for (input::const_iterator it = ir.m_begin.m_it; it != ir.m_end.m_it; ++it) {
        stream << (typename T::char_type) * it;
    }
    return stream;
}
}  // namespace parserlib
namespace parserlib {
class ast_node;
template <bool Required, class T>
class ast_ptr;
template <bool Required, class T>
class ast_list;
template <class T>
class ast;
typedef std::vector<ast_node *> ast_stack;
typedef std::list<ast_node *> node_container;
template <size_t Num>
struct Counter {
    enum { value = Counter<Num - 1>::value };
};
template <>
struct Counter<0> {
    enum { value = 0 };
};
#define COUNTER_READ Counter<__LINE__>::value
#define COUNTER_INC                                        \
    template <>                                            \
    struct Counter<__LINE__> {                             \
        enum { value = Counter<__LINE__ - 1>::value + 1 }; \
    }
class ast_node;
template <class T>
constexpr typename std::enable_if<std::is_base_of<ast_node, T>::value, int>::type id();
enum class traversal { Continue, Return, Stop };
class ast_node : public input_range {
public:
    ast_node() : _ref(0) {}
    void retain() { ++_ref; }
    void release() {
        --_ref;
        if (_ref == 0) {
            delete this;
        }
    }
    virtual void construct(ast_stack &) {}
    virtual traversal traverse(const std::function<traversal(ast_node *)> &func);
    template <typename... Ts>
    struct select_last {
        using type = typename decltype((std::enable_if<true, Ts>{}, ...))::type;
    };
    template <typename... Ts>
    using select_last_t = typename select_last<Ts...>::type;
    template <class... Args>
    select_last_t<Args...> *getByPath() {
        int types[] = {id<Args>()...};
        return static_cast<select_last_t<Args...> *>(getByTypeIds(std::begin(types), std::end(types)));
    }
    virtual bool visitChild(const std::function<bool(ast_node *)> &func);
    virtual int getId() const = 0;
    virtual const std::string_view getName() const = 0;
    template <class T>
    inline ast_ptr<false, T> new_ptr() const {
        auto item = new T;
        item->m_begin.m_line = m_begin.m_line;
        item->m_begin.m_col = m_begin.m_col;
        item->m_end.m_line = m_end.m_line;
        item->m_end.m_col = m_end.m_col;
        return ast_ptr<false, T>(item);
    }

private:
    int _ref;
    ast_node *getByTypeIds(int *begin, int *end);
};
template <class T>
constexpr typename std::enable_if<std::is_base_of<ast_node, T>::value, int>::type id() {
    return 0;
}
template <class T>
T *ast_cast(ast_node *node) {
    return node && id<T>() == node->getId() ? static_cast<T *>(node) : nullptr;
}
template <class T>
T *ast_to(ast_node *node) {
    assert(node->getId() == id<T>());
    return static_cast<T *>(node);
}
template <class... Args>
bool ast_is(ast_node *node) {
    if (!node) return false;
    bool result = false;
    int i = node->getId();
    using swallow = bool[];
    (void)swallow{result || (result = id<Args>() == i)...};
    return result;
}
class ast_member;
typedef std::vector<ast_member *> ast_member_vector;
class ast_container : public ast_node {
public:
    void add_members(std::initializer_list<ast_member *> members) {
        for (auto member : members) {
            m_members.push_back(member);
        }
    }
    const ast_member_vector &members() const { return m_members; }
    virtual void construct(ast_stack &st) override;
    virtual traversal traverse(const std::function<traversal(ast_node *)> &func) override;
    virtual bool visitChild(const std::function<bool(ast_node *)> &func) override;

private:
    ast_member_vector m_members;
    friend class ast_member;
};
enum class ast_holder_type { Pointer, List };
class ast_member {
public:
    virtual ~ast_member() {}
    virtual void construct(ast_stack &st) = 0;
    virtual bool accept(ast_node *node) = 0;
    virtual ast_holder_type get_type() const = 0;
};
class _ast_ptr : public ast_member {
public:
    _ast_ptr(ast_node *node) : m_ptr(node) {
        if (node) node->retain();
    }
    virtual ~_ast_ptr() {
        if (m_ptr) {
            m_ptr->release();
            m_ptr = nullptr;
        }
    }
    ast_node *get() const { return m_ptr; }
    template <class T>
    T *as() const {
        return ast_cast<T>(m_ptr);
    }
    template <class T>
    T *to() const {
        assert(m_ptr && m_ptr->getId() == id<T>());
        return static_cast<T *>(m_ptr);
    }
    template <class T>
    bool is() const {
        return m_ptr && m_ptr->getId() == id<T>();
    }
    void set(ast_node *node) {
        if (node == m_ptr) {
            return;
        } else if (!node) {
            if (m_ptr) m_ptr->release();
            m_ptr = nullptr;
        } else {
            assert(accept(node));
            node->retain();
            if (m_ptr) m_ptr->release();
            m_ptr = node;
        }
    }
    virtual ast_holder_type get_type() const override { return ast_holder_type::Pointer; }

protected:
    ast_node *m_ptr;
};
template <bool Required, class T>
class ast_ptr : public _ast_ptr {
public:
    ast_ptr(T *node = nullptr) : _ast_ptr(node) {}
    ast_ptr(const ast_ptr &other) : _ast_ptr(other.get()) {}
    ast_ptr &operator=(const ast_ptr &other) {
        set(other.get());
        return *this;
    }
    T *get() const { return static_cast<T *>(m_ptr); }
    operator T *() const { return static_cast<T *>(m_ptr); }
    T *operator->() const {
        assert(m_ptr);
        return static_cast<T *>(m_ptr);
    }
    virtual void construct(ast_stack &st) override {
        if (st.empty()) {
            if (!Required) return;
            throw std::logic_error("Invalid AST stack.");
        }
        ast_node *node = st.back();
        if (!ast_ptr::accept(node)) {
            if (!Required) return;
            throw std::logic_error("Invalid AST node.");
        }
        st.pop_back();
        m_ptr = node;
        node->retain();
    }

private:
    virtual bool accept(ast_node *node) override { return node && (std::is_same<ast_node, T>() || id<T>() == node->getId()); }
};
template <bool Required, class... Args>
class ast_sel : public _ast_ptr {
public:
    ast_sel() : _ast_ptr(nullptr) {}
    ast_sel(const ast_sel &other) : _ast_ptr(other.get()) {}
    ast_sel &operator=(const ast_sel &other) {
        set(other.get());
        return *this;
    }
    operator ast_node *() const { return m_ptr; }
    ast_node *operator->() const {
        assert(m_ptr);
        return m_ptr;
    }
    virtual void construct(ast_stack &st) override {
        if (st.empty()) {
            if (!Required) return;
            throw std::logic_error("Invalid AST stack.");
        }
        ast_node *node = st.back();
        if (!ast_sel::accept(node)) {
            if (!Required) return;
            throw std::logic_error("Invalid AST node.");
        }
        st.pop_back();
        m_ptr = node;
        node->retain();
    }

private:
    virtual bool accept(ast_node *node) override {
        if (!node) return false;
        using swallow = bool[];
        bool result = false;
        (void)swallow{result || (result = id<Args>() == node->getId())...};
        return result;
    }
};
class _ast_list : public ast_member {
public:
    ~_ast_list() { clear(); }
    inline ast_node *back() const { return m_objects.back(); }
    inline ast_node *front() const { return m_objects.front(); }
    inline size_t size() const { return m_objects.size(); }
    inline bool empty() const { return m_objects.empty(); }
    void push_back(ast_node *node) {
        assert(node && accept(node));
        m_objects.push_back(node);
        node->retain();
    }
    void push_front(ast_node *node) {
        assert(node && accept(node));
        m_objects.push_front(node);
        node->retain();
    }
    void pop_front() {
        auto node = m_objects.front();
        m_objects.pop_front();
        node->release();
    }
    void pop_back() {
        auto node = m_objects.back();
        m_objects.pop_back();
        node->release();
    }
    bool swap(ast_node *node, ast_node *other) {
        for (auto it = m_objects.begin(); it != m_objects.end(); ++it) {
            if (*it == node) {
                *it = other;
                other->retain();
                node->release();
                return true;
            }
        }
        return false;
    }
    const node_container &objects() const { return m_objects; }
    void clear() {
        for (ast_node *obj : m_objects) {
            if (obj) obj->release();
        }
        m_objects.clear();
    }
    void dup(const _ast_list &src) {
        for (ast_node *obj : src.m_objects) {
            push_back(obj);
        }
    }
    virtual ast_holder_type get_type() const override { return ast_holder_type::List; }

protected:
    node_container m_objects;
};
template <bool Required, class T>
class ast_list : public _ast_list {
public:
    ast_list() {}
    ast_list(const ast_list &other) { dup(other); }
    ast_list &operator=(const ast_list &other) {
        clear();
        dup(other);
        return *this;
    }
    virtual void construct(ast_stack &st) override {
        while (!st.empty()) {
            ast_node *node = st.back();
            if (!ast_list::accept(node)) {
                if (Required && m_objects.empty()) {
                    throw std::logic_error("Invalid AST node.");
                }
                return;
            }
            st.pop_back();
            m_objects.push_front(node);
            node->retain();
        }
        if (Required && m_objects.empty()) {
            throw std::logic_error("Invalid AST stack.");
        }
    }

private:
    virtual bool accept(ast_node *node) override { return node && (std::is_same<ast_node, T>() || id<T>() == node->getId()); }
};
template <bool Required, class... Args>
class ast_sel_list : public _ast_list {
public:
    ast_sel_list() {}
    ast_sel_list(const ast_sel_list &other) { dup(other); }
    ast_sel_list &operator=(const ast_sel_list &other) {
        clear();
        dup(other);
        return *this;
    }
    virtual void construct(ast_stack &st) override {
        while (!st.empty()) {
            ast_node *node = st.back();
            if (!ast_sel_list::accept(node)) {
                if (Required && m_objects.empty()) {
                    throw std::logic_error("Invalid AST node.");
                }
                return;
            }
            st.pop_back();
            m_objects.push_front(node);
            node->retain();
        }
        if (Required && m_objects.empty()) {
            throw std::logic_error("Invalid AST stack.");
        }
    }

private:
    virtual bool accept(ast_node *node) override {
        if (!node) return false;
        using swallow = bool[];
        bool result = false;
        (void)swallow{result || (result = id<Args>() == node->getId())...};
        return result;
    }
};
template <class T>
class ast {
public:
    ast(rule &r) { r.set_parse_proc(&_parse_proc); }

private:
    static void _parse_proc(const pos &b, const pos &e, void *d) {
        ast_stack *st = reinterpret_cast<ast_stack *>(d);
        T *obj = new T;
        obj->m_begin = b;
        obj->m_end = e;
        obj->construct(*st);
        st->push_back(obj);
    }
};
ast_node *parse(input &i, rule &g, error_list &el, void *ud);
}  // namespace parserlib

#pragma endregion ParserLib

#pragma region AST

namespace parserlib {
using namespace std::string_view_literals;
#define AST_LEAF(type)                 \
    COUNTER_INC;                       \
    class type##_t : public ast_node { \
    public:                            \
        virtual int getId() const override { return COUNTER_READ; }
#define AST_NODE(type)                      \
    COUNTER_INC;                            \
    class type##_t : public ast_container { \
    public:                                 \
        virtual int getId() const override { return COUNTER_READ; }
#define AST_MEMBER(type, ...) \
    type##_t() { add_members({__VA_ARGS__}); }
#define AST_END(type, name)                                                  \
    virtual const std::string_view getName() const override { return name; } \
    }                                                                        \
    ;                                                                        \
    template <>                                                              \
    constexpr int id<type##_t>() {                                           \
        return COUNTER_READ;                                                 \
    }
AST_LEAF(Num)
AST_END(Num, "num"sv)
AST_LEAF(Name)
AST_END(Name, "name"sv)
AST_NODE(Variable)
ast_ptr<true, Name_t> name;
AST_MEMBER(Variable, &name)
AST_END(Variable, "variable"sv)
AST_NODE(LabelName)
ast_ptr<true, Name_t> name;
AST_MEMBER(LabelName, &name)
AST_END(LabelName, "label_name"sv)
AST_NODE(LuaKeyword)
ast_ptr<true, Name_t> name;
AST_MEMBER(LuaKeyword, &name)
AST_END(LuaKeyword, "lua_keyword"sv)
AST_LEAF(self)
AST_END(self, "self"sv)
AST_NODE(self_name)
ast_ptr<true, Name_t> name;
AST_MEMBER(self_name, &name)
AST_END(self_name, "self_name"sv)
AST_LEAF(self_class)
AST_END(self_class, "self_name"sv)
AST_NODE(self_class_name)
ast_ptr<true, Name_t> name;
AST_MEMBER(self_class_name, &name)
AST_END(self_class_name, "self_class_name"sv)
AST_NODE(SelfName)
ast_sel<true, self_class_name_t, self_class_t, self_name_t, self_t> name;
AST_MEMBER(SelfName, &name)
AST_END(SelfName, "self_item"sv)
AST_NODE(KeyName)
ast_sel<true, SelfName_t, Name_t> name;
AST_MEMBER(KeyName, &name)
AST_END(KeyName, "key_name"sv)
AST_LEAF(VarArg)
AST_END(VarArg, "var_arg"sv)
AST_LEAF(local_flag)
AST_END(local_flag, "local_flag"sv)
AST_LEAF(Seperator)
AST_END(Seperator, "seperator"sv)
AST_NODE(NameList)
ast_ptr<true, Seperator_t> sep;
ast_list<true, Variable_t> names;
AST_MEMBER(NameList, &sep, &names)
AST_END(NameList, "name_list"sv)
class ExpListLow_t;
class TableBlock_t;
class Attrib_t;
AST_NODE(local_values)
ast_ptr<true, NameList_t> nameList;
ast_sel<false, TableBlock_t, ExpListLow_t> valueList;
AST_MEMBER(local_values, &nameList, &valueList)
AST_END(local_values, "local_values"sv)
AST_NODE(Local)
ast_sel<true, local_flag_t, local_values_t> item;
std::list<std::string> forceDecls;
std::list<std::string> decls;
bool collected = false;
bool defined = false;
AST_MEMBER(Local, &item)
AST_END(Local, "local"sv)
AST_LEAF(const_attrib)
AST_END(const_attrib, "const"sv)
AST_LEAF(close_attrib)
AST_END(close_attrib, "close"sv)
class simple_table_t;
class TableLit_t;
class Assign_t;
AST_NODE(LocalAttrib)
ast_sel<true, const_attrib_t, close_attrib_t> attrib;
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, Variable_t, simple_table_t, TableLit_t> leftList;
ast_ptr<true, Assign_t> assign;
AST_MEMBER(LocalAttrib, &attrib, &sep, &leftList, &assign)
AST_END(LocalAttrib, "local_attrib"sv)
AST_NODE(colon_import_name)
ast_ptr<true, Variable_t> name;
AST_MEMBER(colon_import_name, &name)
AST_END(colon_import_name, "colon_import_name"sv)
AST_LEAF(import_literal_inner)
AST_END(import_literal_inner, "import_literal_inner"sv)
AST_NODE(ImportLiteral)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, import_literal_inner_t> inners;
AST_MEMBER(ImportLiteral, &sep, &inners)
AST_END(ImportLiteral, "import_literal"sv)
class Exp_t;
AST_NODE(ImportFrom)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, colon_import_name_t, Variable_t> names;
ast_ptr<true, Exp_t> exp;
AST_MEMBER(ImportFrom, &sep, &names, &exp)
AST_END(ImportFrom, "import_from"sv)
class MacroName_t;
AST_NODE(macro_name_pair)
ast_ptr<true, MacroName_t> key;
ast_ptr<true, MacroName_t> value;
AST_MEMBER(macro_name_pair, &key, &value)
AST_END(macro_name_pair, "macro_name_pair"sv)
AST_LEAF(import_all_macro)
AST_END(import_all_macro, "import_all_macro"sv)
class variable_pair_t;
class normal_pair_t;
class meta_variable_pair_t;
class meta_normal_pair_t;
AST_NODE(ImportTabLit)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<false, variable_pair_t, normal_pair_t, MacroName_t, macro_name_pair_t, import_all_macro_t, Exp_t, meta_variable_pair_t, meta_normal_pair_t> items;
AST_MEMBER(ImportTabLit, &sep, &items)
AST_END(ImportTabLit, "import_tab_lit"sv)
AST_NODE(ImportAs)
ast_ptr<true, ImportLiteral_t> literal;
ast_sel<false, Variable_t, ImportTabLit_t, import_all_macro_t> target;
AST_MEMBER(ImportAs, &literal, &target)
AST_END(ImportAs, "import_as"sv)
AST_NODE(Import)
ast_sel<true, ImportAs_t, ImportFrom_t> content;
AST_MEMBER(Import, &content)
AST_END(Import, "import"sv)
AST_NODE(Label)
ast_ptr<true, LabelName_t> label;
AST_MEMBER(Label, &label)
AST_END(Label, "label"sv)
AST_NODE(Goto)
ast_ptr<true, LabelName_t> label;
AST_MEMBER(Goto, &label)
AST_END(Goto, "goto"sv)
AST_NODE(ShortTabAppending)
ast_ptr<true, Assign_t> assign;
AST_MEMBER(ShortTabAppending, &assign)
AST_END(ShortTabAppending, "short_table_appending"sv)
class FnArgsDef_t;
AST_LEAF(fn_arrow_back)
AST_END(fn_arrow_back, "fn_arrow_back"sv)
class ChainValue_t;
AST_NODE(Backcall)
ast_ptr<false, FnArgsDef_t> argsDef;
ast_ptr<true, fn_arrow_back_t> arrow;
ast_ptr<true, ChainValue_t> value;
AST_MEMBER(Backcall, &argsDef, &arrow, &value)
AST_END(Backcall, "backcall"sv)
AST_NODE(ExpListLow)
ast_ptr<true, Seperator_t> sep;
ast_list<true, Exp_t> exprs;
AST_MEMBER(ExpListLow, &sep, &exprs)
AST_END(ExpListLow, "exp_list_low"sv)
AST_NODE(ExpList)
ast_ptr<true, Seperator_t> sep;
ast_list<true, Exp_t> exprs;
AST_MEMBER(ExpList, &sep, &exprs)
AST_END(ExpList, "exp_list"sv)
class TableBlock_t;
AST_NODE(Return)
bool allowBlockMacroReturn = false;
ast_sel<false, TableBlock_t, ExpListLow_t> valueList;
AST_MEMBER(Return, &valueList)
AST_END(Return, "return"sv)
class existential_op_t;
class Assign_t;
class Block_t;
class Statement_t;
AST_NODE(With)
ast_ptr<false, existential_op_t> eop;
ast_ptr<true, ExpList_t> valueList;
ast_ptr<false, Assign_t> assigns;
ast_sel<true, Block_t, Statement_t> body;
AST_MEMBER(With, &eop, &valueList, &assigns, &body)
AST_END(With, "with"sv)
AST_NODE(SwitchList)
ast_ptr<true, Seperator_t> sep;
ast_list<true, Exp_t> exprs;
AST_MEMBER(SwitchList, &sep, &exprs)
AST_END(SwitchList, "switch_list"sv)
AST_NODE(SwitchCase)
ast_ptr<true, SwitchList_t> valueList;
ast_sel<true, Block_t, Statement_t> body;
AST_MEMBER(SwitchCase, &valueList, &body)
AST_END(SwitchCase, "switch_case"sv)
AST_NODE(Switch)
ast_ptr<true, Exp_t> target;
ast_ptr<true, Seperator_t> sep;
ast_list<true, SwitchCase_t> branches;
ast_sel<false, Block_t, Statement_t> lastBranch;
AST_MEMBER(Switch, &target, &sep, &branches, &lastBranch)
AST_END(Switch, "switch"sv)
AST_NODE(assignment)
ast_ptr<true, ExpList_t> expList;
ast_ptr<true, Assign_t> assign;
AST_MEMBER(assignment, &expList, &assign)
AST_END(assignment, "assignment"sv)
AST_NODE(IfCond)
ast_sel<true, Exp_t, assignment_t> condition;
AST_MEMBER(IfCond, &condition)
AST_END(IfCond, "if_cond"sv)
AST_LEAF(IfType)
AST_END(IfType, "if_type"sv)
AST_NODE(If)
ast_ptr<true, IfType_t> type;
ast_sel_list<true, IfCond_t, Block_t, Statement_t> nodes;
AST_MEMBER(If, &type, &nodes)
AST_END(If, "if"sv)
AST_LEAF(WhileType)
AST_END(WhileType, "while_type"sv)
AST_NODE(While)
ast_ptr<true, WhileType_t> type;
ast_ptr<true, Exp_t> condition;
ast_sel<true, Block_t, Statement_t> body;
AST_MEMBER(While, &type, &condition, &body)
AST_END(While, "while"sv)
class Body_t;
AST_NODE(Repeat)
ast_ptr<true, Body_t> body;
ast_ptr<true, Exp_t> condition;
AST_MEMBER(Repeat, &body, &condition)
AST_END(Repeat, "repeat"sv)
AST_NODE(for_step_value)
ast_ptr<true, Exp_t> value;
AST_MEMBER(for_step_value, &value)
AST_END(for_step_value, "for_step_value"sv)
AST_NODE(For)
ast_ptr<true, Variable_t> varName;
ast_ptr<true, Exp_t> startValue;
ast_ptr<true, Exp_t> stopValue;
ast_ptr<false, for_step_value_t> stepValue;
ast_sel<true, Block_t, Statement_t> body;
AST_MEMBER(For, &varName, &startValue, &stopValue, &stepValue, &body)
AST_END(For, "for"sv)
class AssignableNameList_t;
class star_exp_t;
AST_NODE(ForEach)
ast_ptr<true, AssignableNameList_t> nameList;
ast_sel<true, star_exp_t, ExpList_t> loopValue;
ast_sel<true, Block_t, Statement_t> body;
AST_MEMBER(ForEach, &nameList, &loopValue, &body)
AST_END(ForEach, "for_each"sv)
AST_NODE(Do)
ast_ptr<true, Body_t> body;
AST_MEMBER(Do, &body)
AST_END(Do, "do"sv)
AST_NODE(catch_block)
ast_ptr<true, Variable_t> err;
ast_ptr<true, Block_t> body;
AST_MEMBER(catch_block, &err, &body)
AST_END(catch_block, "catch_block"sv)
AST_NODE(Try)
ast_sel<true, Block_t, Exp_t> func;
ast_ptr<false, catch_block_t> catchBlock;
AST_MEMBER(Try, &func, &catchBlock)
AST_END(Try, "try"sv)
class CompInner_t;
AST_NODE(Comprehension)
ast_sel<true, Exp_t, Statement_t> value;
ast_ptr<true, CompInner_t> forLoop;
AST_MEMBER(Comprehension, &value, &forLoop)
AST_END(Comprehension, "comp"sv)
AST_NODE(comp_value)
ast_ptr<true, Exp_t> value;
AST_MEMBER(comp_value, &value)
AST_END(comp_value, "comp_value"sv)
AST_NODE(TblComprehension)
ast_ptr<true, Exp_t> key;
ast_ptr<false, comp_value_t> value;
ast_ptr<true, CompInner_t> forLoop;
AST_MEMBER(TblComprehension, &key, &value, &forLoop)
AST_END(TblComprehension, "tbl_comp"sv)
AST_NODE(star_exp)
ast_ptr<true, Exp_t> value;
AST_MEMBER(star_exp, &value)
AST_END(star_exp, "star_exp"sv)
AST_NODE(CompForEach)
ast_ptr<true, AssignableNameList_t> nameList;
ast_sel<true, star_exp_t, Exp_t> loopValue;
AST_MEMBER(CompForEach, &nameList, &loopValue)
AST_END(CompForEach, "comp_for_each"sv)
AST_NODE(CompFor)
ast_ptr<true, Variable_t> varName;
ast_ptr<true, Exp_t> startValue;
ast_ptr<true, Exp_t> stopValue;
ast_ptr<false, for_step_value_t> stepValue;
AST_MEMBER(CompFor, &varName, &startValue, &stopValue, &stepValue)
AST_END(CompFor, "comp_for"sv)
AST_NODE(CompInner)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, CompFor_t, CompForEach_t, Exp_t> items;
AST_MEMBER(CompInner, &sep, &items)
AST_END(CompInner, "comp_inner"sv)
class TableBlock_t;
AST_NODE(Assign)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, With_t, If_t, Switch_t, TableBlock_t, Exp_t> values;
AST_MEMBER(Assign, &sep, &values)
AST_END(Assign, "assign"sv)
AST_LEAF(update_op)
AST_END(update_op, "update_op"sv)
AST_NODE(Update)
ast_ptr<true, update_op_t> op;
ast_ptr<true, Exp_t> value;
AST_MEMBER(Update, &op, &value)
AST_END(Update, "update"sv)
AST_LEAF(BinaryOperator)
AST_END(BinaryOperator, "binary_op"sv)
AST_LEAF(unary_operator)
AST_END(unary_operator, "unary_op"sv)
class AssignableChain_t;
AST_NODE(Assignable)
ast_sel<true, AssignableChain_t, Variable_t, SelfName_t> item;
AST_MEMBER(Assignable, &item)
AST_END(Assignable, "assignable"sv)
class unary_exp_t;
AST_NODE(exp_op_value)
ast_ptr<true, BinaryOperator_t> op;
ast_list<true, unary_exp_t> pipeExprs;
AST_MEMBER(exp_op_value, &op, &pipeExprs)
AST_END(exp_op_value, "exp_op_value"sv)
AST_NODE(Exp)
ast_ptr<true, Seperator_t> sep;
ast_list<true, unary_exp_t> pipeExprs;
ast_list<false, exp_op_value_t> opValues;
ast_ptr<false, Exp_t> nilCoalesed;
AST_MEMBER(Exp, &sep, &pipeExprs, &opValues, &nilCoalesed)
AST_END(Exp, "exp"sv)
class Parens_t;
class MacroName_t;
AST_NODE(Callable)
ast_sel<true, Variable_t, SelfName_t, VarArg_t, Parens_t, MacroName_t> item;
AST_MEMBER(Callable, &item)
AST_END(Callable, "callable"sv)
AST_NODE(variable_pair)
ast_ptr<true, Variable_t> name;
AST_MEMBER(variable_pair, &name)
AST_END(variable_pair, "variable_pair"sv)
AST_NODE(variable_pair_def)
ast_ptr<true, variable_pair_t> pair;
ast_ptr<false, Exp_t> defVal;
AST_MEMBER(variable_pair_def, &pair, &defVal)
AST_END(variable_pair_def, "variable_pair_def"sv)
class String_t;
AST_NODE(normal_pair)
ast_sel<true, KeyName_t, Exp_t, String_t> key;
ast_sel<true, Exp_t, TableBlock_t> value;
AST_MEMBER(normal_pair, &key, &value)
AST_END(normal_pair, "normal_pair"sv)
AST_NODE(normal_pair_def)
ast_ptr<true, normal_pair_t> pair;
ast_ptr<false, Exp_t> defVal;
AST_MEMBER(normal_pair_def, &pair, &defVal)
AST_END(normal_pair_def, "normal_pair_def"sv)
AST_NODE(normal_def)
ast_ptr<true, Exp_t> item;
ast_ptr<true, Seperator_t> sep;
ast_ptr<false, Exp_t> defVal;
AST_MEMBER(normal_def, &item, &sep, &defVal)
AST_END(normal_def, "normal_def")
AST_NODE(meta_variable_pair)
ast_ptr<true, Variable_t> name;
AST_MEMBER(meta_variable_pair, &name)
AST_END(meta_variable_pair, "meta_variable_pair"sv)
AST_NODE(meta_variable_pair_def)
ast_ptr<true, meta_variable_pair_t> pair;
ast_ptr<false, Exp_t> defVal;
AST_MEMBER(meta_variable_pair_def, &pair, &defVal)
AST_END(meta_variable_pair_def, "meta_variable_pair_def"sv)
AST_NODE(meta_normal_pair)
ast_sel<false, Name_t, Exp_t, String_t> key;
ast_sel<true, Exp_t, TableBlock_t> value;
AST_MEMBER(meta_normal_pair, &key, &value)
AST_END(meta_normal_pair, "meta_normal_pair"sv)
AST_NODE(meta_normal_pair_def)
ast_ptr<true, meta_normal_pair_t> pair;
ast_ptr<false, Exp_t> defVal;
AST_MEMBER(meta_normal_pair_def, &pair, &defVal)
AST_END(meta_normal_pair_def, "meta_normal_pair_def"sv)
AST_NODE(simple_table)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, variable_pair_t, normal_pair_t, meta_variable_pair_t, meta_normal_pair_t> pairs;
AST_MEMBER(simple_table, &sep, &pairs)
AST_END(simple_table, "simple_table"sv)
class const_value_t;
class ClassDecl_t;
class unary_value_t;
class FunLit_t;
AST_NODE(SimpleValue)
ast_sel<true, const_value_t, If_t, Switch_t, With_t, ClassDecl_t, ForEach_t, For_t, While_t, Do_t, Try_t, unary_value_t, TblComprehension_t, TableLit_t, Comprehension_t, FunLit_t, Num_t> value;
AST_MEMBER(SimpleValue, &value)
AST_END(SimpleValue, "simple_value"sv)
AST_LEAF(LuaStringOpen)
AST_END(LuaStringOpen, "lua_string_open"sv)
AST_LEAF(LuaStringContent)
AST_END(LuaStringContent, "lua_string_content"sv)
AST_LEAF(LuaStringClose)
AST_END(LuaStringClose, "lua_string_close"sv)
AST_NODE(LuaString)
ast_ptr<true, LuaStringOpen_t> open;
ast_ptr<true, LuaStringContent_t> content;
ast_ptr<true, LuaStringClose_t> close;
AST_MEMBER(LuaString, &open, &content, &close)
AST_END(LuaString, "lua_string"sv)
AST_LEAF(SingleString)
AST_END(SingleString, "single_string"sv)
AST_LEAF(double_string_inner)
AST_END(double_string_inner, "double_string_inner"sv)
AST_NODE(double_string_content)
ast_sel<true, double_string_inner_t, Exp_t> content;
AST_MEMBER(double_string_content, &content)
AST_END(double_string_content, "double_string_content"sv)
AST_NODE(DoubleString)
ast_ptr<true, Seperator_t> sep;
ast_list<false, double_string_content_t> segments;
AST_MEMBER(DoubleString, &sep, &segments)
AST_END(DoubleString, "double_string"sv)
AST_NODE(String)
ast_sel<true, DoubleString_t, SingleString_t, LuaString_t> str;
AST_MEMBER(String, &str)
AST_END(String, "string"sv)
AST_LEAF(Metatable)
AST_END(Metatable, "metatable"sv)
AST_NODE(Metamethod)
ast_sel<true, Name_t, Exp_t, String_t> item;
AST_MEMBER(Metamethod, &item)
AST_END(Metamethod, "metamethod"sv)
AST_NODE(DotChainItem)
ast_sel<true, Name_t, Metatable_t, Metamethod_t> name;
AST_MEMBER(DotChainItem, &name)
AST_END(DotChainItem, "dot_chain_item"sv)
AST_NODE(ColonChainItem)
ast_sel<true, LuaKeyword_t, Name_t, Metamethod_t> name;
bool switchToDot = false;
AST_MEMBER(ColonChainItem, &name)
AST_END(ColonChainItem, "colon_chain_item"sv)
class default_value_t;
AST_NODE(Slice)
ast_sel<true, Exp_t, default_value_t> startValue;
ast_sel<true, Exp_t, default_value_t> stopValue;
ast_sel<true, Exp_t, default_value_t> stepValue;
AST_MEMBER(Slice, &startValue, &stopValue, &stepValue)
AST_END(Slice, "slice"sv)
AST_NODE(Parens)
ast_ptr<true, Exp_t> expr;
AST_MEMBER(Parens, &expr)
AST_END(Parens, "parens"sv)
AST_NODE(Invoke)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<false, Exp_t, SingleString_t, DoubleString_t, LuaString_t, TableLit_t> args;
AST_MEMBER(Invoke, &sep, &args)
AST_END(Invoke, "invoke"sv)
AST_LEAF(existential_op)
AST_END(existential_op, "existential_op"sv)
AST_LEAF(table_appending_op)
AST_END(table_appending_op, "table_appending_op"sv)
class InvokeArgs_t;
AST_NODE(ChainValue)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, Callable_t, Invoke_t, DotChainItem_t, ColonChainItem_t, Slice_t, Exp_t, String_t, InvokeArgs_t, existential_op_t, table_appending_op_t> items;
AST_MEMBER(ChainValue, &sep, &items)
AST_END(ChainValue, "chain_value"sv)
AST_NODE(AssignableChain)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, Callable_t, Invoke_t, DotChainItem_t, ColonChainItem_t, Exp_t, String_t> items;
AST_MEMBER(AssignableChain, &sep, &items)
AST_END(AssignableChain, "assignable_chain"sv)
AST_NODE(Value)
ast_sel<true, SimpleValue_t, simple_table_t, ChainValue_t, String_t> item;
AST_MEMBER(Value, &item)
AST_END(Value, "value"sv)
AST_LEAF(default_value)
AST_END(default_value, "default_value"sv)
AST_NODE(SpreadExp)
ast_ptr<true, Exp_t> exp;
AST_MEMBER(SpreadExp, &exp)
AST_END(SpreadExp, "spread_exp"sv)
class TableBlockIndent_t;
AST_NODE(TableLit)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<false, variable_pair_def_t, normal_pair_def_t, SpreadExp_t, normal_def_t, meta_variable_pair_def_t, meta_normal_pair_def_t, variable_pair_t, normal_pair_t, Exp_t, TableBlockIndent_t,
             meta_variable_pair_t, meta_normal_pair_t>
        values;
AST_MEMBER(TableLit, &sep, &values)
AST_END(TableLit, "table_lit"sv)
AST_NODE(TableBlockIndent)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<false, variable_pair_t, normal_pair_t, Exp_t, TableBlockIndent_t, meta_variable_pair_t, meta_normal_pair_t> values;
AST_MEMBER(TableBlockIndent, &sep, &values)
AST_END(TableBlockIndent, "table_block_indent"sv)
AST_NODE(TableBlock)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<false, variable_pair_t, normal_pair_t, TableBlockIndent_t, Exp_t, TableBlock_t, SpreadExp_t, meta_variable_pair_t, meta_normal_pair_t> values;
AST_MEMBER(TableBlock, &sep, &values)
AST_END(TableBlock, "table_block"sv)
AST_NODE(class_member_list)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, variable_pair_t, normal_pair_t, meta_variable_pair_t, meta_normal_pair_t> values;
AST_MEMBER(class_member_list, &sep, &values)
AST_END(class_member_list, "class_member_list"sv)
AST_NODE(ClassBlock)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, class_member_list_t, Statement_t> contents;
AST_MEMBER(ClassBlock, &sep, &contents)
AST_END(ClassBlock, "class_block"sv)
AST_NODE(ClassDecl)
ast_ptr<false, Assignable_t> name;
ast_ptr<false, Exp_t> extend;
ast_ptr<false, ExpList_t> mixes;
ast_ptr<false, ClassBlock_t> body;
AST_MEMBER(ClassDecl, &name, &extend, &mixes, &body)
AST_END(ClassDecl, "class_decl"sv)
AST_NODE(global_values)
ast_ptr<true, NameList_t> nameList;
ast_sel<false, TableBlock_t, ExpListLow_t> valueList;
AST_MEMBER(global_values, &nameList, &valueList)
AST_END(global_values, "global_values"sv)
AST_LEAF(global_op)
AST_END(global_op, "global_op"sv)
AST_NODE(Global)
ast_sel<true, ClassDecl_t, global_op_t, global_values_t> item;
AST_MEMBER(Global, &item)
AST_END(Global, "global"sv)
AST_LEAF(export_default)
AST_END(export_default, "export_default"sv)
class Macro_t;
AST_NODE(Export)
ast_ptr<false, export_default_t> def;
ast_sel<true, ExpList_t, Exp_t, Macro_t> target;
ast_ptr<false, Assign_t> assign;
AST_MEMBER(Export, &def, &target, &assign)
AST_END(Export, "export"sv)
AST_NODE(FnArgDef)
ast_sel<true, Variable_t, SelfName_t> name;
ast_ptr<false, existential_op_t> op;
ast_ptr<false, Exp_t> defaultValue;
AST_MEMBER(FnArgDef, &name, &op, &defaultValue)
AST_END(FnArgDef, "fn_arg_def"sv)
AST_NODE(FnArgDefList)
ast_ptr<true, Seperator_t> sep;
ast_list<false, FnArgDef_t> definitions;
ast_ptr<false, VarArg_t> varArg;
AST_MEMBER(FnArgDefList, &sep, &definitions, &varArg)
AST_END(FnArgDefList, "fn_arg_def_list"sv)
AST_NODE(outer_var_shadow)
ast_ptr<false, NameList_t> varList;
AST_MEMBER(outer_var_shadow, &varList)
AST_END(outer_var_shadow, "outer_var_shadow"sv)
AST_NODE(FnArgsDef)
ast_ptr<false, FnArgDefList_t> defList;
ast_ptr<false, outer_var_shadow_t> shadowOption;
AST_MEMBER(FnArgsDef, &defList, &shadowOption)
AST_END(FnArgsDef, "fn_args_def"sv)
AST_LEAF(fn_arrow)
AST_END(fn_arrow, "fn_arrow"sv)
AST_NODE(FunLit)
ast_ptr<false, FnArgsDef_t> argsDef;
ast_ptr<true, fn_arrow_t> arrow;
ast_ptr<false, Body_t> body;
AST_MEMBER(FunLit, &argsDef, &arrow, &body)
AST_END(FunLit, "fun_lit"sv)
AST_NODE(MacroName)
ast_ptr<true, Name_t> name;
AST_MEMBER(MacroName, &name)
AST_END(MacroName, "macro_name"sv)
AST_NODE(MacroLit)
ast_ptr<false, FnArgDefList_t> argsDef;
ast_ptr<true, Body_t> body;
AST_MEMBER(MacroLit, &argsDef, &body)
AST_END(MacroLit, "macro_lit"sv)
AST_NODE(MacroInPlace)
ast_ptr<true, Body_t> body;
AST_MEMBER(MacroInPlace, &body)
AST_END(MacroInPlace, "macro_in_place"sv)
AST_NODE(Macro)
ast_ptr<true, Name_t> name;
ast_ptr<true, MacroLit_t> macroLit;
AST_MEMBER(Macro, &name, &macroLit)
AST_END(Macro, "macro"sv)
AST_NODE(NameOrDestructure)
ast_sel<true, Variable_t, TableLit_t> item;
AST_MEMBER(NameOrDestructure, &item)
AST_END(NameOrDestructure, "name_or_des"sv)
AST_NODE(AssignableNameList)
ast_ptr<true, Seperator_t> sep;
ast_list<true, NameOrDestructure_t> items;
AST_MEMBER(AssignableNameList, &sep, &items)
AST_END(AssignableNameList, "assignable_name_list"sv)
AST_NODE(InvokeArgs)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<true, Exp_t, TableBlock_t> args;
AST_MEMBER(InvokeArgs, &sep, &args)
AST_END(InvokeArgs, "invoke_args"sv)
AST_LEAF(const_value)
AST_END(const_value, "const_value"sv)
AST_NODE(unary_value)
ast_list<true, unary_operator_t> ops;
ast_ptr<true, Value_t> value;
AST_MEMBER(unary_value, &ops, &value)
AST_END(unary_value, "unary_value"sv)
AST_NODE(unary_exp)
ast_list<false, unary_operator_t> ops;
ast_list<true, Value_t> expos;
AST_MEMBER(unary_exp, &ops, &expos)
AST_END(unary_exp, "unary_exp"sv)
AST_NODE(ExpListAssign)
ast_ptr<true, ExpList_t> expList;
ast_sel<false, Update_t, Assign_t> action;
AST_MEMBER(ExpListAssign, &expList, &action)
AST_END(ExpListAssign, "exp_list_assign"sv)
AST_NODE(if_line)
ast_ptr<true, IfType_t> type;
ast_ptr<true, IfCond_t> condition;
AST_MEMBER(if_line, &type, &condition)
AST_END(if_line, "if_line"sv)
AST_NODE(while_line)
ast_ptr<true, WhileType_t> type;
ast_ptr<true, Exp_t> condition;
AST_MEMBER(while_line, &type, &condition)
AST_END(while_line, "while_line"sv)
AST_LEAF(BreakLoop)
AST_END(BreakLoop, "break_loop"sv)
AST_NODE(PipeBody)
ast_ptr<true, Seperator_t> sep;
ast_list<true, unary_exp_t> values;
AST_MEMBER(PipeBody, &sep, &values)
AST_END(PipeBody, "pipe_body"sv)
AST_NODE(statement_appendix)
ast_sel<true, if_line_t, while_line_t, CompInner_t> item;
AST_MEMBER(statement_appendix, &item)
AST_END(statement_appendix, "statement_appendix"sv)
AST_LEAF(statement_sep)
AST_END(statement_sep, "statement_sep"sv)
AST_LEAF(MuLineComment)
AST_END(MuLineComment, "comment"sv)
AST_LEAF(MuMultilineComment)
AST_END(MuMultilineComment, "comment"sv)
AST_NODE(ChainAssign)
ast_ptr<true, Seperator_t> sep;
ast_list<true, Exp_t> exprs;
ast_ptr<true, Assign_t> assign;
AST_MEMBER(ChainAssign, &sep, &exprs, &assign)
AST_END(ChainAssign, "chain_assign")
AST_NODE(Statement)
ast_ptr<true, Seperator_t> sep;
ast_sel_list<false, MuLineComment_t, MuMultilineComment_t> comments;
ast_sel<true, Import_t, While_t, Repeat_t, For_t, ForEach_t, Return_t, Local_t, Global_t, Export_t, Macro_t, MacroInPlace_t, BreakLoop_t, Label_t, Goto_t, ShortTabAppending_t, Backcall_t,
        LocalAttrib_t, PipeBody_t, ExpListAssign_t, ChainAssign_t>
        content;
ast_ptr<false, statement_appendix_t> appendix;
ast_ptr<false, statement_sep_t> needSep;
AST_MEMBER(Statement, &sep, &comments, &content, &appendix, &needSep)
AST_END(Statement, "statement"sv)
class Block_t;
AST_NODE(Body)
ast_sel<true, Block_t, Statement_t> content;
AST_MEMBER(Body, &content)
AST_END(Body, "body"sv)
AST_NODE(Block)
ast_ptr<true, Seperator_t> sep;
ast_list<false, Statement_t> statements;
AST_MEMBER(Block, &sep, &statements)
AST_END(Block, "block"sv)
AST_NODE(BlockEnd)
ast_ptr<true, Block_t> block;
AST_MEMBER(BlockEnd, &block)
AST_END(BlockEnd, "block_end"sv)
AST_NODE(File)
ast_ptr<false, Block_t> block;
AST_MEMBER(File, &block)
AST_END(File, "file"sv)
}

#pragma endregion AST

#pragma region Parser

namespace Meo {
using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace parserlib;
struct ParseInfo {
    ast_ptr<false, ast_node> node;
    std::string error;
    std::unique_ptr<input> codes;
    bool exportDefault = false;
    bool exportMacro = false;
    std::string moduleName;
    std::string errorMessage(std::string_view msg, const input_range *loc) const;
};
class ParserError : public std::logic_error {
public:
    explicit ParserError(const std::string &msg, const pos &begin, const pos &end) : std::logic_error(msg), loc(begin, end) {}
    explicit ParserError(const char *msg, const pos &begin, const pos &end) : std::logic_error(msg), loc(begin, end) {}
    input_range loc;
};
template <typename T>
struct identity {
    typedef T type;
};
#define AST_RULE(type)                \
    rule type;                        \
    ast<type##_t> type##_impl = type; \
    inline rule &getRule(identity<type##_t>) { return type; }
extern std::unordered_set<std::string> LuaKeywords;
extern std::unordered_set<std::string> Keywords;
class MeoParser {
public:
    MeoParser();
    template <class AST>
    ParseInfo parse(std::string_view codes) {
        return parse(codes, getRule<AST>());
    }
    template <class AST>
    bool match(std::string_view codes) {
        auto rEnd = rule(getRule<AST>() >> eof());
        return parse(codes, rEnd).node;
    }
    std::string toString(ast_node *node);
    std::string toString(input::iterator begin, input::iterator end);
    input encode(std::string_view input);
    std::string decode(const input &input);

protected:
    ParseInfo parse(std::string_view codes, rule &r);
    struct State {
        State() { indents.push(0); }
        bool exportDefault = false;
        bool exportMacro = false;
        int exportCount = 0;
        int moduleFix = 0;
        size_t stringOpen = 0;
        std::string moduleName = "_module_0"s;
        std::string buffer;
        std::stack<int> indents;
        std::stack<bool> noDoStack;
        std::stack<bool> noChainBlockStack;
        std::stack<bool> noTableBlockStack;
    };
    template <class T>
    inline rule &getRule() {
        return getRule(identity<T>());
    }

private:
    Converter _converter;
    template <class T>
    inline rule &getRule(identity<T>) {
        assert(false);
        return Cut;
    }
    rule empty_block_error;
    rule leading_spaces_error;
    rule indentation_error;
    rule num_char;
    rule num_char_hex;
    rule num_lit;
    rule num_expo;
    rule num_expo_hex;
    rule lj_num;
    rule plain_space;
    rule Break;
    rule Any;
    rule White;
    rule Stop;
    rule Comment;
    rule multi_line_open;
    rule multi_line_close;
    rule multi_line_content;
    rule MultiLineComment;
    rule Indent;
    rule EscapeNewLine;
    rule space_one;
    rule Space;
    rule SpaceBreak;
    rule EmptyLine;
    rule AlphaNum;
    rule Cut;
    rule check_indent;
    rule CheckIndent;
    rule advance;
    rule Advance;
    rule push_indent;
    rule PushIndent;
    rule PreventIndent;
    rule PopIndent;
    rule InBlock;
    rule ImportName;
    rule ImportNameList;
    rule import_literal_chain;
    rule ImportTabItem;
    rule ImportTabList;
    rule ImportTabLine;
    rule import_tab_lines;
    rule WithExp;
    rule DisableDo;
    rule EnableDo;
    rule DisableChain;
    rule EnableChain;
    rule DisableDoChainArgTableBlock;
    rule EnableDoChainArgTableBlock;
    rule DisableArgTableBlock;
    rule EnableArgTableBlock;
    rule SwitchElse;
    rule SwitchBlock;
    rule IfElseIf;
    rule IfElse;
    rule for_args;
    rule for_in;
    rule CompClause;
    rule Chain;
    rule KeyValue;
    rule single_string_inner;
    rule interp;
    rule double_string_plain;
    rule lua_string_open;
    rule lua_string_close;
    rule FnArgsExpList;
    rule FnArgs;
    rule macro_args_def;
    rule chain_call;
    rule chain_index_chain;
    rule ChainItems;
    rule chain_dot_chain;
    rule ColonChain;
    rule chain_with_colon;
    rule ChainItem;
    rule chain_line;
    rule chain_block;
    rule meta_index;
    rule Index;
    rule invoke_chain;
    rule TableValue;
    rule table_lit_lines;
    rule TableLitLine;
    rule TableValueList;
    rule TableBlockInner;
    rule ClassLine;
    rule KeyValueLine;
    rule KeyValueList;
    rule ArgLine;
    rule ArgBlock;
    rule invoke_args_with_table;
    rule arg_table_block;
    rule PipeOperator;
    rule ExponentialOperator;
    rule pipe_value;
    rule pipe_exp;
    rule expo_value;
    rule expo_exp;
    rule exp_not_tab;
    rule local_const_item;
    rule empty_line_break;
    rule mu_comment;
    rule mu_line_comment;
    rule mu_multiline_comment;
    rule Line;
    rule Shebang;
    AST_RULE(Num)
    AST_RULE(Name)
    AST_RULE(Variable)
    AST_RULE(LabelName)
    AST_RULE(LuaKeyword)
    AST_RULE(self)
    AST_RULE(self_name)
    AST_RULE(self_class)
    AST_RULE(self_class_name)
    AST_RULE(SelfName)
    AST_RULE(KeyName)
    AST_RULE(VarArg)
    AST_RULE(Seperator)
    AST_RULE(NameList)
    AST_RULE(local_flag)
    AST_RULE(local_values)
    AST_RULE(Local)
    AST_RULE(const_attrib)
    AST_RULE(close_attrib)
    AST_RULE(LocalAttrib);
    AST_RULE(colon_import_name)
    AST_RULE(import_literal_inner)
    AST_RULE(ImportLiteral)
    AST_RULE(ImportFrom)
    AST_RULE(macro_name_pair)
    AST_RULE(import_all_macro)
    AST_RULE(ImportTabLit)
    AST_RULE(ImportAs)
    AST_RULE(Import)
    AST_RULE(Label)
    AST_RULE(Goto)
    AST_RULE(ShortTabAppending)
    AST_RULE(fn_arrow_back)
    AST_RULE(Backcall)
    AST_RULE(PipeBody)
    AST_RULE(ExpListLow)
    AST_RULE(ExpList)
    AST_RULE(Return)
    AST_RULE(With)
    AST_RULE(SwitchList)
    AST_RULE(SwitchCase)
    AST_RULE(Switch)
    AST_RULE(assignment)
    AST_RULE(IfCond)
    AST_RULE(IfType)
    AST_RULE(If)
    AST_RULE(WhileType)
    AST_RULE(While)
    AST_RULE(Repeat)
    AST_RULE(for_step_value)
    AST_RULE(For)
    AST_RULE(ForEach)
    AST_RULE(Do)
    AST_RULE(catch_block)
    AST_RULE(Try)
    AST_RULE(Comprehension)
    AST_RULE(comp_value)
    AST_RULE(TblComprehension)
    AST_RULE(star_exp)
    AST_RULE(CompForEach)
    AST_RULE(CompFor)
    AST_RULE(CompInner)
    AST_RULE(Assign)
    AST_RULE(update_op)
    AST_RULE(Update)
    AST_RULE(BinaryOperator)
    AST_RULE(unary_operator)
    AST_RULE(Assignable)
    AST_RULE(AssignableChain)
    AST_RULE(exp_op_value)
    AST_RULE(Exp)
    AST_RULE(Callable)
    AST_RULE(ChainValue)
    AST_RULE(simple_table)
    AST_RULE(SimpleValue)
    AST_RULE(Value)
    AST_RULE(LuaStringOpen);
    AST_RULE(LuaStringContent);
    AST_RULE(LuaStringClose);
    AST_RULE(LuaString)
    AST_RULE(SingleString)
    AST_RULE(double_string_inner)
    AST_RULE(double_string_content)
    AST_RULE(DoubleString)
    AST_RULE(String)
    AST_RULE(Parens)
    AST_RULE(DotChainItem)
    AST_RULE(ColonChainItem)
    AST_RULE(Metatable)
    AST_RULE(Metamethod)
    AST_RULE(default_value)
    AST_RULE(Slice)
    AST_RULE(Invoke)
    AST_RULE(existential_op)
    AST_RULE(table_appending_op)
    AST_RULE(SpreadExp)
    AST_RULE(TableLit)
    AST_RULE(TableBlock)
    AST_RULE(TableBlockIndent)
    AST_RULE(class_member_list)
    AST_RULE(ClassBlock)
    AST_RULE(ClassDecl)
    AST_RULE(global_values)
    AST_RULE(global_op)
    AST_RULE(Global)
    AST_RULE(export_default)
    AST_RULE(Export)
    AST_RULE(variable_pair)
    AST_RULE(normal_pair)
    AST_RULE(meta_variable_pair)
    AST_RULE(meta_normal_pair)
    AST_RULE(variable_pair_def)
    AST_RULE(normal_pair_def)
    AST_RULE(normal_def)
    AST_RULE(meta_variable_pair_def)
    AST_RULE(meta_normal_pair_def)
    AST_RULE(FnArgDef)
    AST_RULE(FnArgDefList)
    AST_RULE(outer_var_shadow)
    AST_RULE(FnArgsDef)
    AST_RULE(fn_arrow)
    AST_RULE(FunLit)
    AST_RULE(MacroName)
    AST_RULE(MacroLit)
    AST_RULE(Macro)
    AST_RULE(MacroInPlace)
    AST_RULE(NameOrDestructure)
    AST_RULE(AssignableNameList)
    AST_RULE(InvokeArgs)
    AST_RULE(const_value)
    AST_RULE(unary_value)
    AST_RULE(unary_exp)
    AST_RULE(ExpListAssign)
    AST_RULE(if_line)
    AST_RULE(while_line)
    AST_RULE(BreakLoop)
    AST_RULE(statement_appendix)
    AST_RULE(statement_sep)
    AST_RULE(Statement)
    AST_RULE(MuLineComment)
    AST_RULE(MuMultilineComment)
    AST_RULE(ChainAssign)
    AST_RULE(Body)
    AST_RULE(Block)
    AST_RULE(BlockEnd)
    AST_RULE(File)
};

namespace Utils {
void replace(std::string &str, std::string_view from, std::string_view to);
void trim(std::string &str);
}  // namespace Utils

}  // namespace Meo

#pragma endregion Parser

#pragma region Compiler

namespace Meo {
extern const std::string_view extension;
using Options = std::unordered_map<std::string, std::string>;
struct MuConfig {
    bool lintGlobalVariable = false;
    bool implicitReturnRoot = true;
    bool reserveLineNumber = true;
    bool useSpaceOverTab = false;
    bool exporting = false;
    bool profiling = false;
    int lineOffset = 0;
    std::string module;
    Options options;
};
struct GlobalVar {
    std::string name;
    int line;
    int col;
};
using GlobalVars = std::vector<GlobalVar>;
struct CompileInfo {
    std::string codes;
    std::string error;
    std::unique_ptr<GlobalVars> globals;
    std::unique_ptr<Options> options;
    double parseTime;
    double compileTime;
};
class MeoCompilerImpl;
class MeoCompiler {
public:
    MeoCompiler(void *luaState = nullptr, const std::function<void(void *)> &luaOpen = nullptr, bool sameModule = false);
    virtual ~MeoCompiler();
    CompileInfo compile(std::string_view codes, const MuConfig &config = {});

private:
    std::unique_ptr<MeoCompilerImpl> _compiler;
};
}  // namespace Meo

#pragma endregion Compiler

#endif