

#pragma once
#ifndef META_USERDATA_HPP
#define META_USERDATA_HPP

#include "engine/meta/config.hpp"
#include "engine/meta/value.hpp"

namespace Meta {

/**
 * \brief Name-value user data
 *
 * This is used to store user data for declared Types. The name is a string
 * and the Value can store any of the allowed types.
 *
 * \snippet userdata.cpp userdata_example
 */
class UserData
{
public:
    /** \brief Constructor
     *  \param name : Id string
     *  \param value : user Value
     */
    UserData(IdRef name, Value&& value) : m_name(name), m_value(value) {}
    
    /** \brief Get the UserData name.
     *  \return Id name
     */
    IdReturn getName() const { return m_name; }
    
    /** \brief Get the Value
     *  \return const Value
     */ 
    const Value& getValue() const { return m_value; }
    //Value& getValue() { return m_value; }

private:
    Id m_name;
    Value m_value;
};
    
/**
 * \brief Interface to UserData store.
 */ 
class IUserDataStore
{
public:
    virtual ~IUserDataStore() {}
    virtual void setValue(const Type& t, IdRef name, const Value& v) = 0;
    virtual const Value* getValue(const Type& t, IdRef name) const = 0;
    virtual void removeValue(const Type& t, IdRef name) = 0;
};

IUserDataStore* userDataStore();

} // namespace Meta

#endif // META_USERDATA_HPP
