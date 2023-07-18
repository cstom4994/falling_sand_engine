
#ifndef ME_ECS_HPP
#define ME_ECS_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/core/mathlib.hpp"
#include "ecs_impl.hpp"

namespace ME::ecs {

class entity final {
public:
    explicit entity(registry& owner) noexcept;
    entity(registry& owner, entity_id id) noexcept;

    entity(const entity&) = default;
    entity& operator=(const entity&) = default;

    entity(entity&&) noexcept = default;
    entity& operator=(entity&&) noexcept = default;

    registry& owner() noexcept;
    const registry& owner() const noexcept;

    entity_id id() const noexcept;

    entity clone() const;
    void destroy() noexcept;
    bool valid() const noexcept;

    template <typename T, typename... Args>
    T& assign_component(Args&&... args);

    template <typename T, typename... Args>
    T& ensure_component(Args&&... args);

    template <typename T>
    bool remove_component() noexcept;

    template <typename T>
    bool exists_component() const noexcept;

    std::size_t remove_all_components() noexcept;

    template <typename T>
    T& get_component();

    template <typename T>
    const T& get_component() const;

    template <typename T>
    T* find_component() noexcept;

    template <typename T>
    const T* find_component() const noexcept;

    template <typename... Ts>
    std::tuple<Ts&...> get_components();
    template <typename... Ts>
    std::tuple<const Ts&...> get_components() const;

    template <typename... Ts>
    std::tuple<Ts*...> find_components() noexcept;
    template <typename... Ts>
    std::tuple<const Ts*...> find_components() const noexcept;

    std::size_t component_count() const noexcept;

private:
    registry* owner_{nullptr};
    entity_id id_{0u};
};

bool operator<(const entity& l, const entity& r) noexcept;

bool operator==(const entity& l, const entity& r) noexcept;
bool operator==(const entity& l, const const_entity& r) noexcept;

bool operator!=(const entity& l, const entity& r) noexcept;
bool operator!=(const entity& l, const const_entity& r) noexcept;
}  // namespace ME::ecs

namespace std {
template <>
struct hash<ME::ecs::entity> final {
    std::size_t operator()(const ME::ecs::entity& ent) const noexcept {
        return NewMaths::hash_combine(std::hash<const ME::ecs::registry*>()(&ent.owner()), std::hash<ME::ecs::entity_id>()(ent.id()));
    }
};
}  // namespace std

// -----------------------------------------------------------------------------
//
// const_entity
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
class const_entity final {
public:
    const_entity(const entity& ent) noexcept;

    explicit const_entity(const registry& owner) noexcept;
    const_entity(const registry& owner, entity_id id) noexcept;

    const_entity(const const_entity&) = default;
    const_entity& operator=(const const_entity&) = default;

    const_entity(const_entity&&) noexcept = default;
    const_entity& operator=(const_entity&&) noexcept = default;

    const registry& owner() const noexcept;
    entity_id id() const noexcept;

    bool valid() const noexcept;

    template <typename T>
    bool exists_component() const noexcept;

    template <typename T>
    const T& get_component() const;

    template <typename T>
    const T* find_component() const noexcept;

    template <typename... Ts>
    std::tuple<const Ts&...> get_components() const;

    template <typename... Ts>
    std::tuple<const Ts*...> find_components() const noexcept;

    std::size_t component_count() const noexcept;

private:
    const registry* owner_{nullptr};
    entity_id id_{0u};
};

bool operator<(const const_entity& l, const const_entity& r) noexcept;

bool operator==(const const_entity& l, const entity& r) noexcept;
bool operator==(const const_entity& l, const const_entity& r) noexcept;

bool operator!=(const const_entity& l, const entity& r) noexcept;
bool operator!=(const const_entity& l, const const_entity& r) noexcept;
}  // namespace ME::ecs

namespace std {
template <>
struct hash<ME::ecs::const_entity> final {
    std::size_t operator()(const ME::ecs::const_entity& ent) const noexcept {
        return NewMaths::hash_combine(std::hash<const ME::ecs::registry*>()(&ent.owner()), std::hash<ME::ecs::entity_id>()(ent.id()));
    }
};
}  // namespace std

// -----------------------------------------------------------------------------
//
// component
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <typename T>
class component final {
public:
    explicit component(const entity& owner) noexcept;

    component(const component&) = default;
    component& operator=(const component&) = default;

    component(component&&) noexcept = default;
    component& operator=(component&&) noexcept = default;

    entity& owner() noexcept;
    const entity& owner() const noexcept;

    bool valid() const noexcept;
    bool exists() const noexcept;

    template <typename... Args>
    T& assign(Args&&... args);

    template <typename... Args>
    T& ensure(Args&&... args);

    bool remove() noexcept;

    T& get();
    const T& get() const;

    T* find() noexcept;
    const T* find() const noexcept;

    T& operator*();
    const T& operator*() const;

    T* operator->() noexcept;
    const T* operator->() const noexcept;

    explicit operator bool() const noexcept;

private:
    entity owner_;
};

template <typename T>
bool operator<(const component<T>& l, const component<T>& r) noexcept;

template <typename T>
bool operator==(const component<T>& l, const component<T>& r) noexcept;
template <typename T>
bool operator==(const component<T>& l, const const_component<T>& r) noexcept;

template <typename T>
bool operator!=(const component<T>& l, const component<T>& r) noexcept;
template <typename T>
bool operator!=(const component<T>& l, const const_component<T>& r) noexcept;
}  // namespace ME::ecs

namespace std {
template <typename T>
struct hash<ME::ecs::component<T>> {
    std::size_t operator()(const ME::ecs::component<T>& comp) const noexcept { return std::hash<ME::ecs::entity>()(comp.owner()); }
};
}  // namespace std

// -----------------------------------------------------------------------------
//
// const_component
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <typename T>
class const_component final {
public:
    const_component(const component<T>& comp) noexcept;
    explicit const_component(const const_entity& owner) noexcept;

    const_component(const const_component&) = default;
    const_component& operator=(const const_component&) = default;

    const_component(const_component&&) noexcept = default;
    const_component& operator=(const_component&&) noexcept = default;

    const const_entity& owner() const noexcept;

    bool valid() const noexcept;
    bool exists() const noexcept;

    const T& get() const;
    const T* find() const noexcept;

    const T& operator*() const;
    const T* operator->() const noexcept;
    explicit operator bool() const noexcept;

private:
    const_entity owner_;
};

