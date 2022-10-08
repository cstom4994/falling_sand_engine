#pragma once

#include "Engine/InEngine.h"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <ostream>
#include <optional>
#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <cassert>
#include <iomanip>
#include <functional>
#include <sstream>
#include <exception>
#include <string_view>
#include <fstream>
#include <type_traits>

#include <duktape/duktape.h>

namespace dukpp {
    inline duk_ret_t throw_error(duk_context* ctx) {
        duk_error(ctx, DUK_ERR_TYPE_ERROR, "Property does not have getter or setter.");
        return DUK_RET_TYPE_ERROR;
    }

    class duk_exception : public std::exception {
    public:
        virtual char const* what() const noexcept override {
            return mMsg.c_str();
        }

        template<typename T>
        duk_exception& operator<<(T rhs) {
            std::stringstream ss;
            ss << mMsg << rhs;
            mMsg = ss.str();
            return *this;
        }

    protected:
        std::string mMsg;
    };

    class duk_error_exception : public duk_exception {
    public:
        duk_error_exception(duk_context* ctx, int return_code, bool pop_error = true) {
            if (return_code != 0) {
                duk_get_prop_string(ctx, -1, "stack");
                mMsg = duk_safe_to_string(ctx, -1);
                duk_pop(ctx);

                if (pop_error) {
                    duk_pop(ctx);
                }
            }
        }
    };
}

namespace dukpp {

    class duk_value;

    struct variadic_args {
        typedef std::vector<duk_value> va_t;
        va_t args;

        va_t::iterator begin() noexcept { return args.begin(); }

        METADOT_NODISCARD va_t::const_iterator begin() const noexcept { return args.begin(); }

        va_t::iterator end() noexcept { return args.end(); }

        METADOT_NODISCARD va_t::const_iterator end() const noexcept { return args.end(); }

        duk_value& operator[](va_t::size_type const pos) noexcept { return args[pos]; }

        void push(duk_value&& value) noexcept { args.push_back(value); }

        void push(duk_value const&& value) noexcept { args.push_back(value); }
    };
}

namespace dukpp {
    struct this_context {
        duk_context* mCtx;

        this_context(duk_context* ctx) noexcept : mCtx(ctx) {}

        operator duk_context* () const noexcept { return duk_context(); }

        duk_context* operator->() const noexcept { return duk_context(); }

        duk_context* duk_context() const noexcept { return mCtx; }
    };
}

namespace dukpp {
    // A variant class for Duktape values.
    // This class is not really dependant on the rest of dukpp, but the rest of dukpp is integrated to support it.
    // Script objects are persisted by copying a reference to the object into an array in the heap stash.
    // When we need to push a reference to the object, we just look up that reference in the stash.

    // duk_values can be copied freely. We use reference counting behind the scenes to keep track of when we need
    // to remove our reference from the heap stash. Memory for reference counting is only allocated once a duk_value
    // is copied (either by copy constructor or operator=). std::move can be used if you are trying to avoid ref counting
    // for some reason.

    // One script object can have multiple, completely separate duk_values "pointing" to it - in this case, there will be
    // multiple entries in the "ref array" that point to the same object. This will happen if the same script object is
    // put on the stack and turned into a duk_value multiple times independently (copy-constructing/operator=-ing
    // duk_values will not do this!). This is okay, as we are only keeping track of these objects to prevent garbage
    // collection (and access them later). This could be changed to use a map structure to look up one canonical entry per
    // script object in the "ref array" (I guess it would be more like a ref map in this case), but this would require a map
    // lookup every time we construct a duk_value. The performance difference probably isn't *that* noticeable (a good map
    // would probably be amortized constant-time lookup), but I am guessing constructing many separate duk_values that point
    // to the same script object isn't a very common thing.
    class duk_value {
    public:
        enum class Type : uint8_t {
            //None = DUK_TYPE_NONE,
            Undefined = DUK_TYPE_UNDEFINED,
            NullRef = DUK_TYPE_NULL,
            Boolean = DUK_TYPE_BOOLEAN,
            Number = DUK_TYPE_NUMBER,
            String = DUK_TYPE_STRING,
            Object = DUK_TYPE_OBJECT,
            Buffer = DUK_TYPE_BUFFER,
            Pointer = DUK_TYPE_POINTER,
            LightFunc = DUK_TYPE_LIGHTFUNC  // not implemented
        };

        enum class BufferType : uint8_t {
            Undefined = 0,
            Fixed = 1,
            Dynamic = 2,
            External = 3
        };

        // default constructor just makes an undefined-type duk_value
        inline duk_value() : mContext(nullptr), mType(Type::Undefined), mRefCount(nullptr) {}

        virtual ~duk_value() {
            // release any references we have
            release_ref_count();
        }

        // move constructor
        inline duk_value(duk_value&& move) noexcept : mContext(move.mContext), mType(move.mType),
            mPOD(move.mPOD), mBufSize(move.mBufSize),
            mBufferType(move.mBufferType), mRefCount(move.mRefCount) {
            if (mType == Type::String)
                mString = std::move(move.mString);

            move.mType = Type::Undefined;
            move.mRefCount = nullptr;
            move.mBufSize = 0;
            move.mBufferType = BufferType::Undefined;
        }

        inline duk_value& operator=(const duk_value& rhs);

        // copy constructor
        inline duk_value(const duk_value& copy) : duk_value() {
            *this = copy;
        }

        // equality operator
        bool operator==(const duk_value& rhs) const;

        inline bool operator!=(const duk_value& rhs) const {
            return !(*this == rhs);
        }

        // copies the object at idx on the stack into a new duk_value and returns it
        static duk_value copy_from_stack(duk_context* ctx, duk_idx_t idx = -1);

    protected:
        static duk_ret_t json_decode_safe(duk_context* ctx, void*) {
            duk_json_decode(ctx, -1);
            return 1;
        }

    public:
        static_assert(sizeof(char) == 1, "Serialization probably broke");
        static duk_value deserialize(duk_context* ctx, char const* data, size_t data_len);

        // Important limitations:
        // - The returned value is binary and will not behave well if you treat it like a string (it will almost certainly contain '\0').
        //   If you need to transport it like a string, maybe try encoding it as base64.
        // - Strings longer than 2^32 (UINT32_MAX) characters will throw an exception. You can raise this to be a uint64_t if you need
        //   really long strings for some reason (be sure to change duk_value::deserialize() as well).
        // - Objects are encoded to JSON and then sent like a string. If your object can't be encoded as JSON (i.e. it's a function),
        //   this will not work. This can be done, but I chose not to because it poses a security issue if you deserializing untrusted data.
        //   If you require this functionality, you'll have to add it yourself with using duk_dump_function(...).
        METADOT_NODISCARD std::vector<char> serialize() const;

        // same as above (copy_from_stack), but also removes the value we copied from the stack
        static duk_value take_from_stack(duk_context* ctx, duk_idx_t idx = -1) {
            duk_value val = copy_from_stack(ctx, idx);
            duk_remove(ctx, idx);
            return val;
        }

        // push the value we hold onto the stack
        inline void push() const;

        // various (type-safe) getters
        inline bool as_bool() const {
            if (mType != Type::Boolean) {
                throw duk_exception() << "Expected boolean, got " << type_name();
            }
            return mPOD.boolean;
        }

        inline double as_double() const {
            if (mType != Type::Number) {
                throw duk_exception() << "Expected number, got " << type_name();
            }
            return mPOD.number;
        }

        inline float as_float() const {
            if (mType != Type::Number) {
                throw duk_exception() << "Expected number, got " << type_name();
            }
            return static_cast<float>(mPOD.number);
        }

        inline duk_int_t as_int() const {
            if (mType != Type::Number) {
                throw duk_exception() << "Expected number, got " << type_name();
            }
            return static_cast<int32_t>(mPOD.number);
        }

        inline duk_uint_t as_uint() const {
            if (mType != Type::Number) {
                throw duk_exception() << "Expected number, got " << type_name();
            }
            return static_cast<uint32_t>(mPOD.number);
        }

        inline void* as_pointer() const {
            if (mType != Type::Pointer && mType != Type::NullRef) {
                throw duk_exception() << "Expected pointer or null, got " << type_name();
            }
            return mPOD.pointer;
        }

        inline uint8_t* as_plain_buffer(duk_size_t* size = nullptr, BufferType* bufferType = nullptr) const {
            if (mType != Type::Buffer) {
                throw duk_exception() << "Expected buffer, got " << type_name();
            }

            if (size != nullptr) {
                *size = mBufSize;
            }

            if (bufferType != nullptr) {
                *bufferType = mBufferType;
            }

            return mPOD.plain_buffer;
        }

        inline const std::string& as_string() const {
            if (mType != Type::String) {
                throw duk_exception() << "Expected string, got " << type_name();
            }
            return mString;
        }

        inline char const* as_c_string() const {
            if (mType != Type::String) {
                throw duk_exception() << "Expected string, got " << type_name();
            }
            return mString.data();
        }

        template<typename T = duk_value>
        std::vector<T> as_array() const;

        template<typename T = duk_value>
        std::map<std::string, T> as_map() const;

        inline Type type() const {
            return mType;
        }

        // same as duk_get_type_name(), but that's internal to Duktape, so we shouldn't use it
        inline char const* type_name() const noexcept;

        inline duk_context* context() const noexcept {
            return mContext;
        }

    private:
        // THIS IS COMPLETELY UNRELATED TO DETAIL_REFS.H.
        // detail_refs.h stores a mapping of native object -> script object.
        // This just stores arbitrary script objects (which likely have no native object backing them).
        // If I was smarter I might merge the two implementations, but this one is simpler
        // (since we don't need the std::map here).
        static void push_ref_array(duk_context* ctx) noexcept;

        // put a new reference into the ref array and return its index in the array
        static duk_uint_t stash_ref(duk_context* ctx, duk_idx_t idx) noexcept;

        // remove ref_array_idx from the ref array and add its spot to the free list (at refs[0])
        static void free_ref(duk_context* ctx, duk_uarridx_t ref_array_idx) noexcept;

        // this is for reference counting - used to release our reference based on the state
        // of mRefCount. If mRefCount is NULL, we never got copy constructed, so we have ownership
        // of our reference and can free it. If it's not null and above 1, we decrement the counter
        // (someone else owns the reference). If it's not null and equal to 1, we are the last owner
        // of a previously shared reference, so we can free it.
        void release_ref_count() noexcept;

        duk_context* mContext;
        Type mType;  // our type - one of the standard Duktape DUK_TYPE_* values

        // This holds the plain-old-data types. Since this is a variant,
        // we hold only one value at a time, so this is a union to save
        // a bit of space.
        union ValueTypes {
            bool boolean;
            double number;
            void* pointer;  // if mType == NULLREF, this is 0 (otherwise holds pointer value when mType == POINTER)
            uint8_t* plain_buffer;
            duk_uarridx_t ref_array_idx;
        } mPOD;

        duk_size_t mBufSize;
        BufferType mBufferType;

        std::string mString;  // if it's a string, we store it with std::string
        int* mRefCount;  // if mType == OBJECT and we're sharing, this will point to our ref counter
    };
}

namespace dukpp::detail {
    // This class handles keeping a map of void* -> script object.
    // It also prevents script objects from being GC'd until someone
    // explicitly frees the underlying native object.

    // Implemented by keeping an array of script objects in the heap stash.
    // An std::unordered_map maps pointer -> array index.
    // Thanks to std::unordered_map, lookup time is O(1) on average.

    // Using std::unordered_map has some memory overhead (~32 bytes per object),
    // which could be removed by using a different data structure:

    // 1. Use no data structure. Blindly scan through the reference registry,
    //    checking \xFFobj_ptr on every object until you find yours.
    //    Performance when returning native objects from functions when a lot
    //    of native objects are registered will suffer.

    // 2. Implement a self-balancing binary tree on top of a Duktape array
    //    for the registry. Still fast - O(log(N)) - and no memory overhead.

    // 3. A sorted list would work too, though insertion speed might be worse
    //    than a binary tree.

    struct RefManager {
    public:

        // Find the script object corresponding to obj_ptr and push it.
        // Returns true if successful, false if obj_ptr has not been registered.
        // Stack: ... -> ...              (if object has been registered before)
        //        ... -> ... [object]     (if object has not been registered)
        static bool find_and_push_native_object(duk_context* ctx, void* obj_ptr) {
            RefMap* ref_map = get_ref_map(ctx);

            auto const it = ref_map->find(obj_ptr);

            if (it == ref_map->end()) {
                return false;
            }
            else {
                push_ref_array(ctx);
                duk_get_prop_index(ctx, -1, it->second);
                duk_remove(ctx, -2);
                return true;
            }
        }

