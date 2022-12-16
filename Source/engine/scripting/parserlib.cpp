// Copyright(c) 2022, KaoruXun All rights reserved.

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "parserlib.hpp"

namespace parserlib {

    class _private {
    public:
        static _expr *get_expr(const expr &e) { return e.m_expr; }

        static expr construct_expr(_expr *e) { return e; }

        static _expr *get_expr(rule &r) { return r.m_expr; }

        static parse_proc get_parse_proc(rule &r) { return r.m_parse_proc; }
    };

    class _context;

    class _state {
    public:
        pos m_pos;

        size_t m_matches;

        _state(_context &con);
    };

    class _match {
    public:
        rule *m_rule;

        pos m_begin;

        pos m_end;

        _match() {}

        _match(rule *r, const pos &b, const pos &e) : m_rule(r), m_begin(b), m_end(e) {}
    };

    typedef std::vector<_match> _match_vector;

    class _context {
    public:
        void *m_user_data;

        pos m_pos;

        pos m_error_pos;

        input::iterator m_begin;

        input::iterator m_end;

        _match_vector m_matches;

        _context(input &i, void *ud)
            : m_user_data(ud), m_pos(i), m_error_pos(i), m_begin(i.begin()), m_end(i.end()) {}

        bool end() const { return m_pos.m_it == m_end; }

        input::value_type symbol() const {
            assert(!end());
            return *m_pos.m_it;
        }

        void set_error_pos() {
            if (m_pos.m_it > m_error_pos.m_it) { m_error_pos = m_pos; }
        }

        void next_col() {
            ++m_pos.m_it;
            ++m_pos.m_col;
        }

        void next_line() {
            ++m_pos.m_line;
            m_pos.m_col = 1;
        }

        void restore(const _state &st) {
            m_pos = st.m_pos;
            m_matches.resize(st.m_matches);
        }

        bool parse_non_term(rule &r);

        bool parse_term(rule &r);

        void do_parse_procs(void *d) const {
            for (_match_vector::const_iterator it = m_matches.begin(); it != m_matches.end();
                 ++it) {
                const _match &m = *it;
                parse_proc p = _private::get_parse_proc(*m.m_rule);
                p(m.m_begin, m.m_end, d);
            }
        }

    private:
        bool _parse_non_term(rule &r);

        bool _parse_term(rule &r);
    };

    class _expr {
    public:
        virtual ~_expr() {}

        virtual bool parse_non_term(_context &con) const = 0;

        virtual bool parse_term(_context &con) const = 0;
    };

    class _char : public _expr {
    public:
        _char(char c) : m_char(c) {}

        virtual bool parse_non_term(_context &con) const { return _parse(con); }

        virtual bool parse_term(_context &con) const { return _parse(con); }

    private:
        input::value_type m_char;

        bool _parse(_context &con) const {
            if (!con.end()) {
                input::value_type ch = con.symbol();
                if (ch == m_char) {
                    con.next_col();
                    return true;
                }
            }
            con.set_error_pos();
            return false;
        }
    };

    class _string : public _expr {
    public:
        _string(const char *s) : m_string(Converter{}.from_bytes(s)) {}

        virtual bool parse_non_term(_context &con) const { return _parse(con); }

        virtual bool parse_term(_context &con) const { return _parse(con); }

    private:
        input m_string;

        bool _parse(_context &con) const {
            for (input::const_iterator it = m_string.begin(), end = m_string.end();;) {
                if (it == end) return true;
                if (con.end()) break;
                if (con.symbol() != *it) break;
                ++it;
                con.next_col();
            }
            con.set_error_pos();
            return false;
        }
    };

    class _set : public _expr {
    public:
        _set(const char *s) {
            auto str = Converter{}.from_bytes(s);
            for (auto ch: str) { _add(ch); }
        }

        _set(int min, int max) {
            assert(min >= 0);
            assert(min <= max);
            m_quick_set.resize((size_t) max + 1U);
            for (; min <= max; ++min) { m_quick_set[(size_t) min] = true; }
        }

        virtual bool parse_non_term(_context &con) const { return _parse(con); }

        virtual bool parse_term(_context &con) const { return _parse(con); }

    private:
        std::vector<bool> m_quick_set;
        std::unordered_set<size_t> m_large_set;

        void _add(size_t i) {
            if (i <= m_quick_set.size() || i <= 255) {
                if (i >= m_quick_set.size()) { m_quick_set.resize(i + 1); }
                m_quick_set[i] = true;
            } else {
                m_large_set.insert(i);
            }
        }

