

#ifndef META_ENUMBUILDER_HPP
#define META_ENUMBUILDER_HPP

#include <string>

#include "engine/meta/config.hpp"

namespace Meta {

class Enum;

/**
 * \brief Proxy class which fills a metaenum with its members
 *
 * This class is returned by Enum::declare<T> in order construct a
 * new metaenum. It contains functions to declare <name, value> pairs to
 * fill the metaenum.
 *
 * This class should never be explicitely instantiated, unless you
 * need to split the metaenum creation in multiple parts.
 */
class EnumBuilder {
public:
    /**
     * \brief Construct the builder with a target metaenum
     *
     * \param target Target metaenum to construct
     */
    explicit EnumBuilder(Enum& target);

    /**
     * \brief Add a new pair to the metaenum
     *
     * \param name Name of the pair
     * \param value Value of the pair
     */
    EnumBuilder& value(IdRef name, long value);

    /**
     * \brief Add a new pair to the metaenum using enum class
     *
     * e.g. `value("one", MyEnum::One)`
     *
     * \param name Name of the pair
     * \param enumValue Value of the pair
     */
    template <typename E>
    EnumBuilder& value(IdRef name, E enumValue) {
        return value(name, static_cast<long>(enumValue));
    }

private:
    Enum* m_target;  // Target metaenum to construct
};

}  // namespace Meta

#endif  // META_ENUMBUILDER_HPP
