

#include <cassert>
#include "engine/meta/args.hpp"
#include "engine/meta/arrayproperty.hpp"
#include "engine/meta/class.hpp"
#include "engine/meta/classcast.hpp"
#include "engine/meta/classvisitor.hpp"
#include "engine/meta/constructor.hpp"
#include "engine/meta/classmanager.hpp"
#include "engine/meta/dictionary.hpp"
#include "engine/meta/enummanager.hpp"
#include "engine/meta/observernotifier.hpp"
#include "engine/meta/util.hpp"
#include "engine/meta/enum.hpp"
#include "engine/meta/enumbuilder.hpp"
#include "engine/meta/enumobject.hpp"
#include "engine/meta/enumproperty.hpp"
#include "engine/meta/error.hpp"
#include "engine/meta/errors.hpp"
#include "engine/meta/function.hpp"
#include "engine/meta/observer.hpp"
#include "engine/meta/pondertype.hpp"
#include "engine/meta/property.hpp"
#include "engine/meta/simpleproperty.hpp"
#include "engine/meta/userdata.hpp"
#include "engine/meta/userobject.hpp"
#include "engine/meta/userproperty.hpp"
#include "engine/meta/value.hpp"

namespace Meta {

const Args Args::empty;

size_t Args::count() const { return m_values.size(); }

const Value& Args::operator[](size_t index) const {
    // Make sure that the index is not out of range
    if (index >= m_values.size()) META_ERROR(OutOfRange(index, m_values.size()));

    return m_values[index];
}

Args Args::operator+(const Value& arg) const {
    Args newArgs(*this);
    newArgs += arg;

    return newArgs;
}

Args& Args::operator+=(const Value& arg) {
    m_values.push_back(arg);

    return *this;
}

Args& Args::insert(size_t index, const Value& v) {
    m_values.insert(m_values.begin() + index, v);
    return *this;
}

ArrayProperty::ArrayProperty(IdRef name, ValueKind elementType, bool dynamic) : Property(name, ValueKind::Array), m_elementType(elementType), m_dynamic(dynamic) {}

ArrayProperty::~ArrayProperty() {}

ValueKind ArrayProperty::elementType() const { return m_elementType; }

bool ArrayProperty::dynamic() const { return m_dynamic; }

size_t ArrayProperty::size(const UserObject& object) const {
    // Check if the property is readable
    if (!isReadable()) META_ERROR(ForbiddenRead(name()));

    return getSize(object);
}

void ArrayProperty::resize(const UserObject& object, size_t newSize) const {
    // Check if the array is dynamic
    if (!dynamic()) META_ERROR(ForbiddenWrite(name()));

    // Check if the property is writable
    if (!isWritable()) META_ERROR(ForbiddenWrite(name()));

    setSize(object, newSize);
}

Value ArrayProperty::get(const UserObject& object, size_t index) const {
    // Check if the property is readable
    if (!isReadable()) META_ERROR(ForbiddenRead(name()));

    // Make sure that the index is not out of range
    const size_t range = size(object);
    if (index >= range) META_ERROR(OutOfRange(index, range));

    return getElement(object, index);
}

void ArrayProperty::set(const UserObject& object, size_t index, const Value& value) const {
    // Check if the property is writable
    if (!isWritable()) META_ERROR(ForbiddenWrite(name()));

    // Check if the index is in range
    const size_t range = size(object);
    if (index >= range) META_ERROR(OutOfRange(index, range));

    return setElement(object, index, value);
}

void ArrayProperty::insert(const UserObject& object, size_t before, const Value& value) const {
    // Check if the array is dynamic
    if (!dynamic()) META_ERROR(ForbiddenWrite(name()));

    // Check if the property is writable
    if (!isWritable()) META_ERROR(ForbiddenWrite(name()));

    // Check if the index is in range
    const size_t range = size(object) + 1;
    if (before >= range) META_ERROR(OutOfRange(before, range));

    return insertElement(object, before, value);
}

void ArrayProperty::remove(const UserObject& object, size_t index) const {
    // Check if the array is dynamic
    if (!dynamic()) META_ERROR(ForbiddenWrite(name()));

    // Check if the property is writable
    if (!isWritable()) META_ERROR(ForbiddenWrite(name()));

    // Check if the index is in range
    const size_t range = size(object);
    if (index >= range) META_ERROR(OutOfRange(index, range));

    return removeElement(object, index);
}

void ArrayProperty::accept(ClassVisitor& visitor) const { visitor.visit(*this); }

Value ArrayProperty::getValue(const UserObject& object) const {
    // Return first element
    return get(object, 0);
}

void ArrayProperty::setValue(const UserObject& object, const Value& value) const {
    // Set first element
    set(object, 0, value);
}

Class::Class(TypeId const& id, IdRef name) : m_sizeof(0), m_id(id), m_name(name) {}

IdReturn Class::name() const { return m_name; }

size_t Class::sizeOf() const { return m_sizeof; }

size_t Class::constructorCount() const { return m_constructors.size(); }

const Constructor* Class::constructor(size_t index) const { return m_constructors[index].get(); }

void Class::destruct(const UserObject& uobj, bool destruct) const { m_destructor(uobj, destruct); }

size_t Class::baseCount() const { return m_bases.size(); }

const Class& Class::base(size_t index) const {
    // Make sure that the index is not out of range
    if (index >= m_bases.size()) META_ERROR(OutOfRange(index, m_bases.size()));

    return *m_bases[index].base;
}

size_t Class::functionCount() const { return m_functions.size(); }

bool Class::hasFunction(IdRef id) const { return m_functions.containsKey(id); }

const Function& Class::function(size_t index) const {
    // Make sure that the index is not out of range
    if (index >= m_functions.size()) META_ERROR(OutOfRange(index, m_functions.size()));

    FunctionTable::const_iterator it = m_functions.begin();
    std::advance(it, index);

    return *it->second;
}

const Function& Class::function(IdRef id) const {
    FunctionTable::const_iterator it;
    if (!m_functions.tryFind(id, it)) {
        META_ERROR(FunctionNotFound(id, name()));
    }

    return *it->second;
}

size_t Class::propertyCount() const { return m_properties.size(); }

bool Class::hasProperty(IdRef id) const { return m_properties.containsKey(id); }

const Property& Class::property(size_t index) const {
    // Make sure that the index is not out of range
    if (index >= m_properties.size()) META_ERROR(OutOfRange(index, m_properties.size()));

    PropertyTable::const_iterator it = m_properties.begin();
    std::advance(it, index);

    return *it->second;
}

const Property& Class::property(IdRef id) const {
    PropertyTable::const_iterator it;
    if (!m_properties.tryFind(id, it)) {
        META_ERROR(PropertyNotFound(id, name()));
    }

    return *it->second;
}

void Class::visit(ClassVisitor& visitor) const {
    // First visit properties
    for (PropertyTable::pair_t const& prop : m_properties) {
        prop.value()->accept(visitor);
    }

    // Then visit functions
    for (FunctionTable::pair_t const& func : m_functions) {
        func.value()->accept(visitor);
    }
}

void* Class::applyOffset(void* pointer, const Class& target) const {
    // Special case for null pointers: don't apply offset to leave them null
    if (!pointer) return pointer;

    // Check target as a base class of this
    int offset = baseOffset(target);
    if (offset != -1) return static_cast<void*>(static_cast<char*>(pointer) + offset);

    // Check target as a derived class of this
    offset = target.baseOffset(*this);
    if (offset != -1) return static_cast<void*>(static_cast<char*>(pointer) - offset);

    // No match found, target is not a base class nor a derived class of this
    META_ERROR(ClassUnrelated(name(), target.name()));
}

bool Class::operator==(const Class& other) const { return m_id == other.m_id; }

bool Class::operator!=(const Class& other) const { return m_id != other.m_id; }

int Class::baseOffset(const Class& base) const {
    // Check self
    if (&base == this) return 0;

    // Search base in the base classes
    for (auto const& b : m_bases) {
        const int offset = b.base->baseOffset(base);
        if (offset != -1) return offset + b.offset;
    }

    return -1;
}

void* classCast(void* pointer, const Class& sourceClass, const Class& targetClass) { return sourceClass.applyOffset(pointer, targetClass); }

namespace detail {

ClassManager& ClassManager::instance() {
    static ClassManager cm;
    return cm;
}

Class& ClassManager::addClass(TypeId const& id, IdRef name) {
    // First make sure that the class doesn't already exist
    // Note, we check by id and name. Neither should be registered.
    if (classExists(id) || (!name.empty() && getByNameSafe(name) != nullptr)) {
        META_ERROR(ClassAlreadyCreated(name));
    }

    // Create the new class
    Class* newClass = new Class(id, name);

    // Insert it into the table
    m_classes.insert(std::make_pair(id, newClass));
    m_names.insert(std::make_pair(Id(name), newClass));

    // Notify observers
    notifyClassAdded(*newClass);

    // Done
    return *newClass;
}

void ClassManager::removeClass(TypeId const& id) {
    if (!classExists(id)) {
        META_ERROR(ClassNotFound("?"));
    }

    auto it = m_classes.find(id);
    Class* classPtr{it->second};
    auto itName = m_names.find(classPtr->m_name);

    // Notify observers
    notifyClassRemoved(*classPtr);

    m_names.erase(itName);
    delete classPtr;
    m_classes.erase(it);
}

size_t ClassManager::count() const { return m_classes.size(); }

ClassManager::ClassView ClassManager::getClasses() const { return ClassView(m_classes.begin(), m_classes.end()); }

const Class* ClassManager::getByIdSafe(TypeId const& id) const {
    ClassTable::const_iterator it{m_classes.find(id)};
    return (it == m_classes.end()) ? nullptr : it->second;
}

const Class& ClassManager::getById(TypeId const& id) const {
    const Class* cls{getByIdSafe(id)};
    if (!cls) META_ERROR(ClassNotFound("?"));

    return *cls;
}

const Class* ClassManager::getByNameSafe(const IdRef name) const {
    NameTable::const_iterator it{std::find_if(m_names.begin(), m_names.end(), [=](const std::pair<Id, Class*>& a) { return a.first.compare(name) == 0; })};
    return (it == m_names.end()) ? nullptr : it->second;
}

const Class& ClassManager::getByName(const IdRef name) const {
    const Class* cls{getByNameSafe(name)};
    if (!cls) META_ERROR(ClassNotFound(name));

    return *cls;
}

bool ClassManager::classExists(TypeId const& id) const { return m_classes.find(id) != m_classes.end(); }

ClassManager::ClassManager() {}

ClassManager::~ClassManager() {
    // Notify observers
    for (ClassTable::const_iterator it = m_classes.begin(); it != m_classes.end(); ++it) {
        Class* classPtr = it->second;
        notifyClassRemoved(*classPtr);
        delete classPtr;
    }
}

}  // namespace detail

ClassVisitor::~ClassVisitor() {
    // Nothing to do
}

void ClassVisitor::visit(const Property&) {
    // The default implementation does nothing
}

void ClassVisitor::visit(const SimpleProperty&) {
    // The default implementation does nothing
}

void ClassVisitor::visit(const ArrayProperty&) {
    // The default implementation does nothing
}

void ClassVisitor::visit(const EnumProperty&) {
    // The default implementation does nothing
}

void ClassVisitor::visit(const UserProperty&) {
    // The default implementation does nothing
}

void ClassVisitor::visit(const Function&) {
    // The default implementation does nothing
}

ClassVisitor::ClassVisitor() {
    // Nothing to do
}

Enum::Enum(IdRef name) : m_name(name) {}

IdReturn Enum::name() const { return m_name; }

size_t Enum::size() const { return m_enums.size(); }

Enum::Pair Enum::pair(size_t index) const {
    // Make sure that the index is not out of range
    if (index >= m_enums.size()) META_ERROR(OutOfRange(index, m_enums.size()));

    auto it = m_enums.at(index);
    return Pair(it->first, it->second);
}

bool Enum::hasName(IdRef name) const { return m_enums.containsKey(name); }

bool Enum::hasValue(EnumValue value) const { return m_enums.containsValue(value); }

IdReturn Enum::name(EnumValue value) const {
    auto it = m_enums.findValue(value);

    if (it == m_enums.end()) META_ERROR(EnumValueNotFound(value, name()));

    return it->first;
}

Enum::EnumValue Enum::value(IdRef name) const {
    auto it = m_enums.findKey(name);

    if (it == m_enums.end()) META_ERROR(EnumNameNotFound(name, m_name));

    return it->second;
}

bool Enum::operator==(const Enum& other) const { return name() == other.name(); }

bool Enum::operator!=(const Enum& other) const { return name() != other.name(); }

EnumBuilder::EnumBuilder(Enum& target) : m_target(&target) {}

EnumBuilder& EnumBuilder::value(IdRef name, Enum::EnumValue value) {
    assert(!m_target->hasName(name));
    assert(!m_target->hasValue(value));

    m_target->m_enums.insert(name, value);

    return *this;
}

namespace detail {

EnumManager& EnumManager::instance() {
    static EnumManager manager;
    return manager;
}

Enum& EnumManager::addClass(TypeId const& id, IdRef name) {
    // First make sure that the enum doesn't already exist
    if (enumExists(id) || (!name.empty() && getByNameSafe(name) != nullptr)) {
        META_ERROR(EnumAlreadyCreated(name));
    }

    // Create the new class
    Enum* newEnum = new Enum(name);

    // Insert it into the table
    m_enums.insert(std::make_pair(id, newEnum));
    m_names.insert(std::make_pair(name, newEnum));

    // Notify observers
    notifyEnumAdded(*newEnum);

    // Done
    return *newEnum;
}

void EnumManager::removeClass(TypeId const& id) {
    auto it = m_enums.find(id);
    if (it == m_enums.end()) return;  // META_ERROR(EnumNotFound(id));

    Enum* en{it->second};

    // Notify observers
    notifyEnumRemoved(*en);

    m_names.erase(en->name());
    delete en;
    m_enums.erase(id);
}

size_t EnumManager::count() const { return m_enums.size(); }

const Enum* EnumManager::getByIdSafe(TypeId const& id) const {
    const auto it = m_enums.find(id);
    return it == m_enums.end() ? nullptr : it->second;
}

const Enum& EnumManager::getById(TypeId const& id) const {
    const Enum* e{getByIdSafe(id)};
    if (!e) META_ERROR(EnumNotFound("?"));

    return *e;
}

const Enum* EnumManager::getByNameSafe(const IdRef name) const {
    const auto it{std::find_if(m_names.begin(), m_names.end(), [&name](const auto& a) { return a.first.compare(name.data()) == 0; })};

    return it == m_names.end() ? nullptr : it->second;
}

const Enum& EnumManager::getByName(const IdRef name) const {
    const Enum* e{getByNameSafe(name)};
    if (!e) META_ERROR(EnumNotFound(name));

    return *e;
}

bool EnumManager::enumExists(TypeId const& id) const { return m_enums.find(id) != m_enums.end(); }

EnumManager::EnumManager() {}

EnumManager::~EnumManager() {
    // Notify observers
    for (EnumTable::const_iterator it = m_enums.begin(); it != m_enums.end(); ++it) {
        Enum* enumPtr = it->second;
        notifyEnumRemoved(*enumPtr);
        delete enumPtr;
    }
}

}  // namespace detail

EnumProperty::EnumProperty(IdRef name, const Enum& propEnum) : Property(name, ValueKind::Enum), m_enum(&propEnum) {}

EnumProperty::~EnumProperty() {}

const Enum& EnumProperty::getEnum() const { return *m_enum; }

void EnumProperty::accept(ClassVisitor& visitor) const { visitor.visit(*this); }

Error::~Error() throw() {}

const char* Error::what() const throw() { return m_message.c_str(); }

const char* Error::where() const throw() { return m_location.c_str(); }

Error::Error(IdRef message) : m_message(message), m_location("") {}

BadType::BadType(ValueKind provided, ValueKind expected) : Error("value of type " + typeName(provided) + " couldn't be converted to type " + typeName(expected)) {}

BadType::BadType(const String& message) : Error(message) {}

Meta::String BadType::typeName(ValueKind type) { return detail::valueKindAsString(type); }

BadArgument::BadArgument(ValueKind provided, ValueKind expected, size_t index, IdRef functionName)
    : BadType("argument #" + str(index) + " of function " + String(functionName) + " couldn't be converted from type " + typeName(provided) + " to type " + typeName(expected)) {}

ClassAlreadyCreated::ClassAlreadyCreated(IdRef type) : Error("class named " + String(type) + " already exists") {}

ClassNotFound::ClassNotFound(IdRef name) : Error("the metaclass " + String(name) + " couldn't be found") {}

ClassUnrelated::ClassUnrelated(IdRef sourceClass, IdRef requestedClass)
    : Error("failed to convert from " + String(sourceClass) + " to " + String(requestedClass) + ": it is not a base nor a derived") {}

EnumAlreadyCreated::EnumAlreadyCreated(IdRef typeName) : Error("enum named " + String(typeName) + " already exists") {}

EnumNameNotFound::EnumNameNotFound(IdRef name, IdRef enumName) : Error("the value " + String(name) + " couldn't be found in metaenum " + String(enumName)) {}

EnumNotFound::EnumNotFound(IdRef name) : Error("the metaenum " + String(name) + " couldn't be found") {}

EnumValueNotFound::EnumValueNotFound(long value, IdRef enumName) : Error("the value " + str(value) + " couldn't be found in metaenum " + String(enumName)) {}

ForbiddenCall::ForbiddenCall(IdRef functionName) : Error("the function " + String(functionName) + " is not callable") {}

ForbiddenRead::ForbiddenRead(IdRef propertyName) : Error("the property " + String(propertyName) + " is not readable") {}

ForbiddenWrite::ForbiddenWrite(IdRef propertyName) : Error("the property " + String(propertyName) + " is not writable") {}

FunctionNotFound::FunctionNotFound(IdRef name, IdRef className) : Error("the function " + String(name) + " couldn't be found in metaclass " + String(className)) {}

NotEnoughArguments::NotEnoughArguments(IdRef functionName, size_t provided, size_t expected)
    : Error("not enough arguments for calling " + String(functionName) + " - provided " + str(provided) + ", expected " + str(expected)) {}

NullObject::NullObject(const Class* objectClass) : Error("trying to use a null metaobject of class " + (objectClass ? String(objectClass->name()) : String("unknown"))) {}

OutOfRange::OutOfRange(size_t index, size_t size) : Error("the index (" + str(index) + ") is out of the allowed range [0, " + str(size - 1) + "]") {}

PropertyNotFound::PropertyNotFound(IdRef name, IdRef className) : Error("the property " + String(name) + " couldn't be found in metaclass " + String(className)) {}

TypeAmbiguity::TypeAmbiguity(IdRef typeName) : Error("type " + String(typeName) + " ambiguity") {}

Function::~Function() {}

IdReturn Function::name() const { return m_name; }

ValueKind Function::returnType() const { return m_returnType; }

void Function::accept(ClassVisitor& visitor) const { visitor.visit(*this); }

Observer::~Observer() {
    // Nothing to do
}

void Observer::classAdded(const Class&) {
    // Default implementation does nothing
}

void Observer::classRemoved(const Class&) {
    // Default implementation does nothing
}

void Observer::enumAdded(const Enum&) {
    // Default implementation does nothing
}

void Observer::enumRemoved(const Enum&) {
    // Default implementation does nothing
}

Observer::Observer() {
    // Nothing to do
}

void addObserver(Observer* observer) {
    detail::ClassManager::instance().addObserver(observer);
    detail::EnumManager::instance().addObserver(observer);
}

void removeObserver(Observer* observer) {
    detail::ClassManager::instance().removeObserver(observer);
    detail::EnumManager::instance().removeObserver(observer);
}

namespace detail {

void ObserverNotifier::addObserver(Observer* observer) {
    assert(observer != nullptr);

    m_observers.insert(observer);
}

void ObserverNotifier::removeObserver(Observer* observer) { m_observers.erase(observer); }

ObserverNotifier::ObserverNotifier() {}

void ObserverNotifier::notifyClassAdded(const Class& theClass) {
    for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
        (*it)->classAdded(theClass);
    }
}

void ObserverNotifier::notifyClassRemoved(const Class& theClass) {
    for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
        (*it)->classRemoved(theClass);
    }
}

