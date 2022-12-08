// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_IMGUI_BINDER_HPP_
#define _METADOT_IMGUI_BINDER_HPP_

struct imgui;

#include "Libs/quickjs/quickjs.h"

extern "C"
{
#include "Libs/quickjs/cutils.h"
}

#if defined(_Thread_local) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L))
#define thread_local _Thread_local
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define thread_local __thread
#elif defined(_WIN32)
#define thread_local __declspec(thread)
#else
#error No TLS implementation found.
#endif

template<typename T>
class JSVal {
    static T to(JSContext *ctx, JSValueConst jsval);
    static JSValue from(JSContext *ctx, T v);
};

template<>
class JSVal<int32_t> {
    static int32_t to(JSContext *ctx, JSValueConst jsval) {
        int32_t r;
        JS_ToInt32(ctx, &r, jsval);
        return r;
    }
    static JSValue from(JSContext *ctx, int32_t v) { return JS_NewInt32(ctx, v); }
};

template<>
class JSVal<bool> {
public:
    static bool to(JSContext *ctx, JSValueConst jsval) { return JS_ToBool(ctx, jsval); }
    static JSValue from(JSContext *ctx, bool v) { return JS_NewBool(ctx, v); }
};

class JSRef {
public:
    JSValueConst param;

    JSRef(JSValueConst output) : param(output) {}

    JSValue get(JSContext *ctx) {
        JSValue ret = JS_UNDEFINED;
        if (JS_IsFunction(ctx, param)) {
            ret = JS_Call(ctx, param, JS_UNDEFINED, 0, 0);
        } else if (JS_IsObject(param)) {
            ret = JS_GetPropertyUint32(ctx, param, 0);
        }
        return ret;
    }

    void set(JSContext *ctx, JSValueConst value) {
        if (JS_IsFunction(ctx, param)) {
            JSValue ret;
            ret = JS_Call(ctx, param, JS_UNDEFINED, 1, &value);
            JS_FreeValue(ctx, ret);
        } else if (JS_IsObject(param) /*JS_IsArray(ctx, param)*/) {
            JS_SetPropertyUint32(ctx, param, 0, JS_DupValue(ctx, value));
        }
    }
};

template<typename T>
class OutputArg {
public:
    T value;
    JSRef param;
    JSContext *ctx;

    OutputArg() : param(JS_NULL), ctx(0) {}
    OutputArg(JSContext *_ctx, JSValueConst _arg) : param(_arg), ctx(_ctx) {
        value = JSVal<T>::to(ctx, param.get(ctx));
    }
    ~OutputArg() {
        if (ctx) param.set(ctx, JSVal<T>::from(ctx, value));
    }

    operator T *() { return &value; }
};

JSModuleDef *init_imgui_module(JSContext *ctx, const char *module_name);

#endif
