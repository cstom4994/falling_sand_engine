

#ifndef META_ERROR_HPP
#define META_ERROR_HPP

#include <exception>
#include <string>

#include "meta/config.hpp"
#include "meta/util.hpp"

namespace Meta {

/**
 * \brief Base class for every exception thrown in Meta.
 */
class Error : public std::exception {
public:
    /**
     * \brief Destructor
     */
    virtual ~Error() throw();

    /**
     * \brief Return a description of the error
     *
     * \return Pointer to a string containing the error message
     */
    virtual const char* what() const throw();

    /**
     * \brief Return the error location (file + line + function)
     *
     * \return String containing the error location
     */
    virtual const char* where() const throw();

    /**
     * \brief Prepare an error to be thrown
     *
     * This function is meant for internal use only. It adds
     * the current context of execution (file, line and function)
     * to the given error and returns it.
     *
     * \param error Error to prepare
     * \param file Source filename
     * \param line Line number in the source file
     * \param function Name of the function where the error was thrown
     *
     * \return Modified error, ready to be thrown
     */
    template <typename T>
    static T prepare(T error, const String& file, int line, const String& function);

protected:
    /**
     * \brief Default constructor
     *
     * \param message Error message to return in what()
     */
    Error(IdRef message);

    /**
     * \brief Helper function to convert anything to a string
     *
     * This is a convenience function provided to help derived
     * classes to easily build their full message
     *
     * \param x Value to convert
     *
     * \return \a x converted to a string
     */
    template <typename T>
    static Meta::String str(T x);

private:
    Meta::String m_message;   ///< Error message
    Meta::String m_location;  ///< Location of the error (file, line and function)
};

}  // namespace Meta

namespace Meta {

template <typename T>
T Error::prepare(T error, const String& file, int line, const String& function) {
    error.m_location = String(file) + " (" + str(line) + " ) - " + function;
    return error;
}

template <typename T>
Meta::String Error::str(T value) {
    return detail::convert<Meta::String>(value);
}

}  // namespace Meta

/**
 * \brief Trigger a Meta error
 */
#define META_ERROR(error) throw Meta::Error::prepare(error, __FILE__, __LINE__, __func__)

#endif  // META_ERROR_HPP