void ObserverNotifier::notifyEnumAdded(const Enum& theEnum) {
    for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
        (*it)->enumAdded(theEnum);
    }
}

void ObserverNotifier::notifyEnumRemoved(const Enum& theEnum) {
    for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
        (*it)->enumRemoved(theEnum);
    }
}

void ensureTypeRegistered(TypeId const& id, void (*registerFunc)()) {
    if (registerFunc && !ClassManager::instance().classExists(id) && !EnumManager::instance().enumExists(id)) {
        registerFunc();
    }
}

}  // namespace detail

Property::~Property() {}

IdReturn Property::name() const { return m_name; }

ValueKind Property::kind() const { return m_type; }

bool Property::isReadable() const { return true; }

bool Property::isWritable() const { return true; }

Value Property::get(const UserObject& object) const {
    // Check if the property is readable
    if (!isReadable()) META_ERROR(ForbiddenRead(name()));

    return getValue(object);
}

void Property::set(const UserObject& object, const Value& value) const {
    // Check if the property is writable
    if (!isWritable()) META_ERROR(ForbiddenWrite(name()));

    // Here we don't call setValue directly, we rather let the user object do it
    // and add any processing needed for proper propagation of the modification
    object.set(*this, value);
}

void Property::accept(ClassVisitor& visitor) const { visitor.visit(*this); }