        // Takes a script object and adds it to the registry, associating
        // it with obj_ptr. unregistered_object is not modified.
        // If obj_ptr has already been registered with another object,
        // the old registry entry will be overidden.
        // Does nothing if obj_ptr is NULL.
        // Stack: ... [object]  ->  ... [object]
        static void register_native_object(duk_context* ctx, void* obj_ptr) {
            if (obj_ptr == nullptr) {
                return;
            }

            RefMap* ref_map = get_ref_map(ctx);

            push_ref_array(ctx);

            // find next free index
            // free indices are kept in a linked list, starting at ref_array[0]
            duk_get_prop_index(ctx, -1, 0);
            duk_uarridx_t next_free_idx = duk_get_uint(ctx, -1);
            duk_pop(ctx);

            if (next_free_idx == 0) {
                // no free spots in the array, make a new one at arr.length
                next_free_idx = (duk_uarridx_t)duk_get_length(ctx, -1);
            }
            else {
                // free spot found, need to remove it from the free list
                // ref_array[0] = ref_array[next_free_idx]
                duk_get_prop_index(ctx, -1, next_free_idx);
                duk_put_prop_index(ctx, -2, 0);
            }

            // std::cout << "putting reference at ref_array[" << next_free_idx << "]" << std::endl;
            (*ref_map)[obj_ptr] = next_free_idx;

            duk_dup(ctx, -2);  // put object on top

            // ... [object] [ref_array] [object]
            duk_put_prop_index(ctx, -2, next_free_idx);

            duk_pop(ctx);  // pop ref_array
        }

        // Remove the object associated with obj_ptr from the registry
        // and invalidate the object's internal native pointer (by setting it to undefined).
        // Does nothing if obj_ptr if object was never registered or obj_ptr is NULL.
        // Does not affect the stack.
        static void find_and_invalidate_native_object(duk_context* ctx, void* obj_ptr) noexcept {
            if (obj_ptr == nullptr) {
                return;
            }

            RefMap* ref_map = get_ref_map(ctx);
            auto it = ref_map->find(obj_ptr);
            if (it == ref_map->end()) { // was never registered
                return;
            }

            push_ref_array(ctx);
            duk_get_prop_index(ctx, -1, it->second);

            // invalidate internal pointer
            duk_push_undefined(ctx);
            duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("obj_ptr"));
            duk_pop(ctx);  // pop object

            // remove from references array and add the space it was in to free list
            // (refs[0] -> tail) -> (refs[0] -> old_obj_idx -> tail)

            // refs[old_obj_idx] = refs[0]
            duk_get_prop_index(ctx, -1, 0);
            duk_put_prop_index(ctx, -2, it->second);

            // refs[0] = old_obj_idx
            duk_push_uint(ctx, it->second);
            duk_put_prop_index(ctx, -2, 0);

            duk_pop(ctx);  // pop ref_array

            // also remove from map
            // std::cout << "Freeing ref_array[" << it->second << "]" << std::endl;
            ref_map->erase(it);
        }

    private:
        typedef std::unordered_map<void*, duk_uarridx_t> RefMap;

        static RefMap* get_ref_map(duk_context* ctx) {

            duk_push_heap_stash(ctx);

            if (!duk_has_prop_string(ctx, -1, "dukpp_ref_map")) {
                // doesn't exist yet, need to create it
                duk_push_object(ctx);

                duk_push_pointer(ctx, new RefMap());
                duk_put_prop_string(ctx, -2, "ptr");

                duk_push_c_function(ctx, ref_map_finalizer, 1);
                duk_set_finalizer(ctx, -2);

                duk_put_prop_string(ctx, -2, "dukpp_ref_map");
            }

            duk_get_prop_string(ctx, -1, "dukpp_ref_map");
            duk_get_prop_string(ctx, -1, "ptr");
            auto map = static_cast<RefMap*>(duk_require_pointer(ctx, -1));
            duk_pop_3(ctx);

            return map;
        }

        static duk_ret_t ref_map_finalizer(duk_context* ctx) {
            duk_get_prop_string(ctx, 0, "ptr");
            auto map = static_cast<RefMap*>(duk_require_pointer(ctx, -1));
            delete map;

            return 0;
        }

        static void push_ref_array(duk_context* ctx) {
            duk_push_heap_stash(ctx);

            if (!duk_has_prop_string(ctx, -1, "dukpp_ref_array")) {
                duk_push_array(ctx);

                // ref_array[0] = 0 (initialize free list as empty)
                duk_push_int(ctx, 0);
                duk_put_prop_index(ctx, -2, 0);

                duk_put_prop_string(ctx, -2, "dukpp_ref_array");
            }

            duk_get_prop_string(ctx, -1, "dukpp_ref_array");
            duk_remove(ctx, -2); // pop heap stash
        }
    };
}

namespace dukpp::detail {
    // same as duk_get_type_name, which is private for some reason *shakes fist*
    inline char const* get_type_name(duk_int_t type_idx) noexcept {
        static char const* names[] = {
                "none",
                "undefined",
                "null",
                "boolean",
                "number",
                "string",
                "object",
                "buffer",
                "pointer",
                "lightfunc"
        };

        if (type_idx >= 0 && type_idx < (duk_int_t)(sizeof(names) / sizeof(names[0]))) {
            return names[type_idx];
        }
        else {
            return "unknown";
        }
    }

    class TypeInfo {
    public:
        TypeInfo(std::type_index&& idx) noexcept : index_(idx), base_(nullptr) {}

        TypeInfo(TypeInfo const& rhs) noexcept : index_(rhs.index_), base_(rhs.base_) {}

        inline void set_base(TypeInfo* base) noexcept {
            base_ = base;
        }

        template<typename T>
        METADOT_NODISCARD bool can_cast() const noexcept {
            if (index_ == typeid(T)) {
                return true;
            }

            if (base_) {
                return base_->can_cast<T>();
            }

            return false;
        }

        inline bool operator<(TypeInfo const& rhs) const noexcept { return index_ < rhs.index_; }

        inline bool operator<=(TypeInfo const& rhs) const noexcept { return index_ <= rhs.index_; }

        inline bool operator>(TypeInfo const& rhs) const noexcept { return index_ > rhs.index_; }

        inline bool operator>=(TypeInfo const& rhs) const noexcept { return index_ >= rhs.index_; }

        inline bool operator==(TypeInfo const& rhs) const noexcept { return index_ == rhs.index_; }

        inline bool operator!=(TypeInfo const& rhs) const noexcept { return index_ != rhs.index_; }

    private:
        std::type_index index_;
        TypeInfo* base_;
    };
}

namespace dukpp::detail {
    struct ProtoManager {
    public:
        template<typename Cls>
        static void push_prototype(duk_context* ctx) {
            push_prototype(ctx, TypeInfo(typeid(Cls)));
        }

        static void push_prototype(duk_context* ctx, TypeInfo const& check_info) noexcept {
            if (!find_and_push_prototype(ctx, check_info)) {
                // nope, need to create our prototype object
                duk_push_object(ctx);

                // add reference to this class' info object so we can do type checking
                // when trying to pass this object into method calls
                typedef detail::TypeInfo TypeInfo;
                auto info = new TypeInfo(check_info);

                duk_push_pointer(ctx, info);
                duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("type_info"));

                // Clean up the TypeInfo object when this prototype is destroyed.
                // We can't put a finalizer directly on this prototype, because it
                // will be run whenever the wrapper for an object of this class is
                // destroyed; instead, we make a dummy object and put the finalizer
                // on that.
                // If you're memory paranoid: this duplicates the type_info pointer
                // once per registered class. If you don't care about freeing memory
                // during shutdown, you can probably comment out this part.
                duk_push_object(ctx);
                duk_push_pointer(ctx, info);
                duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("type_info"));
                duk_push_c_function(ctx, type_info_finalizer, 1);
                duk_set_finalizer(ctx, -2);
                duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("type_info_finalizer"));

                // register it in the stash
                register_prototype(ctx, info);
            }
        }

        template<typename Cls>
        static void make_script_object(duk_context* ctx, Cls* obj) noexcept {
            assert(obj != NULL);

            duk_push_object(ctx);
            duk_push_pointer(ctx, obj);
            duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("obj_ptr"));

            // push the appropriate prototype
#ifdef DUKPP_INFER_BASE_CLASS
            // In the "infer base class" case, we push the prototype
            // corresponding to the compile-time class if no prototype
            // for the run-time type has been defined. This allows us to
            // skip calling dukpp::_class::add_base_class() for every derived class,
            // so long as we:
            // (1) Always use the derived class as a pointer typed as the base class
            // (2) Do not create a prototype for the derived class
            //     (i.e. do not register any functions on the derived class).

            // For big projects with hundreds of derived classes, this is preferrable
            // to registering each type's base class individually. However,
            // registering a native method on a derived class will cause the
            // base class's methods to disappear until dukpp::_class::add_base_class() is
            // also called (because registering the native method causes a prototype
            // to be created for the run-time type). This behavior may be unexpected,
            // and for "small" projects it is reasonable to require
            // dukpp::_class::add_base_class() to be called, so it is opt-in via an ifdef.

            // does a prototype exist for the run-time type? if so, push it
            if (!find_and_push_prototype(ctx, TypeInfo(typeid(*obj)))) {
                // nope, find or create the prototype for the compile-time type
                // and push that
                push_prototype<Cls>(ctx);
            }
#else
            // always use the prototype for the run-time type
            push_prototype(ctx, TypeInfo(typeid(*obj)));
#endif
            duk_set_prototype(ctx, -2);
        }

    private:
        static duk_ret_t type_info_finalizer(duk_context* ctx) noexcept {
            duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("type_info"));
            auto info = static_cast<detail::TypeInfo*>(duk_require_pointer(ctx, -1));
            delete info;

            // set pointer to NULL in case this finalizer runs again
            duk_push_pointer(ctx, nullptr);
            duk_put_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("type_info"));

            return 0;
        }

        // puts heap_stash["dukpp_prototypes"] on the stack,
        // or creates it if it doesn't exist
        static void push_prototypes_array(duk_context* ctx) noexcept {
            duk_push_heap_stash(ctx);

            // does the prototype array already exist?
            if (!duk_has_prop_string(ctx, -1, "dukpp_prototypes")) {
                // nope, we need to create it
                duk_push_array(ctx);
                duk_put_prop_string(ctx, -2, "dukpp_prototypes");
            }

            duk_get_prop_string(ctx, -1, "dukpp_prototypes");

            // remove the heap stash from the stack
            duk_remove(ctx, -2);
        }

        // Stack: ... [proto]  ->  ... [proto]
        static void register_prototype(duk_context* ctx, TypeInfo const* info) noexcept {
            // 1. We assume info is not in the prototype array already
            // 2. Duktape has no efficient "shift array indices" operation (at least publicly)
            // 3. This method doesn't need to be fast, it's only called during registration

            // Work from high to low in the prototypes array, shifting as we go,
            // until we find the spot for info.

            push_prototypes_array(ctx);
            duk_size_t i = duk_get_length(ctx, -1);
            while (i > 0) {
                duk_get_prop_index(ctx, -1, i - 1);

                duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("type_info"));
                const TypeInfo* chk_info = static_cast<TypeInfo*>(duk_require_pointer(ctx, -1));
                duk_pop(ctx);  // pop type_info

                if (*chk_info > *info) {
                    duk_put_prop_index(ctx, -2, i);
                    i--;
                }
                else {
                    duk_pop(ctx);  // pop prototypes_array[i]
                    break;
                }
            }

            //std::cout << "Registering prototype for " << typeid(Cls).name() << " at " << i << std::endl;

            duk_dup(ctx, -2);  // copy proto to top
            duk_put_prop_index(ctx, -2, i);
            duk_pop(ctx);  // pop prototypes_array
        }

        static bool find_and_push_prototype(duk_context* ctx, TypeInfo const& search_info) noexcept {
            push_prototypes_array(ctx);

            // these are ints and not duk_size_t to deal with negative indices
            int min = 0;
            int max = (int)duk_get_length(ctx, -1) - 1;

            while (min <= max) {
                int mid = (max - min) / 2 + min;

                duk_get_prop_index(ctx, -1, mid);

                duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("type_info"));
                auto mid_info = static_cast<TypeInfo*>(duk_require_pointer(ctx, -1));
                duk_pop(ctx);  // pop type_info pointer

                if (*mid_info == search_info) {
                    // found it
                    duk_remove(ctx, -2);  // pop prototypes_array
                    return true;
                }
                else if (*mid_info < search_info) {
                    min = mid + 1;
                }
                else {
                    max = mid - 1;
                }

                duk_pop(ctx);  // pop prototypes_array[mid]
            }

            duk_pop(ctx);  // pop prototypes_array
            return false;
        }
    };
}