template <typename T>
bool operator<(const const_component<T>& l, const const_component<T>& r) noexcept;

template <typename T>
bool operator==(const const_component<T>& l, const component<T>& r) noexcept;
template <typename T>
bool operator==(const const_component<T>& l, const const_component<T>& r) noexcept;

template <typename T>
bool operator!=(const const_component<T>& l, const component<T>& r) noexcept;
template <typename T>
bool operator!=(const const_component<T>& l, const const_component<T>& r) noexcept;
}  // namespace ME::ecs

namespace std {
template <typename T>
struct hash<ME::ecs::const_component<T>> {
    std::size_t operator()(const ME::ecs::const_component<T>& comp) const noexcept { return std::hash<ME::ecs::const_entity>()(comp.owner()); }
};
}  // namespace std

// -----------------------------------------------------------------------------
//
// prototype
//
// -----------------------------------------------------------------------------

namespace ME::ecs {

class prototype final {
public:
    prototype() = default;
    ~prototype() noexcept = default;

    prototype(const prototype& other);
    prototype& operator=(const prototype& other);

    prototype(prototype&& other) noexcept;
    prototype& operator=(prototype&& other) noexcept;

    void clear() noexcept;
    bool empty() const noexcept;
    void swap(prototype& other) noexcept;

    template <typename T>
    bool has_component() const noexcept;

    template <typename T, typename... Args>
    prototype& component(Args&&... args) &;
    template <typename T, typename... Args>
    prototype&& component(Args&&... args) &&;

    prototype& merge_with(const prototype& other, bool override) &;
    prototype&& merge_with(const prototype& other, bool override) &&;

    template <typename T>
    bool apply_to_component(T& component) const;
    void apply_to_entity(entity& ent, bool override) const;

private:
    detail::sparse_map<family_id, detail::applier_uptr> appliers_;
};

void swap(prototype& l, prototype& r) noexcept;
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// triggers
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <typename E>
class after {
public:
    const E& event;
};

template <typename E>
class before {
public:
    const E& event;
};
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// system
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <>
class system<> {
public:
    virtual ~system() = default;
};

template <typename E>
class system<E> : public virtual system<> {
public:
    virtual void process(registry& owner, const E& event) = 0;
};

template <typename E, typename... Es>
class system<E, Es...> : public system<E>, public system<Es...> {};
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// feature
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
class feature final {
public:
    feature() = default;

    feature(const feature&) = delete;
    feature& operator=(const feature&) = delete;

    feature(feature&&) noexcept = default;
    feature& operator=(feature&&) noexcept = default;

    feature& enable() & noexcept;
    feature&& enable() && noexcept;

    feature& disable() & noexcept;
    feature&& disable() && noexcept;

    bool is_enabled() const noexcept;
    bool is_disabled() const noexcept;

    template <typename T, typename... Args>
    feature& add_system(Args&&... args) &;
    template <typename T, typename... Args>
    feature&& add_system(Args&&... args) &&;

    template <typename Event>
    feature& process_event(registry& owner, const Event& event);

private:
    bool disabled_{false};
    std::vector<std::unique_ptr<system<>>> systems_;
    mutable detail::incremental_locker systems_locker_;
};
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// registry
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
class registry final {
private:
    class uentity {
    public:
        uentity(registry& owner, entity_id id) noexcept;

        uentity(entity_id ent) noexcept;
        uentity(entity ent) noexcept;

        operator entity_id() const noexcept;
        operator entity() const noexcept;
        operator const_entity() const noexcept;

        entity_id id() const noexcept;
        registry* owner() noexcept;
        const registry* owner() const noexcept;

        bool check_owner(const registry* owner) const noexcept;

    private:
        entity_id id_{0u};
        registry* owner_{nullptr};
    };

    class const_uentity {
    public:
        const_uentity(const registry& owner, entity_id id) noexcept;

        const_uentity(entity_id ent) noexcept;
        const_uentity(entity ent) noexcept;
        const_uentity(const_entity ent) noexcept;
        const_uentity(const uentity& ent) noexcept;

        operator entity_id() const noexcept;
        operator const_entity() const noexcept;

        entity_id id() const noexcept;
        const registry* owner() const noexcept;

        bool check_owner(const registry* owner) const noexcept;

    private:
        entity_id id_{0u};
        const registry* owner_{nullptr};
    };

public:
    registry() = default;

    registry(const registry& other) = delete;
    registry& operator=(const registry& other) = delete;

    registry(registry&& other) noexcept = default;
    registry& operator=(registry&& other) noexcept = default;

    entity wrap_entity(const const_uentity& ent) noexcept;
    const_entity wrap_entity(const const_uentity& ent) const noexcept;

    template <typename T>
    component<T> wrap_component(const const_uentity& ent) noexcept;
    template <typename T>
    const_component<T> wrap_component(const const_uentity& ent) const noexcept;

    entity create_entity();
    entity create_entity(const prototype& proto);
    entity create_entity(const const_uentity& proto);

    void destroy_entity(const uentity& ent) noexcept;
    bool valid_entity(const const_uentity& ent) const noexcept;

    template <typename T, typename... Args>
    T& assign_component(const uentity& ent, Args&&... args);

    template <typename T, typename... Args>
    T& ensure_component(const uentity& ent, Args&&... args);

    template <typename T>
    bool remove_component(const uentity& ent) noexcept;

    template <typename T>
    bool exists_component(const const_uentity& ent) const noexcept;

    std::size_t remove_all_components(const uentity& ent) noexcept;

    template <typename T>
    std::size_t remove_all_components() noexcept;

    template <typename T>
    T& get_component(const uentity& ent);
    template <typename T>
    const T& get_component(const const_uentity& ent) const;

    template <typename T>
    T* find_component(const uentity& ent) noexcept;
    template <typename T>
    const T* find_component(const const_uentity& ent) const noexcept;

    template <typename... Ts>
    std::tuple<Ts&...> get_components(const uentity& ent);
    template <typename... Ts>
    std::tuple<const Ts&...> get_components(const const_uentity& ent) const;

    template <typename... Ts>
    std::tuple<Ts*...> find_components(const uentity& ent) noexcept;
    template <typename... Ts>
    std::tuple<const Ts*...> find_components(const const_uentity& ent) const noexcept;

    template <typename T>
    std::size_t component_count() const noexcept;
    std::size_t entity_count() const noexcept;
    std::size_t entity_component_count(const const_uentity& ent) const noexcept;

