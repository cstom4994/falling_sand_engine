

#ifndef META_USES_SERIALISE_HPP
#define META_USES_SERIALISE_HPP

#include "engine/meta/arrayproperty.hpp"
#include "engine/meta/class.hpp"

namespace Meta {
namespace archive {

/**
 For writing archive requires the following concepts:

    class Archive
    {
    public:
        NodeType beginChild(NodeType parent, const std::string& name);
        void setText(NodeType node, const std::string& text);
        bool isValid(NodeType node);
    };

 */
template <class ARCHIVE>
class ArchiveWriter {
public:
    using ArchiveType = ARCHIVE;
    using NodeType = typename ArchiveType::Node;

    ArchiveWriter(ArchiveType& archive) : m_archive(archive) {}

    void write(NodeType parent, const UserObject& object);

private:
    ArchiveType& m_archive;
};

/**
 For reading an archive requires the following concepts:

 class Archive
 {
 public:
     NodeType findArray(NodeType node, const std::string& name);
     NodeType arrayNextItem(NodeType node, const std::string& name);
     std::string getText(NodeType node)
     bool isValid(Node node);
 };

 */
template <class ARCHIVE>
class ArchiveReader {
public:
    using ArchiveType = ARCHIVE;
    using NodeType = typename ArchiveType::Node;
    using ArrayIterator = typename ArchiveType::ArrayIterator;

    ArchiveReader(ArchiveType& archive) : m_archive(archive) {}

    void read(NodeType node, const UserObject& object);

private:
    ArchiveType& m_archive;
};

}  // namespace archive
}  // namespace Meta

namespace Meta {
namespace archive {

template <class ARCHIVE>
void ArchiveWriter<ARCHIVE>::write(NodeType parent, const UserObject& object) {
    const Class& metaclass = object.getClass();
    for (size_t i = 0; i < metaclass.propertyCount(); ++i) {
        const Property& property = metaclass.property(i);

        // If the property has the exclude tag, ignore it
        //                if ((exclude != Value::nothing) && property.hasTag(exclude))
        //                    continue;

        if (property.kind() == ValueKind::User) {
            NodeType child = m_archive.beginChild(parent, property.name());

            // recurse
            write(child, property.get(object).to<UserObject>());

            m_archive.endChild(parent, child);
        } else if (property.kind() == ValueKind::Array) {
            auto const& arrayProperty = static_cast<const ArrayProperty&>(property);

            NodeType arrayNode = m_archive.beginArray(parent, property.name());

            // Iterate over the array elements
            const size_t count = arrayProperty.size(object);
            const std::string itemName("item");
            for (size_t j = 0; j < count; ++j) {
                if (arrayProperty.elementType() == ValueKind::User) {
                    write(arrayNode, arrayProperty.get(object, j).to<UserObject>());
                } else {
                    m_archive.setProperty(arrayNode, itemName, arrayProperty.get(object, j).to<std::string>());
                }
            }

            m_archive.endArray(parent, arrayNode);
        } else {
            m_archive.setProperty(parent, property.name(), property.get(object).to<std::string>());
        }
    }
}

template <class ARCHIVE>
void ArchiveReader<ARCHIVE>::read(NodeType node, const UserObject& object) {
    // Iterate over the object's properties using its metaclass
    const Class& metaclass = object.getClass();
    for (size_t i = 0; i < metaclass.propertyCount(); ++i) {
        const Property& property = metaclass.property(i);

        // If the property has the exclude tag, ignore it
        //                if ((exclude != Value::nothing) && property.hasTag(exclude))
        //                    continue;

        // Find the child node corresponding to the new property
        NodeType child = m_archive.findProperty(node, property.name());
        if (!m_archive.isValid(child)) continue;

        if (property.kind() == ValueKind::User) {
            // The current property is a composed type: deserialize it recursively
            read(child, property.get(object).to<UserObject>());
        } else if (property.kind() == ValueKind::Array) {
            auto const& arrayProperty = static_cast<const ArrayProperty&>(property);

            size_t index = 0;
            const std::string itemName("item");
            for (ArrayIterator it{m_archive.createArrayIterator(child, itemName)}; !it.isEnd(); it.next()) {
                // Make sure that there are enough elements in the array
                size_t count = arrayProperty.size(object);
                if (index >= count) {
                    if (!arrayProperty.dynamic()) break;
                    arrayProperty.resize(object, index + 1);
                }

                if (arrayProperty.elementType() == ValueKind::User) {
                    read(it.getItem(), arrayProperty.get(object, index).to<UserObject>());
                } else {
                    arrayProperty.set(object, index, m_archive.getValue(it.getItem()));
                }

                ++index;
            }
        } else {
            property.set(object, m_archive.getValue(child));
        }
    }
}

}  // namespace archive
}  // namespace Meta

#endif  // META_USES_SERIALISE_HPP
