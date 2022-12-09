// Copyright(c) 2022, KaoruXun All rights reserved.

#define REGISTER_REFLECTION

#include "Core/Core.hpp"
#include "Core/DebugImpl.hpp"

#include "CodeReflection.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <string>

//####################################################################################
//####################################################################################
//####################################################################################
//##    BEGIN REFLECT IMPLEMENTATION
//####################################################################################
//####################################################################################
//####################################################################################
#ifdef REGISTER_REFLECTION

// Gloabls
std::shared_ptr<SnReflect> g_reflect{nullptr};// Meta data singleton
Functions g_register_list{};                  // Keeps list of registration functions

// ########## General Registration ##########
// Initializes global reflection object, registers classes with reflection system
void InitializeReflection() {
    // Create Singleton
    g_reflect = std::make_shared<SnReflect>();

    // Register Structs / Classes
    for (int func = 0; func < g_register_list.size(); ++func) { g_register_list[func](); }
    g_register_list.clear();// Clean up
}

// Used in registration macros to automatically create nice display name from class / member variable names
void CreateTitle(std::string &name) {
    // Replace underscores, capitalize first letters
    std::replace(name.begin(), name.end(), '_', ' ');
    name[0] = toupper(name[0]);
    for (int c = 1; c < name.length(); c++) {
        if (name[c - 1] == ' ') name[c] = toupper(name[c]);
    }

    // Add spaces to seperate words
    std::string title = "";
    title += name[0];
    for (int c = 1; c < name.length(); c++) {
        if (islower(name[c - 1]) && isupper(name[c])) {
            title += std::string(" ");
        } else if ((isalpha(name[c - 1]) && isdigit(name[c]))) {
            title += std::string(" ");
        }
        title += name[c];
    }
    name = title;
}

// ########## Class / Member Registration ##########
// Update class TypeData
void RegisterClass(TypeData class_data) { g_reflect->AddClass(class_data); }

// Update member TypeData
void RegisterMember(TypeData class_data, TypeData member_data) {
    g_reflect->AddMember(class_data, member_data);
}

//####################################################################################
//##    TypeData Fetching
//####################################################################################
// ########## Class Data Fetching ##########
// Class TypeData fetching from passed in class TypeHash
TypeData &ClassData(TypeHash class_hash) {
    for (auto &pair: g_reflect->classes) {
        if (pair.first == class_hash) return pair.second;
    }
    return unknown_type;
}
// Class TypeData fetching from passed in class name
TypeData &ClassData(std::string class_name) {
    for (auto &pair: g_reflect->classes) {
        if (pair.second.name == class_name) return pair.second;
    }
    return unknown_type;
}
// Class TypeData fetching from passed in class name
TypeData &ClassData(const char *class_name) { return ClassData(std::string(class_name)); }

// ########## Member Data Fetching ##########
// Member TypeData fetching by member variable index and class TypeHash
TypeData &MemberData(TypeHash class_hash, int member_index) {
    int count = 0;
    for (auto &member: g_reflect->members[class_hash]) {
        if (count == member_index) return member.second;
        ++count;
    }
    return unknown_type;
}
// Member TypeData fetching by member variable name and class TypeHash
TypeData &MemberData(TypeHash class_hash, std::string member_name) {
    for (auto &member: g_reflect->members[class_hash]) {
        if (member.second.name == member_name) return member.second;
    }
    return unknown_type;
}

//####################################################################################
//##    Meta Data (User Info)
//####################################################################################
void SetMetaData(TypeData &type_data, int key, std::string data) {
    if (type_data.type_hash != 0) type_data.meta_int_map[key] = data;
}
void SetMetaData(TypeData &type_data, std::string key, std::string data) {
    if (type_data.type_hash != 0) type_data.meta_string_map[key] = data;
}
std::string GetMetaData(TypeData &type_data, int key) {
    if (type_data.type_hash != 0) {
        if (type_data.meta_int_map.find(key) != type_data.meta_int_map.end())
            return type_data.meta_int_map[key];
    }
    return "";
}
std::string GetMetaData(TypeData &type_data, std::string key) {
    if (type_data.type_hash != 0) {
        if (type_data.meta_string_map.find(key) != type_data.meta_string_map.end())
            return type_data.meta_string_map[key];
    }
    return "";
}

#endif// REGISTER_REFLECTION

namespace reflect {

    //--------------------------------------------------------
    // A type descriptor for int
    //--------------------------------------------------------

    struct TypeDescriptor_Int : TypeDescriptor
    {
        TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int)} {}
        virtual void dump(const void *obj, int /* unused */) const override {
            std::cout << "int{" << *(const int *) obj << "}";
        }
    };

    template<>
    TypeDescriptor *getPrimitiveDescriptor<int>() {
        static TypeDescriptor_Int typeDesc;
        return &typeDesc;
    }

    //--------------------------------------------------------
    // A type descriptor for std::string
    //--------------------------------------------------------

    struct TypeDescriptor_StdString : TypeDescriptor
    {
        TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string)} {}
        virtual void dump(const void *obj, int /* unused */) const override {
            std::cout << "std::string{\"" << *(const std::string *) obj << "\"}";
        }
    };

    template<>
    TypeDescriptor *getPrimitiveDescriptor<std::string>() {
        static TypeDescriptor_StdString typeDesc;
        return &typeDesc;
    }

}// namespace reflect