    template <typename F, typename... Opts>
    void for_each_entity(F&& f, Opts&&... opts);
    template <typename F, typename... Opts>
    void for_each_entity(F&& f, Opts&&... opts) const;

    template <typename T, typename F, typename... Opts>
    void for_each_component(F&& f, Opts&&... opts);
    template <typename T, typename F, typename... Opts>
    void for_each_component(F&& f, Opts&&... opts) const;

    template <typename... Ts, typename F, typename... Opts>
    void for_joined_components(F&& f, Opts&&... opts);
    template <typename... Ts, typename F, typename... Opts>
    void for_joined_components(F&& f, Opts&&... opts) const;

    template <typename Tag, typename... Args>
    feature& assign_feature(Args&&... args);

    template <typename Tag, typename... Args>
    feature& ensure_feature(Args&&... args);

    template <typename Tag>
    bool has_feature() const noexcept;

    template <typename Tag>
    feature& get_feature();

    template <typename Tag>
    const feature& get_feature() const;

    template <typename Event>
    registry& process_event(const Event& event);

    struct memory_usage_info {
        std::size_t entities{0u};
        std::size_t components{0u};
    };
    memory_usage_info memory_usage() const noexcept;

    template <typename T>
    std::size_t component_memory_usage() const noexcept;

private:
    template <typename T>
    detail::component_storage<T>* find_storage_() noexcept;

    template <typename T>
    const detail::component_storage<T>* find_storage_() const noexcept;

    template <typename T>
    detail::component_storage<T>& get_or_create_storage_();

    template <typename F, typename... Opts>
    void for_joined_components_impl_(std::index_sequence<>, F&& f, Opts&&... opts);

    template <typename F, typename... Opts>
    void for_joined_components_impl_(std::index_sequence<>, F&& f, Opts&&... opts) const;

    template <typename T, typename... Ts, typename F, typename... Opts, std::size_t I, std::size_t... Is>
    void for_joined_components_impl_(std::index_sequence<I, Is...>, F&& f, Opts&&... opts);

    template <typename T, typename... Ts, typename F, typename... Opts, std::size_t I, std::size_t... Is>
    void for_joined_components_impl_(std::index_sequence<I, Is...>, F&& f, Opts&&... opts) const;

    template <typename T, typename... Ts, typename F, typename Ss, typename... Cs>
    void for_joined_components_impl_(const uentity& e, const F& f, const Ss& ss, Cs&... cs);

    template <typename T, typename... Ts, typename F, typename Ss, typename... Cs>
    void for_joined_components_impl_(const const_uentity& e, const F& f, const Ss& ss, const Cs&... cs) const;

    template <typename F, typename... Cs>
    void for_joined_components_impl_(const uentity& e, const F& f, const std::tuple<>& ss, Cs&... cs);

    template <typename F, typename... Cs>
    void for_joined_components_impl_(const const_uentity& e, const F& f, const std::tuple<>& ss, const Cs&... cs) const;

private:
    entity_id last_entity_id_{0u};
    std::vector<entity_id> free_entity_ids_;

    mutable detail::incremental_locker entity_ids_locker_;
    detail::sparse_set<entity_id, detail::entity_id_indexer> entity_ids_;

    using storage_uptr = std::unique_ptr<detail::component_storage_base>;
    detail::sparse_map<family_id, storage_uptr> storages_;

    mutable detail::incremental_locker features_locker_;
    detail::sparse_map<family_id, feature> features_;
};
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// options
//
// -----------------------------------------------------------------------------

namespace ME::ecs {

//
// options
//

template <typename T>
class exists final {
public:
    bool operator()(const const_entity& e) const { return e.exists_component<T>(); }
};

template <typename... Ts>
class exists_any final {
public:
    bool operator()(const const_entity& e) const { return (... || e.exists_component<Ts>()); }
};

template <typename... Ts>
class exists_all final {
public:
    bool operator()(const const_entity& e) const { return (... && e.exists_component<Ts>()); }
};

//
// combinators
//

template <typename T>
class option_neg final {
public:
    option_neg(T opt) : opt_(std::move(opt)) {}

    bool operator()(const const_entity& e) const { return !opt_(e); }

private:
    T opt_;
};

template <typename... Ts>
class option_conj final {
public:
    option_conj(Ts... opts) : opts_(std::make_tuple(std::move(opts)...)) {}

    bool operator()(const const_entity& e) const {
        return std::apply([&e](auto&&... opts) { return (... && opts(e)); }, opts_);
    }

private:
    std::tuple<Ts...> opts_;
};

template <typename... Ts>
class option_disj final {
public:
    option_disj(Ts... opts) : opts_(std::make_tuple(std::move(opts)...)) {}

    bool operator()(const const_entity& e) const {
        return std::apply([&e](auto&&... opts) { return (... || opts(e)); }, opts_);
    }

private:
    std::tuple<Ts...> opts_;
};

class option_bool final {
public:
    option_bool(bool b) : bool_(b) {}

    bool operator()(const const_entity& e) const {
        (void)e;
        return bool_;
    }

private:
    bool bool_{false};
};

//
// operators
//

template <typename A, typename = std::enable_if_t<detail::is_option_v<A>>>
option_neg<std::decay_t<A>> operator!(A&& a) {
    return {std::forward<A>(a)};
}

template <typename A, typename B, typename = std::enable_if_t<detail::is_option_v<A>>, typename = std::enable_if_t<detail::is_option_v<B>>>
option_conj<std::decay_t<A>, std::decay_t<B>> operator&&(A&& a, B&& b) {
    return {std::forward<A>(a), std::forward<B>(b)};
}

template <typename A, typename B, typename = std::enable_if_t<detail::is_option_v<A>>, typename = std::enable_if_t<detail::is_option_v<B>>>
option_disj<std::decay_t<A>, std::decay_t<B>> operator||(A&& a, B&& b) {
    return {std::forward<A>(a), std::forward<B>(b)};
}
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// aspect
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <typename... Ts>
class aspect {
public:
    static auto to_option() noexcept { return (option_bool{true} && ... && exists<Ts>{}); }

    static bool match_entity(const const_entity& e) noexcept { return (... && e.exists_component<Ts>()); }

    template <typename F, typename... Opts>
    static void for_each_entity(registry& owner, F&& f, Opts&&... opts) {
        owner.for_joined_components<Ts...>([&f](const auto& e, const auto&...) { f(e); }, std::forward<Opts>(opts)...);
    }

