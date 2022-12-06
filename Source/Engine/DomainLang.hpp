// Metadot code copyright(c) 2022, KaoruXun All rights reserved.
// including some code by Garrett Skelton distributed under the MIT License

#ifndef _METADOT_DSL_HPP_
#define _METADOT_DSL_HPP_

#include "Core/Macros.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#pragma region

namespace MuDSL {
    struct Exception : public std::exception
    {
        std::string wh;
        Exception(const std::string &w) : wh("MuDSL Exception:" + w){};
    };

#pragma region PreSetting

    using std::string;
    using std::string_view;
    using std::vector;

    // Convert a string into a double
    inline double fromChars(const string &token) {
        double x;
#ifdef _MSC_VER
        // std::from_chars is amazing, but only works properly in MSVC
        std::from_chars(token.data(), token.data() + token.size(), x);
#else
        x = std::stod(token);
#endif
        return x;
    }

    inline double fromChars(string_view token) {
        double x;
#ifdef _MSC_VER
        // std::from_chars is amazing, but only works properly in MSVC
        std::from_chars(token.data(), token.data() + token.size(), x);
#else
        x = std::stod(string(token));
#endif
        return x;
    }

    // Does a collection contain a specific item?
    // works on strings, vectors, etc
    template<typename T, typename C>
    inline bool contains(const C &container, const T &element) {
        return std::find(container.begin(), container.end(), element) != container.end();
    }

    inline bool endswith(const string &v, const string &end) {
        if (end.size() > v.size()) { return false; }
        return equal(end.rbegin(), end.rend(), v.rbegin());
    }

    inline bool startswith(const string &v, const string &start) {
        if (start.size() > v.size()) { return false; }
        return equal(start.begin(), start.end(), v.begin());
    }

    inline vector<string> split(const string &input, const string &delimiter) {
        vector<string> ret;
        if (input.empty()) return ret;
        size_t pos = 0;
        size_t lpos = 0;
        auto dlen = delimiter.length();
        while ((pos = input.find(delimiter, lpos)) != string::npos) {
            ret.push_back(input.substr(lpos, pos - lpos));
            lpos = pos + dlen;
        }
        if (lpos < input.length()) { ret.push_back(input.substr(lpos, input.length())); }
        return ret;
    }

    inline vector<string_view> split(string_view input, char delimiter) {
        vector<string_view> ret;
        if (input.empty()) return ret;
        size_t pos = 0;
        size_t lpos = 0;
        while ((pos = input.find(delimiter, lpos)) != string::npos) {
            ret.push_back(input.substr(lpos, pos - lpos));
            lpos = pos + 1;
        }
        ret.push_back(input.substr(lpos, input.length()));
        return ret;
    }

    inline void replaceWhitespaceLiterals(string &input) {
        size_t pos = 0;
        size_t lpos = 0;
        while ((pos = input.find('\\', lpos)) != string::npos) {
            if (pos + 1 < input.size()) {
                switch (input[pos + 1]) {
                    case 't':
                        input.replace(pos, 2, "\t");
                        break;
                    case 'n':
                        input.replace(pos, 2, "\n");
                        break;
                    default:
                        break;
                }
            }
            lpos = pos;
        }
    }

    using std::function;
    using std::get;
    using std::make_shared;
    using std::max;
    using std::min;
    using std::shared_ptr;
    using std::string;
    using std::string_view;
    using std::unordered_map;
    using std::variant;
    using std::vector;
    using namespace std::string_literals;

#ifdef MuDSL_USE_32_BIT_NUMBERS
    using Int = int;
    using Float = float;
#else
    using Int = int64_t;
    using Float = double;
#endif// MuDSL_USE_32_BIT_TYPES

    // our basic Type flag
    enum class Type : uint8_t {
        // these are capital, otherwise they'd conflict with the c++ types of the same names
        Null = 0,// void
        Int,
        Float,
        Vec3,
        Function,
        UserPointer,
        String,
        Array,
        List,
        Dictionary,
        Class
    };

    // Get strings of type names for debugging and typeof function
    inline string getTypeName(Type t) {
        switch (t) {
            case Type::Null:
                return "null";
                break;
            case Type::Int:
                return "int";
                break;
            case Type::Float:
                return "float";
                break;
            case Type::Vec3:
                return "vec3";
                break;
            case Type::Function:
                return "function";
                break;
            case Type::UserPointer:
                return "userpointer";
                break;
            case Type::String:
                return "string";
                break;
            case Type::Array:
                return "array";
                break;
            case Type::List:
                return "list";
                break;
            case Type::Dictionary:
                return "dictionary";
                break;
            case Type::Class:
                return "class";
                break;
            default:
                return "unknown";
                break;
        }
    }

    inline size_t typeHashBits(Type type) { return ((size_t) INT32_MAX << ((size_t) type)); }

    // vec3 type is compatible with glm::vec3
    struct vec3
    {
        // data
        float x, y, z;

        // constructors
        constexpr vec3() : x(0), y(0), z(0) {}
        constexpr vec3(vec3 const &v) : x(v.x), y(v.y), z(v.z) {}
        constexpr vec3(float scalar) : x(scalar), y(scalar), z(scalar) {}
        constexpr vec3(float a, float b, float c) : x(a), y(b), z(c) {}

