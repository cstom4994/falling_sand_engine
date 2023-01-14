

#ifndef META_VALUE_REF_HPP
#define META_VALUE_REF_HPP

#include "engine/meta/config.hpp"
#include "engine/meta/type.hpp"

namespace Meta {
namespace detail {

class ValueRef {
public:
    template <typename T>
    static ValueRef make(T *p) {
        return ValueRef(p, Type<T>::info());
    }

    template <typename T>
    static ValueRef make(T &p) {
        return ValueRef(&p, std::type_index(typeid(T)));
    }

    ValueRef(const ValueRef &) = default;

    ValueRef &operator=(const ValueRef &) = delete;

    bool operator<(const ValueRef &other) const { return m_type->less(m_ptr, other.m_ptr); }

    bool operator==(const ValueRef &other) const { return m_type->equal(m_ptr, other.m_ptr); }

    template <typename T>
    T *getRef() {
        return static_cast<T *>(const_cast<void *>(m_ptr));
    }

    template <typename T>
    const T *getRef() const {
        return static_cast<const T *>(m_ptr);
    }

private:
    struct IType {
        virtual ~IType() {}
        virtual bool less(const void *a, const void *b) const = 0;
        virtual bool equal(const void *a, const void *b) const = 0;
    };

    template <typename T>
    struct Type final : public IType {
        static const IType *info() {
            static const Type<T> i;
            return &i;
        }

        bool less(const void *a, const void *b) const final { return *static_cast<const T *>(a) < *static_cast<const T *>(b); }
        bool equal(const void *a, const void *b) const final { return *static_cast<const T *>(a) == *static_cast<const T *>(b); }
    };

    ValueRef(void *p, const IType *t) : m_ptr(p), m_type(t) {}

    const void *const m_ptr;
    // std::type_index m_type;
    const IType *const m_type;
};

}  // namespace detail
}  // namespace Meta

#endif  // META_VALUE_REF_HPP
