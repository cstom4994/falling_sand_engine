/* *Copyright (c) 2019 LIONant*
## :warning: About the examples and tests
> Please note that the examples showcases how to add properties to your classes. However at the 
bottom of the documentation there is a test which is used for all the examples 
(Take me there [Test Function](#TestFunction)).
The test consist of creating two instances of the given example class. Then it will 
serialize-out one of the instances into a std::vector and from there it will serialize-into 
the other instance. Finally it will check that the new instance have all the right values. 
Additionally to this, the test will print out all the properties of the given example.
--------------
<img src="https://i.imgur.com/GfJb3sQ.jpg" align="right" width="150px" />

# Documentation & Examples

### *Quick note before we start...*
This document file contains both the documentation and the code needed to test the property
system. The comments in the code may look a little bit strange since I am using markup language
in order to format the document.

I will be using game engines as a reference for the examples and explanations however the 
use the property systems goes well beyond games.

I would advice to skip the API section at the begging and focus on the examples first.
The examples will tell you which part of the API is been used. At that point to can check
the API. I recommend this way because in the API sections contains all the information
which may be a bit too much at the start.

## *What is a property system?*
A **property system** also offend call a **reflection system** is a coding methodology or
language feature which allows exposing certain information about your code to other systems.
C++ does not provide a language based reflection system and this one of the reasons why
this project exists. Property systems are used heavily in game engines, tools, and applications.
I would even say that they have become essential to build modern applications. 

Properties usually are member variables of classes or functions. By generalizing your data as properties
any system can operate on them. Because of this it allows to generalize/refactor-out more code.
This leads to less and simpler code.

For reference here are some interesting links to explore:

1. https://www.unrealengine.com/en-US/blog/unreal-property-system-reflection
2. TODO: Add more references

## *Why is a property system so useful?*
The main power relays in its ability to refactor the code into generic systems which otherwise 
need to be special case per each object. The typical example is a game component.

Without a property system each game component is likely to have the following functions:
- Serialize out the component
- Serialize in the component
- Display data of the component into an editor
- Copy component
- etc...

With this property system, you won't need any of those functions per component. Father more 
it will enable other functionality which other wise will be impossible. (Such prefabs/blueprints)

Here are some examples of property panels found in popular editors.
![Examples of Property Editors](https://i.imgur.com/oYCzL71.jpg "Examples of Property Editors")

# Introduction to LIONant's property system
There are a few ways to add properties to your classes.
1. Deriving your class from `property::base` this is the Object Oriented way.
2. By using a non-intrusive opt-in.
3. By mixing case 1 and case 2 (hybrid)

### <a name="UsingPropertyBase"></a> Using *property::base*
`property::base` is an pure abstract class. Why would you want to derive from this base class? 
The main benefit of doing so is that you can use a pointer to `property::base` to access the 
property system of any class. This turns out to very really useful. Especially if your system is 
object-oriented. If you look at the actual code for this base object it looks like this:
```cpp
struct base
{
    virtual const property::table& getPropertyVTable( void ) const noexcept = 0;
};
```
Note that is a pure abstract function which means we must override it if we want to use it. 
We will cover how we do this in detail later.

### *Using Opt-In*
This method is faster because it does not deal with virtual functions, however, 
you must know always which type you are dealing with. Which is not always the case on 
traditional object-oriented programming. This method is usually used for ECS (Entity Component Systems).
Which you always have an indirect class that knows all the types.

## <a name="PropertyConfigFile"></a> *PropertyConfig File*
At this point you may be asking yourself, how many types of properties can I have? 
Is there any limits?

This questions is answer by the `PropertyConfig.h` file.
This file is meant for the user to override/replace with any specifics that he/she desires.
It is the config file the property system. Inside this file you will find:

```cpp
using settings_data_variant = std::variant
<
      int
    , bool
    , float
    , string_t
    , oobb
>;
```

This std::variant has all the types which are allowed by our property system. You will see this oobb 
been define here as well. It is a made up stupid structure which you can delete. It is meant to serve 
as an example. Note that this variant must be declared inside `namespace property`

There is one more thing that is defined in this file:
```cpp
struct settings_user_entry
{
    // A simple string describing to the game editor's user what this property does
    const char* m_pHelp { nullptr };

    // Function for user to setup a help string for their properties
    // We use a template here to ask the compiler to resolve this function later
    // Thanks to that we can make this function a constexpr function
    template< typename T = property::setup_entry >
    constexpr  T   Help ( const char* pHelp ) const noexcept
    {
        // Because we are a constexpr function we can not modify the class directly we must copy it
        T r = *static_cast<const T*>(this);

        // Now we can modify the variable that we care about
        r.m_pHelp = pHelp;

        // We return our new instance 
        return std::move(r);
    }
};
```
This structure allows us to add user define variables to our properties. 
In this example, we show how you can add a help string to each property. Note that we add 
a Help setter function which accepts our string and the assigns it. Note that the function is 'constexpr'.
To accomplish this feat we must convince the compiler to help us. This means we must use this 
special template formulation which forces the compiler to resolve the function only when is actually 
used. If you need to add more fields to your properties follow this pattern. If you are not interested 
in any user-defined data per property you can leave this structure empty.

## <a name="PropertyTypes"></a> Property types
There are two kinds of properties. Atomic properties which are provided by the user in the 
[Property Config File](#PropertyConfigFile) and lists. A list of a type of property which contains
other properties. These properties are address by an Index. This Index is a uint64_t. It can contain
any data which allows the property list to find the specify property. Please look at [Example5](#Example5)
for an example. LIONant property system supports two types of list natively. std::arrays and std::vectors.
However std::array usually have two meanings:

1. It is a complete set of data (all entries in the array are meaningful and always used). This is how 
this system is assuming the std::array operates. 
2. It is an incomplete set of data, which means that not all the entries in the std::array are valid and it 
  acts more like a buffer where somewhere else there is a variable which contains the real count. This is more
  commonly associated with a std::vector. Do not use std::arrays like this for this system. If you need something
  like this created your own std::array type by deriving from it and creating your own list.

You can create your own type of lists. This is done with [`property_list_fnbegin`](#property_list_fnbegin), 
[`property_list_fnenum`](#property_list_fnenum), and [`property_list_fnend`](#property_list_fnend) you can
also check [Example0 Custom Lists](#Example0CustomLists). This function will allow to create a unique type of
list per property block. However if what you want to do is to build a completely generic list just the same
way that LIONand property system defined std::vector and std::array you can do that as well. Please check
[Example2 Custom Lists](#Example2CustomLists). In this example we make std::map into another list that the
system knows about.

--------------
--------------
--------------
# API
--------------

Most of the API is done with macros. Why? Because it simplifies the API for the user 
which in turn removes potential bugs and unnecessary code. It also allows me to update the 
system without having the change the user code, The way it is done does not prevent any debugging 
from happening. We will try to keep the API as clean and efficient as possible.

## property_vtable()
*Declares the virtual function for classes with property::base*

Creates the virtual function definition which will override the property::base class virtual function.
See: [Using property::base](#UsingPropertyBase)
While this is a bit cumbersome it allows the properties to be compiled using constexpr
since C++17 does not handle virtual function been constexpr (expected in C++20). 
This macro should be place inside any class with a 'property::base' derived class.

## <a name="property_begin"></a> property_begin( TYPE_WITH_PROPERTIES, ... )
*begins property block*

This macro starts the main property block where all the properties will be listed.
The block like any other C++ block requires a C++ scope `{ }`
Inside that scope we will be adding the properties. Note that this block must end with either 
[`property_end()`](#PropertyEnd) or with [`property_vend()`](#property_vend)
<br>
*Macro Parameters:*
* First parameter is the class type which we are creating the property block for.
Note that the system will use the type name to create the scope name.

*optional parameters:*
* **name( STRING_SCOPE_NAME )** this sub-macro can be used to rename the property scope.
Example: *name("test")*

See: [Example1](#Example1), [`property_end()`](#PropertyEnd), [`property_vend()`](#property_vend)

## <a name="property_vend"></a> property_vend_h( TYPE_WITH_PROPERTIES ) && property_vend_cpp( TYPE_WITH_PROPERTIES )
*Ends property block for classes with property::base*

This macro will end the main property block which was started with [`property_begin(...)`](#property_begin). 
Behinds the scene this macro will implement the actual virtual function that overrides [property::base](#UsingPropertyBase).
Note that there is a different macro which is very similar named [`property_end()`](#PropertyEnd) the difference is
that (_vend_h/_vend_cpp) is for classes that are derived from [property::base](#UsingPropertyBase) while the other macro (_end)
is for opt-in properties. The difference between (_vend_h/_vend_cpp) is that one 
is meant to use in the header files the other is meant to be use in cpp files.

*Macro Parameters:*
* The parameter should be the class type which we are creating the property block for.

See: [Example0](#Example0), [`property_begin(...)`](#property_begin), [`property_end()`](#PropertyEnd)

## property_var( VARIABLE, ... )
*Defines a property*

This macro is used to list the property variables that we want the system to know about.
Note that the name of the variable will be use as the default name of the property. For variables
which are formated as `m_Speed` the `m_` will be eliminated from the name.

*Macro Parameters:*
* The actual variable from the class.

*optional parameters:*
* **name( STRING_SCOPE_NAME )** this sub-macro can be used to rename the property.

See: [Example0](#Example0), [Property Extended Info](#PropertyExtendedInfo)

## <a name="PropertyExtendedInfo"></a> property_var()... *extended info* 
*User setters for their extensions*

Please note as well that properties can have extended information. 
See [Example1](#Example1), 
<br>
`property_var( m_Bool   ).Help( "I am..." )`
<br>
`.Help` is a user define setup function. Please check [PropertyConfig File](#PropertyConfigFile) 
for more information. Extended Information function are suppose to be able to chain them together. Example: 
<br>
`property_var( m_Bool   ).Help( "I am..." ).Disable( ... )`
<br>
The property system provides a single extended information function for the properties:

### `Disable( property::function_ptr_dynamic_flags )` 
*Only system setter for extended info*

This function is use to dynamically disabling properties. This is useful to control 
when certain properties are relevant and when they are not.

*Parameters:*
<br>
This function takes a lambda with the following signature:
<br>
`using function_ptr_dynamic_flags = bool(*)( const std::byte& Self ) noexcept;`

See: [Example8](#Example8) 

## <a name="property_var_fnbegin"></a> property_var_fnbegin( PROPERTY_NAME, PROPERTY_TYPE )

*Starts the block of a function base property*

This macro represents a block which requires a C++ scope `{}`. This block is an actual lambda
function. Inside this function we can have the following Variables available: 

1. Self   - ref to class (so that we can access our variables)
2. InOut  - ref to an reference variable of the type given in PROPERTY_TYPE. is used to Read/get or Write/set 
            data in and out of the class
3. isRead - is a variable which will tell us what the system is trying to do, Read/get or Write/set
4. Index  - We also have a 64bit unsigned integer for lists types, this is handy for lists.

Note that after we close the c++ block we must also use [`property_var_fnend()`](#property_var_fnend).
This is the standard convention for the API. All starts have and end.

Because we are a function we can also return at any point. However this lambda returns a boolean.
This boolean is used to indicate if the Read or Write was successful. A typical case to return
early is if we are dealing with list which base on the Index we are require to find an entry. 
If we don't find that entry we must `return false;` If we don't return anything it will default to 
returning true.

*Macro Parameters:*
* The first parameter is the name of the property. Since we do not have a variable associated with 
it we must enter a name with quotes. 
* The second parameter we must specify the type which this property is related to.
Even if this function is going to do nothing related with that type still are require to associated 
with a type. The Type has to be one of the types known by the system given in 
`settings_data_variant` in the [PropertyConfig File](#PropertyConfigFile) file. 

See: [Example2](#Example2), [`property_var_fnend`](#property_var_fnend)

## <a name="property_var_fnend"></a> property_var_fnend()

*Ends the block of a function base property*

This macro end the lambda that was started with [`property_var_fnbegin`](#property_var_fnbegin).
Because still a property (`property_var`) you can still use any of the 
[Property Extended Info](#PropertyExtendedInfo).

See: [Example2](#Example2), [property_var_fnbegin](#property_var_fnbegin), [`property_var_fnbegin`](#property_var_fnbegin)

## property_parent( PARENT_TYPE )
*Lets the property system know about a parent class*

This macro lets the property system know that the parents of this class. Note that the only parents
we care about are parents which themselves have also properties. Otherwise we really don't care
how many parents it has. This enables the property system to link all the properties together.
Also please note that you can only have one parameter. Which means if you have multiple parents 
you will need to have multiple instances of this macro. See [Example3](#Example3).

The way LIONant's property system thinks is in terms of paths to properties. For example to reach 
property in side a class name `others` from [Example2](#Example2) the full path for it will be as 
follows "example2/others". If we want to reach our parent properties inside example1 we will think 
in terms of "example2/example1/SomeInt". So first us (example2) then our parents (example1) then 
the actual property. 

*Macro Parameters:*
* The parameter should be the parent's class type.

See: [Example2](#Example2), [Example3](#Example3)

## <a name="property_scope_begin"></a> property_scope_begin( SCOPE_NAME )

*Begins a scope block*

A scope is just a group of properties. This macro is a block which requires a C++ scope `{}`. 
Because it has a begin it also means it must have a [property_scope_end](#property_scope_end)
The scope has very little dynamic behavior and so a convenient way to think of it is as a 
static group.

*Macro Parameters:*
* The parameter is the name of the scope. Make sure is in quotes. This name is require to 
access any properties inside the block. 

See: [Example3](#Example3), [property_scope_end](#property_scope_end)

## <a name="property_scope_end"></a> property_scope_end()

*Ends a scope block*

This macro closes the scope block. You can you can still use any of the 
[Property Extended Info](#PropertyExtendedInfo). Which adds some dynamic behavior to the hold group.

See: [Example3](#Example3), [property_scope_begin](#property_scope_begin), [Property Extended Info](#PropertyExtendedInfo)

## <a name="property_list_fnbegin"></a> property_list_fnbegin(NAME, TYPE)

*Starts a list block*

This macro is the begging of a special type of block. This block unlike the others contains two different 
C++ scopes `{}` + `{}`. Stead of the + we use [`property_list_fnenum()`](#property_list_fnenum). Also we 
will require an end at the end of these sequence ['property_list_fnend()'](#property_list_fnend). So what 
is going on here? Well the first C++ block is a lambda function which actually is identical to 
[property_var_fnbegin](#property_var_fnbegin). So you can look at that one for reference. However here
we are dealing with a list which means that the parameter Index will actually get use.

See: [property_var_fnbegin](#property_var_fnbegin), [property_list_fnenum](#property_list_fnenum),
     [property_list_fnend](#property_list_fnend)

## <a name="property_list_fnenum"></a> property_list_fnenum()

*End the first part of a list block and starts the second and final part*

## <a name="property_list_fnend"></a> property_list_fnend()

--------------
--------------
--------------
# Examples

## <a name="Example0"></a> Example 0 - The basics
This example shows the very basics of adding properties to any class via `property::base`.

See: [`property_vtable`](#property_vtable), [`property_begin`](#property_begin), 
[`property_var`](#property_var), [`property_vend`](#property_vend)

```cpp
------------------------------------------------------------------------------------------- */


