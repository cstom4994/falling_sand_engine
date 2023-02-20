// Metadot Code Copyright(c) 2022-2023, KaoruXun All rights reserved.

// https://github.com/pigpigyyy/Yuescript
// https://github.com/axilmar/parserlib

#pragma once

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

#pragma region AST

namespace parserlib {
/// type of the parser's input.
typedef std::basic_string<wchar_t> input;
typedef input::iterator input_it;
typedef std::wstring_convert<std::codecvt_utf8_utf16<input::value_type>> Converter;
class _private;
class _expr;
class _context;
class rule;
/// position into the input.
class pos {
public:
    /// interator into the input.
    input::iterator m_it;
    /// line.
    int m_line;
    /// column.
    int m_col;
    /// null constructor.
    pos() : m_line(0), m_col(0) {}
    /** constructor from input.
        @param i input.
    */
    pos(input& i);
};
struct item_t {
    pos* begin;
    pos* end;
    void* user_data;
};
typedef std::function<bool(const item_t&)> user_handler;
/** a grammar expression.
 */
class expr {
public:
    /** character terminal constructor.
        @param c character.
    */
    expr(char c);
    /** null-terminated string terminal constructor.
        @param s null-terminated string.
    */
    expr(const char* s);
    /** rule reference constructor.
        @param r rule.
    */
    expr(rule& r);
    /** creates a zero-or-more loop out of this expression.
        @return a zero-or-more loop expression.
    */
    expr operator*() const;
    /** creates a one-or-more loop out of this expression.
        @return a one-or-more loop expression.
    */
    expr operator+() const;
    /** creates an optional out of this expression.
        @return an optional expression.
    */
    expr operator-() const;
    /** creates an AND-expression.
        @return an AND-expression.
    */
    expr operator&() const;
    /** creates a NOT-expression.
        @return a NOT-expression.
    */
    expr operator!() const;

private:
    // internal expression
    _expr* m_expr;
    // internal constructor from internal expression
    expr(_expr* e) : m_expr(e) {}
    // assignment not allowed
    expr& operator=(expr&);
    friend class _private;
};
/** type of procedure to invoke when a rule is successfully parsed.
    @param b begin position of input.
    @param e end position of input.
    @param d pointer to user data.
*/
typedef void (*parse_proc)(const pos& b, const pos& e, void* d);
/// input range.
class input_range {
public:
    virtual ~input_range() {}
    /// begin position.
    pos m_begin;
    /// end position.
    pos m_end;
    /// empty constructor.
    input_range() {}
    /** constructor.
        @param b begin position.
        @param e end position.
    */
    input_range(const pos& b, const pos& e);
};
/// enum with error types.
enum ERROR_TYPE {
    /// syntax error
    ERROR_SYNTAX_ERROR = 1,
    /// invalid end of file
    ERROR_INVALID_EOF,
    /// first user error
    ERROR_USER = 100
};
/// error.
class error : public input_range {
public:
    /// type
    int m_type;
    /** constructor.
        @param b begin position.
        @param e end position.
        @param t type.
    */
    error(const pos& b, const pos& e, int t);
    /** compare on begin position.
        @param e the other error to compare this with.
        @return true if this comes before the previous error, false otherwise.
    */
    bool operator<(const error& e) const;
};
/// type of error list.
typedef std::list<error> error_list;
/** represents a rule.
 */
class rule {
public:
    /** character terminal constructor.
        @param c character.
    */
    rule();
    rule(char c);
    /** null-terminated string terminal constructor.
        @param s null-terminated string.
    */
    rule(const char* s);
    /** constructor from expression.
        @param e expression.
    */
    rule(const expr& e);
    /** constructor from rule.
        @param r rule.
    */
    rule(rule& r);
    /** invalid constructor from rule (required by gcc).
        @param r rule.
        @exception std::logic_error always thrown.
    */
    rule(const rule& r);
    /** deletes the internal object that represents the expression.
     */
    ~rule();
    /** creates a zero-or-more loop out of this rule.
        @return a zero-or-more loop rule.
    */
    expr operator*();
    /** creates a one-or-more loop out of this rule.
        @return a one-or-more loop rule.
    */
    expr operator+();
    /** creates an optional out of this rule.
        @return an optional rule.
    */
    expr operator-();
    /** creates an AND-expression out of this rule.
        @return an AND-expression out of this rule.
    */
    expr operator&();
    /** creates a NOT-expression out of this rule.
        @return a NOT-expression out of this rule.
    */
    expr operator!();
    /** sets the parse procedure.
        @param p procedure.
    */
    void set_parse_proc(parse_proc p);
    /** get the this ptr (since operator & is overloaded).
        @return pointer to this.
    */
    rule* this_ptr() { return this; }
    rule& operator=(rule&);
    rule& operator=(const expr&);
#ifndef NDEBUG
    /** constructor from a expression name.
        @param name name of expression.
    */
    struct initTag {};
    rule(const char* name, initTag);
    const char* get_name() const { return m_name; }
#endif  // NDEBUG
private:
    // mode
    enum _MODE { _PARSE, _REJECT, _ACCEPT };
    // state
    struct _state {
        // position in source code, relative to start
        size_t m_pos;
        // mode
        _MODE m_mode;
        // constructor
        _state(size_t pos = -1, _MODE mode = _PARSE) : m_pos(pos), m_mode(mode) {}
    };
    // internal expression
    _expr* m_expr;
#ifndef NDEBUG
    const char* m_name = nullptr;
#endif
    // associated parse procedure.
    parse_proc m_parse_proc;
    // state
    _state m_state;
    friend class _private;
    friend class _context;
};
/** creates a sequence of expressions.
    @param left left operand.
    @param right right operand.
    @return an expression which parses a sequence.
*/
expr operator>>(const expr& left, const expr& right);
expr operator>>(expr&& left, expr&& right);
/** creates a choice of expressions.
    @param left left operand.
    @param right right operand.
    @return an expression which parses a choice.
*/
expr operator|(const expr& left, const expr& right);
expr operator|(expr&& left, expr&& right);
/** converts a parser expression into a terminal.
    @param e expression.
    @return an expression which parses a terminal.
*/
expr term(const expr& e);
/** creates a set expression from a null-terminated string.
    @param s null-terminated string with characters of the set.
    @return an expression which parses a single character out of a set.
*/
expr set(const char* s);
/** creates a range expression.
    @param min min character.
    @param max max character.
    @return an expression which parses a single character out of range.
*/
expr range(int min, int max);
/** creates an expression which increments the line counter
    and resets the column counter when the given expression
    is parsed successfully; used for newline characters.
    @param e expression to wrap into a newline parser.
    @return an expression that handles newlines.
*/
expr nl(const expr& e);
/** creates an expression which tests for the end of input.
    @return an expression that handles the end of input.
*/
expr eof();
/** creates a not expression.
    @param e expression.
    @return the appropriate expression.
*/
expr not_(const expr& e);
/** creates an and expression.
    @param e expression.
    @return the appropriate expression.
*/
expr and_(const expr& e);
/** creates an expression that parses any character.
    @return the appropriate expression.
*/
expr any();
/** parsing succeeds without consuming any input.
 */
expr true_();
/** parsing fails without consuming any input.
 */
expr false_();
/** parse with target expression and let user handle result.
 */
expr user(const expr& e, const user_handler& handler);
/** parses the given input.
    The parse procedures of each rule parsed are executed
    before this function returns, if parsing succeeds.
    @param i input.
    @param g root rule of grammar.
    @param el list of errors.
    @param d user data, passed to the parse procedures.
    @return true on parsing success, false on failure.
*/
bool parse(input& i, rule& g, error_list& el, void* d, void* ud);
/** output the specific input range to the specific stream.
    @param stream stream.
    @param ir input range.
    @return the stream.
*/
template <class T>
T& operator<<(T& stream, const input_range& ir) {
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
/** type of AST node stack.
 */
typedef std::vector<ast_node*> ast_stack;
typedef std::list<ast_node*> node_container;
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
/** Base class for AST nodes.
 */
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
    /** interface for filling the contents of the node
        from a node stack.
        @param st stack.
    */
    virtual void construct(ast_stack&) {}
    /** interface for visiting AST tree use.
     */
    virtual traversal traverse(const std::function<traversal(ast_node*)>& func);
    template <typename... Ts>
    struct select_last {
        using type = typename decltype((std::enable_if<true, Ts>{}, ...))::type;
    };
    template <typename... Ts>
    using select_last_t = typename select_last<Ts...>::type;
    template <class... Args>
    select_last_t<Args...>* getByPath() {
        int types[] = {id<Args>()...};
        return static_cast<select_last_t<Args...>*>(getByTypeIds(std::begin(types), std::end(types)));
    }
    virtual bool visitChild(const std::function<bool(ast_node*)>& func);
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
    ast_node* getByTypeIds(int* begin, int* end);
};
template <class T>
constexpr typename std::enable_if<std::is_base_of<ast_node, T>::value, int>::type id() {
    return 0;
}
template <class T>
T* ast_cast(ast_node* node) {
    return node && id<T>() == node->getId() ? static_cast<T*>(node) : nullptr;
}
template <class T>
T* ast_to(ast_node* node) {
    assert(node->getId() == id<T>());
    return static_cast<T*>(node);
}
template <class... Args>
bool ast_is(ast_node* node) {
    if (!node) return false;
    bool result = false;
    int i = node->getId();
    using swallow = bool[];
    (void)swallow{result || (result = id<Args>() == i)...};
    return result;
}
class ast_member;
/** type of ast member vector.
 */
typedef std::vector<ast_member*> ast_member_vector;
/** base class for AST nodes with children.
 */
class ast_container : public ast_node {
public:
    void add_members(std::initializer_list<ast_member*> members) {
        for (auto member : members) {
            m_members.push_back(member);
        }
    }
    /** returns the vector of AST members.
        @return the vector of AST members.
    */
    const ast_member_vector& members() const { return m_members; }
    /** Asks all members to construct themselves from the stack.
        The members are asked to construct themselves in reverse order.
        from a node stack.
        @param st stack.
    */
    virtual void construct(ast_stack& st) override;
    virtual traversal traverse(const std::function<traversal(ast_node*)>& func) override;
    virtual bool visitChild(const std::function<bool(ast_node*)>& func) override;

private:
    ast_member_vector m_members;
    friend class ast_member;
};
enum class ast_holder_type { Pointer, List };
/** Base class for children of ast_container.
 */
class ast_member {
public:
    virtual ~ast_member() {}
    /** interface for filling the the member from a node stack.
        @param st stack.
    */
    virtual void construct(ast_stack& st) = 0;
    virtual bool accept(ast_node* node) = 0;
    virtual ast_holder_type get_type() const = 0;
};
class _ast_ptr : public ast_member {
public:
    _ast_ptr(ast_node* node) : m_ptr(node) {
        if (node) node->retain();
    }
    virtual ~_ast_ptr() {
        if (m_ptr) {
            m_ptr->release();
            m_ptr = nullptr;
        }
    }
    ast_node* get() const { return m_ptr; }
    template <class T>
    T* as() const {
        return ast_cast<T>(m_ptr);
    }
    template <class T>
    T* to() const {
        assert(m_ptr && m_ptr->getId() == id<T>());
        return static_cast<T*>(m_ptr);
    }
    template <class T>
    bool is() const {
        return m_ptr && m_ptr->getId() == id<T>();
    }
    void set(ast_node* node) {
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
    ast_node* m_ptr;
};
/** pointer to an AST object.
    It assumes ownership of the object.
    It pops an object of the given type from the stack.
    @tparam Required if true, the object is required.
    @tparam T type of object to control.
*/
template <bool Required, class T>
class ast_ptr : public _ast_ptr {
public:
    ast_ptr(T* node = nullptr) : _ast_ptr(node) {}
    ast_ptr(const ast_ptr& other) : _ast_ptr(other.get()) {}
    ast_ptr& operator=(const ast_ptr& other) {
        set(other.get());
        return *this;
    }
    /** gets the underlying ptr value.
        @return the underlying ptr value.
    */
    T* get() const { return static_cast<T*>(m_ptr); }
    /** auto conversion to the underlying object ptr.
        @return the underlying ptr value.
    */
    operator T*() const { return static_cast<T*>(m_ptr); }
    /** member access.
        @return the underlying ptr value.
    */
    T* operator->() const {
        assert(m_ptr);
        return static_cast<T*>(m_ptr);
    }
    /** Pops a node from the stack.
        @param st stack.
        @exception std::logic_error thrown if the node is not of the appropriate type;
            thrown only if Required == true or if the stack is empty.
    */
    virtual void construct(ast_stack& st) override {
        // check the stack node
        if (st.empty()) {
            if (!Required) return;
            throw std::logic_error("Invalid AST stack.");
        }
        ast_node* node = st.back();
        if (!ast_ptr::accept(node)) {
            // if the object is not required, simply return
            if (!Required) return;
            // else if the object is mandatory, throw an exception
            throw std::logic_error("Invalid AST node.");
        }
        st.pop_back();
        m_ptr = node;
        node->retain();
    }

private:
    virtual bool accept(ast_node* node) override { return node && (std::is_same<ast_node, T>() || id<T>() == node->getId()); }
};
template <bool Required, class... Args>
class ast_sel : public _ast_ptr {
public:
    ast_sel() : _ast_ptr(nullptr) {}
    ast_sel(const ast_sel& other) : _ast_ptr(other.get()) {}
    ast_sel& operator=(const ast_sel& other) {
        set(other.get());
        return *this;
    }
    operator ast_node*() const { return m_ptr; }
    ast_node* operator->() const {
        assert(m_ptr);
        return m_ptr;
    }
    virtual void construct(ast_stack& st) override {
        if (st.empty()) {
            if (!Required) return;
            throw std::logic_error("Invalid AST stack.");
        }
        ast_node* node = st.back();
        if (!ast_sel::accept(node)) {
            if (!Required) return;
            throw std::logic_error("Invalid AST node.");
        }
        st.pop_back();
        m_ptr = node;
        node->retain();
    }

private:
    virtual bool accept(ast_node* node) override {
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
    inline ast_node* back() const { return m_objects.back(); }
    inline ast_node* front() const { return m_objects.front(); }
    inline size_t size() const { return m_objects.size(); }
    inline bool empty() const { return m_objects.empty(); }
    void push_back(ast_node* node) {
        assert(node && accept(node));
        m_objects.push_back(node);
        node->retain();
    }
    void push_front(ast_node* node) {
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
    bool swap(ast_node* node, ast_node* other) {
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
    const node_container& objects() const { return m_objects; }
    void clear() {
        for (ast_node* obj : m_objects) {
            if (obj) obj->release();
        }
        m_objects.clear();
    }
    void dup(const _ast_list& src) {
        for (ast_node* obj : src.m_objects) {
            push_back(obj);
        }
    }
    virtual ast_holder_type get_type() const override { return ast_holder_type::List; }

protected:
    node_container m_objects;
};
/** A list of objects.
    It pops objects of the given type from the ast stack, until no more objects can be popped.
    It assumes ownership of objects.
    @tparam Required if true, the object is required.
    @tparam T type of object to control.
*/
template <bool Required, class T>
class ast_list : public _ast_list {
public:
    ast_list() {}
    ast_list(const ast_list& other) { dup(other); }
    ast_list& operator=(const ast_list& other) {
        clear();
        dup(other);
        return *this;
    }
    /** Pops objects of type T from the stack until no more objects can be popped.
        @param st stack.
    */
    virtual void construct(ast_stack& st) override {
        while (!st.empty()) {
            ast_node* node = st.back();
            // if the object was not not of the appropriate type,
            // end the list parsing
            if (!ast_list::accept(node)) {
                if (Required && m_objects.empty()) {
                    throw std::logic_error("Invalid AST node.");
                }
                return;
            }
            st.pop_back();
            // insert the object in the list, in reverse order
            m_objects.push_front(node);
            node->retain();
        }
        if (Required && m_objects.empty()) {
            throw std::logic_error("Invalid AST stack.");
        }
    }

private:
    virtual bool accept(ast_node* node) override { return node && (std::is_same<ast_node, T>() || id<T>() == node->getId()); }
};
template <bool Required, class... Args>
class ast_sel_list : public _ast_list {
public:
    ast_sel_list() {}
    ast_sel_list(const ast_sel_list& other) { dup(other); }
    ast_sel_list& operator=(const ast_sel_list& other) {
        clear();
        dup(other);
        return *this;
    }
    virtual void construct(ast_stack& st) override {
        while (!st.empty()) {
            ast_node* node = st.back();
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
    virtual bool accept(ast_node* node) override {
        if (!node) return false;
        using swallow = bool[];
        bool result = false;
        (void)swallow{result || (result = id<Args>() == node->getId())...};
        return result;
    }
};
/** AST function which creates an object of type T
    and pushes it to the node stack.
*/
template <class T>
class ast {
public:
    /** constructor.
        @param r rule to attach the AST function to.
    */
    ast(rule& r) { r.set_parse_proc(&_parse_proc); }

private:
    // parse proc
    static void _parse_proc(const pos& b, const pos& e, void* d) {
        ast_stack* st = reinterpret_cast<ast_stack*>(d);
        T* obj = new T;
        obj->m_begin = b;
        obj->m_end = e;
        obj->construct(*st);
        st->push_back(obj);
    }
};
/** parses the given input.
    @param i input.
    @param g root rule of grammar.
    @param el list of errors.
    @param ud user data, passed to the parse procedures.
    @return pointer to ast node created, or null if there was an error.
        The return object must be deleted by the caller.
*/
ast_node* parse(input& i, rule& g, error_list& el, void* ud);
}  // namespace parserlib

#pragma endregion AST

#pragma region MeoAST

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

// clang-format off

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

AST_LEAF(Self)
AST_END(Self, "self"sv)

AST_NODE(SelfName)
	ast_ptr<true, Name_t> name;
	AST_MEMBER(SelfName, &name)
AST_END(SelfName, "self_name"sv)

AST_LEAF(SelfClass)
AST_END(SelfClass, "self_class"sv)

AST_NODE(SelfClassName)
	ast_ptr<true, Name_t> name;
	AST_MEMBER(SelfClassName, &name)
AST_END(SelfClassName, "self_class_name"sv)

AST_NODE(SelfItem)
	ast_sel<true, SelfClassName_t, SelfClass_t, SelfName_t, Self_t> name;
	AST_MEMBER(SelfItem, &name)
AST_END(SelfItem, "self_item"sv)

AST_NODE(KeyName)
	ast_sel<true, SelfItem_t, Name_t> name;
	AST_MEMBER(KeyName, &name)
AST_END(KeyName, "key_name"sv)

AST_LEAF(VarArg)
AST_END(VarArg, "var_arg"sv)

AST_LEAF(LocalFlag)
AST_END(LocalFlag, "local_flag"sv)

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

AST_NODE(LocalValues)
	ast_ptr<true, NameList_t> nameList;
	ast_sel<false, TableBlock_t, ExpListLow_t> valueList;
	AST_MEMBER(LocalValues, &nameList, &valueList)
AST_END(LocalValues, "local_values"sv)

AST_NODE(Local)
	ast_sel<true, LocalFlag_t, LocalValues_t> item;
	std::list<std::string> forceDecls;
	std::list<std::string> decls;
	bool collected = false;
	bool defined = false;
	AST_MEMBER(Local, &item)
AST_END(Local, "local"sv)

AST_LEAF(ConstAttrib)
AST_END(ConstAttrib, "const"sv)

AST_LEAF(CloseAttrib)
AST_END(CloseAttrib, "close"sv)

class SimpleTable_t;
class TableLit_t;
class Assign_t;

AST_NODE(LocalAttrib)
	ast_sel<true, ConstAttrib_t, CloseAttrib_t> attrib;
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, Variable_t, SimpleTable_t, TableLit_t> leftList;
	ast_ptr<true, Assign_t> assign;
	AST_MEMBER(LocalAttrib, &attrib, &sep, &leftList, &assign)
AST_END(LocalAttrib, "local_attrib"sv)

AST_NODE(ColonImportName)
	ast_ptr<true, Variable_t> name;
	AST_MEMBER(ColonImportName, &name)
AST_END(ColonImportName, "colon_import_name"sv)

AST_LEAF(ImportLiteralInner)
AST_END(ImportLiteralInner, "import_literal_inner"sv)

AST_NODE(ImportLiteral)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, ImportLiteralInner_t> inners;
	AST_MEMBER(ImportLiteral, &sep, &inners)
AST_END(ImportLiteral, "import_literal"sv)

class Exp_t;

AST_NODE(ImportFrom)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, ColonImportName_t, Variable_t> names;
	ast_ptr<true, Exp_t> exp;
	AST_MEMBER(ImportFrom, &sep, &names, &exp)
AST_END(ImportFrom, "import_from"sv)

class MacroName_t;

AST_NODE(MacroNamePair)
	ast_ptr<true, MacroName_t> key;
	ast_ptr<true, MacroName_t> value;
	AST_MEMBER(MacroNamePair, &key, &value)
AST_END(MacroNamePair, "macro_name_pair"sv)

AST_LEAF(ImportAllMacro)
AST_END(ImportAllMacro, "import_all_macro"sv)

class VariablePair_t;
class NormalPair_t;
class MetaVariablePair_t;
class MetaNormalPair_t;

AST_NODE(ImportTabLit)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<false, VariablePair_t, NormalPair_t, MacroName_t, MacroNamePair_t, ImportAllMacro_t, Exp_t, MetaVariablePair_t, MetaNormalPair_t> items;
	AST_MEMBER(ImportTabLit, &sep, &items)
AST_END(ImportTabLit, "import_tab_lit"sv)

AST_NODE(ImportAs)
	ast_ptr<true, ImportLiteral_t> literal;
	ast_sel<false, Variable_t, ImportTabLit_t, ImportAllMacro_t> target;
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

AST_LEAF(FnArrowBack)
AST_END(FnArrowBack, "fn_arrow_back"sv)

class ChainValue_t;

AST_NODE(Backcall)
	ast_ptr<false, FnArgsDef_t> argsDef;
	ast_ptr<true, FnArrowBack_t> arrow;
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

class ExistentialOp_t;
class Assign_t;
class Block_t;
class Statement_t;

AST_NODE(With)
	ast_ptr<false, ExistentialOp_t> eop;
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

AST_NODE(Assignment)
	ast_ptr<true, ExpList_t> expList;
	ast_ptr<true, Assign_t> assign;
	AST_MEMBER(Assignment, &expList, &assign)
AST_END(Assignment, "assignment"sv)

AST_NODE(IfCond)
	ast_sel<true, Exp_t, Assignment_t> condition;
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

AST_NODE(ForStepValue)
	ast_ptr<true, Exp_t> value;
	AST_MEMBER(ForStepValue, &value)
AST_END(ForStepValue, "for_step_value"sv)

AST_NODE(For)
	ast_ptr<true, Variable_t> varName;
	ast_ptr<true, Exp_t> startValue;
	ast_ptr<true, Exp_t> stopValue;
	ast_ptr<false, ForStepValue_t> stepValue;
	ast_sel<true, Block_t, Statement_t> body;
	AST_MEMBER(For, &varName, &startValue, &stopValue, &stepValue, &body)
AST_END(For, "for"sv)

class AssignableNameList_t;
class StarExp_t;

AST_NODE(ForEach)
	ast_ptr<true, AssignableNameList_t> nameList;
	ast_sel<true, StarExp_t, ExpList_t> loopValue;
	ast_sel<true, Block_t, Statement_t> body;
	AST_MEMBER(ForEach, &nameList, &loopValue, &body)
AST_END(ForEach, "for_each"sv)

AST_NODE(Do)
	ast_ptr<true, Body_t> body;
	AST_MEMBER(Do, &body)
AST_END(Do, "do"sv)

AST_NODE(CatchBlock)
	ast_ptr<true, Variable_t> err;
	ast_ptr<true, Block_t> body;
	AST_MEMBER(CatchBlock, &err, &body)
AST_END(CatchBlock, "catch_block"sv)

AST_NODE(Try)
	ast_sel<true, Block_t, Exp_t> func;
	ast_ptr<false, CatchBlock_t> catchBlock;
	AST_MEMBER(Try, &func, &catchBlock)
AST_END(Try, "try"sv)

class CompInner_t;

AST_NODE(Comprehension)
	ast_sel<true, Exp_t, Statement_t> value;
	ast_ptr<true, CompInner_t> forLoop;
	AST_MEMBER(Comprehension, &value, &forLoop)
AST_END(Comprehension, "comp"sv)

AST_NODE(CompValue)
	ast_ptr<true, Exp_t> value;
	AST_MEMBER(CompValue, &value)
AST_END(CompValue, "comp_value"sv)

AST_NODE(TblComprehension)
	ast_ptr<true, Exp_t> key;
	ast_ptr<false, CompValue_t> value;
	ast_ptr<true, CompInner_t> forLoop;
	AST_MEMBER(TblComprehension, &key, &value, &forLoop)
AST_END(TblComprehension, "tbl_comp"sv)

AST_NODE(StarExp)
	ast_ptr<true, Exp_t> value;
	AST_MEMBER(StarExp, &value)
AST_END(StarExp, "star_exp"sv)

AST_NODE(CompForEach)
	ast_ptr<true, AssignableNameList_t> nameList;
	ast_sel<true, StarExp_t, Exp_t> loopValue;
	AST_MEMBER(CompForEach, &nameList, &loopValue)
AST_END(CompForEach, "comp_for_each"sv)

AST_NODE(CompFor)
	ast_ptr<true, Variable_t> varName;
	ast_ptr<true, Exp_t> startValue;
	ast_ptr<true, Exp_t> stopValue;
	ast_ptr<false, ForStepValue_t> stepValue;
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

AST_LEAF(UpdateOp)
AST_END(UpdateOp, "update_op"sv)

AST_NODE(Update)
	ast_ptr<true, UpdateOp_t> op;
	ast_ptr<true, Exp_t> value;
	AST_MEMBER(Update, &op, &value)
AST_END(Update, "update"sv)

AST_LEAF(BinaryOperator)
AST_END(BinaryOperator, "binary_op"sv)

AST_LEAF(UnaryOperator)
AST_END(UnaryOperator, "unary_op"sv)

class AssignableChain_t;

AST_NODE(Assignable)
	ast_sel<true, AssignableChain_t, Variable_t, SelfItem_t> item;
	AST_MEMBER(Assignable, &item)
AST_END(Assignable, "assignable"sv)

class UnaryExp_t;

AST_NODE(ExpOpValue)
	ast_ptr<true, BinaryOperator_t> op;
	ast_list<true, UnaryExp_t> pipeExprs;
	AST_MEMBER(ExpOpValue, &op, &pipeExprs)
AST_END(ExpOpValue, "exp_op_value"sv)

AST_NODE(Exp)
	ast_ptr<true, Seperator_t> sep;
	ast_list<true, UnaryExp_t> pipeExprs;
	ast_list<false, ExpOpValue_t> opValues;
	ast_ptr<false, Exp_t> nilCoalesed;
	AST_MEMBER(Exp, &sep, &pipeExprs, &opValues, &nilCoalesed)
AST_END(Exp, "exp"sv)

class Parens_t;
class MacroName_t;

AST_NODE(Callable)
	ast_sel<true, Variable_t, SelfItem_t, Parens_t, MacroName_t> item;
	AST_MEMBER(Callable, &item)
AST_END(Callable, "callable"sv)

AST_NODE(VariablePair)
	ast_ptr<true, Variable_t> name;
	AST_MEMBER(VariablePair, &name)
AST_END(VariablePair, "variable_pair"sv)

AST_NODE(VariablePairDef)
	ast_ptr<true, VariablePair_t> pair;
	ast_ptr<false, Exp_t> defVal;
	AST_MEMBER(VariablePairDef, &pair, &defVal)
AST_END(VariablePairDef, "variable_pair_def"sv)

class String_t;

AST_NODE(NormalPair)
	ast_sel<true, KeyName_t, Exp_t, String_t> key;
	ast_sel<true, Exp_t, TableBlock_t> value;
	AST_MEMBER(NormalPair, &key, &value)
AST_END(NormalPair, "normal_pair"sv)

AST_NODE(NormalPairDef)
	ast_ptr<true, NormalPair_t> pair;
	ast_ptr<false, Exp_t> defVal;
	AST_MEMBER(NormalPairDef, &pair, &defVal)
AST_END(NormalPairDef, "normal_pair_def"sv)

AST_NODE(NormalDef)
	ast_ptr<true, Exp_t> item;
	ast_ptr<true, Seperator_t> sep;
	ast_ptr<false, Exp_t> defVal;
	AST_MEMBER(NormalDef, &item, &sep, &defVal)
AST_END(NormalDef, "normal_def")

AST_NODE(MetaVariablePair)
	ast_ptr<true, Variable_t> name;
	AST_MEMBER(MetaVariablePair, &name)
AST_END(MetaVariablePair, "meta_variable_pair"sv)

AST_NODE(MetaVariablePairDef)
	ast_ptr<true, MetaVariablePair_t> pair;
	ast_ptr<false, Exp_t> defVal;
	AST_MEMBER(MetaVariablePairDef, &pair, &defVal)
AST_END(MetaVariablePairDef, "meta_variable_pair_def"sv)

AST_NODE(MetaNormalPair)
	ast_sel<false, Name_t, Exp_t, String_t> key;
	ast_sel<true, Exp_t, TableBlock_t> value;
	AST_MEMBER(MetaNormalPair, &key, &value)
AST_END(MetaNormalPair, "meta_normal_pair"sv)

AST_NODE(MetaNormalPairDef)
	ast_ptr<true, MetaNormalPair_t> pair;
	ast_ptr<false, Exp_t> defVal;
	AST_MEMBER(MetaNormalPairDef, &pair, &defVal)
AST_END(MetaNormalPairDef, "meta_normal_pair_def"sv)

AST_NODE(SimpleTable)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, VariablePair_t, NormalPair_t, MetaVariablePair_t, MetaNormalPair_t> pairs;
	AST_MEMBER(SimpleTable, &sep, &pairs)
AST_END(SimpleTable, "simple_table"sv)

class ConstValue_t;
class ClassDecl_t;
class UnaryValue_t;
class FunLit_t;

AST_NODE(SimpleValue)
	ast_sel<true,
	TableLit_t, ConstValue_t,
	If_t, Switch_t, With_t, ClassDecl_t,
	ForEach_t, For_t, While_t, Do_t, Try_t,
	UnaryValue_t,
	TblComprehension_t, Comprehension_t,
	FunLit_t, Num_t, VarArg_t> value;
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

AST_LEAF(DoubleStringInner)
AST_END(DoubleStringInner, "double_string_inner"sv)

AST_NODE(DoubleStringContent)
	ast_sel<true, DoubleStringInner_t, Exp_t> content;
	AST_MEMBER(DoubleStringContent, &content)
AST_END(DoubleStringContent, "double_string_content"sv)

AST_NODE(DoubleString)
	ast_ptr<true, Seperator_t> sep;
	ast_list<false, DoubleStringContent_t> segments;
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

class DefaultValue_t;

AST_NODE(Slice)
	ast_sel<true, Exp_t, DefaultValue_t> startValue;
	ast_sel<true, Exp_t, DefaultValue_t> stopValue;
	ast_sel<true, Exp_t, DefaultValue_t> stepValue;
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

AST_LEAF(ExistentialOp)
AST_END(ExistentialOp, "existential_op"sv)

AST_LEAF(TableAppendingOp)
AST_END(TableAppendingOp, "table_appending_op"sv)

class InvokeArgs_t;

AST_NODE(ChainValue)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, Callable_t, Invoke_t, DotChainItem_t, ColonChainItem_t, Slice_t, Exp_t, String_t, InvokeArgs_t, ExistentialOp_t, TableAppendingOp_t> items;
	AST_MEMBER(ChainValue, &sep, &items)
AST_END(ChainValue, "chain_value"sv)

AST_NODE(AssignableChain)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, Callable_t, Invoke_t, DotChainItem_t, ColonChainItem_t, Exp_t, String_t> items;
	AST_MEMBER(AssignableChain, &sep, &items)
AST_END(AssignableChain, "assignable_chain"sv)

AST_NODE(Value)
	ast_sel<true, SimpleValue_t, SimpleTable_t, ChainValue_t, String_t> item;
	AST_MEMBER(Value, &item)
AST_END(Value, "value"sv)

AST_LEAF(DefaultValue)
AST_END(DefaultValue, "default_value"sv)

AST_NODE(SpreadExp)
	ast_ptr<true, Exp_t> exp;
	AST_MEMBER(SpreadExp, &exp)
AST_END(SpreadExp, "spread_exp"sv)

class TableBlockIndent_t;

AST_NODE(TableLit)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<false,
		VariablePairDef_t, NormalPairDef_t, SpreadExp_t, NormalDef_t,
		MetaVariablePairDef_t, MetaNormalPairDef_t,
		VariablePair_t, NormalPair_t, Exp_t, TableBlockIndent_t,
		MetaVariablePair_t, MetaNormalPair_t> values;
	AST_MEMBER(TableLit, &sep, &values)
AST_END(TableLit, "table_lit"sv)

AST_NODE(TableBlockIndent)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<false,
		VariablePair_t, NormalPair_t, Exp_t, TableBlockIndent_t,
		MetaVariablePair_t, MetaNormalPair_t> values;
	AST_MEMBER(TableBlockIndent, &sep, &values)
AST_END(TableBlockIndent, "table_block_indent"sv)

AST_NODE(TableBlock)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<false, VariablePair_t, NormalPair_t, TableBlockIndent_t, Exp_t, TableBlock_t, SpreadExp_t, MetaVariablePair_t, MetaNormalPair_t> values;
	AST_MEMBER(TableBlock, &sep, &values)
AST_END(TableBlock, "table_block"sv)

AST_NODE(ClassMemberList)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, VariablePair_t, NormalPair_t, MetaVariablePair_t, MetaNormalPair_t> values;
	AST_MEMBER(ClassMemberList, &sep, &values)
AST_END(ClassMemberList, "class_member_list"sv)

AST_NODE(ClassBlock)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<true, ClassMemberList_t, Statement_t> contents;
	AST_MEMBER(ClassBlock, &sep, &contents)
AST_END(ClassBlock, "class_block"sv)

AST_NODE(ClassDecl)
	ast_ptr<false, Assignable_t> name;
	ast_ptr<false, Exp_t> extend;
	ast_ptr<false, ExpList_t> mixes;
	ast_ptr<false, ClassBlock_t> body;
	AST_MEMBER(ClassDecl, &name, &extend, &mixes, &body)
AST_END(ClassDecl, "class_decl"sv)

AST_NODE(GlobalValues)
	ast_ptr<true, NameList_t> nameList;
	ast_sel<false, TableBlock_t, ExpListLow_t> valueList;
	AST_MEMBER(GlobalValues, &nameList, &valueList)
AST_END(GlobalValues, "global_values"sv)

AST_LEAF(GlobalOp)
AST_END(GlobalOp, "global_op"sv)

AST_NODE(Global)
	ast_sel<true, ClassDecl_t, GlobalOp_t, GlobalValues_t> item;
	AST_MEMBER(Global, &item)
AST_END(Global, "global"sv)

AST_LEAF(ExportDefault)
AST_END(ExportDefault, "export_default"sv)

class Macro_t;

AST_NODE(Export)
	ast_ptr<false, ExportDefault_t> def;
	ast_sel<true, ExpList_t, Exp_t, Macro_t> target;
	ast_ptr<false, Assign_t> assign;
	AST_MEMBER(Export, &def, &target, &assign)
AST_END(Export, "export"sv)

AST_NODE(FnArgDef)
	ast_sel<true, Variable_t, SelfItem_t> name;
	ast_ptr<false, ExistentialOp_t> op;
	ast_ptr<false, Exp_t> defaultValue;
	AST_MEMBER(FnArgDef, &name, &op, &defaultValue)
AST_END(FnArgDef, "fn_arg_def"sv)

AST_NODE(FnArgDefList)
	ast_ptr<true, Seperator_t> sep;
	ast_list<false, FnArgDef_t> definitions;
	ast_ptr<false, VarArg_t> varArg;
	AST_MEMBER(FnArgDefList, &sep, &definitions, &varArg)
AST_END(FnArgDefList, "fn_arg_def_list"sv)

AST_NODE(OuterVarShadow)
	ast_ptr<false, NameList_t> varList;
	AST_MEMBER(OuterVarShadow, &varList)
AST_END(OuterVarShadow, "outer_var_shadow"sv)

AST_NODE(FnArgsDef)
	ast_ptr<false, FnArgDefList_t> defList;
	ast_ptr<false, OuterVarShadow_t> shadowOption;
	AST_MEMBER(FnArgsDef, &defList, &shadowOption)
AST_END(FnArgsDef, "fn_args_def"sv)

AST_LEAF(FnArrow)
AST_END(FnArrow, "fn_arrow"sv)

AST_NODE(FunLit)
	ast_ptr<false, FnArgsDef_t> argsDef;
	ast_ptr<true, FnArrow_t> arrow;
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

AST_LEAF(ConstValue)
AST_END(ConstValue, "const_value"sv)

AST_NODE(UnaryValue)
	ast_list<true, UnaryOperator_t> ops;
	ast_ptr<true, Value_t> value;
	AST_MEMBER(UnaryValue, &ops, &value)
AST_END(UnaryValue, "unary_value"sv)

AST_NODE(UnaryExp)
	ast_list<false, UnaryOperator_t> ops;
	ast_list<true, Value_t> expos;
	AST_MEMBER(UnaryExp, &ops, &expos)
AST_END(UnaryExp, "unary_exp"sv)

AST_NODE(ExpListAssign)
	ast_ptr<true, ExpList_t> expList;
	ast_sel<false, Update_t, Assign_t> action;
	AST_MEMBER(ExpListAssign, &expList, &action)
AST_END(ExpListAssign, "exp_list_assign"sv)

AST_NODE(IfLine)
	ast_ptr<true, IfType_t> type;
	ast_ptr<true, IfCond_t> condition;
	AST_MEMBER(IfLine, &type, &condition)
AST_END(IfLine, "if_line"sv)

AST_NODE(WhileLine)
	ast_ptr<true, WhileType_t> type;
	ast_ptr<true, Exp_t> condition;
	AST_MEMBER(WhileLine, &type, &condition)
AST_END(WhileLine, "while_line"sv)

AST_LEAF(BreakLoop)
AST_END(BreakLoop, "break_loop"sv)

AST_NODE(PipeBody)
	ast_ptr<true, Seperator_t> sep;
	ast_list<true, UnaryExp_t> values;
	AST_MEMBER(PipeBody, &sep, &values)
AST_END(PipeBody, "pipe_body"sv)

AST_NODE(StatementAppendix)
	ast_sel<true, IfLine_t, WhileLine_t, CompInner_t> item;
	AST_MEMBER(StatementAppendix, &item)
AST_END(StatementAppendix, "statement_appendix"sv)

AST_LEAF(StatementSep)
AST_END(StatementSep, "statement_sep"sv)

AST_LEAF(MeoLineComment)
AST_END(MeoLineComment, "comment"sv)

AST_LEAF(MeoMultilineComment)
AST_END(MeoMultilineComment, "comment"sv)

AST_NODE(ChainAssign)
	ast_ptr<true, Seperator_t> sep;
	ast_list<true, Exp_t> exprs;
	ast_ptr<true, Assign_t> assign;
	AST_MEMBER(ChainAssign, &sep, &exprs, &assign)
AST_END(ChainAssign, "chain_assign")

AST_NODE(Statement)
	ast_ptr<true, Seperator_t> sep;
	ast_sel_list<false, MeoLineComment_t, MeoMultilineComment_t> comments;
	ast_sel<true,
		Import_t, While_t, Repeat_t, For_t, ForEach_t,
		Return_t, Local_t, Global_t, Export_t, Macro_t, MacroInPlace_t,
		BreakLoop_t, Label_t, Goto_t, ShortTabAppending_t,
		Backcall_t, LocalAttrib_t, PipeBody_t, ExpListAssign_t, ChainAssign_t
	> content;
	ast_ptr<false, StatementAppendix_t> appendix;
	ast_ptr<false, StatementSep_t> needSep;
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

// clang-format on

}  // namespace parserlib

#pragma endregion MeoAST

namespace meo {
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
    std::string errorMessage(std::string_view msg, const input_range* loc) const;
};

class ParserError : public std::logic_error {
public:
    explicit ParserError(const std::string& msg, const pos& begin, const pos& end) : std::logic_error(msg), loc(begin, end) {}

    explicit ParserError(const char* msg, const pos& begin, const pos& end) : std::logic_error(msg), loc(begin, end) {}
    input_range loc;
};

template <typename T>
struct identity {
    typedef T type;
};

#ifdef NDEBUG
#define NONE_AST_RULE(type) rule type;

#define AST_RULE(type)                \
    rule type;                        \
    ast<type##_t> type##_impl = type; \
    inline rule& getRule(identity<type##_t>) { return type; }
#else  // NDEBUG
#define NONE_AST_RULE(type) rule type{#type, rule::initTag{}};

#define AST_RULE(type)                 \
    rule type{#type, rule::initTag{}}; \
    ast<type##_t> type##_impl = type;  \
    inline rule& getRule(identity<type##_t>) { return type; }
#endif  // NDEBUG

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

    std::string toString(ast_node* node);
    std::string toString(input::iterator begin, input::iterator end);

    input encode(std::string_view input);
    std::string decode(const input& input);

protected:
    ParseInfo parse(std::string_view codes, rule& r);

    struct State {
        State() { indents.push(0); }
        bool exportDefault = false;
        bool exportMacro = false;
        int exportCount = 0;
        int moduleFix = 0;
        int expLevel = 0;
        size_t stringOpen = 0;
        std::string moduleName = "_module_0"s;
        std::string buffer;
        std::stack<int> indents;
        std::stack<bool> noDoStack;
        std::stack<bool> noChainBlockStack;
        std::stack<bool> noTableBlockStack;
        std::stack<bool> noForStack;
    };

    template <class T>
    inline rule& getRule() {
        return getRule(identity<T>());
    }

private:
    Converter _converter;

    template <class T>
    inline rule& getRule(identity<T>) {
        assert(false);
        return cut;
    }

    NONE_AST_RULE(empty_block_error);
    NONE_AST_RULE(leading_spaces_error);
    NONE_AST_RULE(indentation_error);
    NONE_AST_RULE(braces_expression_error);
    NONE_AST_RULE(brackets_expression_error);

    NONE_AST_RULE(inc_exp_level);
    NONE_AST_RULE(dec_exp_level);

    NONE_AST_RULE(num_char);
    NONE_AST_RULE(num_char_hex);
    NONE_AST_RULE(num_lit);
    NONE_AST_RULE(num_expo);
    NONE_AST_RULE(num_expo_hex);
    NONE_AST_RULE(lj_num);
    NONE_AST_RULE(plain_space);
    NONE_AST_RULE(line_break);
    NONE_AST_RULE(any_char);
    NONE_AST_RULE(white);
    NONE_AST_RULE(stop);
    NONE_AST_RULE(comment);
    NONE_AST_RULE(multi_line_open);
    NONE_AST_RULE(multi_line_close);
    NONE_AST_RULE(multi_line_content);
    NONE_AST_RULE(multi_line_comment);
    NONE_AST_RULE(escape_new_line);
    NONE_AST_RULE(space_one);
    NONE_AST_RULE(space);
    NONE_AST_RULE(space_break);
    NONE_AST_RULE(alpha_num);
    NONE_AST_RULE(not_alpha_num);
    NONE_AST_RULE(cut);
    NONE_AST_RULE(check_indent_match);
    NONE_AST_RULE(check_indent);
    NONE_AST_RULE(advance_match);
    NONE_AST_RULE(advance);
    NONE_AST_RULE(push_indent_match);
    NONE_AST_RULE(push_indent);
    NONE_AST_RULE(prevent_indent);
    NONE_AST_RULE(pop_indent);
    NONE_AST_RULE(in_block);
    NONE_AST_RULE(import_name);
    NONE_AST_RULE(import_name_list);
    NONE_AST_RULE(import_literal_chain);
    NONE_AST_RULE(import_tab_item);
    NONE_AST_RULE(import_tab_list);
    NONE_AST_RULE(import_tab_line);
    NONE_AST_RULE(import_tab_lines);
    NONE_AST_RULE(with_exp);
    NONE_AST_RULE(disable_do);
    NONE_AST_RULE(enable_do);
    NONE_AST_RULE(disable_chain);
    NONE_AST_RULE(enable_chain);
    NONE_AST_RULE(disable_do_chain_arg_table_block);
    NONE_AST_RULE(enable_do_chain_arg_table_block);
    NONE_AST_RULE(disable_arg_table_block);
    NONE_AST_RULE(enable_arg_table_block);
    NONE_AST_RULE(disable_for);
    NONE_AST_RULE(enable_for);
    NONE_AST_RULE(switch_else);
    NONE_AST_RULE(switch_block);
    NONE_AST_RULE(if_else_if);
    NONE_AST_RULE(if_else);
    NONE_AST_RULE(for_key);
    NONE_AST_RULE(for_args);
    NONE_AST_RULE(for_in);
    NONE_AST_RULE(comp_clause);
    NONE_AST_RULE(chain);
    NONE_AST_RULE(chain_list);
    NONE_AST_RULE(key_value);
    NONE_AST_RULE(single_string_inner);
    NONE_AST_RULE(interp);
    NONE_AST_RULE(double_string_plain);
    NONE_AST_RULE(lua_string_open);
    NONE_AST_RULE(lua_string_close);
    NONE_AST_RULE(fn_args_exp_list);
    NONE_AST_RULE(fn_args);
    NONE_AST_RULE(destruct_def);
    NONE_AST_RULE(macro_args_def);
    NONE_AST_RULE(chain_call);
    NONE_AST_RULE(chain_call_list);
    NONE_AST_RULE(chain_index_chain);
    NONE_AST_RULE(chain_items);
    NONE_AST_RULE(chain_dot_chain);
    NONE_AST_RULE(colon_chain);
    NONE_AST_RULE(chain_with_colon);
    NONE_AST_RULE(chain_item);
    NONE_AST_RULE(chain_line);
    NONE_AST_RULE(chain_block);
    NONE_AST_RULE(meta_index);
    NONE_AST_RULE(index);
    NONE_AST_RULE(invoke_chain);
    NONE_AST_RULE(table_value);
    NONE_AST_RULE(table_lit_lines);
    NONE_AST_RULE(table_lit_line);
    NONE_AST_RULE(table_value_list);
    NONE_AST_RULE(table_block_inner);
    NONE_AST_RULE(class_line);
    NONE_AST_RULE(key_value_line);
    NONE_AST_RULE(key_value_list);
    NONE_AST_RULE(arg_line);
    NONE_AST_RULE(arg_block);
    NONE_AST_RULE(invoke_args_with_table);
    NONE_AST_RULE(arg_table_block);
    NONE_AST_RULE(pipe_operator);
    NONE_AST_RULE(exponential_operator);
    NONE_AST_RULE(pipe_value);
    NONE_AST_RULE(pipe_exp);
    NONE_AST_RULE(expo_value);
    NONE_AST_RULE(expo_exp);
    NONE_AST_RULE(exp_not_tab);
    NONE_AST_RULE(local_const_item);
    NONE_AST_RULE(empty_line_break);
    NONE_AST_RULE(meo_comment);
    NONE_AST_RULE(meo_line_comment);
    NONE_AST_RULE(meo_multiline_comment);
    NONE_AST_RULE(line);
    NONE_AST_RULE(shebang);

    AST_RULE(Num)
    AST_RULE(Name)
    AST_RULE(Variable)
    AST_RULE(LabelName)
    AST_RULE(LuaKeyword)
    AST_RULE(Self)
    AST_RULE(SelfName)
    AST_RULE(SelfClass)
    AST_RULE(SelfClassName)
    AST_RULE(SelfItem)
    AST_RULE(KeyName)
    AST_RULE(VarArg)
    AST_RULE(Seperator)
    AST_RULE(NameList)
    AST_RULE(LocalFlag)
    AST_RULE(LocalValues)
    AST_RULE(Local)
    AST_RULE(ConstAttrib)
    AST_RULE(CloseAttrib)
    AST_RULE(LocalAttrib);
    AST_RULE(ColonImportName)
    AST_RULE(ImportLiteralInner)
    AST_RULE(ImportLiteral)
    AST_RULE(ImportFrom)
    AST_RULE(MacroNamePair)
    AST_RULE(ImportAllMacro)
    AST_RULE(ImportTabLit)
    AST_RULE(ImportAs)
    AST_RULE(Import)
    AST_RULE(Label)
    AST_RULE(Goto)
    AST_RULE(ShortTabAppending)
    AST_RULE(FnArrowBack)
    AST_RULE(Backcall)
    AST_RULE(PipeBody)
    AST_RULE(ExpListLow)
    AST_RULE(ExpList)
    AST_RULE(Return)
    AST_RULE(With)
    AST_RULE(SwitchList)
    AST_RULE(SwitchCase)
    AST_RULE(Switch)
    AST_RULE(Assignment)
    AST_RULE(IfCond)
    AST_RULE(IfType)
    AST_RULE(If)
    AST_RULE(WhileType)
    AST_RULE(While)
    AST_RULE(Repeat)
    AST_RULE(ForStepValue)
    AST_RULE(For)
    AST_RULE(ForEach)
    AST_RULE(Do)
    AST_RULE(CatchBlock)
    AST_RULE(Try)
    AST_RULE(Comprehension)
    AST_RULE(CompValue)
    AST_RULE(TblComprehension)
    AST_RULE(StarExp)
    AST_RULE(CompForEach)
    AST_RULE(CompFor)
    AST_RULE(CompInner)
    AST_RULE(Assign)
    AST_RULE(UpdateOp)
    AST_RULE(Update)
    AST_RULE(BinaryOperator)
    AST_RULE(UnaryOperator)
    AST_RULE(Assignable)
    AST_RULE(AssignableChain)
    AST_RULE(ExpOpValue)
    AST_RULE(Exp)
    AST_RULE(Callable)
    AST_RULE(ChainValue)
    AST_RULE(SimpleTable)
    AST_RULE(SimpleValue)
    AST_RULE(Value)
    AST_RULE(LuaStringOpen);
    AST_RULE(LuaStringContent);
    AST_RULE(LuaStringClose);
    AST_RULE(LuaString)
    AST_RULE(SingleString)
    AST_RULE(DoubleStringInner)
    AST_RULE(DoubleStringContent)
    AST_RULE(DoubleString)
    AST_RULE(String)
    AST_RULE(Parens)
    AST_RULE(DotChainItem)
    AST_RULE(ColonChainItem)
    AST_RULE(Metatable)
    AST_RULE(Metamethod)
    AST_RULE(DefaultValue)
    AST_RULE(Slice)
    AST_RULE(Invoke)
    AST_RULE(ExistentialOp)
    AST_RULE(TableAppendingOp)
    AST_RULE(SpreadExp)
    AST_RULE(TableLit)
    AST_RULE(TableBlock)
    AST_RULE(TableBlockIndent)
    AST_RULE(ClassMemberList)
    AST_RULE(ClassBlock)
    AST_RULE(ClassDecl)
    AST_RULE(GlobalValues)
    AST_RULE(GlobalOp)
    AST_RULE(Global)
    AST_RULE(ExportDefault)
    AST_RULE(Export)
    AST_RULE(VariablePair)
    AST_RULE(NormalPair)
    AST_RULE(MetaVariablePair)
    AST_RULE(MetaNormalPair)
    AST_RULE(VariablePairDef)
    AST_RULE(NormalPairDef)
    AST_RULE(NormalDef)
    AST_RULE(MetaVariablePairDef)
    AST_RULE(MetaNormalPairDef)
    AST_RULE(FnArgDef)
    AST_RULE(FnArgDefList)
    AST_RULE(OuterVarShadow)
    AST_RULE(FnArgsDef)
    AST_RULE(FnArrow)
    AST_RULE(FunLit)
    AST_RULE(MacroName)
    AST_RULE(MacroLit)
    AST_RULE(Macro)
    AST_RULE(MacroInPlace)
    AST_RULE(NameOrDestructure)
    AST_RULE(AssignableNameList)
    AST_RULE(InvokeArgs)
    AST_RULE(ConstValue)
    AST_RULE(UnaryValue)
    AST_RULE(UnaryExp)
    AST_RULE(ExpListAssign)
    AST_RULE(IfLine)
    AST_RULE(WhileLine)
    AST_RULE(BreakLoop)
    AST_RULE(StatementAppendix)
    AST_RULE(StatementSep)
    AST_RULE(Statement)
    AST_RULE(MeoLineComment)
    AST_RULE(MeoMultilineComment)
    AST_RULE(ChainAssign)
    AST_RULE(Body)
    AST_RULE(Block)
    AST_RULE(BlockEnd)
    AST_RULE(File)
};

namespace Utils {
void replace(std::string& str, std::string_view from, std::string_view to);
void trim(std::string& str);
}  // namespace Utils

extern const std::string_view extension;

using Options = std::unordered_map<std::string, std::string>;

struct MeoConfig {
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
    MeoCompiler(void* luaState = nullptr, const std::function<void(void*)>& luaOpen = nullptr, bool sameModule = false);
    virtual ~MeoCompiler();
    CompileInfo compile(std::string_view codes, const MeoConfig& config = {});

private:
    std::unique_ptr<MeoCompilerImpl> _compiler;
};

}  // namespace meo
