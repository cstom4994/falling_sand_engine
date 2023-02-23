
#include "property.hpp"

BinderBase::BinderBase() = default;
BinderBase::~BinderBase() {}

template <typename Origin, typename Target>
Binder<Origin, Target>::Binder(Property<Target> &property, const std::function<Target(const Origin &)> func) : property{&property}, func{func} {}

template <typename Origin, typename Target>
void Binder<Origin, Target>::update_value(const void *pData) {
    const Origin &value = *static_cast<const Origin *>(pData);
    property->set_value(func(value));
}

template <typename Origin, typename Target>
const Property<Target> &Binder<Origin, Target>::get_binder() const {
    return *property;
}

template class Binder<int, int>;

template <typename T>
Property<T>::Property(const T &value) : value{value}, args{nullptr} {}
template <typename T>
Property<T>::Property() : value{}, args{nullptr} {}

template <typename T>
const T &Property<T>::get_value() const {
    return value;
}

template <typename T>
void Property<T>::set_value(const T &value) {
    this->value = value;
    if (on_change) {
        on_change(value, args);
    }
    // update all
    for (auto p : binders) {
        p->update_value(&value);
    }
}

template <typename T>
void Property<T>::set_listener(const std::function<void(const T &value, void *)> &func, void *args) {
    on_change = func;
    this->args = args;
}

template <typename T>
template <typename Target>
void Property<T>::add_binded(Property<Target> &property, std::function<Target(const T &)> func) {
    binders.push_back(new Binder<T, Target>(property, func));
    property.set_value(func(get_value()));
}

template <typename T>
template <typename Target>
void Property<T>::remove_binded(Property<Target> &property) {
    BinderBase *pbb = nullptr;
    for (auto iter = binders.begin(); iter != binders.end();) {
        if (&dynamic_cast<Binder<T, Target> *>(*iter)->get_binder() == &property) {
            pbb = *iter;
            iter = binders.erase(iter);
            delete pbb;
        } else {
            ++iter;
        }
    }
}

template <typename T>
template <typename Origin>
void Property<T>::bind(Property<Origin> &property, std::function<T(const Origin &)> func) {
    property.add_binded(*this, func);
}

template <typename T>
template <typename Origin>
void Property<T>::unbind(Property<Origin> &property) {
    property.remove_binded(*this);
}

template <typename T>
Property<T>::~Property() {
    // clean all
    for (auto p : binders) {
        delete (p);
    }
}
// build template

#define BUILD_PROPERTY(t)                                                                              \
    template class Property<t>;                                                                        \
    template void Property<t>::add_binded<>(Property<t> & property, std::function<t(const t &)> func); \
    template void Property<t>::bind<>(Property<t> & property, std::function<t(const t &)> func);       \
    template void Property<t>::remove_binded<>(Property<t> & property);                                \
    template void Property<t>::unbind<>(Property<t> & property);

BUILD_PROPERTY(bool)
BUILD_PROPERTY(int)