        bool _parse(_context &con) const {
            if (!con.end()) {
                size_t ch = con.symbol();
                if (ch < m_quick_set.size()) {
                    if (m_quick_set[ch]) {
                        con.next_col();
                        return true;
                    }
                } else if (m_large_set.find(ch) != m_large_set.end()) {
                    con.next_col();
                    return true;
                }
            }
            con.set_error_pos();
            return false;
        }
    };

    class _unary : public _expr {
    public:
        _unary(_expr *e) : m_expr(e) {}

        virtual ~_unary() { delete m_expr; }

    protected:
        _expr *m_expr;
    };

    class _term : public _unary {
    public:
        _term(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const { return m_expr->parse_term(con); }

        virtual bool parse_term(_context &con) const { return m_expr->parse_term(con); }
    };

    class _user : public _unary {
    public:
        _user(_expr *e, const user_handler &callback) : _unary(e), m_handler(callback) {}

        virtual bool parse_non_term(_context &con) const {
            pos pos = con.m_pos;
            if (m_expr->parse_non_term(con)) {
                item_t item = {&pos, &con.m_pos, con.m_user_data};
                return m_handler(item);
            }
            return false;
        }

        virtual bool parse_term(_context &con) const {
            pos pos = con.m_pos;
            if (m_expr->parse_term(con)) {
                item_t item = {&pos, &con.m_pos, con.m_user_data};
                return m_handler(item);
            }
            return false;
        }

    private:
        user_handler m_handler;
    };

    class _loop0 : public _unary {
    public:
        _loop0(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const {

            _state st(con);
            if (!m_expr->parse_non_term(con)) {
                con.restore(st);
                return true;
            }

            for (;;) {
                _state st(con);
                if (!m_expr->parse_non_term(con)) {
                    con.restore(st);
                    break;
                }
            }

            return true;
        }

        virtual bool parse_term(_context &con) const {

            _state st(con);
            if (!m_expr->parse_term(con)) {
                con.restore(st);
                return true;
            }

            for (;;) {
                _state st(con);
                if (!m_expr->parse_term(con)) {
                    con.restore(st);
                    break;
                }
            }

            return true;
        }
    };

    class _loop1 : public _unary {
    public:
        _loop1(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const {

            if (!m_expr->parse_non_term(con)) return false;

            for (;;) {
                _state st(con);
                if (!m_expr->parse_non_term(con)) {
                    con.restore(st);
                    break;
                }
            }

            return true;
        }

        virtual bool parse_term(_context &con) const {

            if (!m_expr->parse_term(con)) return false;

            for (;;) {
                _state st(con);
                if (!m_expr->parse_term(con)) {
                    con.restore(st);
                    break;
                }
            }

            return true;
        }
    };

    class _optional : public _unary {
    public:
        _optional(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const {
            _state st(con);
            if (!m_expr->parse_non_term(con)) con.restore(st);
            return true;
        }

        virtual bool parse_term(_context &con) const {
            _state st(con);
            if (!m_expr->parse_term(con)) con.restore(st);
            return true;
        }
    };

    class _and : public _unary {
    public:
        _and(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const {
            _state st(con);
            bool ok = m_expr->parse_non_term(con);
            con.restore(st);
            return ok;
        }

        virtual bool parse_term(_context &con) const {
            _state st(con);
            bool ok = m_expr->parse_term(con);
            con.restore(st);
            return ok;
        }
    };

    class _not : public _unary {
    public:
        _not(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const {
            _state st(con);
            bool ok = !m_expr->parse_non_term(con);
            con.restore(st);
            return ok;
        }

        virtual bool parse_term(_context &con) const {
            _state st(con);
            bool ok = !m_expr->parse_term(con);
            con.restore(st);
            return ok;
        }
    };

    class _nl : public _unary {
    public:
        _nl(_expr *e) : _unary(e) {}

        virtual bool parse_non_term(_context &con) const {
            if (!m_expr->parse_non_term(con)) return false;
            con.next_line();
            return true;
        }

        virtual bool parse_term(_context &con) const {
            if (!m_expr->parse_term(con)) return false;
            con.next_line();
            return true;
        }
    };

    class _binary : public _expr {
    public:
        _binary(_expr *left, _expr *right) : m_left(left), m_right(right) {}

        virtual ~_binary() {
            delete m_left;
            delete m_right;
        }

    protected:
        _expr *m_left, *m_right;
    };

    class _seq : public _binary {
    public:
        _seq(_expr *left, _expr *right) : _binary(left, right) {}

