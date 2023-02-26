
#include <algorithm>
#include <cassert>
#include <tuple>
#include <vector>

#include "ecs_fwd.hpp"

namespace MetaEngine::ECS {
namespace detail {

//
// tuple_tail
//

namespace impl {
template <typename T, typename... Ts, std::size_t... Is>
std::tuple<Ts...> tuple_tail_impl(std::index_sequence<Is...>, std::tuple<T, Ts...>&& t) {
    (void)t;
    return std::make_tuple(std::move(std::get<Is + 1u>(t))...);
}

template <typename T, typename... Ts, std::size_t... Is>
std::tuple<Ts...> tuple_tail_impl(std::index_sequence<Is...>, const std::tuple<T, Ts...>& t) {
    (void)t;
    return std::make_tuple(std::get<Is + 1u>(t)...);
}
}  // namespace impl

template <typename T, typename... Ts>
std::tuple<Ts...> tuple_tail(std::tuple<T, Ts...>&& t) {
    return impl::tuple_tail_impl(std::make_index_sequence<sizeof...(Ts)>(), std::move(t));
}

template <typename T, typename... Ts>
std::tuple<Ts...> tuple_tail(const std::tuple<T, Ts...>& t) {
    return impl::tuple_tail_impl(std::make_index_sequence<sizeof...(Ts)>(), t);
}

//
// tuple_contains
//

namespace impl {
template <typename V, typename... Ts, std::size_t... Is>
bool tuple_contains_impl(std::index_sequence<Is...>, const std::tuple<Ts...>& t, const V& v) {
    (void)t;
    (void)v;
    return (... || (std::get<Is>(t) == v));
}
}  // namespace impl

template <typename V, typename... Ts>
bool tuple_contains(const std::tuple<Ts...>& t, const V& v) {
    return impl::tuple_contains_impl(std::make_index_sequence<sizeof...(Ts)>(), t, v);
}

//
// next_capacity_size
//

inline std::size_t next_capacity_size(std::size_t cur_size, std::size_t min_size, std::size_t max_size) {
    if (min_size > max_size) {
        throw std::length_error("MetaEngine::ECS::next_capacity_size");
    }
    if (cur_size >= max_size / 2u) {
        return max_size;
    }
    return std::max(cur_size * 2u, min_size);
}

//
// entity_id index/version
//

constexpr std::size_t entity_id_index_mask = (1u << entity_id_index_bits) - 1u;
constexpr std::size_t entity_id_version_mask = (1u << entity_id_version_bits) - 1u;

constexpr inline entity_id entity_id_index(entity_id id) noexcept { return id & entity_id_index_mask; }

constexpr inline entity_id entity_id_version(entity_id id) noexcept { return (id >> entity_id_index_bits) & entity_id_version_mask; }

constexpr inline entity_id entity_id_join(entity_id index, entity_id version) noexcept { return index | (version << entity_id_index_bits); }

constexpr inline entity_id upgrade_entity_id(entity_id id) noexcept { return entity_id_join(entity_id_index(id), entity_id_version(id) + 1u); }

template <typename Void = void>
class type_family_base {
    static_assert(std::is_void_v<Void>, "unexpected internal error");

protected:
    static family_id last_id_;
};

template <typename T>
class type_family final : public type_family_base<> {
public:
    static family_id id() noexcept {
        static family_id self_id = ++last_id_;
        assert(self_id > 0u && "MetaEngine::ECS::family_id overflow");
        return self_id;
    }
};

template <typename Void>
family_id type_family_base<Void>::last_id_ = 0u;

template <typename T>
struct sparse_indexer final {
    static_assert(std::is_unsigned_v<T>);
    static_assert(sizeof(T) <= sizeof(std::size_t));
    std::size_t operator()(const T v) const noexcept { return static_cast<std::size_t>(v); }
};

class incremental_locker final {
public:
    incremental_locker() = default;
    ~incremental_locker() noexcept = default;

    incremental_locker(incremental_locker&& other) noexcept = default;
    incremental_locker(const incremental_locker& other) noexcept = default;

    incremental_locker& operator=(incremental_locker&& other) noexcept {
        assert(!is_locked());
        (void)other;
        return *this;
    }

    incremental_locker& operator=(const incremental_locker& other) noexcept {
        assert(!is_locked());
        (void)other;
        return *this;
    }

    void lock() noexcept { ++lock_count_; }

    void unlock() noexcept {
        assert(lock_count_);
        --lock_count_;
    }

