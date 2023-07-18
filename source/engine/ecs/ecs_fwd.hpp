
#include <cstddef>
#include <type_traits>

namespace ME::ecs {
class entity;
class const_entity;

template <typename T>
class component;
template <typename T>
class const_component;

class prototype;

template <typename E>
class after;
template <typename E>
class before;

template <typename... Es>
class system;
class feature;
class registry;

template <typename T>
class exists;
template <typename... Ts>
class exists_any;
template <typename... Ts>
class exists_all;

template <typename T>
class option_neg;
template <typename... Ts>
class option_conj;
template <typename... Ts>
class option_disj;
class option_bool;

template <typename... Ts>
class aspect;

class entity_filler;
class registry_filler;

using family_id = std::uint16_t;
using entity_id = std::uint32_t;

constexpr std::size_t entity_id_index_bits = 22u;
constexpr std::size_t entity_id_version_bits = 10u;

static_assert(std::is_unsigned_v<family_id>, "ME::ecs (family_id must be an unsigned integer)");

static_assert(std::is_unsigned_v<entity_id>, "ME::ecs (entity_id must be an unsigned integer)");

static_assert(entity_id_index_bits > 0u && entity_id_version_bits > 0u && sizeof(entity_id) == (entity_id_index_bits + entity_id_version_bits) / 8u,
              "ME::ecs (invalid entity id index and version bits)");
}  // namespace ME::ecs