        virtual bool parse_non_term(_context &con) const {
            if (!m_left->parse_non_term(con)) return false;
            return m_right->parse_non_term(con);
        }

        virtual bool parse_term(_context &con) const {
            if (!m_left->parse_term(con)) return false;
            return m_right->parse_term(con);
        }
    };

    class _choice : public _binary {
    public:
        _choice(_expr *left, _expr *right) : _binary(left, right) {}

        virtual bool parse_non_term(_context &con) const {
            _state st(con);
            if (m_left->parse_non_term(con)) return true;
            con.restore(st);
            return m_right->parse_non_term(con);
        }

        virtual bool parse_term(_context &con) const {
            _state st(con);
            if (m_left->parse_term(con)) return true;
            con.restore(st);
            return m_right->parse_term(con);
        }
    };

    class _ref : public _expr {
    public:
        _ref(rule &r) : m_rule(r) {}

        virtual bool parse_non_term(_context &con) const { return con.parse_non_term(m_rule); }

        virtual bool parse_term(_context &con) const { return con.parse_term(m_rule); }

    private:
        rule &m_rule;
    };

    class _eof : public _expr {
    public:
        virtual bool parse_non_term(_context &con) const { return parse_term(con); }

        virtual bool parse_term(_context &con) const { return con.end(); }
    };

    class _any : public _expr {
    public:
        virtual bool parse_non_term(_context &con) const { return parse_term(con); }

        virtual bool parse_term(_context &con) const {
            if (!con.end()) {
                con.next_col();
                return true;
            }
            con.set_error_pos();
            return false;
        }
    };

    class _true : public _expr {
    public:
        virtual bool parse_non_term(_context &) const { return true; }

        virtual bool parse_term(_context &) const { return true; }
    };

    class _false : public _expr {
    public:
        virtual bool parse_non_term(_context &) const { return false; }

        virtual bool parse_term(_context &) const { return false; }
    };

    struct _lr_ok
    {
        rule *m_rule;
        _lr_ok(rule *r) : m_rule(r) {}
    };

    _state::_state(_context &con) : m_pos(con.m_pos), m_matches(con.m_matches.size()) {}

    bool _context::parse_non_term(rule &r) {

        rule::_state old_state = r.m_state;

        bool ok = false;

        size_t new_pos = m_pos.m_it - m_begin;

        bool lr = new_pos == r.m_state.m_pos;

        r.m_state.m_pos = new_pos;

        switch (r.m_state.m_mode) {

            case rule::_PARSE:
                if (lr) {

                    r.m_state.m_mode = rule::_REJECT;
                    ok = _parse_non_term(r);

                    if (ok) {
                        r.m_state.m_mode = rule::_ACCEPT;

                        for (;;) {

                            _state st(*this);

                            r.m_state.m_pos = m_pos.m_it - m_begin;

                            if (!_parse_non_term(r)) {
                                restore(st);
                                break;
                            }
                        }

                        r.m_state = old_state;
                        throw _lr_ok(r.this_ptr());
                    }
                } else {
                    try {
                        ok = _parse_non_term(r);
                    } catch (const _lr_ok &ex) {

                        if (ex.m_rule == r.this_ptr()) {
                            ok = true;
                        } else {
                            r.m_state = old_state;
                            throw;
                        }
                    }
                }
                break;

            case rule::_REJECT:
                if (lr) {
                    ok = false;
                } else {
                    r.m_state.m_mode = rule::_PARSE;
                    ok = _parse_non_term(r);
                    r.m_state.m_mode = rule::_REJECT;
                }
                break;

            case rule::_ACCEPT:
                if (lr) {
                    ok = true;
                } else {
                    r.m_state.m_mode = rule::_PARSE;
                    ok = _parse_non_term(r);
                    r.m_state.m_mode = rule::_ACCEPT;
                }
                break;
        }

        r.m_state = old_state;

        return ok;
    }