Property::Property(IdRef name, ValueKind type) : m_name(name), m_type(type) {}

SimpleProperty::SimpleProperty(IdRef name, ValueKind type) : Property(name, type) {}

SimpleProperty::~SimpleProperty() {}

void SimpleProperty::accept(ClassVisitor& visitor) const { visitor.visit(*this); }

class TypeUserDataStore : public IUserDataStore {
    typedef const Type* key_t;
    typedef detail::Dictionary<Id, IdRef, Value> store_t;
    typedef std::map<key_t, store_t> class_store_t;
    class_store_t m_store;

public:
    void setValue(const Type& t, IdRef name, const Value& v) final;
    const Value* getValue(const Type& t, IdRef name) const final;
    void removeValue(const Type& t, IdRef name) final;
};

void TypeUserDataStore::setValue(const Type& t, IdRef name, const Value& v) {
    auto it = m_store.find(&t);
    if (it == m_store.end()) {
        auto ret = m_store.insert(class_store_t::value_type(&t, store_t()));
        it = ret.first;
    }
    it->second.insert(name, v);
}

const Value* TypeUserDataStore::getValue(const Type& t, IdRef name) const {
    auto it = m_store.find(&t);
    if (it != m_store.end()) {
        auto vit = it->second.findKey(name);
        if (vit != it->second.end()) return &vit->second;
    }
    return nullptr;
}

