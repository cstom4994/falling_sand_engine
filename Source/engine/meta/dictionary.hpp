

#ifndef META_DICTIONARY_HPP
#define META_DICTIONARY_HPP

#include <algorithm>  // std::lower_bound
#include <utility>
#include <vector>

#include "engine/meta/config.hpp"

namespace Meta {
namespace detail {

template <typename T>
struct DictKeyCmp {
    bool operator()(T a, T b) const { return a < b; }
};

//
// Key-value pair dictionary.
//  - Stored as vector of pairs, more cache friendly.
//  - Sorted on keys. Once only insertion cost gives better access times.
//
template <typename KEY, typename KEY_REF, typename VALUE, class CMP = DictKeyCmp<KEY_REF>>
class Dictionary {
public:
    struct pair_t : public std::pair<KEY, VALUE> {
        pair_t() : std::pair<KEY, VALUE>() {}
        pair_t(KEY_REF k, const VALUE& v) : std::pair<KEY, VALUE>(KEY(k), v) {}
        pair_t(const pair_t& p) = default;
        KEY_REF name() const { return std::pair<KEY, VALUE>::first; }
        const VALUE& value() const { return std::pair<KEY, VALUE>::second; }
    };

private:
    struct KeyCmp {
        bool operator()(const pair_t& a, KEY_REF b) const { return CMP()(a.first, b); }
    };

    typedef std::vector<pair_t> container_t;
    container_t m_contents;

public:
    typedef pair_t value_type;
    typedef typename container_t::const_iterator const_iterator;

    const_iterator begin() const { return m_contents.cbegin(); }
    const_iterator end() const { return m_contents.cend(); }

    const_iterator findKey(KEY_REF key) const {
        // binary search for key
        const_iterator it(std::lower_bound(m_contents.begin(), m_contents.end(), key, KeyCmp()));
        if (it != m_contents.end() && CMP()(key, it->first))  // it > it-1, check ==
            it = m_contents.end();
        return it;
    }

    const_iterator findValue(const VALUE& value) const {
        for (auto&& it = m_contents.begin(); it != m_contents.end(); ++it) {
            if (it->second == value) return it;
        }
        return m_contents.end();
    }

    bool tryFind(KEY_REF key, const_iterator& returnValue) const {
        const_iterator it = findKey(key);
        if (it != m_contents.end()) {
            returnValue = it;
            return true;
        }
        return false;  // not found
    }

    bool containsKey(KEY_REF key) const { return findKey(key) != m_contents.end(); }

    bool containsValue(const VALUE& value) const { return findValue(value) != m_contents.end(); }

    size_t size() const { return m_contents.size(); }

    void insert(KEY_REF key, const VALUE& value) {
        erase(key);
        auto it = std::lower_bound(m_contents.begin(), m_contents.end(), key, KeyCmp());
        m_contents.insert(it, pair_t(key, value));
    }

    void insert(const_iterator it) { insert(it->first, it->second); }

    void erase(KEY_REF key) {
        const_iterator it = findKey(key);
        if (it != m_contents.end()) {
            m_contents.erase(it);
        }
    }

    const_iterator at(size_t index) const {
        const_iterator it(begin());
        std::advance(it, index);
        return it;
    }
};

}  // namespace detail
}  // namespace Meta

#endif  // META_DICTIONARY_HPP