    template <typename F, typename... Opts>
    static void for_each_entity(const registry& owner, F&& f, Opts&&... opts) {
        owner.for_joined_components<Ts...>([&f](const auto& e, const auto&...) { f(e); }, std::forward<Opts>(opts)...);
    }

    template <typename F, typename... Opts>
    static void for_joined_components(registry& owner, F&& f, Opts&&... opts) {
        owner.for_joined_components<Ts...>(std::forward<F>(f), std::forward<Opts>(opts)...);
    }

    template <typename F, typename... Opts>
    static void for_joined_components(const registry& owner, F&& f, Opts&&... opts) {
        owner.for_joined_components<Ts...>(std::forward<F>(f), std::forward<Opts>(opts)...);
    }
};
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// fillers
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
class entity_filler final {
public:
    entity_filler(entity& entity) noexcept : entity_(entity) {}

    template <typename T, typename... Args>
    entity_filler& component(Args&&... args) {
        entity_.assign_component<T>(std::forward<Args>(args)...);
        return *this;
    }

private:
    entity& entity_;
};

class registry_filler final {
public:
    registry_filler(registry& registry) noexcept : registry_(registry) {}

    template <typename Tag, typename... Args>
    registry_filler& feature(Args&&... args) {
        registry_.assign_feature<Tag>(std::forward<Args>(args)...);
        return *this;
    }

private:
    registry& registry_;
};
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// entity impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
inline entity::entity(registry& owner) noexcept : owner_(&owner) {}

inline entity::entity(registry& owner, entity_id id) noexcept : owner_(&owner), id_(id) {}

inline registry& entity::owner() noexcept { return *owner_; }

inline const registry& entity::owner() const noexcept { return *owner_; }

inline entity_id entity::id() const noexcept { return id_; }

inline entity entity::clone() const { return (*owner_).create_entity(id_); }

inline void entity::destroy() noexcept { (*owner_).destroy_entity(id_); }

inline bool entity::valid() const noexcept { return std::as_const(*owner_).valid_entity(id_); }

template <typename T, typename... Args>
T& entity::assign_component(Args&&... args) {
    return (*owner_).assign_component<T>(id_, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
T& entity::ensure_component(Args&&... args) {
    return (*owner_).ensure_component<T>(id_, std::forward<Args>(args)...);
}

template <typename T>
bool entity::remove_component() noexcept {
    return (*owner_).remove_component<T>(id_);
}

template <typename T>
bool entity::exists_component() const noexcept {
    return std::as_const(*owner_).exists_component<T>(id_);
}

inline std::size_t entity::remove_all_components() noexcept { return (*owner_).remove_all_components(id_); }

template <typename T>
T& entity::get_component() {
    return (*owner_).get_component<T>(id_);
}

template <typename T>
const T& entity::get_component() const {
    return std::as_const(*owner_).get_component<T>(id_);
}

template <typename T>
T* entity::find_component() noexcept {
    return (*owner_).find_component<T>(id_);
}

template <typename T>
const T* entity::find_component() const noexcept {
    return std::as_const(*owner_).find_component<T>(id_);
}

template <typename... Ts>
std::tuple<Ts&...> entity::get_components() {
    return (*owner_).get_components<Ts...>(id_);
}

template <typename... Ts>
std::tuple<const Ts&...> entity::get_components() const {
    return std::as_const(*owner_).get_components<Ts...>(id_);
}

template <typename... Ts>
std::tuple<Ts*...> entity::find_components() noexcept {
    return (*owner_).find_components<Ts...>(id_);
}

template <typename... Ts>
std::tuple<const Ts*...> entity::find_components() const noexcept {
    return std::as_const(*owner_).find_components<Ts...>(id_);
}

inline std::size_t entity::component_count() const noexcept { return std::as_const(*owner_).entity_component_count(id_); }

inline bool operator<(const entity& l, const entity& r) noexcept { return (&l.owner() < &r.owner()) || (&l.owner() == &r.owner() && l.id() < r.id()); }

inline bool operator==(const entity& l, const entity& r) noexcept { return &l.owner() == &r.owner() && l.id() == r.id(); }

inline bool operator==(const entity& l, const const_entity& r) noexcept { return &l.owner() == &r.owner() && l.id() == r.id(); }

inline bool operator!=(const entity& l, const entity& r) noexcept { return !(l == r); }

inline bool operator!=(const entity& l, const const_entity& r) noexcept { return !(l == r); }
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// const_entity impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
inline const_entity::const_entity(const entity& ent) noexcept : owner_(&ent.owner()), id_(ent.id()) {}

inline const_entity::const_entity(const registry& owner) noexcept : owner_(&owner) {}

inline const_entity::const_entity(const registry& owner, entity_id id) noexcept : owner_(&owner), id_(id) {}

inline const registry& const_entity::owner() const noexcept { return *owner_; }

inline entity_id const_entity::id() const noexcept { return id_; }

inline bool const_entity::valid() const noexcept { return (*owner_).valid_entity(id_); }

template <typename T>
bool const_entity::exists_component() const noexcept {
    return (*owner_).exists_component<T>(id_);
}

template <typename T>
const T& const_entity::get_component() const {
    return (*owner_).get_component<T>(id_);
}

template <typename T>
const T* const_entity::find_component() const noexcept {
    return (*owner_).find_component<T>(id_);
}

template <typename... Ts>
std::tuple<const Ts&...> const_entity::get_components() const {
    return (*owner_).get_components<Ts...>(id_);
}

template <typename... Ts>
std::tuple<const Ts*...> const_entity::find_components() const noexcept {
    return (*owner_).find_components<Ts...>(id_);
}

inline std::size_t const_entity::component_count() const noexcept { return (*owner_).entity_component_count(id_); }

inline bool operator<(const const_entity& l, const const_entity& r) noexcept { return (&l.owner() < &r.owner()) || (&l.owner() == &r.owner() && l.id() < r.id()); }

inline bool operator==(const const_entity& l, const entity& r) noexcept { return &l.owner() == &r.owner() && l.id() == r.id(); }

inline bool operator==(const const_entity& l, const const_entity& r) noexcept { return &l.owner() == &r.owner() && l.id() == r.id(); }

inline bool operator!=(const const_entity& l, const entity& r) noexcept { return !(l == r); }

inline bool operator!=(const const_entity& l, const const_entity& r) noexcept { return !(l == r); }
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// component impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <typename T>
component<T>::component(const entity& owner) noexcept : owner_(owner) {}

template <typename T>
entity& component<T>::owner() noexcept {
    return owner_;
}

template <typename T>
const entity& component<T>::owner() const noexcept {
    return owner_;
}

template <typename T>
bool component<T>::valid() const noexcept {
    return owner_.valid();
}

template <typename T>
bool component<T>::exists() const noexcept {
    return owner_.exists_component<T>();
}

template <typename T>
template <typename... Args>
T& component<T>::assign(Args&&... args) {
    return owner_.assign_component<T>(std::forward<Args>(args)...);
}

template <typename T>
template <typename... Args>
T& component<T>::ensure(Args&&... args) {
    return owner_.ensure_component<T>(std::forward<Args>(args)...);
}

template <typename T>
bool component<T>::remove() noexcept {
    return owner_.remove_component<T>();
}

template <typename T>
T& component<T>::get() {
    return owner_.get_component<T>();
}

template <typename T>
const T& component<T>::get() const {
    return std::as_const(owner_).template get_component<T>();
}

template <typename T>
T* component<T>::find() noexcept {
    return owner_.find_component<T>();
}

template <typename T>
const T* component<T>::find() const noexcept {
    return std::as_const(owner_).template find_component<T>();
}

template <typename T>
T& component<T>::operator*() {
    return get();
}

template <typename T>
const T& component<T>::operator*() const {
    return get();
}

template <typename T>
T* component<T>::operator->() noexcept {
    return find();
}

template <typename T>
const T* component<T>::operator->() const noexcept {
    return find();
}

template <typename T>
component<T>::operator bool() const noexcept {
    return exists();
}

template <typename T>
bool operator<(const component<T>& l, const component<T>& r) noexcept {
    return l.owner() < r.owner();
}

template <typename T>
bool operator==(const component<T>& l, const component<T>& r) noexcept {
    return l.owner() == r.owner();
}

template <typename T>
bool operator==(const component<T>& l, const const_component<T>& r) noexcept {
    return l.owner() == r.owner();
}

template <typename T>
bool operator!=(const component<T>& l, const component<T>& r) noexcept {
    return !(l == r);
}

template <typename T>
bool operator!=(const component<T>& l, const const_component<T>& r) noexcept {
    return !(l == r);
}
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// const_component impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
template <typename T>
const_component<T>::const_component(const component<T>& comp) noexcept : owner_(comp.owner()) {}

template <typename T>
const_component<T>::const_component(const const_entity& owner) noexcept : owner_(owner) {}

template <typename T>
const const_entity& const_component<T>::owner() const noexcept {
    return owner_;
}

template <typename T>
bool const_component<T>::valid() const noexcept {
    return owner_.valid();
}

template <typename T>
bool const_component<T>::exists() const noexcept {
    return std::as_const(owner_).template exists_component<T>();
}

template <typename T>
const T& const_component<T>::get() const {
    return std::as_const(owner_).template get_component<T>();
}

template <typename T>
const T* const_component<T>::find() const noexcept {
    return std::as_const(owner_).template find_component<T>();
}

template <typename T>
const T& const_component<T>::operator*() const {
    return get();
}

template <typename T>
const T* const_component<T>::operator->() const noexcept {
    return find();
}

template <typename T>
const_component<T>::operator bool() const noexcept {
    return exists();
}

template <typename T>
bool operator<(const const_component<T>& l, const const_component<T>& r) noexcept {
    return l.owner() < r.owner();
}

template <typename T>
bool operator==(const const_component<T>& l, const component<T>& r) noexcept {
    return l.owner() == r.owner();
}

template <typename T>
bool operator==(const const_component<T>& l, const const_component<T>& r) noexcept {
    return l.owner() == r.owner();
}

template <typename T>
bool operator!=(const const_component<T>& l, const component<T>& r) noexcept {
    return !(l == r);
}

template <typename T>
bool operator!=(const const_component<T>& l, const const_component<T>& r) noexcept {
    return !(l == r);
}
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// prototype impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
namespace detail {
template <typename T, typename... Args>
typed_applier_with_args<T, Args...>::typed_applier_with_args(std::tuple<Args...>&& args) : args_(std::move(args)) {}

template <typename T, typename... Args>
typed_applier_with_args<T, Args...>::typed_applier_with_args(const std::tuple<Args...>& args) : args_(args) {}

template <typename T, typename... Args>
applier_uptr typed_applier_with_args<T, Args...>::clone() const {
    return std::make_unique<typed_applier_with_args>(args_);
}

template <typename T, typename... Args>
void typed_applier_with_args<T, Args...>::apply_to_entity(entity& ent, bool override) const {
    std::apply(
            [&ent, override](const Args&... args) {
                if (override || !ent.exists_component<T>()) {
                    ent.assign_component<T>(args...);
                }
            },
            args_);
}

template <typename T, typename... Args>
void typed_applier_with_args<T, Args...>::apply_to_component(T& component) const {
    std::apply([&component](const Args&... args) { component = T{args...}; }, args_);
}
}  // namespace detail

inline prototype::prototype(const prototype& other) {
    for (const family_id family : other.appliers_) {
        appliers_.insert(family, other.appliers_.get(family)->clone());
    }
}

inline prototype& prototype::operator=(const prototype& other) {
    if (this != &other) {
        prototype p(other);
        swap(p);
    }
    return *this;
}

inline prototype::prototype(prototype&& other) noexcept : appliers_(std::move(other.appliers_)) {}

inline prototype& prototype::operator=(prototype&& other) noexcept {
    if (this != &other) {
        swap(other);
        other.clear();
    }
    return *this;
}

inline void prototype::clear() noexcept { appliers_.clear(); }

inline bool prototype::empty() const noexcept { return appliers_.empty(); }

inline void prototype::swap(prototype& other) noexcept {
    using std::swap;
    swap(appliers_, other.appliers_);
}

template <typename T>
bool prototype::has_component() const noexcept {
    const auto family = detail::type_family<T>::id();
    return appliers_.has(family);
}

template <typename T, typename... Args>
prototype& prototype::component(Args&&... args) & {
    using applier_t = detail::typed_applier_with_args<T, std::decay_t<Args>...>;
    auto applier = std::make_unique<applier_t>(std::make_tuple(std::forward<Args>(args)...));
    const auto family = detail::type_family<T>::id();
    appliers_.insert_or_assign(family, std::move(applier));
    return *this;
}

template <typename T, typename... Args>
prototype&& prototype::component(Args&&... args) && {
    component<T>(std::forward<Args>(args)...);
    return std::move(*this);
}

inline prototype& prototype::merge_with(const prototype& other, bool override) & {
    for (const auto family : other.appliers_) {
        if (override || !appliers_.has(family)) {
            appliers_.insert_or_assign(family, other.appliers_.get(family)->clone());
        }
    }
    return *this;
}

inline prototype&& prototype::merge_with(const prototype& other, bool override) && {
    merge_with(other, override);
    return std::move(*this);
}

template <typename T>
bool prototype::apply_to_component(T& component) const {
    const auto family = detail::type_family<T>::id();
    const auto applier_base_ptr = appliers_.find(family);
    if (!applier_base_ptr) {
        return false;
    }
    using applier_t = detail::typed_applier<T>;
    const auto applier = static_cast<applier_t*>(applier_base_ptr->get());
    applier->apply_to_component(component);
    return true;
}

inline void prototype::apply_to_entity(entity& ent, bool override) const {
    for (const auto family : appliers_) {
        appliers_.get(family)->apply_to_entity(ent, override);
    }
}

inline void swap(prototype& l, prototype& r) noexcept { l.swap(r); }
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// feature impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
inline feature& feature::enable() & noexcept {
    disabled_ = false;
    return *this;
}

inline feature&& feature::enable() && noexcept {
    enable();
    return std::move(*this);
}

inline feature& feature::disable() & noexcept {
    disabled_ = true;
    return *this;
}

inline feature&& feature::disable() && noexcept {
    disable();
    return std::move(*this);
}

inline bool feature::is_enabled() const noexcept { return !disabled_; }

inline bool feature::is_disabled() const noexcept { return disabled_; }

template <typename T, typename... Args>
feature& feature::add_system(Args&&... args) & {
    assert(!systems_locker_.is_locked());
    systems_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    return *this;
}

template <typename T, typename... Args>
feature&& feature::add_system(Args&&... args) && {
    add_system<T>(std::forward<Args>(args)...);
    return std::move(*this);
}

template <typename Event>
feature& feature::process_event(registry& owner, const Event& event) {
    detail::incremental_lock_guard lock(systems_locker_);

    const auto fire_event = [this, &owner](const auto& wrapped_event) {
        for (const auto& base_system : systems_) {
            using system_type = system<std::decay_t<decltype(wrapped_event)>>;
            if (auto event_system = dynamic_cast<system_type*>(base_system.get())) {
                event_system->process(owner, wrapped_event);
            }
        }
    };

    fire_event(before<Event>{event});
    fire_event(event);
    fire_event(after<Event>{event});

    return *this;
}
}  // namespace ME::ecs

// -----------------------------------------------------------------------------
//
// registry impl
//
// -----------------------------------------------------------------------------

namespace ME::ecs {
//
// registry::uentity
//

inline registry::uentity::uentity(registry& owner, entity_id id) noexcept : id_(id), owner_(&owner) {}

inline registry::uentity::uentity(entity_id ent) noexcept : id_(ent) {}

inline registry::uentity::uentity(entity ent) noexcept : id_(ent.id()), owner_(&ent.owner()) {}

inline registry::uentity::operator entity_id() const noexcept { return id_; }

inline registry::uentity::operator entity() const noexcept {
    assert(owner_);
    return {*owner_, id_};
}

inline registry::uentity::operator const_entity() const noexcept {
    assert(owner_);
    return {*owner_, id_};
}

inline entity_id registry::uentity::id() const noexcept { return id_; }

inline registry* registry::uentity::owner() noexcept { return owner_; }

inline const registry* registry::uentity::owner() const noexcept { return owner_; }

inline bool registry::uentity::check_owner(const registry* owner) const noexcept { return !owner_ || owner_ == owner; }

//
// registry::const_uentity
//

inline registry::const_uentity::const_uentity(const registry& owner, entity_id id) noexcept : id_(id), owner_(&owner) {}

inline registry::const_uentity::const_uentity(entity_id ent) noexcept : id_(ent) {}

inline registry::const_uentity::const_uentity(entity ent) noexcept : id_(ent.id()), owner_(&ent.owner()) {}

inline registry::const_uentity::const_uentity(const_entity ent) noexcept : id_(ent.id()), owner_(&ent.owner()) {}

inline registry::const_uentity::const_uentity(const uentity& ent) noexcept : id_(ent.id()), owner_(ent.owner()) {}

inline registry::const_uentity::operator entity_id() const noexcept { return id_; }

inline registry::const_uentity::operator const_entity() const noexcept {
    assert(owner_);
    return {*owner_, id_};
}

inline entity_id registry::const_uentity::id() const noexcept { return id_; }

inline const registry* registry::const_uentity::owner() const noexcept { return owner_; }

inline bool registry::const_uentity::check_owner(const registry* owner) const noexcept { return !owner_ || owner_ == owner; }

//
// registry
//

inline entity registry::wrap_entity(const const_uentity& ent) noexcept { return {*this, ent.id()}; }

inline const_entity registry::wrap_entity(const const_uentity& ent) const noexcept { return {*this, ent.id()}; }

template <typename T>
component<T> registry::wrap_component(const const_uentity& ent) noexcept {
    return component<T>{wrap_entity(ent)};
}

template <typename T>
const_component<T> registry::wrap_component(const const_uentity& ent) const noexcept {
    return const_component<T>{wrap_entity(ent)};
}

inline entity registry::create_entity() {
    assert(!entity_ids_locker_.is_locked());
    if (!free_entity_ids_.empty()) {
        const auto free_ent_id = free_entity_ids_.back();
        const auto new_ent_id = detail::upgrade_entity_id(free_ent_id);
        entity_ids_.insert(new_ent_id);
        free_entity_ids_.pop_back();
        return wrap_entity(new_ent_id);
    }
    if (last_entity_id_ >= detail::entity_id_index_mask) {
        throw std::logic_error("ME::ecs::registry (entity index overlow)");
    }
    if (free_entity_ids_.capacity() <= entity_ids_.size()) {
        // ensure free entity ids capacity for safe (noexcept) entity destroying
        free_entity_ids_.reserve(detail::next_capacity_size(free_entity_ids_.capacity(), entity_ids_.size() + 1, free_entity_ids_.max_size()));
    }
    entity_ids_.insert(last_entity_id_ + 1);
    return wrap_entity(++last_entity_id_);
}

inline entity registry::create_entity(const prototype& proto) {
    auto ent = create_entity();
    try {
        proto.apply_to_entity(ent, true);
    } catch (...) {
        destroy_entity(ent);
        throw;
    }
    return ent;
}

inline entity registry::create_entity(const const_uentity& proto) {
    assert(valid_entity(proto));
    entity ent = create_entity();
    try {
        for (const auto family : storages_) {
            storages_.get(family)->clone(proto, ent.id());
        }
    } catch (...) {
        destroy_entity(ent);
        throw;
    }
    return ent;
}

inline void registry::destroy_entity(const uentity& ent) noexcept {
    assert(!entity_ids_locker_.is_locked());
    assert(valid_entity(ent));
    remove_all_components(ent);
    if (entity_ids_.unordered_erase(ent)) {
        assert(free_entity_ids_.size() < free_entity_ids_.capacity());
        free_entity_ids_.push_back(ent);
    }
}

inline bool registry::valid_entity(const const_uentity& ent) const noexcept {
    assert(ent.check_owner(this));
    return entity_ids_.has(ent);
}

template <typename T, typename... Args>
T& registry::assign_component(const uentity& ent, Args&&... args) {
    assert(valid_entity(ent));
    return get_or_create_storage_<T>().assign(ent, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
T& registry::ensure_component(const uentity& ent, Args&&... args) {
    assert(valid_entity(ent));
    return get_or_create_storage_<T>().ensure(ent, std::forward<Args>(args)...);
}

template <typename T>
bool registry::remove_component(const uentity& ent) noexcept {
    assert(valid_entity(ent));
    detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->remove(ent) : false;
}

template <typename T>
bool registry::exists_component(const const_uentity& ent) const noexcept {
    assert(valid_entity(ent));
    const detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->exists(ent) : false;
}

inline std::size_t registry::remove_all_components(const uentity& ent) noexcept {
    assert(valid_entity(ent));
    std::size_t removed_count = 0u;
    for (const auto family : storages_) {
        if (storages_.get(family)->remove(ent)) {
            ++removed_count;
        }
    }
    return removed_count;
}

template <typename T>
std::size_t registry::remove_all_components() noexcept {
    detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->remove_all() : 0u;
}

template <typename T>
T& registry::get_component(const uentity& ent) {
    assert(valid_entity(ent));
    if (T* component = find_component<T>(ent)) {
        return *component;
    }
    throw std::logic_error("ME::ecs::registry (component not found)");
}

template <typename T>
const T& registry::get_component(const const_uentity& ent) const {
    assert(valid_entity(ent));
    if (const T* component = find_component<T>(ent)) {
        return *component;
    }
    throw std::logic_error("ME::ecs::registry (component not found)");
}

template <typename T>
T* registry::find_component(const uentity& ent) noexcept {
    assert(valid_entity(ent));
    detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->find(ent) : nullptr;
}

template <typename T>
const T* registry::find_component(const const_uentity& ent) const noexcept {
    assert(valid_entity(ent));
    const detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->find(ent) : nullptr;
}

template <typename... Ts>
std::tuple<Ts&...> registry::get_components(const uentity& ent) {
    (void)ent;
    assert(valid_entity(ent));
    return std::make_tuple(std::ref(get_component<Ts>(ent))...);
}

template <typename... Ts>
std::tuple<const Ts&...> registry::get_components(const const_uentity& ent) const {
    (void)ent;
    assert(valid_entity(ent));
    return std::make_tuple(std::cref(get_component<Ts>(ent))...);
}

template <typename... Ts>
std::tuple<Ts*...> registry::find_components(const uentity& ent) noexcept {
    (void)ent;
    assert(valid_entity(ent));
    return std::make_tuple(find_component<Ts>(ent)...);
}

template <typename... Ts>
std::tuple<const Ts*...> registry::find_components(const const_uentity& ent) const noexcept {
    (void)ent;
    assert(valid_entity(ent));
    return std::make_tuple(find_component<Ts>(ent)...);
}

template <typename T>
std::size_t registry::component_count() const noexcept {
    const detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->count() : 0u;
}

inline std::size_t registry::entity_count() const noexcept { return entity_ids_.size(); }

inline std::size_t registry::entity_component_count(const const_uentity& ent) const noexcept {
    assert(valid_entity(ent));
    std::size_t component_count = 0u;
    for (const auto family : storages_) {
        if (storages_.get(family)->has(ent)) {
            ++component_count;
        }
    }
    return component_count;
}

template <typename F, typename... Opts>
void registry::for_each_entity(F&& f, Opts&&... opts) {
    detail::incremental_lock_guard lock(entity_ids_locker_);
    for (const auto e : entity_ids_) {
        if (uentity ent{*this, e}; (... && opts(ent))) {
            f(ent);
        }
    }
}

template <typename F, typename... Opts>
void registry::for_each_entity(F&& f, Opts&&... opts) const {
    detail::incremental_lock_guard lock(entity_ids_locker_);
    for (const auto e : entity_ids_) {
        if (const_uentity ent{*this, e}; (... && opts(ent))) {
            f(ent);
        }
    }
}

template <typename T, typename F, typename... Opts>
void registry::for_each_component(F&& f, Opts&&... opts) {
    if (detail::component_storage<T>* storage = find_storage_<T>()) {
        storage->for_each_component([this, &f, &opts...](const entity_id e, T& t) {
            if (uentity ent{*this, e}; (... && opts(ent))) {
                f(ent, t);
            }
        });
    }
}

template <typename T, typename F, typename... Opts>
void registry::for_each_component(F&& f, Opts&&... opts) const {
    if (const detail::component_storage<T>* storage = find_storage_<T>()) {
        storage->for_each_component([this, &f, &opts...](const entity_id e, const T& t) {
            if (const_uentity ent{*this, e}; (... && opts(ent))) {
                f(ent, t);
            }
        });
    }
}

template <typename... Ts, typename F, typename... Opts>
void registry::for_joined_components(F&& f, Opts&&... opts) {
    for_joined_components_impl_<Ts...>(std::make_index_sequence<sizeof...(Ts)>(), std::forward<F>(f), std::forward<Opts>(opts)...);
}

template <typename... Ts, typename F, typename... Opts>
void registry::for_joined_components(F&& f, Opts&&... opts) const {
    for_joined_components_impl_<Ts...>(std::make_index_sequence<sizeof...(Ts)>(), std::forward<F>(f), std::forward<Opts>(opts)...);
}

template <typename Tag, typename... Args>
feature& registry::assign_feature(Args&&... args) {
    const auto feature_id = detail::type_family<Tag>::id();
    if (feature* f = features_.find(feature_id)) {
        return *f = feature{std::forward<Args>(args)...};
    }
    assert(!features_locker_.is_locked());
    return *features_.insert(feature_id, feature{std::forward<Args>(args)...}).first;
}

template <typename Tag, typename... Args>
feature& registry::ensure_feature(Args&&... args) {
    const auto feature_id = detail::type_family<Tag>::id();
    if (feature* f = features_.find(feature_id)) {
        return *f;
    }
    assert(!features_locker_.is_locked());
    return *features_.insert(feature_id, feature{std::forward<Args>(args)...}).first;
}

template <typename Tag>
bool registry::has_feature() const noexcept {
    const auto feature_id = detail::type_family<Tag>::id();
    return features_.has(feature_id);
}

template <typename Tag>
feature& registry::get_feature() {
    const auto feature_id = detail::type_family<Tag>::id();
    if (feature* f = features_.find(feature_id)) {
        return *f;
    }
    throw std::logic_error("ME::ecs::registry (feature not found)");
}

template <typename Tag>
const feature& registry::get_feature() const {
    const auto feature_id = detail::type_family<Tag>::id();
    if (const feature* f = features_.find(feature_id)) {
        return *f;
    }
    throw std::logic_error("ME::ecs::registry (feature not found)");
}

template <typename Event>
registry& registry::process_event(const Event& event) {
    detail::incremental_lock_guard lock(features_locker_);
    for (const auto family : features_) {
        if (feature& f = features_.get(family); f.is_enabled()) {
            f.process_event(*this, event);
        }
    }
    return *this;
}

inline registry::memory_usage_info registry::memory_usage() const noexcept {
    memory_usage_info info;
    info.entities += free_entity_ids_.capacity() * sizeof(free_entity_ids_[0]);
    info.entities += entity_ids_.memory_usage();
    for (const auto family : storages_) {
        info.components += storages_.get(family)->memory_usage();
    }
    return info;
}

template <typename T>
std::size_t registry::component_memory_usage() const noexcept {
    const detail::component_storage<T>* storage = find_storage_<T>();
    return storage ? storage->memory_usage() : 0u;
}

template <typename T>
detail::component_storage<T>* registry::find_storage_() noexcept {
    const auto family = detail::type_family<T>::id();
    using raw_storage_ptr = detail::component_storage<T>*;
    const storage_uptr* storage_uptr_ptr = storages_.find(family);
    return storage_uptr_ptr && *storage_uptr_ptr ? static_cast<raw_storage_ptr>(storage_uptr_ptr->get()) : nullptr;
}

template <typename T>
const detail::component_storage<T>* registry::find_storage_() const noexcept {
    const auto family = detail::type_family<T>::id();
    using raw_storage_ptr = const detail::component_storage<T>*;
    const storage_uptr* storage_uptr_ptr = storages_.find(family);
    return storage_uptr_ptr && *storage_uptr_ptr ? static_cast<raw_storage_ptr>(storage_uptr_ptr->get()) : nullptr;
}

template <typename T>
detail::component_storage<T>& registry::get_or_create_storage_() {
    if (detail::component_storage<T>* storage = find_storage_<T>()) {
        return *storage;
    }
    const auto family = detail::type_family<T>::id();
    storages_.insert(family, std::make_unique<detail::component_storage<T>>(*this));
    return *static_cast<detail::component_storage<T>*>(storages_.get(family).get());
}

template <typename F, typename... Opts>
void registry::for_joined_components_impl_(std::index_sequence<>, F&& f, Opts&&... opts) {
    for_each_entity(std::forward<F>(f), std::forward<Opts>(opts)...);
}

template <typename F, typename... Opts>
void registry::for_joined_components_impl_(std::index_sequence<>, F&& f, Opts&&... opts) const {
    for_each_entity(std::forward<F>(f), std::forward<Opts>(opts)...);
}

template <typename T, typename... Ts, typename F, typename... Opts, std::size_t I, std::size_t... Is>
void registry::for_joined_components_impl_(std::index_sequence<I, Is...>, F&& f, Opts&&... opts) {
    const auto ss = std::make_tuple(find_storage_<Ts>()...);
    if (detail::tuple_contains(ss, nullptr)) {
        return;
    }
    for_each_component<T>([this, &f, &ss](const uentity& e, T& t) { for_joined_components_impl_<Ts...>(e, f, ss, t); }, std::forward<Opts>(opts)...);
}

template <typename T, typename... Ts, typename F, typename... Opts, std::size_t I, std::size_t... Is>
void registry::for_joined_components_impl_(std::index_sequence<I, Is...>, F&& f, Opts&&... opts) const {
    const auto ss = std::make_tuple(find_storage_<Ts>()...);
    if (detail::tuple_contains(ss, nullptr)) {
        return;
    }
    for_each_component<T>([this, &f, &ss](const const_uentity& e, const T& t) { std::as_const(*this).for_joined_components_impl_<Ts...>(e, f, ss, t); }, std::forward<Opts>(opts)...);
}

template <typename T, typename... Ts, typename F, typename Ss, typename... Cs>
void registry::for_joined_components_impl_(const uentity& e, const F& f, const Ss& ss, Cs&... cs) {
    if (T* c = std::get<0>(ss)->find(e)) {
        for_joined_components_impl_<Ts...>(e, f, detail::tuple_tail(ss), cs..., *c);
    }
}

template <typename T, typename... Ts, typename F, typename Ss, typename... Cs>
void registry::for_joined_components_impl_(const const_uentity& e, const F& f, const Ss& ss, const Cs&... cs) const {
    if (const T* c = std::get<0>(ss)->find(e)) {
        for_joined_components_impl_<Ts...>(e, f, detail::tuple_tail(ss), cs..., *c);
    }
}

template <typename F, typename... Cs>
void registry::for_joined_components_impl_(const uentity& e, const F& f, const std::tuple<>& ss, Cs&... cs) {
    (void)ss;
    f(e, cs...);
}

template <typename F, typename... Cs>
void registry::for_joined_components_impl_(const const_uentity& e, const F& f, const std::tuple<>& ss, const Cs&... cs) const {
    (void)ss;
    f(e, cs...);
}
}  // namespace ME::ecs

#endif