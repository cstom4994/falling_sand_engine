// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CJSON_WARPPER_H_
#define _METADOT_CJSON_WARPPER_H_

#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

#include "core/const.h"
#include "libs/cJSON.h"
#include "libs/cJSON_Utils.h"

namespace MetaEngine::Json {

class Json {
public:
    enum Type {
        kInvalid = cJSON_Invalid,
        kFalse = cJSON_False,
        kTrue = cJSON_True,
        kNull = cJSON_NULL,
        kNumber = cJSON_Number,
        kString = cJSON_String,
        kArray = cJSON_Array,
        kObject = cJSON_Object,
        kRaw = cJSON_Raw,
    };

    Json() : json_(cJSON_CreateNull()) {}

    Json(nullptr_t) : json_(cJSON_CreateNull()) {}

    Json(bool b) : json_(cJSON_CreateBool(b)) {}

    Json(int n) : json_(cJSON_CreateNumber(n)) {}

    Json(unsigned int n) : json_(cJSON_CreateNumber(n)) {}

    Json(long n) : json_(cJSON_CreateNumber(n)) {}

    Json(long long n) : json_(cJSON_CreateNumber(n)) {}

    Json(double n) : json_(cJSON_CreateNumber(n)) {}

    Json(const char* s) : json_(cJSON_CreateString(s)) {}

    // base 64
    Json(const uint8_t* s, size_t len);

    Json(const std::vector<uint8_t>& data) : Json(data.data(), data.size()) {}

    Json(const std::string& s) : json_(cJSON_CreateString(s.c_str())) {}

    Json(const std::initializer_list<Json>& v) : json_(cJSON_CreateArray()) {
        for (auto itm : v) {
            add(itm);
        }
    }

    Json(cJSON* j, bool ref = true) : json_(j) {
        if (ref) cJSON_Ref(json_);
    }

    Json(const Json& j) : json_(j.json_) { cJSON_Ref(json_); }

    Json(Json&& j) : json_(j.json_) { j.json_ = nullptr; }

    ~Json() { cJSON_Release(json_); }

    Json& operator=(const Json& j) {
        cJSON_Ref(j.json_);
        cJSON_Release(json_);
        json_ = j.json_;
        return *this;
    }

    static Json object() { return Json(cJSON_CreateObject(), false); }
    static Json array() { return Json(cJSON_CreateArray(), false); }
    static Json null() { return Json(); }

    Type type() const {
        if (json_) {
            return (Type)(json_->type & 0xff);
        } else {
            return kInvalid;
        } 
    }

    Json clone() const {
        cJSON* j = cJSON_Duplicate(json_, true);
        return Json(j, false);
    }

    template <class T>
    T to() const;

    static Json parse(const char* str) {
        cJSON* j = cJSON_Parse(str);
        return Json(j, false);
    }

    static Json parse(const std::string& str) {
        cJSON* j = cJSON_ParseWithLength(str.data(), str.length());
        return Json(j, false);
    }

    std::string print() const {
        char* s = cJSON_Print(json_);
        if (!s) {
            return "";
        }
        std::string s2(s);
        cJSON_free(s);
        return std::move(s2);
    }

    std::string dump() const {
        char* s = cJSON_PrintUnformatted(json_);
        if (!s) {
            return "";
        }
        std::string s2(s);
        cJSON_free(s);
        return std::move(s2);
    }

    bool isNumber() const { return cJSON_IsNumber(json_); }

    bool isString() const { return cJSON_IsString(json_); }

    bool isBool() const {
        auto t = type();
        return t == kTrue || t == kFalse;
    }

    bool isTrue() const { return cJSON_IsTrue(json_); }

    bool isFalse() const { return cJSON_IsFalse(json_); }

    bool isArray() const { return cJSON_IsArray(json_); }

    bool isObject() const { return cJSON_IsObject(json_); }

    bool isNull() const { return cJSON_IsNull(json_); }

    int size() const {
        switch (type()) {
            case kString:
                return strlen(json_->valuestring);
            case kArray:
            case kObject:
                return cJSON_GetArraySize(json_);
            default:
                return 0;
        }
    }

    Json at(int idx) const { return Json(cJSON_GetArrayItem(json_, idx)); }

    Json at(const char* key) const { return Json(cJSON_GetObjectItem(json_, key)); }

    Json operator[](int idx) const { return Json(cJSON_GetArrayItem(json_, idx)); }

    Json operator[](const char* key) const { return Json(cJSON_GetObjectItem(json_, key)); }

    const char* name() const { return json_->string ? json_->string : ""; }

    void add(const char* key, const Json& item) { cJSON_AddItemToObject(json_, key, item.json_); }

    void add(const Json& item) { cJSON_AddItemToArray(json_, item.json_); }

    void insert(int which, const Json& item) { cJSON_InsertItemInArray(json_, which, item.json_); }

    bool replace(int which, const Json& item) { return cJSON_ReplaceItemInArray(json_, which, item.json_); }

    bool replace(const char* key, const Json& item) { return cJSON_ReplaceItemInObject(json_, key, item.json_); }

    void replaceAdd(const char* key, const Json& item) {
        if (!replace(key, item)) {
            add(key, item);
        }
    }

    void remove(int which) { cJSON_DeleteItemFromArray(json_, which); }

    Json detach(int which) {
        cJSON* j = cJSON_DetachItemFromArray(json_, which);
        if (j) {
            return Json(j, false);
        } else {
            return Json();
        }
    }

    void remove(const char* key) { cJSON_DeleteItemFromObject(json_, key); }

    Json detach(const char* key) {
        cJSON* j = cJSON_DetachItemFromObject(json_, key);
        if (j) {
            return Json(j, false);
        } else {
            return Json();
        }
    }