        // operators
        vec3 &operator=(vec3 const &v) {
            x = v.x;
            y = v.y;
            z = v.z;
            return *this;
        }
        vec3 &operator+=(float scalar) {
            x += scalar;
            y += scalar;
            z += scalar;
            return *this;
        }
        vec3 &operator+=(vec3 const &v) {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }
        vec3 &operator-=(float scalar) {
            x -= scalar;
            y -= scalar;
            z -= scalar;
            return *this;
        }
        vec3 &operator-=(vec3 const &v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }
        vec3 &operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }
        vec3 &operator*=(vec3 const &v) {
            x *= v.x;
            y *= v.y;
            z *= v.z;
            return *this;
        }
        vec3 &operator/=(float scalar) {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            return *this;
        }
        vec3 &operator/=(vec3 const &v) {
            x /= v.x;
            y /= v.y;
            z /= v.z;
            return *this;
        }
        vec3 &operator++() {
            ++x;
            ++y;
            ++z;
            return *this;
        }
        vec3 &operator--() {
            --x;
            --y;
            --z;
            return *this;
        }
        vec3 operator++(int) {
            vec3 r(*this);
            ++*this;
            return r;
        }
        vec3 operator--(int) {
            vec3 r(*this);
            --*this;
            return r;
        }
        // This can cast to glm::vec3
        template<typename T>
        operator T() {
            return T(x, y, z);
        }
    };
    // static vec3 operators
    inline vec3 operator+(vec3 const &v) { return v; }
    inline vec3 operator-(vec3 const &v) { return vec3(-v.x, -v.y, -v.z); }
    inline vec3 operator+(vec3 const &v, float scalar) {
        return vec3(v.x + scalar, v.y + scalar, v.z + scalar);
    }
    inline vec3 operator+(float scalar, vec3 const &v) {
        return vec3(v.x + scalar, v.y + scalar, v.z + scalar);
    }
    inline vec3 operator+(vec3 const &v1, vec3 const &v2) {
        return vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
    }
    inline vec3 operator-(vec3 const &v, float scalar) {
        return vec3(v.x - scalar, v.y - scalar, v.z - scalar);
    }
    inline vec3 operator-(float scalar, vec3 const &v) {
        return vec3(v.x - scalar, v.y - scalar, v.z - scalar);
    }
    inline vec3 operator-(vec3 const &v1, vec3 const &v2) {
        return vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
    }
    inline vec3 operator*(vec3 const &v, float scalar) {
        return vec3(v.x * scalar, v.y * scalar, v.z * scalar);
    }
    inline vec3 operator*(float scalar, vec3 const &v) {
        return vec3(v.x * scalar, v.y * scalar, v.z * scalar);
    }
    inline vec3 operator*(vec3 const &v1, vec3 const &v2) {
        return vec3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
    }
    inline vec3 operator/(vec3 const &v, float scalar) {
        return vec3(v.x / scalar, v.y / scalar, v.z / scalar);
    }
    inline vec3 operator/(float scalar, vec3 const &v) {
        return vec3(v.x / scalar, v.y / scalar, v.z / scalar);
    }
    inline vec3 operator/(vec3 const &v1, vec3 const &v2) {
        return vec3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
    }
    inline bool operator==(vec3 const &v1, vec3 const &v2) {
        return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
    }
    inline bool operator!=(vec3 const &v1, vec3 const &v2) {
        return v1.x != v2.x || v1.y != v2.y || v1.z != v2.z;
    }
    inline std::ostream &operator<<(std::ostream &os, vec3 const &v) {
        return (os << v.x << ", " << v.y << ", " << v.z);
    }

    // forward declare our value type so we can build abstract collections
    struct Value;

    // MuDSL uses shared_ptr for ref counting, anything with a name
    // like fooRef is a a shared_ptr to foo
    using ValueRef = shared_ptr<Value>;

    // Backing for the List type and for function arguments
    using List = vector<ValueRef>;

    // forward declare function for functions as values
    struct Function;
    using FunctionRef = shared_ptr<Function>;

    // variant allows us to have a union with a little more safety and ease
    using ArrayVariant = variant<vector<Int>, vector<Float>, vector<vec3>, vector<FunctionRef>,
                                 vector<void *>, vector<string>>;

    inline Type getArrayType(const ArrayVariant arr) {
        switch (arr.index()) {
            case 0:
                return Type::Int;
            case 1:
                return Type::Float;
            case 2:
                return Type::Vec3;
            case 3:
                return Type::Function;
            case 4:
                return Type::UserPointer;
            case 5:
                return Type::String;
            default:
                return Type::Null;
        }
    }

    // Backing for the Array type
    struct Array
    {
        ArrayVariant value;

        // constructors
        Array() { value = vector<Int>(); }
        Array(vector<Int> a) : value(a) {}
        Array(vector<Float> a) : value(a) {}
        Array(vector<vec3> a) : value(a) {}
        Array(vector<FunctionRef> a) : value(a) {}
        Array(vector<void *> a) : value(a) {}
        Array(vector<string> a) : value(a) {}
        Array(ArrayVariant a) : value(a) {}

        Type getType() const { return getArrayType(value); }

        template<typename T>
        vector<T> &getStdVector();

        bool operator==(const Array &o) const {
            if (size() != o.size()) { return false; }
            if (getType() != o.getType()) { return false; }
            switch (getType()) {
                case Type::Int: {
                    auto &aarr = get<vector<Int>>(value);
                    auto &barr = get<vector<Int>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) { return false; }
                    }
                } break;
                case Type::Float: {
                    auto &aarr = get<vector<Float>>(value);
                    auto &barr = get<vector<Float>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) { return false; }
                    }
                } break;
                case Type::Vec3: {
                    auto &aarr = get<vector<vec3>>(value);
                    auto &barr = get<vector<vec3>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) { return false; }
                    }
                } break;
                case Type::String: {
                    auto &aarr = get<vector<string>>(value);
                    auto &barr = get<vector<string>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) { return false; }
                    }
                } break;
                default:
                    break;
            }
            return true;
        }

        bool operator!=(const Array &o) const { return !(operator==(o)); }

        // doing these switches is tedious
        // so ideally they all get burried beneath template functions
        // and we get a nice clean api out the other end

        // get the size without caring about the underlying type
        size_t size() const {
            switch (getType()) {
                case Type::Null:
                    return get<vector<Int>>(value).size();
                    break;
                case Type::Int:
                    return get<vector<Int>>(value).size();
                    break;
                case Type::Float:
                    return get<vector<Float>>(value).size();
                    break;
                case Type::Vec3:
                    return get<vector<vec3>>(value).size();
                    break;
                case Type::Function:
                    return get<vector<FunctionRef>>(value).size();
                    break;
                case Type::UserPointer:
                    return get<vector<void *>>(value).size();
                    break;
                case Type::String:
                    return get<vector<string>>(value).size();
                    break;
                default:
                    return 0;
                    break;
            }
        }

        // Helper for push back
        template<typename T>
        void insert(const ArrayVariant &b) {
            auto &aa = get<vector<T>>(value);
            auto &bb = get<vector<T>>(b);
            aa.insert(aa.end(), bb.begin(), bb.end());
        }

        // Default is unsupported
        template<typename T>
        void push_back(const T &t) {
            throw Exception("Unsupported type");
        }

        // implementation for push_back int
        void push_back(const Int &t) {
            switch (getType()) {
                case Type::Null:
                case Type::Int:
                    get<vector<Int>>(value).push_back(t);
                    break;
                default:
                    throw Exception("Unsupported type");
                    break;
            }
        }

        // implementation for push_back Float
        void push_back(const Float &t) {
            switch (getType()) {
                case Type::Float:
                    get<vector<Float>>(value).push_back(t);
                    break;
                default:
                    throw Exception("Unsupported type");
                    break;
            }
        }

        // implementation for push_back vec3
        void push_back(const vec3 &t) {
            switch (getType()) {
                case Type::Vec3:
                    get<vector<vec3>>(value).push_back(t);
                    break;
                default:
                    throw Exception("Unsupported type");
                    break;
            }
        }

        // implementation for push_back string
        void push_back(const string &t) {
            switch (getType()) {
                case Type::String:
                    get<vector<string>>(value).push_back(t);
                    break;
                default:
                    throw Exception("Unsupported type");
                    break;
            }
        }

        // implementation for push_back function
        void push_back(const FunctionRef &t) {
            switch (getType()) {
                case Type::Function:
                    get<vector<FunctionRef>>(value).push_back(t);
                    break;
                default:
                    throw Exception("Unsupported type");
                    break;
            }
        }

        // implementation for push_back function
        void push_back(void *t) {
            switch (getType()) {
                case Type::UserPointer:
                    get<vector<void *>>(value).push_back(t);
                    break;
                default:
                    throw Exception("Unsupported type");
                    break;
            }
        }

        // implementation for push_back another array
        void push_back(const Array &barr) {
            if (getType() == barr.getType()) {
                switch (getType()) {
                    case Type::Int:
                        insert<Int>(barr.value);
                        break;
                    case Type::Float:
                        insert<Float>(barr.value);
                        break;
                    case Type::Vec3:
                        insert<vec3>(barr.value);
                        break;
                    case Type::Function:
                        insert<FunctionRef>(barr.value);
                        break;
                    case Type::UserPointer:
                        insert<void *>(barr.value);
                        break;
                    case Type::String:
                        insert<string>(barr.value);
                        break;
                    default:
                        break;
                }
            }
        }

        void pop_back() {
            switch (getType()) {
                case Type::Null:
                case Type::Int:
                    get<vector<Int>>(value).pop_back();
                    break;
                case Type::Float:
                    get<vector<Float>>(value).pop_back();
                    break;
                case Type::Vec3:
                    get<vector<vec3>>(value).pop_back();
                    break;
                case Type::Function:
                    get<vector<FunctionRef>>(value).pop_back();
                    break;
                case Type::UserPointer:
                    get<vector<void *>>(value).pop_back();
                    break;
                case Type::String:
                    get<vector<string>>(value).pop_back();
                    break;
                default:
                    break;
            }
        }
    };

    using Dictionary = unordered_map<size_t, ValueRef>;
    using DictionaryRef = shared_ptr<Dictionary>;

    struct Scope;
    using ScopeRef = shared_ptr<Scope>;

    struct Class
    {
        // this is the main storage object for all functions and variables
        string name;
        unordered_map<string, ValueRef> variables;
        ScopeRef functionScope;
#ifndef MuDSL_THREAD_UNSAFE
        std::mutex varInsert;
#endif

        Class(const string &name_) : name(name_) {}
        Class(const string &name_, const unordered_map<string, ValueRef> &variables_)
            : name(name_), variables(variables_) {}
        Class(const Class &o);
        Class(const ScopeRef &o);
        ~Class();

        ValueRef &insertVar(const string &n, ValueRef val) {
#ifndef MuDSL_THREAD_UNSAFE
            auto l = std::unique_lock(varInsert);
#endif
            auto &ref = variables[n];
            ref = val;
            return ref;
        }
    };

    using ClassRef = shared_ptr<Class>;

    // operator precedence for pemdas
    enum class OperatorPrecedence : int {
        assign = 0,
        boolean,
        compare,
        addsub,
        muldiv,
        incdec,
        func
    };

    // Lambda is a "native function" it's how you wrap c++ code for use inside MuDSL
    using Lambda = function<ValueRef(const List &)>;
    using ScopedLambda = function<ValueRef(ScopeRef, const List &)>;
    using ClassLambda = function<ValueRef(Class *, ScopeRef, const List &)>;

    // forward declare so we can cross refernce types
    // Expression is a 'generic' expression
    struct Expression;
    using ExpressionRef = shared_ptr<Expression>;

    enum class FunctionType : uint8_t {
        free,
        constructor,
        member,
    };

    using FunctionBodyVariant = variant<vector<ExpressionRef>, Lambda, ScopedLambda, ClassLambda>;

    enum class FunctionBodyType : uint8_t {
        Subexpressions,
        Lambda,
        ScopedLambda,
        ClassLambda
    };

    // our basic function type
    struct Function
    {
        OperatorPrecedence opPrecedence;
        FunctionType type = FunctionType::free;
        string name;
        vector<string> argNames;

        FunctionBodyVariant body;

        FunctionBodyType getBodyType() { return static_cast<FunctionBodyType>(body.index()); }

        static OperatorPrecedence getPrecedence(const string &n) {
            if (n.size() > 2) { return OperatorPrecedence::func; }
            if (n == "||" || n == "&&") { return OperatorPrecedence::boolean; }
            if (n == "++" || n == "--") { return OperatorPrecedence::incdec; }
            if (contains("!<>|&"s, n[0]) || n == "==") { return OperatorPrecedence::compare; }
            if (contains(n, '=')) { return OperatorPrecedence::assign; }
            if (contains("/*%"s, n[0])) { return OperatorPrecedence::muldiv; }
            if (contains("-+"s, n[0])) { return OperatorPrecedence::addsub; }
            return OperatorPrecedence::func;
        }

        Function(const string &name_, const Lambda &l)
            : name(name_), opPrecedence(getPrecedence(name_)), body(l) {}
        Function(const string &name_, const ScopedLambda &l)
            : name(name_), opPrecedence(getPrecedence(name_)), body(l) {}
        Function(const string &name_, const ClassLambda &l)
            : name(name_), opPrecedence(getPrecedence(name_)), body(l) {}
        // when using a MuDSL function body
        // the operator precedence will always be "func" level (aka the highest)
        Function(const string &name_, const vector<string> &argNames_,
                 const vector<ExpressionRef> &body_)
            : name(name_), body(body_), argNames(argNames_),
              opPrecedence(OperatorPrecedence::func) {}
        Function(const string &name_, const vector<string> &argNames_)
            : Function(name_, argNames_, {}) {}
        // default constructor makes a function with no args that returns void
        Function(const string &name) : Function(name, [](List) { return make_shared<Value>(); }) {}
        Function() : name("__anon") {}
        Function(const Function &o) = default;
    };

    struct Null
    {
    };
    // Now that we have our collection types defined, we can finally define our value variant
    using ValueVariant = variant<Null, Int, Float, vec3, FunctionRef, void *, string, Array, List,
                                 DictionaryRef, ClassRef>;

    // our basic Object/Value type
    struct Value
    {
        ValueVariant value;

        // Construct a Value from any underlying type
        explicit Value() : value(Null{}) {}
        explicit Value(bool a) : value(static_cast<Int>(a)) {}
        explicit Value(Int a) : value(a) {}
        explicit Value(Float a) : value(a) {}
        explicit Value(vec3 a) : value(a) {}
        explicit Value(FunctionRef a) : value(a) {}
        explicit Value(void *a) : value(a) {}
        explicit Value(string a) : value(a) {}
        explicit Value(const char *a) : value(string(a)) {}
        explicit Value(Array a) : value(a) {}
        explicit Value(List a) : value(a) {}
        explicit Value(DictionaryRef a) : value(a) {}
        explicit Value(ClassRef a) : value(a) {}
        explicit Value(ValueVariant a) : value(a) {}
        explicit Value(ValueRef o) : value(o->value) {}
        ~Value(){};

        Type getType() const { return static_cast<Type>(value.index()); }

        // get a string that represents this value
        string getPrintString() const {
            auto t = *this;
            t.hardconvert(Type::String);
            return get<string>(t.value);
        }

        // get this value as an int
        Int &getInt() { return get<Int>(value); }

        // get this value as a float
        Float &getFloat() { return get<Float>(value); }

        // get this value as a vec3
        vec3 &getVec3() { return get<vec3>(value); }

        // get this value as a function
        FunctionRef &getFunction() { return get<FunctionRef>(value); }

        // get this value as a function
        void *&getPointer() { return get<void *>(value); }

        // get this value as a string
        string &getString() { return get<string>(value); }

        // get this value as an array
        Array &getArray() { return get<Array>(value); }

        // get this value as an std::vector<T>
        template<typename T>
        vector<T> &getStdVector();

        // get this value as a list
        List &getList() { return get<List>(value); }

        DictionaryRef &getDictionary() { return get<DictionaryRef>(value); }

        ClassRef &getClass() { return get<ClassRef>(value); }

        // get a boolean representing the truthiness of this value
        bool getBool() {
            // non zero or "true" are true
            bool truthiness = false;
            switch (getType()) {
                case Type::Int:
                    truthiness = getInt();
                    break;
                case Type::Float:
                    truthiness = getFloat() != 0;
                    break;
                case Type::Vec3:
                    truthiness = getVec3() != vec3(0);
                    break;
                case Type::String:
                    truthiness = getString().size() > 0;
                    break;
                case Type::Array:
                    truthiness = getArray().size() > 0;
                    break;
                case Type::List:
                    truthiness = getList().size() > 0;
                    break;
                default:
                    break;
            }
            return truthiness;
        }

        size_t getHash();

        // convert this value up to the newType
        void upconvert(Type newType);

        // convert this value to the newType even if it's a downcast
        void hardconvert(Type newType);
    };

    // cout << operators for examples

    // define cout operator for List
    inline std::ostream &operator<<(std::ostream &os, const Null &) {
        os << "null";

        return os;
    }

    // define cout operator for List
    inline std::ostream &operator<<(std::ostream &os, const List &values) {
        os << Value(values).getPrintString();

        return os;
    }

    // define cout operator for Array
    inline std::ostream &operator<<(std::ostream &os, const Array &arr) {
        os << Value(arr).getPrintString();

        return os;
    }

    // define cout operator for Dictionary
    inline std::ostream &operator<<(std::ostream &os, const DictionaryRef &dict) {
        os << Value(dict).getPrintString();

        return os;
    }

    // define cout operator for Class
    inline std::ostream &operator<<(std::ostream &os, const ClassRef &dict) {
        os << Value(dict).getPrintString();

        return os;
    }

    // functions for working with Values

    // makes both values have matching types
    inline void upconvert(Value &a, Value &b) {
        if (a.getType() != b.getType()) {
            if (a.getType() < b.getType()) {
                a.upconvert(b.getType());
            } else {
                b.upconvert(a.getType());
            }
        }
    }

    // makes both values have matching types but doesn't allow converting between numbers and non-numbers
    inline void upconvertThrowOnNonNumberToNumberCompare(Value &a, Value &b) {
        if (a.getType() != b.getType()) {
            if (max((int) a.getType(), (int) b.getType()) > (int) Type::Vec3) {
                throw Exception("Types `"s + getTypeName(a.getType()) + " " + a.getPrintString() +
                                "` and `" + getTypeName(b.getType()) + " " + b.getPrintString() +
                                "` are incompatible for this operation");
            }
            if (a.getType() < b.getType()) {
                a.upconvert(b.getType());
            } else {
                b.upconvert(a.getType());
            }
        }
    }

    // math operators
    inline Value operator+(Value a, Value b) {
        upconvert(a, b);
        switch (a.getType()) {
            case Type::Int:
                return Value{a.getInt() + b.getInt()};
                break;
            case Type::Float:
                return Value{a.getFloat() + b.getFloat()};
                break;
            case Type::Vec3:
                return Value{a.getVec3() + b.getVec3()};
                break;
            case Type::String:
                return Value{a.getString() + b.getString()};
                break;
            case Type::Array: {
                auto arr = Array(a.getArray());
                arr.push_back(b.getArray());
                return Value{arr};
            } break;
            case Type::List: {
                auto list = List(a.getList());
                auto &blist = b.getList();
                list.insert(list.end(), blist.begin(), blist.end());
                return Value{list};
            } break;
            case Type::Dictionary: {
                auto list = make_shared<Dictionary>(*a.getDictionary());
                list->merge(*b.getDictionary());
                return Value{list};
            } break;
            default:
                throw Exception("Operator + not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
    }

    inline Value operator-(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return Value{a.getInt() - b.getInt()};
                break;
            case Type::Float:
                return Value{a.getFloat() - b.getFloat()};
                break;
            case Type::Vec3:
                return Value{a.getVec3() - b.getVec3()};
                break;
            default:
                throw Exception("Operator - not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
    }

    inline Value operator*(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return Value{a.getInt() * b.getInt()};
                break;
            case Type::Float:
                return Value{a.getFloat() * b.getFloat()};
                break;
            case Type::Vec3:
                return Value{a.getVec3() * b.getVec3()};
                break;
            default:
                throw Exception("Operator * not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
    }

    inline Value operator/(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return Value{a.getInt() / b.getInt()};
                break;
            case Type::Float:
                return Value{a.getFloat() / b.getFloat()};
                break;
            case Type::Vec3:
                return Value{a.getVec3() / b.getVec3()};
                break;
            default:
                throw Exception("Operator / not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
    }

    inline Value operator+=(Value &a, Value b) {
        if ((int) a.getType() < (int) Type::Array || b.getType() == Type::List) { upconvert(a, b); }
        switch (a.getType()) {
            case Type::Int:
                a.getInt() += b.getInt();
                break;
            case Type::Float:
                a.getFloat() += b.getFloat();
                break;
            case Type::Vec3:

                a.getVec3() += b.getVec3();
                break;
            case Type::String:
                a.getString() += b.getString();
                break;
            case Type::Array: {
                auto &arr = a.getArray();
                if (arr.getType() == b.getType() ||
                    (b.getType() == Type::Array && b.getArray().getType() == arr.getType())) {
                    switch (b.getType()) {
                        case Type::Int:
                            arr.push_back(b.getInt());
                            break;
                        case Type::Float:
                            arr.push_back(b.getFloat());
                            break;
                        case Type::Vec3:
                            arr.push_back(b.getVec3());
                            break;
                        case Type::Function:
                            arr.push_back(b.getFunction());
                            break;
                        case Type::String:
                            arr.push_back(b.getString());
                            break;
                        case Type::UserPointer:
                            arr.push_back(b.getPointer());
                            break;
                        case Type::Array:
                            arr.push_back(b.getArray());
                            break;
                        default:
                            break;
                    }
                }
                return Value{arr};
            } break;
            case Type::List: {
                auto &list = a.getList();
                switch (b.getType()) {
                    case Type::Int:
                    case Type::Float:
                    case Type::Vec3:
                    case Type::Function:
                    case Type::String:
                    case Type::UserPointer:
                        list.push_back(make_shared<Value>(b));
                        break;
                    default: {
                        b.upconvert(Type::List);
                        auto &blist = b.getList();
                        list.insert(list.end(), blist.begin(), blist.end());
                    } break;
                }
            } break;
            case Type::Dictionary: {
                auto &dict = a.getDictionary();
                b.upconvert(Type::Dictionary);
                dict->merge(*b.getDictionary());
            } break;
            default:
                throw Exception("Operator += not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
        return a;
    }

    inline Value operator-=(Value &a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                a.getInt() -= b.getInt();
                break;
            case Type::Float:
                a.getFloat() -= b.getFloat();
                break;
            case Type::Vec3:
                a.getVec3() -= b.getVec3();
                break;
            default:
                throw Exception("Operator -= not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
        return a;
    }

    inline Value operator*=(Value &a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                a.getInt() *= b.getInt();
                break;
            case Type::Float:
                a.getFloat() *= b.getFloat();
                break;
            case Type::Vec3:
                a.getVec3() *= b.getVec3();
                break;
            default:
                throw Exception("Operator *= not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
        return a;
    }

    inline Value operator/=(Value &a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                a.getInt() /= b.getInt();
                break;
            case Type::Float:
                a.getFloat() /= b.getFloat();
                break;
            case Type::Vec3:
                a.getVec3() /= b.getVec3();
                break;
            default:
                throw Exception("Operator /= not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
        return a;
    }

    inline Value operator%(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return Value{a.getInt() % b.getInt()};
                break;
            case Type::Float:
                return Value{std::fmod(a.getFloat(), b.getFloat())};
                break;
            default:
                throw Exception("Operator %% not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
    }

    // comparison operators
    bool operator!=(Value a, Value b);
    inline bool operator==(Value a, Value b) {
        if (a.getType() != b.getType()) { return false; }
        switch (a.getType()) {
            case Type::Null:
                return true;
            case Type::Int:
                return a.getInt() == b.getInt();
                break;
            case Type::Float:
                return a.getFloat() == b.getFloat();
                break;
            case Type::Vec3:
                return a.getVec3() == b.getVec3();
                break;
            case Type::String:
                return a.getString() == b.getString();
                break;
            case Type::Array:
                return a.getArray() == b.getArray();
                break;
            case Type::List: {
                auto &alist = a.getList();
                auto &blist = b.getList();
                if (alist.size() != blist.size()) { return false; }
                for (size_t i = 0; i < alist.size(); ++i) {
                    if (*alist[i] != *blist[i]) { return false; }
                }
                return true;
            } break;
            default:
                throw Exception("Operator == not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
        return true;
    }

    inline bool operator!=(Value a, Value b) {
        if (a.getType() != b.getType()) { return true; }
        switch (a.getType()) {
            case Type::Null:
                return false;
            case Type::Int:
                return a.getInt() != b.getInt();
                break;
            case Type::Float:
                return a.getFloat() != b.getFloat();
                break;
            case Type::Vec3:
                return a.getVec3() != b.getVec3();
                break;
            case Type::String:
                return a.getString() != b.getString();
                break;
            case Type::Array:
            case Type::List:
                return !(a == b);
                break;
            default:
                throw Exception("Operator != not defined for type `"s + getTypeName(a.getType()) +
                                "`");
                break;
        }
        return false;
    }

    inline bool operator||(Value &a, Value &b) { return a.getBool() || b.getBool(); }

    inline bool operator&&(Value &a, Value &b) { return a.getBool() && b.getBool(); }

    inline bool operator<(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return a.getInt() < b.getInt();
                break;
            case Type::Float:
                return a.getFloat() < b.getFloat();
                break;
            case Type::String:
                return a.getString() < b.getString();
                break;
            case Type::Array:
                return a.getArray().size() < b.getArray().size();
                break;
            case Type::List:
                return a.getList().size() < b.getList().size();
                break;
            case Type::Dictionary:
                return a.getDictionary()->size() < b.getDictionary()->size();
                break;
            default:
                break;
        }
        return false;
    }

    inline bool operator>(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return a.getInt() > b.getInt();
                break;
            case Type::Float:
                return a.getFloat() > b.getFloat();
                break;
            case Type::String:
                return a.getString() > b.getString();
                break;
            case Type::Array:
                return a.getArray().size() > b.getArray().size();
                break;
            case Type::List:
                return a.getList().size() > b.getList().size();
                break;
            case Type::Dictionary:
                return a.getDictionary()->size() > b.getDictionary()->size();
                break;
            default:
                break;
        }
        return false;
    }

    inline bool operator<=(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return a.getInt() <= b.getInt();
                break;
            case Type::Float:
                return a.getFloat() <= b.getFloat();
                break;
            case Type::String:
                return a.getString() <= b.getString();
                break;
            case Type::Array:
                return a.getArray().size() <= b.getArray().size();
                break;
            case Type::List:
                return a.getList().size() <= b.getList().size();
                break;
            case Type::Dictionary:
                return a.getDictionary()->size() <= b.getDictionary()->size();
                break;
            default:
                break;
        }
        return false;
    }

    inline bool operator>=(Value a, Value b) {
        upconvertThrowOnNonNumberToNumberCompare(a, b);
        switch (a.getType()) {
            case Type::Int:
                return a.getInt() >= b.getInt();
                break;
            case Type::Float:
                return a.getFloat() >= b.getFloat();
                break;
            case Type::String:
                return a.getString() >= b.getString();
                break;
            case Type::Array:
                return a.getArray().size() >= b.getArray().size();
                break;
            case Type::List:
                return a.getList().size() >= b.getList().size();
                break;
            case Type::Dictionary:
                return a.getDictionary()->size() >= b.getDictionary()->size();
                break;
            default:
                break;
        }
        return false;
    }

#pragma endregion PreSetting

    // describes an expression tree with a function at the root
    struct FunctionExpression
    {
        ValueRef function;
        vector<ExpressionRef> subexpressions;

        FunctionExpression(const FunctionExpression &o) {
            function = o.function;
            for (auto sub: o.subexpressions) {
                subexpressions.push_back(make_shared<Expression>(*sub));
            }
        }
        FunctionExpression(FunctionRef fnc) : function(new Value(fnc)) {}
        FunctionExpression(ValueRef fncvalue) : function(fncvalue) {}
        FunctionExpression() {}

        void clear() { subexpressions.clear(); }

        ~FunctionExpression() { clear(); }
    };

    struct MemberVariable
    {
        ExpressionRef object;
        string name;

        MemberVariable(const MemberVariable &o) {
            object = o.object;
            name = o.name;
        }
        MemberVariable(ExpressionRef ob, const string &name_) : object(ob), name(name_) {}
        MemberVariable() {}
    };

    struct MemberFunctionCall
    {
        ExpressionRef object;
        string functionName;
        vector<ExpressionRef> subexpressions;

        MemberFunctionCall(const MemberFunctionCall &o) {
            object = o.object;
            functionName = o.functionName;
            for (auto sub: o.subexpressions) {
                subexpressions.push_back(make_shared<Expression>(*sub));
            }
        }
        MemberFunctionCall(ExpressionRef ob, const string &fncvalue,
                           const vector<ExpressionRef> &sub)
            : object(ob), functionName(fncvalue), subexpressions(sub) {}
        MemberFunctionCall() {}

        void clear() { subexpressions.clear(); }

        ~MemberFunctionCall() { clear(); }
    };

    struct Return
    {
        ExpressionRef expression;

        Return(const Return &o) {
            expression = o.expression ? make_shared<Expression>(*o.expression) : nullptr;
        }
        Return(ExpressionRef e) : expression(e) {}
        Return() {}
    };

    struct If
    {
        ExpressionRef testExpression;
        vector<ExpressionRef> subexpressions;

        If(const If &o) {
            testExpression =
                    o.testExpression ? make_shared<Expression>(*o.testExpression) : nullptr;
            for (auto sub: o.subexpressions) {
                subexpressions.push_back(make_shared<Expression>(*sub));
            }
        }
        If() {}
    };

    using IfElse = vector<If>;

    struct Loop
    {
        ExpressionRef initExpression;
        ExpressionRef testExpression;
        ExpressionRef iterateExpression;
        vector<ExpressionRef> subexpressions;

        Loop(const Loop &o) {
            initExpression =
                    o.initExpression ? make_shared<Expression>(*o.initExpression) : nullptr;
            testExpression =
                    o.testExpression ? make_shared<Expression>(*o.testExpression) : nullptr;
            iterateExpression =
                    o.iterateExpression ? make_shared<Expression>(*o.iterateExpression) : nullptr;
            for (auto sub: o.subexpressions) {
                subexpressions.push_back(make_shared<Expression>(*sub));
            }
        }
        Loop() {}
    };

    struct Foreach
    {
        ExpressionRef listExpression;
        string iterateName;
        vector<ExpressionRef> subexpressions;

        Foreach(const Foreach &o) {
            listExpression =
                    o.listExpression ? make_shared<Expression>(*o.listExpression) : nullptr;
            iterateName = o.iterateName;
            for (auto sub: o.subexpressions) {
                subexpressions.push_back(make_shared<Expression>(*sub));
            }
        }
        Foreach() {}
    };

    struct ResolveVar
    {
        string name;

        ResolveVar(const ResolveVar &o) { name = o.name; }
        ResolveVar() {}
        ResolveVar(const string &n) : name(n) {}
    };

    struct DefineVar
    {
        string name;
        ExpressionRef defineExpression;

        DefineVar(const DefineVar &o) {
            name = o.name;
            defineExpression =
                    o.defineExpression ? make_shared<Expression>(*o.defineExpression) : nullptr;
        }
        DefineVar() {}
        DefineVar(const string &n) : name(n) {}
        DefineVar(const string &n, ExpressionRef defExpr) : name(n), defineExpression(defExpr) {}
    };

    enum class ExpressionType : uint8_t {
        Value,
        ResolveVar,
        DefineVar,
        FunctionDef,
        FunctionCall,
        MemberFunctionCall,
        MemberVariable,
        Return,
        Loop,
        ForEach,
        IfElse
    };

    using ExpressionVariant =
            variant<ValueRef, ResolveVar, DefineVar, FunctionExpression, MemberFunctionCall,
                    MemberVariable, Return, Loop, Foreach, IfElse>;

    // forward declare so we can use the parser to process functions
    class MuDSLInterpreter;
    // describes a 'generic' expression tree, with either a value or function at the root
    struct Expression
    {
        // either we have a value, or a function expression which then has sub-expressions
        ExpressionVariant expression;
        ExpressionType type;
        ExpressionRef parent = nullptr;

        Expression(ValueRef val)
            : type(ExpressionType::FunctionCall), expression(FunctionExpression(val)),
              parent(nullptr) {}

        Expression(ExpressionRef obj, const string &name)
            : type(ExpressionType::MemberVariable), expression(MemberVariable(obj, name)),
              parent(nullptr) {}
        Expression(ExpressionRef obj, const string &name, const vector<ExpressionRef> subs)
            : type(ExpressionType::MemberFunctionCall),
              expression(MemberFunctionCall(obj, name, subs)), parent(nullptr) {}
        Expression(FunctionRef val, ExpressionRef par)
            : type(ExpressionType::FunctionDef), expression(FunctionExpression(val)), parent(par) {}
        Expression(ValueRef val, ExpressionRef par)
            : type(ExpressionType::Value), expression(val), parent(par) {}
        Expression(Foreach val, ExpressionRef par = nullptr)
            : type(ExpressionType::ForEach), expression(val), parent(par) {}
        Expression(Loop val, ExpressionRef par = nullptr)
            : type(ExpressionType::Loop), expression(val), parent(par) {}
        Expression(IfElse val, ExpressionRef par = nullptr)
            : type(ExpressionType::IfElse), expression(val), parent(par) {}
        Expression(Return val, ExpressionRef par = nullptr)
            : type(ExpressionType::Return), expression(val), parent(par) {}
        Expression(ResolveVar val, ExpressionRef par = nullptr)
            : type(ExpressionType::ResolveVar), expression(val), parent(par) {}
        Expression(DefineVar val, ExpressionRef par = nullptr)
            : type(ExpressionType::DefineVar), expression(val), parent(par) {}

        Expression(ExpressionVariant val, ExpressionType ty) : type(ty), expression(val) {}

        ExpressionRef back() {
            switch (type) {
                case ExpressionType::FunctionDef:
                    return get<vector<ExpressionRef>>(get<FunctionExpression>(expression)
                                                              .function->getFunction()
                                                              ->body)
                            .back();
                    break;
                case ExpressionType::FunctionCall:
                    return get<FunctionExpression>(expression).subexpressions.back();
                    break;
                case ExpressionType::Loop:
                    return get<Loop>(expression).subexpressions.back();
                    break;
                case ExpressionType::ForEach:
                    return get<Foreach>(expression).subexpressions.back();
                    break;
                case ExpressionType::IfElse:
                    return get<IfElse>(expression).back().subexpressions.back();
                    break;
                default:
                    break;
            }
            return nullptr;
        }

        auto begin() {
            switch (type) {
                case ExpressionType::FunctionCall:
                    return get<FunctionExpression>(expression).subexpressions.begin();
                    break;
                case ExpressionType::FunctionDef:
                    return get<vector<ExpressionRef>>(get<FunctionExpression>(expression)
                                                              .function->getFunction()
                                                              ->body)
                            .begin();
                    break;
                case ExpressionType::Loop:
                    return get<Loop>(expression).subexpressions.begin();
                    break;
                case ExpressionType::ForEach:
                    return get<Foreach>(expression).subexpressions.begin();
                    break;
                case ExpressionType::IfElse:
                    return get<IfElse>(expression).back().subexpressions.begin();
                    break;
                default:
                    return vector<ExpressionRef>::iterator();
                    break;
            }
        }

        auto end() {
            switch (type) {
                case ExpressionType::FunctionCall:
                    return get<FunctionExpression>(expression).subexpressions.end();
                    break;
                case ExpressionType::FunctionDef:
                    return get<vector<ExpressionRef>>(get<FunctionExpression>(expression)
                                                              .function->getFunction()
                                                              ->body)
                            .end();
                    break;
                case ExpressionType::Loop:
                    return get<Loop>(expression).subexpressions.end();
                    break;
                case ExpressionType::ForEach:
                    return get<Foreach>(expression).subexpressions.end();
                    break;
                case ExpressionType::IfElse:
                    return get<IfElse>(expression).back().subexpressions.end();
                    break;
                default:
                    return vector<ExpressionRef>::iterator();
                    break;
            }
        }

        void push_back(ExpressionRef ref) {
            switch (type) {
                case ExpressionType::FunctionCall:
                    get<FunctionExpression>(expression).subexpressions.push_back(ref);
                    break;
                case ExpressionType::FunctionDef:
                    get<vector<ExpressionRef>>(
                            get<FunctionExpression>(expression).function->getFunction()->body)
                            .push_back(ref);
                    break;
                case ExpressionType::Loop:
                    get<Loop>(expression).subexpressions.push_back(ref);
                    break;
                case ExpressionType::ForEach:
                    get<Foreach>(expression).subexpressions.push_back(ref);
                    break;
                case ExpressionType::IfElse:
                    get<IfElse>(expression).back().subexpressions.push_back(ref);
                    break;
                default:
                    break;
            }
        }

        void push_back(const If &ref) {
            switch (type) {
                case ExpressionType::IfElse:
                    get<IfElse>(expression).push_back(ref);
                    break;
                default:
                    break;
            }
        }
    };

    using ModulePrivilegeFlags = uint8_t;

    // bitfield for privileges
    enum class ModulePrivilege : ModulePrivilegeFlags {
        allPrivilege = static_cast<ModulePrivilegeFlags>(-1),
        unrestricted = 0,
        localFolderRead = 1,
        localFolderWrite = 2,
        fileSystemRead = 4,
        fileSystemWrite = 8,
        localNetwork = 16,
        internet = 32,
        experimental = 64,
    };

    inline ModulePrivilegeFlags operator|(const ModulePrivilege ours, const ModulePrivilege other) {
        return static_cast<ModulePrivilegeFlags>(ours) | static_cast<ModulePrivilegeFlags>(other);
    }

    inline ModulePrivilegeFlags operator^(const ModulePrivilege ours, const ModulePrivilege other) {
        return static_cast<ModulePrivilegeFlags>(ours) ^ static_cast<ModulePrivilegeFlags>(other);
    }

    inline ModulePrivilegeFlags operator&(const ModulePrivilege ours, const ModulePrivilege other) {
        return static_cast<ModulePrivilegeFlags>(ours) & static_cast<ModulePrivilegeFlags>(other);
    }

    inline ModulePrivilegeFlags operator|(const ModulePrivilege ours,
                                          const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) | other;
    }

    inline ModulePrivilegeFlags operator^(const ModulePrivilege ours,
                                          const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) ^ other;
    }

    inline ModulePrivilegeFlags operator&(const ModulePrivilege ours,
                                          const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) & other;
    }

    inline bool shouldAllow(const ModulePrivilegeFlags allowPolicy,
                            const ModulePrivilegeFlags requiredPermissions) {
        return 0 == ((!allowPolicy) & requiredPermissions);
    }

    struct Module
    {
        ModulePrivilegeFlags requiredPermissions;
        ScopeRef scope;
        Module(ModulePrivilegeFlags f, ScopeRef s) : requiredPermissions(f), scope(s) {}
    };

    struct Scope
    {
        // this is the main storage object for all functions and variables
        string name;
        ScopeRef parent;
        MuDSLInterpreter *host;
#ifndef MuDSL_THREAD_UNSAFE
        std::mutex varInsert;
        std::mutex scopeInsert;
        std::mutex fncInsert;
#endif
        unordered_map<string, ValueRef> variables;
        unordered_map<string, ScopeRef> scopes;
        unordered_map<string, FunctionRef> functions;
        bool isClassScope = false;

        ValueRef &insertVar(const string &n, ValueRef val) {
#ifndef MuDSL_THREAD_UNSAFE
            auto l = std::unique_lock(varInsert);
#endif
            auto &ref = variables[n];
            ref = val;

            return ref;
        }

        ScopeRef insertScope(ScopeRef val) {
#ifndef MuDSL_THREAD_UNSAFE
            auto l = std::unique_lock(scopeInsert);
#endif
            scopes[val->name] = val;
            return val;
        }

        Scope(MuDSLInterpreter *interpereter)
            : name("global"), parent(nullptr), host(interpereter) {}
        Scope(const string &name_, MuDSLInterpreter *interpereter)
            : name(name_), parent(nullptr), host(interpereter) {}
        Scope(const string &name_, ScopeRef scope)
            : name(name_), parent(scope), host(scope->host) {}
        Scope(const Scope &o)
            : name(o.name), parent(o.parent), scopes(o.scopes), functions(o.functions),
              host(o.host) {
            // copy vars by value when cloning a scope
            for (auto &&v: o.variables) {
                variables[v.first] = make_shared<Value>(v.second->value);
            }
        }
        Scope(const string &name_, const unordered_map<string, ValueRef> &variables_)
            : name(name_), variables(variables_) {}
    };

    // state enum for state machine for token by token parsing
    enum class ParseState : uint8_t {
        beginExpression,
        readLine,
        defineVar,
        defineFunc,
        defineClass,
        classArgs,
        funcArgs,
        returnLine,
        ifCall,
        expectIfEnd,
        loopCall,
        forEach,
        importModule
    };

    // finally we have our interpereter
    class MuDSLInterpreter {
        friend Expression;
        vector<Module> modules;
        vector<Module> optionalModules;
        ScopeRef globalScope = make_shared<Scope>(this);
        ScopeRef parseScope = globalScope;
        ExpressionRef currentExpression;
        ValueRef listIndexFunctionVarLocation;
        ValueRef identityFunctionVarLocation;

        ParseState parseState = ParseState::beginExpression;
        vector<string_view> parseStrings;
        int outerNestLayer = 0;
        bool lastStatementClosedScope = false;
        bool lastStatementWasElse = false;
        bool lastTokenEndCurlBraket = false;
        uint64_t currentLine = 0;
        ParseState prevState = ParseState::beginExpression;
        ModulePrivilegeFlags allowedModulePrivileges;

        ValueRef needsToReturn(ExpressionRef expr, ScopeRef scope, Class *classs);
        ValueRef needsToReturn(const vector<ExpressionRef> &subexpressions, ScopeRef scope,
                               Class *classs);
        ExpressionRef consolidated(ExpressionRef exp, ScopeRef scope, Class *classs);

        ExpressionRef getResolveVarExpression(const string &name, bool classScope);
        ExpressionRef getExpression(const vector<string_view> &strings, ScopeRef scope,
                                    Class *classs);
        ValueRef getValue(const vector<string_view> &strings, ScopeRef scope, Class *classs);
        ValueRef getValue(ExpressionRef expr, ScopeRef scope, Class *classs);

        void clearParseStacks();
        void closeDanglingIfExpression();
        void parse(string_view token);

        ScopeRef newClassScope(const string &name, ScopeRef scope);
        void closeScope(ScopeRef &scope);
        bool closeCurrentExpression();
        FunctionRef newFunction(const string &name, ScopeRef scope, FunctionRef func);
        FunctionRef newFunction(const string &name, ScopeRef scope, const vector<string> &argNames);
        FunctionRef newConstructor(const string &name, ScopeRef scope, FunctionRef func);
        FunctionRef newConstructor(const string &name, ScopeRef scope,
                                   const vector<string> &argNames);
        Module *getOptionalModule(const string &name);
        void createStandardLibrary();
        void createOptionalModules();

    public:
        ScopeRef insertScope(ScopeRef existing, ScopeRef parent);
        ScopeRef newScope(const string &name, ScopeRef scope);
        ScopeRef newScope(const string &name) { return newScope(name, globalScope); }
        ScopeRef insertScope(ScopeRef existing) { return insertScope(existing, globalScope); }
        FunctionRef newClass(const string &name, ScopeRef scope,
                             const unordered_map<string, ValueRef> &variables,
                             const ClassLambda &constructor,
                             const unordered_map<string, ClassLambda> &functions);
        FunctionRef newClass(const string &name, const unordered_map<string, ValueRef> &variables,
                             const ClassLambda &constructor,
                             const unordered_map<string, ClassLambda> &functions) {
            return newClass(name, globalScope, variables, constructor, functions);
        }
        FunctionRef newFunction(const string &name, ScopeRef scope, const Lambda &lam) {
            return newFunction(name, scope, make_shared<Function>(name, lam));
        }
        FunctionRef newFunction(const string &name, const Lambda &lam) {
            return newFunction(name, globalScope, lam);
        }
        FunctionRef newFunction(const string &name, ScopeRef scope, const ScopedLambda &lam) {
            return newFunction(name, scope, make_shared<Function>(name, lam));
        }
        FunctionRef newFunction(const string &name, const ScopedLambda &lam) {
            return newFunction(name, globalScope, lam);
        }
        FunctionRef newFunction(const string &name, ScopeRef scope, const ClassLambda &lam) {
            return newFunction(name, scope, make_shared<Function>(name, lam));
        }
        ScopeRef newModule(const string &name, ModulePrivilegeFlags flags,
                           const unordered_map<string, Lambda> &functions);
        ValueRef callFunction(const string &name, ScopeRef scope, const List &args);
        ValueRef callFunction(FunctionRef fnc, ScopeRef scope, const List &args,
                              Class *classs = nullptr);
        ValueRef callFunction(FunctionRef fnc, ScopeRef scope, const List &args, ClassRef classs) {
            return callFunction(fnc, scope, args, classs.get());
        }
        ValueRef callFunction(const string &name, const List &args = List()) {
            return callFunction(name, globalScope, args);
        }
        ValueRef callFunction(FunctionRef fnc, const List &args) {
            return callFunction(fnc, globalScope, args);
        }
        template<typename... Ts>
        ValueRef callFunctionWithArgs(FunctionRef fnc, ScopeRef scope, Ts... args) {
            List argsList = {make_shared<Value>(args)...};
            return callFunction(fnc, scope, argsList);
        }
        template<typename... Ts>
        ValueRef callFunctionWithArgs(FunctionRef fnc, Ts... args) {
            List argsList = {make_shared<Value>(args)...};
            return callFunction(fnc, globalScope, argsList);
        }

        ValueRef &resolveVariable(const string &name, Class *classs, ScopeRef scope);
        ValueRef &resolveVariable(const string &name, ScopeRef scope);
        FunctionRef resolveFunction(const string &name, Class *classs, ScopeRef scope);
        FunctionRef resolveFunction(const string &name, ScopeRef scope);
        ScopeRef resolveScope(const string &name, ScopeRef scope);

        ValueRef resolveVariable(const string &name) { return resolveVariable(name, globalScope); }
        FunctionRef resolveFunction(const string &name) {
            return resolveFunction(name, globalScope);
        }
        ScopeRef resolveScope(const string &name) { return resolveScope(name, globalScope); }

        bool readLine(string_view text);
        bool evaluate(string_view script);
        bool evaluateFile(const string &path);
        bool readLine(string_view text, ScopeRef scope);
        bool evaluate(string_view script, ScopeRef scope);
        bool evaluateFile(const string &path, ScopeRef scope);
        void clearState();
        MuDSLInterpreter(ModulePrivilegeFlags priv) : allowedModulePrivileges(priv) {
            createStandardLibrary();
            if (priv) { createOptionalModules(); }
        }
        MuDSLInterpreter(ModulePrivilege priv)
            : MuDSLInterpreter(static_cast<ModulePrivilegeFlags>(priv)) {}
        MuDSLInterpreter() : MuDSLInterpreter(ModulePrivilegeFlags()) {}
    };
}// namespace MuDSL

#if defined(__EMSCRIPTEN__) || defined(MuDSL_INTERNAL_PRINT)
#define MuDSL_DO_INTERNAL_PRINT
#endif

#pragma endregion

#pragma region CVar

// Hack form https://github.com/rmxbalanque/csys
namespace CVar {

    struct String
    {

        String() = default;

        String(const char *str [[maybe_unused]]) : m_String(str ? str : "") {}

        String(std::string str) : m_String(std::move(str)) {}

        operator const char *() { return m_String.c_str(); }

        operator std::string() { return m_String; }

        std::pair<size_t, size_t> NextPoi(size_t &start) const {
            size_t end = m_String.size();
            std::pair<size_t, size_t> range(end + 1, end);
            size_t pos = start;

            for (; pos < end; ++pos)
                if (!std::isspace(m_String[pos])) {
                    range.first = pos;
                    break;
                }

            for (; pos < end; ++pos)
                if (std::isspace(m_String[pos])) {
                    range.second = pos;
                    break;
                }

            start = range.second;
            return range;
        }

        [[nodiscard]] size_t End() const { return m_String.size() + 1; }

        std::string m_String;
    };

    class Exception : public std::exception {
    public:
        explicit Exception(const std::string &message, const std::string &arg)
            : m_Msg(message + ": '" + arg + "'") {}

        explicit Exception(std::string message) : m_Msg(std::move(message)) {}

        ~Exception() override = default;

        [[nodiscard]] const char *what() const noexcept override { return m_Msg.c_str(); }

    protected:
        std::string m_Msg;
    };

    namespace {
        inline const std::string_view s_Reserved("\\[]\"");
        inline constexpr char s_ErrMsgReserved[] = "Reserved chars '\\, [, ], \"' must be escaped "
                                                   "with \\";
    }// namespace

    struct Reserved
    {

        static inline bool IsEscapeChar(char c) { return c == '\\'; }

        static inline bool IsReservedChar(char c) {
            for (auto rc: s_Reserved)
                if (c == rc) return true;
            return false;
        }

        static inline bool IsEscaping(std::string &input, size_t pos) {
            return pos < input.size() - 1 && IsEscapeChar(input[pos]) &&
                   IsReservedChar(input[pos + 1]);
        }

        static inline bool IsEscaped(std::string &input, size_t pos) {
            bool result = false;

            for (size_t i = pos; i > 0; --i)
                if (IsReservedChar(input[i]) && IsEscapeChar(input[i - 1])) result = !result;
                else
                    break;
            return result;
        }

        Reserved() = delete;
        ~Reserved() = delete;
        Reserved(Reserved &&) = delete;
        Reserved(const Reserved &) = delete;
        Reserved &operator=(Reserved &&) = delete;
        Reserved &operator=(const Reserved &) = delete;
    };

    template<typename T>
    struct ArgumentParser
    {

        inline ArgumentParser(String &input, size_t &start);

        T m_Value;
    };

#define ARG_PARSE_BASE_SPEC(TYPE)                                                                  \
    template<>                                                                                     \
    struct ArgumentParser<TYPE>                                                                    \
    {                                                                                              \
        inline ArgumentParser(String &input, size_t &start);                                       \
        TYPE m_Value = 0;                                                                          \
    };                                                                                             \
    inline ArgumentParser<TYPE>::ArgumentParser(String &input, size_t &start)

#define ARG_PARSE_SUBSTR(range) input.m_String.substr(range.first, range.second - range.first)

#define ARG_PARSE_GENERAL_SPEC(TYPE, TYPE_NAME, FUNCTION)                                          \
    ARG_PARSE_BASE_SPEC(TYPE) {                                                                    \
        auto range = input.NextPoi(start);                                                         \
        try {                                                                                      \
            m_Value = (TYPE) FUNCTION(ARG_PARSE_SUBSTR(range), &range.first);                      \
        } catch (const std::out_of_range &) {                                                      \
            throw Exception(std::string("Argument too large for ") + TYPE_NAME,                    \
                            input.m_String.substr(range.first, range.second));                     \
        } catch (const std::invalid_argument &) {                                                  \
            throw Exception(std::string("Missing or invalid ") + TYPE_NAME + " argument",          \
                            input.m_String.substr(range.first, range.second));                     \
        }                                                                                          \
    }

    ARG_PARSE_BASE_SPEC(CVar::String) {
        m_Value.m_String.clear();

        static auto GetWord = [](std::string &str, size_t start, size_t end) {
            static std::string invalid_chars;
            invalid_chars.clear();

            std::string result;

            for (size_t i = start; i < end; ++i)

                if (!Reserved::IsReservedChar(str[i])) result.push_back(str[i]);

                else {

                    if (Reserved::IsEscapeChar(str[i]) && Reserved::IsEscaping(str, i))
                        result.push_back(str[++i]);

                    else
                        throw Exception(s_ErrMsgReserved, str.substr(start, end - start));
                }

            return result;
        };

        auto range = input.NextPoi(start);

        if (input.m_String[range.first] != '"')
            m_Value = GetWord(input.m_String, range.first, range.second);

        else {
            ++range.first;
            while (true) {

                range.second = input.m_String.find('"', range.first);
                while (range.second != std::string::npos &&
                       Reserved::IsEscaped(input.m_String, range.second))
                    range.second = input.m_String.find('"', range.second + 1);

                if (range.second == std::string::npos) {
                    range.second = input.m_String.size();
                    throw Exception("Could not find closing '\"'", ARG_PARSE_SUBSTR(range));
                }

                m_Value.m_String += GetWord(input.m_String, range.first, range.second);

                range.first = range.second + 1;

                if (range.first < input.m_String.size() &&
                    !std::isspace(input.m_String[range.first])) {

                    if (input.m_String[range.first] == '"') ++range.first;
                } else

                    break;
            }
        }

        start = range.second + 1;
    }

    ARG_PARSE_BASE_SPEC(bool) {

        static const char *s_err_msg = "Missing or invalid boolean argument";
        static const char *s_false = "false";
        static const char *s_true = "true";

        auto range = input.NextPoi(start);

        input.m_String[range.first] = char(std::tolower(input.m_String[range.first]));

        if (range.second - range.first == 4 && input.m_String[range.first] == 't') {

            for (size_t i = range.first + 1; i < range.second; ++i)
                if ((input.m_String[i] = char(std::tolower(input.m_String[i]))) !=
                    s_true[i - range.first])
                    throw Exception(s_err_msg + std::string(", expected true"),
                                    ARG_PARSE_SUBSTR(range));
            m_Value = true;
        }

        else if (range.second - range.first == 5 && input.m_String[range.first] == 'f') {

            for (size_t i = range.first + 1; i < range.second; ++i)
                if ((input.m_String[i] = char(std::tolower(input.m_String[i]))) !=
                    s_false[i - range.first])
                    throw Exception(s_err_msg + std::string(", expected false"),
                                    ARG_PARSE_SUBSTR(range));
            m_Value = false;
        }

        else
            throw Exception(s_err_msg, ARG_PARSE_SUBSTR(range));
    }

    ARG_PARSE_BASE_SPEC(char) {

        auto range = input.NextPoi(start);
        size_t len = range.second - range.first;

        if (len > 2 || len <= 0)
            throw Exception("Too many or no chars were given", ARG_PARSE_SUBSTR(range));

        else if (len == 2) {

            if (!Reserved::IsEscaping(input.m_String, range.first))
                throw Exception("Too many chars were given", ARG_PARSE_SUBSTR(range));

            m_Value = input.m_String[range.first + 1];
        }

        else if (Reserved::IsReservedChar(input.m_String[range.first]))
            throw Exception(s_ErrMsgReserved, ARG_PARSE_SUBSTR(range));

        else
            m_Value = input.m_String[range.first];
    }

    ARG_PARSE_BASE_SPEC(unsigned char) {

        auto range = input.NextPoi(start);
        size_t len = range.second - range.first;

        if (len > 2 || len <= 0)
            throw Exception("Too many or no chars were given", ARG_PARSE_SUBSTR(range));

        else if (len == 2) {

            if (!Reserved::IsEscaping(input.m_String, range.first))
                throw Exception("Too many chars were given", ARG_PARSE_SUBSTR(range));

            m_Value = static_cast<unsigned char>(input.m_String[range.first + 1]);
        }

        else if (Reserved::IsReservedChar(input.m_String[range.first]))
            throw Exception(s_ErrMsgReserved, ARG_PARSE_SUBSTR(range));

        else
            m_Value = static_cast<unsigned char>(input.m_String[range.first]);
    }

    ARG_PARSE_GENERAL_SPEC(short, "signed short", std::stoi)
    ARG_PARSE_GENERAL_SPEC(unsigned short, "unsigned short", std::stoul)
    ARG_PARSE_GENERAL_SPEC(int, "signed int", std::stoi)
    ARG_PARSE_GENERAL_SPEC(unsigned int, "unsigned int", std::stoul)
    ARG_PARSE_GENERAL_SPEC(long, "long", std::stol)
    ARG_PARSE_GENERAL_SPEC(unsigned long, "unsigned long", std::stoul)
    ARG_PARSE_GENERAL_SPEC(long long, "long long", std::stoll)
    ARG_PARSE_GENERAL_SPEC(unsigned long long, "unsigned long long", std::stoull)
    ARG_PARSE_GENERAL_SPEC(float, "float", std::stof)
    ARG_PARSE_GENERAL_SPEC(double, "double", std::stod)
    ARG_PARSE_GENERAL_SPEC(long double, "long double", std::stold)

    template<typename T>
    struct ArgumentParser<std::vector<T>>
    {

        ArgumentParser(String &input, size_t &start);

        std::vector<T> m_Value;
    };

    template<typename T>
    ArgumentParser<std::vector<T>>::ArgumentParser(String &input, size_t &start) {

        m_Value.clear();

        auto range = input.NextPoi(start);

        if (range.first == input.End()) return;

        if (input.m_String[range.first] != '[')
            throw Exception("Invalid vector argument missing opening [", ARG_PARSE_SUBSTR(range));

        input.m_String[range.first] = ' ';
        while (true) {

            range = input.NextPoi(range.first);

            if (range.first == input.End()) return;

            else if (input.m_String[range.first] == '[')
                m_Value.push_back(ArgumentParser<T>(input, range.first).m_Value);
            else {

                range.second = input.m_String.find(']', range.first);
                while (range.second != std::string::npos &&
                       Reserved::IsEscaped(input.m_String, range.second))
                    range.second = input.m_String.find(']', range.second + 1);

                if (range.second == std::string::npos) {
                    range.second = input.m_String.size();
                    throw Exception("Invalid vector argument missing closing ]",
                                    ARG_PARSE_SUBSTR(range));
                }

                input.m_String[range.second] = ' ';
                start = range.first;

                while (true) {

                    if ((range.first = input.NextPoi(range.first).first) >= range.second) {
                        start = range.first;
                        return;
                    }

                    m_Value.push_back(ArgumentParser<T>(input, start).m_Value);
                    range.first = start;
                }
            }
        }
    }

#define SUPPORT_TYPE(TYPE, TYPE_NAME)                                                              \
    template<>                                                                                     \
    struct is_supported_type<TYPE>                                                                 \
    {                                                                                              \
        static constexpr bool value = true;                                                        \
    };                                                                                             \
    template<>                                                                                     \
    struct ArgData<TYPE>                                                                           \
    {                                                                                              \
        explicit ArgData(String name) : m_Name(std::move(name)), m_Value() {}                      \
        const String m_Name;                                                                       \
        String m_TypeName = TYPE_NAME;                                                             \
        TYPE m_Value;                                                                              \
    };

    using NULL_ARGUMENT = void (*)();

    template<typename>
    struct is_supported_type
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct ArgData
    {

        explicit ArgData(String name) : m_Name(std::move(name)), m_Value() {}

        const String m_Name = "";
        String m_TypeName = "Unsupported Type";
        T m_Value;
    };

    SUPPORT_TYPE(String, "String")
    SUPPORT_TYPE(bool, "Boolean")
    SUPPORT_TYPE(char, "Char")
    SUPPORT_TYPE(unsigned char, "Unsigned_Char")
    SUPPORT_TYPE(short, "Signed_Short")
    SUPPORT_TYPE(unsigned short, "Unsigned_Short")
    SUPPORT_TYPE(int, "Signed_Int")
    SUPPORT_TYPE(unsigned int, "Unsigned_Int")
    SUPPORT_TYPE(long, "Signed_Long")
    SUPPORT_TYPE(unsigned long, "Unsigned_Long")
    SUPPORT_TYPE(long long, "Signed_Long_Long")
    SUPPORT_TYPE(unsigned long long, "Unsigned_Long_Long")
    SUPPORT_TYPE(float, "Float")
    SUPPORT_TYPE(double, "Double")
    SUPPORT_TYPE(long double, "Long_Double")

    template<typename U>
    struct is_supported_type<std::vector<U>>
    {
        static constexpr bool value = is_supported_type<U>::value;
    };
    template<typename T>
    struct ArgData<std::vector<T>>
    {

        explicit ArgData(String name) : m_Name(std::move(name)) {}

        const String m_Name;
        String m_TypeName = std::string("Vector_Of_") + ArgData<T>("").m_TypeName.m_String;
        std::vector<T> m_Value;
    };

    template<typename T>
    struct Arg
    {

        template<typename U>
        static constexpr bool is_supported_type_v = is_supported_type<U>::value;

    public:
        using ValueType = std::remove_cv_t<std::remove_reference_t<T>>;

        explicit Arg(const String &name) : m_Arg(name) {
            static_assert(is_supported_type_v<ValueType>,
                          "ValueType 'T' is not supported, see 'Supported types' for more help");
        }

        Arg<T> &Parse(String &input, size_t &start) {
            size_t index = start;

            if (input.NextPoi(index).first == input.End())
                throw Exception("Not enough arguments were given", input.m_String);

            m_Arg.m_Value = ArgumentParser<ValueType>(input, start).m_Value;
            return *this;
        }

        std::string Info() {
            return std::string(" [") + m_Arg.m_Name.m_String + ":" + m_Arg.m_TypeName.m_String +
                   "]";
        }

        ArgData<ValueType> m_Arg;
    };

    template<>
    struct Arg<NULL_ARGUMENT>
    {

        Arg<NULL_ARGUMENT> &Parse(String &input, size_t &start) {
            if (input.NextPoi(start).first != input.End())
                throw Exception("Too many arguments were given", input.m_String);
            return *this;
        }
    };
    static const char endl = '\n';

    enum ItemType {
        COMMAND = 0,
        LOG,
        WARNING,
        ERROR,
        INFO,
        NONE
    };

    struct Item
    {

        explicit Item(ItemType type = ItemType::LOG);

        Item(Item &&rhs) = default;

        Item(const Item &rhs) = default;

        Item &operator=(Item &&rhs) = default;

        Item &operator=(const Item &rhs) = default;

        Item &operator<<(std::string_view str);

        [[nodiscard]] std::string Get() const;

        ItemType m_Type;
        std::string m_Data;
        unsigned int m_TimeStamp;
    };

#define LOG_BASIC_TYPE_DECL(type) ItemLog &operator<<(type data)

    class ItemLog {
    public:
        ItemLog &log(ItemType type);

        ItemLog() = default;

        ItemLog(ItemLog &&rhs) = default;

        ItemLog(const ItemLog &rhs) = default;

        ItemLog &operator=(ItemLog &&rhs) = default;

        ItemLog &operator=(const ItemLog &rhs) = default;

        std::vector<Item> &Items();

        void Clear();

        LOG_BASIC_TYPE_DECL(int);
        LOG_BASIC_TYPE_DECL(long);
        LOG_BASIC_TYPE_DECL(float);
        LOG_BASIC_TYPE_DECL(double);
        LOG_BASIC_TYPE_DECL(long long);
        LOG_BASIC_TYPE_DECL(long double);
        LOG_BASIC_TYPE_DECL(unsigned int);
        LOG_BASIC_TYPE_DECL(unsigned long);
        LOG_BASIC_TYPE_DECL(unsigned long long);
        LOG_BASIC_TYPE_DECL(std::string_view);
        LOG_BASIC_TYPE_DECL(char);

    protected:
        std::vector<Item> m_Items;
    };

    METADOT_INLINE static const std::string_view s_Command = "> ";
    METADOT_INLINE static const std::string_view s_Warning = "\t[WARNING]: ";
    METADOT_INLINE static const std::string_view s_Error = "[ERROR]: ";
    METADOT_INLINE static const auto s_TimeBegin = std::chrono::steady_clock::now();

    METADOT_INLINE Item::Item(ItemType type) : m_Type(type) {
        auto timeNow = std::chrono::steady_clock::now();
        m_TimeStamp = static_cast<unsigned int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - s_TimeBegin)
                        .count());
    }

    METADOT_INLINE Item &Item::operator<<(const std::string_view str) {
        m_Data.append(str);
        return *this;
    }

    METADOT_INLINE std::string Item::Get() const {
        switch (m_Type) {
            case COMMAND:
                return s_Command.data() + m_Data;
            case LOG:
                return '\t' + m_Data;
            case WARNING:
                return s_Warning.data() + m_Data;
            case ERROR:
                return s_Error.data() + m_Data;
            case INFO:
                return m_Data;
            case NONE:
            default:
                return "";
        }
    }

#define LOG_BASIC_TYPE_DEF(type)                                                                   \
    METADOT_INLINE ItemLog &ItemLog::operator<<(type data) {                                       \
        m_Items.back() << std::to_string(data);                                                    \
        return *this;                                                                              \
    }

    METADOT_INLINE ItemLog &ItemLog::log(ItemType type) {

        m_Items.emplace_back(type);
        return *this;
    }

    METADOT_INLINE std::vector<Item> &ItemLog::Items() { return m_Items; }

    METADOT_INLINE void ItemLog::Clear() { m_Items.clear(); }

    METADOT_INLINE ItemLog &ItemLog::operator<<(const std::string_view data) {
        m_Items.back() << data;
        return *this;
    }

    METADOT_INLINE ItemLog &ItemLog::operator<<(const char data) {
        m_Items.back().m_Data.append(1, data);
        return *this;
    }

    LOG_BASIC_TYPE_DEF(int)
    LOG_BASIC_TYPE_DEF(long)
    LOG_BASIC_TYPE_DEF(float)
    LOG_BASIC_TYPE_DEF(double)
    LOG_BASIC_TYPE_DEF(long long)
    LOG_BASIC_TYPE_DEF(long double)
    LOG_BASIC_TYPE_DEF(unsigned int)
    LOG_BASIC_TYPE_DEF(unsigned long)
    LOG_BASIC_TYPE_DEF(unsigned long long)

    struct CommandBase
    {

        virtual ~CommandBase() = default;

        virtual Item operator()(String &input) = 0;

        [[nodiscard]] virtual std::string Help() = 0;

        [[nodiscard]] virtual size_t ArgumentCount() const = 0;

        [[nodiscard]] virtual CommandBase *Clone() const = 0;
    };

    template<typename Fn, typename... Args>
    class Command : public CommandBase {
    public:
        Command(String name, String description, Fn function, Args... args)
            : m_Name(std::move(name)), m_Description(std::move(description)), m_Function(function),
              m_Arguments(args..., Arg<NULL_ARGUMENT>()) {}

        Item operator()(String &input) final {
            try {

                constexpr int argumentSize = sizeof...(Args);
                Call(input, std::make_index_sequence<argumentSize + 1>{},
                     std::make_index_sequence<argumentSize>{});
            } catch (Exception &ae) { return Item(ERROR) << (m_Name.m_String + ": " + ae.what()); }
            return Item(NONE);
        }

        [[nodiscard]] std::string Help() final {
            return m_Name.m_String + DisplayArguments(std::make_index_sequence<sizeof...(Args)>{}) +
                   "\n\t\t- " + m_Description.m_String + "\n\n";
        }

        [[nodiscard]] size_t ArgumentCount() const final { return sizeof...(Args); }

        [[nodiscard]] CommandBase *Clone() const final { return new Command<Fn, Args...>(*this); }

    private:
        template<size_t... Is_p, size_t... Is_c>
        void Call(String &input, const std::index_sequence<Is_p...> &,
                  const std::index_sequence<Is_c...> &) {
            size_t start = 0;

            int _[]{0, (void(std::get<Is_p>(m_Arguments).Parse(input, start)), 0)...};
            (void) (_);

            m_Function((std::get<Is_c>(m_Arguments).m_Arg.m_Value)...);
        }

        template<size_t... Is>
        std::string DisplayArguments(const std::index_sequence<Is...> &) {
            return (std::get<Is>(m_Arguments).Info() + ...);
        }

        const String m_Name;
        const String m_Description;
        std::function<void(typename Args::ValueType...)> m_Function;
        std::tuple<Args..., Arg<NULL_ARGUMENT>> m_Arguments;
    };

    template<typename Fn>
    class Command<Fn> : public CommandBase {
    public:
        Command(String name, String description, Fn function)
            : m_Name(std::move(name)), m_Description(std::move(description)), m_Function(function),
              m_Arguments(Arg<NULL_ARGUMENT>()) {}

        Item operator()(String &input) final {

            size_t start = 0;
            try {

                std::get<0>(m_Arguments).Parse(input, start);
            } catch (Exception &ae) { return Item(ERROR) << (m_Name.m_String + ": " + ae.what()); }

            m_Function();
            return Item(NONE);
        }

        [[nodiscard]] std::string Help() final {
            return m_Name.m_String + "\n\t\t- " + m_Description.m_String + "\n\n";
        }

        [[nodiscard]] size_t ArgumentCount() const final { return 0; }

        [[nodiscard]] CommandBase *Clone() const final { return new Command<Fn>(*this); }

    private:
        const String m_Name;
        const String m_Description;
        std::function<void(void)> m_Function;
        std::tuple<Arg<NULL_ARGUMENT>> m_Arguments;
    };

    class AutoComplete {
    public:
        using r_sVector = std::vector<std::string> &;
        using sVector = std::vector<std::string>;

        struct ACNode
        {
            explicit ACNode(const char data, bool isWord = false)
                : m_Data(data), m_IsWord(isWord), m_Less(nullptr), m_Equal(nullptr),
                  m_Greater(nullptr){};

            explicit ACNode(const char &&data, bool isWord = false)
                : m_Data(data), m_IsWord(isWord), m_Less(nullptr), m_Equal(nullptr),
                  m_Greater(nullptr){};

            ~ACNode() {
                delete m_Less;
                delete m_Equal;
                delete m_Greater;
            };

            char m_Data;
            bool m_IsWord;
            ACNode *m_Less;
            ACNode *m_Equal;
            ACNode *m_Greater;
        };

        AutoComplete() = default;

        AutoComplete(const AutoComplete &tree);

        AutoComplete(AutoComplete &&rhs) = default;

        AutoComplete &operator=(const AutoComplete &rhs);

        AutoComplete &operator=(AutoComplete &&rhs) = default;

        template<typename inputType>
        AutoComplete(std::initializer_list<inputType> il) {
            for (const auto &item: il) { Insert(item); }
        }

        template<typename T>
        explicit AutoComplete(const T &items) {
            for (const auto &item: items) { Insert(item); }
        }

        ~AutoComplete();

        [[nodiscard]] size_t Size() const;

        [[nodiscard]] size_t Count() const;

        bool Search(const char *word);

        void Insert(const char *word);

        void Insert(const std::string &word);

        template<typename strType>
        void Insert(const strType &word) {
            ACNode **ptr = &m_Root;
            ++m_Count;

            while (*word != '\0') {

                if (*ptr == nullptr) {
                    *ptr = new ACNode(*word);
                    ++m_Size;
                }

                if (*word < (*ptr)->m_Data) {
                    ptr = &(*ptr)->m_Less;
                } else if (*word == (*ptr)->m_Data) {

                    if (*(word + 1) == '\0') {
                        if ((*ptr)->m_IsWord) --m_Count;

                        (*ptr)->m_IsWord = true;
                    }

                    ptr = &(*ptr)->m_Equal;
                    ++word;
                } else {
                    ptr = &(*ptr)->m_Greater;
                }
            }
        }

        void Remove(const std::string &word);

        template<typename strType>
        void Suggestions(const strType &prefix, r_sVector ac_options) {
            ACNode *ptr = m_Root;
            auto temp = prefix;

            while (ptr) {
                if (*prefix < ptr->m_Data) {
                    ptr = ptr->m_Less;
                } else if (*prefix == ptr->m_Data) {

                    if (*(prefix + 1) == '\0') break;

                    ptr = ptr->m_Equal;
                    ++prefix;
                } else {
                    ptr = ptr->m_Greater;
                }
            }

            if (ptr && ptr->m_IsWord) return;

            if (!ptr) return;

            SuggestionsAux(ptr->m_Equal, ac_options, temp);
        }

        void Suggestions(const char *prefix, r_sVector ac_options);

        std::string Suggestions(const std::string &prefix, r_sVector ac_options);

        void Suggestions(std::string &prefix, r_sVector ac_options, bool partial_complete);

        template<typename strType>
        std::unique_ptr<sVector> Suggestions(const strType &prefix) {
            auto temp = std::make_unique<sVector>();
            Suggestions(prefix, *temp);
            return temp;
        }

        std::unique_ptr<sVector> Suggestions(const char *prefix);

    protected:
        void SuggestionsAux(ACNode *root, r_sVector ac_options, std::string buffer);

        bool RemoveAux(ACNode *root, const char *word);

        void DeepClone(ACNode *src, ACNode *&dest);

        ACNode *m_Root = nullptr;
        size_t m_Size = 0;
        size_t m_Count = 0;
    };

    METADOT_INLINE AutoComplete::~AutoComplete() { delete m_Root; }

    METADOT_INLINE AutoComplete::AutoComplete(const AutoComplete &tree)
        : m_Size(tree.m_Size), m_Count(tree.m_Count) {
        DeepClone(tree.m_Root, m_Root);
    }

    METADOT_INLINE AutoComplete &AutoComplete::operator=(const AutoComplete &rhs) {

        if (&rhs == this) return *this;

        delete m_Root;

        DeepClone(rhs.m_Root, m_Root);
        m_Size = rhs.m_Size;
        m_Count = rhs.m_Count;

        return *this;
    }

    METADOT_INLINE size_t AutoComplete::Size() const { return m_Size; }

    METADOT_INLINE size_t AutoComplete::Count() const { return m_Count; }

    METADOT_INLINE bool AutoComplete::Search(const char *word) {
        ACNode *ptr = m_Root;

        while (ptr) {
            if (*word < ptr->m_Data) {
                ptr = ptr->m_Less;
            } else if (*word == ptr->m_Data) {

                if (*(word + 1) == '\0' && ptr->m_IsWord) return true;

                ptr = ptr->m_Equal;
                ++word;
            } else {
                ptr = ptr->m_Greater;
            }
        }

        return false;
    }

    METADOT_INLINE void AutoComplete::Insert(const char *word) {
        ACNode **ptr = &m_Root;
        ++m_Count;

        while (*word != '\0') {

            if (*ptr == nullptr) {
                *ptr = new ACNode(*word);
                ++m_Size;
            }

            if (*word < (*ptr)->m_Data) {
                ptr = &(*ptr)->m_Less;
            } else if (*word == (*ptr)->m_Data) {

                if (*(word + 1) == '\0') {
                    if ((*ptr)->m_IsWord) --m_Count;

                    (*ptr)->m_IsWord = true;
                }

                ptr = &(*ptr)->m_Equal;
                ++word;
            } else {
                ptr = &(*ptr)->m_Greater;
            }
        }
    }

    METADOT_INLINE void AutoComplete::Insert(const std::string &word) { Insert(word.c_str()); }

    METADOT_INLINE void AutoComplete::Remove(const std::string &word) {
        RemoveAux(m_Root, word.c_str());
    }

    METADOT_INLINE void AutoComplete::Suggestions(const char *prefix,
                                                  std::vector<std::string> &ac_options) {
        ACNode *ptr = m_Root;
        auto temp = prefix;

        while (ptr) {
            if (*prefix < ptr->m_Data) {
                ptr = ptr->m_Less;
            } else if (*prefix == ptr->m_Data) {

                if (*(prefix + 1) == '\0') break;

                ptr = ptr->m_Equal;
                ++prefix;
            } else {
                ptr = ptr->m_Greater;
            }
        }

        if (ptr && ptr->m_IsWord) return;

        if (!ptr) return;

        SuggestionsAux(ptr->m_Equal, ac_options, temp);
    }

    METADOT_INLINE std::string AutoComplete::Suggestions(const std::string &prefix,
                                                         r_sVector &ac_options) {
        std::string temp = prefix;
        Suggestions(temp, ac_options, true);
        return temp;
    }

    METADOT_INLINE void AutoComplete::Suggestions(std::string &prefix, r_sVector ac_options,
                                                  bool partial_complete) {
        ACNode *ptr = m_Root;
        const char *temp = prefix.data();
        size_t prefix_end = prefix.size();

        while (ptr) {
            if (*temp < ptr->m_Data) {
                ptr = ptr->m_Less;
            } else if (*temp == ptr->m_Data) {

                if (*(temp + 1) == '\0') {
                    if (partial_complete) {
                        ACNode *pc_ptr = ptr->m_Equal;

                        while (pc_ptr) {
                            if (pc_ptr->m_Equal && !pc_ptr->m_Less && !pc_ptr->m_Greater)
                                prefix.push_back(pc_ptr->m_Data);
                            else
                                break;

                            pc_ptr = pc_ptr->m_Equal;
                        }
                    }

                    break;
                }

                ptr = ptr->m_Equal;
                ++temp;
            } else {
                ptr = ptr->m_Greater;
            }
        }

        if (ptr && ptr->m_IsWord) return;

        if (!ptr) return;

        SuggestionsAux(ptr->m_Equal, ac_options, prefix.substr(0, prefix_end));
    }

    METADOT_INLINE std::unique_ptr<AutoComplete::sVector> AutoComplete::Suggestions(
            const char *prefix) {
        auto temp = std::make_unique<sVector>();
        Suggestions(prefix, *temp);
        return temp;
    }

    METADOT_INLINE void AutoComplete::SuggestionsAux(AutoComplete::ACNode *root,
                                                     r_sVector ac_options, std::string buffer) {
        if (!root) return;

        if (root->m_Less) SuggestionsAux(root->m_Less, ac_options, buffer);

        if (root->m_IsWord) {
            ac_options.push_back(buffer.append(1, root->m_Data));
            buffer.pop_back();
        }

        if (root->m_Equal) {
            SuggestionsAux(root->m_Equal, ac_options, buffer.append(1, root->m_Data));
            buffer.pop_back();
        }

        if (root->m_Greater) { SuggestionsAux(root->m_Greater, ac_options, buffer); }
    }

    METADOT_INLINE bool AutoComplete::RemoveAux(AutoComplete::ACNode *root, const char *word) {
        if (!root) return false;

        if (*(word + 1) == '\0' && root->m_Data == *word) {

            if (root->m_IsWord) {
                root->m_IsWord = false;
                return (!root->m_Equal && !root->m_Less && !root->m_Greater);
            }

            else
                return false;
        } else {

            if (*word < root->m_Data) RemoveAux(root->m_Less, word);
            else if (*word > root->m_Data)
                RemoveAux(root->m_Greater, word);

            else if (*word == root->m_Data) {

                if (RemoveAux(root->m_Equal, word + 1)) {
                    delete root->m_Equal;
                    root->m_Equal = nullptr;
                    return !root->m_IsWord && (!root->m_Equal && !root->m_Less && !root->m_Greater);
                }
            }
        }

        return false;
    }

    METADOT_INLINE void AutoComplete::DeepClone(AutoComplete::ACNode *src,
                                                AutoComplete::ACNode *&dest) {
        if (!src) return;

        dest = new ACNode(*src);
        DeepClone(src->m_Less, dest->m_Less);
        DeepClone(src->m_Equal, dest->m_Equal);
        DeepClone(src->m_Greater, dest->m_Greater);
    }

    class CommandHistory {
    public:
        explicit CommandHistory(unsigned int maxRecord = 100);

        CommandHistory(CommandHistory &&rhs) = default;

        CommandHistory(const CommandHistory &rhs) = default;

        CommandHistory &operator=(CommandHistory &&rhs) = default;

        CommandHistory &operator=(const CommandHistory &rhs) = default;

        void PushBack(const std::string &line);

        [[nodiscard]] unsigned int GetNewIndex() const;

        const std::string &GetNew();

        [[nodiscard]] unsigned int GetOldIndex() const;

        const std::string &GetOld();

        void Clear();

        const std::string &operator[](size_t index);

        friend std::ostream &operator<<(std::ostream &os, const CommandHistory &history);

        size_t Size();

        size_t Capacity();

    protected:
        unsigned int m_Record;
        unsigned int m_MaxRecord;
        std::vector<std::string> m_History;
    };

    METADOT_INLINE CommandHistory::CommandHistory(unsigned int maxRecord)
        : m_Record(0), m_MaxRecord(maxRecord), m_History(maxRecord) {}

    METADOT_INLINE void CommandHistory::PushBack(const std::string &line) {
        m_History[m_Record++ % m_MaxRecord] = line;
    }

    METADOT_INLINE unsigned int CommandHistory::GetNewIndex() const {
        return (m_Record - 1) % m_MaxRecord;
    }

    METADOT_INLINE const std::string &CommandHistory::GetNew() {
        return m_History[(m_Record - 1) % m_MaxRecord];
    }

    METADOT_INLINE unsigned int CommandHistory::GetOldIndex() const {
        if (m_Record <= m_MaxRecord) return 0;
        else
            return m_Record % m_MaxRecord;
    }

    METADOT_INLINE const std::string &CommandHistory::GetOld() {
        if (m_Record <= m_MaxRecord) return m_History.front();
        else
            return m_History[m_Record % m_MaxRecord];
    }

    METADOT_INLINE void CommandHistory::Clear() { m_Record = 0; }

    METADOT_INLINE const std::string &CommandHistory::operator[](size_t index) {
        return m_History[index];
    }

    METADOT_INLINE std::ostream &operator<<(std::ostream &os, const CommandHistory &history) {
        os << "History: " << '\n';
        for (unsigned int i = 0; i < history.m_Record && history.m_Record <= history.m_MaxRecord;
             ++i)
            std::cout << history.m_History[i] << '\n';
        return os;
    }

    METADOT_INLINE size_t CommandHistory::Size() {
        return m_Record < m_MaxRecord ? m_Record : m_MaxRecord;
    }

    METADOT_INLINE size_t CommandHistory::Capacity() { return m_History.capacity(); }

    class System {
    public:
        System();

        System(System &&rhs) = default;

        System(const System &rhs);

        System &operator=(System &&rhs) = default;

        System &operator=(const System &rhs);

        void RunCommand(const std::string &line);

        AutoComplete &CmdAutocomplete();

        AutoComplete &VarAutocomplete();

        CommandHistory &History();

        std::vector<Item> &Items();

        ItemLog &Log(ItemType type = ItemType::LOG);

        std::unordered_map<std::string, std::unique_ptr<CommandBase>> &Commands();

        template<typename Fn, typename... Args>
        void RegisterCommand(const String &name, const String &description, Fn function,
                             Args... args) {

            static_assert(std::is_invocable_v<Fn, typename Args::ValueType...>,
                          "Arguments specified do not match that of the function");
            static_assert(!std::is_member_function_pointer_v<Fn>,
                          "Non-static member functions are not allowed");

            size_t name_index = 0;
            auto range = name.NextPoi(name_index);

            if (m_Commands.find(name.m_String) != m_Commands.end())
                throw CVar::Exception("ERROR: Command already exists");

            else if (range.first == name.End()) {
                Log(ERROR) << "Empty command name given" << CVar::endl;
                return;
            }

            std::string command_name =
                    name.m_String.substr(range.first, range.second - range.first);

            if (name.NextPoi(name_index).first != name.End())
                throw CVar::Exception("ERROR: Whitespace separated command names are forbidden");

            if (m_RegisterCommandSuggestion) {
                m_CommandSuggestionTree.Insert(command_name);
                m_VariableSuggestionTree.Insert(command_name);
            }

            m_Commands[name.m_String] =
                    std::make_unique<Command<Fn, Args...>>(name, description, function, args...);

            auto help = [this, command_name]() {
                Log(LOG) << m_Commands[command_name]->Help() << CVar::endl;
            };

            m_Commands["help " + command_name] = std::make_unique<Command<decltype(help)>>(
                    "help " + command_name, "Displays help info about command " + command_name,
                    help);
        }

        template<typename T, typename... Types>
        void RegisterVariable(const String &name, T &var, Arg<Types>... args) {
            static_assert(std::is_constructible_v<T, Types...>,
                          "Type of var 'T' can not be constructed with types of 'Types'");
            static_assert(sizeof...(Types) != 0, "Empty variadic list");

            auto var_name = RegisterVariableAux(name, var);

            auto setter = [&var](Types... params) { var = T(params...); };
            m_Commands["set " + var_name] =
                    std::make_unique<Command<decltype(setter), Arg<Types>...>>(
                            "set " + var_name, "Sets the variable " + var_name, setter, args...);
        }

        template<typename T, typename... Types>
        void RegisterVariable(const String &name, T &var, void (*setter)(T &, Types...)) {

            auto var_name = RegisterVariableAux(name, var);

            auto setter_l = [&var, setter](Types... args) { setter(var, args...); };
            m_Commands["set " + var_name] =
                    std::make_unique<Command<decltype(setter_l), Arg<Types>...>>(
                            "set " + var_name, "Sets the variable " + var_name, setter_l,
                            Arg<Types>("")...);
        }

        void UnregisterCommand(const std::string &cmd_name);

        void UnregisterVariable(const std::string &var_name);

        void UnregisterScript(const std::string &script_name);

    protected:
        template<typename T>
        std::string RegisterVariableAux(const String &name, T &var) {

            m_RegisterCommandSuggestion = false;

            size_t name_index = 0;
            auto range = name.NextPoi(name_index);
            if (name.NextPoi(name_index).first != name.End())
                throw CVar::Exception("ERROR: Whitespace separated variable names are forbidden");

            std::string var_name = name.m_String.substr(range.first, range.second - range.first);

            const auto GetFunction = [this, &var]() { m_ItemLog.log(LOG) << var << endl; };

            m_Commands["get " + var_name] = std::make_unique<Command<decltype(GetFunction)>>(
                    "get " + var_name, "Gets the variable " + var_name, GetFunction);

            m_RegisterCommandSuggestion = true;

            m_VariableSuggestionTree.Insert(var_name);

            return var_name;
        }

        void ParseCommandLine(const String &line);

        std::unordered_map<std::string, std::unique_ptr<CommandBase>> m_Commands;
        AutoComplete m_CommandSuggestionTree;
        AutoComplete m_VariableSuggestionTree;
        CommandHistory m_CommandHistory;
        ItemLog m_ItemLog;
        bool m_RegisterCommandSuggestion = true;
    };

    static const std::string_view s_Set = "set";
    static const std::string_view s_Get = "get";
    static const std::string_view s_Help = "help";
    static const std::string_view s_ErrorNoVar = "No variable provided";
    static const std::string_view s_ErrorSetGetNotFound = "命令不存在/变量未注册";

    METADOT_INLINE System::System() {

        RegisterCommand(s_Help.data(), "指令信息", [this]() {
            Log() << "help [command_name:String] (Optional)\n\t\t- Display command(s) information\n"
                  << CVar::endl;
            Log() << "set [variable_name:String] [data]\n\t\t- Assign data to given variable\n"
                  << CVar::endl;
            Log() << "get [variable_name:String]\n\t\t- Display data of given variable\n"
                  << CVar::endl;

            for (const auto &tuple: Commands()) {

                if (tuple.first.size() >= 5 && (tuple.first[3] == ' ' || tuple.first[4] == ' '))
                    continue;

                if (tuple.first.size() == 4 && (tuple.first == "help")) continue;

                Log() << tuple.second->Help();
            }
        });

        m_CommandSuggestionTree.Insert(s_Set.data());
        m_CommandSuggestionTree.Insert(s_Get.data());
    }

    METADOT_INLINE System::System(const System &rhs)
        : m_CommandSuggestionTree(rhs.m_CommandSuggestionTree),
          m_VariableSuggestionTree(rhs.m_VariableSuggestionTree),
          m_CommandHistory(rhs.m_CommandHistory), m_ItemLog(rhs.m_ItemLog),
          m_RegisterCommandSuggestion(rhs.m_RegisterCommandSuggestion) {

        for (const auto &pair: rhs.m_Commands) {
            m_Commands[pair.first] = std::unique_ptr<CommandBase>(pair.second->Clone());
        }
    }

    METADOT_INLINE System &System::operator=(const System &rhs) {
        if (this == &rhs) return *this;

        for (const auto &pair: rhs.m_Commands) {
            m_Commands[pair.first] = std::unique_ptr<CommandBase>(pair.second->Clone());
        }

        m_CommandSuggestionTree = rhs.m_CommandSuggestionTree;
        m_VariableSuggestionTree = rhs.m_VariableSuggestionTree;
        m_CommandHistory = rhs.m_CommandHistory;
        m_ItemLog = rhs.m_ItemLog;

        m_RegisterCommandSuggestion = rhs.m_RegisterCommandSuggestion;

        return *this;
    }

    METADOT_INLINE void System::RunCommand(const std::string &line) {

        if (line.empty()) return;

        Log(CVar::ItemType::COMMAND) << line << CVar::endl;

        ParseCommandLine(line);
    }

    METADOT_INLINE void System::UnregisterCommand(const std::string &cmd_name) {

        if (cmd_name.empty()) return;

        auto command_it = m_Commands.find(cmd_name);
        auto help_command_it = m_Commands.find("help " + cmd_name);

        if (command_it != m_Commands.end() && help_command_it != m_Commands.end()) {
            m_CommandSuggestionTree.Remove(cmd_name);
            m_VariableSuggestionTree.Remove(cmd_name);

            m_Commands.erase(command_it);
            m_Commands.erase(help_command_it);
        }
    }

    METADOT_INLINE void System::UnregisterVariable(const std::string &var_name) {

        if (var_name.empty()) return;

        auto s_it = m_Commands.find("set " + var_name);
        auto g_it = m_Commands.find("get " + var_name);

        if (s_it != m_Commands.end() && g_it != m_Commands.end()) {
            m_VariableSuggestionTree.Remove(var_name);
            m_Commands.erase(s_it);
            m_Commands.erase(g_it);
        }
    }

    METADOT_INLINE AutoComplete &System::CmdAutocomplete() { return m_CommandSuggestionTree; }

    METADOT_INLINE AutoComplete &System::VarAutocomplete() { return m_VariableSuggestionTree; }

    METADOT_INLINE CommandHistory &System::History() { return m_CommandHistory; }

    METADOT_INLINE std::vector<Item> &System::Items() { return m_ItemLog.Items(); }

    METADOT_INLINE ItemLog &System::Log(ItemType type) { return m_ItemLog.log(type); }

    METADOT_INLINE std::unordered_map<std::string, std::unique_ptr<CommandBase>>
            &System::Commands() {
        return m_Commands;
    }

    METADOT_INLINE void System::ParseCommandLine(const String &line) {

        size_t line_index = 0;
        std::pair<size_t, size_t> range = line.NextPoi(line_index);

        if (range.first == line.End()) return;

        m_CommandHistory.PushBack(line.m_String);

        std::string command_name = line.m_String.substr(range.first, range.second - range.first);

        bool is_cmd_set = command_name == s_Set;
        bool is_cmd_get = command_name == s_Get;
        bool is_cmd_help = !(is_cmd_set || is_cmd_get) ? command_name == s_Help : false;

        if (is_cmd_help) {
            range = line.NextPoi(line_index);
            if (range.first != line.End())
                command_name += " " + line.m_String.substr(range.first, range.second - range.first);
        }

        else if (is_cmd_set || is_cmd_get) {

            if ((range = line.NextPoi(line_index)).first == line.End()) {
                Log(ERROR) << s_ErrorNoVar << endl;
                return;
            } else

                command_name += " " + line.m_String.substr(range.first, range.second - range.first);
        }

        auto command = m_Commands.find(command_name);
        if (command == m_Commands.end()) Log(ERROR) << s_ErrorSetGetNotFound << endl;

        else {

            String arguments =
                    line.m_String.substr(range.second, line.m_String.size() - range.first);

            auto cmd_out = (*command->second)(arguments);

            if (cmd_out.m_Type != NONE) m_ItemLog.Items().emplace_back(cmd_out);
        }
    }
}// namespace CVar

#pragma endregion CVar

#endif