namespace dukpp::types {
    // Bare<T>::type is T stripped of reference, pointer, and const off a type, like so:
    // Bare<Dog>::type          = Dog
    // Bare<const Dog>::type    = Dog
    // Bare<Dog*>::type         = Dog
    // Bare<const Dog*>::type   = Dog
    // Bare<Dog&>::type         = Dog
    // Bare<const Dog&>::type   = Dog
    template<typename T>
    struct Bare {
        typedef typename std::remove_const<typename std::remove_pointer<typename std::remove_reference<T>::type>::type>::type type;
    };

    // DukType<T> provides functions for reading and writing T from the Duktape stack.
    // T is always a "bare type," i.e. "Dog" rather than "Dog*".

    // There are two kinds of DukTypes:
    //   1. "Native" DukTypes. This is the default.
    //      These types use an underlying native object allocated on the heap.
    //      A pointer to the object (of type T*) is expected at script_object.\xFFobj_ptr.
    //      "Native" DukTypes can return a value (returns a copy-constructed T from the native object),
    //      a pointer (just returns script_object.\xFFobj_ptr), or a reference (dereferences script_object.\xFFobj_ptr if it is not null).

    //   2. "Value" DukTypes. These are implemented through template specialization.
    //      This is how primitive types are implemented (int, float, char const *).
    //      These types can only be returned by value (T) or by const reference (const T&).
    //      Attempting to read a pointer (T*) or non-const reference (T&) will give a compile-time error.
    //      You can also use this to implement your own lightweight types, such as a 3D vector.
    //      (Strictly speaking, non-const references (T&) *could* be returned, but any changes to the reference would
    //      be discarded. So, I wrote a static assert to disable the option. If you understand the implications,
    //      you should be able to safely comment out the static_assert in ArgStorage.)
    template<typename T>
    struct DukType {
        static_assert(std::is_same<T, typename Bare<T>::type>::value, "Invalid base type, expected bare type");

        typedef std::false_type IsValueType;

        // read pointer
        template<typename FullT, typename = typename std::enable_if<std::is_pointer<FullT>::value>::type>
        static T* read(duk_context* ctx, duk_idx_t arg_idx) {

            if (duk_is_null(ctx, arg_idx)) {
                return nullptr;
            }

            if (!duk_is_object(ctx, arg_idx)) {
                duk_int_t type_idx = duk_get_type(ctx, arg_idx);
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: expected native object, got %s", arg_idx,
                    detail::get_type_name(type_idx));
            }

            duk_get_prop_string(ctx, arg_idx, DUK_HIDDEN_SYMBOL("type_info"));
            if (!duk_is_pointer(ctx, -1)) {  // missing type_info, must not be a native object
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: expected native object (missing type_info)", arg_idx);
            }

            // make sure this object can be safely returned as a T*
            auto info = static_cast<detail::TypeInfo*>(duk_get_pointer(ctx, -1));
            if (!info->can_cast<T>()) {
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: wrong type of native object", arg_idx);
            }

            duk_pop(ctx);  // pop type_info

            duk_get_prop_string(ctx, arg_idx, DUK_HIDDEN_SYMBOL("obj_ptr"));
            if (!duk_is_pointer(ctx, -1)) {
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: invalid native object.", arg_idx);
            }

            auto obj = static_cast<T*>(duk_get_pointer(ctx, -1));

            duk_pop(ctx);  // pop obj_ptr

            return obj;
        }

        // read reference
        template<typename FullT, typename = typename std::enable_if<std::is_reference<FullT>::value>::type>
        static T& read(duk_context* ctx, duk_idx_t arg_idx) {
            auto obj = read<T*>(ctx, arg_idx);
            if (obj == nullptr) {
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: cannot be null (native function expects reference)",
                    arg_idx);
            }

            return *obj;
        }

        // read value
        // commented out because it breaks for abstract classes
        /*template<typename FullT, typename = typename std::enable_if< std::is_same<T, typename std::remove_const<FullT>::type >::value>::type >
        static T read(duk_context* ctx, duk_idx_t arg_idx) {
            static_assert(std::is_copy_constructible<T>::value, "Reading a value requires a copy-constructable type");
            const T& obj = read<T&>(ctx, arg_idx);
            return T(obj);
        }*/

        // -----------------------------------------------------
        // Writing

        // Reference
        template<typename FullT, typename = typename std::enable_if<std::is_reference<FullT>::value>::type>
        static void push(duk_context* ctx, T& value) {

            if (!detail::RefManager::find_and_push_native_object(ctx, &value)) {
                // need to create new script object
                detail::ProtoManager::make_script_object<T>(ctx, &value);
                detail::RefManager::register_native_object(ctx, &value);
            }
        }

        // Pointer
        template<typename FullT, typename = typename std::enable_if<std::is_pointer<FullT>::value>::type>
        static void push(duk_context* ctx, T* value) {
            if (value == nullptr) {
                duk_push_null(ctx);
            }
            else {
                push<T&>(ctx, *value);
            }
        }

        // Value (create new instance on the heap)
        // commented out because this is an easy way to accidentally cause a memory leak
        /*template<typename FullT, typename = typename std::enable_if< std::is_same<T, typename std::remove_const<FullT>::type >::value>::type >
        static void push(duk_context* ctx, T value) {
            static_assert(std::is_copy_constructible<T>::value, "Cannot push value for non-copy-constructable type.");
            return push<T*>(ctx, new T(value));
        }*/
    };

    // Figure out what the type for an argument should be inside the tuple.
    // If a function expects a reference to a value type, we need temporary storage for the value.
    // For example, a reference to a value type (const int&) will need to be temporarily
    // stored in the tuple, so ArgStorage<const int&>::type == int.
    // Native objects are already allocated on the heap, so there's no problem storing, say, const Dog& in the tuple.
    template<typename T>
    struct ArgStorage {
    private:
        typedef typename Bare<T>::type BareType;
        //typedef DukType<BareType> ThisDukType;
        typedef typename DukType<BareType>::IsValueType IsValueType;

        static_assert(!IsValueType::value || !std::is_pointer<T>::value, "Cannot return pointer to value type.");
        static_assert(!IsValueType::value ||
            (!std::is_reference<T>::value ||
                std::is_const<typename std::remove_reference<T>::type>::value),
            "Value types can only be returned as const references.");

    public:
        typedef typename std::conditional<IsValueType::value, BareType, T>::type type;
    };
}