    bool is_locked() const noexcept { return !!lock_count_; }

private:
    std::size_t lock_count_{0u};
};

class incremental_lock_guard final {
public:
    incremental_lock_guard(incremental_locker& locker) : locker_(locker) { locker_.lock(); }

    ~incremental_lock_guard() noexcept { locker_.unlock(); }

    incremental_lock_guard(const incremental_lock_guard&) = delete;
    incremental_lock_guard& operator=(const incremental_lock_guard&) = delete;

private:
    incremental_locker& locker_;
};

template <typename T, typename Indexer = sparse_indexer<T>>
class sparse_set final {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

public:
    iterator begin() noexcept { return dense_.begin(); }

    iterator end() noexcept { return dense_.end(); }

    const_iterator begin() const noexcept { return dense_.begin(); }

    const_iterator end() const noexcept { return dense_.end(); }

    const_iterator cbegin() const noexcept { return dense_.cbegin(); }

    const_iterator cend() const noexcept { return dense_.cend(); }

public:
    sparse_set(const Indexer& indexer = Indexer()) : indexer_(indexer) {}

    sparse_set(const sparse_set& other) = default;
    sparse_set& operator=(const sparse_set& other) = default;

    sparse_set(sparse_set&& other) noexcept = default;
    sparse_set& operator=(sparse_set&& other) noexcept = default;

    void swap(sparse_set& other) noexcept {
        using std::swap;
        swap(dense_, other.dense_);
        swap(sparse_, other.sparse_);
    }

    template <typename UT>
    bool insert(UT&& v) {
        if (has(v)) {
            return false;
        }
        const std::size_t vi = indexer_(v);
        if (vi >= sparse_.size()) {
            sparse_.resize(next_capacity_size(sparse_.size(), vi + 1u, sparse_.max_size()));
        }
        dense_.push_back(std::forward<UT>(v));
        sparse_[vi] = dense_.size() - 1u;
        return true;
    }

    bool unordered_erase(const T& v) noexcept {
        if (!has(v)) {
            return false;
        }
        const std::size_t vi = indexer_(v);
        const std::size_t dense_index = sparse_[vi];
        if (dense_index != dense_.size() - 1) {
            using std::swap;
            swap(dense_[dense_index], dense_.back());
            sparse_[indexer_(dense_[dense_index])] = dense_index;
        }
        dense_.pop_back();
        return true;
    }

    void clear() noexcept { dense_.clear(); }

    bool has(const T& v) const noexcept {
        const std::size_t vi = indexer_(v);
        return vi < sparse_.size() && sparse_[vi] < dense_.size() && dense_[sparse_[vi]] == v;
    }

    const_iterator find(const T& v) const noexcept { return has(v) ? begin() + static_cast<std::ptrdiff_t>(sparse_[indexer_(v)]) : end(); }

    std::size_t get_dense_index(const T& v) const {
        const auto p = find_dense_index(v);
        if (p.second) {
            return p.first;
        }
        throw std::logic_error("MetaEngine::ECS::sparse_set (value not found)");
    }

    std::pair<std::size_t, bool> find_dense_index(const T& v) const noexcept { return has(v) ? std::make_pair(sparse_[indexer_(v)], true) : std::make_pair(std::size_t(-1), false); }

    bool empty() const noexcept { return dense_.empty(); }

    std::size_t size() const noexcept { return dense_.size(); }

    std::size_t memory_usage() const noexcept { return dense_.capacity() * sizeof(dense_[0]) + sparse_.capacity() * sizeof(sparse_[0]); }

private:
    Indexer indexer_;
    std::vector<T> dense_;
    std::vector<std::size_t> sparse_;
};

template <typename T, typename Indexer>
void swap(sparse_set<T, Indexer>& l, sparse_set<T, Indexer>& r) noexcept {
    l.swap(r);
}

template <typename K, typename T, typename Indexer = sparse_indexer<K>>
class sparse_map final {
public:
    using iterator = typename std::vector<K>::iterator;
    using const_iterator = typename std::vector<K>::const_iterator;

public:
    iterator begin() noexcept { return keys_.begin(); }

    iterator end() noexcept { return keys_.end(); }

    const_iterator begin() const noexcept { return keys_.begin(); }

    const_iterator end() const noexcept { return keys_.end(); }

    const_iterator cbegin() const noexcept { return keys_.cbegin(); }

    const_iterator cend() const noexcept { return keys_.cend(); }

public:
    sparse_map(const Indexer& indexer = Indexer()) : keys_(indexer) {}

    sparse_map(const sparse_map& other) = default;
    sparse_map& operator=(const sparse_map& other) = default;