// This is a simple class note however that is derived from property::base
struct example0 : property::base
{
    virtual ~example0() = default;

    // Example of a simple variable (nothing special here)
    float m_Others { 0 };

    // Function which will set our variable to a specific value.
    // This value will indicate that the structure was initialize correctly 
    void DefaultValues( void ) noexcept
    {
      //  m_Others = 102.10101f;
    }

    // Function to make sure that our structure was initially correctly
    void SanityCheck( void ) const noexcept
    {
      //  assert( m_Others == 102.10101f );
    }

    property_vtable()                            
};

property_begin( example0 )                         
{                                               
    property_var( m_Others )    
}
property_vend_h(example0)


/* ----------------------------------------------------------------------------------------------
``` 
--------------
--------------
## <a name="Example1"></a> Example 1 - More property types
We focus here on adding more types of properties to show that is not a scary thing.
Note that we also overwrite the name of the property block for more info([property_begin](#property_begin)).
We also override the name of a property([property_var](#property_var)) add we add a help string with a 
function `Help` see [Property Extended Info ](#PropertyExtendedInfo).
 
```cpp 
------------------------------------------------------------------------------------------- */

struct example1 : property::base
{
    void DefaultValues( void ) noexcept
    {
        m_Int       = 10;
        m_Float     = 10.10f;
        m_Bool      = true;
        m_String    = "Hello";
        m_OOBB      = oobb{ -100.0f, 100.0f };
    }