namespace dukpp::types {

#define DUKPP_SIMPLE_OPT_VALUE_TYPE(TYPE, DUK_IS_FUNC, DUK_GET_FUNC, DUK_PUSH_FUNC, PUSH_VALUE)      \
        template<>                                                                                   \
            struct DukType<std::optional<TYPE>> {                                                    \
                typedef std::true_type IsValueType;                                                  \
                                                                                                     \
                template<typename FullT>                                                             \
                static std::optional<TYPE> read(duk_context *ctx, duk_idx_t arg_idx) {               \
                    if (DUK_IS_FUNC(ctx, arg_idx) || duk_is_null(ctx, arg_idx)) {                    \
                        return static_cast<TYPE>(DUK_GET_FUNC(ctx, arg_idx));                        \
                    }                                                                                \
                    else {                                                                           \
                        return std::nullopt;                                                         \
                    }                                                                                \
                }                                                                                    \
                                                                                                     \
                template<typename FullT>                                                             \
                static void push(duk_context *ctx, std::optional<TYPE> const &value) {               \
                    if (value.has_value()) {                                                         \
                        DUK_PUSH_FUNC(ctx, PUSH_VALUE);                                              \
                    } else {                                                                         \
                        duk_push_null(ctx);                                                          \
                    }                                                                                \
                }                                                                                    \
            };
#define DUKPP_SIMPLE_VALUE_TYPE(TYPE, DUK_IS_FUNC, DUK_GET_FUNC, DUK_PUSH_FUNC, PUSH_VALUE, PUSH_OPT)       \
        template<>                                                                                          \
        struct DukType<TYPE> {                                                                              \
            typedef std::true_type IsValueType;                                                             \
                                                                                                            \
            template<typename FullT>                                                                        \
            static TYPE read(duk_context* ctx, duk_idx_t arg_idx) {                                         \
                if (DUK_IS_FUNC(ctx, arg_idx)) {                                                            \
                    return static_cast<TYPE>(DUK_GET_FUNC(ctx, arg_idx));                                   \
                } else {                                                                                    \
                    duk_int_t type_idx = duk_get_type(ctx, arg_idx);                                        \
                    duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: expected " #TYPE ", got %s", arg_idx,  \
                              detail::get_type_name(type_idx));                                             \
                    if (std::is_constructible<TYPE>::value) {                                               \
                        return TYPE();                                                                      \
                    }                                                                                       \
                }                                                                                           \
            }                                                                                               \
                                                                                                            \
            template<typename FullT>                                                                        \
            static void push(duk_context* ctx, TYPE value) {                                                \
                DUK_PUSH_FUNC(ctx, PUSH_VALUE);                                                             \
            }                                                                                               \
        };                                                                                                  \
    DUKPP_SIMPLE_OPT_VALUE_TYPE(TYPE, DUK_IS_FUNC, DUK_GET_FUNC, DUK_PUSH_FUNC, PUSH_OPT)

    DUKPP_SIMPLE_VALUE_TYPE(bool, duk_is_boolean, 0 != duk_get_boolean, duk_push_boolean, value, value.value())

        DUKPP_SIMPLE_VALUE_TYPE(uint8_t, duk_is_number, duk_get_uint, duk_push_uint, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(uint16_t, duk_is_number, duk_get_uint, duk_push_uint, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(uint32_t, duk_is_number, duk_get_uint, duk_push_uint, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(uint64_t, duk_is_number, duk_get_number, duk_push_number, value,
            value.value()) // have to cast to double

        DUKPP_SIMPLE_VALUE_TYPE(int8_t, duk_is_number, duk_get_int, duk_push_int, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(int16_t, duk_is_number, duk_get_int, duk_push_int, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(int32_t, duk_is_number, duk_get_int, duk_push_int, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(int64_t, duk_is_number, duk_get_number, duk_push_number, value,
            value.value()) // have to cast to double

#ifdef __APPLE__
        DUKPP_SIMPLE_VALUE_TYPE(size_t, duk_is_number, duk_get_number, duk_push_number, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(time_t, duk_is_number, duk_get_number, duk_push_number, value, value.value())
#elif defined(__arm__)
        DUKPP_SIMPLE_VALUE_TYPE(time_t, duk_is_number, duk_get_uint, duk_push_uint, value, value.value())
#endif

        // signed char and unsigned char are surprisingly *both* different from char, at least in MSVC
        DUKPP_SIMPLE_VALUE_TYPE(char, duk_is_number, duk_get_int, duk_push_int, value, value.value())

        DUKPP_SIMPLE_VALUE_TYPE(float, duk_is_number, duk_get_number, duk_push_number, value, value.value())
        DUKPP_SIMPLE_VALUE_TYPE(double, duk_is_number, duk_get_number, duk_push_number, value, value.value())

        DUKPP_SIMPLE_VALUE_TYPE(std::string, duk_is_string, duk_get_string, duk_push_string, value.c_str(),
            value.value().c_str())

#undef DUKPP_SIMPLE_VALUE_TYPE

        // We have to do some magic for char const * to work correctly.
        // We override the "bare type" and "storage type" to both be char const *.
        // char* is a bit tricky because its "bare type" should still be char const *, to differentiate it from just char
        template<>
    struct Bare<char*> {
        typedef char const* type;
    };
    template<>
    struct Bare<char const*> {
        typedef char const* type;
    };

    // the storage type should also be char const * - if we don't do this, it will end up as just "char"
    template<>
    struct ArgStorage<char const*> {
        typedef char const* type;
    };

    template<>
    struct DukType<char const*> {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static char const* read(duk_context* ctx, duk_idx_t arg_idx) {
            if (duk_is_string(ctx, arg_idx)) {
                return duk_get_string(ctx, arg_idx);
            }
            else {
                duk_int_t type_idx = duk_get_type(ctx, arg_idx);
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: expected string, got %s", arg_idx,
                    detail::get_type_name(type_idx));
                return nullptr;
            }
        }

        template<typename FullT>
        static void push(duk_context* ctx, char const* value) {
            duk_push_string(ctx, value);
        }
    };

    template<>
    struct DukType<std::optional<char const*>> {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static std::optional<char const*> read(duk_context* ctx, duk_idx_t arg_idx) {
            if (duk_is_string(ctx, arg_idx) || duk_is_null(ctx, arg_idx)) {
                return duk_get_string(ctx, arg_idx);
            }
            else {
                return std::nullopt;
            }
        }

        template<typename FullT>
        static void push(duk_context* ctx, std::optional<char const*> value) {
            if (value.has_value()) {
                duk_push_string(ctx, value.value());
            }
            else {
                duk_push_null(ctx);
            }
        }
    };

    // duk_value
    template<>
    struct DukType<duk_value> {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static duk_value read(duk_context* ctx, duk_idx_t arg_idx) {
            try {
                return duk_value::copy_from_stack(ctx, arg_idx);
            }
            catch (duk_exception& e) {
                // only duk_exception can be thrown by duk_value::copy_from_stack
                duk_error(ctx, DUK_ERR_ERROR, e.what());
                return duk_value();
            }
        }

        template<typename FullT>
        static void push(duk_context* ctx, duk_value const& value) {
            if (value.context() == nullptr) {
                duk_error(ctx, DUK_ERR_ERROR, "duk_value is uninitialized");
                return;
            }

            if (value.context() != ctx) {
                duk_error(ctx, DUK_ERR_ERROR, "duk_value comes from a different context");
                return;
            }

            try {
                value.push();
            }
            catch (duk_exception& e) {
                // only duk_exception can be thrown by duk_value::copy_from_stack
                duk_error(ctx, DUK_ERR_ERROR, e.what());
            }
        }
    };

    // std::vector (as value)
    // TODO - probably leaks memory if duktape is using longjmp and an error is encountered while reading an element
    template<typename T>
    struct DukType<std::vector<T> > {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static std::vector<T> read(duk_context* ctx, duk_idx_t arg_idx) {
            if (!duk_is_array(ctx, arg_idx)) {
                duk_int_t type_idx = duk_get_type(ctx, arg_idx);
                duk_error(ctx, DUK_ERR_TYPE_ERROR, "Argument %d: expected array, got %s", arg_idx,
                    detail::get_type_name(type_idx));
            }

            duk_size_t len = duk_get_length(ctx, arg_idx);
            const duk_idx_t elem_idx = duk_get_top(ctx);

            std::vector<T> vec;
            vec.reserve(len);
            for (duk_size_t i = 0; i < len; i++) {
                duk_get_prop_index(ctx, arg_idx, i);
                vec.push_back(DukType<typename Bare<T>::type>::template read<T>(ctx, elem_idx));
                duk_pop(ctx);
            }
            return vec;
        }

        template<typename FullT>
        static void push(duk_context* ctx, std::vector<T> const& value) {
            duk_idx_t obj_idx = duk_push_array(ctx);

            for (size_t i = 0; i < value.size(); i++) {
                DukType<typename Bare<T>::type>::template push<T>(ctx, value[i]);
                duk_put_prop_index(ctx, obj_idx, i);
            }
        }
    };

    // std::shared_ptr (as value)
    template<typename T>
    struct DukType<std::shared_ptr<T> > {
        typedef std::true_type IsValueType;

        static_assert(std::is_same<typename DukType<T>::IsValueType, std::false_type>::value,
            "Dukglue can only use std::shared_ptr to non-value types!");

        template<typename FullT>
        static std::shared_ptr<T> read(duk_context* ctx, duk_idx_t arg_idx) {
            if (duk_is_null(ctx, arg_idx)) {
                return nullptr;
            }

            if (!duk_is_object(ctx, arg_idx)) {
                duk_int_t type_idx = duk_get_type(ctx, arg_idx);
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: expected shared_ptr object, got ", arg_idx,
                    detail::get_type_name(type_idx));
            }

            duk_get_prop_string(ctx, arg_idx, DUK_HIDDEN_SYMBOL("type_info"));
            if (!duk_is_pointer(ctx, -1)) { // missing type_info, must not be a native object
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: expected shared_ptr object (missing type_info)",
                    arg_idx);
            }

            // make sure this object can be safely returned as a T*
            auto info = static_cast<detail::TypeInfo*>(duk_get_pointer(ctx, -1));
            if (!info->can_cast<T>()) {
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: wrong type of shared_ptr object", arg_idx);
            }
            duk_pop(ctx);  // pop type_info

            duk_get_prop_string(ctx, arg_idx, DUK_HIDDEN_SYMBOL("shared_ptr"));
            if (!duk_is_pointer(ctx, -1)) {
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Argument %d: not a shared_ptr object (missing shared_ptr)",
                    arg_idx);
            }
            void* ptr = duk_get_pointer(ctx, -1);
            duk_pop(ctx);  // pop pointer to shared_ptr

            return *((std::shared_ptr<T> *) ptr);
        }

        static duk_ret_t shared_ptr_finalizer(duk_context* ctx) {
            duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("shared_ptr"));
            auto ptr = (std::shared_ptr<T> *) duk_require_pointer(ctx, -1);
            duk_pop(ctx);  // pop shared_ptr ptr

            if (ptr != NULL) {
                delete ptr;

                // for safety, set the pointer to undefined
                // (finalizers can run multiple times)
                duk_push_undefined(ctx);
                duk_put_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("shared_ptr"));
            }

            return 0;
        }

        template<typename FullT>
        static void push(duk_context* ctx, std::shared_ptr<T> const& value) {
            detail::ProtoManager::make_script_object(ctx, value.get());

            // create + set shared_ptr
            duk_push_pointer(ctx, new std::shared_ptr<T>(value));
            duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("shared_ptr"));

            // set shared_ptr finalizer
            duk_push_c_function(ctx, &shared_ptr_finalizer, 1);
            duk_set_finalizer(ctx, -2);
        }
    };

    // std::map (as value)
    // TODO - probably leaks memory if duktape is using longjmp and an error is encountered while reading values
    template<typename T>
    struct DukType<std::map<std::string, T> > {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static std::map<std::string, T> read(duk_context* ctx, duk_idx_t arg_idx) {
            if (!duk_is_object(ctx, arg_idx)) {
                duk_error(ctx, DUK_ERR_TYPE_ERROR, "Argument %d: expected object.", arg_idx);
            }

            std::map<std::string, T> map;
            duk_enum(ctx, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
            while (duk_next(ctx, -1, 1)) {
                map[duk_safe_to_string(ctx, -2)] = DukType<typename Bare<T>::type>::template read<T>(ctx, -1);
                duk_pop_2(ctx);
            }
            duk_pop(ctx);  // pop enum object
            return map;
        }

        template<typename FullT>
        static void push(duk_context* ctx, std::map<std::string, T> const& value) {
            duk_idx_t obj_idx = duk_push_object(ctx);
            for (auto const& kv : value) {
                DukType<typename Bare<T>::type>::template push<T>(ctx, kv.second);
                duk_put_prop_lstring(ctx, obj_idx, kv.first.data(), kv.first.size());
            }
        }
    };

    template<>
    struct DukType<variadic_args> {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static variadic_args read(duk_context* ctx, duk_idx_t arg_idx) {
            duk_idx_t nargs = duk_get_top(ctx);

            variadic_args vargs;
            vargs.args.resize(nargs);
            for (duk_idx_t i = 0; i < nargs; i++) {
                vargs[nargs - 1 - i] = duk_value::copy_from_stack(ctx);
                duk_pop(ctx);
            }
            return vargs;
        }

        template<typename FullT>
        static void push(duk_context* ctx, variadic_args const& value) {
            for (auto const& v : value.args) {
                if (v.context() == nullptr) {
                    duk_error(ctx, DUK_ERR_ERROR, "duk_value is uninitialized");
                }

                if (v.context() != ctx) {
                    duk_error(ctx, DUK_ERR_ERROR, "duk_value comes from a different context");
                }

                try {
                    v.push();
                }
                catch (duk_exception& e) {
                    // only duk_exception can be thrown by duk_value::copy_from_stack
                    duk_error(ctx, DUK_ERR_ERROR, e.what());
                }
            }
        }
    };

    template<>
    struct DukType<this_context> {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static this_context read(duk_context* ctx, duk_idx_t arg_idx) {
            return ctx;
        }

        template<typename FullT>
        static void push(duk_context* ctx, this_context const& value) {
            // static_assert(false, "Invalid operation");
        }
    };

    // std::function
    /*template <typename RetT, typename... ArgTs>
    struct DukType< std::function<RetT(ArgTs...)> > {
        typedef std::true_type IsValueType;

        template<typename FullT>
        static std::function<RetT(ArgTs...)> read(duk_context* ctx, duk_idx_t arg_idx) {
            duk_value callable = duk_value::copy_from_stack(ctx, -1, DUK_TYPE_MASK_OBJECT);
            return [ctx, callable] (ArgTs... args) -> RetT {
                dukpp::call<RetT>(ctx, callable, args...);
            };
        }

        template<typename FullT>
        static void push(duk_context* ctx, std::function<RetT(ArgTs...)> value) {
            static_assert(false, "Pushing an std::function has not been implemented yet. Sorry!");
        }
    };*/
}



namespace dukpp {

    duk_value& duk_value::operator=(const duk_value& rhs) {
        if (this == &rhs) {
            throw duk_exception() << "Self-assignment is prohibited";
        }
        // free whatever we had
        release_ref_count();

        // copy things
        mContext = rhs.mContext;
        mType = rhs.mType;
        mPOD = rhs.mPOD;

        if (mType == Type::String) {
            mString = rhs.mString;
        }
        else if (mType == Type::Object) {
            // ref counting increment
            if (rhs.mRefCount == nullptr) {
                // not ref counted before, need to allocate memory
                const_cast<duk_value&>(rhs).mRefCount = new int(2);
                mRefCount = rhs.mRefCount;
            }
            else {
                // already refcounting, just increment
                mRefCount = rhs.mRefCount;
                *mRefCount = *mRefCount + 1;
            }
        }
        else if (mType == Type::Buffer) {
            mBufSize = rhs.mBufSize;
            mBufferType = rhs.mBufferType;
        }

        return *this;
    }

    bool duk_value::operator==(const duk_value& rhs) const {
        if (mType != rhs.mType || mContext != rhs.mContext) {
            return false;
        }

        switch (mType) {
        case Type::Undefined:
        case Type::NullRef:
            return true;
        case Type::Boolean:
            return mPOD.boolean == rhs.mPOD.boolean;
        case Type::Number:
            return mPOD.number == rhs.mPOD.number;
        case Type::String:
            return mString == rhs.mString;
        case Type::Object: {
            // todo: this could be optimized to only push ref_array once...
            this->push();
            rhs.push();
            bool equal = duk_equals(mContext, -1, -2) != 0;
            duk_pop_2(mContext);
            return equal;
        }
        case Type::Pointer:
            return mPOD.pointer == rhs.mPOD.pointer;
        case Type::Buffer:
        case Type::LightFunc:
        default:
            throw duk_exception() << "operator== not implemented (" << type_name() << ")";
        }
    }

    const char* duk_value::type_name() const noexcept {
        switch (mType) {
        case Type::Undefined:
            return "undefined";
        case Type::NullRef:
            return "null";
        case Type::Boolean:
            return "boolean";
        case Type::Number:
            return "number";
        case Type::String:
            return "string";
        case Type::Object:
            return "object";
        case Type::Buffer:
            return "buffer";
        case Type::Pointer:
            return "pointer";
        case Type::LightFunc:
            return "lightfunc";
        }
        return "?";
    }

    duk_value duk_value::copy_from_stack(duk_context* ctx, duk_idx_t idx) {
        duk_value value;
        value.mContext = ctx;
        value.mType = (Type)duk_get_type(ctx, idx);
        switch (value.mType) {
        case Type::Undefined:
            break;
        case Type::NullRef:
            value.mPOD.pointer = nullptr;
            break;
        case Type::Boolean:
            value.mPOD.boolean = duk_require_boolean(ctx, idx) != 0;
            break;
        case Type::Number:
            value.mPOD.number = duk_require_number(ctx, idx);
            break;
        case Type::String: {
            duk_size_t len;
            char const* data = duk_get_lstring(ctx, idx, &len);
            value.mString.assign(data, len);
            break;
        }
        case Type::Object:
            value.mPOD.ref_array_idx = stash_ref(ctx, idx);
            break;
        case Type::Pointer:
            value.mPOD.pointer = duk_require_pointer(ctx, idx);
            break;
        case Type::Buffer: {
            if (duk_is_fixed_buffer(ctx, idx)) {
                value.mBufferType = BufferType::Fixed;
            }
            else if (duk_is_dynamic_buffer(ctx, idx)) {
                value.mBufferType = BufferType::Dynamic;
            }
            else if (duk_is_external_buffer(ctx, idx)) {
                value.mBufferType = BufferType::External;
            }
            value.mPOD.plain_buffer = (uint8_t*)duk_require_buffer(ctx, idx, &value.mBufSize);
            break;
        }
        case Type::LightFunc:
        default:
            throw duk_exception() << "Cannot turn type into duk_value (" << value.type_name() << ")";
        }

        return value;
    }

    void duk_value::push() const {

        switch (mType) {
        case Type::Undefined:
            duk_push_undefined(mContext);
            break;
        case Type::NullRef:
            duk_push_null(mContext);
            break;
        case Type::Boolean:
            duk_push_boolean(mContext, mPOD.boolean);
            break;
        case Type::Number:
            duk_push_number(mContext, mPOD.number);
            break;
        case Type::String:
            duk_push_lstring(mContext, mString.data(), mString.size());
            break;
        case Type::Object:
            push_ref_array(mContext);
            duk_get_prop_index(mContext, -1, mPOD.ref_array_idx);
            duk_remove(mContext, -2);
            break;
        case Type::Pointer:
            duk_push_pointer(mContext, mPOD.pointer);
            break;
        case Type::Buffer: {
            // todo: Maybe deep copy would be better?
            duk_push_external_buffer(mContext);
            duk_config_buffer(mContext, -1, mPOD.plain_buffer, mBufSize); // map existing buffer to new pointer
            break;
        }
        case Type::LightFunc:
        default:
            throw duk_exception() << "duk_value.push() not implemented for type (" << type_name() << ")";
        }
    }

    template<typename T>
    std::vector<T> duk_value::as_array() const {
        std::vector<T> vec;

        if (mType != Type::Object) {
            throw duk_exception() << "Expected array, got " << type_name();
        }

        int prev_top = duk_get_top(mContext);

        push();

        if (!duk_is_array(mContext, -1)) {
            duk_pop_n(mContext, duk_get_top(mContext) - prev_top);
            throw duk_exception() << "Expected array, got " << type_name();
        }

        duk_size_t len = duk_get_length(mContext, -1);
        const duk_idx_t elem_idx = duk_get_top(mContext);

        vec.reserve(len);

        for (duk_size_t i = 0; i < len; i++) {
            duk_get_prop_index(mContext, -1, i);
            vec.push_back(types::DukType<typename types::Bare<T>::type>
                ::template read<T>(mContext, elem_idx));
            duk_pop(mContext);
        }

        duk_pop_n(mContext, duk_get_top(mContext) - prev_top);  // pop any results

        return vec;
    }

    template<typename T>
    std::map<std::string, T> duk_value::as_map() const {
        std::map<std::string, T> map;

        if (mType != Type::Object) {
            throw duk_exception() << "Expected array, got " << type_name();
        }

        int prev_top = duk_get_top(mContext);

        push();

        if (!duk_is_object(mContext, -1)) {
            duk_pop_n(mContext, duk_get_top(mContext) - prev_top);
            throw duk_exception() << "Expected array, got " << type_name();
        }

        duk_enum(mContext, -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
        while (duk_next(mContext, -1, 1)) {
            map[duk_safe_to_string(mContext, -2)] = types::DukType<typename types::Bare<T>::type>
                ::template read<T>(mContext, -1);
            duk_pop_2(mContext);
        }

        duk_pop_n(mContext, duk_get_top(mContext) - prev_top);  // pop any results
        return map;
    }

    duk_value duk_value::deserialize(duk_context* ctx, const char* data, size_t data_len) {
        duk_value v;
        v.mContext = ctx;
        v.mType = *((Type*)data);

        char const* data_ptr = data + sizeof(Type);
        data_len -= sizeof(Type);

        switch (v.mType) {
        case Type::Undefined:
        case Type::NullRef:
            break;

        case Type::Boolean: {
            if (data_len < 1) {
                throw duk_exception() << "Malformed boolean data";
            }

            v.mPOD.boolean = data[1] == 1;
            break;
        }

        case Type::Number: {
            if (data_len < sizeof(double)) {
                throw duk_exception() << "Malformed number data";
            }

            v.mPOD.number = *((double*)data_ptr);
            break;
        }

        case Type::String: {
            if (data_len < sizeof(uint32_t)) {
                throw duk_exception() << "Malformed string data (no length)";
            }
            uint32_t str_len = *((uint32_t*)data_ptr);

            if (data_len < sizeof(uint32_t) + str_len) {
                throw duk_exception() << "Malformed string data (appears truncated)";
            }

            char const* str_data = (data_ptr + sizeof(uint32_t));
            v.mString.assign(str_data, str_len);
            break;
        }

        case Type::Object: {
            if (data_len < sizeof(uint32_t)) {
                throw duk_exception() << "Malformed object JSON data (no length)";
            }
            uint32_t json_len = *((uint32_t*)data_ptr);

            if (data_len < sizeof(uint32_t) + json_len) {
                throw duk_exception() << "Malformed object JSON data (appears truncated)";
            }

            char const* json_data = (data_ptr + sizeof(uint32_t));
            duk_push_lstring(ctx, json_data, json_len);
            int rc = duk_safe_call(ctx, &json_decode_safe, nullptr, 1, 1);
            if (rc) {
                throw duk_error_exception(ctx, rc) << "Could not decode JSON";
            }
            else {
                v.mPOD.ref_array_idx = stash_ref(ctx, -1);
                duk_pop(ctx);
            }
            break;
        }

        default:
            throw duk_exception() << "not implemented";
        }

        return v;
    }

    std::vector<char> duk_value::serialize() const {
        std::vector<char> buff;
        buff.resize(sizeof(Type));
        *((Type*)buff.data()) = mType;

        switch (mType) {
        case Type::Undefined:
        case Type::NullRef:
            break;

        case Type::Boolean: {
            buff.push_back(mPOD.boolean ? 1 : 0);
            break;
        }

        case Type::Number: {
            buff.resize(buff.size() + sizeof(double));
            *((double*)(buff.data() + sizeof(Type))) = mPOD.number;
            break;
        }

        case Type::String: {
            if (mString.length() > static_cast<size_t>(UINT32_MAX)) {
                throw duk_exception() << "String length larger than uint32_t max";
            }

            auto len = (uint32_t)mString.length();
            buff.resize(buff.size() + sizeof(uint32_t) + len);

            auto len_ptr = (uint32_t*)(buff.data() + sizeof(Type));
            *len_ptr = len;

            auto out_ptr = (char*)(buff.data() + sizeof(Type) + sizeof(uint32_t));
            strncpy(out_ptr, mString.data(), len);  // note: this will NOT be null-terminated
            break;
        }

        case Type::Object: {
            push();
            if (duk_is_function(mContext, -1)) {
                duk_pop(mContext);
                throw duk_exception() << "Functions cannot be serialized";
                // well, technically they can...see the comments at the start of this method
            }

            std::string json = duk_json_encode(mContext, -1);
            duk_pop(mContext);

            if (json.length() > static_cast<size_t>(UINT32_MAX)) {
                throw duk_exception() << "JSON length larger than uint32_t max";
            }

            uint32_t len = (uint32_t)json.length();
            buff.resize(buff.size() + sizeof(uint32_t) + len);

            auto len_ptr = (uint32_t*)(buff.data() + sizeof(Type));
            *len_ptr = len;

            char* out_ptr = (char*)(buff.data() + sizeof(Type) + sizeof(uint32_t));
            strncpy(out_ptr, json.data(), len);  // note: this will NOT be null-terminated
            break;
        }

        default:
            throw duk_exception() << "Type not implemented for serialization.";
        }

        return buff;
    }

    void duk_value::push_ref_array(duk_context* ctx) noexcept {
        duk_push_heap_stash(ctx);

        if (!duk_has_prop_string(ctx, -1, "dukpp_duk_value_refs")) {
            duk_push_array(ctx);

            // ref_array[0] = 0 (initialize free list as empty)
            duk_push_int(ctx, 0);
            duk_put_prop_index(ctx, -2, 0);

            duk_put_prop_string(ctx, -2, "dukpp_duk_value_refs");
        }

        duk_get_prop_string(ctx, -1, "dukpp_duk_value_refs");
        duk_remove(ctx, -2); // pop heap stash
    }

    duk_uint_t duk_value::stash_ref(duk_context* ctx, duk_idx_t idx) noexcept {
        push_ref_array(ctx);

        // if idx is relative, we need to adjust it to deal with the array we just pushed
        if (idx < 0) {
            idx--;
        }

        // find next free index
        // free indices are kept in a linked list, starting at ref_array[0]
        duk_get_prop_index(ctx, -1, 0);
        duk_uarridx_t next_free_idx = duk_get_uint(ctx, -1);
        duk_pop(ctx);

        if (next_free_idx == 0) {
            // no free spots in the array, make a new one at arr.length
            next_free_idx = (duk_uarridx_t)duk_get_length(ctx, -1);
        }
        else {
            // free spot found, need to remove it from the free list
            // ref_array[0] = ref_array[next_free_idx]
            duk_get_prop_index(ctx, -1, next_free_idx);
            duk_put_prop_index(ctx, -2, 0);
        }

        duk_dup(ctx, idx);  // copy value we are storing (since store consumes it)
        duk_put_prop_index(ctx, -2, next_free_idx);  // store it (consumes duplicated value)
        duk_pop(ctx);  // pop ref array

        return next_free_idx;
    }

    void duk_value::free_ref(duk_context* ctx, duk_uarridx_t ref_array_idx) noexcept {
        push_ref_array(ctx);

        // add this spot to the free list
        // refs[old_obj_idx] = refs[0] (implicitly gives up our reference)
        duk_get_prop_index(ctx, -1, 0);
        duk_put_prop_index(ctx, -2, ref_array_idx);

        // refs[0] = old_obj_idx
        duk_push_uint(ctx, ref_array_idx);
        duk_put_prop_index(ctx, -2, 0);

        duk_pop(ctx);  // pop ref array
    }

    void duk_value::release_ref_count() noexcept {
        if (mType == Type::Object) {
            if (mRefCount != nullptr) {
                // sharing with another duk_value, are we the only one left?
                if (*mRefCount > 1) {  // still someone else referencing this
                    *mRefCount = *mRefCount - 1;
                }
                else {
                    // not sharing anymore, we can free it
                    free_ref(mContext, mPOD.ref_array_idx);
                    delete mRefCount;
                }

                mRefCount = nullptr;
            }
            else {
                // not sharing with any other duk_value, free it
                free_ref(mContext, mPOD.ref_array_idx);
            }
            mType = Type::Undefined;
        }
    }
}

namespace dukpp {
    // Prints duk_value to out stream
    std::ostream& operator<<(std::ostream& os, duk_value const& value) noexcept {
        switch (value.type()) {
        case duk_value::Type::Undefined:
        case duk_value::Type::NullRef:
            os << value.type_name();
            break;
        case duk_value::Type::Boolean: {
            std::ios state(nullptr);
            state.copyfmt(os);
            os << std::boolalpha << value.as_bool();
            os.copyfmt(state);
            break;
        }
        case duk_value::Type::Number:
            os << value.as_double();
            break;
        case duk_value::Type::String:
            os << value.as_string();
            break;
        case duk_value::Type::Pointer: {
            std::ios state(nullptr);
            state.copyfmt(os);
            os << "[pointer:" << std::hex << "0x" << value.as_pointer() << ']';
            os.copyfmt(state);
            break;
        }
        case duk_value::Type::Object: {
            auto ctx = value.context();
            int prev_top = duk_get_top(ctx);
            value.push();
            if (duk_is_array(ctx, -1)) {
                os << "[array: [";
                auto arr = value.as_array();
                for (auto it = arr.begin(); it != arr.end(); ++it) {
                    os << *it;
                    if (std::next(it) != arr.end()) {
                        os << ", ";
                    }
                }
                os << "]]";
            }
            else if (duk_is_function(ctx, -1)) {
                os << '[' << "function" << ": " << duk_safe_to_string(ctx, -1) << "]";
            }
            else if (duk_is_object(ctx, -1)) {
                os << "[object: {";
                auto map = value.as_map();
                for (auto it = map.begin(); it != map.end(); ++it) {
                    os << it->first << ": " << it->second;
                    if (std::next(it) != map.end()) {
                        os << ", ";
                    }
                }
                os << "}]";
            }
            else {
                os << '[' << value.type_name() << ": not implemented]";
            }
            duk_pop_n(ctx, duk_get_top(ctx) - prev_top);  // pop any results
            break;
        }
        case duk_value::Type::Buffer: {
            duk_size_t buf_size;
            duk_value::BufferType buf_type;
            auto buf = value.as_plain_buffer(&buf_size, &buf_type);
            std::stringstream ss;
            os << "[plain ";
            switch (buf_type) {
            case duk_value::BufferType::Fixed:
                os << "fixed";
                break;
            case duk_value::BufferType::Dynamic:
                os << "dynamic";
                break;
            case duk_value::BufferType::External:
                os << "external";
                break;
            case duk_value::BufferType::Undefined:
                os << "undefined";
                break;
            }
            os << " buffer: ";
            for (duk_size_t i = 0; i < buf_size; ++i) {
                ss << "0x" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (uint32_t)buf[i];
                if (i + 1 != buf_size) {
                    ss << ", ";
                }
            }
            os << ss.str() << "]";
            break;
        }
        case duk_value::Type::LightFunc:
            os << '[' << value.type_name() << ": not implemented]";
        }
        return os;
    }
}


namespace dukpp::detail {
    template<typename T, typename... Ts>
    struct contains : std::disjunction<std::is_same<T, Ts>...> {
    };

    template<typename... Ts>
    struct last_type {
        template<typename T>
        struct tag {
            using type = T;
        };
        // Use a fold-expression to fold the comma operator over the parameter pack.
        using type = typename decltype((tag<Ts>{}, ...))::type;
    };

    template<typename T, typename...>
    struct first_type {
        using type = T;
    };

    template<size_t i, typename... Args>
    struct nth_type {
        using type = typename std::tuple_element<i, std::tuple<Args...> >::type;
    };

    template<typename...Ts>
    using tuple_cat_t = decltype(std::tuple_cat(std::declval<Ts>()...));

    template<typename T, typename...Ts>
    using remove_t = tuple_cat_t<
        typename std::conditional<
        std::is_same<T, Ts>::value,
        std::tuple<>,
        std::tuple<Ts>>::type...>;

    //////////////////////////////////////////////////////////////////////////////////////////////

    // Credit to LuaState for this code:
    // https://github.com/AdUki/LuaState/blob/master/include/Traits.h

    template<size_t...>
    struct index_tuple {
    };

    template<size_t I, typename IndexTuple, typename ... Types>
    struct make_indexes_impl;

    template<size_t I, size_t... Indexes, typename T, typename ... Types>
    struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...> {
        typedef typename make_indexes_impl<I + 1, index_tuple<Indexes..., I>, Types...>::type type;
    };

    template<size_t I, size_t... Indexes>
    struct make_indexes_impl<I, index_tuple<Indexes...> > {
        typedef index_tuple<Indexes...> type;
    };

    template<typename ... Types>
    struct make_indexes : make_indexes_impl<0, index_tuple<>, Types...> {
    };

    //////////////////////////////////////////////////////////////////////////////////////////////

    template<std::size_t... Is>
    struct indexes {
    };

    template<std::size_t N, std::size_t... Is>
    struct indexes_builder : indexes_builder<N - 1, N - 1, Is...> {
    };

    template<std::size_t... Is>
    struct indexes_builder<0, Is...> {
        typedef indexes<Is...> index;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////

    // This mess is used to use function arugments stored in an std::tuple to an
    // std::function, function pointer, or method.

    // std::function
    template<class Ret, class... Args, size_t... Indexes>
    Ret apply_helper(std::function<Ret(Args...)> pf, index_tuple<Indexes...>, std::tuple<Args...>&& tup) noexcept {
        return pf(std::forward<Args>(std::get<Indexes>(tup))...);
    }

    template<class Ret, class ... Args>
    Ret apply(std::function<Ret(Args...)> pf, const std::tuple<Args...>& tup) noexcept {
        return apply_helper(pf, typename make_indexes<Args...>::type(), std::tuple<Args...>(tup));
    }

    // function pointer
    template<class Ret, class... Args, class... BakedArgs, size_t... Indexes>
    Ret apply_fp_helper(Ret(*pf)(Args...), index_tuple<Indexes...>, std::tuple<BakedArgs...>&& tup) noexcept {
        return pf(std::forward<Args>(std::get<Indexes>(tup))...);
    }

    template<class Ret, class... Args, class... BakedArgs, size_t... Indexes>
    Ret apply_fp_helper_ctx(duk_context* ctx, Ret(*pf)(Args...), index_tuple<Indexes...>,
        std::tuple<BakedArgs...>&& tup) noexcept {
        return reinterpret_cast<Ret(*)(duk_context*, Args...)>(pf)(ctx, std::forward<Args>(std::get<Indexes>(tup))...);
    }

    template<class Ret, class ... Args, class ... BakedArgs>
    Ret apply_fp(Ret(*pf)(Args...), const std::tuple<BakedArgs...>& tup) noexcept {
        return apply_fp_helper(pf, typename make_indexes<BakedArgs...>::type(), std::tuple<BakedArgs...>(tup));
    }

    template<class Ret, class ... Args, class ... BakedArgs>
    Ret apply_fp_ctx(duk_context* ctx, Ret(*pf)(Args...), const std::tuple<BakedArgs...>& tup) noexcept {
        return apply_fp_helper_ctx(ctx, pf, typename make_indexes<BakedArgs...>::type(), std::tuple<BakedArgs...>(tup));
    }

    // method pointer
    template<class Cls, class Ret, class... Args, class... BakedArgs, size_t... Indexes>
    Ret
        apply_method_helper(Ret(Cls::* pf)(Args...),
            index_tuple<Indexes...>, Cls* obj,
            std::tuple<BakedArgs...>&& tup) noexcept {
        return (*obj.*pf)(std::forward<Args>(std::get<Indexes>(tup))...);
    }

    template<class Cls, class Ret, class ... Args, class... BakedArgs>
    Ret apply_method(Ret(Cls::* pf)(Args...), Cls* obj, const std::tuple<BakedArgs...>& tup) noexcept {
        return apply_method_helper(pf, typename make_indexes<Args...>::type(), obj, std::tuple<BakedArgs...>(tup));
    }

    // const method pointer
    template<class Cls, class Ret, class... Args, class... BakedArgs, size_t... Indexes>
    Ret apply_method_helper(Ret(Cls::* pf)(Args...) const, index_tuple<Indexes...>, Cls* obj,
        std::tuple<BakedArgs...>&& tup) noexcept {
        return (*obj.*pf)(std::forward<Args>(std::get<Indexes>(tup))...);
    }

    template<class Cls, class Ret, class ... Args, class... BakedArgs>
    Ret apply_method(Ret(Cls::* pf)(Args...) const, Cls* obj, const std::tuple<BakedArgs...>& tup) noexcept {
        return apply_method_helper(pf, typename make_indexes<Args...>::type(), obj, std::tuple<BakedArgs...>(tup));
    }

    // constructor
    template<class Cls, typename... Args, size_t... Indexes>
    Cls* apply_constructor_helper(index_tuple<Indexes...>, std::tuple<Args...>&& tup) noexcept {
        return new Cls(std::forward<Args>(std::get<Indexes>(tup))...);
    }

    template<class Cls, typename... Args>
    Cls* apply_constructor(const std::tuple<Args...>& tup) noexcept {
        return apply_constructor_helper<Cls>(typename make_indexes<Args...>::type(), std::tuple<Args...>(tup));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
}

namespace dukpp::detail {
    // Helper to get the argument tuple type, with correct storage types.
    template<typename... Args>
    struct ArgsTuple {
        typedef std::tuple<typename types::ArgStorage<Args>::type...> type;
    };

    // Helper to get argument indices.
    // Call read for every Ts[i], for matching argument index Index[i].
    // The traits::index_tuple is used for type inference.
    // A concrete example:
    //   get_values<int, bool>(duktape_context)
    //     get_values_helper<{int, bool}, {0, 1}>(ctx, ignored)
    //       std::make_tuple<int, bool>(read<int>(ctx, 0), read<bool>(ctx, 1))
    template<typename... Args, size_t... Indexes>
    typename ArgsTuple<Args...>::type
        get_stack_values_helper(duk_context* ctx, detail::index_tuple<Indexes...>) {
        return std::forward_as_tuple(types::DukType<typename types::Bare<Args>::type>::template
            read<typename types::ArgStorage<Args>::type>(ctx, Indexes)...);
    }

    // Returns an std::tuple of the values asked for in the template parameters.
    // Values will remain on the stack.
    // Values are indexed from the bottom of the stack up (0, 1, ...).
    // If a value does not exist or does not have the expected type, an error is thrown
    // through Duktape (with duk_error(...)), and the function does not return
    template<typename... Args>
    typename ArgsTuple<Args...>::type get_stack_values(duk_context* ctx) {
        // We need the argument indices for read_value, and we need to be able
        // to unpack them as a template argument to match Ts.
        // So, we use traits::make_indexes<Ts...>, which returns a traits::index_tuple<0, 1, 2, ...> object.
        // We pass that into a helper function so we can put a name to that <0, 1, ...> template argument.
        // Here, the type of Args isn't important, the length of it is.
        auto indices = typename detail::make_indexes<Args...>::type();
        return get_stack_values_helper<Args...>(ctx, indices);
    }
}

//
// Created by Stanislav "Koncord" Zhukov on 17.01.2021.
//

namespace dukpp::detail {
    inline void object_invalidate(duk_context* ctx, void* obj_ptr) noexcept {
        detail::RefManager::find_and_invalidate_native_object(ctx, obj_ptr);
    }
}

namespace dukpp::detail {
    template<bool managed, typename Cls, typename... Ts>
    static duk_ret_t call_native_constructor(duk_context* ctx) {
        if (!duk_is_constructor_call(ctx)) {
            duk_error(ctx, DUK_RET_TYPE_ERROR, "Constructor must be called with new T().");
            return DUK_RET_TYPE_ERROR;
        }

        // construct the new instance
        auto constructor_args = detail::get_stack_values<Ts...>(ctx);
        Cls* obj = detail::apply_constructor<Cls>(std::move(constructor_args));

        duk_push_this(ctx);

        // make the new script object keep the pointer to the new object instance
        duk_push_pointer(ctx, obj);
        duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("obj_ptr"));

        // register it
        if (!managed) {
            detail::RefManager::register_native_object(ctx, obj);
        }

        duk_pop(ctx); // pop this

        return 0;
    }

    template<typename Cls>
    static duk_ret_t managed_finalizer(duk_context* ctx) noexcept {
        duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("obj_ptr"));
        Cls* obj = (Cls*)duk_require_pointer(ctx, -1);
        duk_pop(ctx);  // pop obj_ptr

        if (obj != NULL) {
            delete obj;

            // for safety, set the pointer to undefined
            duk_push_undefined(ctx);
            duk_put_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("obj_ptr"));
        }

        return 0;
    }

    template<typename Cls>
    static duk_ret_t call_native_deleter(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("obj_ptr"));

        if (!duk_is_pointer(ctx, -1)) {
            duk_error(ctx, DUK_RET_REFERENCE_ERROR, "Object has already been invalidated; cannot delete.");
            return DUK_RET_REFERENCE_ERROR;
        }

        Cls* obj = static_cast<Cls*>(duk_require_pointer(ctx, -1));
        object_invalidate(ctx, obj);
        delete obj;

        duk_pop_2(ctx);
        return 0;
    }
}

namespace dukpp::detail {
    template<bool isConst, class Cls, typename RetType, typename... Ts>
    struct MethodInfo {
        typedef typename std::conditional<isConst, RetType(Cls::*)(Ts...) const, RetType(Cls::*)(
            Ts...)>::type MethodType;

        // The size of a method pointer is not guaranteed to be the same size as a function pointer.
        // This means we can't just use duk_push_pointer(ctx, &MyClass::method) to store the method at run time.
        // To get around this, we wrap the method pointer in a MethodHolder (on the heap), and push a pointer to
        // that. The MethodHolder is cleaned up by the finalizer.
        struct MethodHolder {
            MethodType method;
        };

        struct MethodRuntime {
            static duk_ret_t finalize_method(duk_context* ctx) {
                // clean up the MethodHolder reference
                duk_get_prop_string(ctx, 0, DUK_HIDDEN_SYMBOL("method_holder"));

                void* method_holder_void = duk_require_pointer(ctx, -1);
                auto method_holder = static_cast<MethodHolder*>(method_holder_void);
                delete method_holder;

                return 0;
            }

            static duk_ret_t call_native_method(duk_context* ctx) {
                // get this.obj_ptr
                duk_push_this(ctx);
                duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("obj_ptr"));
                void* obj_void = duk_get_pointer(ctx, -1);
                if (obj_void == nullptr) {
                    duk_error(ctx, DUK_RET_REFERENCE_ERROR, "Invalid native object for 'this'");
                    return DUK_RET_REFERENCE_ERROR;
                }

                duk_pop_2(ctx); // pop this.obj_ptr and this

                // get current_function.method_info
                duk_push_current_function(ctx);
                duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("method_holder"));
                void* method_holder_void = duk_require_pointer(ctx, -1);
                if (method_holder_void == nullptr) {
                    duk_error(ctx, DUK_RET_TYPE_ERROR, "Method pointer missing?!");
                    return DUK_RET_TYPE_ERROR;
                }

                duk_pop_2(ctx);

                // (should always be valid unless someone is intentionally messing with this.obj_ptr...)
                Cls* obj = static_cast<Cls*>(obj_void);
                auto method_holder = static_cast<MethodHolder*>(method_holder_void);

                // read arguments and call method
                auto bakedArgs = detail::get_stack_values<Ts...>(ctx);
                actually_call(ctx, method_holder->method, obj, bakedArgs);
                return std::is_void<RetType>::value ? 0 : 1;
            }

            // this mess is to support functions with void return values
            template<typename Dummy = RetType, typename... BakedTs>
            static typename std::enable_if<!std::is_void<Dummy>::value>::type
                actually_call(duk_context* ctx, MethodType method, Cls* obj, std::tuple<BakedTs...> const& args) {
                // ArgStorage has some static_asserts in it that validate value types,
                // so we typedef it to force ArgStorage<RetType> to compile and run the asserts
                typedef typename types::ArgStorage<RetType>::type ValidateReturnType;

                RetType return_val = detail::apply_method<Cls, RetType, Ts...>(method, obj, args);

                types::DukType<typename types::Bare<RetType>::type>::template push<RetType>(ctx, std::move(return_val));
            }

            template<typename Dummy = RetType, typename... BakedTs>
            static typename std::enable_if<std::is_void<Dummy>::value>::type
                actually_call(duk_context* ctx, MethodType method, Cls* obj, std::tuple<BakedTs...> const& args) {
                detail::apply_method(method, obj, args);
            }
        };
    };