void TypeUserDataStore::removeValue(const Type& t, IdRef name) {
    auto it = m_store.find(&t);
    if (it != m_store.end()) {
        it->second.erase(name);
    }
}

std::unique_ptr<TypeUserDataStore> g_memberDataStore;

IUserDataStore* userDataStore() {
    auto p = g_memberDataStore.get();

    if (p == nullptr) {
        g_memberDataStore = detail::make_unique<TypeUserDataStore>();
        p = g_memberDataStore.get();
    }

    return p;
}

const Value Value::nothing;

Value::Value() : m_value(NoType()), m_type(ValueKind::None) {}

Value::Value(const Value& other) : m_value(other.m_value), m_type(other.m_type) {}

Value::Value(Value&& other) {
    std::swap(m_value, other.m_value);
    std::swap(m_type, other.m_type);
}

void Value::operator=(const Value& other) {
    m_value = other.m_value;
    m_type = other.m_type;
}

ValueKind Value::kind() const { return m_type; }

bool Value::operator==(const Value& other) const { return visit(detail::EqualVisitor(), other); }

bool Value::operator<(const Value& other) const { return visit(detail::LessThanVisitor(), other); }

std::istream& operator>>(std::istream& stream, Value& value) {
    // Use the string conversion
    //    Meta::Id str;
    //    stream >> str;
    //    value = str;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const Value& value) {
    // Use the string conversion
    return stream;  // << value.to<Meta::Id>();
}

