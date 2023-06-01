// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "jsonwarp.h"

#include <cmath>
#include <cstring>

namespace MetaEngine::Json {

typedef unsigned char u_char;
typedef unsigned int ngx_uint_t;
typedef int ngx_int_t;

#define NGX_OK 0
#define NGX_ERROR (-1)
#define BASE64_ENCODE_OUT_SIZE(s) (((s) + 2) / 3 * 4)
#define BASE64_DECODE_OUT_SIZE(s) (((s)) / 4 * 3)

struct ngx_str_t {
    size_t len;
    u_char *data;
};

static void ngx_encode_base64_internal(ngx_str_t *dst, ngx_str_t *src, const u_char *basis, ngx_uint_t padding) {
    u_char *d, *s;
    size_t len;

    len = src->len;
    s = src->data;
    d = dst->data;

    while (len > 2) {
        *d++ = basis[(s[0] >> 2) & 0x3f];
        *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len) {
        *d++ = basis[(s[0] >> 2) & 0x3f];

        if (len == 1) {
            *d++ = basis[(s[0] & 3) << 4];
            if (padding) {
                *d++ = '=';
            }

        } else {
            *d++ = basis[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis[(s[1] & 0x0f) << 2];
        }

        if (padding) {
            *d++ = '=';
        }
    }

    dst->len = d - dst->data;
}

static void ngx_encode_base64(ngx_str_t *dst, ngx_str_t *src) {
    static u_char basis64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    ngx_encode_base64_internal(dst, src, basis64, 1);
}

static ngx_int_t ngx_decode_base64_internal(ngx_str_t *dst, ngx_str_t *src, const u_char *basis) {
    size_t len;
    u_char *d, *s;

    for (len = 0; len < src->len; len++) {
        if (src->data[len] == '=') {
            break;
        }

        if (basis[src->data[len]] == 77) {
            return NGX_ERROR;
        }
    }

    if (len % 4 == 1) {
        return NGX_ERROR;
    }

    s = src->data;
    d = dst->data;

    while (len > 3) {
        *d++ = (u_char)(basis[s[0]] << 2 | basis[s[1]] >> 4);
        *d++ = (u_char)(basis[s[1]] << 4 | basis[s[2]] >> 2);
        *d++ = (u_char)(basis[s[2]] << 6 | basis[s[3]]);

        s += 4;
        len -= 4;
    }

    if (len > 1) {
        *d++ = (u_char)(basis[s[0]] << 2 | basis[s[1]] >> 4);
    }

    if (len > 2) {
        *d++ = (u_char)(basis[s[1]] << 4 | basis[s[2]] >> 2);
    }

    dst->len = d - dst->data;

    return NGX_OK;
}

static ngx_int_t ngx_decode_base64(ngx_str_t *dst, ngx_str_t *src) {
    static u_char basis64[] = {
            77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
            62, 77, 77, 77, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77, 77, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            21, 22, 23, 24, 25, 77, 77, 77, 77, 77, 77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

            77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
            77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
            77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77};

    return ngx_decode_base64_internal(dst, src, basis64);
}

Json::Json(const uint8_t *s, size_t len) {
    size_t enlen = BASE64_ENCODE_OUT_SIZE(len) + 1;
    ngx_str_t src, dest;

    src.data = (u_char *)s;
    src.len = len;

    dest.data = (uint8_t *)cJSON_malloc(enlen);
    dest.len = enlen;
    ngx_encode_base64(&dest, &src);
    dest.data[dest.len] = '\0';
    json_ = cJSON_CreateRaw((char *)dest.data);
}

template <>
std::vector<uint8_t> Json::to() const {
    std::vector<uint8_t> data;
    if (type() == kString || type() == kRaw) {
        ngx_str_t src, dest;

        src.data = (u_char *)json_->valuestring;
        src.len = strlen(json_->valuestring);

        size_t buflen = BASE64_DECODE_OUT_SIZE(src.len);
        data.resize(buflen);

        dest.data = data.data();
        dest.len = buflen;
        if (ngx_decode_base64(&dest, &src) == NGX_OK) {
            data.resize(dest.len);
        }
    }
    return data;
}

Rpc::Rpc() : sender_(nullptr), gen_id_(0) {}

Rpc::~Rpc() {}

void Rpc::registerSender(RpcSender *sender) { sender_ = sender; }

void Rpc::registerHandler(const std::string &name, MetaEngine::Json::rpc_handler_t handler) { handlers_[name] = handler; }

Json Rpc::handleRequest(Json &req) {
    Response response;

    Json jid = req.detach("id");

    do {
        Json jrpc = req.detach("jsonrpc");
        Json jmethod = req.detach("method");
        Json jparams = req.detach("params");

        if (jrpc.to<std::string>() != JRPC_VERSION || !jmethod.isString()) {
            response.error(JSONRPC2_EIREQ, "invalid request");
            break;
        }

        Request request(jmethod.to<std::string>(), jparams);
        auto find = handlers_.find(request.method());
        if (find == handlers_.end()) {
            response.error(JSONRPC2_ENOMET, "invalid method");
            break;
        }

        find->second(request, response);
    } while (0);

    if (!response.valied()) {
        response.ok(Json::null());
    }

    Json jres = response.data();
    jres.add("id", jid);
    jres.add("jsonrpc", JRPC_VERSION);
    return jres;
}

bool Rpc::handleResponse(Json &j) {
    Json jrpc = j.detach("jsonrpc");
    if (jrpc.to<std::string>() != JRPC_VERSION) {
        return false;
    }

    Response resp(j);
    Json jid = j.detach("id");
    auto find = calls_.find(jid.to<int>());
    if (find != calls_.end()) {
        find->second(resp);
        calls_.erase(find);
        return true;
    }
    return false;
}

void Rpc::call(const Request &req, rpc_call_t oncall) {
    Json jreq = Json::object();
    int id = ++gen_id_;
    jreq.add("id", Json(id));
    jreq.add("jsonrpc", JRPC_VERSION);
    jreq.add("method", Json(req.method()));
    jreq.add("params", req.params());
    calls_[id] = oncall;
    sender_->send(jreq);
}

void Rpc::call(const std::string &method, Json params, rpc_call_t oncall) {
    Request req(method, params);
    call(req, oncall);
}

bool Rpc::recv(Json &js) {
    if (js["method"].isString()) {
        Json jsend = handleRequest(js);
        sender_->send(jsend);
        return true;
    } else {
        return handleResponse(js);
    }
}

/*
##### 数据类型

​	int,float,bool,string

##### 是否可选，默认为必选

​	optional 可省略为opt

##### 范围区间表示

​	(1,100) 表示 1 < x < 100
​	[1,100) 表示 1 <= x < 100
​	(1,100] 表示 1 < x <= 100
​	[1,100] 表示 1 <= x <= 100
​	{1,2,3,4,5} 表示在集合内
    !{1,2,3} 表示不在集合内

##### 示例如下：

1. int
2. int [1,100]
3. optioal int (1,100)
4. float (0,1)
5. string [10,20]  表示字符串长度范围
6. int >0
*/

#define S_OPTINAL "optinal"
#define S_INT "int"
#define S_FLOAT "float"
#define S_BOOL "bool"
#define S_STRING "string"

#define CSTR_LEN(m) (sizeof(m) - 1)

const char *skipSpace(const char *str) {
    while (*str) {
        if (*str == ' ' || *str == '\t') {
            ++str;
        } else {
            break;
        }
    }
    return str;
}

inline bool isEnd(char ch) { return ch == '\0' || ch == ' '; }

inline bool equare(double d1, double d2) { return fabs(d1 - d2) < 1e-6; }

// 字符串比较，遇到空格字符会结束
int strCmp(const char *dest, const char *source) {
    while (*dest && *source && *dest != ' ' && *source != ' ' && (*dest == *source)) {
        dest++;
        source++;
    }

    if (isEnd(*dest) && isEnd(*source)) {
        return 0;
    } else {
        return *dest - *source;
    }
}

// 检测范围
bool checkRange(const Json &value, const char *str) {
    switch (str[0]) {
        case '(':
        case '[': {
            char *endstr;
            char start = str[0];
            str = skipSpace(str + 1);
            double a = strtod(str, &endstr);
            str = skipSpace(endstr + 1);
            double b = strtod(str, &endstr);
            str = skipSpace(endstr);

            // 检测最小范围
            if (value.isNumber()) {
                if (start == '[') {
                    if (value.to<double>() < a) {
                        return false;
                    }
                } else {
                    if (value.to<double>() <= a) return false;
                }
            } else if (value.isString()) {
                if (start == '[') {
                    if (value.size() < a) return false;
                } else {
                    if (value.size() <= a) return false;
                }
            }

            // 没有结束符，不检测最大范围
            if (*str != ']' && *str != ')') {
                return true;
            }

            // 检测最大范围
            if (value.isNumber()) {
                if (*str == ']') {
                    return value.to<double>() <= b;
                } else {
                    return value.to<double>() < b;
                }
            } else if (value.isString()) {
                if (*str == ']') {
                    return value.size() <= b;
                } else {
                    return value.size() < b;
                }
            }
            break;
        }
        case '{': {
            // 集合
            char *endstr;
            str = skipSpace(str + 1);
            double num;
            if (value.isString())
                num = value.size();
            else
                num = value.to<double>();

            while (*str) {
                double a = strtod(str, &endstr);
                if (equare(a, num)) {
                    return true;
                }
                str = skipSpace(endstr + 1);
            }

            // 不在集合内返回
            return false;
        }
        case '!': {
            // 不在集合内
            if (str[1] == '{') {
                char *endstr;
                str = skipSpace(str + 2);
                double num;
                if (value.isString())
                    num = value.size();
                else
                    num = value.to<double>();
                while (*str) {
                    double a = strtod(str, &endstr);
                    if (equare(a, num)) {
                        return false;
                    }
                    str = skipSpace(endstr + 1);
                }
            }
            break;
        }
        case '>':
            if (str[1] == '=') {
                str = skipSpace(str + 2);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() >= a;
                } else if (value.isString()) {
                    return value.size() >= a;
                }
            } else {
                str = skipSpace(str + 1);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() > a;
                } else if (value.isString()) {
                    return value.size() > a;
                }
            }
            break;
        case '<':
            if (str[1] == '=') {
                str = skipSpace(str + 2);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() <= a;
                } else if (value.isString()) {
                    return value.size() <= a;
                }
            } else {
                str = skipSpace(str + 1);
                double a = atof(str);
                if (value.isNumber()) {
                    return value.to<double>() < a;
                } else if (value.isString()) {
                    return value.size() < a;
                }
            }
            break;
        case '=':
            if (str[1] == '=') {
                str = skipSpace(str + 2);
                double a = atof(str);
                if (value.isNumber()) {
                    return equare(value.to<double>(), a);
                } else if (value.isString()) {
                    return equare(value.size(), a);
                }
            } else {
                str = skipSpace(str + 1);
                double a = atof(str);
                if (value.isNumber()) {
                    return equare(value.to<double>(), a);
                } else if (value.isString()) {
                    return equare(value.size(), a);
                }
            }
            break;
        default:
            break;
    }
    return true;
}

