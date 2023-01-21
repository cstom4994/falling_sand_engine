
#ifndef INC_CSINGLETON_H
#define INC_CSINGLETON_H

#include <memory>
#include <vector>

#include "core/debug_impl.hpp"

namespace MetaEngine {

template <class T>
class CSingleton {
public:
    virtual ~CSingleton() {}

    static T *GetSingletonPtr() {
        if (myInstance.get() == NULL) {
            std::shared_ptr<T> t(new T);
            myInstance = t;
        }

        return myInstance.get();
    }

    static T &GetSingleton() { return (*GetSingletonPtr()); }

    static void Delete() {
        std::shared_ptr<T> t;
        myInstance = t;
    }

protected:
    CSingleton() {}

    static std::shared_ptr<T> myInstance;
};

template <typename T>
std::shared_ptr<T> CSingleton<T>::myInstance;

//=============================================================================

template <typename T>
class CStaticSingleton {
public:
    virtual ~CStaticSingleton() {}

    static T *GetSingletonPtr() {
        static T *myInstance = 0;
        if (myInstance == 0) myInstance = new T;

        return myInstance;
    }

    static T &GetSingleton() { return (*GetSingletonPtr()); }

protected:
    CStaticSingleton() {}
};

template <typename T>
class CSingletonPtr {
public:
    CSingletonPtr() {}
    ~CSingletonPtr() {}

    static T *GetSingletonPtr() {
        if (myInstance.get() == NULL) {
            std::shared_ptr<T> t(::new T);
            myInstance = t;
        }

        return myInstance.get();
    }

    static T &GetSingleton() { return (*GetSingletonPtr()); }

    static void Delete() {
        std::shared_ptr<T> t;
        myInstance = t;
    }

    T *operator->() const { return GetSingletonPtr(); }

    T &operator*() const { return GetSingleton(); }

private:
    static std::shared_ptr<T> myInstance;
};

template <typename T>
std::shared_ptr<T> CSingletonPtr<T>::myInstance;

template <typename T>
inline T *GetSingletonPtr() {
    return CSingletonPtr<T>::GetSingletonPtr();
}

struct pointer_sorter {
    template <class T>
    bool operator()(const T *a, const T *b) const {
        if (a == 0) {
            return b != 0;
        } else if (b == 0) {
            return false;
        } else {
            return (*a) < (*b);
        }
    }

    template <class T>
    bool operator()(const T *a, const T *b) {
        if (a == 0) {
            return b != 0;
        } else if (b == 0) {
            return false;
        } else {
            return (*a) < (*b);
        }
    }
};

//=============================================================================

class CHandlePtr_DefaultBaseClass {
public:
    virtual ~CHandlePtr_DefaultBaseClass(){};
};

template <class TBaseClass>
class CDefaultHandleFactory {
public:
    virtual ~CDefaultHandleFactory() {}

    virtual TBaseClass *GetPointer(int handle) {
        METADOT_ASSERT_E(handle >= 0);
        METADOT_ASSERT_E(handle < (int)mGameObjects.size());
        return mGameObjects[handle];
    }

protected:
    std::vector<TBaseClass *> mGameObjects;
};

//=============================================================================

template <class T, class THandle = int, class TFactory = CDefaultHandleFactory<CHandlePtr_DefaultBaseClass>>
class CHandlePtr {
public:
    CHandlePtr() : mHandleManager(NULL), mHandle(THandle()) {}
    CHandlePtr(const CHandlePtr<T, THandle, TFactory> &other) : mHandleManager(NULL), mHandle(THandle()) { operator=(other); }

    ~CHandlePtr() { Free(); }

    //=========================================================================

    T &operator*() const { return (*Get()); }

    T *operator->() const { return Get(); }

    //.........................................................................

    bool operator==(T *ptr) const { return (ptr == Get()); }

    bool operator==(const CHandlePtr<T, THandle, TFactory> &other) const { return (Get() == other.Get()); }

    bool operator!=(T *ptr) const { return !operator==(ptr); }

    bool operator!=(const CHandlePtr<T, THandle, TFactory> &other) const { return !operator==(other); }

    //.........................................................................
    // checks against NULL

    bool operator==(int i) const {
        METADOT_ASSERT_E(i == 0);
        return IsNull();
    }

    bool operator!=(int i) const {
        METADOT_ASSERT_E(i == 0);
        return !IsNull();
    }

    operator bool() const { return !(IsNull()); }

    //.........................................................................

    bool operator<(T *ptr) const { return (Get() < ptr); }

    bool operator<(const CHandlePtr<T, THandle, TFactory> &other) const { return (Get() < other.Get()); }

    //.........................................................................

    CHandlePtr<T, THandle, TFactory> &operator=(const CHandlePtr<T, THandle, TFactory> &other) {
        if (*this == other) return *this;

        mHandleManager = other.mHandleManager;
        mHandle = other.mHandle;

        return *this;
    }

    //=========================================================================

    T *Get() const {
        // METADOT_ASSERT_E( impl );
        if (mHandleManager) {
            METADOT_ASSERT_E(dynamic_cast<T *>(mHandleManager->GetPointer(mHandle)));
            return dynamic_cast<T *>(mHandleManager->GetPointer(mHandle));
        } else {
            return 0;
        }
    }

    //=========================================================================

    bool IsNull() const { return (mHandleManager == NULL); }

    void Free() {
        mHandleManager = NULL;
        mHandle = THandle();
    }

    //=========================================================================

    void SetImpl(TFactory *handle_manager, THandle handle) {
        mHandleManager = handle_manager;
        mHandle = handle;
    }

    void SetHandleManager(TFactory *handle_manager) { mHandleManager = handle_manager; }

    void SetHandle(THandle handle) { mHandle = handle; }

    //=========================================================================

    TFactory *GetFactory() { return mHandleManager; }

    THandle GetHandle() const { return mHandle; }

    //=========================================================================

    template <class TypeCastTo>
    CHandlePtr<TypeCastTo, THandle, TFactory> CastTo() {
        // insures that we can cast to that type
        METADOT_ASSERT_E(dynamic_cast<TypeCastTo *>(Get()));

        CHandlePtr<TypeCastTo, THandle, TFactory> result;
        result.SetImpl(mHandleManager, mHandle);

        return result;
    }

    //=========================================================================
private:
    TFactory *mHandleManager;
    THandle mHandle;
};

//-----------------------------------------------------------------------------

}  // end of namespace MetaEngine

#endif