UserProperty::UserProperty(IdRef name, const Class& propClass) : Property(name, ValueKind::User), m_class(&propClass) {}

UserProperty::~UserProperty() {}

const Class& UserProperty::getClass() const { return *m_class; }

void UserProperty::accept(ClassVisitor& visitor) const { visitor.visit(*this); }

const UserObject UserObject::nothing;

UserObject::UserObject() : m_class(nullptr), m_holder() {}

UserObject::UserObject(const UserObject& other) : m_class(other.m_class), m_holder(other.m_holder) {}

UserObject::UserObject(UserObject&& other) noexcept {
    std::swap(m_class, other.m_class);
    m_holder.swap(other.m_holder);
}

UserObject& UserObject::operator=(const UserObject& other) {
    m_class = other.m_class;
    m_holder = other.m_holder;
    return *this;
}

UserObject& UserObject::operator=(UserObject&& other) noexcept {
    std::swap(m_class, other.m_class);
    m_holder.swap(other.m_holder);
    return *this;
}

void* UserObject::pointer() const { return m_holder != nullptr ? m_holder->object() : nullptr; }

const Class& UserObject::getClass() const {
    if (m_class) {
        return *m_class;
    } else {
        META_ERROR(NullObject(m_class));
    }
}

Value UserObject::get(IdRef property) const { return getClass().property(property).get(*this); }