    void SanityCheck( void ) const noexcept
    {
        assert( m_Int        == 10      );
        assert( m_Float      == 10.10f  );
        assert( m_Bool       == true    );
        assert( m_String     == "Hello" );
        assert( m_OOBB.m_Min == -100.0f );
        assert( m_OOBB.m_Max ==  100.0f );
    }

    property_vtable()

protected:

    int             m_Int       { 0 };
    float           m_Float     { 0 };
    bool            m_Bool      { 0 };
    string_t        m_String    {};
    oobb            m_OOBB      { 0 };
};

property_begin_name( example1, "example1_renamed" )
{
      property_var( m_Int    )
    , property_var( m_Float  ).Name("Float_renamed")
    , property_var( m_String )
    , property_var( m_Bool   ).Help( "I am adding some help for this property (Hello!!!)" )
    , property_var( m_OOBB   )
} property_vend_h( example1 )

/* ----------------------------------------------------------------------------------------------
``` 
--------------
--------------
## <a name="Example2"></a> Example 2 - Single inheritance & virtual properties
This example cover two different concepts. First the fact that we are not longer deriving from 
`property::base` directly rather we are using the previous example as our base class. The only
additional work is that we must let the property system know about this case. We do that by
using [`property_parent`](#property_parent).

We are also going to introduce the concept of a "virtual property" also known as a "function property".
This actually is a very powerful feature which allows to add allot of cool dynamic behaviors. But
for this example we will limit to just return an very simple equation. Please check 
[`property_var_fnbegin`](#property_var_fnbegin) for reference. Also note that we could have use
this function to get/set one of our member variables.

See: [`property_parent`](#property_parent), [`property_var_fnbegin`](#property_var_fnbegin),
[`property_var_fnend`](#property_var_fnend)
```cpp 
------------------------------------------------------------------------------------------- */

struct example2 : example1
{
    enum num
    {
        NONE
        , SOME = 22
        , OTHER
    };

    constexpr static std::array List
    {
            std::pair{ "NONE", (int)num::NONE }
        ,   std::pair{ "SOME", (int)num::SOME }
        ,   std::pair{ "OTHER", (int)num::OTHER }
    };

    num   m_Num{ num::NONE };

    void DefaultValues( void ) noexcept
    {
        example1::DefaultValues();
        m_Others = 22;
        m_Num = num::OTHER;
    }

    void SanityCheck( void ) const noexcept
    {
        example1::SanityCheck();
        assert( m_Others == 22 );
        assert(m_Num == num::OTHER);
    }

    property_vtable()

protected:

    int m_Others { 0 };
};

property_begin( example2 )
{
      property_parent( example1 )
    , property_var( m_Others )
    , property_var_fnbegin( "SomeVirtualIntFPS", int )
    { 
        (void)Self;
        if ( isRead )
        {
            static int a=0;
            InOut = a++;   // Example of a stupid equation
        }
        else
        {
            // It does not handle writing
        }
    } property_var_fnend()
        .Help("This is a virtual property one of the most powerful features of this property system"
              "Virtual properties also known as function properties are extremely useful for a variety of things"
              "For instance as in this example to show read only data, such fps or other stats. It can also be"
              "Used for backwards compatibility by using an old name of an old property and redirecting to the new version"
              "They can also be used to create other complex behaviors" )
        .Flags( property::flags::SHOW_READONLY | property::flags::DONTSAVE )

    , property_var_fnbegin("Num", string_t)
    {
        if (isRead)
        {
            for (auto& E : example2::List)
            {
                if (Self.m_Num == E.second)
                {
                    InOut = E.first;
                    break;
                }
            }
        }
        else
        {
            for (auto& E : example2::List)
            {
                if (InOut == E.first)
                {
                    Self.m_Num = static_cast<example2::num>(E.second);
                    break;
                }
            }
        }
    } property_var_fnend()
        .EDStyle(property::edstyle<string_t>::Enumeration(example2::List))
        .Help("This is an example of an Enumeration")

} property_vend_h(example2)


