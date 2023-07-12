
#pragma once

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"

#if defined(ME_CS_DISABLE_EXCEPTIONS)
namespace ME::CSharpWrapper {
inline void throw_exception(const char* message) {
    (void)message;
    std::abort();
}
}  // namespace ME::CSharpWrapper
#else
#include <exception>

namespace ME::CSharpWrapper {
class mono_exception : public std::exception {
public:
    mono_exception(const char* message) : std::exception(message) {}
};

inline void throw_exception(const char* message) { throw mono_exception(message); }
}  // namespace ME::CSharpWrapper
#endif

#include <cassert>

#define ME_CS_CALLABLE(func_name) [](auto... args) -> decltype(auto) { return func_name(std::move(args)...); }

#define ME_CS_GET(type, method_name) [](uintptr_t ptr) -> decltype(auto) { return reinterpret_cast<type*>(ptr)->method_name(); }
#define ME_CS_SET(type, method_name) [](uintptr_t ptr, auto arg) -> void { reinterpret_cast<type*>(ptr)->method_name(std::move(arg)); }
#define ME_CS_METHOD(type, method_name) [](uintptr_t ptr, auto... args) -> decltype(auto) { return reinterpret_cast<type*>(ptr)->method_name(std::move(args)...); }
#define ME_CS_STATIC_METHOD(type, method_name) [](auto... args) -> decltype(auto) { return type::method_name(std::move(args)...); }