    template<bool isConst, typename Cls>
    struct MethodVariadicRuntime {
        typedef MethodInfo<isConst, Cls, duk_ret_t, duk_context*> MethodInfoVariadic;
        typedef typename MethodInfoVariadic::MethodHolder MethodHolderVariadic;

        static duk_ret_t finalize_method(duk_context* ctx) {
            return MethodInfoVariadic::MethodRuntime::finalize_method(ctx);
        }

        static duk_ret_t call_native_method(duk_context* ctx) {
            // get this.obj_ptr
            duk_push_this(ctx);
            duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("obj_ptr"));
            void* obj_void = duk_get_pointer(ctx, -1);
            if (obj_void == nullptr) {
                duk_error(ctx, DUK_RET_REFERENCE_ERROR, "Invalid native object for 'this'");
                return DUK_RET_REFERENCE_ERROR;
            }

            duk_pop_2(ctx);  // pop this.obj_ptr and this

            // get current_function.method_info
            duk_push_current_function(ctx);
            duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("method_holder"));
            void* method_holder_void = duk_require_pointer(ctx, -1);
            if (method_holder_void == nullptr) {
                duk_error(ctx, DUK_RET_TYPE_ERROR, "Method pointer missing?!");
                return DUK_RET_TYPE_ERROR;
            }