/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example3"></a> Example 3 - Multiple inheritance & Scopes
This example shows how we can add more than one parent which contains properties. This shows cases the
flexibility of this system. Note that is simply solve by adding more [`property_parent`](#property_parent).

Additional to that concept we are adding the concept of scopes. Scopes are mainly used to group
properties together. This specially useful for game editors. Note that you can next scopes inside other 
scopes. 

See: [`property_parent`](#property_parent), [`property_scope_begin`](#property_scope_begin),
[`property_scope_end`](#property_scope_end)
```cpp 
------------------------------------------------------------------------------------------- */

struct example3 : example0, example2
{
    void DefaultValues( void ) noexcept
    {
        example0::DefaultValues();
        example2::DefaultValues();
        m_Others       = 101;
        m_ScopedOthers = 103;
    }

    void SanityCheck( void ) const noexcept
    {
        example0::SanityCheck();
        example2::SanityCheck();
        assert( m_Others        == 101 );
        assert( m_ScopedOthers  == 103 );
    }

    property_vtable()

vs2017_hack_protected

    int m_Others        { 0 };
    int m_ScopedOthers  { 0 };
};

property_begin_name( example3, "example3_pepe" )
{
      property_parent( example0 )
    , property_parent( example2 )
    , property_var( m_Others ).Help( "some help" )
    , property_scope_begin( "Scope" )
    {
        property_var( m_ScopedOthers ).Help( "some help" )
#if !defined(_MSC_VER) || (_MSC_VER >= 1920)                                    // This should work, only visual studio 2017 has issues with this
        .DynamicFlags([](const std::byte&) noexcept->property::flags::type
        {
            return {};
        })
#endif
    } property_scope_end().Help( "This is a example for the scope" )
} property_vend_h( example3 )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example4"></a> Example 4 - Member variable with properties
We are going to use example3 as our parent, because why not? We can see a variable m_A of 
type example0 which as we know also has properties. The way we are going to let the system know that we
have a variable which has properties is to simply added as a regular property. The system will automatically 
will detect that it derives from `property::base` and it will connect it in. Here the path to any property
inside m_A will look like this: `"example4/A/othersFloat"` note that it will ignore the property block 
name example0 and stead use the name of our variable 'A'. 

```cpp 
------------------------------------------------------------------------------------------- */

struct example4 : example3
{
    example4() 
        : m_Shared{ std::shared_ptr<example0>{ new example0 } }
        , m_UniquePtr{ std::unique_ptr<example0>{ new example0 } }
        , m_Pointer{ new example0 }
        , m_RegularPointerList{ new example0, new example0, new example0 }
        , m_SmartpointerList{ std::make_unique<example0>(), std::make_unique<example0>() }
    {
    
    }

    ~example4() 
    { 
        delete m_Pointer; 
        for( auto& p : m_RegularPointerList ) delete p;
    }

    void DefaultValues( void ) noexcept
    {
        example3::DefaultValues();
        m_A.DefaultValues();
        m_Shared->DefaultValues();
        m_UniquePtr->DefaultValues();
        m_Pointer->DefaultValues();
        for( auto& p : m_RegularPointerList ) p->DefaultValues();
        for( auto& p : m_SmartpointerList ) p->DefaultValues();
    }

    void SanityCheck( void ) const noexcept
    {
        example3::SanityCheck();
        m_A.SanityCheck();
        m_Shared->SanityCheck();
        m_UniquePtr->SanityCheck();
        m_Pointer->SanityCheck();
        for( auto& p : m_RegularPointerList ) p->SanityCheck();
        for( auto& p : m_SmartpointerList ) p->SanityCheck();
    }

    property_vtable()

protected:

    example0                                m_A {};
    std::shared_ptr<example0>               m_Shared;
    std::unique_ptr<example0>               m_UniquePtr;
    std::unique_ptr<example0>               m_UniquePtr2{};
    example0*                               m_Pointer;
    example0*                               m_Pointer2{nullptr};
    std::vector<example0*>                  m_RegularPointerList{};
    std::array<std::unique_ptr<example0>,2> m_SmartpointerList{};
};

property_begin( example4 )
{
    property_parent( example3 )
    , property_var( m_A )
    , property_var(m_Shared)
    , property_var(m_UniquePtr)
    , property_var(m_Pointer)
    , property_var(m_UniquePtr2)
    , property_var(m_Pointer2)
} property_vend_h( example4 )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example5"></a> Example 5 - std::array with properties
One of the tricky things to solve in property systems are lists. LIONant's property system supports natively
two kinds of lists: std::arrays and std::vectors. See [PropertyTypes](#PropertyTypes) for more details about lists.

In this example we see that we have an array of size 3 which contains instances of class [example0](#Example0). 
The path for the variables inside the classes inside the array would look like this: 
`"example5/A[0]/Others"` As you can see the list has brackets `[]` similar to an actual 
array in a programming lingo, so it looks very much like you would expect. This is a powerful concept
because now not only we have alpha-numeric characters which are unique identifiers by now we also have
numeric characters which are actual numbers. BTW these numbers can mean anything the user decides which
mean that they can be indices, or hash-keys, or anything else. As long as the number is not larger than
64bits. For std::array and std::vector this number is the actual index.

Note that we really did not need to do anything special here. We just let the property system know
about our variable m_A.
```cpp 
------------------------------------------------------------------------------------------- */

struct example5 : example3
{
    void DefaultValues( void ) noexcept
    {
        example3::DefaultValues();
        for( auto& E : m_A ) E.DefaultValues();
    }

    void SanityCheck( void ) const noexcept
    {
        example3::SanityCheck();
        for ( auto& E : m_A ) E.SanityCheck();
    }

    property_vtable()

protected:

    std::array<example0, 3> m_A {};
};

property_begin( example5 )
{
    property_parent( example3 )
    , property_var( m_A ).Help( "Some help" )
} property_vend_h( example5 )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example6"></a> Example 6 - td::vector within properties
Note thiat this example works very much like [Example 5](#Example5). The only difference is that we are 
using a std::vector rather than std::array. But everything else is about the same.

```cpp 
------------------------------------------------------------------------------------------- */

struct example6 : example3
{
    void DefaultValues( void ) noexcept
    {
        example3::DefaultValues();
        m_A.resize(10);
        for ( auto& E : m_A ) E.DefaultValues();
    }

    void SanityCheck( void ) const noexcept
    {
        example3::SanityCheck();
        for ( auto& E : m_A ) E.SanityCheck();
    }

    property_vtable()

protected:

    std::vector<example0> m_A {};
};

property_begin( example6 )
{
    property_parent( example3 )
    , property_var( m_A ).Help( "Some help" )
} property_vend_h( example6 )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example7"></a> Example 7 - atomic lists and, std::array  
Just like the previous two Examples ([Example 5](#Example5), and [Example 6](#Example6)) we are going to 
create a simple lists. But rather than using classes we are just going to use mondaine properties. The 
ones that the user provided us in the [Property Config File](#PropertyConfigFile). This case the list 
will be ints. Note again how we don't need to specify anything special. 

```cpp 
------------------------------------------------------------------------------------------- */

struct example7 : example3
{
    void DefaultValues( void ) noexcept
    {
        example3::DefaultValues();
        for ( auto& E : m_A ) E = static_cast<int>(&E - &m_A[0]);
    }

    void SanityCheck( void ) const noexcept
    {
        example3::SanityCheck();
        for ( auto& E : m_A ) assert( E == static_cast<int>( &E - &m_A[ 0 ] ) );
    }

    property_vtable()

protected:

    std::array<int, 3> m_A { 0 };
};

property_begin( example7 )
{
    property_parent( example3 )
    , property_var( m_A ).Help( "Some help" )
} property_vend_h( example7 )

/* ----------------------------------------------------------------------------------------------
```

--------------
--------------
## <a name="Example8"></a> Example 8 - Opt-in properties & Disable a property.
This example is the introduction of Opt-in properties. Opt-in properties are not officially OOP since
they don't have a base class where you can get the properties of the entire hierarchy. This method
requires always some other system to know about the class type to access the properties. However the
big payoff is that it does not require to use virtual functions at all. Note the way we end the property
block is different from before. We now use [`property_end`](#property_end) which requires no parameters.

This example also covers how to disable a property. Note in the property block we are adding a
Disable function (see [Property Extended Info](#PropertyExtendedInfo)). What this function says is that
we will disable m_Y if m_X == 5. So no very special really... but there an easy way to add dynamic behavior
to something that other wise will be static. 

See: [`property_end`](#property_end), [`property_begin`](#property_begin), 
[Property Extended Info](#PropertyExtendedInfo)

```cpp 
------------------------------------------------------------------------------------------- */

struct example8
{
    void DefaultValues( void ) noexcept
    {
        m_X = 22;
        m_Y = 222;
    }

    void SanityCheck( void ) const noexcept
    {
        assert( m_X == 22 );
        assert( m_Y == 222 );
    }

vs2017_hack_protected

    int m_X { 0 };
    int m_Y { 0 };

    property_friend
};

property_begin(example8)
{
      property_var( m_X ).Help( "component of some cool vector" )
    , property_var( m_Y )
        .DynamicFlags([](const std::byte& Bytes) noexcept -> property::flags::type
        { 
            auto& Self = reinterpret_cast<const t_self&>(Bytes);
            // Disable Y property when m_X is equal to 5
            if( Self.m_X == 5 ) return property::flags::DISABLE;
            return {};
        })  
        .Help( "component of some cool vector" )
} property_end()

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example9"></a> Example 9 - Opt-in properties with a hierarchy & Scopes
This example by now should be not very exciting. Here we are simply adding a parent to an Opt-in 
property definition. So this example confirms that yes you can have a parent in an Opt-in, and Scopes.

See: [`property_parent`](#property_parent), [`property_scope_begin`](#property_scope_begin)

```cpp 
------------------------------------------------------------------------------------------- */

struct example9 : example8
{
    void DefaultValues( void ) noexcept
    {
        example8::DefaultValues();
        m_T = 0.42f;
        for ( auto& E : m_Rita ) E = static_cast<int>( &E - &m_Rita[ 0 ] );
    }

    void SanityCheck( void ) const noexcept
    {
        example8::SanityCheck();
        assert( m_T == 0.42f );
        for ( auto& E : m_Rita ) assert( E == static_cast<int>( &E - &m_Rita[ 0 ] ) );
    }

vs2017_hack_protected

    float               m_T     { 0 };
    std::array<int, 2>  m_Rita  { 0 };

    property_friend
};

property_begin(example9)
{
    property_parent( example8 )
    , property_var( m_T )
    , property_scope_begin( "Scope" )
    {
        property_var( m_Rita ).Help( "some help" )
    } property_scope_end().Help( "This is a example for the scope" )
} property_end()


/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example10"></a> Example 10 - Hybrid between Opt-in and Object Oriented
This is the last example which shows the different methods of property blocks. This method
showcases the mixing between Opt-in properties and Object Oriented. In this example
we have as parents `example9` which is an Opt-in property and `example6` which is object oriented.
Note that `example10` is object oriented (OO) because `example6` is. So OO is viral ones 
you include a parent which is then you become as well. Because of this you can see the 
`property_vtable()` and `property_vend` showing up.

See: [`property_vtable`](#property_vtable), [`property_vend`](#property_vend)

```cpp 
------------------------------------------------------------------------------------------- */

struct example10 : example9, example6
{
    void DefaultValues( void ) noexcept
    {
        example9::DefaultValues();
        example6::DefaultValues();
        m_Scrary = 0.999f;
        for ( auto& E : m_Time ) E = static_cast<int>( &E - &m_Time[ 0 ] ) + 100;
    }

    void SanityCheck( void ) const noexcept
    {
        example9::SanityCheck();
        example6::SanityCheck();
        assert( m_Scrary == 0.999f );
        for ( auto& E : m_Time ) assert( E == (static_cast<int>( &E - &m_Time[ 0 ] ) + 100) );
    }

    property_vtable()

protected:

    float               m_Scrary { 0 };
    std::array<int, 6>  m_Time   { 0 };
};

property_begin( example10 )
{
    property_parent( example9 )
    , property_parent( example6 )
    , property_var( m_Scrary )
    , property_var( m_Time ).Help( "some help" )
} property_vend_h( example10 )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
# <a name="Example0CustomLists"></a> Example 0 Custom Lists, std::array
Example of doing a simple custom list in this case an std::array even though we support it natively.
But sometimes is desirable to have your own lists unique to your component for instance.
An example of this is using a virtual list, which does not exists as data.
So we are replacing the standard implementation with our own. Check [Property Types](#PropertyTypes)
section to get a general idea of this subject. Note that we will use 
[property_list_fnbegin](#property_list_fnbegin) which is used to tell the system we are going to 
create a property list. This is the header for a lambda, which we are somewhat familiar with.
[property_list_fnenum](#property_list_fnenum) finish the previous lambda and starts the next lambda.
this next lambda is the brains for lists. They system will ask some questions using `Cmd`
parameter which is an enum of type `property::lists_cmd`. You also have the InOut parameter to 
get/set information base on the `Cmd`. 

```cpp 
------------------------------------------------------------------------------------------- */

struct example0_custom_lists : property::base
{
    void DefaultValues( void ) noexcept
    {
        float a = 0.0f;
        for( auto& E : m_Others ) E = (a += 0.1f);
    }

    void SanityCheck( void ) const noexcept
    {
        float a = 0.0f;
        for ( auto& E : m_Others ) assert( E == ( a += 0.1f ) );
    }

     property_vtable()

vs2017_hack_protected

    std::array<float, 3> m_Others {};
};

property_begin( example0_custom_lists )
{
    property_list_fnbegin( "CustomFloatArray", float )
    {   // Self     - is our class
        // isRead   - Will give ask us what to do
        // InOut    - is the argument
        // Index    - is the index used to access the entry, is 64 bits so we may want to cast down a bit to avoid warnings
        auto  i = static_cast<int>( Index );
        if( isRead )       InOut              = Self.m_Others[ i ];
        else               Self.m_Others[ i ] = InOut;
    } property_list_fnenum()
    {   // Self         - is our class
        // Cmd          - Will give ask us what to do
        // InOut        - is the argument
        // MemoryBlock  - block of memory that can be use to store iterators
        switch ( Cmd )
        {
            case property::lists_cmd::READ_COUNT:   InOut = Self.m_Others.size(); break;
            case property::lists_cmd::WRITE_COUNT:  break;
            case property::lists_cmd::READ_FIRST:   InOut = 0; break;
            case property::lists_cmd::READ_NEXT:    if ( ++InOut == Self.m_Others.size() ) InOut = property::lists_iterator_ends_v; break;
            default: assert( false );
        }
    } property_list_fnend()
} property_vend_h( example0_custom_lists )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example1CustomLists"></a> Example 1 Custom Lists, std::map
Example of doing a custom list This time we dealing with a non-natively-supported list type (std::map).
This is a better example of (why) is going to be able to replace lists. Here when we try to get/set the 
property (first lambda) reading is the easy case, but note that the index is where the hash key value is.
When writing we really have two cases here. If we find the value we just set the property. But if we
can not find the value we will create a new entry into the hash map. This behavior is unique to our example.
But generally speaking is a useful construct because allows us to serialize-in new values.

In the second lambda after `property_list_fnenum` the key thing to look at here is how we create 
an instance of the iterator. `case property::lists_cmd::READ_FIRST:` note how we use new to 
construct in-place out iterator. We use the parameter `MemoryBlock` for the memory. This
is done this way to avoid allocations. Note that you should always check if MemoryBlock is big enough.

Finally `case property::lists_cmd::READ_NEXT:` in the last entry we destroy our iterator.
I assume most cases the destructor of the iterator will do nothing. But just in case is a good
practice to end the variable lifetime correctly.

```cpp 
------------------------------------------------------------------------------------------- */
#include <map>

struct example1_custom_lists : property::base
{
    void DefaultValues( void ) noexcept
    {
        seed = original_seed_v;
        for( int x=0; x<10; x++ )
        {
            auto                    ID  { rand()   };
            std::unique_ptr<node>   Node{ new node };

            Node->m_Value = (ID / 22.0f);

            m_Map1.insert( std::pair( ID, std::move(Node) ) );
        }
    }

    void SanityCheck( void ) const noexcept
    {
        for( const auto& [ ID, Node] : m_Map1 )
        {
            auto f = (ID / 22.0f);
            auto v = Node->m_Value;
            assert( v == f );
            assert( [](auto ID)
            {
                seed = original_seed_v;
                for ( int i = 0; i < 10; i++ ) 
                    if ( rand() == ID ) return true;
                return false;
            }(ID));
        }
    }

    property_vtable()

vs2017_hack_protected

    struct node
    {
        float   m_Value;            // Value of this node
    };

    using mymap = std::map<uint32_t, std::unique_ptr<node>>;

    mymap m_Map1;

    constexpr static const auto original_seed_v = 123456789u; 
    inline static uint32_t seed = original_seed_v;
    static uint32_t rand( void )
    {
        const auto a    = 1103515245u;
        const auto c    = 12345u;
        const auto m    = 1 << 31;
        seed = ( a * seed + c ) % m;
        return seed;
    }

    friend struct example2_custom_lists;
    friend struct example3_custom_lists;
};

property_begin( example1_custom_lists )
{
    property_list_fnbegin( "MapContainer", float )
    {   // Self         - is our class
        // isRead       - Will give ask us what to do
        // InOut        - is the argument
        // Index        - is the index used to access the entry, is 64 bits so we may want to cast down a bit to avoid warnings
        auto hash = static_cast<uint32_t>(Index);
        if ( isRead )      InOut = Self.m_Map1[hash]->m_Value;
        else
        {
            auto e = Self.m_Map1.find( hash );
            if( e != Self.m_Map1.end() ) e->second->m_Value = InOut;
            else
            {
                // could not find it so lets insert it
                std::unique_ptr<t_self::node> Node{new t_self::node};
                Node->m_Value = InOut;
                Self.m_Map1.insert( std::pair{hash, std::move(Node) } );
            }
        }
    } property_list_fnenum()
    {   // Self         - is our class
        // Cmd          - Will give ask us what to do
        // InOut        - is the argument
        // MemoryBlock  - block of memory that can be use to store iterators
        using iterator = t_self::mymap::iterator;
        static_assert( sizeof( MemoryBlock ) >= sizeof( iterator ), "Need more memory to store the std::map iterator" );
        switch( Cmd )
        {
            case property::lists_cmd::READ_COUNT:   InOut = Self.m_Map1.size(); break; 
            case property::lists_cmd::WRITE_COUNT:  break;
            case property::lists_cmd::READ_FIRST:   InOut = (Self.m_Map1.size() == 0) ? property::lists_iterator_ends_v : (*new( MemoryBlock.data() ) iterator{ Self.m_Map1.begin() })->first; break;
            case property::lists_cmd::READ_NEXT:
            {
                auto& Iterator = ++*reinterpret_cast<iterator*>( MemoryBlock.data() );
                InOut = (Iterator == Self.m_Map1.end()) ? Iterator.~iterator(), property::lists_iterator_ends_v : Iterator->first;
                break;
            }
            default: assert(false);
        }
    } property_list_fnend()
} property_vend_h( example1_custom_lists )


/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example2CustomLists"></a> Example 2 Custom Lists, adding a new system list
This example cover the case where you may want to add a new type of container into the supported lists
of containers which the system has. Ones we add this new type of container in such away then any class
with properties can take advantage of it. Which is a very nice and generic way to do things.

```cpp 
------------------------------------------------------------------------------------------- */

namespace property
{
    //--------------------------------------------------------------------------------------------------------------------
    // Map List. Deals with the std::map states
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T_VAR >
    void MapHashList( void* pSelf, std::uint64_t& InOut, property::lists_cmd Cmd, std::array<std::uint64_t,4>& MemoryBlock ) noexcept  
    {   
        auto& Map = *reinterpret_cast<T_VAR*>( pSelf ); 
        switch( Cmd )
        {
            using iterator = decltype( Map.begin() );
            static_assert( sizeof( MemoryBlock ) >= sizeof( iterator ), "Need more memory to store the std::map iterator" );

            case property::lists_cmd::READ_COUNT:   InOut = Map.size(); break; 
            case property::lists_cmd::WRITE_COUNT:  break;
            case property::lists_cmd::READ_FIRST:   InOut = (Map.size() == 0) ? property::lists_iterator_ends_v : (*new( MemoryBlock.data() ) iterator{ Map.begin() })->first; break;
            case property::lists_cmd::READ_NEXT:
            {
                auto& Iterator = ++*reinterpret_cast<iterator*>( MemoryBlock.data() );
                InOut = (Iterator == Map.end()) ? Iterator.~iterator(), property::lists_iterator_ends_v : Iterator->first;
                break;
            }
            default: assert(false);
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    // GetSet for atomic types
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T_VAR, typename T_TYPE >
    bool MapHashAtomicGetSet( void* pSelf, T_TYPE& InOut, bool isRead, std::uint64_t Index ) noexcept  
    {   
        auto& Map  = *reinterpret_cast<T_VAR*>( pSelf );
        auto  hash = static_cast<uint32_t>( Index );
        auto  e    = Map.find( hash );

        if ( isRead ) 
        {
            if( e == Map.end() ) return false;
            InOut = e->second;
        }
        else
        {
            if( e == Map.end() )    Map.insert( std::pair{hash, InOut } );
            else                    e->second = InOut;
        }
        return true;
    }

    //--------------------------------------------------------------------------------------------------------------------
    // GetSet for property Tables (a hash map with a pointer to other properties)
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T > constexpr
    std::optional<std::tuple<const property::table&, void*>> HashMapTableGetSet( void* pSelf, std::uint64_t Index ) noexcept
    {
        auto& Map      = *reinterpret_cast<T*>( pSelf );
        auto  MapEntry = Map.find( static_cast<typename T::key_type>( Index ) );
        
        if( MapEntry == Map.end() ) return std::nullopt;

        auto& Table    = property::getTable(*MapEntry->second);
        return std::tuple<const property::table&, void*>{ Table, &(*MapEntry->second) };
    }

    //--------------------------------------------------------------------------------------------------------------------
    // Link to the property system
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< is_specialized_v<std::map, std::decay_t<T_VAR>>, property::setup_entry >
    PropertyVar( const char( &pName )[ N ], int Offset ) noexcept 
    { 
        using var = std::decay_t<T_VAR>;
        using e   = typename std::decay_t<typename var::mapped_type>;

        if constexpr ( is_specialized_v<std::unique_ptr,e> || std::is_pointer_v<typename var::mapped_type> )
            return property::setup_entry( pName, HashMapTableGetSet<var>, Offset, MapHashList<var> );
        else
            return property::setup_entry( pName, MapHashAtomicGetSet<var, e>, Offset, MapHashList<var> );
    }
}

// The actual class that is about to example this new system
struct example2_custom_lists : property::base
{
    std::map<uint32_t, float> m_Map1;

    void DefaultValues( void ) noexcept
    {
        example1_custom_lists::seed = example1_custom_lists::original_seed_v;
        for ( int x = 0; x < 10; x++ )
        {
            auto    ID { example1_custom_lists::rand() };
            float   Node;
            Node = ( ID / 22.0f );
            m_Map1.insert( std::pair( ID, Node ) );
        }
    }

    void SanityCheck( void ) const noexcept
    {
        for ( const auto&[ ID, Node ] : m_Map1 )
        {
            auto k = ( ID / 22.0f );
            auto l = k == Node;
            assert( l );
            assert( []( auto ID )
            {
                example1_custom_lists::seed = example1_custom_lists::original_seed_v;
                for ( int i = 0; i < 10; i++ )
                    if ( example1_custom_lists::rand() == ID ) return true;
                return false;
            }( ID ) );
        }
    }

    property_vtable()
};

property_begin( example2_custom_lists )
{
    property_var( m_Map1 )
}
property_vend_h( example2_custom_lists )

/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example3CustomLists"></a> Example 3 Custom Lists
Example of extends the previous example by having two std::maps one with atomic properties and another 
with more properties.
```cpp 
------------------------------------------------------------------------------------------- */

struct example3_custom_lists : property::base
{
    example3_custom_lists( void )
    {
        example1_custom_lists::seed = example1_custom_lists::original_seed_v;

        // unique pointer
        for ( int x = 0; x < 10; x++ )
        {
            auto                    ID   { example1_custom_lists::rand() };
            std::unique_ptr<example0>  Node { new example0 };
            m_Map1.insert( std::pair( ID, std::move( Node ) ) );
        }

        //regular pointer
        for ( int x = 0; x < 3; x++ )
        {
            auto                    ID { example1_custom_lists::rand() };
            m_Map2.insert( std::pair( ID, new example9 ) );
        }
    }

    ~example3_custom_lists( void )
    {
        // free all the regular pointers
        for ( auto& Pair : m_Map2 )
            delete Pair.second;
    }

    void DefaultValues( void ) noexcept
    {
        // Initialize all the entries
        for( auto& Pair : m_Map1 )
        {
            Pair.second->DefaultValues();        
        }

        // Initialize all the entries
        for ( auto& Pair : m_Map2 )
        {
            Pair.second->DefaultValues();
        }
    }

    void SanityCheck( void ) const noexcept
    {
        for ( const auto&[ ID, Node ] : m_Map1 )
        {
            Node->SanityCheck();
            assert( []( auto ID )
            {
                example1_custom_lists::seed = example1_custom_lists::original_seed_v;
                for ( int i = 0; i < 10; i++ )
                    if ( example1_custom_lists::rand() == ID ) return true;
                return false;
            }( ID ) );
        }

        for ( const auto&[ ID, Node ] : m_Map2 )
        {
            Node->SanityCheck();
            assert( []( auto ID )
            {
                example1_custom_lists::seed = example1_custom_lists::original_seed_v;
                for ( int i = 0; i < 13; i++ )
                    if ( example1_custom_lists::rand() == ID ) return true;
                return false;
            }( ID ) );
        }
    }

    property_vtable()

protected:

    std::map<uint32_t, std::unique_ptr<example0>>  m_Map1;     // this would be the most common case for general entries
    std::map<uint32_t, example9* >                 m_Map2;     // sometimes the memory may exits somewhere so we just want pointers
};

property_begin( example3_custom_lists )
{
      property_var( m_Map1 )
    , property_var( m_Map2 )
}
property_vend_h( example3_custom_lists )


/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
## <a name="Example4CustomLists"></a> Example 4 Custom Lists
Example of a custom link list that could be use in an entity component system 
```cpp
------------------------------------------------------------------------------------------- */

template< typename T >
struct llist
{
    using type = T;

    auto append(T* pNode) noexcept
    {
        assert(pNode);
        if (m_pLast) m_pLast->m_pNext = pNode;
        else m_pValue = pNode;
        m_pLast = pNode;
        m_Count++;
    }

    auto size(void) const noexcept {return m_Count;}

    T*              m_pValue{ nullptr };
    T*              m_pLast { nullptr };
    std::uint32_t   m_Count{ 0 };            // This needs to come after the pointer
};

namespace property
{
    //--------------------------------------------------------------------------------------------------------------------
    // Map List. Deals with the std::map states
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T_VAR >
    void LListController( void* pSelf, std::uint64_t& InOut, property::lists_cmd Cmd, std::array<std::uint64_t, 4>& MemoryBlock ) noexcept
    {
        auto& LList = *reinterpret_cast<T_VAR*>(pSelf);
        switch (Cmd)
        {
        case property::lists_cmd::READ_COUNT:   InOut = LList.m_Count; 
                                                break;
        case property::lists_cmd::WRITE_COUNT:  break; 
        case property::lists_cmd::READ_FIRST:   MemoryBlock[0] = reinterpret_cast<std::uint64_t>(LList.m_pValue); 
                                                MemoryBlock[1] = 0; 
                                                InOut = MemoryBlock[0] ? MemoryBlock[1] : property::lists_iterator_ends_v;
                                                InOut |= (static_cast<uint64_t>(LList.m_pValue->getUID()) << 32);
                                                break;
        case property::lists_cmd::READ_NEXT:
        {
            auto p = reinterpret_cast<typename T_VAR::type*>(MemoryBlock[0])->m_pNext;
            if (p)
            {
                MemoryBlock[0] = reinterpret_cast<std::uint64_t>(p);
                InOut = ++MemoryBlock[1];
                InOut |= (static_cast<uint64_t>(p->getUID()) << 32);
            }
            else
            {
                InOut = property::lists_iterator_ends_v;
            }
            break;
        }
        default: assert(false);
        }
    }

    //--------------------------------------------------------------------------------------------------------------------
    // GetSet for property Tables (a hash map with a pointer to other properties)
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T > constexpr
    std::optional<std::tuple<const property::table&, void*>> LListGetSet(void* pSelf, std::uint64_t TypeID ) noexcept
    {
        using var   = typename std::decay_t<T>::type;
        using e     = std::decay_t<var>;
        auto& LList = *reinterpret_cast<T*>(pSelf);
        auto  p     = LList.m_pValue;

        if (auto Index = TypeID & 0xffffffff;  Index < LList.size())
        {
            // Search for our entry
            for (; Index && p; p = p->m_pNext, --Index);
            assert(Index == 0);
        }
        else if( Index == LList.size() )
        {
            // Create new entry
            auto& Map = e::getFactoriesMap();
            auto Res = Map.find(static_cast<std::uint32_t>(TypeID >> 32));
            assert(Res != Map.end());

            p = Res->second->New();
            LList.append(p);
        }
        else 
        { 
            // Bad index
            assert(false);  
        }

        assert(p);
        auto& Table = property::getTable(*p);
        return std::tuple<const property::table&, void*>{ Table, &(*p) };
    }

    //--------------------------------------------------------------------------------------------------------------------
    // Link to the property system
    //--------------------------------------------------------------------------------------------------------------------
    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< is_specialized_v<llist, std::decay_t<T_VAR>>, property::setup_entry >
    PropertyVar(const char(&pName)[N], int Offset) noexcept
    {
        using var = std::decay_t<T_VAR>;
        return property::setup_entry(pName, LListGetSet<var>, Offset, LListController<var>);
    }
}

//
// Example of factory
//
struct component_base;
struct factory_base
{
    virtual component_base*         New( void ) const noexcept = 0;
    virtual std::uint32_t           UID( void ) noexcept = 0;
};

template< typename T_COMPONENT, std::uint32_t T_UID_V >
struct factory final : factory_base
{
    static_assert(T_UID_V);
    constexpr static std::uint32_t ms_UID = T_UID_V;

    virtual component_base*         New(void) const noexcept { return new T_COMPONENT; };
    virtual std::uint32_t           UID(void) noexcept { return ms_UID;  };
};

//
// Example of component
//

struct component_base : property::base
{
    component_base* m_pNext{ nullptr };
    string_t        m_Name;

    property_vtable()
    component_base() = default;
    component_base(string_t&& X) : m_Name{ std::move(X) } {}
    virtual std::uint32_t getUID(void) const noexcept = 0;

    static const std::map<std::uint32_t, const factory_base* >& getFactoriesMap(void) noexcept;
};
property_begin(component_base)
{
      property_var(m_Name)
    , property_var_fnbegin("TypeID", int)
      {
          if (isRead) InOut = Self.getUID();
      }
      property_var_fnend()
}
property_vend_h(component_base)

struct component_a :component_base
{
    using component_base::component_base;

    virtual std::uint32_t getUID(void) const noexcept override { return ms_Factory.ms_UID; }
    constexpr static factory<component_a, 0x12353> ms_Factory{};
};

struct component_b : component_base
{
    using component_base::component_base;

    virtual std::uint32_t getUID(void) const noexcept override { return ms_Factory.ms_UID; }
    constexpr static factory<component_b, 0x222> ms_Factory{};
};

const std::map<std::uint32_t, const factory_base* >& component_base::getFactoriesMap(void) noexcept
{
    static const std::map<std::uint32_t, const factory_base* > Map
    { std::pair{ component_a::ms_Factory.ms_UID, &component_a::ms_Factory }
    , std::pair{ component_b::ms_Factory.ms_UID, &component_b::ms_Factory }  };
    return Map;
}

//--------------------------------------------------------------------------------------------------------------------

// The actual class that is about to example this new system
struct example4_custom_lists : property::base
{
    llist<component_base> m_ComponentList;

    void DefaultValues(void) noexcept
    {
        m_ComponentList.append(new component_a{ "First Component" });
        m_ComponentList.append(new component_b{ "Second Component" });
    }

    void SanityCheck(void) const noexcept
    {
        assert(m_ComponentList.size() == 2);
        int I = 0;
        for (auto p = m_ComponentList.m_pValue; p ; p = p->m_pNext, I++ )
        {
            switch (I)
            {
            case 0: assert(p->m_Name == "First Component"); break;
            case 1: assert(p->m_Name == "Second Component"); break;
            default: assert(false);
            }
        }
        assert(I == 2);
    }

    property_vtable()
};

property_begin(example4_custom_lists)
{
    property_var(m_ComponentList)
}
property_vend_h(example4_custom_lists)





/* ----------------------------------------------------------------------------------------------
```
--------------
--------------
--------------
# <a name="TestFunction"></a> Test01 Function
The test function job is to create two instances (A,B) of one of the examples, then we will initialize 
some values in A and serialize them into a `std::vector`. We use the `property::Enum` to accomplish that. 
We could have serialize them to disk but that is less interesting. Note that the entry type for the `std::vector`
are `property::entry` A property entry has the key for the actual property which is the 
full path name, as well as its value. The value is a `std::variant` which contains the type as well.

After collecting all the properties we copy them into B. At the same time we also print them out
for debugging purposes. After that we do a `B.SanityCheck` to make sure B has all the right values.
```cpp 
------------------------------------------------------------------------------------------- */
#include <iostream>

template< typename T_EXAMPLE >
void Test01( void )
{
    T_EXAMPLE A;

    //
    // Print out the name of the example
    //
    printf( "------------------------------------------------------------------------------\n" ); 
    std::cout.flush();
    printf( "[Test01 - String Properties. Saving and Loading to a std::vector]\n" );              
    std::cout.flush();  

    //
    // Check the basic stuff first
    //
    A.DefaultValues();
    A.SanityCheck();

    //
    // Collect all properties into a list as an example, here we could save them too
    //
    std::vector<property::entry> List;
    property::SerializeEnum( A, [&]( std::string_view PropertyName, property::data&& Data, const property::table&, std::size_t, property::flags::type Flags )
    {
        // If we are dealing with a scope that is not an array someone may have change the SerializeEnum to a DisplayEnum they only show up there.
        assert( Flags.m_isScope == false || PropertyName.back() == ']' );
        List.push_back( property::entry { PropertyName, Data } );
    });

    //
    // Copy values to B
    //
    T_EXAMPLE B;
    int i=0;
    for ( const auto&[ Name, Data ] : List )
    {
        // Copy to B
        property::set( B, Name.c_str(), Data );

        //
        // Print out the property
        //
        printf( "[%3d] -> ", i++ );                 
        std::cout.flush();

        printf( "%-70s", Name.c_str() );            
        std::cout.flush();

        std::visit( [&]( auto&& Value )
        {
            using T = std::decay_t<decltype( Value )>;

            if constexpr ( std::is_same_v<T, int> )
            {
                printf( " int    (%d)", Value );    
            }
            else if constexpr ( std::is_same_v<T, float> )
            {
                printf( " float  (%f)", Value );
            }
            else if constexpr ( std::is_same_v<T, bool> )
            {
                printf( " bool   (%s)", Value ? "true" : "false" );
            }
            else if constexpr ( std::is_same_v<T, string_t> )
            {
                printf( " string (%s)", Value.c_str() );
            }
            else if constexpr ( std::is_same_v<T, oobb> )
            {
                printf( " oobb   (%f, %f)", Value.m_Min, Value.m_Max );
            }
            else static_assert( always_false<T>::value, "We are not covering all the cases!" );
        }
        , Data );

        printf( "\n" ); 
        std::cout.flush();
    }

    // Confirm that all values were copied
    B.SanityCheck();
    printf( "[SUCCESSFUL] Copied all properties to new class\n" );
}

/* ----------------------------------------------------------------------------------------------
```
--------------
# <a name="Test2Function"></a> Test02 Function
This test function works very similar to Test1. The only difference is that stead of saving the
properties to a std::vector it saves them to a property::pack. This allows for much faster lookups
and since it is entirely binary there is not usage of strings at all. This saves memory and farther improves 
performance. The typical case for this is when dealing with final data or when doing something 
at run time that requires performance such copying properties.
```cpp
------------------------------------------------------------------------------------------- */

template< typename T_EXAMPLE >
void Test02( void )
{
    T_EXAMPLE A;

    //
    // Print out the name of the example
    //
    printf( "------------------------------------------------------------------------------\n" );
    printf( "[Test02 - Optimized properties - Saving and Loading to a property::pack]\n" );

    //
    // Check the basic stuff first
    //
    A.DefaultValues();
    A.SanityCheck();

    //
    // Collect all properties into a list as an example, here we could save them too
    //
    property::pack Pack;
    property::Pack( A, Pack );

    //
    // Debug display the pack
    //
    float LookUps = -1;         // The very first one is actually not a lookup
    {
        int iPath = 0;
        for( const auto& E : Pack.m_lEntry )
        {
            printf( "[%3d] -> ", static_cast<int>(&E - &Pack.m_lEntry[0]) );

            int c = 0;
            // Mark how many pops we are making
            for( int i=0; i<E.m_nPopPaths; ++i )
                c += printf( "/.." );

            for( int i=0; i<E.m_nPaths; ++i, ++iPath )
            {
                // Are we dealing with an array type?
                if( Pack.m_lPath[ iPath ].m_Index != property::lists_iterator_ends_v )
                {
                    // Arrays/Lists we get them for free
                    c += printf( "/[%" PRIu64 "]", Pack.m_lPath[ iPath ].m_Index );
                    LookUps -= 1.0f;
                }
                else
                {
                    c += printf( "/%u", Pack.m_lPath[ iPath ].m_Key );
                    if ( i == (E.m_nPaths - 1) && E.m_isArrayCount ) c += printf( "[]" );
                }
            }

            // Keep track of lookups
            LookUps += E.m_nPaths;

            // Make sure everything is align
            while( (c++)!=70 ) printf(" ");

            // print type and data
            std::visit( [&]( auto&& Value )
            {
                using T = std::decay_t<decltype( Value )>;

                if constexpr ( std::is_same_v<T, int> )
                {
                    printf( " int    (%d)", Value );
                }
                else if constexpr ( std::is_same_v<T, float> )
                {
                    printf( " float  (%f)", Value );
                }
                else if constexpr ( std::is_same_v<T, bool> )
                {
                    printf( " bool   (%s)", Value ? "true" : "false" );
                }
                else if constexpr ( std::is_same_v<T, string_t> )
                {
                    printf( " string (%s)", Value.c_str() );
                }
                else if constexpr ( std::is_same_v<T, oobb> )
                {
                    printf( " oobb   (%f, %f)", Value.m_Min, Value.m_Max );
                }
                else static_assert( always_false<T>::value, "We are not covering all the cases!" );
            }
            , E.m_Data );

            // next line
            printf( "\n" );
        }
    }

    //
    // Copy to B
    //
    T_EXAMPLE B;
    property::set( B, Pack );

    // Confirm that all values were copied
    B.SanityCheck();
    printf( "[SUCCESSFUL] Copied all properties to new class. Lookups per property (%.2f)\n", LookUps/Pack.m_lEntry.size() );
}
