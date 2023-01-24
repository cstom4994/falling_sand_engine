

#ifndef META_CONSTRUCTOR_HPP
#define META_CONSTRUCTOR_HPP

namespace Meta {

class Args;
class UserObject;

/**
 * \brief Represents a metaconstructor which is used to create objects instances from metaclasses
 *
 * This class is an interface which has to be derived to implement typed constructors.
 *
 * \sa Property, Function
 */
class Constructor : public Type {
public:
    /**
     * \brief Destructor
     */
    virtual ~Constructor() {}

    /**
     * \brief Check if the constructor matches the given set of arguments
     *
     * \param args Set of arguments to check
     *
     * \return True if the constructor is compatible with the given arguments
     */
    virtual bool matches(const Args& args) const = 0;

    /**
     * \brief Use the constructor to create a new object
     *
     * \param ptr If not null, use for placement new, otherwise heap allocate using new
     * \param args Set of arguments to pass to the constructor
     *
     * \return Pointer to the new object wrapped in a UserObject, or UserObject::nothing on failure
     */
    virtual UserObject create(void* ptr, const Args& args) const = 0;
};

}  // namespace Meta

#endif  // META_CONSTRUCTOR_HPP