#include <mono/jit/jit.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/object.h>
#include <mono/utils/mono-publib.h>

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace ME::CSharpWrapper {
class method;

template <typename... Args>
void internal_init_params(MonoDomain* domain, void** current) {}

template <typename T, typename... Args>
void internal_init_params(MonoDomain* domain, void** current, T&& t, Args&&... other) {
    using TypeToConvert = typename std::decay<T>::type;
    auto converted = to_mono_converter<TypeToConvert>::convert(domain, std::forward<T>(t));
    static_assert(std::is_pointer<decltype(converted)>::value, "conversion function must return objects by pointer");

    *current = (void*)converted;
    internal_init_params(domain, current + 1, std::forward<Args>(other)...);
}

template <typename T>
struct internal_deduce_functor_type {
    template <typename... Args>
    static decltype(auto) invoke(const method& this_ref, MonoObject* self, Args&&... args) {
        return internal_deduce_functor_type<T(Args...)>::invoke(this_ref, self, std::forward<Args>(args)...);
    }
};

template <typename T, typename... Args>
struct internal_deduce_functor_type<T(Args...)> {
    template <typename Method, typename... CurrentArgs>
    static auto invoke(const Method& this_ref, MonoObject* self, CurrentArgs&&... args) {
        std::array<void*, sizeof...(args)> params;
        internal_init_params(this_ref.get_domain(), params.data(), std::forward<CurrentArgs>(args)...);
        MonoObject* result = mono_runtime_invoke(this_ref.get_pointer(), self, params.data(), nullptr);
        if (!std::is_void<T>::value && result == nullptr) {
            throw_exception("mono method returned null");
        }
        return from_mono_converter<T>::convert(this_ref.get_domain(), result);
    }
};

template <typename T>
struct internal_get_function_type;

template <typename R, typename... Args>
struct internal_get_function_type<R(Args...)> {
    using type = std::function<R(Args...)>;
    using result_type = R;

    static constexpr size_t argument_count = sizeof...(Args);
};

class method {
    MonoDomain* m_domain;
    MonoMethod* m_native_ptr;

public:
    method(MonoDomain* domain, MonoMethod* native_ptr) : m_domain(domain), m_native_ptr(native_ptr) { ME_ASSERT(m_domain != nullptr); }

    MonoMethod* get_pointer() const { return m_native_ptr; }

    MonoDomain* get_domain() const { return m_domain; }

    template <typename T, typename... Args>
    decltype(auto) invoke_instance(MonoObject* self, Args&&... args) const {
        return internal_deduce_functor_type<T>::invoke(*this, self, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args, typename Object>
    decltype(auto) invoke_instance(const Object& object, Args&&... args) const {
        return invoke_instance<T>(object.get_pointer(), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    decltype(auto) invoke_static(Args&&... args) const {
        return invoke_instance<T>(static_cast<MonoObject*>(nullptr), std::forward<Args>(args)...);
    }

    template <typename FunctionSignature>
    auto as_function() const {
        using FunctorInfo = internal_get_function_type<FunctionSignature>;
        using ResultType = typename FunctorInfo::result_type;
        using Functor = typename FunctorInfo::type;

        return Functor([method = *this](auto&&... args) { return method.invoke_static<ResultType>(std::forward<decltype(args)>(args)...); });
    }

    const char* get_signature() const {
        ME_ASSERT(m_native_ptr != nullptr);
        return mono_method_get_reflection_name(m_native_ptr);
    }

    const char* get_name() const {
        ME_ASSERT(m_native_ptr != nullptr);
        return mono_method_get_name(m_native_ptr);
    }

    std::string to_string() const { return get_signature(); }
};

template <class T>
std::string to_string(const T& m) {
    return m.to_string();
}

inline MonoDomain* get_current_domain() { return mono_domain_get(); }

class field_view {
    MonoClass* m_class;

public:
    class field_iterator {
        void* m_iter = nullptr;
        MonoClass* m_class = nullptr;
        MonoClassField* m_field = nullptr;

    public:
        field_iterator() = default;

        field_iterator(MonoClass* cl, void* iter) : m_iter(iter), m_class(cl) { m_field = mono_class_get_fields(m_class, &m_iter); }

        const char* operator*() const { return mono_field_get_name(m_field); }

        bool operator==(const field_iterator& it) const { return m_field == it.m_field; }

        bool operator!=(const field_iterator& it) const { return !(*this == it); }

        field_iterator& operator++() {
            m_field = mono_class_get_fields(m_class, &m_iter);
            return *this;
        }

        field_iterator operator++(int) {
            auto copy = *this;
            ++(*this);
            return copy;
        }
    };

    field_view(MonoClass* cl) : m_class(cl) {}

    auto begin() const { return field_iterator(m_class, nullptr); }

    auto end() const { return field_iterator(); }

    size_t size() const { return (size_t)mono_class_num_fields(m_class); }
};

class property_view {
    MonoClass* m_class;

public:
    class property_iterator {
        void* m_iter = nullptr;
        MonoClass* m_class = nullptr;
        MonoProperty* m_property = nullptr;

    public:
        property_iterator() = default;

        property_iterator(MonoClass* cl, void* iter) : m_iter(iter), m_class(cl) { m_property = mono_class_get_properties(m_class, &m_iter); }

        const char* operator*() const { return mono_property_get_name(m_property); }

        bool operator==(const property_iterator& it) const { return m_property == it.m_property; }

        bool operator!=(const property_iterator& it) const { return !(*this == it); }

        property_iterator& operator++() {
            m_property = mono_class_get_properties(m_class, &m_iter);
            return *this;
        }

        property_iterator operator++(int) {
            auto copy = *this;
            ++(*this);
            return copy;
        }
    };

    property_view(MonoClass* cl) : m_class(cl) {}

    auto begin() const { return property_iterator(m_class, nullptr); }

    auto end() const { return property_iterator(); }

    size_t size() const { return (size_t)mono_class_num_properties(m_class); }
};

struct type_accessor {
    static MonoClass* get_type(MonoObject* obj) { return mono_object_get_class(obj); }

    static MonoClass* get_type(const MonoString* str) { return mono_get_string_class(); }

    static MonoClass* get_type(const uint16_t* t) { return mono_get_uint16_class(); }

    static MonoClass* get_type(const int16_t* t) { return mono_get_int16_class(); }

    static MonoClass* get_type(const uint32_t* t) { return mono_get_uint32_class(); }

    static MonoClass* get_type(const int32_t* t) { return mono_get_int32_class(); }

    static MonoClass* get_type(const uint64_t* t) { return mono_get_uint64_class(); }

    static MonoClass* get_type(const int64_t* t) { return mono_get_int64_class(); }

    static MonoClass* get_type(const char* t) { return mono_get_byte_class(); }

    static MonoClass* get_type(const wchar_t* t) { return mono_get_char_class(); }

    static MonoClass* get_type(const bool* t) { return mono_get_boolean_class(); }
};

template <typename T>
struct to_mono_converter {
    static auto convert(MonoDomain* domain, const typename std::decay<T>::type& t) { return std::addressof(t); }
};

template <typename T>
struct from_mono_converter {
    template <typename U>
    static auto convert(MonoDomain* domain, U t) -> typename std::enable_if<std::is_same<U, MonoObject*>::value, T>::type {
        return *(T*)mono_object_unbox(t);
    }

    template <typename U>
    static auto convert(MonoDomain* domain, const U* t) -> typename std::enable_if<!std::is_same<U, MonoObject*>::value, T>::type {
        return *t;
    }
};

template <typename T>
struct can_be_trivially_converted {
    static constexpr size_t value = std::is_standard_layout<T>::value;
};

class method_view {
    MonoClass* m_class = nullptr;
    MonoDomain* m_domain = nullptr;

public:
    class method_iterator {
        void* m_iter = nullptr;
        MonoDomain* m_domain = nullptr;
        MonoClass* m_class = nullptr;
        method m_method;

    public:
        method_iterator() : m_method(nullptr, nullptr) {}

        method_iterator(MonoDomain* domain, MonoClass* cl, void* iter) : m_iter(iter), m_domain(domain), m_class(cl), m_method(m_domain, mono_class_get_methods(m_class, &m_iter)) {}

        const method& operator*() const { return m_method; }

        const method* operator->() const { return std::addressof(m_method); }

        bool operator==(const method_iterator& it) const { return m_method.get_pointer() == it.m_method.get_pointer(); }

        bool operator!=(const method_iterator& it) const { return !(*this == it); }

        method_iterator& operator++() {
            auto m = mono_class_get_methods(m_class, &m_iter);
            m_method = method(m_domain, m);
            return *this;
        }

        method_iterator operator++(int) {
            auto copy = *this;
            ++(*this);
            return copy;
        }
    };

    method_view(MonoDomain* domain, MonoClass* cl) : m_class(cl), m_domain(domain) {}

    auto begin() const { return method_iterator(m_domain, m_class, nullptr); }

    auto end() const { return method_iterator(); }

    size_t size() const { return (size_t)mono_class_num_methods(m_class); }
};

class object;

class class_type {
public:
    class static_field_wrapper {
        MonoDomain* m_domain = nullptr;
        MonoVTable* m_vtable = nullptr;
        MonoClass* m_parent = nullptr;
        MonoClassField* m_field = nullptr;

        MonoObject* get_underlying_object_no_static_constructor_check() const {
            MonoObject* result = nullptr;
            class_type field_type(mono_field_get_type(m_field));
            if (mono_class_is_valuetype(field_type.get_pointer())) {
                std::aligned_storage_t<512> data;
                mono_field_static_get_value(m_vtable, m_field, (void*)&data);
                result = mono_value_box(m_domain, field_type.get_pointer(), (void*)&data);
            } else {
                mono_field_static_get_value(m_vtable, m_field, (void*)&result);
            }
            return result;
        }

        MonoObject* get_underlying_object() const {
            auto result = get_underlying_object_no_static_constructor_check();
            if (result == nullptr) {
                invoke_static_constructor(m_parent);
                result = get_underlying_object_no_static_constructor_check();
            }
            return result;
        }

    public:
        static_field_wrapper(MonoDomain* domain, MonoClass* parent, MonoClassField* field) : m_domain(domain), m_parent(parent), m_field(field) { m_vtable = mono_class_vtable(m_domain, m_parent); }

        template <typename T = object>
        T get() {
            return as<T>();
        }

        template <typename T>
        void set(T&& value) {
            auto obj = to_mono_converter<T>::convert(m_domain, std::forward<T>(value));
            mono_field_static_set_value(m_parent, m_field, (void*)obj);
        }

        template <typename T>
        void operator=(T&& value) {
            set(std::forward<T>(value));
        }

        template <typename T>
        operator T() const {
            return as<T>();
        }

        template <typename T>
        T as() const {
            return from_mono_converter<T>::convert(m_domain, get_underlying_object());
        }
    };

private:
    MonoClass* m_class = nullptr;

    static void invoke_static_constructor(MonoClass* cl) {
        MonoMethod* m = mono_class_get_method_from_name(cl, ".cctor", 0);
        mono_runtime_invoke(m, nullptr, nullptr, nullptr);
    }

    MonoMethod* get_method_pointer_or_null(const char* name) const {
        auto desc = mono_method_desc_new(name, false);
        if (desc == nullptr) {
            throw_exception("invalid method signature");
        }
        // also handle virtual methods
        MonoClass* current_class = m_class;
        while (current_class != nullptr) {
            MonoMethod* m = mono_method_desc_search_in_class(desc, current_class);
            if (m != nullptr) {
                mono_method_desc_free(desc);
                return m;  // method was found
            }
            current_class = mono_class_get_parent(current_class);
        }
        // method was not found
        mono_method_desc_free(desc);
        return nullptr;
    }

public:
    class_type() = default;

    class_type(MonoImage* image, const char* namespace_name, const char* class_name) {
        m_class = mono_class_from_name(image, namespace_name, class_name);
        if (m_class == nullptr) {
            throw_exception("could not find class in image");
        }
    }

    class_type(MonoClass* cl) : m_class(cl) {}

    class_type(MonoType* type) : m_class(mono_class_from_mono_type(type)) {}

    MonoClass* get_pointer() const { return m_class; }

    MonoClassField* get_field_pointer(const char* field_name) const {
        ME_ASSERT(m_class != nullptr);
        MonoClassField* field = mono_class_get_field_from_name(m_class, field_name);
        if (field == nullptr) {
            throw_exception("could not find field in class");
        }
        return field;
    }

    bool has_field(const char* field_name) const {
        ME_ASSERT(m_class != nullptr);
        MonoClassField* field = mono_class_get_field_from_name(m_class, field_name);
        return field != nullptr;
    }

    MonoProperty* get_property_pointer(const char* property_name) const {
        ME_ASSERT(m_class != nullptr);
        MonoProperty* prop = mono_class_get_property_from_name(m_class, property_name);
        if (prop == nullptr) {
            throw_exception("could not find property in class");
        }
        return prop;
    }

    bool has_property(const char* property_name) const {
        ME_ASSERT(m_class != nullptr);
        MonoProperty* prop = mono_class_get_property_from_name(m_class, property_name);
        return prop != nullptr;
    }

    MonoMethod* get_method_pointer(const char* name) const {
        ME_ASSERT(m_class != nullptr);
        MonoMethod* m = get_method_pointer_or_null(name);
        if (m == nullptr) {
            throw_exception("could not find method in class");
        }
        return m;
    }

    bool has_method(const char* name) const {
        ME_ASSERT(m_class != nullptr);
        MonoMethod* m = get_method_pointer_or_null(name);
        return m != nullptr;
    }

    template <typename T>
    void set_property(const char* name, const T& value) const {
        MonoProperty* prop = get_property_pointer(name);
        MonoObject* exc = nullptr;
        std::array<void*, 1> params{(void*)to_mono_converter<T>::convert(get_current_domain(), value)};

        mono_property_set_value(prop, nullptr, params.data(), &exc);
        if (exc != nullptr) {
            throw_exception("exception occured while setting property value");
        }
    }

    // template<typename T = object>
    // T get_property(const char* name) const
    //{
    //     MonoProperty* prop = get_property_pointer(name);
    //     MonoObject* exc = nullptr;

    //    MonoObject* result = mono_property_get_value(prop, nullptr, nullptr, &exc);
    //    if (exc != nullptr)
    //    {
    //        throw_exception("excpetion occured while getting property value");
    //    }
    //    return object(result).as<T>();
    //}

    template <typename FunctionSignature>
    auto get_method(const char* method_name) {
        using FunctorTraits = internal_get_function_type<FunctionSignature>;
        auto method_type = get_method_pointer(method_name);

        auto functor = [f = method(get_current_domain(), method_type)](auto&&... args) mutable ->
                typename FunctorTraits::result_type { return f.invoke_static<FunctionSignature>(std::forward<decltype(args)>(args)...); };
        return FunctorTraits::type(std::move(functor));
    }

    static_field_wrapper operator[](const char* field_name) const {
        auto field = get_field_pointer(field_name);
        return static_field_wrapper(get_current_domain(), m_class, field);
    }

    method_view get_methods() const {
        ME_ASSERT(m_class != nullptr);
        return method_view(get_current_domain(), m_class);
    }

    field_view get_fields() const {
        ME_ASSERT(m_class != nullptr);
        return field_view(m_class);
    }

    property_view get_properties() const {
        ME_ASSERT(m_class != nullptr);
        return property_view(m_class);
    }

    const char* get_namespace() const {
        ME_ASSERT(m_class != nullptr);
        return mono_class_get_namespace(m_class);
    }

    class_type get_nesting_type() const {
        ME_ASSERT(m_class != nullptr);
        return class_type(mono_class_get_nesting_type(m_class));
    }

    class_type get_parent_type() const {
        ME_ASSERT(m_class != nullptr);
        return class_type(mono_class_get_parent(m_class));
    }

    const char* get_name() const {
        ME_ASSERT(m_class != nullptr);
        return mono_class_get_name(m_class);
    }

    std::string to_string() const { return get_name(); }
};

class object {
    MonoObject* m_object = nullptr;
    MonoDomain* m_domain = nullptr;
    uint32_t m_gchandle = 0;

    static void alloc_object(MonoDomain* domain, MonoObject** obj, MonoClass* class_type) { *obj = mono_object_new(domain, class_type); }

public:
    class field_wrapper {
        MonoDomain* m_domain = nullptr;
        MonoObject* m_parent = nullptr;
        MonoClassField* m_field = nullptr;

        MonoObject* get_underlying_object() const { return mono_field_get_value_object(m_domain, m_field, m_parent); }

    public:
        field_wrapper(MonoDomain* domain, MonoObject* parent, MonoClassField* field) : m_domain(domain), m_parent(parent), m_field(field) {}

        object get() const { return object(get_underlying_object()); }

        template <typename T>
        void set(T&& value) {
            auto obj = to_mono_converter<T>::convert(m_domain, std::forward<T>(value));
            mono_field_set_value(m_parent, m_field, (void*)obj);
        }

        template <typename T>
        void operator=(T&& value) {
            set(std::forward<T>(value));
        }

        template <typename T>
        operator T() const {
            return as<T>();
        }

        template <typename T>
        T as() const {
            return from_mono_converter<T>::convert(m_domain, get_underlying_object());
        }
    };

    object(MonoDomain* domain, const class_type& class_t) : object(domain, class_t.get_pointer()) {}

    object(MonoObject* object) : m_object(object), m_domain(mono_object_get_domain(object)) {}

    object(MonoDomain* domain, MonoClass* class_t) : m_domain(domain) {
        ME_ASSERT(m_domain != nullptr);
        alloc_object(m_domain, &m_object, class_t);
        mono_runtime_object_init(m_object);
    }

    template <typename... Args>
    object(MonoDomain* domain, const class_type& class_t, const char* construtor_signature, Args&&... args) : m_domain(domain) {
        alloc_object(m_domain, &m_object, class_t.get_pointer());
        MonoMethod* ctor = get_method_pointer(construtor_signature);
        if (ctor == nullptr) {
            throw_exception("could not found appropriate constructor for given class");
        }
        method constructor(m_domain, ctor);
        constructor.invoke_instance<void(Args...)>(*this, std::forward<Args>(args)...);
    }

    object(field_wrapper wrapper) : object(wrapper.get()) {}

    class_type get_class() const {
        ME_ASSERT(m_object != nullptr);
        MonoClass* cl = mono_object_get_class(m_object);
        return class_type(cl);
    }

    MonoObject* get_pointer() const { return m_object; }

    size_t get_gc_generation() const { return (size_t)mono_gc_get_generation(m_object); }

    void lock() {
        ME_ASSERT(m_gchandle == 0);
        m_gchandle = mono_gchandle_new(m_object, true);
    }

    void unlock() {
        ME_ASSERT(m_gchandle != 0);
        mono_gchandle_free(m_gchandle);
        m_gchandle = 0;
    }

    MonoClassField* get_field_pointer(const char* field_name) const { return get_class().get_field_pointer(field_name); }

    MonoProperty* get_property_pointer(const char* property_name) const { return get_class().get_property_pointer(property_name); }

    MonoMethod* get_method_pointer(const char* name) const { return get_class().get_method_pointer(name); }

    bool has_field(const char* field_name) const { return get_class().has_field(field_name); }

    bool has_property(const char* property_name) const { return get_class().has_property(property_name); }

    bool has_method(const char* name) const { return get_class().has_method(name); }

    field_wrapper operator[](const char* field_name) const {
        MonoClassField* field = get_field_pointer(field_name);
        return field_wrapper(m_domain, m_object, field);
    }

    template <typename T>
    void set_property(const char* name, const T& value) const {
        MonoProperty* prop = get_property_pointer(name);
        MonoObject* exc = nullptr;
        std::array<void*, 1> params{(void*)to_mono_converter<T>::convert(m_domain, value)};

        mono_property_set_value(prop, m_object, params.data(), &exc);
        if (exc != nullptr) {
            throw_exception("exception occured while setting property value");
        }
    }

    object get_property(const char* name) const {
        MonoProperty* prop = get_property_pointer(name);
        MonoObject* exc = nullptr;

        MonoObject* result = mono_property_get_value(prop, m_object, nullptr, &exc);
        if (exc != nullptr) {
            throw_exception("excpetion occured while getting property value");
        }
        return object(result);
    }

    template <typename T>
    T get_property(const char* name) const {
        return get_property(name).as<T>();
    }

    template <typename FunctionSignature>
    auto get_method(const char* method_name) {
        using FunctorTraits = internal_get_function_type<FunctionSignature>;
        auto method_type = get_method_pointer(method_name);
        auto functor = [f = method(m_domain, method_type), o = m_object](auto&&... args) mutable ->
                typename FunctorTraits::result_type { return f.invoke_instance<FunctionSignature>(o, std::forward<decltype(args)>(args)...); };
        using MethodType = typename FunctorTraits::type;
        return MethodType(std::move(functor));
    }

    template <typename FunctionSignature>
    auto get_static_method(const char* method_name) {
        using FunctorTraits = internal_get_function_type<FunctionSignature>;
        auto method_type = get_method_pointer(method_name);
        auto functor = [f = method(m_domain, method_type)](auto&&... args) mutable ->
                typename FunctorTraits::result_type { return f.invoke_static<FunctionSignature>(std::forward<decltype(args)>(args)...); };
        return FunctorTraits::type(std::move(functor));
    }

    template <typename T>
    T as() const {
        ME_ASSERT(m_object != nullptr);
        return from_mono_converter<T>::convert(m_domain, m_object);
    }

    std::string to_string() const { return as<std::string>(); }
};

template <typename T, typename Container>
auto internal_copy_to_mono_array(MonoDomain* domain, const Container& vec, MonoArray* arr) -> typename std::enable_if<can_be_trivially_converted<T>::value>::type {
    for (size_t i = 0; i < vec.size(); i++) {
        mono_array_set(arr, T, i, vec[i]);
    }
}

template <typename T, typename Container>
static auto internal_copy_to_mono_array(MonoDomain* domain, const Container& vec, MonoArray* arr) -> typename std::enable_if<!can_be_trivially_converted<T>::value>::type {
    for (size_t i = 0; i < vec.size(); i++) {
        auto converted = to_mono_converter<T>::convert(domain, vec[i]);
        mono_array_setref(arr, i, converted);
    }
}

template <>
struct from_mono_converter<void> {
    static void convert(MonoDomain* domain, MonoObject* t) { (void)t; }
};

template <>
struct to_mono_converter<const char*> {
    static MonoString* convert(MonoDomain* domain, const char* str) { return mono_string_new(domain, str); }
};

template <>
struct to_mono_converter<char*> {
    static MonoString* convert(MonoDomain* domain, const char* str) { return mono_string_new(domain, str); }
};

template <typename T>
struct from_mono_converter<std::vector<T>> {
    static std::vector<T> convert(MonoDomain* domain, MonoArray* arr) {
        size_t size = mono_array_length(arr);
        std::vector<T> vec(size);
        for (size_t i = 0; i < size; i++) {
            auto object = mono_array_get(arr, MonoObject*, i);
            vec[i] = from_mono_converter<T>::convert(domain, object);
        }
        return std::move(vec);
    }

    static std::vector<T> convert(MonoDomain* domain, MonoObject* obj) {
        MonoArray* arr = reinterpret_cast<MonoArray*>(obj);
        return convert(domain, arr);
    }
};

template <typename T>
struct to_mono_converter<std::vector<T>> {
    static MonoArray* convert(MonoDomain* domain, const std::vector<T>& vec) {
        if (vec.empty()) return nullptr;

        auto converted = to_mono_converter<T>::convert(domain, vec.front());
        MonoClass* class_type = type_accessor::get_type(converted);
        MonoArray* arr = mono_array_new(domain, class_type, vec.size());

        internal_copy_to_mono_array<T>(domain, vec, arr);
        return arr;
    }
};

template <typename T, size_t N>
struct from_mono_converter<std::array<T, N>> {
    static std::array<T, N> convert(MonoDomain* domain, MonoArray* arr) {
        size_t size = mono_array_length(arr);
        std::array<T, N> vec;
        for (size_t i = 0; i < (N < size ? N : size); i++) {
            auto object = mono_array_get(arr, MonoObject*, i);
            vec[i] = from_mono_converter<T>::convert(domain, object);
        }
        return vec;
    }

    static std::array<T, N> convert(MonoDomain* domain, MonoObject* obj) {
        MonoArray* arr = reinterpret_cast<MonoArray*>(obj);
        return convert(domain, arr);
    }
};

template <typename T, size_t N>
struct to_mono_converter<std::array<T, N>> {
    static MonoArray* convert(MonoDomain* domain, const std::array<T, N>& vec) {
        if (vec.empty()) return nullptr;

        auto converted = to_mono_converter<T>::convert(domain, vec.front());
        MonoClass* class_type = type_accessor::get_type(converted);
        MonoArray* arr = mono_array_new(domain, class_type, vec.size());

        internal_copy_to_mono_array<T>(domain, vec, arr);
        return arr;
    }
};

template <>
struct from_mono_converter<std::string> {
    static std::string convert(MonoDomain* domain, MonoString* str) {
        std::string result;
        MonoError err;
        char* utf8str = mono_string_to_utf8_checked(str, &err);
        if (err.error_code == MONO_ERROR_NONE) {
            result = utf8str;
            mono_free(utf8str);
        }
        return result;
    }

    static std::string convert(MonoDomain* domain, MonoObject* obj) {
        MonoObject* exc = nullptr;
        MonoString* str = mono_object_to_string(obj, &exc);
        if (exc != nullptr) {
            throw_exception("cannot convert object to string");
        }
        return convert(domain, str);
    }
};

template <>
struct to_mono_converter<std::string> {
    static MonoString* convert(MonoDomain* domain, const std::string& str) { return mono_string_new(domain, str.c_str()); }
};

template <>
struct to_mono_converter<std::wstring> {
    static MonoString* convert(MonoDomain* domain, const std::wstring& str) { return mono_string_new_utf16(domain, str.c_str(), (int32_t)str.size()); }
};

template <>
struct from_mono_converter<std::wstring> {
    static std::wstring convert(MonoDomain* domain, MonoString* str) { return mono_string_chars(str); }

    static std::wstring convert(MonoDomain* domain, MonoObject* obj) {
        MonoObject* exc = nullptr;
        MonoString* str = mono_object_to_string(obj, &exc);
        if (exc != nullptr) {
            throw_exception("cannot convert object to string");
        }
        return convert(domain, str);
    }
};

template <>
struct to_mono_converter<object> {
    static MonoObject* convert(MonoDomain* domain, const object& obj) { return obj.get_pointer(); }
};

template <>
struct from_mono_converter<object> {
    static object convert(MonoDomain* domain, MonoObject* obj) { return object(obj); }
};

template <>
struct can_be_trivially_converted<object> {
    static constexpr size_t value = false;
};

inline std::string to_string(MonoString* str) {
    std::string result;
    MonoError err;
    char* utf8str = mono_string_to_utf8_checked(str, &err);
    if (err.error_code == MONO_ERROR_NONE) {
        result = utf8str;
        mono_free(utf8str);
    }
    return result;
}

inline std::wstring to_wstring(MonoString* str) {
    std::wstring result;
    wchar_t* utf16str = mono_string_chars(str);
    result = utf16str;
    return result;
}

inline object to_object(MonoObject* obj) { return object(obj); }

template <typename T>
struct internal_convert_type_to_mono {
    using decay_t = typename std::decay<T>::type;

    using result =
            typename std::conditional<can_be_trivially_converted<decay_t>::value, const decay_t*, decltype(to_mono_converter<T>::convert(std::declval<MonoDomain*>(), std::declval<decay_t>()))>::type;
};

template <>
struct internal_convert_type_to_mono<void> {
    using result = void;
};

template <typename T>
class internal_convert_tuple_types_to_mono;

template <typename T>
class internal_convert_tuple_types_to_mono<std::tuple<T>> {
    using result_type = typename internal_convert_type_to_mono<T>::result;

public:
    using result = std::tuple<result_type>;
};

template <typename T, typename... Args>
class internal_convert_tuple_types_to_mono<std::tuple<T, Args...>> {
    template <typename... Args1, typename... Args2>
    static auto merge_tuples(std::tuple<Args1...>*, std::tuple<Args2...>*) -> std::tuple<Args1..., Args2...>;

    using result_head_tuple = typename internal_convert_tuple_types_to_mono<std::tuple<T>>::result;
    using result_tail_tuple = typename internal_convert_tuple_types_to_mono<std::tuple<Args...>>::result;

public:
    using result = decltype(merge_tuples((result_head_tuple*)nullptr, (result_tail_tuple*)nullptr));
};

template <>
class internal_convert_tuple_types_to_mono<std::tuple<>> {
public:
    using result = std::tuple<>;
};

template <typename T>
struct internal_function_wrapper;

template <typename R, typename... Args>
struct internal_function_wrapper<R(Args...)> {
private:
    template <typename F, typename MonoReturnType, typename... MonoArgs>
    static auto invoke_inner_function(MonoArgs&&... args) -> typename std::enable_if<!std::is_void<MonoReturnType>::value, MonoReturnType>::type {
        MonoDomain* domain = get_current_domain();
        std::aligned_storage<sizeof(F)> dummy;
        auto result = reinterpret_cast<F*>(&dummy)->operator()(from_mono_converter<Args>::convert(domain, args)...);
        return to_mono_converter<R>::convert(domain, std::move(result));
    }

    template <typename F, typename MonoReturnType, typename... MonoArgs>
    static auto invoke_inner_function(MonoArgs&&... args) -> typename std::enable_if<std::is_void<MonoReturnType>::value, void>::type {
        MonoDomain* domain = get_current_domain();
        std::aligned_storage<sizeof(F)> dummy;
        reinterpret_cast<F*>(&dummy)->operator()(from_mono_converter<Args>::convert(domain, args)...);
    }

    using argument_list_tuple = typename internal_convert_tuple_types_to_mono<std::tuple<Args...>>::result;
    using return_type = typename internal_convert_type_to_mono<R>::result;

    template <typename F, typename MonoReturnType, typename... MonoArgs>
    static auto get_impl(MonoReturnType*, std::tuple<MonoArgs...>*) {
        using PureFuncType = MonoReturnType (*)(MonoArgs...);
        PureFuncType pure_func = [](MonoArgs... args) -> MonoReturnType { return invoke_inner_function<F, MonoReturnType, MonoArgs...>(std::forward<MonoArgs>(args)...); };
        return pure_func;
    }

public:
    template <typename F>
    static auto get() {
        static_assert(std::is_convertible<F, R (*)(Args...)>::value, "functor type must be convertible to function pointer");
        return get_impl<F>((return_type*)nullptr, (argument_list_tuple*)nullptr);
    }
};

class mono {
    std::string m_mono_root_dir;
    MonoDomain* m_domain = nullptr;

public:
    mono(const char* mono_root_dir) : m_mono_root_dir(mono_root_dir) { mono_set_dirs((m_mono_root_dir + "/lib").c_str(), (m_mono_root_dir + "/etc").c_str()); }

    mono(const char* mono_root_dir, const char* mono_assembly_dir, const char* mono_config_dir) : m_mono_root_dir(mono_root_dir) { mono_set_dirs(mono_assembly_dir, mono_config_dir); }

    mono(const mono&) = delete;
    mono& operator=(const mono&) = delete;

    mono(mono&& other) noexcept : m_mono_root_dir(std::move(other.m_mono_root_dir)), m_domain(other.m_domain) { other.m_domain = nullptr; }

    mono& operator=(mono&& other) noexcept {
        m_mono_root_dir = std::move(other.m_mono_root_dir);
        m_domain = other.m_domain;

        other.m_domain = nullptr;

        return *this;
    }

    ~mono() {
        if (m_domain != nullptr) {
            mono_jit_cleanup(m_domain);
        }
    }

    void init_jit(const char* app_name) {
        m_domain = mono_jit_init(app_name);
        if (m_domain == nullptr) {
            throw_exception("mono_jit_init failed");
        }
    }

    void init_jit(const char* app_name, const char* framework_version) {
        m_domain = mono_jit_init_version(app_name, framework_version);
        if (m_domain == nullptr) {
            throw_exception("mono_jit_init failed");
        }
    }

    MonoDomain* get_domain() const { return m_domain; }

    const std::string& get_root_dir() const { return m_mono_root_dir; }

    template <typename FunctionSignature, typename F>
    void add_internal_call(const char* signature, F&& f) {
        auto wrapper = internal_function_wrapper<FunctionSignature>::template get<F>();
        mono_add_internal_call(signature, reinterpret_cast<const void*>(wrapper));
    }
};

class assembly {
    MonoDomain* m_domain = nullptr;
    MonoAssembly* m_assembly = nullptr;
    MonoImage* m_assembly_image = nullptr;

    mutable std::map<std::string, MonoMethod*, std::less<>> m_method_cache;

    bool add_to_cache(const char* method_signature) const {
        MonoMethodDesc* desc = mono_method_desc_new(method_signature, false);
        if (desc == nullptr) {
            throw_exception("invalid method signature");
        }
        MonoMethod* method_ptr = mono_method_desc_search_in_image(desc, m_assembly_image);
        mono_method_desc_free(desc);
        if (method_ptr != nullptr) {
            m_method_cache.emplace(method_signature, method_ptr);
        }
        return method_ptr != nullptr;
    }

public:
    assembly(MonoDomain* domain, const char* path) : m_domain(domain) {
        ME_ASSERT(domain != nullptr);

        m_assembly = mono_domain_assembly_open(domain, path);
        if (m_assembly == nullptr) {
            throw_exception("could not open assembly");
        }

        m_assembly_image = mono_assembly_get_image(m_assembly);
        if (m_assembly_image == nullptr) {
            m_assembly = nullptr;
            throw_exception("could not get assembly image");
        }
    }

    MonoAssembly* get_assembly() const { return m_assembly; }

    MonoImage* get_image() const { return m_assembly_image; }

    method get_method(const char* signature) const {
        if (!has_method(signature)) {
            throw_exception("could not find method in assembly");
            return method(m_domain, nullptr);
        } else {
            return method(m_domain, m_method_cache[signature]);
        }
    }

    bool has_method(const char* signature) const {
        auto it = m_method_cache.find(signature);
        if (it != m_method_cache.end()) {
            return true;
        } else {
            bool added = add_to_cache(signature);
            return added;
        }
    }
};

template <typename T, typename... Args>
void internal_append_to_command(T&& command) {}

template <typename T, typename U, typename... Args>
void internal_append_to_command(T&& command, U&& to_append, Args&&... other) {
    command += " \"";
    command += to_append;
    command += '\"';
    internal_append_to_command(std::forward<T>(command), std::forward<Args>(other)...);
}

class compiler {
    std::string m_compiler_path;

public:
    compiler(const char* mono_root_dir) : m_compiler_path('\"' + std::string(mono_root_dir) + "/bin/mcs\"") {}

    compiler(const std::string& mono_root_dir) : m_compiler_path('\"' + mono_root_dir + "/bin/mcs\"") {}

    compiler(const char* mono_root_dir, const char* mono_compiler_path) : m_compiler_path(mono_compiler_path) { (void)mono_root_dir; }

    const std::string& get_path() const { return m_compiler_path; }

    template <typename... Paths>
    void build_library(const char* library_name, Paths&&... paths) {
        std::string command = '\"' + m_compiler_path;
        internal_append_to_command(command, std::forward<Paths>(paths)...);

        command += " -target:library";
        command += " -out:\"";
        command += library_name;
        command += "\"\"";

        std::system(command.c_str());
    }
};

std::ostream& operator<<(std::ostream& out, const object& obj);
std::ostream& operator<<(std::ostream& out, const object::field_wrapper& wrapper);
std::ostream& operator<<(std::ostream& out, const class_type::static_field_wrapper& wrapper);
std::ostream& operator<<(std::ostream& out, const method& m);
std::ostream& operator<<(std::ostream& out, const class_type& cl);

class gc {
public:
    static void collect(size_t generation) { mono_gc_collect((int)generation); }

    static size_t get_collection_count(size_t generation) { return (size_t)mono_gc_collection_count((int)generation); }

    static size_t get_max_generation() { return (size_t)mono_gc_max_generation(); }

    static size_t get_total_heap_size() { return (size_t)mono_gc_get_heap_size(); }

    static size_t get_used_heap_size() { return (size_t)mono_gc_get_used_size(); }
};

template <class T, typename U>
constexpr std::ptrdiff_t internal_get_member_offset(U T::*member) {
    std::aligned_storage_t<sizeof(T)> dummy;
    return (uint8_t*)std::addressof(reinterpret_cast<T*>(&dummy)->*member) - (uint8_t*)std::addressof(dummy);
}

class code_generator {
    std::map<std::type_index, std::string> m_type_cache;
    std::ostream& m_out;
    mono& m_mono;

    template <typename T>
    const char* get_type_name() {
        auto it = m_type_cache.find(typeid(T));
        if (it == m_type_cache.end()) {
            throw_exception("type was not added to generator before used as a field");
        }
        return it->second.c_str();
    }

    template <typename R>
    void generate_method_paramater_names(std::string& args, R (*)() = nullptr) {}

    template <typename R, typename T, typename... Args>
    void generate_method_paramater_names(std::string& res, R (*)(T, Args...) = nullptr) {
        if (!res.empty()) res += ", ";
        res += get_type_name<T>();
        res += " _arg" + std::to_string(sizeof...(Args) + 1);

        generate_method_paramater_names<R, Args...>(res);
    }

    template <typename R>
    void generate_method_paramater_types(std::string& args, R (*)() = nullptr) {}

    template <typename R, typename T, typename... Args>
    void generate_method_paramater_types(std::string& res, R (*)(T, Args...) = nullptr) {
        if (!res.empty()) res += ",";
        res += get_type_name<T>();

        generate_method_paramater_types<R, Args...>(res);
    }

public:
    code_generator(mono& m, std::ostream& out) : m_out(out), m_mono(m) {
        add_type<char>("byte");
        add_type<wchar_t>("char");
        add_type<float>("float");
        add_type<double>("double");
        add_type<bool>("bool");
        add_type<void>("void");
        add_type<int8_t>("byte");
        add_type<uint8_t>("byte");
        add_type<int16_t>("short");
        add_type<uint16_t>("ushort");
        add_type<int32_t>("int");
        add_type<uint32_t>("uint");
        add_type<int64_t>("long");
        add_type<uint64_t>("ulong");
        add_type<std::string>("string");
        add_type<std::wstring>("string");

        m_out << "using System;\n";
        m_out << "using System.Runtime.CompilerServices;\n";
        m_out << "using System.Runtime.InteropServices;\n";
        m_out << '\n';
    }

    template <typename T>
    void generate_class_header(const char* name) {
        add_type<T>(name);

        m_out << "class " << get_type_name<T>() << '\n';
        m_out << "{\n";
        m_out << "\tprivate IntPtr _nativeHandle;\n";
        m_out << "\tpublic " << name << "(IntPtr nativeHandle) { _nativeHandle = nativeHandle; }\n\n";
    }

    template <typename T>
    void generate_struct_header(const char* name) {
        add_type<T>(name);

        m_out << "[StructLayout(LayoutKind.Explicit)]\n";
        m_out << "struct " << get_type_name<T>() << '\n';
        m_out << "{\n";
    }

    void generate_footer() { m_out << "}\n\n"; }

    template <typename T, typename X>
    void generate_struct_field(const char* field_name, X T::*f) {
        const char* field_type = get_type_name<X>();
        size_t field_offset = internal_get_member_offset(f);

        m_out << "\t[FieldOffset(" << field_offset << ")] ";
        m_out << "public " << field_type << ' ' << field_name << ";\n\n";
    }

    template <typename T, typename X>
    void generate_readonly_struct_field(const char* field_name, X T::*f) {
        const char* field_type = get_type_name<X>();
        size_t field_offset = internal_get_member_offset(f);

        m_out << "\t[FieldOffset(" << field_offset << ")] ";
        m_out << "private " << field_type << " _" << field_name << ";\n";
        m_out << "\tpublic " << field_type << ' ' << field_name << " => _" << field_name << ";\n\n";
    }

    template <typename T, typename X>
    void generate_class_field(const char* field_name, X T::*f) {
        auto getter = (std::string) "get_" + field_name;
        auto setter = (std::string) "set_" + field_name;

        const char* field_type = get_type_name<X>();
        const char* class_type = get_type_name<T>();
        size_t field_offset = internal_get_member_offset(f);

        auto getter_sig = (std::string)class_type + "::" + getter + "(intptr,uint)";
        auto setter_sig = (std::string)class_type + "::" + setter + "(intptr,uint," + field_type + ')';

        m_mono.add_internal_call<X(uintptr_t, uint32_t)>(getter_sig.c_str(), [](uintptr_t ptr, uint32_t offset) -> X { return *reinterpret_cast<X*>((uint8_t*)ptr + offset); });

        m_mono.add_internal_call<void(uintptr_t, uint32_t, X)>(setter_sig.c_str(), [](uintptr_t ptr, uint32_t offset, X x) { *reinterpret_cast<X*>((uint8_t*)ptr + offset) = std::move(x); });

        m_out << "\t// " << getter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern " << field_type << ' ' << getter << "(IntPtr _self, uint _offset);\n";

        m_out << "\t// " << setter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern void " << setter << "(IntPtr _self, uint _offset, " << field_type << " _value);\n";

        m_out << "\tpublic " << field_type << ' ' << field_name << " { get => " << getter << "(_nativeHandle, " << field_offset << "); ";
        m_out << "set => " << setter << "(_nativeHandle, " << field_offset << ", value); }\n";

        m_out << '\n';
    }

    template <typename T, typename X>
    void generate_readonly_class_field(const char* field_name, X T::*f) {
        auto getter = (std::string) "get_" + field_name;

        const char* field_type = get_type_name<X>();
        const char* class_type = get_type_name<T>();
        size_t field_offset = internal_get_member_offset(f);

        auto getter_sig = (std::string)class_type + "::" + getter + "(intptr,uint)";

        m_mono.add_internal_call<X(uintptr_t, uint32_t)>(getter_sig.c_str(), [](uintptr_t ptr, uint32_t offset) -> X { return *reinterpret_cast<X*>((uint8_t*)ptr + offset); });

        m_out << "\t// " << getter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern " << field_type << ' ' << getter << "(IntPtr _self, uint _offset);\n";

        m_out << "\tpublic " << field_type << ' ' << field_name << " => " << getter << "(_nativeHandle, " << field_offset << ");\n\n";
    }

    template <typename T, typename GetCallable, typename SetCallable>
    void generate_class_property(const char* name, GetCallable&& get, SetCallable&& set) {
        using ReturnType = decltype(get(uintptr_t()));

        auto getter = (std::string) "get_" + name;
        auto setter = (std::string) "set_" + name;

        const char* field_type = get_type_name<ReturnType>();
        const char* class_type = get_type_name<T>();

        auto getter_sig = (std::string)class_type + "::" + getter + "(intptr)";
        auto setter_sig = (std::string)class_type + "::" + setter + "(intptr," + field_type + ')';

        m_mono.add_internal_call<ReturnType(uintptr_t)>(getter_sig.c_str(), std::forward<GetCallable>(get));
        m_mono.add_internal_call<void(uintptr_t, ReturnType)>(setter_sig.c_str(), std::forward<SetCallable>(set));

        m_out << "\t// " << getter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern " << field_type << ' ' << getter << "(IntPtr _self);\n";

        m_out << "\t// " << setter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern void " << setter << "(IntPtr _self, " << field_type << " _value);\n";

        m_out << "\tpublic " << field_type << ' ' << name << " { get => " << getter << "(_nativeHandle); ";
        m_out << "set => " << setter << "(_nativeHandle, value); }\n";

        m_out << '\n';
    }

    template <typename T, typename GetCallable>
    void generate_readonly_class_property(const char* name, GetCallable&& get) {
        using ReturnType = decltype(get(uintptr_t()));

        auto getter = (std::string) "get_" + name;

        const char* field_type = get_type_name<ReturnType>();
        const char* class_type = get_type_name<T>();

        auto getter_sig = (std::string)class_type + "::" + getter + "(intptr)";

        m_mono.add_internal_call<ReturnType(uintptr_t)>(getter_sig.c_str(), std::forward<GetCallable>(get));

        m_out << "\t// " << getter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern " << field_type << ' ' << getter << "(IntPtr _self);\n";

        m_out << "\tpublic " << field_type << ' ' << name << " => " << getter << "(_nativeHandle); ";
        m_out << '\n';
    }

    template <typename T, typename GetCallable, typename SetCallable>
    void generate_struct_property(const char* name, GetCallable&& get, SetCallable&& set) {
        using ReturnType = decltype(get(uintptr_t()));

        auto getter = (std::string) "get_" + name;
        auto setter = (std::string) "set_" + name;

        const char* field_type = get_type_name<ReturnType>();
        const char* struct_type = get_type_name<T>();

        auto getter_sig = (std::string)struct_type + "::" + getter + '(' + struct_type + "&)";
        auto setter_sig = (std::string)struct_type + "::" + getter + '(' + struct_type + "&," + field_type + ')';

        m_mono.add_internal_call<ReturnType(uintptr_t)>(getter_sig.c_str(), std::forward<GetCallable>(get));
        m_mono.add_internal_call<void(uintptr_t, ReturnType)>(setter_sig.c_str(), std::forward<SetCallable>(set));

        m_out << "\t// " << getter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern " << field_type << ' ' << getter << "(ref " << struct_type << " _self);\n";

        m_out << "\t// " << setter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern void " << setter << "(ref " << struct_type << " _self, " << field_type << " _value);\n";

        m_out << "\tpublic " << field_type << ' ' << name << " { get => " << getter << "(ref this); ";
        m_out << "set => " << setter << "(ref this, value); }\n";

        m_out << '\n';
    }

    template <typename T, typename GetCallable>
    void generate_readonly_struct_property(const char* name, GetCallable&& get) {
        using ReturnType = decltype(get(uintptr_t()));

        auto getter = (std::string) "get_" + name;

        const char* field_type = get_type_name<ReturnType>();
        const char* struct_type = get_type_name<T>();

        auto getter_sig = (std::string)struct_type + "::" + getter + '(' + struct_type + "&)";

        m_mono.add_internal_call<ReturnType(uintptr_t)>(getter_sig.c_str(), std::forward<GetCallable>(get));

        m_out << "\t// " << getter_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tprivate static extern " << field_type << ' ' << getter << "(ref " << struct_type << " _self);\n";

        m_out << "\tpublic " << field_type << ' ' << name << " => " << getter << "(ref this); ";
        m_out << '\n';
    }

    template <typename T, typename FunctionSignature, typename Callable>
    void generate_static_method(const char* name, Callable&& f) {
        using FuncInfo = internal_get_function_type<FunctionSignature>;

        std::string arguments;
        generate_method_paramater_names(arguments, (FunctionSignature*)nullptr);

        const char* return_type = get_type_name<typename FuncInfo::result_type>();
        const char* class_type = get_type_name<T>();

        std::string sig_args;
        generate_method_paramater_types(sig_args, (FunctionSignature*)nullptr);

        auto method_sig = (std::string)class_type + "::" + name + '(' + sig_args + ')';

        m_mono.add_internal_call<FunctionSignature>(method_sig.c_str(), std::forward<Callable>(f));

        m_out << "\t// " << method_sig << '\n';
        m_out << "\t[MethodImpl(MethodImplOptions.InternalCall)]\n";
        m_out << "\tpublic static extern " << return_type << ' ' << name << '(' << arguments << ");\n\n";
    }

    template <typename T>
    void add_type(const char* name) {
        std::type_index idx = typeid(T);
        if (m_type_cache.find(idx) == m_type_cache.end()) m_type_cache[idx] = name;
    }
};

template <typename T>
class export_struct {
    code_generator& m_generator;

public:
    export_struct(const char* name, code_generator& gen) : m_generator(gen) { m_generator.generate_struct_header<T>(name); }

    ~export_struct() { m_generator.generate_footer(); }

    template <typename X>
    auto& field(const char* name, X T::*f) {
        m_generator.generate_struct_field(name, f);
        return *this;
    }

    template <typename X>
    auto& readonly_field(const char* name, X T::*f) {
        m_generator.generate_readonly_struct_field(name, f);
        return *this;
    }

    template <typename GetCallable, typename SetCallable>
    auto& property(const char* name, GetCallable&& get, SetCallable&& set) {
        m_generator.generate_struct_property<T>(name, std::forward<GetCallable>(get), std::forward<SetCallable>(set));
        return *this;
    }

    template <typename GetCallable>
    auto& property(const char* name, GetCallable&& get) {
        m_generator.generate_readonly_struct_property<T>(name, std::forward<GetCallable>(get));
        return *this;
    }

    template <typename FunctionSignature, typename Callable>
    auto& static_method(const char* name, Callable&& callable) {
        m_generator.generate_static_method<T, FunctionSignature>(name, std::forward<Callable>(callable));
        return *this;
    }
};

template <typename T>
class export_class {
    code_generator& m_generator;

public:
    export_class(const char* name, code_generator& gen) : m_generator(gen) { m_generator.generate_class_header<T>(name); }

    ~export_class() { m_generator.generate_footer(); }

    template <typename X>
    auto& field(const char* name, X T::*f) {
        m_generator.generate_class_field(name, f);
        return *this;
    }

    template <typename X>
    auto& readonly_field(const char* name, X T::*f) {
        m_generator.generate_readonly_class_field(name, f);
        return *this;
    }

    template <typename GetCallable, typename SetCallable>
    auto& property(const char* name, GetCallable&& get, SetCallable&& set) {
        m_generator.generate_class_property<T>(name, std::forward<GetCallable>(get), std::forward<SetCallable>(set));
        return *this;
    }

    template <typename GetCallable>
    auto& property(const char* name, GetCallable&& get) {
        m_generator.generate_readonly_class_property(name, std::forward<GetCallable>(get));
        return *this;
    }

    template <typename FunctionSignature, typename Callable>
    auto& static_method(const char* name, Callable&& callable) {
        m_generator.generate_static_method<T, FunctionSignature>(name, std::forward<Callable>(callable));
        return *this;
    }
};
}  // namespace ME::CSharpWrapper