    void removeCs(const char* key) { cJSON_DeleteItemFromObjectCaseSensitive(json_, key); }

    void removeAll() { cJSON_DeleteAllItem(json_); }

    bool operator==(const Json& ref) const {
        if (json_ == ref.json_) return true;
        return cJSON_Compare(json_, ref.json_, true);
    }

    bool operator!=(const Json& ref) const { return !cJSON_Compare(json_, ref.json_, true); }

    bool valied() const {
        switch (type()) {
            case kInvalid:
            case kNull:
                return false;
            default:
                return true;
        }
    }

    // �ж�list,object �Ƿ�Ϊ��
    bool empty() {
        switch (type()) {
            case kArray:
            case kObject:
                return json_->child == nullptr;
            case kString:
                return !json_->valuestring || !json_->valuestring[0];
            default:
                return true;
        }
    }

    operator bool() const {
        switch (type()) {
            case kInvalid:
            case kNull:
            case kFalse:
                return false;
            default:
                return true;
        }
    }

    class Iterator : public std::iterator<std::input_iterator_tag, cJSON> {
    public:
        Iterator(cJSON* j) : json_(j) {}

        Iterator(const Iterator& j) : json_(j.json_) {}

        const char* key() {
            if (json_)
                return json_->string;
            else
                return "";
        }

        Json operator*() { return Json(json_); }

        Iterator& operator++() {
            if (json_) json_ = json_->next;
            return *this;
        }

        Iterator operator++(int) {
            cJSON* json = json_;
            if (json) json_ = json->next;
            return Iterator(json);
        }

        bool operator==(const Iterator& rhs) const { return json_ == rhs.json_; }
        bool operator!=(const Iterator& rhs) const { return json_ != rhs.json_; }

    private:
        cJSON* json_;
    };

    Iterator begin() const {
        if (json_)
            return Iterator(json_->child);
        else
            return Iterator(nullptr);
    }

    Iterator end() const { return Iterator(nullptr); }

private:
    cJSON* json_;
};

template <>
inline double Json::to() const {
    if (type() == kNumber) {
        return json_->valuedouble;
    } else {
        return 0.0;
    }
}

template <>
inline float Json::to() const {
    if (type() == kNumber) {
        return (float)json_->valuedouble;
    } else {
        return 0.0f;
    }
}

template <>
inline int Json::to() const {
    if (type() == kNumber) {
        return json_->valueint;
    } else {
        return 0;
    }
}

template <>
inline const char* Json::to() const {
    if (type() == kString) {
        return json_->valuestring;
    } else {
        return "";
    }
}

template <>
inline std::string Json::to() const {
    if (type() == kString) {
        return json_->valuestring;
    } else {
        return "";
    }
}

template <>
inline bool Json::to() const {
    switch (type()) {
        case kTrue:
            return true;
        case kFalse:
            return false;
        case kString:
            return json_->valuestring[0] != '\0';
        case kNumber:
            return json_->valueint != 0;
        default:
            return false;
    }
}

enum {
    JSONRPC2_EPARSE = -32700,   // Parse error: Invalid JSON was received by the server
    JSONRPC2_EIREQ = -32600,    // Invalid Request: The JSON sent is not a valid Request object
    JSONRPC2_ENOMET = -32601,   // Method not found: The method doesn't exist/is not available
    JSONRPC2_EIPARAM = -32602,  // Invalid params: Invalid method parameter(s)
    JSONRPC2_EINTERN = -32603,  // Internal error: Internal JSON-RPC error
};

class Request {
public:
    Request(const std::string& method, const Json& params) : method_(method), params_(params) {}

    const std::string& method() const { return method_; }
    const Json& params() const { return params_; }
    Json param(const char* name) { return params_.at(name); }

private:
    std::string method_;
    Json params_;
};

class Response {
public:
    Response() {}

    Response(Json& data) : data_(data) {}

    void ok(const Json& result) {
        data_ = Json::object();
        data_.add("result", result);
    }

    void error(const Json& error) {
        data_ = Json::object();
        data_.add("error", error);
    }

    void error(int code, const char* msg) {
        data_ = Json::object();
        Json error = Json::object();
        error.add("code", Json(code));
        error.add("message", Json(msg));
        data_.add("error", error);
    }

    Json data() { return data_; }

    Json result() { return data_["result"]; }

    bool valied() { return data_.isObject(); }

private:
    Json data_;
};

typedef std::function<void(Request& req, Response& resp)> rpc_handler_t;
typedef std::function<void(Response& resp)> rpc_call_t;

class RpcSender {
public:
    virtual ~RpcSender() {}
    virtual void send(Json& js) = 0;
};

class Rpc {
public:
    Rpc();
    Rpc(const Rpc&) = delete;
    Rpc& operator=(const Rpc&) = delete;
    ~Rpc();

    void registerSender(RpcSender* sender);
    void registerHandler(const std::string& name, rpc_handler_t handler);

    bool recv(Json& js);

    void call(const Request& req, rpc_call_t oncall);
    void call(const std::string& method, Json params, rpc_call_t oncall);

private:
    // ����response json
    Json handleRequest(Json& req);
    bool handleResponse(Json& resp);

    RpcSender* sender_;
    int gen_id_;
    std::unordered_map<std::string, rpc_handler_t> handlers_;
    std::unordered_map<int, rpc_call_t> calls_;
};

class Validator {
public:
    Validator() {}
    ~Validator() {}

    bool validate(const Json& data, const Json& schema);

    const std::string& error() { return error_; }

private:
    bool verifyValue(const Json& value, const char* opt);
    std::string error_;
};

}  // namespace MetaEngine::Json

#endif