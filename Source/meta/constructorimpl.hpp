

#ifndef META_DETAIL_CONSTRUCTORIMPL_HPP
#define META_DETAIL_CONSTRUCTORIMPL_HPP

#include "meta/constructor.hpp"
#include "meta/userobject.hpp"
#include "meta/value.hpp"
#include "meta/valuemapper.hpp"
#include "meta/valuevisitor.hpp"

namespace Meta {
namespace detail {
/**
 * \brief Helper function which converts an argument to a C++ type
 *
 * The main purpose of this function is to convert any BadType error to
 * a BadArgument one.
 *
 * \param args List of arguments
 * \param index Index of the argument to convert
 *
 * \return Value of args[index] converted to T
 *
 * \thrown BadArgument conversion triggered a BadType error
 */
template <typename T>
inline typename std::remove_reference<T>::type convertArg(const Args& args, size_t index) {
    try {
        return args[index].to<typename std::remove_reference<T>::type>();
    } catch (const BadType&) {
        META_ERROR(BadArgument(args[index].kind(), mapType<T>(), index, "constructor"));
    }
}

/**
 * \brief Check if a value is compatible with a C++ type
 *
 * This is a strong test, we check if the type of the value is directly mappable to T, not
 * just convertible.
 *
 * \param value Value to check
 *
 * \return True if the value is compatible with the type T
 */
template <typename T>
bool checkArg(const Value& value);

/**
 * \brief Implementation of metaconstructors with variable parameters
 */
template <typename T, typename... A>
class ConstructorImpl : public Constructor {
    template <typename... As, size_t... Is>
    static inline bool checkArgs(const Args& args, META__SEQNS::index_sequence<Is...>) {
        return allTrue(checkArg<As>(args[Is])...);
    }

    template <typename... As, size_t... Is>
    static inline UserObject createWithArgs(void* ptr, const Args& args, META__SEQNS::index_sequence<Is...>) {
        if (ptr)
            return UserObject::makeRef(new (ptr) T(convertArg<As>(args, Is)...));  // placement new
        else
            return UserObject::makeOwned(T(convertArg<As>(args, Is)...));
    }

public:
    /**
     * \see Constructor::matches
     */
    bool matches(const Args& args) const override { return args.count() == sizeof...(A) && checkArgs<A...>(args, META__SEQNS::make_index_sequence<sizeof...(A)>()); }

    /**
     * \see Constructor::create
     */
    UserObject create(void* ptr, const Args& args) const override { return createWithArgs<A...>(ptr, args, META__SEQNS::make_index_sequence<sizeof...(A)>()); }
};

/**
 * \brief Value visitor which checks the type of the visited value against the C++ type T
 */
template <typename T>
struct CheckTypeVisitor : public ValueVisitor<bool> {
    /**
     * \brief Common case: check mapping
     */
    template <typename U>
    bool operator()(const U&) {
        return mapType<T>() == mapType<U>();
    }

    /**
     * \brief Special case of enum objects: check metaenum and bound type
     */
    bool operator()(const EnumObject& obj) {
        const Enum* targetEnum = enumByTypeSafe<T>();
        return targetEnum && (*targetEnum == obj.getEnum());
    }

    /**
     * \brief Special case of user objects: check metaclass and bound type
     */
    bool operator()(const UserObject& obj) {
        const Class* targetClass = classByTypeSafe<T>();
        return targetClass && (*targetClass == obj.getClass());
    }
};

template <typename T>
bool checkArg(const Value& value) {
    return value.visit(CheckTypeVisitor<T>());
}

}  // namespace detail

}  // namespace Meta

#endif  // META_DETAIL_CONSTRUCTORIMPL_HPP
