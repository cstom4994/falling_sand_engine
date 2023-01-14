

#pragma once
#ifndef META_FUNCTION_HPP
#define META_FUNCTION_HPP

#include "engine/meta/config.hpp"
#include "engine/meta/getter.hpp"
#include "engine/meta/args.hpp"
#include "engine/meta/type.hpp"
#include "engine/meta/value.hpp"
#include <string>
#include <vector>

namespace Meta {
    
class Args;
class UserObject;
class ClassVisitor;
    
/**
 * \brief Abstract representation of a function
 *
 * Functions are members of metaclasses. Their purpose is to provide detailed information
 * about their prototype.
 */
class Function : public Type
{
    META__NON_COPYABLE(Function);
public:

    /**
     * \brief Destructor
     */
    virtual ~Function();

    /**
     * \brief Get the name of the function
     *
     * \return Name of the function
     */
    IdReturn name() const;
    
   /**
    * \brief Get the kind of function represented here
    *
    * \return Kind of the function
    */
    FunctionKind kind() const { return m_funcType; }

    /**
     * \brief Get the type of variable returned by the function
     *
     * \return Type of the result of the function
     */
    ValueKind returnType() const;
    
    /**
     * \brief Get the kind of return policy this function uses
     *
     * \return Kind of return policy enum
     */
    policy::ReturnKind returnPolicy() const { return m_returnPolicy; }

    /**
     * \brief Get the number of parameters of the function
     *
     * \return Total number of parameters taken by the function
     */
    virtual size_t paramCount() const = 0;

    /**
     * \brief Get the type of an parameter given by its index
     *
     * \param index Index of the parameter
     *
     * \return Type of the index-th parameter
     *
     * \throw OutOfRange index is out of range
     */
    virtual ValueKind paramType(size_t index) const = 0;

    /**
     * \brief Accept the visitation of a ClassVisitor
     *
     * \param visitor Visitor to accept
     */
    virtual void accept(ClassVisitor& visitor) const;
    
   /**
    * \brief Get the per-function uses data (internal)
    *
    * \note This data is private to the uses module that created it.
    *
    * \return Opaque data pointer
    */
    const void* getUsesData() const {return m_usesData;}
    
protected:

    // FunctionImpl inherits from this and constructs.
    Function(IdRef name) : m_name(name) {}

    Id m_name;                          // Name of the function
    FunctionKind m_funcType;            // Kind of function
    ValueKind m_returnType;             // Runtime return type
    policy::ReturnKind m_returnPolicy;  // Return policy
    const void *m_usesData;
};
    
} // namespace Meta

#endif // META_FUNCTION_HPP