    bool _context::parse_term(rule &r) {

        rule::_state old_state = r.m_state;

        bool ok = false;

        size_t new_pos = m_pos.m_it - m_begin;

        bool lr = new_pos == r.m_state.m_pos;

        r.m_state.m_pos = new_pos;

        switch (r.m_state.m_mode) {

            case rule::_PARSE:
                if (lr) {

                    r.m_state.m_mode = rule::_REJECT;
                    ok = _parse_term(r);

                    if (ok) {
                        r.m_state.m_mode = rule::_ACCEPT;

                        for (;;) {

                            _state st(*this);

                            r.m_state.m_pos = m_pos.m_it - m_begin;

                            if (!_parse_term(r)) {
                                restore(st);
                                break;
                            }
                        }

                        r.m_state = old_state;
                        throw _lr_ok(r.this_ptr());
                    }
                } else {
                    try {
                        ok = _parse_term(r);
                    } catch (const _lr_ok &ex) {

                        if (ex.m_rule == r.this_ptr()) {
                            ok = true;
                        } else {
                            r.m_state = old_state;
                            throw;
                        }
                    }
                }
                break;

            case rule::_REJECT:
                if (lr) {
                    ok = false;
                } else {
                    r.m_state.m_mode = rule::_PARSE;
                    ok = _parse_term(r);
                    r.m_state.m_mode = rule::_REJECT;
                }
                break;

            case rule::_ACCEPT:
                if (lr) {
                    ok = true;
                } else {
                    r.m_state.m_mode = rule::_PARSE;
                    ok = _parse_term(r);
                    r.m_state.m_mode = rule::_ACCEPT;
                }
                break;
        }

        r.m_state = old_state;

        return ok;
    }

    bool _context::_parse_non_term(rule &r) {
        bool ok = false;
        if (_private::get_parse_proc(r)) {
            pos b = m_pos;
            ok = _private::get_expr(r)->parse_non_term(*this);
            if (ok) { m_matches.push_back(_match(r.this_ptr(), b, m_pos)); }
        } else {
            ok = _private::get_expr(r)->parse_non_term(*this);
        }
        return ok;
    }

    bool _context::_parse_term(rule &r) {
        bool ok = false;
        if (_private::get_parse_proc(r)) {
            pos b = m_pos;
            ok = _private::get_expr(r)->parse_term(*this);
            if (ok) { m_matches.push_back(_match(r.this_ptr(), b, m_pos)); }
        } else {
            ok = _private::get_expr(r)->parse_term(*this);
        }
        return ok;
    }

    static error _syntax_error(_context &con) {
        return error(con.m_error_pos, con.m_error_pos, ERROR_SYNTAX_ERROR);
    }

    static error _eof_error(_context &con) {
        return error(con.m_error_pos, con.m_error_pos, ERROR_INVALID_EOF);
    }

    pos::pos(input &i) : m_it(i.begin()), m_line(1), m_col(1) {}

    expr::expr(char c) : m_expr(new _char(c)) {}

    expr::expr(const char *s) : m_expr(new _string(s)) {}

    expr::expr(rule &r) : m_expr(new _ref(r)) {}

    expr expr::operator*() const { return _private::construct_expr(new _loop0(m_expr)); }

    expr expr::operator+() const { return _private::construct_expr(new _loop1(m_expr)); }

    expr expr::operator-() const { return _private::construct_expr(new _optional(m_expr)); }

    expr expr::operator&() const { return _private::construct_expr((new _and(m_expr))); }

    expr expr::operator!() const { return _private::construct_expr(new _not(m_expr)); }

    input_range::input_range(const pos &b, const pos &e) : m_begin(b), m_end(e) {}

    error::error(const pos &b, const pos &e, int t) : input_range(b, e), m_type(t) {}

    bool error::operator<(const error &e) const { return m_begin.m_it < e.m_begin.m_it; }

    rule::rule() : m_expr(nullptr), m_parse_proc(nullptr) {}

    rule::rule(char c) : m_expr(new _char(c)), m_parse_proc(nullptr) {}

    rule::rule(const char *s) : m_expr(new _string(s)), m_parse_proc(nullptr) {}

    rule::rule(const expr &e) : m_expr(_private::get_expr(e)), m_parse_proc(nullptr) {}

    rule::rule(rule &r) : m_expr(new _ref(r)), m_parse_proc(nullptr) {}

    rule &rule::operator=(rule &r) {
        m_expr = new _ref(r);
        return *this;
    }

    rule &rule::operator=(const expr &e) {
        m_expr = _private::get_expr(e);
        return *this;
    }

    rule::rule(const rule &) { throw std::logic_error("invalid operation"); }

    rule::~rule() { delete m_expr; }

    expr rule::operator*() { return _private::construct_expr(new _loop0(new _ref(*this))); }

    expr rule::operator+() { return _private::construct_expr(new _loop1(new _ref(*this))); }

    expr rule::operator-() { return _private::construct_expr(new _optional(new _ref(*this))); }

    expr rule::operator&() { return _private::construct_expr(new _and(new _ref(*this))); }

    expr rule::operator!() { return _private::construct_expr(new _not(new _ref(*this))); }

    void rule::set_parse_proc(parse_proc p) {
        assert(p);
        m_parse_proc = p;
    }

