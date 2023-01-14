

#pragma once
#ifndef META_DETAIL_OBSERVERNOTIFIER_HPP
#define META_DETAIL_OBSERVERNOTIFIER_HPP

#include "engine/meta/config.hpp"
#include <set>

namespace Meta {
    
class Observer;
class Class;
class Enum;

namespace detail {
    
/**
 * \brief Base class for classes that can notify global observers
 */
class ObserverNotifier
{
public:

    /**
     * \brief Register a new observer
     *
     * The new observer will be notified of metaclasses and metaenums creation/removal.
     * This function doesn't take the ownership of the given object,
     * don't forget to destroy it on your side
     *
     * \param observer New observer to register
     */
    void addObserver(Observer* observer);

    /**
     * \brief Unregister an existing observer
     *
     * This function doesn't destroy the object, it just removes it
     * from the list of registered observers.
     *
     * \param observer Observer to unregister
     */
    void removeObserver(Observer* observer);

protected:

    /**
     * \brief Default constructor
     */
    ObserverNotifier();

    /**
     * \brief Notify all the registered observers of a class creation
     *
     * \param theClass Class that have been added
     */
    void notifyClassAdded(const Class& theClass);

    /**
     * \brief Notify all the registered observers of a class removal
     *
     * \param theClass Class that have been removed
     */
    void notifyClassRemoved(const Class& theClass);

    /**
     * \brief Notify all the registered observers of an enum creation
     *
     * \param theEnum Enum that have been added
     */
    void notifyEnumAdded(const Enum& theEnum);

    /**
     * \brief Notify all the registered observers of an enum removal
     *
     * \param theEnum Enum that have been removed
     */
    void notifyEnumRemoved(const Enum& theEnum);

private:

    typedef std::set<Observer*> ObserverSet;

    ObserverSet m_observers; ///< Sequence of registered observers
};

} // namespace detail
} // namespace Meta

#endif // META_DETAIL_OBSERVERNOTIFIER_HPP
