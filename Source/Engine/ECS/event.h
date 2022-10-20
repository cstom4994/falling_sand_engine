// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include <cstdint>
#include <functional>
#include <tuple>

#include "container.h"
#include "entity.h"
#include "metafunctions.h"
#include "typelist.h"

namespace MetaEngine::ECS {
    template<typename Entity>
    struct entity_created
    {
        Entity entity;
    };

    template<typename Entity>
    struct entity_destroyed
    {
        Entity entity;
    };

    template<typename Entity, typename Component>
    struct component_added
    {
        Entity entity;
        Component &component;
    };

    template<typename Entity, typename Component>
    struct component_removed
    {
        Entity entity;
        Component &component;
    };

    template<typename Entity, typename Tag>
    struct tag_added
    {
        Entity entity;
    };

    template<typename Entity, typename Tag>
    struct tag_removed
    {
        Entity entity;
    };

    template<typename Event>
    class subscriber_handle;

    template<typename... Ts>
    class event_manager {
        static_assert(meta::delay<Ts...>,
                      "The first two template paramaters must be of type component_list and type_list");
    };

    namespace detail {
        using subscriber_handle_id_t = std::uintmax_t;

        template<typename T>
        using event_sig_t = void(const T &);
        template<typename T>
        using event_func_t = std::function<event_sig_t<T>>;
        template<typename T>
        using event_map_t = flat_map<subscriber_handle_id_t, event_func_t<T>>;

        template<typename, typename>
        struct entity_event_manager;

        // Does not need to be own struct, can exist inside the event_manager
        template<typename... Components, typename... Tags>
        struct entity_event_manager<component_list<Components...>, tag_list<Tags...>>
        {
            using entity_t = entity<component_list<Components...>, tag_list<Tags...>>;
            using entity_created_t = entity_created<entity_t>;
            using entity_destroyed_t = entity_destroyed<entity_t>;
            template<typename T>
            using component_added_t = component_added<entity_t, T>;
            template<typename T>
            using component_removed_t = component_removed<entity_t, T>;
            template<typename T>
            using tag_added_t = tag_added<entity_t, T>;
            template<typename T>
            using tag_removed_t = tag_removed<entity_t, T>;
            using entity_events_t =
                    meta::typelist<entity_created_t, entity_destroyed_t, component_added_t<Components>...,
                                   component_removed_t<Components>..., tag_added_t<Tags>...,
                                   tag_removed_t<Tags>...>;

            static_assert(meta::is_typelist_unique_v<entity_events_t>,
                          "Internal events not unique. Panic!");

            detail::subscriber_handle_id_t currentId = 0;
            meta::tuple_from_typelist_t<entity_events_t, event_map_t> eventQueues;

            template<typename Event>
            void broadcast(const Event &event) const;

            template<typename Event, typename Func>
            subscriber_handle_id_t subscribe(Func &&func);

            template<typename Event>
            auto unsubscribe(detail::subscriber_handle_id_t id);
        };
    }// namespace detail

    template<typename... Components, typename... Tags, typename... Events>
    class event_manager<component_list<Components...>, tag_list<Tags...>, Events...> {
        using custom_events_t = meta::typelist<Events...>;
        static_assert(meta::is_typelist_unique_v<custom_events_t>, "Events must be unique");
        using entity_event_manager_t =
                detail::entity_event_manager<component_list<Components...>, tag_list<Tags...>>;
        using entity_events_t = typename entity_event_manager_t::entity_events_t;
        using events_t = meta::typelist_concat_t<custom_events_t, entity_events_t>;
        static_assert(meta::is_typelist_unique_v<events_t>, "Do not specify native events");

        template<typename Event>
        friend class subscriber_handle;

        friend class entity_manager<component_list<Components...>, tag_list<Tags...>>;

        detail::subscriber_handle_id_t currentId = 0;
        std::tuple<detail::event_map_t<Events>...> eventQueues;
        entity_event_manager_t entityEventManager;

        template<typename Event>
        void unsubscribe(detail::subscriber_handle_id_t id);

        const auto &get_entity_event_manager() const;

    public:
        event_manager() = default;
        event_manager(const event_manager &) = delete;
        event_manager &operator=(const event_manager &) = delete;

        template<typename Event, typename Func>
        subscriber_handle<Event> subscribe(Func &&func);

        template<typename Event>
        void broadcast(const Event &event) const;
    };

    template<typename Event>
    class subscriber_handle {
        detail::subscriber_handle_id_t id;
        void *manager = nullptr;
        void (*unsubscribe_ptr)(subscriber_handle *);

        template<typename... Events>
        static void unsubscribe_impl(subscriber_handle *self);

        template<typename...>
        friend class event_manager;

        struct private_access
        {
            explicit private_access() = default;
        };

    public:
        subscriber_handle() = default;

        template<typename... Events>
        subscriber_handle(private_access, event_manager<Events...> &em,
                          detail::subscriber_handle_id_t id) noexcept;

        subscriber_handle(subscriber_handle &&other) noexcept;
        subscriber_handle &operator=(subscriber_handle &&other) noexcept;

        bool is_valid() const;

        bool unsubscribe();
    };
}// namespace MetaEngine::ECS

// INC

// Copyright(c) 2022, KaoruXun All rights reserved.

#include <utility>

#include "exception.h"

namespace MetaEngine::ECS {

    namespace detail {
#define ENTITY_EVENT_MANAGER_TEMPS template<typename... CTs, typename... TTs>
#define ENTITY_EVENT_MANAGER_SPEC entity_event_manager<component_list<CTs...>, tag_list<TTs...>>