//
// Here is acknowledgement
// https://stackoverflow.com/questions/41453/how-can-i-add-reflection-to-a-c-application/
//

struct Point
{
    int x, y;
};

struct Rect
{
    Point p1, p2;
    uint32_t color;
};

REFL(Point, FIELD(Point, x), FIELD(Point, y));
REFL(Rect, FIELD(Rect, p1), FIELD(Rect, p2), FIELD(Rect, color));

//####################################################################################
//##    Component: Transform2D
//##        Sample component used to descibe a location of an object
//############################
struct Transform2D
{
    int width;
    int height;
    std::vector<double> position;
    std::vector<double> rotation;
    std::vector<double> scale;
    std::string text;

    REFLECT();
};

//####################################################################################
//##    Register Reflection / Meta Data
//############################
#ifdef REGISTER_REFLECTION
REFLECT_CLASS(Transform2D)
CLASS_META_DATA(META_DATA_DESCRIPTION, "Describes the location and positioning of an object.")
REFLECT_MEMBER(width)
REFLECT_MEMBER(height)
REFLECT_MEMBER(position)
MEMBER_META_TITLE("Object Position")
MEMBER_META_DATA(META_DATA_DESCRIPTION, "Location of an object in space.")
REFLECT_MEMBER(rotation)
REFLECT_MEMBER(scale)
REFLECT_MEMBER(text)
REFLECT_END(Transform2D)
#endif

struct Node
{
    std::string key;
    int value;
    std::vector<Node> children;

    REFLECT_STRUCT()// Enable reflection for this type
};

// Define Node's type descriptor
REFLECT_STRUCT_BEGIN(Node)
REFLECT_STRUCT_MEMBER(key)
REFLECT_STRUCT_MEMBER(value)
REFLECT_STRUCT_MEMBER(children)
REFLECT_STRUCT_END()