inline bool isOptional(const char *opt) {
    opt = skipSpace(opt);
    return strCmp(opt, S_OPTINAL) == 0;
}

bool Validator::verifyValue(const Json &value, const char *opt) {
    opt = skipSpace(opt);

    // 解析optinal
    bool optinal = strCmp(opt, S_OPTINAL) == 0;
    if (optinal) {
        if (!value.valied()) {
            return true;
        }
        opt += CSTR_LEN(S_OPTINAL);
        opt = skipSpace(opt);
    }

    // 解析type
    if (strCmp(opt, S_INT) == 0) {
        if (!value.isNumber()) {
            error_ = std::string(value.name()) + " must int";
            return false;
        }
        opt += CSTR_LEN(S_INT);
    } else if (strCmp(opt, S_FLOAT) == 0) {
        if (!value.isNumber()) {
            error_ = std::string(value.name()) + " must float";
            return false;
        }
        opt += CSTR_LEN(S_FLOAT);
    } else if (strCmp(opt, S_BOOL) == 0) {
        if (!value.isBool()) {
            error_ = std::string(value.name()) + " must bool";
            return false;
        }
        opt += CSTR_LEN(S_BOOL);
    } else if (strCmp(opt, S_STRING) == 0) {
        if (!value.isString()) {
            error_ = std::string(value.name()) + " must string";
            return false;
        }
        opt += CSTR_LEN(S_STRING);
    }
    opt = skipSpace(opt);

    // 解析范围
    if (!checkRange(value, opt)) {
        error_ = std::string(value.name()) + " not in " + opt;
        return false;
    }
    return true;
}

bool Validator::validate(const Json &data, const Json &schema) {
    if (!data.valied()) {
        return false;
    }
    switch (schema.type()) {
        case Json::kArray:
            if (!data.isArray()) {
                error_ = std::string(data.name()) + " must array";
                return false;
            }

            {
                Json s = schema.at(0);
                if (s) {
                    for (auto item : data) {
                        if (!validate(item, s)) {
                            return false;
                        }
                    }
                }
            }
            break;
        case Json::kObject:
            if (!data.isObject()) {
                error_ = std::string(data.name()) + " must object";
                return false;
            }
            for (auto itr = schema.begin(); itr != schema.end(); ++itr) {
                Json child = data.at(itr.key());
                if (child.valied()) {
                    if (!validate(child, *itr)) return false;
                } else {
                    if (!isOptional((*itr).to<const char *>())) {
                        error_ = std::string(itr.key()) + " is not exist";
                        return false;
                    }
                }
            }
            break;
        case Json::kString:
            if (!verifyValue(data, schema.to<const char *>())) {
                return false;
            }
            break;
        default:
            error_ = "invalied schema value";
            return false;
    }
    return true;
}

}  // namespace MetaEngine::Json