            duk_pop_2(ctx);

            // (should always be valid unless someone is intentionally messing with this.obj_ptr...)
            Cls* obj = static_cast<Cls*>(obj_void);
            auto method_holder = static_cast<MethodHolderVariadic*>(method_holder_void);

            return (*obj.*method_holder->method)(ctx);
        }
    };
}

namespace dukpp {

    template<typename Cls, bool isManaged = false>
    struct _class {
        duk_context* ctx;

        _class(duk_context* ctx, char const* objectName) : ctx(ctx) {
            register_ctor(objectName);
            add_delete();
        }

        // todo: allow this_context in ctor
        template<typename... Ts>
        void register_ctor(char const* name) {
            duk_c_function constructor_func = detail::call_native_constructor<isManaged, Cls, Ts...>;

            duk_push_c_function(ctx, constructor_func, sizeof...(Ts));

            if constexpr (isManaged) {
                duk_c_function finalizer_func = detail::managed_finalizer<Cls>;
                // create new prototype with finalizer
                duk_push_object(ctx);
                // set the finalizer
                duk_push_c_function(ctx, finalizer_func, 1);
                duk_set_finalizer(ctx, -2);
            }
            detail::ProtoManager::push_prototype<Cls>(ctx);

            if constexpr (isManaged) {
                // hook prototype with finalizer up to real class prototype
                // must use duk_set_prototype, not set the .prototype property
                duk_set_prototype(ctx, -2);
            }

            // set constructor_func.prototype
            duk_put_prop_string(ctx, -2, "prototype");

            // set name = constructor_func
            duk_put_global_string(ctx, name);
        }