void TestRefleaction() {

    // ########## Turn on reflection, register classes / member variables
    InitializeReflection();

    // ########## Create class instance
    Transform2D t{};
    t.width = 10;
    t.height = 20;
    t.position = std::vector<double>({1.0, 2.0, 3.0});
    t.rotation = std::vector<double>({4.0, 5.0, 6.0});
    t.scale = std::vector<double>({7.0, 8.0, 9.0});
    t.text = "hello world!";

    // ########## Store TypeHash for later
    TypeHash t_type_hash = TypeHashID<Transform2D>();

    // ########## EXAMPLE: Get class TypeData by class type / instance / type hash / name
    std::cout << "Class Data by Type     - Name:     " << ClassData<Transform2D>().name
              << std::endl;
    std::cout << "Class Data by Instance - Members:  " << ClassData(t).member_count << std::endl;
    std::cout << "Class Data by TypeHash - Title:    " << ClassData(t_type_hash).title << std::endl;
    std::cout << "Class Data by Name     - TypeHash: " << ClassData("Transform2D").type_hash
              << std::endl;

    // ########## EXAMPLE: Get member TypeData by member variable index / name
    std::cout << "By Class Type, Member Index:       " << MemberData<Transform2D>(t, 2).name
              << std::endl;
    std::cout << "By Class Type, Member Name:        " << MemberData<Transform2D>("position").index
              << std::endl;
    std::cout << "By Class Instance, Member Index:   " << MemberData(t, 2).name << std::endl;
    std::cout << "By Class Instance, Member Name:    " << MemberData(t, "position").index
              << std::endl;
    std::cout << "By Class TypeHash, Member Index:   " << MemberData(t_type_hash, 2).name
              << std::endl;
    std::cout << "By Class TypeHash, Member Name:    " << MemberData(t_type_hash, "position").index
              << std::endl;

    // ########## EXAMPLE: Meta Data
    // Class meta data
    std::string description{};
    description = GetMetaData(ClassData<Transform2D>(), META_DATA_DESCRIPTION);
    std::cout << "Class Meta Data -  Description: " << description << std::endl;

    // Member meta data
    description = GetMetaData(MemberData<Transform2D>("position"), META_DATA_DESCRIPTION);
    std::cout << "Member Meta Data - Description: " << description << std::endl;

    // ########## Get Values
    std::cout << "Transform2D instance 't' member variable values:" << std::endl;

    // EXAMPLE: Return member variable by class instance, member variable index
    TypeData member = MemberData(t, 0);
    if (member.type_hash == TypeHashID<int>()) {
        int &width = ClassMember<int>(&t, member);
        std::cout << "  " << member.title << ": " << width << std::endl;
    }

    // EXAMPLE: Return member variable by class instance, member variable name
    member = MemberData(t, "position");
    if (member.type_hash == TypeHashID<std::vector<double>>()) {
        std::vector<double> &position = ClassMember<std::vector<double>>(&t, member);
        std::cout << "  " << MemberData(t, "position").title << " X: " << position[0] << std::endl;
        std::cout << "  " << MemberData(t, "position").title << " Y: " << position[1] << std::endl;
        std::cout << "  " << MemberData(t, "position").title << " Z: " << position[2] << std::endl;
    }

    // EXAMPLE: Return member variable by void* class, class type hash, and member variable name
    member = MemberData(t_type_hash, "text");
    if (member.type_hash == TypeHashID<std::string>()) {
        std::string &txt = ClassMember<std::string>(&t, member);
        std::cout << "  " << MemberData(t_type_hash, "text").title << ": " << txt << std::endl;
    }

    // ########## EXAMPLE: Iterating Members
    std::cout << "Iterating Members (member count: " << ClassData("Transform2D").member_count
              << "): " << std::endl;
    for (int p = 0; p < ClassData("Transform2D").member_count; ++p) {
        std::cout << "  Member Index: " << p << ", Name: " << MemberData(t, p).name
                  << ", Value(s): ";
        member = MemberData(t, p);
        if (member.type_hash == TypeHashID<int>()) {
            std::cout << ClassMember<int>(&t, member);
        } else if (member.type_hash == TypeHashID<std::vector<double>>()) {
            std::vector<double> v = ClassMember<std::vector<double>>(&t, member);
            for (size_t c = 0; c < v.size(); c++) std::cout << v[c] << ", ";
        } else if (member.type_hash == TypeHashID<std::string>()) {
            std::cout << ClassMember<std::string>(&t, member);
        }
        std::cout << std::endl;
    }

    // ########## EXAMPLE: SetValue by Name (can also be called by class type / member variable index, etc...)
    member = MemberData(t, "position");
    if (member.type_hash == TypeHashID<std::vector<double>>()) {
        ClassMember<std::vector<double>>(&t, member) = {56.0, 58.5, 60.2};
        std::cout << "After calling SetValue on 'position':" << std::endl;
        std::cout << "  " << MemberData(t, "position").title
                  << " X: " << ClassMember<std::vector<double>>(&t, member)[0] << std::endl;
        std::cout << "  " << MemberData(t, "position").title
                  << " Y: " << ClassMember<std::vector<double>>(&t, member)[1] << std::endl;
        std::cout << "  " << MemberData(t, "position").title
                  << " Z: " << ClassMember<std::vector<double>>(&t, member)[2] << std::endl;
    }

    // ########## EXAMPLE: GetValue from unknown class types
    //
    //  If using with an entity component system, it's possible you may not have access to class type at runtime. Often a
    //  collection of components are stored in a container of void pointers. Somewhere in your code when your class is initialized,
    //  store the component class TypeHash:
    //
    TypeHash saved_hash = ClassData(t).type_hash;
    void *component_ptr = (void *) (&t);
    //
    //  Later (if your components are stored as void pointers in an array / vector / etc. with other components) you may still
    //  access the member variables of the component back to the original type. This is done by using the saved_hash from earlier:
    //
    std::cout << "Getting member variable value from unknown class type:" << std::endl;
    member = MemberData(saved_hash, 3);
    if (member.type_hash == TypeHashID<std::vector<double>>()) {
        std::vector<double> &rotation = ClassMember<std::vector<double>>(component_ptr, member);
        std::cout << "  Rotation X: " << rotation[0] << ", Rotation Y: " << rotation[1]
                  << ", Rotation Z: " << rotation[2] << std::endl;
    }

    // ########## END DEMO
}

namespace IamAfuckingNamespace {
    int func1(float f, char c) {
        std::cout << "func1(" << f << ", " << c << ")" << std::endl;
        return 42;
    }

    void func2(void) { std::cout << "func2()" << std::endl; }

    void func_log_info(std::string info) { METADOT_INFO(info.c_str()); }
}// namespace IamAfuckingNamespace

#define RETURNTYPE(fc) MetaEngine::return_type_t<decltype(&fc)>

auto fuckme() -> void {
    //std::map<std::string, std::function<double(double)>> func_map;

    Meta::any_function f{&TestRefleaction};

    METADOT_INFO(f.get_result_type().info->name());

    f.invoke({});

    Rect rect{
            {0, 0},
            {8, 9},
            123353,
    };

    metadot_struct_recur_obj(
            rect,
            [](const char *name, int depth) {
                std::cout << "field name:" << name << std::endl;
                return;
            },
            "", 0);

    // Create an object of type Node
    Node node = {"apple", 3, {{"banana", 7, {}}, {"cherry", 11, {}}}};

    // Find Node's type descriptor
    reflect::TypeDescriptor *typeDesc = reflect::TypeResolver<Node>::get();

    // Dump a description of the Node object to the console
    typeDesc->dump(&node);
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

        //using REGISTRY = typelist<I, C, Z, G, D, F, L, C, I, A, T, B, J, K, H, E, E>;
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

}// namespace MetaEngine