    sparse_map(sparse_map&& other) noexcept = default;
    sparse_map& operator=(sparse_map&& other) noexcept = default;

    void swap(sparse_map& other) noexcept {
        using std::swap;
        swap(keys_, other.keys_);
        swap(values_, other.values_);
    }

    template <typename UK, typename UT>
    std::pair<T*, bool> insert(UK&& k, UT&& v) {
        if (T* value = find(k)) {
            return std::make_pair(value, false);
        }
        values_.push_back(std::forward<UT>(v));
        try {
            keys_.insert(std::forward<UK>(k));
            return std::make_pair(&values_.back(), true);
        } catch (...) {
            values_.pop_back();
            throw;
        }
    }

    template <typename UK, typename UT>
    std::pair<T*, bool> insert_or_assign(UK&& k, UT&& v) {
        if (T* value = find(k)) {
            *value = std::forward<UT>(v);
            return std::make_pair(value, false);
        }
        values_.push_back(std::forward<UT>(v));
        try {
            keys_.insert(std::forward<UK>(k));
            return std::make_pair(&values_.back(), true);
        } catch (...) {
            values_.pop_back();
            throw;
        }
    }

    bool unordered_erase(const K& k) noexcept {
        const auto value_index_p = keys_.find_dense_index(k);
        if (!value_index_p.second) {
            return false;
        }
        if (value_index_p.first != values_.size() - 1) {
            using std::swap;
            swap(values_[value_index_p.first], values_.back());
        }
        values_.pop_back();
        keys_.unordered_erase(k);
        return true;
    }

    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    bool has(const K& k) const noexcept { return keys_.has(k); }

    T& get(const K& k) { return values_[keys_.get_dense_index(k)]; }

    const T& get(const K& k) const { return values_[keys_.get_dense_index(k)]; }

    T* find(const K& k) noexcept {
        const auto value_index_p = keys_.find_dense_index(k);
        return value_index_p.second ? &values_[value_index_p.first] : nullptr;
    }

    const T* find(const K& k) const noexcept {
        const auto value_index_p = keys_.find_dense_index(k);
        return value_index_p.second ? &values_[value_index_p.first] : nullptr;
    }

    bool empty() const noexcept { return values_.empty(); }

    std::size_t size() const noexcept { return values_.size(); }

    std::size_t memory_usage() const noexcept { return keys_.memory_usage() + values_.capacity() * sizeof(values_[0]); }

private:
    sparse_set<K, Indexer> keys_;
    std::vector<T> values_;
};

template <typename K, typename T, typename Indexer>
void swap(sparse_map<K, T, Indexer>& l, sparse_map<K, T, Indexer>& r) noexcept {
    l.swap(r);
}

struct entity_id_indexer final {
    std::size_t operator()(entity_id id) const noexcept { return entity_id_index(id); }
};

class component_storage_base {
public:
    virtual ~component_storage_base() = default;
    virtual bool remove(entity_id id) noexcept = 0;
    virtual bool has(entity_id id) const noexcept = 0;
    virtual void clone(entity_id from, entity_id to) = 0;
    virtual std::size_t memory_usage() const noexcept = 0;
};

template <typename T, bool E = std::is_empty_v<T>>
class component_storage final : public component_storage_base {
public:
    component_storage(registry& owner) : owner_(owner) {}

    template <typename... Args>
    T& assign(entity_id id, Args&&... args) {
        if (T* value = components_.find(id)) {
            *value = T{std::forward<Args>(args)...};
            return *value;
        }
        assert(!components_locker_.is_locked());
        return *components_.insert(id, T{std::forward<Args>(args)...}).first;
    }

    template <typename... Args>
    T& ensure(entity_id id, Args&&... args) {
        if (T* value = components_.find(id)) {
            return *value;
        }
        assert(!components_locker_.is_locked());
        return *components_.insert(id, T{std::forward<Args>(args)...}).first;
    }

    bool exists(entity_id id) const noexcept { return components_.has(id); }

    bool remove(entity_id id) noexcept override {
        assert(!components_locker_.is_locked());
        return components_.unordered_erase(id);
    }

    std::size_t remove_all() noexcept {
        assert(!components_locker_.is_locked());
        const std::size_t count = components_.size();
        components_.clear();
        return count;
    }

    T* find(entity_id id) noexcept { return components_.find(id); }

    const T* find(entity_id id) const noexcept { return components_.find(id); }

    std::size_t count() const noexcept { return components_.size(); }

    bool has(entity_id id) const noexcept override { return components_.has(id); }

