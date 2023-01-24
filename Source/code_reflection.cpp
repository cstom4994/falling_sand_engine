// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "code_reflection.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <string>

#include "core/core.hpp"
#include "core/debug_impl.hpp"

namespace reflect {

//--------------------------------------------------------
// A type descriptor for int
//--------------------------------------------------------

struct TypeDescriptor_Int : TypeDescriptor {
    TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int)} {}
    virtual void dump(const void *obj, int /* unused */) const override { std::cout << "int{" << *(const int *)obj << "}"; }
};

template <>
TypeDescriptor *getPrimitiveDescriptor<int>() {
    static TypeDescriptor_Int typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// A type descriptor for std::string
//--------------------------------------------------------

struct TypeDescriptor_StdString : TypeDescriptor {
    TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string)} {}
    virtual void dump(const void *obj, int /* unused */) const override { std::cout << "std::string{\"" << *(const std::string *)obj << "\"}"; }
};

template <>
TypeDescriptor *getPrimitiveDescriptor<std::string>() {
    static TypeDescriptor_StdString typeDesc;
    return &typeDesc;
}

}  // namespace reflect

// Here is acknowledgement
// https://stackoverflow.com/questions/41453/how-can-i-add-reflection-to-a-c-application/

struct Node {
    std::string key;
    int value;
    std::vector<Node> children;

    REFLECT_STRUCT()  // Enable reflection for this type
};

// Define Node's type descriptor
REFLECT_STRUCT_BEGIN(Node)
REFLECT_STRUCT_MEMBER(key)
REFLECT_STRUCT_MEMBER(value)
REFLECT_STRUCT_MEMBER(children)
REFLECT_STRUCT_END()

void TestRefleaction() {

    // Create an object of type Node
    Node node = {"apple", 3, {{"banana", 7, {}}, {"cherry", 11, {}}}};

    // Find Node's type descriptor
    reflect::TypeDescriptor *typeDesc = reflect::TypeResolver<Node>::get();

    // Dump a description of the Node object to the console
    typeDesc->dump(&node);
}

namespace IamAfuckingNamespace {
int func1(float f, char c) {
    std::cout << "func1(" << f << ", " << c << ")" << std::endl;
    return 42;
}

void func2(void) { std::cout << "func2()" << std::endl; }

void func_log_info(std::string info) { METADOT_INFO(info.c_str()); }
}  // namespace IamAfuckingNamespace

#define RETURNTYPE(fc) MetaEngine::return_type_t<decltype(&fc)>

auto fuckme() -> void {
    // std::map<std::string, std::function<double(double)>> func_map;

    Meta::AnyFunction f{&TestRefleaction};

    METADOT_INFO(f.get_result_type().info->name());

    f.invoke({});
}

void InitCppReflection() {
}

namespace MetaEngine {
auto tedtH() -> void {

    /*
    =================================================================
                                        F
                                       / \
         A                            H   \
        / \                          / \   \
       B   C                        I   J   G
      /   / \                        \ /   / \
     T   D   E                        K   L   Z
    ================================================================= */
    class A {};
    class F {};
    class B : public A {};
    class G : public F {};
    class C : public A {};
    class L : public G {};
    class T : public B {};
    class Z : public G {};
    class D : public C {};
    class H : public F {};
    class E : public C {};
    class I : public H {};
    class J : public H {};
    class K : public I, public J {};
    // =================================

    using namespace std;

    // using REGISTRY = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
    using REGISTRY = typelist<A, B, C, D, E, F, G, H, I, J, K, L, T>;
    using D_ANCESTORS = find_ancestors<REGISTRY, D>::type;
    using T_ANCESTORS = find_ancestors<REGISTRY, T>::type;
    using K_ANCESTORS = find_ancestors<REGISTRY, K>::type;
    using D_EXPECTED = typelist<A, C, D>;
    using K_EXPECTED = typelist<F, H, J, I, K>;

    static_assert(is_same<D_ANCESTORS, D_EXPECTED>::value, "Ancestor of D test failed");
    static_assert(is_same<K_ANCESTORS, K_EXPECTED>::value, "Ancestor of K test failed");

    D d_instance;
    printf("The hierarchy tree of class D is:\n");
    hierarchy_iterator<D_ANCESTORS>::exec(&d_instance);
    printf("\n\n");

    T t_instance;
    printf("The hierarchy tree of class T is:\n");
    hierarchy_iterator<T_ANCESTORS>::exec(&t_instance);
    printf("\n\n");

    K k_instance;
    printf("The hierarchy tree of class K is:\n");
    hierarchy_iterator<K_ANCESTORS>::exec(&k_instance);
    printf("\n\n");
}

}  // namespace MetaEngine