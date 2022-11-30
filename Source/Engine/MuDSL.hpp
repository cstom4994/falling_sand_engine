// copyright Garrett Skelton 2020
// copyright KaoruXun 2022
// MIT license

#ifndef _METADOT_MUDSL_HPP_
#define _METADOT_MUDSL_HPP_

#include <algorithm>
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
#include <unordered_map>
#include <variant>
#include <vector>

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
        if (end.size() > v.size()) {
            return false;
        }
        return equal(end.rbegin(), end.rend(), v.rbegin());
    }

    inline bool startswith(const string &v, const string &start) {
        if (start.size() > v.size()) {
            return false;
        }
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
        if (lpos < input.length()) {
            ret.push_back(input.substr(lpos, input.length()));
        }
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

    inline size_t typeHashBits(Type type) {
        return ((size_t) INT32_MAX << ((size_t) type));
    }

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
    inline vec3 operator+(vec3 const &v) {
        return v;
    }
    inline vec3 operator-(vec3 const &v) {
        return vec3(-v.x, -v.y, -v.z);
    }
    inline vec3 operator+(vec3 const &v, float scalar) {
        return vec3(
                v.x + scalar,
                v.y + scalar,
                v.z + scalar);
    }
    inline vec3 operator+(float scalar, vec3 const &v) {
        return vec3(
                v.x + scalar,
                v.y + scalar,
                v.z + scalar);
    }
    inline vec3 operator+(vec3 const &v1, vec3 const &v2) {
        return vec3(
                v1.x + v2.x,
                v1.y + v2.y,
                v1.z + v2.z);
    }
    inline vec3 operator-(vec3 const &v, float scalar) {
        return vec3(
                v.x - scalar,
                v.y - scalar,
                v.z - scalar);
    }
    inline vec3 operator-(float scalar, vec3 const &v) {
        return vec3(
                v.x - scalar,
                v.y - scalar,
                v.z - scalar);
    }
    inline vec3 operator-(vec3 const &v1, vec3 const &v2) {
        return vec3(
                v1.x - v2.x,
                v1.y - v2.y,
                v1.z - v2.z);
    }
    inline vec3 operator*(vec3 const &v, float scalar) {
        return vec3(
                v.x * scalar,
                v.y * scalar,
                v.z * scalar);
    }
    inline vec3 operator*(float scalar, vec3 const &v) {
        return vec3(
                v.x * scalar,
                v.y * scalar,
                v.z * scalar);
    }
    inline vec3 operator*(vec3 const &v1, vec3 const &v2) {
        return vec3(
                v1.x * v2.x,
                v1.y * v2.y,
                v1.z * v2.z);
    }
    inline vec3 operator/(vec3 const &v, float scalar) {
        return vec3(
                v.x / scalar,
                v.y / scalar,
                v.z / scalar);
    }
    inline vec3 operator/(float scalar, vec3 const &v) {
        return vec3(
                v.x / scalar,
                v.y / scalar,
                v.z / scalar);
    }
    inline vec3 operator/(vec3 const &v1, vec3 const &v2) {
        return vec3(
                v1.x / v2.x,
                v1.y / v2.y,
                v1.z / v2.z);
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
    using ArrayVariant =
            variant<
                    vector<Int>,
                    vector<Float>,
                    vector<vec3>,
                    vector<FunctionRef>,
                    vector<void *>,
                    vector<string>>;

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

        Type getType() const {
            return getArrayType(value);
        }

        template<typename T>
        vector<T> &getStdVector();

        bool operator==(const Array &o) const {
            if (size() != o.size()) {
                return false;
            }
            if (getType() != o.getType()) {
                return false;
            }
            switch (getType()) {
                case Type::Int: {
                    auto &aarr = get<vector<Int>>(value);
                    auto &barr = get<vector<Int>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) {
                            return false;
                        }
                    }
                } break;
                case Type::Float: {
                    auto &aarr = get<vector<Float>>(value);
                    auto &barr = get<vector<Float>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) {
                            return false;
                        }
                    }
                } break;
                case Type::Vec3: {
                    auto &aarr = get<vector<vec3>>(value);
                    auto &barr = get<vector<vec3>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) {
                            return false;
                        }
                    }
                } break;
                case Type::String: {
                    auto &aarr = get<vector<string>>(value);
                    auto &barr = get<vector<string>>(o.value);
                    for (size_t i = 0; i < size(); ++i) {
                        if (aarr[i] != barr[i]) {
                            return false;
                        }
                    }
                } break;
                default:
                    break;
            }
            return true;
        }

        bool operator!=(const Array &o) const {
            return !(operator==(o));
        }

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
        Class(const string &name_, const unordered_map<string, ValueRef> &variables_) : name(name_), variables(variables_) {}
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

    using FunctionBodyVariant =
            variant<
                    vector<ExpressionRef>,
                    Lambda,
                    ScopedLambda,
                    ClassLambda>;

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

        FunctionBodyType getBodyType() {
            return static_cast<FunctionBodyType>(body.index());
        }

        static OperatorPrecedence getPrecedence(const string &n) {
            if (n.size() > 2) {
                return OperatorPrecedence::func;
            }
            if (n == "||" || n == "&&") {
                return OperatorPrecedence::boolean;
            }
            if (n == "++" || n == "--") {
                return OperatorPrecedence::incdec;
            }
            if (contains("!<>|&"s, n[0]) || n == "==") {
                return OperatorPrecedence::compare;
            }
            if (contains(n, '=')) {
                return OperatorPrecedence::assign;
            }
            if (contains("/*%"s, n[0])) {
                return OperatorPrecedence::muldiv;
            }
            if (contains("-+"s, n[0])) {
                return OperatorPrecedence::addsub;
            }
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
        Function(const string &name_, const vector<string> &argNames_, const vector<ExpressionRef> &body_)
            : name(name_), body(body_), argNames(argNames_), opPrecedence(OperatorPrecedence::func) {}
        Function(const string &name_, const vector<string> &argNames_) : Function(name_, argNames_, {}) {}
        // default constructor makes a function with no args that returns void
        Function(const string &name)
            : Function(name, [](List) { return make_shared<Value>(); }) {}
        Function() : name("__anon") {}
        Function(const Function &o) = default;
    };

    struct Null
    {
    };
    // Now that we have our collection types defined, we can finally define our value variant
    using ValueVariant =
            variant<
                    Null,
                    Int,
                    Float,
                    vec3,
                    FunctionRef,
                    void *,
                    string,
                    Array,
                    List,
                    DictionaryRef,
                    ClassRef>;

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

        Type getType() const {
            return static_cast<Type>(value.index());
        }

        // get a string that represents this value
        string getPrintString() const {
            auto t = *this;
            t.hardconvert(Type::String);
            return get<string>(t.value);
        }

        // get this value as an int
        Int &getInt() {
            return get<Int>(value);
        }

        // get this value as a float
        Float &getFloat() {
            return get<Float>(value);
        }

        // get this value as a vec3
        vec3 &getVec3() {
            return get<vec3>(value);
        }

        // get this value as a function
        FunctionRef &getFunction() {
            return get<FunctionRef>(value);
        }

        // get this value as a function
        void *&getPointer() {
            return get<void *>(value);
        }

        // get this value as a string
        string &getString() {
            return get<string>(value);
        }

        // get this value as an array
        Array &getArray() {
            return get<Array>(value);
        }

        // get this value as an std::vector<T>
        template<typename T>
        vector<T> &getStdVector();

        // get this value as a list
        List &getList() {
            return get<List>(value);
        }

        DictionaryRef &getDictionary() {
            return get<DictionaryRef>(value);
        }

        ClassRef &getClass() {
            return get<ClassRef>(value);
        }

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
                throw Exception(
                        "Types `"s + getTypeName(a.getType()) + " " + a.getPrintString() + "` and `" + getTypeName(b.getType()) + " " + b.getPrintString() + "` are incompatible for this operation");
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
                throw Exception("Operator + not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator - not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator * not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator / not defined for type `"s + getTypeName(a.getType()) + "`");
                break;
        }
    }

    inline Value operator+=(Value &a, Value b) {
        if ((int) a.getType() < (int) Type::Array || b.getType() == Type::List) {
            upconvert(a, b);
        }
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
                if (arr.getType() == b.getType() || (b.getType() == Type::Array && b.getArray().getType() == arr.getType())) {
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
                throw Exception("Operator += not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator -= not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator *= not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator /= not defined for type `"s + getTypeName(a.getType()) + "`");
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
                throw Exception("Operator %% not defined for type `"s + getTypeName(a.getType()) + "`");
                break;
        }
    }

    // comparison operators
    bool operator!=(Value a, Value b);
    inline bool operator==(Value a, Value b) {
        if (a.getType() != b.getType()) {
            return false;
        }
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
                if (alist.size() != blist.size()) {
                    return false;
                }
                for (size_t i = 0; i < alist.size(); ++i) {
                    if (*alist[i] != *blist[i]) {
                        return false;
                    }
                }
                return true;
            } break;
            default:
                throw Exception("Operator == not defined for type `"s + getTypeName(a.getType()) + "`");
                break;
        }
        return true;
    }

    inline bool operator!=(Value a, Value b) {
        if (a.getType() != b.getType()) {
            return true;
        }
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
                throw Exception("Operator != not defined for type `"s + getTypeName(a.getType()) + "`");
                break;
        }
        return false;
    }

    inline bool operator||(Value &a, Value &b) {
        return a.getBool() || b.getBool();
    }

    inline bool operator&&(Value &a, Value &b) {
        return a.getBool() && b.getBool();
    }

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

        void clear() {
            subexpressions.clear();
        }

        ~FunctionExpression() {
            clear();
        }
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
        MemberFunctionCall(ExpressionRef ob, const string &fncvalue, const vector<ExpressionRef> &sub)
            : object(ob), functionName(fncvalue), subexpressions(sub) {}
        MemberFunctionCall() {}

        void clear() {
            subexpressions.clear();
        }

        ~MemberFunctionCall() {
            clear();
        }
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
            testExpression = o.testExpression ? make_shared<Expression>(*o.testExpression) : nullptr;
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
            initExpression = o.initExpression ? make_shared<Expression>(*o.initExpression) : nullptr;
            testExpression = o.testExpression ? make_shared<Expression>(*o.testExpression) : nullptr;
            iterateExpression = o.iterateExpression ? make_shared<Expression>(*o.iterateExpression) : nullptr;
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
            listExpression = o.listExpression ? make_shared<Expression>(*o.listExpression) : nullptr;
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

        ResolveVar(const ResolveVar &o) {
            name = o.name;
        }
        ResolveVar() {}
        ResolveVar(const string &n) : name(n) {}
    };

    struct DefineVar
    {
        string name;
        ExpressionRef defineExpression;

        DefineVar(const DefineVar &o) {
            name = o.name;
            defineExpression = o.defineExpression ? make_shared<Expression>(*o.defineExpression) : nullptr;
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
            variant<
                    ValueRef,
                    ResolveVar,
                    DefineVar,
                    FunctionExpression,
                    MemberFunctionCall,
                    MemberVariable,
                    Return,
                    Loop,
                    Foreach,
                    IfElse>;

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
            : type(ExpressionType::FunctionCall), expression(FunctionExpression(val)), parent(nullptr) {}

        Expression(ExpressionRef obj, const string &name)
            : type(ExpressionType::MemberVariable), expression(MemberVariable(obj, name)), parent(nullptr) {}
        Expression(ExpressionRef obj, const string &name, const vector<ExpressionRef> subs)
            : type(ExpressionType::MemberFunctionCall), expression(MemberFunctionCall(obj, name, subs)), parent(nullptr) {}
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

        Expression(ExpressionVariant val, ExpressionType ty)
            : type(ty), expression(val) {}

        ExpressionRef back() {
            switch (type) {
                case ExpressionType::FunctionDef:
                    return get<vector<ExpressionRef>>(get<FunctionExpression>(expression).function->getFunction()->body).back();
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
                    return get<vector<ExpressionRef>>(get<FunctionExpression>(expression).function->getFunction()->body).begin();
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
                    return get<vector<ExpressionRef>>(get<FunctionExpression>(expression).function->getFunction()->body).end();
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
                    get<vector<ExpressionRef>>(get<FunctionExpression>(expression).function->getFunction()->body).push_back(ref);
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

    inline ModulePrivilegeFlags operator|(const ModulePrivilege ours, const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) | other;
    }

    inline ModulePrivilegeFlags operator^(const ModulePrivilege ours, const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) ^ other;
    }

    inline ModulePrivilegeFlags operator&(const ModulePrivilege ours, const ModulePrivilegeFlags other) {
        return static_cast<ModulePrivilegeFlags>(ours) & other;
    }

    inline bool shouldAllow(const ModulePrivilegeFlags allowPolicy, const ModulePrivilegeFlags requiredPermissions) {
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

        Scope(MuDSLInterpreter *interpereter) : name("global"), parent(nullptr), host(interpereter) {}
        Scope(const string &name_, MuDSLInterpreter *interpereter) : name(name_), parent(nullptr), host(interpereter) {}
        Scope(const string &name_, ScopeRef scope) : name(name_), parent(scope), host(scope->host) {}
        Scope(const Scope &o) : name(o.name), parent(o.parent), scopes(o.scopes), functions(o.functions), host(o.host) {
            // copy vars by value when cloning a scope
            for (auto &&v: o.variables) {
                variables[v.first] = make_shared<Value>(v.second->value);
            }
        }
        Scope(const string &name_, const unordered_map<string, ValueRef> &variables_) : name(name_), variables(variables_) {}
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
        ValueRef needsToReturn(const vector<ExpressionRef> &subexpressions, ScopeRef scope, Class *classs);
        ExpressionRef consolidated(ExpressionRef exp, ScopeRef scope, Class *classs);

        ExpressionRef getResolveVarExpression(const string &name, bool classScope);
        ExpressionRef getExpression(const vector<string_view> &strings, ScopeRef scope, Class *classs);
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
        FunctionRef newConstructor(const string &name, ScopeRef scope, const vector<string> &argNames);
        Module *getOptionalModule(const string &name);
        void createStandardLibrary();
        void createOptionalModules();

    public:
        ScopeRef insertScope(ScopeRef existing, ScopeRef parent);
        ScopeRef newScope(const string &name, ScopeRef scope);
        ScopeRef newScope(const string &name) { return newScope(name, globalScope); }
        ScopeRef insertScope(ScopeRef existing) { return insertScope(existing, globalScope); }
        FunctionRef newClass(const string &name, ScopeRef scope, const unordered_map<string, ValueRef> &variables, const ClassLambda &constructor, const unordered_map<string, ClassLambda> &functions);
        FunctionRef newClass(const string &name, const unordered_map<string, ValueRef> &variables, const ClassLambda &constructor, const unordered_map<string, ClassLambda> &functions) { return newClass(name, globalScope, variables, constructor, functions); }
        FunctionRef newFunction(const string &name, ScopeRef scope, const Lambda &lam) { return newFunction(name, scope, make_shared<Function>(name, lam)); }
        FunctionRef newFunction(const string &name, const Lambda &lam) { return newFunction(name, globalScope, lam); }
        FunctionRef newFunction(const string &name, ScopeRef scope, const ScopedLambda &lam) { return newFunction(name, scope, make_shared<Function>(name, lam)); }
        FunctionRef newFunction(const string &name, const ScopedLambda &lam) { return newFunction(name, globalScope, lam); }
        FunctionRef newFunction(const string &name, ScopeRef scope, const ClassLambda &lam) { return newFunction(name, scope, make_shared<Function>(name, lam)); }
        ScopeRef newModule(const string &name, ModulePrivilegeFlags flags, const unordered_map<string, Lambda> &functions);
        ValueRef callFunction(const string &name, ScopeRef scope, const List &args);
        ValueRef callFunction(FunctionRef fnc, ScopeRef scope, const List &args, Class *classs = nullptr);
        ValueRef callFunction(FunctionRef fnc, ScopeRef scope, const List &args, ClassRef classs) { return callFunction(fnc, scope, args, classs.get()); }
        ValueRef callFunction(const string &name, const List &args = List()) { return callFunction(name, globalScope, args); }
        ValueRef callFunction(FunctionRef fnc, const List &args) { return callFunction(fnc, globalScope, args); }
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
        FunctionRef resolveFunction(const string &name) { return resolveFunction(name, globalScope); }
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
        MuDSLInterpreter(ModulePrivilege priv) : MuDSLInterpreter(static_cast<ModulePrivilegeFlags>(priv)) {}
        MuDSLInterpreter() : MuDSLInterpreter(ModulePrivilegeFlags()) {}
    };
}// namespace MuDSL

#if defined(__EMSCRIPTEN__) || defined(MuDSL_INTERNAL_PRINT)
#define MuDSL_DO_INTERNAL_PRINT
#endif

#endif