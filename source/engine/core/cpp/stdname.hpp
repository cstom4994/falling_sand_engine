#ifndef ME_CPP_STDNAME_HPP
#define ME_CPP_STDNAME_HPP

#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "name.hpp"

template <typename T>
struct MetaEngine::details::custom_type_name<std::queue<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::queue<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::forward_list<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::forward_list<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::list<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::list<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename T>
struct MetaEngine::details::custom_type_name<std::map<Key, T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::map<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename T, typename Less>
struct MetaEngine::details::custom_type_name<std::map<Key, T, Less>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::map<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Less>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename T>
struct MetaEngine::details::custom_type_name<std::multimap<Key, T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::multimap<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename T, typename Less>
struct MetaEngine::details::custom_type_name<std::multimap<Key, T, Less>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::multimap<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Less>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::unique_ptr<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unique_ptr<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::deque<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::deque<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::priority_queue<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::priority_queue<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T, typename Container>
struct MetaEngine::details::custom_type_name<std::priority_queue<T, Container>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::priority_queue<{"), type_name<T>(), TStr_of_a<','>{}, type_name<Container>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::set<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::set<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T, typename Less>
struct MetaEngine::details::custom_type_name<std::set<T, Less>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::set<{"), type_name<T>(), TStr_of_a<','>{}, type_name<Less>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::multiset<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::multiset<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T, typename Less>
struct MetaEngine::details::custom_type_name<std::multiset<T, Less>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::multiset<{"), type_name<T>(), TStr_of_a<','>{}, type_name<Less>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::span<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::span<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::stack<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::stack<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename Elem>
struct MetaEngine::details::custom_type_name<std::basic_string_view<Elem>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::basic_string_view<{"), type_name<Elem>(), TStrC_of<'}', '>'>{}); }
};

template <>
constexpr auto MetaEngine::type_name<std::string_view>() noexcept {
    return TSTR("std::string_view");
}

template <>
constexpr auto MetaEngine::type_name<std::wstring_view>() noexcept {
    return TSTR("std::wstring_view");
}

template <>
constexpr auto MetaEngine::type_name<std::u8string_view>() noexcept {
    return TSTR("std::u8string_view");
}

template <>
constexpr auto MetaEngine::type_name<std::u16string_view>() noexcept {
    return TSTR("std::u16string_view");
}

template <>
constexpr auto MetaEngine::type_name<std::u32string_view>() noexcept {
    return TSTR("std::u32string_view");
}

template <typename Elem>
struct MetaEngine::details::custom_type_name<std::basic_string<Elem>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::basic_string<{"), type_name<Elem>(), TStrC_of<'}', '>'>{}); }
};

template <typename Elem, typename Traits>
struct MetaEngine::details::custom_type_name<std::basic_string<Elem, Traits>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::basic_string<{"), type_name<Elem>(), TStr_of_a<','>{}, type_name<Traits>(), TStrC_of<'}', '>'>{}); }
};

template <>
constexpr auto MetaEngine::type_name<std::string>() noexcept {
    return TSTR("std::string");
}

template <>
constexpr auto MetaEngine::type_name<std::wstring>() noexcept {
    return TSTR("std::wstring");
}

template <>
constexpr auto MetaEngine::type_name<std::u8string>() noexcept {
    return TSTR("std::u8string");
}

template <>
constexpr auto MetaEngine::type_name<std::u16string>() noexcept {
    return TSTR("std::u16string");
}

template <>
constexpr auto MetaEngine::type_name<std::u32string>() noexcept {
    return TSTR("std::u32string");
}

template <typename Key, typename T>
struct MetaEngine::details::custom_type_name<std::unordered_map<Key, T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unordered_map<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename T, typename Hash>
struct MetaEngine::details::custom_type_name<std::unordered_map<Key, T, Hash>> {
    static constexpr auto get() noexcept {
        return concat_seq(TSTR("std::unordered_map<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Hash>(), TStrC_of<'}', '>'>{});
    }
};

template <typename Key, typename T, typename Hash, typename KeyEqual>
struct MetaEngine::details::custom_type_name<std::unordered_map<Key, T, Hash, KeyEqual>> {
    static constexpr auto get() noexcept {
        return concat_seq(TSTR("std::unordered_map<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Hash>(), TStr_of_a<','>{}, type_name<KeyEqual>(),
                          TStrC_of<'}', '>'>{});
    }
};

// template <typename Key, typename T, typename Hash>
// struct MetaEngine::details::custom_type_name<std::pmr::unordered_map<Key, T, Hash>> {
//     static constexpr auto get() noexcept {
//         return concat_seq(TSTR("std::pmr::unordered_map<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Hash>(), TStrC_of<'}', '>'>{});
//     }
// };

template <typename Key, typename T>
struct MetaEngine::details::custom_type_name<std::unordered_multimap<Key, T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unordered_multimap<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename T, typename Hash>
struct MetaEngine::details::custom_type_name<std::unordered_multimap<Key, T, Hash>> {
    static constexpr auto get() noexcept {
        return concat_seq(TSTR("std::unordered_multimap<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Hash>(), TStrC_of<'}', '>'>{});
    }
};

template <typename Key, typename T, typename Hash, typename KeyEqual>
struct MetaEngine::details::custom_type_name<std::unordered_multimap<Key, T, Hash, KeyEqual>> {
    static constexpr auto get() noexcept {
        return concat_seq(TSTR("std::unordered_multimap<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStr_of_a<','>{}, type_name<Hash>(), TStr_of_a<','>{}, type_name<KeyEqual>(),
                          TStrC_of<'}', '>'>{});
    }
};

// template <typename Key, typename T>
// struct MetaEngine::details::custom_type_name<std::pmr::unordered_multimap<Key, T>> {
//     static constexpr auto get() noexcept { return concat_seq(TSTR("std::pmr::unordered_multimap<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<T>(), TStrC_of<'}', '>'>{}); }
// };

template <typename Key>
struct MetaEngine::details::custom_type_name<std::unordered_set<Key>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unordered_set<{"), type_name<Key>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename Hash>
struct MetaEngine::details::custom_type_name<std::unordered_set<Key, Hash>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unordered_set<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<Hash>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename Hash, typename KeyEqual>
struct MetaEngine::details::custom_type_name<std::unordered_set<Key, Hash, KeyEqual>> {
    static constexpr auto get() noexcept {
        return concat_seq(TSTR("std::unordered_set<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<Hash>(), TStr_of_a<','>{}, type_name<KeyEqual>(), TStrC_of<'}', '>'>{});
    }
};

template <typename Key>
struct MetaEngine::details::custom_type_name<std::unordered_multiset<Key>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unordered_multiset<{"), type_name<Key>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename Hash>
struct MetaEngine::details::custom_type_name<std::unordered_multiset<Key, Hash>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::unordered_multiset<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<Hash>(), TStrC_of<'}', '>'>{}); }
};

template <typename Key, typename Hash, typename KeyEqual>
struct MetaEngine::details::custom_type_name<std::unordered_multiset<Key, Hash, KeyEqual>> {
    static constexpr auto get() noexcept {
        return concat_seq(TSTR("std::unordered_multiset<{"), type_name<Key>(), TStr_of_a<','>{}, type_name<Hash>(), TStr_of_a<','>{}, type_name<KeyEqual>(), TStrC_of<'}', '>'>{});
    }
};

template <typename T>
struct MetaEngine::details::custom_type_name<std::vector<T>> {
    static constexpr auto get() noexcept { return concat_seq(TSTR("std::vector<{"), type_name<T>(), TStrC_of<'}', '>'>{}); }
};

#endif