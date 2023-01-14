

#ifndef META_DETAIL_ENUMMANAGER_HPP
#define META_DETAIL_ENUMMANAGER_HPP

#include <map>
#include <string>

#include "engine/meta/observernotifier.hpp"
#include "engine/meta/util.hpp"

namespace Meta {
class Enum;

namespace detail {
/**
 * \brief Manages creation, storage, retrieval and destruction of metaenums
 *
 * Meta::EnumManager is the place where all metaenums are stored and accessed.
 * It consists of a singleton which is created on first use and destroyed at global exit.
 *
 * \sa Enum
 */
class EnumManager : public ObserverNotifier {
    META__NON_COPYABLE(EnumManager);

public:
    /**
     * \brief Get the unique instance of the class
     *
     * \return Reference to the unique instance of EnumManager
     */
    static EnumManager& instance();

    /**
     * \brief Create and register a new metaenum
     *
     * This is the entry point for every metaenum creation. This
     * function also notifies registered observers after successful creations.
     *
     * \param id Identifier of the C++ enum bound to the metaenum (unique).
     *
     * \return Reference to the new metaenum
     */
    Enum& addClass(TypeId const& id, IdRef name);

    /**
     * \brief Unregister a metaenum
     *
     * This unregisters the metaenum so that it may not be used again.
     *
     * \param id Identifier of the C++ enum bound to the metaenum (unique).
     */
    void removeClass(TypeId const& id);

    /**
     * \brief Get the total number of metaenums
     *
     * \return Number of metaenums that have been registered
     */
    size_t count() const;

    /**
     * \brief Get a metaenum from a C++ type
     *
     * \param id Identifier of the C++ type
     *
     * \return Reference to the requested metaenum
     *
     * \throw EnumNotFound id is not the name of an existing metaenum
     */
    const Enum& getById(TypeId const& id) const;

    /**
     * \brief Get a metaenum from a C++ type
     *
     * This version returns a null pointer if no metaenum is found, instead
     * of throwing an exception.
     *
     * \param id Identifier of the C++ type
     *
     * \return Pointer to the requested metaenum, or null pointer if not found
     */
    const Enum* getByIdSafe(TypeId const& id) const;

    /**
     * \brief Get a metaenum by name
     *
     * \param name Name of the metaenum to retrieve
     *
     * \return Reference to the requested metaenum
     *
     * \throw EnumNotFound id is not the name of an existing metaenum
     */
    const Enum& getByName(const IdRef id) const;

    /**
     * \brief Get a metaenum by name
     *
     * This version returns a null pointer if no metaenum is found, instead
     * of throwing an exception.
     *
     * \param name Name of the metaenum to retrieve
     *
     * \return Pointer to the requested metaenum, or null pointer if not found
     */
    const Enum* getByNameSafe(const IdRef id) const;

    /**
     * \brief Check if a given type has a metaenum
     *
     * \param id Identifier of the C++ type
     *
     * \return True if the enum exists, false otherwise
     */
    bool enumExists(TypeId const& id) const;

private:
    /**
     * \brief Default constructor
     */
    EnumManager();

    /**
     * \brief Destructor
     *
     * The destructor destroys all the registered metaenums and notifies the observers.
     */
    ~EnumManager();

    typedef std::map<TypeId, Enum*> EnumTable;
    typedef std::map<Id, Enum*> NameTable;
    EnumTable m_enums;  // Table storing enums indexed by their TypeId
    NameTable m_names;  // Table storing enums indexed by their name
};

}  // namespace detail

}  // namespace Meta

#endif  // META_DETAIL_ENUMMANAGER_HPP
