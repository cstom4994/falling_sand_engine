

#ifndef META_ERRORS_HPP
#define META_ERRORS_HPP

#include "engine/meta/error.hpp"
#include "engine/meta/type.hpp"

namespace Meta {

class Class;

/**
 * \brief Error thrown when providing a metavalue whose type is incompatible with what's expected
 */
class BadType : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param provided Provided type
     * \param expected Expected type
     */
    BadType(ValueKind provided, ValueKind expected);

protected:
    /**
     * \brief Constructor for derived classes
     *
     * \param message Description of the error
     */
    BadType(const String& message);

    /**
     * \brief Get the string name of a Meta type
     *
     * \param type Type to convert
     *
     * \return Name of the provided type
     */
    static Meta::String typeName(ValueKind type);
};

/**
 * \brief Error thrown when providing a function argument which is incompatible with
 *        what the function expects
 */
class BadArgument : public BadType {
public:
    /**
     * \brief Constructor
     *
     * \param provided Provided type
     * \param expected Expected type
     * \param index Index of the argument in the function prototype
     * \param functionName Name of the function
     */
    BadArgument(ValueKind provided, ValueKind expected, size_t index, IdRef functionName);
};

/**
 * \brief Error thrown when a declaring a metaclass that already exists
 */
class ClassAlreadyCreated : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param idType Name of the class
     */
    ClassAlreadyCreated(IdRef idType);
};

/**
 * \brief Error thrown when a metaclass couldn't be found (either by its name or its id)
 */
class ClassNotFound : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param name Name of the requested class
     */
    ClassNotFound(IdRef name);
};

/**
 * \brief Error thrown when trying to convert an object to a class that is not a
 *        base nor a derived
 */
class ClassUnrelated : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param sourceClass Name of the source class
     * \param requestedClass Name of the requested class
     */
    ClassUnrelated(IdRef sourceClass, IdRef requestedClass);
};

/**
 * \brief Error thrown when a declaring a metaenum that already exists
 */
class EnumAlreadyCreated : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param typeName Name of the enum
     */
    EnumAlreadyCreated(IdRef typeName);
};

/**
 * \brief Error thrown when the value of a metaenum couldn't be found by its name
 */
class EnumNameNotFound : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param name Name of the requested metaenum member
     * \param enumName Name of the owner metaenum
     */
    EnumNameNotFound(IdRef name, IdRef enumName);
};

/**
 * \brief Error thrown when a metaenum couldn't be found (either by its name or its id)
 */
class EnumNotFound : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param name Name of the requested enum
     */
    EnumNotFound(IdRef name);
};

/**
 * \brief Error thrown when a value in a metaenum couldn't be found
 */
class EnumValueNotFound : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param value Value of the requested metaenum member
     * \param enumName Name of the owner metaenum
     */
    EnumValueNotFound(long value, IdRef enumName);
};

/**
 * \brief Error thrown when calling a function that is not callable
 */
class ForbiddenCall : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param functionName Name of the function
     */
    ForbiddenCall(IdRef functionName);
};

/**
 * \brief Error thrown when trying to read a property that is not readable
 */
class ForbiddenRead : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param propertyName Name of the property
     */
    ForbiddenRead(IdRef propertyName);
};

/**
 * \brief Error thrown when trying to write a function that is not writable
 */
class ForbiddenWrite : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param propertyName Name of the property
     */
    ForbiddenWrite(IdRef propertyName);
};

/**
 * \brief Error thrown when a function can't be found in a metaclass (by its name)
 */
class FunctionNotFound : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param name Name of the requested function
     * \param className Name of the owner metaclass
     */
    FunctionNotFound(IdRef name, IdRef className);
};

/**
 * \brief Error thrown when a declaring a metaclass that already exists
 */
class NotEnoughArguments : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param functionName Name of the function
     * \param provided Number of arguments provided
     * \param expected Number of arguments expected
     */
    NotEnoughArguments(IdRef functionName, size_t provided, size_t expected);
};

/**
 * \brief Error thrown when trying to use an empty metaobject
 */
class NullObject : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param objectClass Metaclass of the object (may be null if object has no class)
     */
    NullObject(const Class* objectClass);
};

/**
 * \brief Error thrown when using an index which is out of bounds
 */
class OutOfRange : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param index Invalid index
     * \param size Allowed size
     */
    OutOfRange(size_t index, size_t size);
};

/**
 * \brief Error thrown when a property can't be found in a metaclass (by its name)
 */
class PropertyNotFound : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param name Name of the requested property
     * \param className Name of the owner metaclass
     */
    PropertyNotFound(IdRef name, IdRef className);
};

/**
 * \brief Error thrown when cannot distinguish between multiple type instance
 */
class TypeAmbiguity : public Error {
public:
    /**
     * \brief Constructor
     *
     * \param typeName Name of the type causing problems
     */
    TypeAmbiguity(IdRef typeName);
};

}  // namespace Meta

#endif  // META_ERRORS_HPP
