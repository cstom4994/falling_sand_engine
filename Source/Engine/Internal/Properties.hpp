
#ifndef _METADOT_PROPERTIES_HPP_
#define _METADOT_PROPERTIES_HPP_

#include <cassert>
#include <concepts>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace Meta::properties {

    class properties;

    /**
	 * Archiver interface for (de)serialization of properties.
	 *
	 * Implementing this interface allows adding support for (de)serialization
	 * using any suitable format. For example, there are built-in archivers (by
	 * implementing this interface) for JSON and XML (de)serialization.
	 */
    struct archiver
    {
    public:
        /**
		 * Serialize properties to string.
		 *
		 * @param p The properties to serialize.
		 * @return The serialized properties.
		 */
        [[nodiscard]] virtual std::string save(const properties &p) const = 0;

        /**
		 * Deserialize properties from string.
		 *
		 * @param p The target properties.
		 * @param str The serialized properties.
		 * @return `true` on success, `false` on failure with optional error message.
		 */
        virtual std::pair<bool, std::string> load(properties &p, const std::string &str) const = 0;

        /**
		 * Serialize properties to a file.
		 *
		 * @param p The properties to serialize to a file.
		 * @param path The path of the output file.
		 * @return `true` on success, `false` on failure with optional error message.
		 */
        [[nodiscard("file i/o might fail")]] std::pair<bool, std::string> save(
                const properties &p, const std::filesystem::path &path) const {
            // Prepare file
            std::ofstream file;
            file.open(path, std::ios::out | std::ios::trunc);
            if (not file.is_open())
                return {false, "Could not open file for writing at path " + path.string()};

            // Write to file
            file << save(p);

            // Close file
            file.close();

            return {true, ""};
        }

        /**
		 * Deserialize properties from a file.
		 *
		 * @param p The target properties.
		 * @param path The path of the input file.
		 * @return `true` on success, `false` on failure with optional error message.
		 */
        [[nodiscard("file i/o might fail")]] std::pair<bool, std::string> load(
                properties &p, const std::filesystem::path &path) const {
            // Prepare file
            std::ifstream file;
            file.open(path, std::ios::in);
            if (not file.is_open())
                return {false, "Could not open file for reading at path " + path.string()};

            // Read from file
            std::stringstream ss;
            ss << file.rdbuf();

            // Close the file
            file.close();

            // Load from string
            return load(p, ss.str());
        }
    };

}// namespace Meta::properties

namespace Meta::properties {

    /**
	 * Exception indicating that a non-existing property was accessed.
	 */
    struct property_nonexist : std::runtime_error
    {
        /**
		 * Constructor.
		 *
		 * @param _property_name The name of the property that does not exist.
		 */
        explicit property_nonexist(const std::string &_property_name)
            : std::runtime_error("property \"" + _property_name + "\" does not exist."),
              property_name(_property_name) {}

    private:
        std::string property_name;
    };

    /**
	 * Exception indicating that a property already exists.
	 */
    struct property_exists : std::runtime_error
    {
        /**
		 * Constructor.
		 *
		 * @param _property_name The name of the property that exists.
		 */
        explicit property_exists(const std::string &_property_name)
            : std::runtime_error("property \"" + _property_name + "\" exists already."),
              property_name(_property_name) {}

    private:
        std::string property_name;
    };
}// namespace Meta::properties

