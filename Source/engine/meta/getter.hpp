

#ifndef META_DETAIL_GETTER_HPP
#define META_DETAIL_GETTER_HPP

#include <functional>

#include "engine/meta/userobject.hpp"

namespace Meta {
namespace detail {

/**
 * \brief Represents an abstract getter
 *
 * This class is an interface which must be inherited by typed specializations.
 * The template parameter T is the type returned by the getter.
 *
 * \sa Getter, GetterImpl
 */
template <typename T>
class GetterInterface {
public:
    /**
     * \brief Destructor
     */
    virtual ~GetterInterface();

    /**
     * \brief Retrieve the value returned by the getter
     *
     * \param object User object to apply the getter to
     */
    virtual T get(const UserObject& object) const = 0;
};

/**
 * \brief Typed implementation of GetterInterface
 */
template <typename T, typename C>
class GetterImpl : public GetterInterface<T> {
public:
    /**
     * \brief Construct the getter implementation from a function
     */
    GetterImpl(std::function<T(C&)> function);

    /**
     * \see GetterInterface::get
     */
    T get(const UserObject& object) const override;

private:
    std::function<T(C&)> m_function;  ///< Function object storing the actual getter
};

/**
 * \brief Generic getter which can be applied to user objects
 *
 * This class models a Getter as a non-member function; this way it can be
 * used without knowledge about the actual owner class, and be applied
 * uniformly to user objects.
 *
 * Getter can be built upon any kind of callable type or a constant value.
 * The template parameter T is the type returned by the getter.
 *
 * \sa UserObject, GetterInterface
 */
template <typename T>
class Getter {
public:
    /**
     * \brief Construct the getter from a constant value
     *
     * This value will be returned either if no object is passed, or if no getter function has been defined
     *
     * \param defaultValue Value to return if no function has been set (default value of T by default)
     */
    Getter(const T& defaultValue = T());

    /**
     * \brief Construct the getter from a function
     *
     * \param function Function object storing the actual getter
     */
    template <typename C>
    Getter(std::function<T(C&)> function);

    /**
     * \brief Get the default value of the getter
     *
     * \return Constant stored in the getter
     */
    const T& get() const;

    /**
     * \brief Retrieve the value returned by the getter
     *
     * If no function has been set, it's equivalent to calling Getter::get().
     *
     * \param object User object to apply the getter to
     *
     * \return Value returned by the getter
     */
    T get(const UserObject& object) const;

private:
    std::shared_ptr<GetterInterface<T> > m_getter;  ///< Implementation of the getter
    T m_defaultValue;                               ///< Default value to return if no function or no object is specified
};

}  // namespace detail
}  // namespace Meta

namespace Meta {
namespace detail {

template <typename T>
GetterInterface<T>::~GetterInterface() {}

template <typename T, typename C>
GetterImpl<T, C>::GetterImpl(std::function<T(C&)> function) : m_function(function) {}

template <typename T, typename C>
T GetterImpl<T, C>::get(const UserObject& object) const {
    return m_function(object.get<C>());
}

template <typename T>
Getter<T>::Getter(const T& defaultValue) : m_getter(), m_defaultValue(defaultValue) {}

template <typename T>
template <typename C>
Getter<T>::Getter(std::function<T(C&)> function) : m_getter(new GetterImpl<T, C>(function)) {}

template <typename T>
const T& Getter<T>::get() const {
    return m_defaultValue;
}

template <typename T>
T Getter<T>::get(const UserObject& object) const {
    return m_getter ? m_getter->get(object) : m_defaultValue;
}

}  // namespace detail
}  // namespace Meta

#endif  // META_DETAIL_GETTER_HPP
