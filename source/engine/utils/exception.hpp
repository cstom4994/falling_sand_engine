// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_EXCEPTION_HPP
#define ME_EXCEPTION_HPP

#include <exception>

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/scripting/lua_wrapper_base.hpp"

namespace ME {

namespace exception {

enum class error_code {
    no_error,

    bad_cast,

    bad_const_access,
    bad_uvalue_access,

    bad_argument_cast,
    bad_instance_cast,

    arity_mismatch,
    instance_type_mismatch,
    argument_type_mismatch,
};

inline const char *get_error_code_message(error_code error) noexcept {
    switch (error) {
        case error_code::no_error:
            return "no error";
        case error_code::bad_cast:
            return "bad cast";
        case error_code::bad_const_access:
            return "bad const access";
        case error_code::bad_uvalue_access:
            return "bad uvalue access";
        case error_code::bad_argument_cast:
            return "bad argument cast";
        case error_code::bad_instance_cast:
            return "bad instance cast";
        case error_code::arity_mismatch:
            return "arity mismatch";
        case error_code::instance_type_mismatch:
            return "instance type mismatch";
        case error_code::argument_type_mismatch:
            return "argument type mismatch";
    }

    ME_ASSERT(false);
    return "unexpected error code";
}

class exception_base : public std::exception {};

class exception_meta final : public exception_base {
public:
    explicit exception_meta(error_code error) : error_{error} {}
    [[nodiscard]] error_code get_error() const noexcept { return error_; }
    [[nodiscard]] const char *what() const noexcept override { return get_error_code_message(error_); }

private:
    error_code error_{};
};

// cvar 异常
class exception_cvar final : public exception_base {
public:
    explicit exception_cvar(const std::string &msg) : error_{msg} {}
    [[nodiscard]] std::string get_error() const noexcept { return error_; }
    [[nodiscard]] const char *what() const noexcept override { return error_.c_str(); }

private:
    std::string error_{};
};

class module_not_initialized final : public exception::exception_base {
public:
    const char *what() const noexcept final { return "module not initialized"; }
};

class module_already_initialized final : public exception::exception_base {
public:
    const char *what() const noexcept final { return "module already initialized"; }
};

class exception_lua final : public exception_base {
private:
    lua_State *m_L;
    std::string m_what;

public:
    exception_lua(lua_State *L, int /*code*/) : m_L(L) { whatFromStack(); }
    exception_lua(std::string what) : m_L(nullptr) { m_what = what; }
    exception_lua(lua_State *L, char const *, char const *, long) : m_L(L) { whatFromStack(); }
    ~exception_lua() throw() {}

    char const *what() const throw() { return m_what.c_str(); }

    template <class Exception>
    static void Throw(Exception e) {
        throw e;
    }

    static void pcall(lua_State *L, int nargs = 0, int nresults = 0, int msgh = 0) {
        int code = lua_pcall(L, nargs, nresults, msgh);

        if (code != LUA_OK) Throw(exception_lua(L, code));
    }

protected:
    void whatFromStack() {
        if (lua_gettop(m_L) > 0) {
            char const *s = lua_tostring(m_L, -1);
            m_what = s ? s : "";
        } else {
            // stack is empty
            m_what = "missing error";
        }
    }
};

struct bad_optional_access : exception_base {};

class LuaException : public exception_base {
    int status_;
    std::string what_;
    const char *what_c_;

public:
    LuaException(int status, const char *what) throw() : status_(status), what_c_(what) {}
    LuaException(int status, const std::string &what) : status_(status), what_(what), what_c_(0) {}
    int status() const throw() { return status_; }
    const char *what() const throw() { return what_c_ ? what_c_ : what_.c_str(); }

    ~LuaException() throw() {}
};
class KaguyaException : public exception_base {
    std::string what_;
    const char *what_c_;

public:
    KaguyaException(const char *what) throw() : what_c_(what) {}
    KaguyaException(const std::string &what) : what_(what), what_c_(0) {}
    const char *what() const throw() { return what_c_ ? what_c_ : what_.c_str(); }

    ~KaguyaException() throw() {}
};
class LuaTypeMismatch : public LuaException {
public:
    LuaTypeMismatch() throw() : LuaException(0, "type mismatch!!") {}
    LuaTypeMismatch(const char *what) throw() : LuaException(0, what) {}
    LuaTypeMismatch(const std::string &what) : LuaException(0, what) {}
};
class LuaMemoryError : public LuaException {
public:
    LuaMemoryError(int status, const char *what) throw() : LuaException(status, what) {}
    LuaMemoryError(int status, const std::string &what) : LuaException(status, what) {}
};
class LuaRuntimeError : public LuaException {
public:
    LuaRuntimeError(int status, const char *what) throw() : LuaException(status, what) {}
    LuaRuntimeError(int status, const std::string &what) : LuaException(status, what) {}
};
class LuaErrorRunningError : public LuaException {
public:
    LuaErrorRunningError(int status, const char *what) throw() : LuaException(status, what) {}
    LuaErrorRunningError(int status, const std::string &what) : LuaException(status, what) {}
};
class LuaGCError : public LuaException {
public:
    LuaGCError(int status, const char *what) throw() : LuaException(status, what) {}
    LuaGCError(int status, const std::string &what) : LuaException(status, what) {}
};
class LuaUnknownError : public LuaException {
public:
    LuaUnknownError(int status, const char *what) throw() : LuaException(status, what) {}
    LuaUnknownError(int status, const std::string &what) : LuaException(status, what) {}
};

class LuaSyntaxError : public LuaException {
public:
    LuaSyntaxError(int status, const std::string &what) : LuaException(status, what) {}
};

}  // namespace exception

}  // namespace ME

#endif