Value UserObject::get(size_t index) const { return getClass().property(index).get(*this); }

void UserObject::set(IdRef property, const Value& value) const { getClass().property(property).set(*this, value); }

void UserObject::set(size_t index, const Value& value) const { getClass().property(index).set(*this, value); }

bool UserObject::operator==(const UserObject& other) const {
    if (m_holder && other.m_holder) {
        return m_holder->object() == other.m_holder->object();
    } else if (!m_class && !other.m_class) {
        return true;  // both are UserObject::nothing
    }

    return false;
}

bool UserObject::operator<(const UserObject& other) const {
    if (m_holder) {
        if (other.m_holder) {
            return m_holder->object() < other.m_holder->object();
        }
    }
    assert(0);
    return false;
}

void UserObject::set(const Property& property, const Value& value) const {
    if (m_holder) {
        // Just forward to the property, no extra processing required
        property.setValue(*this, value);
    } else {
        // Error, null object
        META_ERROR(NullObject(m_class));
    }
}

long EnumObject::value() const { return m_value; }

IdReturn EnumObject::name() const { return m_enum->name(m_value); }

const Enum& EnumObject::getEnum() const { return *m_enum; }

bool EnumObject::operator==(const EnumObject& other) const { return (m_enum == other.m_enum) && (m_value == other.m_value); }

bool EnumObject::operator<(const EnumObject& other) const {
    if (m_enum != other.m_enum) {
        return m_enum < other.m_enum;
    } else {
        return m_value < other.m_value;
    }
}

}  // namespace Meta
