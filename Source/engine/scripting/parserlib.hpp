// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include <cassert>
#include <codecvt>
#include <functional>
#include <list>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

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

enum ERROR_TYPE {

    ERROR_SYNTAX_ERROR = 1,

    ERROR_INVALID_EOF,

    ERROR_USER = 100
};

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
