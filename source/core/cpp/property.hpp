
#ifndef _METADOT_PROPERTY_HPP_
#define _METADOT_PROPERTY_HPP_

#include <functional>
#include <list>

template <typename T>
class Property;

class BinderBase {
public:
    BinderBase();
    virtual void update_value(const void *p) = 0;
    virtual ~BinderBase();
};

template <typename Origin, typename Target>
class Binder : public BinderBase {
public:
    Binder(Property<Target> &property, const std::function<Target(const Origin &)> func);
    void update_value(const void *pData) override;
    const Property<Target> &get_binder() const;

private:
    Property<Target> *property;
    std::function<Target(const Origin &)> func;
};

template <typename T>
class Property {
public:
    Property(const T &value);
    Property();
    Property(const Property &) = delete;
    Property &operator=(const Property &) = delete;
    ~Property();
    const T &get_value() const;
    void set_value(const T &value);
    void set_listener(const std::function<void(const T &value, void *)> &func, void *args);
    // bind to others, when other porperty change, this property can update
    template <typename Origin>
    void bind(Property<Origin> &property, std::function<T(const Origin &)>);
    template <typename Origin>
    void unbind(Property<Origin> &property);
    // bind to self, when the value of this object is change, it will update these property
    template <typename Target>
    void add_binded(Property<Target> &property, std::function<Target(const T &)> func);
    template <typename Target>
    void remove_binded(Property<Target> &property);

private:
    T value;
    std::function<void(const T &value, void *)> on_change;
    void *args;
    std::list<BinderBase *> binders;
};

#endif