        ENTITY_EVENT_MANAGER_TEMPS
        template<typename Event>
        void ENTITY_EVENT_MANAGER_SPEC::broadcast(const Event &event) const {
            static_assert(meta::typelist_has_type_v<Event, entity_events_t>,
                          "broadcast called with invalid event");
            const auto &cont = std::get<event_map_t<Event>>(eventQueues);
            for (const auto &[id, func]: cont) {
                func(event);
            }
        }

        ENTITY_EVENT_MANAGER_TEMPS
        template<typename Event, typename Func>
        subscriber_handle_id_t ENTITY_EVENT_MANAGER_SPEC::subscribe(Func &&func) {
            assert(std::numeric_limits<subscriber_handle_id_t>::max() != currentId);
            auto [itr, emplaced] = std::get<event_map_t<Event>>(eventQueues)
                                           .try_emplace(currentId++, std::forward<Func>(func));
            assert(emplaced);

            return itr->first;
        }

        ENTITY_EVENT_MANAGER_TEMPS
        template<typename Event>
        auto ENTITY_EVENT_MANAGER_SPEC::unsubscribe(detail::subscriber_handle_id_t id) {
            return std::get<event_map_t<Event>>(eventQueues).erase(id);
        }

#undef ENTITY_EVENT_MANAGER_TEMPS
#undef ENTITY_EVENT_MANAGER_SPEC
    }// namespace detail

#define EVENT_MANAGER_TEMPS template<typename... CTs, typename... TTs, typename... Es>
#define EVENT_MANAGER_SPEC event_manager<component_list<CTs...>, tag_list<TTs...>, Es...>

    EVENT_MANAGER_TEMPS
    template<typename Event>
    void EVENT_MANAGER_SPEC::unsubscribe(detail::subscriber_handle_id_t id) {
        constexpr bool isNativeEvent = meta::typelist_has_type_v<Event, entity_events_t>;

        int er;
        if constexpr (isNativeEvent) {
            er = entityEventManager.template unsubscribe<Event>(id);
        } else {
            er = std::get<detail::event_map_t<Event>>(eventQueues).erase(id);
        }
        assert(er == 1);
    }

    EVENT_MANAGER_TEMPS
    const auto &EVENT_MANAGER_SPEC::get_entity_event_manager() const {
        return entityEventManager;
    }

    EVENT_MANAGER_TEMPS
    template<typename Event, typename Func>
    subscriber_handle<Event> EVENT_MANAGER_SPEC::subscribe(Func &&func) {
        constexpr bool isValidEvent = meta::typelist_has_type_v<Event, events_t>;
        constexpr bool isConstructible = std::is_constructible_v<detail::event_func_t<Event>, Func>;
        constexpr bool isNativeEvent = meta::typelist_has_type_v<Event, entity_events_t>;

        METADOT_ECS_CHECK(isValidEvent, "subscribe called with invalid event")
        METADOT_ECS_CHECK_ALSO(isConstructible, "subscribe called with invalid callable")
        else if constexpr (isNativeEvent) {
            return {typename subscriber_handle<Event>::private_access{}, *this,
                    entityEventManager.template subscribe<Event>(std::forward<Func>(func))};
        }
        else {
            assert(std::numeric_limits<detail::subscriber_handle_id_t>::max() != currentId);
            auto [itr, emplaced] = std::get<detail::event_map_t<Event>>(eventQueues)
                                           .try_emplace(currentId++, std::forward<Func>(func));
            assert(emplaced);

            return {typename subscriber_handle<Event>::private_access{}, *this, itr->first};
        }
    }

    EVENT_MANAGER_TEMPS
    template<typename Event>
    void EVENT_MANAGER_SPEC::broadcast(const Event &event) const {
        constexpr bool isValidEvent = meta::typelist_has_type_v<Event, custom_events_t>;

        METADOT_ECS_CHECK(isValidEvent, "broadcast called with invalid event")
        else {
            const auto &container = std::get<detail::event_map_t<Event>>(eventQueues);
            for (const auto &[id, func]: container) {
                func(event);
            }
        }
    }

#undef EVENT_MANAGER_TEMPS
#undef EVENT_MANAGER_SPEC

    template<typename Event>
    template<typename... Events>
    void subscriber_handle<Event>::unsubscribe_impl(subscriber_handle *self) {
        auto em = static_cast<event_manager<Events...> *>(self->manager);
        em->template unsubscribe<Event>(self->id);
    }

    template<typename Event>
    template<typename... Events>
    subscriber_handle<Event>::subscriber_handle(private_access, event_manager<Events...> &em,
                                                detail::subscriber_handle_id_t id) noexcept
        : id(id), manager(&em), unsubscribe_ptr(&unsubscribe_impl<Events...>) {}

    template<typename Event>
    subscriber_handle<Event>::subscriber_handle(subscriber_handle &&other) noexcept {
        *this = std::move(other);
    }

    template<typename Event>
    auto subscriber_handle<Event>::operator=(subscriber_handle &&other) noexcept -> subscriber_handle & {
        id = other.id;
        manager = std::exchange(other.manager, nullptr);
        unsubscribe_ptr = other.unsubscribe_ptr;

        return *this;
    }

    template<typename Event>
    bool subscriber_handle<Event>::is_valid() const {
        return manager != nullptr;
    }

    template<typename Event>
    bool subscriber_handle<Event>::unsubscribe() {
        if (manager) {
            unsubscribe_ptr(this);
            manager = nullptr;
            return true;
        }
        return false;
    }
}// namespace MetaEngine::ECS
