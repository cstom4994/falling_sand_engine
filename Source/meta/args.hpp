

#ifndef META_ARGS_HPP
#define META_ARGS_HPP

#include <initializer_list>
#include <vector>

#include "meta/config.hpp"

namespace Meta {

class Value;

/**
 * \brief Wrapper for packing an arbitrary number of arguments into a single object
 *
 * Meta::Args is defined as a list of arguments of any type (wrapped in Meta::Value
 * instances), which can be passed to all the Meta entities which may need
 * an arbitrary number of arguments in a uniform way.
 *
 * Arguments lists can be constructed on the fly:
 *
 * \code
 * Meta::Args args(1, true, "hello", 5.24, &myObject);
 * \endcode
 *
 * or appended one by one using the + and += operators:
 *
 * \code
 * Meta::Args args;
 * args += 1;
 * args += true;
 * args += "hello";
 * args += 5.24;
 * args = args + myObject;
 * \endcode
 *
 */
class Args {
public:
    /**
     * \brief Construct the list with variable arguments.
     *
     * \param args Parameter pack to be used.
     */
    template <typename... V>
    Args(V&&... args) : Args(std::initializer_list<Value>({std::forward<V>(args)...})) {}

    /**
     * \brief Initialise the list with an initialisation list.
     *
     * \param il Arguments to put in the list.
     */
    Args(std::initializer_list<Value> il) { m_values = il; }

    /**
     * \brief Return the number of arguments contained in the list
     *
     * \return Size of the arguments list
     */
    size_t count() const;

    /**
     * \brief Overload of operator [] to access an argument from its index
     *
     * \param index Index of the argument to get
     * \return Value of the index-th argument
     * \throw OutOfRange index is out of range
     */
    const Value& operator[](size_t index) const;

    /**
     * \brief Overload of operator + to concatenate a list and a new argument
     *
     * \param arg Argument to concatenate to the list
     * \return New list
     */
    Args operator+(const Value& arg) const;

    /**
     * \brief Overload of operator += to append a new argument to the list
     *
     * \param arg Argument to append to the list
     * \return Reference to this
     */
    Args& operator+=(const Value& arg);

    /**
     * \brief Insert an argument into the list at a given index
     *
     * \param index Index at which to insert the argument
     * \param arg Argument to append to the list
     * \return Reference to this
     */
    Args& insert(size_t index, const Value& arg);

public:
    /**
     * \brief Special instance representing an empty set of arguments
     */
    static const Args empty;

private:
    std::vector<Value> m_values;  // List of the values
};

}  // namespace Meta

#endif  // META_ARGS_HPP
