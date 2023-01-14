

#ifndef META_ARCHIVE_RAPIDXML_HPP
#define META_ARCHIVE_RAPIDXML_HPP

#include "engine/meta/string_view.hpp"
#include "libs/rapidxml/rapidxml.hpp"

namespace Meta {
namespace archive {

template <typename CH = char>
class RapidXmlArchive {
public:
    using ch_t = CH;
    using Node = rapidxml::xml_node<ch_t>*;

    //! Facilitate iteration over XML elements/arrays.
    class ArrayIterator {
        Node m_node{};
        detail::string_view m_name;

    public:
        ArrayIterator(Node arrayNode, detail::string_view name) : m_name(name) { m_node = arrayNode->first_node(name.data(), name.length()); }

        bool isEnd() const { return m_node == nullptr; }
        void next() { m_node = m_node->next_sibling(m_name.data(), m_name.length()); }
        Node getItem() { return m_node; }
    };

    // Write

    Node beginChild(Node parent, const std::string& name) {
        const char* nodeName = parent->document()->allocate_string(name.c_str(), name.length() + 1);
        Node child = parent->document()->allocate_node(rapidxml::node_element, nodeName);
        parent->append_node(child);
        return child;
    }

    void endChild(Node /*parent*/, Node /*child*/) {}

    void setProperty(Node parent, const std::string& name, const std::string& text) {
        const char* nodeName = parent->document()->allocate_string(name.c_str(), name.length() + 1);
        Node child = parent->document()->allocate_node(rapidxml::node_element, nodeName);
        parent->append_node(child);
        child->value(child->document()->allocate_string(text.c_str(), text.length() + 1));
    }

    Node beginArray(Node parent, const std::string& name) { return beginChild(parent, name); }

    void endArray(Node /*parent*/, Node /*child*/) {}

    // Read

    Node findProperty(Node node, const std::string& name) { return node->first_node(name.c_str(), name.length()); }

    ArrayIterator createArrayIterator(Node node, const std::string& name) { return ArrayIterator(node, detail::string_view(name.c_str(), name.length())); }

    detail::string_view getValue(Node node) { return detail::string_view(node->value(), node->value_size()); }

    bool isValid(Node node) { return node != nullptr; }
};

}  // namespace archive
}  // namespace Meta

#endif  // META_ARCHIVE_RAPIDXML_HPP