        template<typename BaseCls>
        _class& add_base_class() {
            static_assert(!std::is_pointer<BaseCls>::value && !std::is_pointer<Cls>::value
                && !std::is_const<BaseCls>::value && !std::is_const<Cls>::value, "Use bare class names.");
            static_assert(std::is_base_of<BaseCls, Cls>::value, "Invalid class hierarchy!");

            // Cls.type_info->set_base(BaseCls.type_info)
            detail::ProtoManager::push_prototype<Cls>(ctx);
            duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("type_info"));
            auto derived_type_info = static_cast<detail::TypeInfo*>(duk_require_pointer(ctx, -1));
            duk_pop_2(ctx);

            detail::ProtoManager::push_prototype<BaseCls>(ctx);
            duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("type_info"));
            auto base_type_info = static_cast<detail::TypeInfo*>(duk_require_pointer(ctx, -1));
            duk_pop_2(ctx);

            derived_type_info->set_base(base_type_info);

            // also set up the prototype chain
            detail::ProtoManager::push_prototype<Cls>(ctx);
            detail::ProtoManager::push_prototype<BaseCls>(ctx);
            duk_set_prototype(ctx, -2);
            duk_pop(ctx);
        }

        template<typename RetType, typename... Ts>
        _class& add_method(char const* name, RetType(Cls::* method)(Ts...)) {
            return add_method<false, RetType, Ts...>(name, method);
        }

        template<typename RetType, typename... Ts>
        _class& add_method(char const* name, RetType(Cls::* method)(Ts...) const) {
            return add_method<true, RetType, Ts...>(name, method);
        }

        template<bool isConst, typename RetType, typename... Ts>
        _class& add_method(char const* name,
            typename std::conditional<isConst, RetType(Cls::*)(Ts...) const, RetType(Cls::*)(
                Ts...)>::type method) {
            typedef detail::MethodInfo<isConst, Cls, RetType, Ts...> MethodInfo;

            duk_c_function method_func = MethodInfo::MethodRuntime::call_native_method;

            detail::ProtoManager::push_prototype<Cls>(ctx);

            duk_idx_t nargs = sizeof...(Ts);

            if constexpr (detail::contains<variadic_args, Ts...>() ||
                detail::contains<variadic_args const&, Ts...>() ||
                detail::contains<variadic_args const&&, Ts...>()) {
                nargs = DUK_VARARGS;
            }

            duk_push_c_function(ctx, method_func, nargs);

            duk_push_pointer(ctx, new typename MethodInfo::MethodHolder{ method });
            duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("method_holder")); // consumes raw method pointer

            // make sure we free the method_holder when this function is removed
            duk_push_c_function(ctx, MethodInfo::MethodRuntime::finalize_method, 1);
            duk_set_finalizer(ctx, -2);

            duk_put_prop_string(ctx, -2, name); // consumes method function

            duk_pop(ctx); // pop prototype
            return *this;
        }

        // const getter, setter
        template<typename RetT, typename ArgT>
        _class& add_property(char const* name, RetT(Cls::* getter)() const, void(Cls::* setter)(ArgT)) {
            register_property<true, RetT, ArgT>(getter, setter, name);
            return *this;
        }

        // const getter, no-setter
        template<typename RetT>
        _class& add_property(char const* name, RetT(Cls::* getter)() const, std::nullptr_t setter) {
            register_property<true, RetT, RetT>(getter, setter, name);
            return *this;
        }

        // non-const getter, setter
        template<typename RetT, typename ArgT>
        _class& add_property(char const* name, RetT(Cls::* getter)(), void(Cls::* setter)(ArgT)) {
            register_property<false, RetT, ArgT>(getter, setter, name);
            return *this;
        }

        // non-const getter, no-setter
        template<typename RetT>
        _class& add_property(char const* name, RetT(Cls::* getter)(), std::nullptr_t setter) {
            register_property<false, RetT, RetT>(getter, setter, name);
        }

        // no getter, setter
        template<typename ArgT>
        _class& add_property(char const* name, std::nullptr_t getter, void(Cls::* setter)(ArgT)) {
            register_property<false, ArgT, ArgT>(getter, setter, name);
            return *this;
        }

        // no getter, no setter
        template<typename ValT>
        _class& add_property(char const* name, std::nullptr_t getter, std::nullptr_t setter) {
            static_assert(std::is_void<Cls>::value, "Must have getter or setter");
            return *this;
        }

        template<bool isConstGetter, typename RetT, typename ArgT>
        void
            register_property(typename std::conditional<isConstGetter, RetT(Cls::*)() const,
                RetT(Cls::*)()>::type getter,
                void(Cls::* setter)(ArgT),
                char const* name) {
            typedef detail::MethodInfo<isConstGetter, Cls, RetT> GetterMethodInfo;
            typedef detail::MethodInfo<false, Cls, void, ArgT> SetterMethodInfo;

            detail::ProtoManager::push_prototype<Cls>(ctx);

            // push key
            duk_push_string(ctx, name);

            // push getter
            if (getter != nullptr) {
                duk_c_function method_func = GetterMethodInfo::MethodRuntime::call_native_method;

                duk_push_c_function(ctx, method_func, 0);

                duk_push_pointer(ctx, new typename GetterMethodInfo::MethodHolder{ getter });
                duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("method_holder")); // consumes raw method pointer

                // make sure we free the method_holder when this function is removed
                duk_push_c_function(ctx, GetterMethodInfo::MethodRuntime::finalize_method, 1);
                duk_set_finalizer(ctx, -2);
            }
            else {
                duk_push_c_function(ctx, throw_error, 1);
            }

            if (setter != nullptr) {
                duk_c_function method_func = SetterMethodInfo::MethodRuntime::call_native_method;

                duk_push_c_function(ctx, method_func, 1);

                duk_push_pointer(ctx, new typename SetterMethodInfo::MethodHolder{ setter });
                duk_put_prop_string(ctx, -2, DUK_HIDDEN_SYMBOL("method_holder")); // consumes raw method pointer

                // make sure we free the method_holder when this function is removed
                duk_push_c_function(ctx, SetterMethodInfo::MethodRuntime::finalize_method, 1);
                duk_set_finalizer(ctx, -2);
            }
            else {
                duk_push_c_function(ctx, throw_error, 1);
            }

            duk_uint_t flags = DUK_DEFPROP_HAVE_GETTER
                | DUK_DEFPROP_HAVE_SETTER
                | DUK_DEFPROP_HAVE_CONFIGURABLE // set not configurable (from JS)
                | DUK_DEFPROP_FORCE; // allow overriding built-ins and previously defined properties

            duk_def_prop(ctx, -4, flags);
            duk_pop(ctx);  // pop prototype
        }

    private:
        void add_delete() {
            duk_c_function delete_func;
            if constexpr (isManaged) {
                // managed objects cannot be freed from native code safely
                static auto stub = [](duk_context*) -> duk_ret_t { return 0; };
                delete_func = stub;
            }
            else {
                delete_func = detail::call_native_deleter<Cls>;
            }

            detail::ProtoManager::push_prototype<Cls>(ctx);
            duk_push_c_function(ctx, delete_func, 0);
            duk_put_prop_string(ctx, -2, "delete");
            duk_pop(ctx);  // pop prototype
        }
    };

    template<typename Cls>
    using managed_class = _class<Cls, true>;

    template<typename Cls>
    using unmanaged_class = _class<Cls, false>;

    inline void object_invalidate(duk_context* ctx, void* obj_ptr) noexcept {
        detail::object_invalidate(ctx, obj_ptr);
    }
}

namespace dukpp::detail {
    // This struct can be used to generate a Duktape C function that
    // pulls the argument values off the stack (with type checking),
    // calls the appropriate function with them, and puts the function's
    // return value (if any) onto the stack.
    template<typename RetType, typename... Ts>
    struct FuncInfoHolder {
        typedef RetType(*FuncType)(Ts...);

        struct FuncRuntime {
            // Pull the address of the function to call from the
            // Duktape function object at run time.
            static duk_ret_t call_native_function(duk_context* ctx) {
                duk_push_current_function(ctx);
                duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("func_ptr"));
                void* fp_void = duk_require_pointer(ctx, -1);
                if (fp_void == nullptr) {
                    duk_error(ctx, DUK_RET_TYPE_ERROR, "what even");
                    return DUK_RET_TYPE_ERROR;
                }

                duk_pop_2(ctx);

                static_assert(sizeof(RetType(*)(Ts...)) == sizeof(void*),
                    "Function pointer and data pointer are different sizes");
                auto funcToCall = reinterpret_cast<RetType(*)(Ts...)>(fp_void);

                actually_call(ctx, funcToCall, detail::get_stack_values<Ts...>(ctx));
                return std::is_void<RetType>::value ? 0 : 1;
            }

            static duk_ret_t call_native_ctx_function(duk_context* ctx) {
                duk_push_current_function(ctx);
                duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("func_ptr"));
                void* fp_void = duk_require_pointer(ctx, -1);
                if (fp_void == nullptr) {
                    duk_error(ctx, DUK_RET_TYPE_ERROR, "what even");
                    return DUK_RET_TYPE_ERROR;
                }

                duk_pop_2(ctx);

