

#ifndef META_VALUEVISITOR_HPP
#define META_VALUEVISITOR_HPP

namespace Meta {

/**
 * \brief Base class for writing custom Value visitors
 *
 * A value visitor acts like compile-time dispatchers which automatically
 * calls the function which matches the actual type of the stored value.
 * This is a more direct and straight-forward approach than using a runtime switch,
 * based on value.kind() and then converting to the proper type.
 * It also gives access to enum and user objects, which can give useful informations with
 * no knowledge about the actual C++ class or enum.
 *
 * The template parameter T is the type returned by the visitor.
 *
 * To handle one of the possible types of the value, just write the corresponding
 * \c operator() function. Here is the list of the mapping between Meta types and their
 * corresponding C++ types:
 *
 * \li Meta::ValueKind::None --> Meta::NoType
 * \li Meta::ValueKind::Boolean --> bool
 * \li Meta::ValueKind::Integer --> long
 * \li Meta::ValueKind::Real --> double
 * \li Meta::ValueKind::String --> Meta::String
 * \li Meta::ValueKind::Enum --> Meta::EnumObject
 * \li Meta::ValueKind::User --> Meta::UserObject
 *
 * Here an example of a unary visitor which creates an editor for the value based on its type
 * \code
 * struct EditorFactory : public ValueVisitor<PropertyEditor*>
 * {
 *     PropertyEditor* operator()(bool value)
 *     {
 *         return new BooleanEditor(value);
 *     }
 *
 *     PropertyEditor* operator()(long value)
 *     {
 *         return new IntegerEditor(value);
 *     }
 *
 *     PropertyEditor* operator()(double value)
 *     {
 *         return new RealEditor(value);
 *     }
 *
 *     PropertyEditor* operator()(IdRef value)
 *     {
 *         return new StringEditor(value);
 *     }
 *
 *     PropertyEditor* operator()(const Meta::EnumObject& value)
 *     {
 *         return new EnumEditor(value);
 *     }
 *
 *     PropertyEditor* operator()(const Meta::UserObject& value)
 *     {
 *         return new UserEditor(value);
 *     }
 * };
 *
 * Meta::Value value(5.4);
 * PropertyEditor* editor = value.visit(EditorFactory());
 * \endcode
 */
template <typename T = void>
class ValueVisitor {
public:
    using result_type = T;  //!< Type of value visited.
};

}  // namespace Meta

#endif  // META_VALUEVISITOR_HPP