    void clone(entity_id from, entity_id to) override {
        if (const T* c = find(from)) {
            assign(to, *c);
        }
    }

    template <typename F>
    void for_each_component(F&& f) {
        detail::incremental_lock_guard lock(components_locker_);
        for (const entity_id id : components_) {
            f(id, components_.get(id));
        }
    }

    template <typename F>
    void for_each_component(F&& f) const {
        detail::incremental_lock_guard lock(components_locker_);
        for (const entity_id id : components_) {
            f(id, components_.get(id));
        }
    }

    std::size_t memory_usage() const noexcept override { return components_.memory_usage(); }

private:
    registry& owner_;
    mutable detail::incremental_locker components_locker_;
    detail::sparse_map<entity_id, T, entity_id_indexer> components_;
};

template <typename T>
class component_storage<T, true> final : public component_storage_base {
public:
    component_storage(registry& owner) : owner_(owner) {}

    template <typename... Args>
    T& assign(entity_id id, Args&&...) {
        if (components_.has(id)) {
            return empty_value_;
        }
        assert(!components_locker_.is_locked());
        components_.insert(id);
        return empty_value_;
    }

    template <typename... Args>
    T& ensure(entity_id id, Args&&...) {
        if (components_.has(id)) {
            return empty_value_;
        }
        assert(!components_locker_.is_locked());
        components_.insert(id);
        return empty_value_;
    }

    bool exists(entity_id id) const noexcept { return components_.has(id); }

    bool remove(entity_id id) noexcept override {
        assert(!components_locker_.is_locked());
        return components_.unordered_erase(id);
    }

    std::size_t remove_all() noexcept {
        assert(!components_locker_.is_locked());
        const std::size_t count = components_.size();
        components_.clear();
        return count;
    }

    T* find(entity_id id) noexcept { return components_.has(id) ? &empty_value_ : nullptr; }

    const T* find(entity_id id) const noexcept { return components_.has(id) ? &empty_value_ : nullptr; }

    std::size_t count() const noexcept { return components_.size(); }

    bool has(entity_id id) const noexcept override { return components_.has(id); }

    void clone(entity_id from, entity_id to) override {
        if (const T* c = find(from)) {
            assign(to, *c);
        }
    }

    template <typename F>
    void for_each_component(F&& f) {
        detail::incremental_lock_guard lock(components_locker_);
        for (const entity_id id : components_) {
            f(id, empty_value_);
        }
    }

    template <typename F>
    void for_each_component(F&& f) const {
        detail::incremental_lock_guard lock(components_locker_);
        for (const entity_id id : components_) {
            f(id, empty_value_);
        }
    }

    std::size_t memory_usage() const noexcept override { return components_.memory_usage(); }

private:
    registry& owner_;
    static T empty_value_;
    mutable detail::incremental_locker components_locker_;
    detail::sparse_set<entity_id, entity_id_indexer> components_;
};

template <typename T>
T component_storage<T, true>::empty_value_;

class applier_base;
using applier_uptr = std::unique_ptr<applier_base>;

class applier_base {
public:
    virtual ~applier_base() = default;
    virtual applier_uptr clone() const = 0;
    virtual void apply_to_entity(entity& ent, bool override) const = 0;
};

template <typename T>
class typed_applier : public applier_base {
public:
    virtual void apply_to_component(T& component) const = 0;
};

template <typename T, typename... Args>
class typed_applier_with_args final : public typed_applier<T> {
public:
    typed_applier_with_args(std::tuple<Args...>&& args);
    typed_applier_with_args(const std::tuple<Args...>& args);
    applier_uptr clone() const override;
    void apply_to_entity(entity& ent, bool override) const override;
    void apply_to_component(T& component) const override;

private:
    std::tuple<Args...> args_;
};

template <typename T>
struct is_option : std::false_type {};

template <typename T>
struct is_option<exists<T>> : std::true_type {};

template <typename... Ts>
struct is_option<exists_any<Ts...>> : std::true_type {};

template <typename... Ts>
struct is_option<exists_all<Ts...>> : std::true_type {};

template <typename T>
struct is_option<option_neg<T>> : std::true_type {};

template <typename... Ts>
struct is_option<option_conj<Ts...>> : std::true_type {};

template <typename... Ts>
struct is_option<option_disj<Ts...>> : std::true_type {};

template <>
struct is_option<option_bool> : std::true_type {};

template <typename T>
inline constexpr bool is_option_v = is_option<T>::value;
}  // namespace detail

}  // namespace MetaEngine::ECS