#define MAKE_PROPERTY(name, type)                                                                  \
    ::Meta::properties::property<type> &name = make_property<type>(#name);

#define MAKE_NESTED_PROPERTY(name, type) type &name = make_nested_property<type>(#name);

#define LINK_PROPERTY(name, ptr) make_linked_property(#name, ptr);

#define LINK_PROPERTY_FUNCTIONS(name, type, setter, getter)                                        \
    make_linked_property_functions<type>(#name, std::bind(&setter, this, std::placeholders::_1),   \
                                         std::bind(&getter, this));

#define REGISTER_PROPERTY(type, f_to_string, f_from_string)                                        \
    template<>                                                                                     \
    struct Meta::properties::property<type> : property_impl<type>                                  \
    {                                                                                              \
        using property_impl<type>::operator=;                                                      \
        using property_impl<type>::operator==;                                                     \
                                                                                                   \
        property() {                                                                               \
            this->to_string = f_to_string;                                                         \
            this->from_string = f_from_string;                                                     \
        }                                                                                          \
    };

// -------------------------------------------------------------------------

// Include built-in properties

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Meta::properties {
    /**
	 * Callback type.
	 */
    using callback = std::function<void()>;

    /**
	 * Value setter type.
	 */
    template<typename T>
    using setter = std::function<void(T)>;

    /**
	 * Value getter type.
	 */
    template<typename T>
    using getter = std::function<T()>;

    /**
	 * Base class for any property implementation.
	 */
    struct property_base
    {
        /**
		 * Default constructor.
		 */
        property_base() = default;

        /**
		 * Copy constructor.
		 */
        property_base(const property_base &) = default;

        /**
		 * Move constructor.
		 */
        property_base(property_base &&) noexcept = default;

        /**
		 * Destructor.
		 */
        virtual ~property_base() = default;

        /**
		 * Copy-assignment operator.
		 *
		 * @param rhs The right hand side object to copy-assign from.
		 * @return Reference to the left hand side object.
		 */
        property_base &operator=(const property_base &rhs) = default;

        /**
		 * Move-assignment operator.
		 *
		 * @param rhs The right hand side object to move-assign from.
		 * @return Reference to the left hand side object.
		 */
        property_base &operator=(property_base &&rhs) noexcept = default;

        /**
		 * Function object to serialize property to string.
		 */
        std::function<std::string()> to_string;

        /**
		 * Function object to deserialize property from string.
		 */
        std::function<void(std::string)> from_string;

        /**
		 * Set an attribute value.
		 *
		 * @note If the attribute key already exists it will be updated.
		 *
		 * @param key The attribute key.
		 * @param value The attribute value.
		 */
        void set_attribute(const std::string &key, const std::string &value) {
            m_attributes.insert_or_assign(key, value);
        }

        /**
		 * Get an attribute value.
		 *
		 * @param key The attribute key.
		 * @return The attribute value (if any exists).
		 */
        [[nodiscard]] std::optional<std::string> attribute(const std::string &key) const {
            const auto &it = m_attributes.find(key);
            if (it == std::cend(m_attributes)) return std::nullopt;

            return it->second;
        }

        /**
		 * Get a key-value map of all attributes.
		 */
        [[nodiscard]] std::map<std::string, std::string> attributes() const { return m_attributes; }

    private:
        std::map<std::string, std::string> m_attributes;
    };

    /**
	 * @brief Standard property implementation.
	 *
	 * @tparam The property type.
	 */
    template<typename T>
    struct property_impl : property_base
    {
        /**
		 * The property value.
		 */
        T data = {};

        /**
		 * @brief Default constructor.
		 *
		 * @note This is only available if @p T is default constructible.
		 */
        property_impl()
            requires std::is_default_constructible_v<T>
        = default;

        /**
		 * @brief Copy constructor.
		 *
		 * @note This is only available if @p T is copy constructible.
		 *
		 * @param other The object to copy-construct from.
		 */
        property_impl(const property_impl<T> &other)
            requires std::is_copy_constructible_v<T>
        : property_base(other), data(other.data) {}

        /**
		 * @brief Move constructor.
		 *
		 * @note This is only available if @p T is move constructible.
		 *
		 * @param other The object to move-construct from.
		 */
        property_impl(property_impl<T> &&other) noexcept
            requires std::is_move_constructible_v<T>
        : property_base(std::move(other)), data(std::move(other.data)) {}

        /**
		 * @brief Copy assig the value.
		 *
		 * @note This is only available if @p T is copy assignable.
		 *
		 * @param t The value to copy-assign.
		 * @return Reference to this property.
		 */
        property_impl<T> &operator=(const T &t)
            requires std::is_copy_assignable_v<T>
        {
            this->data = t;
            this->notify();
            return *this;
        }

        /**
		 * @brief Move assig the value.
		 *
		 * @note This is only available if @p T is move assignable.
		 *
		 * @param t The value to move-assign.
		 * @return Reference to this property.
		 */
        property_impl<T> &operator=(T &&t) noexcept
            requires std::is_move_assignable_v<T>
        {
            this->data = std::move(t);
            this->notify();
            return *this;
        }

        /**
		 * @brief Copy assign from another property.
		 *
		 * @note This is only available if @p T is copy assignable.
		 *
		 * @param rhs The right hand side property to copy-assign from.
		 * @return Reference to the left hand side property.
		 */
        property_impl<T> &operator=(const property_impl<T> &rhs)
            requires std::is_copy_assignable_v<T>
        {
            data = rhs.data;
            return *this;
        }

        /**
		 * @brief Move assign from another property.
		 *
		 * @note This is only available if @p T is move assignable.
		 *
		 * @param rhs The right hand side property to move-assign from.
		 * @return Reference to the left hand side property.
		 */
        property_impl<T> &operator=(property_impl<T> &&rhs) noexcept
            requires std::is_move_assignable_v<T>
        {
            data = std::move(rhs.data);
            return *this;
        }

        /**
		 * @brief Compare the value.
		 *
		 * @param t The value to compare with.
		 * @return @p true if the values are equal, @p false otherwise.
		 */
        bool operator==(const T &t) const { return this->data == t; }

        /**
		 * Get the value.
		 */
        explicit operator T() const noexcept { return data; }

        /**
		 * @brief Register an observer.
		 *
		 * @details The callback @p cb will be invoked if the value changes.
		 *
		 * @param cb The callback to register.
		 */
        void register_observer(const callback &cb) { m_observers.push_back(cb); }

    protected:
        /**
		 * @brief Notify observers.
		 *
		 * @brief Notify all registered observers by invoking their callback.
		 */
        void notify() {
            std::for_each(std::begin(m_observers), std::end(m_observers),
                          [](const callback &cb) { std::invoke(cb); });
        }

    private:
        std::vector<callback> m_observers;
    };

    /**
	 * @brief Alias for standard property implementation.
	 */
    template<typename T>
    struct property : property_impl<T>
    {
    };

    /**
	 * Create a property which links to an existing value.
	 *
	 * @tparam T The type of the underlying value.
	 */
    template<typename T>
    struct property_link : property_base
    {
        T *data = nullptr;

        property_link() {
            property_base::to_string = std::bind(&property_link<T>::to_string, this);
            property_base::from_string =
                    std::bind(&property_link<T>::from_string, this, std::placeholders::_1);
        }

    private:
        [[nodiscard]] std::string to_string() const {
            property<T> p;
            p.data = *data;
            return p.to_string();
        }

        void from_string(const std::string &str) {
            property<T> p;
            p.from_string(str);
            *data = p.data;
        }
    };

    template<typename T>
    struct property_link_functions : property_base
    {
        property_link_functions(const setter<T> &setter, const getter<T> &getter)
            : m_setter(setter), m_getter(getter) {
            property_base::to_string = std::bind(&property_link_functions<T>::to_string, this);
            property_base::from_string = std::bind(&property_link_functions<T>::from_string, this,
                                                   std::placeholders::_1);
        }

    private:
        setter<T> m_setter;
        getter<T> m_getter;

        [[nodiscard]] std::string to_string() const {
            property<T> p;
            p.data = m_getter();
            return p.to_string();
        }

        void from_string(const std::string &str) {
            property<T> p;
            p.from_string(str);
            m_setter(p.data);
        }
    };

    template<typename T>
    constexpr T &property_cast(property_base *pb) {
        return dynamic_cast<property<T> *>(pb)->data;
    }

    template<typename T>
    constexpr const T &property_cast(const property_base *pb) {
        return dynamic_cast<const property<T> *>(pb)->data;
    }

    template<typename T>
    std::ostream &operator<<(std::ostream &os, const property<T> &p) {
        os << p.to_string();
        return os;
    }

}// namespace Meta::properties

// -------------------------------------------------------------------------

#include <string>

/**
 * Property for `std::basic_string`.
 */
template<typename T>
struct Meta::properties::property<std::basic_string<T>> : property_impl<std::basic_string<T>>
{
    using property_impl<std::basic_string<T>>::operator=;
    using property_impl<std::basic_string<T>>::operator==;

    property() {
        this->to_string = [this]() { return this->data; };
        this->from_string = [this](const std::string &str) { *this = str; };
    }
};

REGISTER_PROPERTY(
        bool, [this]() { return (this->data ? "true" : "false"); },
        [this](const std::string &str) { this->data = (str == "true" || str == "True"); })

REGISTER_PROPERTY(
        int, [this]() { return std::to_string(this->data); },
        [this](const std::string &str) { this->data = std::stoi(str); })

REGISTER_PROPERTY(
        float, [this]() { return std::to_string(this->data); },
        [this](const std::string &str) { this->data = std::stof(str); })

REGISTER_PROPERTY(
        double, [this]() { return std::to_string(this->data); },
        [this](const std::string &str) { this->data = std::stod(str); })

#include <filesystem>

/**
 * std::filesystem::path
 */
REGISTER_PROPERTY(
        std::filesystem::path, [this]() { return this->data.string(); },
        [this](const std::string &str) { *this = str; })

// -------------------------------------------------------------------------

namespace Meta::properties {
    /**
     * A container for zero or more properties.
     */
    class properties : public property_base {
    public:
        /**
		 * Default constructor.
		 */
        properties() = default;

        properties(const properties &other) = delete;
        properties(properties &&other) = delete;

        /**
		 * Destructor.
		 */
        virtual ~properties() {
            for (auto &[key, value]: m_properties) delete value;
        }

        properties &operator=(const properties &rhs) = delete;
        properties &operator=(properties &&rhs) noexcept = delete;

        /**
         * Iterators
		 *
		 * @{
         */
        auto begin() { return m_properties.begin(); }
        auto begin() const { return m_properties.begin(); }
        auto end() { return m_properties.end(); }
        auto end() const { return m_properties.end(); }
        auto cbegin() const { return m_properties.cbegin(); }
        auto cend() const { return m_properties.cend(); }
        /**
		 * @}
		 */

        /**
         * Create a new property.
         *
         * @tparam T The type of property.
         * @param name The name of the property.
         * @return A reference to the newly created property.
         */
        template<typename T>
        property<T> &make_property(const std::string &name) {
            if (m_properties.contains(name)) throw property_exists(name);

            auto p = new property<T>;
            m_properties.emplace(name, p);
            return *p;
        }

        /**
         * Create a nested property.
         *
         * @tparam T The type of property.
         * @param name The name of the property.
         * @return A reference to the newly created property.
         */
        template<typename T>
            requires std::derived_from<T, properties>
        T &make_nested_property(const std::string &name) {
            if (m_properties.contains(name)) throw property_exists(name);

            auto p = new T;
            m_properties.emplace(name, p);
            return *p;
        }

        /**
		 * Create a linked property.
		 *
		 * @tparam T The type of property.
		 * @param name The name of the property.
		 * @param ptr Pointer to the value.
		 * @return A reference to the newly created property.
		 */
        template<typename T>
        void make_linked_property(const std::string &name, T *ptr) {
            if (m_properties.contains(name)) throw property_exists(name);

            if (!ptr) throw std::logic_error("ptr must not be null.");

            auto p = new property_link<T>;
            p->data = ptr;
            m_properties.emplace(name, p);
        }

        /**
		 * Create a linked functions property.
		 *
		 * @tparam T The type of property.
		 * @param name The name of the property.
		 * @param setter The setter function.
		 * @param getter The getter function.
		 */
        template<typename T>
        void make_linked_property_functions(const std::string &name, const setter<T> &setter,
                                            const getter<T> &getter) {
            if (m_properties.contains(name)) throw property_exists(name);

            if (!setter) throw std::logic_error("setter must not be null.");

            if (!getter) throw std::logic_error("setter must not be null.");

            auto p = new property_link_functions<T>(setter, getter);
            m_properties.template emplace(name, p);
        }

        /**
		 * Get the number of properties.
		 *
		 * @return The number of properties.
		 */
        [[nodiscard]] std::size_t properties_count() const noexcept { return m_properties.size(); }

        /**
		 * Set the value of a specific property.
		 *
		 * @tparam T The property type.
		 * @param name The name of the property.
		 * @param t The value to be set.
		 */
        template<typename T>
        void set_property(const std::string &name, const T &t) {
            if (!m_properties.contains(name)) throw property_nonexist(name);

            property_cast<T>(m_properties[name]) = t;
        }

        /**
		 * Get the value of a specific property.
		 *
		 * @throw @p property_nonexist if no property exists with the specified name.
		 *
		 * @tparam T The property type.
		 * @param name The name of the property.
		 * @return The value of the property.
		 */
        template<typename T>
        [[nodiscard]] const T &get_property(const std::string &name) const {
            try {
                return property_cast<T>(m_properties.at(name));
            } catch ([[maybe_unused]] const std::out_of_range &e) { throw property_nonexist(name); }
        }

        /**
		 * Get a group of nested properties.
		 *
		 * @note The returned pointer is guaranteed not to be null.
		 *
		 * @throw @p @p property_nonexist if no properties group exists with the specified name.
		 *
		 * @param The name of the properties group.
		 * @return The corresponding properties group.
		 */
        [[nodiscard]] properties *get_nested_properties(const std::string &name) {
            auto it = m_properties.find(name);
            if (it == std::cend(m_properties)) throw property_nonexist(name);

            return dynamic_cast<properties *>(it->second);
        }

        /**
		 * Serialize properties to string.
		 *
		 * @param ar The archiver to use.
		 * @return The serialized string.
		 */
        [[nodiscard]] std::string save(const archiver &ar) const { return ar.save(*this); }

        /**
		 * Serialize properties to file.
		 *
		 * @param ar The archiver to use.
		 * @param path The file path.
		 * @return @p true if successful, @p false otherwise with optional error message.
		 */
        [[nodiscard("file i/o might fail")]] std::pair<bool, std::string> save(
                const archiver &ar, const std::filesystem::path &path) {
            return ar.save(*this, path);
        }

        /**
		 * Deserialize properties from string.
		 *
		 * @param ar The archiver to use.
		 * @param str The string to deserialize.
		 */
        void load(const archiver &ar, const std::string &str) { ar.load(*this, str); }

        /**
		 * Deserialize properties from file.
		 *
		 * @param ar The archiver to use.
		 * @path The file path.
		 * @return @p true on success, @p false otherwise with optional error message.
		 */
        [[nodiscard("file i/o might fail")]] std::pair<bool, std::string> load(
                const archiver &ar, const std::filesystem::path &path) {
            return ar.load(*this, path);
        }

    private:
        std::map<std::string, property_base *> m_properties;
    };
}// namespace Meta::properties

#endif