    expr operator>>(const expr &left, const expr &right) {
        return _private::construct_expr(
                new _seq(_private::get_expr(left), _private::get_expr(right)));
    }

    expr operator|(const expr &left, const expr &right) {
        return _private::construct_expr(
                new _choice(_private::get_expr(left), _private::get_expr(right)));
    }

    expr term(const expr &e) { return _private::construct_expr(new _term(_private::get_expr(e))); }

    expr set(const char *s) { return _private::construct_expr(new _set(s)); }

    expr range(int min, int max) { return _private::construct_expr(new _set(min, max)); }

    expr nl(const expr &e) { return _private::construct_expr(new _nl(_private::get_expr(e))); }

    expr eof() { return _private::construct_expr(new _eof()); }

    expr not_(const expr &e) { return !e; }

    expr and_(const expr &e) { return &e; }

    expr any() { return _private::construct_expr(new _any()); }

    expr true_() { return _private::construct_expr(new _true()); }

    expr false_() { return _private::construct_expr(new _false()); }

    expr user(const expr &e, const user_handler &handler) {
        return _private::construct_expr(new _user(_private::get_expr(e), handler));
    }

    bool parse(input &i, rule &g, error_list &el, void *d, void *ud) {

        _context con(i, ud);

        if (!con.parse_non_term(g)) {
            el.push_back(_syntax_error(con));
            return false;
        }

        if (!con.end()) {
            if (con.m_error_pos.m_it < con.m_end) {
                el.push_back(_syntax_error(con));
            } else {
                el.push_back(_eof_error(con));
            }
            return false;
        }

        con.do_parse_procs(d);
        return true;
    }

#pragma region AST

    traversal ast_node::traverse(const std::function<traversal(ast_node *)> &func) {
        return func(this);
    }

    ast_node *ast_node::getByTypeIds(int *begin, int *end) {
        ast_node *current = this;
        auto it = begin;
        while (it != end) {
            ast_node *findNode = nullptr;
            int i = *it;
            current->visitChild([&](ast_node *node) {
                if (node->getId() == i) {
                    findNode = node;
                    return true;
                }
                return false;
            });
            if (findNode) {
                current = findNode;
            } else {
                current = nullptr;
                break;
            }
            ++it;
        }
        return current;
    }

    bool ast_node::visitChild(const std::function<bool(ast_node *)> &) { return false; }

    void ast_container::construct(ast_stack &st) {
        for (ast_member_vector::reverse_iterator it = m_members.rbegin(); it != m_members.rend();
             ++it) {
            ast_member *member = *it;
            member->construct(st);
        }
    }

    traversal ast_container::traverse(const std::function<traversal(ast_node *)> &func) {
        traversal action = func(this);
        switch (action) {
            case traversal::Stop:
                return traversal::Stop;
            case traversal::Return:
                return traversal::Continue;
            default:
                break;
        }
        const auto &members = this->members();
        for (auto member: members) {
            switch (member->get_type()) {
                case ast_holder_type::Pointer: {
                    _ast_ptr *ptr = static_cast<_ast_ptr *>(member);
                    if (ptr->get() && ptr->get()->traverse(func) == traversal::Stop) {
                        return traversal::Stop;
                    }
                    break;
                }
                case ast_holder_type::List: {
                    _ast_list *list = static_cast<_ast_list *>(member);
                    for (auto obj: list->objects()) {
                        if (obj->traverse(func) == traversal::Stop) { return traversal::Stop; }
                    }
                    break;
                }
            }
        }
        return traversal::Continue;
    }

    bool ast_container::visitChild(const std::function<bool(ast_node *)> &func) {
        const auto &members = this->members();
        for (auto member: members) {
            switch (member->get_type()) {
                case ast_holder_type::Pointer: {
                    _ast_ptr *ptr = static_cast<_ast_ptr *>(member);
                    if (ptr->get()) {
                        if (func(ptr->get())) return true;
                    }
                    break;
                }
                case ast_holder_type::List: {
                    _ast_list *list = static_cast<_ast_list *>(member);
                    for (auto obj: list->objects()) {
                        if (obj) {
                            if (func(obj)) return true;
                        }
                    }
                    break;
                }
            }
        }
        return false;
    }

    ast_node *parse(input &i, rule &g, error_list &el, void *ud) {
        ast_stack st;
        if (!parse(i, g, el, &st, ud)) {
            for (auto node: st) { delete node; }
            st.clear();
            return nullptr;
        }
        assert(st.size() == 1);
        return st.front();
    }

#pragma endregion AST

}// namespace parserlib