                static_assert(sizeof(RetType(*)(Ts...)) == sizeof(void*),
                    "Function pointer and data pointer are different sizes");
                auto funcToCall = reinterpret_cast<RetType(*)(Ts...)>(fp_void);

                actually_call_ctx(ctx, funcToCall, detail::get_stack_values<Ts...>(ctx));
                return std::is_void<RetType>::value ? 0 : 1;
            }

            template<typename Dummy = RetType, typename... BakedTs>
            static typename std::enable_if<std::is_void<Dummy>::value>::type
                actually_call(duk_context*, RetType(*funcToCall)(Ts...), std::tuple<BakedTs...> const& args) {
                detail::apply_fp(funcToCall, args);
            }

            template<typename Dummy = RetType, typename... BakedTs>
            static typename std::enable_if<std::is_void<Dummy>::value>::type
                actually_call_ctx(duk_context* ctx, RetType(*funcToCall)(Ts...), std::tuple<BakedTs...> const& args) {
                detail::apply_fp_ctx(ctx, funcToCall, args);
            }

            template<typename Dummy = RetType, typename... BakedTs>
            static typename std::enable_if<!std::is_void<Dummy>::value>::type
                actually_call(duk_context* ctx, RetType(*funcToCall)(Ts...), std::tuple<BakedTs...> const& args) {
                // ArgStorage has some static_asserts in it that validate value types,
                // so we typedef it to force ArgStorage<RetType> to compile and run the asserts
                typedef typename types::ArgStorage<RetType>::type ValidateReturnType;

                RetType return_val = detail::apply_fp(funcToCall, args);

                types::DukType<typename types::Bare<RetType>::type>::template push<RetType>(ctx, std::move(return_val));
            }

            template<typename Dummy = RetType, typename... BakedTs>
            static typename std::enable_if<!std::is_void<Dummy>::value>::type
                actually_call_ctx(duk_context* ctx, RetType(*funcToCall)(Ts...), std::tuple<BakedTs...> const& args) {
                // ArgStorage has some static_asserts in it that validate value types,
                // so we typedef it to force ArgStorage<RetType> to compile and run the asserts
                typedef typename types::ArgStorage<RetType>::type ValidateReturnType;

                RetType return_val = detail::apply_fp_ctx(ctx, funcToCall, args);

                types::DukType<typename types::Bare<RetType>::type>::template push<RetType>(ctx, std::move(return_val));
            }
        };
    };
}

namespace dukpp::detail {
    template<typename RetT>
    void read(duk_context* ctx, duk_idx_t arg_idx, RetT* out) {
        // ArgStorage has some static_asserts in it that validate value types,
        // so we typedef it to force ArgStorage<RetType> to compile and run the asserts
        typedef typename types::ArgStorage<RetT>::type ValidateReturnType;

        *out = types::DukType<typename types::Bare<RetT>::type>::template read<RetT>(ctx, arg_idx);
    }

    template<typename RetT>
    struct SafeEvalData {
        char const* str;
        RetT* out;
    };

    template<typename RetT>
    duk_ret_t eval_safe(duk_context* ctx, void* udata) {
        auto data = (SafeEvalData<RetT> *) udata;

        duk_eval_string(ctx, data->str);
        read(ctx, -1, data->out);
        return 1;
    }
}

namespace dukpp {

    template<typename T>
    struct return_type : return_type<decltype(&T::operator())> {
    };
    // For generic types, directly use the result of the signature of its 'operator()'
    template<typename ClassType, typename ReturnType, typename... Args>
    struct return_type<ReturnType(ClassType::*)(Args...) const> {
        using type = ReturnType;
    };

    struct context_view {

        struct proxy {
            context_view* mCtx;
            std::string idx;

            proxy(context_view* ctx, std::string_view idx)
                : mCtx(ctx), idx(idx) {}

            // Register a function.

            /*template<typename T>
            auto operator=(T const &funcToCall) -> std::void_t<decltype(&T::operator())> {
                // TODO: need more smart logic to provide this_context
                //typedef typename boost::function_types::result_type<T>::type RetType;

                //std::cout << typeid(RetType).name() << std::endl;
                //std::cout << typeid(typename std::result_of_t<T&(...)>).name() << std::endl;
                //auto evalFunc = detail::FuncInfoHolder<RetType, Ts...>::FuncRuntime::call_native_function;
                //mCtx->actually_register_global_function<true>(idx.data(), evalFunc, funcToCall);
            }*/

            template<typename RetType, typename... Ts>
            proxy& operator=(RetType(*funcToCall)(Ts...)) {
                // TODO: need more smart logic to provide this_context
                auto evalFunc = detail::FuncInfoHolder<RetType, Ts...>::FuncRuntime::call_native_function;
                mCtx->actually_register_global_function<false>(idx.data(), evalFunc, funcToCall);
                return *this;
            }

            template<typename RetType, typename... Ts>
            proxy& operator=(RetType(*funcToCall)(this_context, Ts...)) {
                // TODO: need more smart logic to provide this_context
                auto evalFunc = detail::FuncInfoHolder<RetType, Ts...>::FuncRuntime::call_native_ctx_function;
                mCtx->actually_register_global_function<true>(idx.data(), evalFunc, funcToCall);
                return *this;
            }


            // register a global object
            // auto myDog = new Dog("Vector");
            // ctx["dog"] = myDog;
            template<typename T>
            proxy& operator=(T const& obj) {
                mCtx->push(obj);
                duk_put_global_string(mCtx->get_duk_context(), idx.data());
                return *this;
            }

            proxy operator[](std::string_view idx) {
                if (!duk_has_prop_string(mCtx->mCtx, -1, this->idx.c_str()))
                    duk_put_prop_string(mCtx->mCtx, -1, this->idx.c_str());
                else
                    duk_get_prop_string(mCtx->mCtx, -1, this->idx.c_str());
                return proxy(mCtx, idx);
            }
        };

        friend struct proxy;

        duk_context* mCtx;

        context_view(duk_context* ctx) noexcept : mCtx(ctx) {}

        context_view(this_context* ctx) noexcept : mCtx(ctx->mCtx) {}

        METADOT_NODISCARD duk_context* get_duk_context() const noexcept {
            return mCtx;
        }

        proxy operator[](std::string_view idx) {
            duk_push_global_object(mCtx);
            return proxy(this, idx);
        }

        template<typename RetT = void>
        typename std::enable_if<std::is_void<RetT>::value, RetT>::type eval(std::string_view const& code) const {
            int prev_top = duk_get_top(mCtx);
            duk_eval_string(mCtx, code.data());
            duk_pop_n(mCtx, duk_get_top(mCtx) - prev_top);  // pop any results
        }

        template<typename RetT>
        typename std::enable_if<!std::is_void<RetT>::value, RetT>::type eval(std::string_view const& code) const {
            int prev_top = duk_get_top(mCtx);
            duk_eval_string(mCtx, code.data());
            auto out = read<RetT>();
            duk_pop_n(mCtx, duk_get_top(mCtx) - prev_top);  // pop any results
            return out;
        }


        template<typename RetT = void>
        typename std::enable_if<std::is_void<RetT>::value, RetT>::type peval(std::string_view const& code) const {
            int prev_top = duk_get_top(mCtx);
            int rc = duk_peval_string(mCtx, code.data());
            if (rc != 0) {
                throw duk_error_exception(mCtx, rc);
            }

            duk_pop_n(mCtx, duk_get_top(mCtx) - prev_top);  // pop any results
        }

        template<typename RetT>
        typename std::enable_if<!std::is_void<RetT>::value, RetT>::type peval(std::string_view const& code) const {
            int prev_top = duk_get_top(mCtx);

            RetT ret;
            detail::SafeEvalData<RetT> data{
                    code.data(), &ret
            };

            int rc = duk_safe_call(mCtx, &detail::eval_safe<RetT>, (void*)&data, 0, 1);
            if (rc != 0) {
                throw duk_error_exception(mCtx, rc);
            }
            duk_pop_n(mCtx, duk_get_top(mCtx) - prev_top);  // pop any results
            return ret;
        }

        void eval_file(std::string const& filename) const {
            std::ifstream file(filename);
            file.seekg(0, std::ios::end);
            std::vector<char> code(file.tellg());
            file.seekg(0, std::ios::beg);
            file.read(&code[0], code.size());
            duk_eval_string(mCtx, code.data());
        }

        template<typename RetT = void, typename... Ts>
        typename std::enable_if<std::is_void<RetT>::value, RetT>::type call(std::string const& fn, Ts ...args) const {
            duk_get_global_string(mCtx, fn.data());
            push(args...);
            duk_call(mCtx, sizeof...(args));
        }

        template<typename RetT, typename... Ts>
        typename std::enable_if<!std::is_void<RetT>::value, RetT>::type call(std::string const& fn, Ts ...args) const {
            int prev_top = duk_get_top(mCtx);
            call(fn, args...);
            auto val = read<RetT>();
            int cur_top = duk_get_top(mCtx);
            duk_pop_n(mCtx, cur_top - prev_top);
            return val;
        }

        METADOT_NODISCARD duk_value copy_value_from_stack(duk_idx_t idx = -1) const {
            return duk_value::copy_from_stack(mCtx, idx);
        }

        METADOT_NODISCARD duk_value take_value_from_stack(duk_idx_t idx = -1) const {
            return duk_value::take_from_stack(mCtx, idx);
        }

        template<typename Cls>
        unmanaged_class<Cls> register_unmanaged_class(char const* name) const {
            return unmanaged_class<Cls>(mCtx, name);
        }

        template<typename Cls>
        managed_class<Cls> register_class(char const* name) const {
            return managed_class<Cls>(mCtx, name);
        }

        /**
         * WARNING: THIS IS NOT "PROTECTED." If an error occurs while reading (which is possible if you didn't
         *          explicitly check the type), the fatal Duktape error handler will be invoked, and the program
         *          will probably abort.
         */
        template<typename RetT>
        RetT read(duk_idx_t arg_idx = -1) const {
            // ArgStorage has some static_asserts in it that validate value types,
            // so we typedef it to force ArgStorage<RetType> to compile and run the asserts
            typedef typename types::ArgStorage<RetT>::type ValidateReturnType;

            return std::move(types::DukType<typename types::Bare<RetT>::type>::template read<RetT>(mCtx, arg_idx));
        }

        /**
         * @brief      Push a value onto the duktape stack.
         *
         * WARNING: THIS IS NOT "PROTECTED." If an error occurs when pushing (unlikely, but possible),
         *          the Duktape fatal error handler will be invoked (and the program will probably terminate).
         *
         * @param      ctx    duktape context
         * @param[in]  val    value to push
         */
        template<typename FullT>
        void push(FullT const& val) const {
            // ArgStorage has some static_asserts in it that validate value types,
            // so we typedef it to force ArgStorage<RetType> to compile and run the asserts
            typedef typename types::ArgStorage<FullT>::type ValidateReturnType;

            types::DukType<typename types::Bare<FullT>::type>::template push<FullT>(mCtx, std::move(val));
        }

        template<typename T, typename... ArgTs>
        void push(T const& arg, ArgTs... args) const {
            push(mCtx, arg);
            push(mCtx, args...);
        }

        inline void push() const {
            // no-op
        }

    private:
        template<bool isCtxFirstArg, typename RetType, typename... Ts>
        void actually_register_global_function(char const* name, duk_c_function const& evalFunc,
            RetType(*funcToCall)(Ts...)) const noexcept {
            duk_idx_t nargs = sizeof...(Ts);

            if constexpr (detail::contains<this_context, Ts...>()) {
                static_assert(std::is_same<this_context, typename detail::last_type<Ts...>::type>() || isCtxFirstArg,
                    "this_context must be first or last argument");
                nargs -= 1;
            }

            if constexpr (detail::contains<variadic_args, Ts...>() ||
                detail::contains<variadic_args const&, Ts...>() ||
                detail::contains<variadic_args const&&, Ts...>()) {
                nargs = DUK_VARARGS;
            }

            duk_push_c_function(mCtx, evalFunc, nargs);

            static_assert(sizeof(RetType(*)(Ts...)) == sizeof(void*),
                "Function pointer and data pointer are different sizes");
            duk_push_pointer(mCtx, reinterpret_cast<void*>(funcToCall));
            duk_put_prop_string(mCtx, -2, DUK_HIDDEN_SYMBOL("func_ptr"));

            duk_put_prop_string(mCtx, -2, name);
        }
    };

    static void fatal(void* udata, const char* msg)
    {
        METADOT_ERROR("JavaScript FATAL ERROR:\n  {}", (msg ? msg : "no message"));
    }

    struct context : context_view {
        context() noexcept : context_view(duk_create_heap(0, 0, 0, 0, fatal)) {}

        ~context() noexcept {
            duk_destroy_heap(mCtx);
        }
    };
}





