#ifndef _PROPERTY_H
#define _PROPERTY_H
#pragma once

//--------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright ( c ) 2019 LIONant
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files ( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------------
#include <variant>
#include <array>
#include <assert.h>
#include <type_traits>
#include <tuple>
#include <memory>
#include <functional>
#include <optional>
#include <cstddef>
#include <inttypes.h>
#include <cctype>

#ifndef PRIu64
    #define PRIu64 "ld"
#endif

// Microsoft and its macros....
#ifdef max
    #undef max
#endif

//--------------------------------------------------------------------------------------------
// Disable some warnings
//--------------------------------------------------------------------------------------------
#if defined(__clang__)
    // warning: offset of on non-standard-layout type 'test0::t_self' (aka 'test0') [-Winvalid-offsetof]
    #pragma clang diagnostic ignored "-Winvalid-offsetof"
    #pragma clang diagnostic push
    // Properties.h:521:82: warning: field 'm_Map' is uninitialized when used here [-Wuninitialized]
    #pragma clang diagnostic ignored "-Wuninitialized"

    #define vs2017_hack_inline          constexpr
    #define vs2017_hack_constepxr       constexpr
    #define vs2017_hack_protected       protected:

#elif defined(__GNUC__) || defined(__GNUG__)
    // warning: offsetof within non-standard-layout type 'test0::t_self' {aka 'test0'} is conditionally-supported [-Winvalid-offsetof]
    #pragma GCC diagnostic ignored "-Winvalid-offsetof"

    #define vs2017_hack_inline          constexpr
    #define vs2017_hack_constepxr       constexpr
    #define vs2017_hack_protected       protected:

#elif defined(_MSC_VER)
    #pragma warning( push )
    #pragma warning( disable : 4201)                        // warning C4201: nonstandard extension used: nameless struct/union

    #if _MSC_VER >= 1920                                    // If visual studio 2019 then...
        #define vs2017_hack_inline          constexpr
        #define vs2017_hack_constepxr       constexpr
        #define vs2017_hack_protected       protected:
    #else
        #define vs2017_hack_protected   public:             // This should not be needed. This is another bug from visual studio 2017
        #define vs2017_hack_inline          inline              // Failure of visual studio to compile using constexpr (this should be constexpr)
        #define vs2017_hack_constepxr                           // Failure of visual studio to compile using constexpr (this should be constexpr)
    #endif
#endif

//--------------------------------------------------------------------------------------------
// determines if type comes from a particular template class
// https://cukic.co/2019/03/15/template-meta-functions-for-detecting-template-instantiation/
//--------------------------------------------------------------------------------------------
template< template< typename... > typename T_BASE, typename    T_DERIVED > struct                is_specialized                            : std::false_type {};
template< template< typename... > typename T_BASE, typename... T_ARGS    > struct                is_specialized<T_BASE, T_BASE<T_ARGS...>> : std::true_type  {};
template< template< typename... > typename T_BASE, typename    T_DERIVED > constexpr static bool is_specialized_v = is_specialized<T_BASE, T_DERIVED>::value;

//--------------------------------------------------------------------------------------------
// a string view needed to compute the murmur hash
// http://szelei.me/constexpr-murmurhash/
//--------------------------------------------------------------------------------------------
class str_view 
{
public:
    constexpr str_view( const char* a, uint32_t size ) noexcept : m_pStr( a ), m_Size( size - 1 ) {}
    template<size_t N>
    constexpr str_view( const char( &a )[ N ] ) noexcept : m_pStr( a ), m_Size( N - 1 ) {}
    template<size_t N>
    constexpr str_view(const std::array<char,N>& a) noexcept : m_pStr(a.data()), m_Size(N - 1) {}
    constexpr str_view( const char* a, int& i ) noexcept 
              : m_pStr(a)
              , m_Size
              {   static_cast<std::size_t>
                  ([]( const char* a, int& i ) constexpr
                  { 
                     for( i=0; a[i] && a[i] !='/' && a[i] != '[' ; i++ ); 
                     return i;
                  }(a, i)) 
              }{}

    constexpr char          operator[]  ( std::size_t n )                           const noexcept { assert( n < m_Size ); return m_pStr[ n ]; }
    constexpr uint32_t      get_block   ( std::size_t BlockSize, const int idx )    const noexcept 
    {
        const auto     i  = ( BlockSize + idx ) * 4;
        const uint32_t b0 = m_pStr[ i ];
        const uint32_t b1 = m_pStr[ i + 1 ];
        const uint32_t b2 = m_pStr[ i + 2 ];
        const uint32_t b3 = m_pStr[ i + 3 ];
        return ( b3 << 24 ) | ( b2 << 16 ) | ( b1 << 8 ) | b0;
    }

    constexpr uint32_t      size        ( void )                const noexcept { return m_Size; }
    constexpr uint32_t      block_size  ( void )                const noexcept { return m_Size / 4; }
    constexpr char          tail        ( const int n )         const noexcept 
    {
        const int tail_size = m_Size % 4;
        return m_pStr[ m_Size - tail_size + n ];
    }

protected:
    const char*     m_pStr;
    uint32_t        m_Size;
};

//--------------------------------------------------------------------------------------------
// computes murmur hash
// http://szelei.me/constexpr-murmurhash/
//--------------------------------------------------------------------------------------------
constexpr uint32_t mm3_x86_32( str_view key, uint32_t seed = 0x9747b28c ) noexcept
{
    const uint32_t  c1 = 0xcc9e2d51;
    const uint32_t  c2 = 0x1b873593;

    uint32_t h1 = seed;
    {
        const int nblocks = static_cast<int>(key.block_size());
        for ( int i = -nblocks; i; i++ )
        {
            uint32_t k1 = key.get_block( nblocks, i );

            k1 *= c1;
            k1 = ( k1 << 15 ) | ( k1 >> ( 32 - 15 ) );
            k1 *= c2;

            h1 ^= k1;
            h1 = ( h1 << 13 ) | ( h1 >> ( 32 - 13 ) );
            h1 = h1 * 5 + 0xe6546b64;
        }
    }
    {
        uint32_t k1 = 0;
        switch ( key.size() & 3 )
        {
            case 3: k1 ^= key.tail( 2 ) << 16;
            case 2: k1 ^= key.tail( 1 ) << 8;
            case 1: k1 ^= key.tail( 0 );
                k1 *= c1;
                k1 = ( k1 << 15 ) | ( k1 >> ( 32 - 15 ) );
                k1 *= c2;
                h1 ^= k1;
        };
    }

    
    h1 ^= key.size();
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

//--------------------------------------------------------------------------------------------
// Checks if a type is valid 
// https://stackoverflow.com/questions/39816779/check-if-type-is-defined
//--------------------------------------------------------------------------------------------
namespace details
{
    template< class T, class E = void >
    struct is_defined : std::false_type {};

    template< class T >
    struct is_defined< T, std::enable_if_t< std::is_object<T>::value && !std::is_pointer<T>::value && (sizeof(T) > 0) > > : std::true_type{};
}

template< typename T > constexpr static bool is_defined_v = details::is_defined<T>::value;

/*
namespace details
{
    template< typename T, typename E = void >
    struct is_table_defined_v : std::false_type {};

    template< typename T >
    struct is_table_defined_v< T, std::enable_if_t< std::is_object<decltype(T::m_Table)>::value && !std::is_pointer<decltype(T::m_Table)>::value && (sizeof(decltype(T::m_Table)) > 0) > > : std::true_type{};
}

template< typename T > constexpr static bool is_table_defined_v = details::is_table_defined_v<T>::value;
*/

//--------------------------------------------------------------------------------------------
// Helpful when using variadic types
//--------------------------------------------------------------------------------------------
template< typename T > struct always_false : std::false_type {};

//--------------------------------------------------------------------------------------------
// Function traits helper 
//--------------------------------------------------------------------------------------------
template<class T> struct function_traits;

template<class T_RETURN, class... T_ARGS>
struct function_traits<T_RETURN( T_ARGS... )>
{
    using                           return_type     = T_RETURN;
    static constexpr std::size_t    arity_v         = sizeof...( T_ARGS );

    template <std::size_t T_INDEX>
    struct argument
    {
        static_assert( T_INDEX < arity_v, "Error: invalid parameter index." );
        using type = typename std::tuple_element<T_INDEX, std::tuple<T_ARGS...>>::type;
    };
};

template <typename T_RETURN, typename... T_ARGS> struct function_traits<T_RETURN(*)( T_ARGS... ) noexcept> : public function_traits<T_RETURN( T_ARGS... )> {};
template <typename T_RETURN, typename... T_ARGS> struct function_traits<T_RETURN(*)( T_ARGS... )         > : public function_traits<T_RETURN( T_ARGS... )> {};

//--------------------------------------------------------------------------------------------
// Configuration for the property system
//--------------------------------------------------------------------------------------------
namespace property
{
    struct table_entry;
    struct setup_entry;
    struct table;
    namespace settings { struct user_entry; }   // namespace for all the settings require for the property system
    namespace editor   {}                       // namespace for all things that have to do with the editor
}

#include "PropertyConfig.h"

//--------------------------------------------------------------------------------------------
// property system
//--------------------------------------------------------------------------------------------
namespace property
{
    //------------------------------------------------------------------------------------------
    // Variant type to index conversion
    //------------------------------------------------------------------------------------------
    namespace details
    {
        template< class T, class T_VARIANT >
        struct variant_t2i;

        template< class T, class... T_ARGS >
        struct variant_t2i<T, std::variant<T, T_ARGS...>> 
        {
            static const std::size_t value = 0;
        };

        template< class T, class U, class... T_ARGS >
        struct variant_t2i<T, std::variant<U, T_ARGS...>> 
        {
            static const std::size_t value = 1 + variant_t2i<T, std::variant<T_ARGS...>>::value;
        };
    }
    template< typename T_TYPE, typename T_VARIANT >
    constexpr static auto variant_t2i_v = details::variant_t2i< T_TYPE, T_VARIANT >::value;

    //--------------------------------------------------------------------------------------------
    // flags for the properties
    //--------------------------------------------------------------------------------------------
    namespace flags
    {
        union type 
        {
            std::size_t           m_Value{ 0 };                                         // We keep it size_t to make sure we shadow 100% the dynamic flags pointer and avoid endian issues
            struct 
            {
                      bool        m_isShowReadOnly  : 1                                 // When in display mode it will let the user knows to render the property as a read only
                                , m_isDontSave      : 1                                 // Properties with this flag wont be saved
                                , m_isDontShow      : 1                                 // Properties with this flags wont be shown to the user
                                , m_isScope         : 1;                                // When Enumerating in display mode it will let the user know if it is a scope
            };

            constexpr type      operator |  ( type X ) const noexcept { return{ m_Value |  X.m_Value }; }
            inline    type&     operator |= ( type X )       noexcept { return  m_Value |= X.m_Value, *this; }
        };

        static_assert( sizeof(type) == sizeof(std::size_t) );
        namespace details
        {
            constexpr type IS_SCOPE                     { 1 << 3 };                         // **Internal use only** Tells the user we are dealing with a scope
            constexpr type STATIC_MASK                  { (~std::size_t(0)) <<  4 };        // **Internal use only** Use to determine if we have a pointer or static flags
        }

        constexpr type SHOW_READONLY                    { 1 << 0 };                         // Property can not be edited other wise it will save and load just like any other
        constexpr type DONTSAVE                         { 1 << 1 };                         // Makes the property not save under on serialization
        constexpr type DONTSHOW                         { 1 << 2 };                         // Hides a property under enum display so it wont be shown 
        constexpr type DISABLE                          { DONTSAVE.m_Value | DONTSHOW.m_Value };    // Completely disables a property
    }

    //--------------------------------------------------------------------------------------------
    // macros that are used to help define the properties of a class
    //--------------------------------------------------------------------------------------------
    #define property_var( VAR )                         property::PropertyVar<decltype(std::declval<t_self>().VAR)>( #VAR, offsetof( t_self, VAR ) )
    #define property_var_fnbegin(NAME, TYPE)            property::setup_entry{ NAME, []( void* pSelf, TYPE& InOut, bool isRead, std::uint64_t ) noexcept        { t_self& Self = *reinterpret_cast<t_self*>(pSelf); 
    #define property_var_fnend()                        return true; } }

    #define property_list_fnbegin(NAME, TYPE)           property::setup_entry{ NAME, []( void* pSelf, TYPE& InOut, bool isRead, std::uint64_t Index ) noexcept  { t_self& Self = *reinterpret_cast<t_self*>(pSelf); 
    #define property_list_fnenum()                      return true; }, static_cast<std::size_t>(property::table_action_entry::offset_guard), []( void* pSelf, std::uint64_t& InOut, property::lists_cmd Cmd, std::array<std::uint64_t,4>& MemoryBlock ) noexcept  { t_self& Self = *reinterpret_cast<t_self*>(pSelf); (void)MemoryBlock;
    #define property_list_fnend()                       } }

    #define property_scope_begin(NAME)                  property::setup_entry{ NAME, []( void* pSelf, std::uint64_t ) noexcept -> std::optional<std::tuple<const property::table&,void*>> { using t_str_scope = details::string< details::getSize(NAME) >; constexpr static auto Name = t_str_scope::getArray(NAME); vs2017_hack_constepxr static const table_storage Storage
    #define property_scope_end()                        ; constexpr static auto Map = ::property::table_hash<Storage.entry_count_v>::InsertEntries( Storage ); vs2017_hack_inline static const property::table_hash<Storage.entry_count_v> Table{ Map, Storage, Name };return std::tuple<const property::table&,void*>{ Table, pSelf }; } }
    #define property_parent( PARENT_TYPE )              property::PropertyParent<t_self::PARENT_TYPE, const t_self*>()

    //--------------------------------------------------------------------------------------------
    // Macros used to define the properties. Use Opt-in when you don't want to use the virtual function in property::base class.
    // Use property_vend for classes that you used a property_vtable (These are classes must derived from property::base)
    // Use property_end  for classes where you did not use a property_vtable
    //--------------------------------------------------------------------------------------------
    namespace opin
    {
        template< typename T >
        struct def;
    }

    namespace details
    {
        constexpr int getSize( const char* pStr ) noexcept
        {
            int i=0;
            for( ; pStr[i]; ++i );
            return i+1;
        }

        template< int SIZE >
        constexpr const auto getArrayFrom( const char* pStr ) noexcept
        {
            std::array<char, SIZE> Final;
            for (int i=0; Final[i] = pStr[i]; ++i);
            return Final;
        }

        template< std::size_t N >
        struct string
        {
            template< typename T >
            static consteval auto getArray(T&& pStr) noexcept
            {
                return getArrayFrom<N>(pStr);
            }
        };
    }

    #define property_begin_name( CLASS_TYPE, NAME )     template<> struct property::opin::def<CLASS_TYPE>{ using t_self = CLASS_TYPE; using t_str = details::string< details::getSize(NAME) >; constexpr static auto m_Name = t_str::getArray(NAME);   vs2017_hack_inline static const table_storage m_Storage
    #define property_begin( CLASS_TYPE )                property_begin_name( CLASS_TYPE, #CLASS_TYPE )
    #define property_end()                              ; inline constexpr static auto m_Map = ::property::table_hash<m_Storage.entry_count_v>::InsertEntries( m_Storage ); vs2017_hack_inline static const property::table_hash<m_Storage.entry_count_v> m_Table{ m_Map, m_Storage, m_Name }; };
    #define property_vend_h( CLASS_TYPE )               property_end() inline const property::table& CLASS_TYPE::getPropertyVTable( void ) const noexcept { return property::opin::def<CLASS_TYPE>::m_Table; }
    #define property_vend_cpp( CLASS_TYPE )             property_end()        const property::table& CLASS_TYPE::getPropertyVTable( void ) const noexcept { return property::opin::def<CLASS_TYPE>::m_Table; }

    #define property_friend                             template<typename> friend struct property::opin::def;
    #define property_vtable()                           virtual const property::table& getPropertyVTable( void ) const noexcept override;  property_friend

    //--------------------------------------------------------------------------------------------
    // General container for the data of a property.
    // [SETTINGS] The user must provide a global type with a variant which contains all the types
    //            He wishes to use.
    //--------------------------------------------------------------------------------------------
    using data = property::settings::data_variant;

    //--------------------------------------------------------------------------------------------
    // ActionEntry that contains all data that a property needs (Name + Data)
    //--------------------------------------------------------------------------------------------
    using entry = std::pair<std::string, data>;

    //--------------------------------------------------------------------------------------------
    // getset function pointer variant
    //--------------------------------------------------------------------------------------------
    namespace details
    { 
        // Helper used to create the variant for all the getset function pointers
        template<typename... T>
        std::variant
        <
              bool                                                     (*)( void* pSelf, T& InOut, bool isRead, std::uint64_t Index ) noexcept...
            , std::optional<std::tuple< const property::table&, void*>>(*)( void* pSelf, std::uint64_t Index ) noexcept
        >
        CreateGetSetFunctionPtrVariantType( std::variant< T...> );
    }

    // Actual variant with all the getset function pointers
    using function_variant_getset = decltype( details::CreateGetSetFunctionPtrVariantType( std::declval<property::data>() ) );

    //--------------------------------------------------------------------------------------------
    // Special system function pointers
    //--------------------------------------------------------------------------------------------

    // Function pointer for lists. This block/function is the state machine for lists such vectors and arrays
    constexpr static const std::uint64_t lists_iterator_ends_v = ~std::uint64_t( 0 );
    enum class lists_cmd
    {
          WRITE_COUNT   // Notifies the list on how many entries it should be, some type of lists like arrays ignore this state.
        , READ_COUNT    // Gets the total count of entries in the list
        , READ_FIRST    // Sets the iterator to entry 0 if none then iterator is set to lists_iterator_ends_v
        , READ_NEXT     // Updates the iterator to point to the next entry if we have reach the end sets the iterator to lists_iterator_ends_v
    };
    using function_ptr_lists      = void(*)( void* pSelf, std::uint64_t& InOut, lists_cmd Cmd, std::array<std::uint64_t,4>& MemoryBlock ) noexcept;

    // Determines if a property is disable or not
    using function_ptr_dynamic_flags = flags::type(*)( const std::byte& Self ) noexcept;

    //--------------------------------------------------------------------------------------------
    // Find which property type this function pointer is referring to 
    //--------------------------------------------------------------------------------------------
    template< typename T_FUNCTION_PTR_GETSET >
    using vartype_from_functiongetset = std::decay_t<typename function_traits<T_FUNCTION_PTR_GETSET>::template argument<1>::type>;

    //--------------------------------------------------------------------------------------------
    // Table Property Entry used to run most of the logic of the system.
    //--------------------------------------------------------------------------------------------
    struct table_action_entry
    {
        constexpr static const auto offset_guard = std::numeric_limits<std::size_t>::max();
        constexpr table_action_entry( flags::type                F, function_variant_getset GetSet, function_ptr_lists PL, std::size_t  Off ) noexcept : m_Flags{F}, m_FunctionTypeGetSet{GetSet}, m_FunctionLists{PL}, m_Offset{Off}{}
        constexpr table_action_entry( function_ptr_dynamic_flags F, function_variant_getset GetSet, function_ptr_lists PL, std::size_t  Off ) noexcept : m_FunctionDynamicFlags{ F }, m_FunctionTypeGetSet { GetSet }, m_FunctionLists { PL }, m_Offset { Off }{}
        union
        {
            function_ptr_dynamic_flags      m_FunctionDynamicFlags;                         // Callback to determine if a property is disable or enable (nullptr || false is enable)
            flags::type                     m_Flags;
        };
        function_variant_getset             m_FunctionTypeGetSet;                           // Defines the type of the property and it contains a function pointer to get or set the property
        function_ptr_lists                  m_FunctionLists;                                // Function lists
        std::size_t                         m_Offset;                                       // Offset to the actual data when we deal with non virtual properties
    };

    //--------------------------------------------------------------------------------------------
    // Entry use to contain useful data for the user. 
    //--------------------------------------------------------------------------------------------
    struct table_entry : property::settings::user_entry
    {
        const char*                         m_pName;                                        // Name of the property
        std::uint32_t                       m_NameHash;                                     // Hash key used to index the entry 
    };

    //--------------------------------------------------------------------------------------------
    // Pack is a fast data structure used to get/set properties quickly
    // This is used when we are able to use properties as a set such when loading and saving
    // When the version of the data is not a concern.
    // This allows properties to get close to 1 hash lookup per property thanks to making things
    // relative. Farther optimizations of this structure is possible but becomes more complex.
    //--------------------------------------------------------------------------------------------
    struct pack
    {
        struct entry
        {
            property::data     m_Data          {};                      // Actual data 
            std::uint8_t       m_nPopPaths     { 0 };                   // Number of paths that we need to pop out to reach the right one
            bool               m_isArrayCount  { false };               // Determines if this entry is an array count
            std::uint8_t       m_nPaths        { 0 };                   // [DEBUG] This is not needed at runtime but it is useful for debugging
        };

        struct path
        {
            std::uint32_t       m_Key;
            std::uint64_t       m_Index;
        };

              pack          ( void )                                    noexcept { m_lPath.reserve(100); m_lEntry.reserve(100); }
        void  createEntry   ( void )                                    noexcept { m_lEntry.push_back({}); }
        void  pushPath      ( std::uint32_t Key, std::uint64_t Index )  noexcept { m_lEntry.back().m_nPaths++; m_lPath.push_back( {Key, Index} ); }
        void  popPath       ( void )                                    noexcept { m_lEntry.back().m_nPopPaths++; }
        auto& getData       ( void )                                    noexcept { return m_lEntry.back().m_Data; }

        std::vector<path>       m_lPath;
        std::vector<entry>      m_lEntry;
    };

    //--------------------------------------------------------------------------------------------
    // Setup definition for a property entry. This data is not store long term like this.
    //--------------------------------------------------------------------------------------------
    struct setup_entry : property::settings::user_entry
    {
        function_ptr_dynamic_flags          m_FunctionDynamicFlags  {nullptr};                              // Callback to get flags for this property
        flags::type                         m_Flags                 {0};                                    // If first bit is set true then it can not be a pointer so the flags must be static

        function_variant_getset             m_FunctionTypeGetSet;                                           // Defines the type of the property and it contains a function pointer to get or set the property
        function_ptr_lists                  m_FunctionLists         { nullptr };                            // Function lists
        std::size_t                         m_Offset                { table_action_entry::offset_guard };
        const char*                         m_pName;                                                        // Name of the property
        std::uint32_t                       m_NameHash;                                                     // Hash key used to index the entry 

        constexpr setup_entry( const char* pName, std::uint32_t NameHash, function_variant_getset Fn )
            : property::settings::user_entry    ()
            , m_FunctionTypeGetSet              ( Fn    )
            , m_pName                           ( pName )
            , m_NameHash                        ( NameHash  ) {}

        template< std::size_t N >
        constexpr setup_entry( const char( &pName )[ N ], function_variant_getset Fn, std::size_t Offset = table_action_entry::offset_guard, function_ptr_lists PtrLists = nullptr )
            : property::settings::user_entry    ()
            , m_FunctionTypeGetSet              ( Fn          )
            , m_FunctionLists                   ( PtrLists    )
            , m_Offset                          ( Offset      )
            , m_pName                           ( ((pName[0] == 'm' && pName[1] == '_')?&pName[2]:pName) )
            , m_NameHash                        ( mm3_x86_32( str_view{ m_pName, m_pName==pName ? N : N-2 } ) ) {}

        constexpr setup_entry DynamicFlags( function_ptr_dynamic_flags Callback ) const noexcept
        { 
            assert( m_Flags.m_Value == 0 && m_FunctionDynamicFlags == nullptr );
            setup_entry r = *this;
            r.m_FunctionDynamicFlags = Callback; 
            return r; 
        }

        constexpr setup_entry Flags( flags::type Flags ) const noexcept
        {
            assert( m_Flags.m_Value == 0 && m_FunctionDynamicFlags == nullptr );
            setup_entry r = *this;
            r.m_Flags.m_Value = Flags.m_Value | flags::details::STATIC_MASK.m_Value;
            return r;
        }

        template< std::size_t N >
        constexpr setup_entry Name( const char(&pName)[N] ) const noexcept
        {
            setup_entry r = *this;
            r.m_pName	  = pName;
            r.m_NameHash  = mm3_x86_32(pName);
            return r;
        }

        constexpr operator table_action_entry ( void ) const noexcept { if(m_FunctionDynamicFlags) return{ m_FunctionDynamicFlags, m_FunctionTypeGetSet, m_FunctionLists, m_Offset }; return{ m_Flags | flags::details::STATIC_MASK, m_FunctionTypeGetSet, m_FunctionLists, m_Offset }; }
        constexpr operator table_entry        ( void ) const noexcept { return{ *(static_cast<const property::settings::user_entry*>(this)), m_pName, m_NameHash }; }
    };

    //--------------------------------------------------------------------------------------------
    // basic table of the property system which allows to access the properties of a class instance
    //--------------------------------------------------------------------------------------------
    struct table
    {
        constexpr static auto map_size_factor_v = 4;

        using map_entry = std::pair<const property::table_action_entry*, uint32_t>;

        template< std::size_t N >
        constexpr table( const std::array<char, N>& Name, std::size_t Count, const table_action_entry* pEntries, const map_entry* pMap, const table_entry* pEntry ) noexcept
            : m_pActionEntries      { pEntries }
            , m_pMap                { pMap     }
            , m_pEntry              { pEntry   }
            , m_Count               { Count    }
            , m_pName               { Name.data() }
            , m_NameHash            { mm3_x86_32(Name) }
            {}

        constexpr table( std::size_t Count, const table_action_entry* pEntries, const map_entry* pMap, const table_entry* pEntry ) noexcept
            : m_pActionEntries      { pEntries }
            , m_pMap                { pMap     }
            , m_pEntry              { pEntry   }
            , m_Count               { Count    }
            , m_pName               { nullptr  }
            , m_NameHash            { 0        }
            {}

        table() = delete;

        constexpr const table_action_entry* find( std::uint32_t H ) const noexcept
        {
            if(m_pMap == nullptr) return nullptr;
            const auto      n       = m_Count * map_size_factor_v;
            const auto*     pPair   = &m_pMap[ H % n ];
            if( pPair->first == nullptr || pPair->second == H ) return pPair->first;
            const auto      pEnd    = &m_pMap[ n ];
            do 
            {
                if ( ++pPair == pEnd ) pPair = &m_pMap[ 0 ];
                if ( pPair->first == nullptr || pPair->second == H ) return pPair->first;
            } while( true );
        }

        constexpr const auto getIndexFromEntry( const table_action_entry& ActionEntry ) const noexcept
        {
            const auto Index = static_cast<std::size_t>(&ActionEntry - m_pActionEntries);
            assert( Index < m_Count );
            return Index;
        }

        constexpr const auto getIndexFromEntry( const table_entry& Entry ) const noexcept
        {
            const auto Index = static_cast<std::size_t>( &Entry - m_pEntry );
            assert( Index < m_Count );
            return Index;
        }

        const table_action_entry*   const   m_pActionEntries;   // List of entries (system side)
        const map_entry*            const   m_pMap;             // Hash map to the property entries
        const table_entry*          const   m_pEntry;           // List of entries (user side)
        const std::size_t                   m_Count;            // Number of entries
        const char*                 const   m_pName;            // Name of the table
        const std::uint32_t                 m_NameHash;         // Hash of the table
    };

    //--------------------------------------------------------------------------------------------
    // Structure used to avoid allocations
    //--------------------------------------------------------------------------------------------
    template< typename... T_ARGS >
    struct table_storage
    {
        constexpr static auto entry_count_v = sizeof...( T_ARGS );

        constexpr table_storage( T_ARGS&&... Arg ) noexcept
            : m_ActionEntry { std::forward<T_ARGS>( Arg )... }
            , m_UserEntry   { std::forward<T_ARGS>( Arg )... }
        {}

        const std::array< property::table_action_entry, entry_count_v>  m_ActionEntry;
        const std::array< property::table_entry,        entry_count_v>  m_UserEntry;
    };

    template<>
    struct table_storage<>
    {
        constexpr static auto entry_count_v = 0;
    };

    //--------------------------------------------------------------------------------------------
    // property storage class used to avoid using dynamic memory to keep a record of the properties
    //--------------------------------------------------------------------------------------------
    template< std::size_t entry_count_v >
    struct table_hash : table
    {
        constexpr static auto map_size_v                = entry_count_v * map_size_factor_v;

        template< typename T >
        constexpr static auto InsertEntries( const T& Storage ) noexcept
        {
            std::array< map_entry, map_size_v> Map{};
            std::fill(Map.begin(),Map.end(), std::pair{nullptr,0});

            auto const pEnd = &Map.data()[ map_size_v ];
            for( std::size_t i=0; i< Storage.m_UserEntry.size(); ++i )
            {
                auto&       U       = Storage.m_UserEntry[i];
                auto const  Hash    = U.m_NameHash;
                auto        pPair   = &Map[ Hash % map_size_v ];
                do
                {
                    if( pPair->first == nullptr )
                    {
                        pPair->first  = &Storage.m_ActionEntry[i];
                        pPair->second = Hash; 
                        break;
                    }

                    // Check for duplicates, if this happens please change the property name (bad luck)
                    assert( pPair->second != U.m_NameHash );
                    ++pPair;
                    if ( pPair == pEnd ) pPair = Map.data();
                } while ( true );
            }

            return Map;
        }

        template< typename T, std::size_t N >
        constexpr table_hash( const std::array< map_entry, map_size_v>& Map, T& Storage, const std::array<char, N>& Name ) noexcept
            : table         { Name, entry_count_v, Storage.m_ActionEntry.data(), Map.data(), Storage.m_UserEntry.data() }
            {}
    };

    template<>
    struct table_hash<0> : table
    {
        template< typename T>
        constexpr static auto InsertEntries(const T&) noexcept
        {
            return std::array< map_entry, 0>{};
        }

        template< typename T, std::size_t N >
        constexpr table_hash(const std::array< map_entry, 0>&, const T&, const std::array<char, N>& Name ) noexcept
            : table{ Name, 0, nullptr, nullptr, nullptr }
        {}
    };

    //--------------------------------------------------------------------------------------------
    // interface needed for the property system to work. 
    //--------------------------------------------------------------------------------------------
    struct base
    {
        virtual const property::table& getPropertyVTable( void ) const noexcept = 0;
        virtual                       ~base             ( void )       noexcept = default;
    };

    //--------------------------------------------------------------------------------------------
    // Basic functions to get the property table
    //--------------------------------------------------------------------------------------------
    template< typename T > constexpr    bool            isValidTable    ( void  ) noexcept { return is_defined_v<property::opin::def<std::decay_t<T>>> ? true : std::is_base_of_v< property::base, std::decay_t<T>>; }
    template< typename T > constexpr    const table&    getTableByType  ( void  ) noexcept { return property::opin::def<std::decay_t<T>>::m_Table;  }
    template< typename T > constexpr    const table&    getTable        ( T&& A ) noexcept
    { 
        // If it does not compiled most likely is becasue it can not find the property table for the given type.
        
             if constexpr ( std::is_base_of_v< property::base, std::decay_t<T> > ) return A.getPropertyVTable();
        else if constexpr (is_defined_v<property::opin::def<std::decay_t<T>>>   ) { (void)A; return property::opin::def<std::decay_t<T>>::m_Table; }
    }
    
    //--------------------------------------------------------------------------------------------

    using enum_callback_fn = void( std::string_view PropertyName, property::data&& Data, const property::table& Table, std::size_t Index, property::flags::type Flags );

    //--------------------------------------------------------------------------------------------
    // empty type
    //--------------------------------------------------------------------------------------------
    struct empty : base 
    {
        property_vtable();
    };
    inline static empty empty_v;

    //--------------------------------------------------------------------------------------------
    // Details
    //--------------------------------------------------------------------------------------------
    namespace details
    {
        //--------------------------------------------------------------------------------------------
        // System get/set functions 
        //--------------------------------------------------------------------------------------------

        // Regular atomic properties
        template< typename T > constexpr
        bool SystemVarGetSet(void* pSelf, T& InOut, bool isRead, std::uint64_t) noexcept
        {
            auto& Var = *reinterpret_cast<T*>(pSelf);
            if (isRead)      InOut = Var;
            else               Var = InOut;
            return true;
        }

        // System Var List get/set
        template< typename T, typename T_ENTRY > constexpr
        bool SystemListVarGetSet(void* pSelf, T_ENTRY& InOut, bool isRead, std::uint64_t Index) noexcept
        {
            auto& Var = *reinterpret_cast<T*>(pSelf);
            auto  i   = static_cast<std::uint32_t>(Index);
            if (i >= Var.size()) return false;
            if (isRead)      InOut  = Var[i];
            else             Var[i] = InOut;
            return true;
        }

        // System Var List state machine function
        template< typename T > constexpr
        void SystemLists(void* pSelf, std::uint64_t& InOut, lists_cmd Cmd, std::array<uint64_t, 4>&) noexcept
        {
            auto& Var = *reinterpret_cast<T*>(pSelf);
            if constexpr (is_specialized_v<std::vector, T>)
            {
                switch (Cmd)
                {
                case lists_cmd::READ_COUNT:   InOut = Var.size(); break;
                case lists_cmd::WRITE_COUNT:  Var.resize(static_cast<int>(InOut)); break;
                case lists_cmd::READ_FIRST:   InOut = (Var.size() == 0) ? lists_iterator_ends_v : 0; break;
                case lists_cmd::READ_NEXT:    if (++InOut == Var.size()) InOut = lists_iterator_ends_v; break;
                default: assert(false);
                }
            }
            else // this version should be the array
            {
                switch (Cmd)
                {
                case lists_cmd::READ_COUNT:   InOut = Var.size(); break;
                case lists_cmd::WRITE_COUNT:  break;
                case lists_cmd::READ_FIRST:   InOut = 0; break;
                case lists_cmd::READ_NEXT:    if (++InOut == Var.size()) InOut = lists_iterator_ends_v; break;
                default: assert(false);
                }
            }
        }

        // System Table get/set
        template< typename T > constexpr
        std::optional<std::tuple<const property::table&, void*>> SystemVarTableGetSet(void* pSelf, std::uint64_t) noexcept
        {
            auto& Var = *reinterpret_cast<T*>(pSelf);
            return std::tuple<const property::table&, void*>{ property::getTable(Var), &Var };
        }

        // System Table List get/set
        template< typename T > constexpr
        std::optional<std::tuple<const property::table&, void*>> SystemListTableGetSet(void* pSelf, std::uint64_t Index) noexcept
        {
            auto&      Var = *reinterpret_cast<T*>(pSelf);
            const auto i   =  static_cast<std::uint32_t>(Index);
            if (Var.size() <= i) return std::nullopt;
            return std::tuple<const property::table&, void*>{ property::getTable(Var[i]), &Var[i] };
        }

        // System Table List get/set
        template< typename T > constexpr
        std::optional<std::tuple<const property::table&, void*>> SystemListTableGetSetForPointers(void* pSelf, std::uint64_t Index) noexcept
        {
            auto&      Var = *reinterpret_cast<T*>(pSelf);
            const auto i   =  static_cast<std::uint32_t>(Index);
            if (Var.size() <= i)   return std::nullopt;
            if (Var[i] == nullptr) return std::tuple<const property::table&, void*>{ property::getTable(empty_v), &empty_v };
            return std::tuple<const property::table&, void*>{ property::getTable(*Var[i]), &(*Var[i]) };
        }

        //--------------------------------------------------------------------------------------------
        // System Functions to read and write the supported properties
        //--------------------------------------------------------------------------------------------

        // Handle lists
        template< typename T_VAR, std::size_t N > constexpr
        auto HandleListsPropertyVar(const char(&pName)[N], std::size_t Offset) noexcept
        {
            using var = std::decay_t<T_VAR>;
            using val = std::remove_reference_t<decltype((*(var*)0)[0])>;
            using e   = std::decay_t<val>;

            if constexpr (      is_specialized_v<std::unique_ptr, val> 
                             || is_specialized_v<std::shared_ptr, val>
                             || std::is_pointer_v<val> )                return property::setup_entry(pName, details::SystemListTableGetSetForPointers<var>,  Offset, SystemLists<var>);
            else if constexpr (isValidTable<e>())                       return property::setup_entry(pName, details::SystemListTableGetSet<var>,  Offset, SystemLists<var>);
            else                                                        return property::setup_entry(pName, details::SystemListVarGetSet<var, e>, Offset, SystemLists<var>);
        }

        // help to determine if type comes from a std::array
        template< typename T, std::size_t V >
        constexpr bool is_stdarray(std::array<T, V>&&) { return true; }
    }

    //--------------------------------------------------------------------------------------------
    // PropertyVar / PropertyParent These are the generic functions when defining properties
    //--------------------------------------------------------------------------------------------

    // Deal with std::arrays
    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< std::is_same_v<decltype(details::is_stdarray(std::decay_t<T_VAR>{})), bool>, property::setup_entry >
    PropertyVar( const char( &pName )[ N ], std::size_t Offset ) noexcept { return details::HandleListsPropertyVar<T_VAR>( pName, Offset ); }

    // deal with std::vectors
    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< is_specialized_v<std::vector, std::decay_t<T_VAR>>, property::setup_entry >
    PropertyVar( const char( &pName )[ N ], std::size_t Offset ) noexcept { return details::HandleListsPropertyVar<T_VAR>( pName, Offset ); }

    // deal with regular properties such int and such
    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< std::is_same_v< decltype( data().emplace<std::decay_t<T_VAR>>() ), decltype( data().emplace<std::decay_t<T_VAR>>() )>, property::setup_entry>
    PropertyVar( const char( &pName )[ N ], std::size_t Offset ) noexcept 
    {
        using var = std::decay_t<T_VAR>;
        return property::setup_entry( pName, details::SystemVarGetSet<var>, Offset ); 
    }

    // deal with atomic but property::table base properties
    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< isValidTable<std::decay_t<T_VAR>>(), property::setup_entry >
    PropertyVar( const char( &pName )[ N ], std::size_t Offset ) noexcept 
    {
        using var = std::decay_t<T_VAR>;
        return property::setup_entry( pName, details::SystemVarTableGetSet<var>, Offset ); 
    }

    //--------------------------------------------------------------------------------------------------------------------
    // Deal with std::smart pointers std::unique_ptr<> or std::shared_ptr to an object with property::base
    //--------------------------------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T > constexpr
        std::optional<std::tuple<const property::table&, void*>> UniquePtrGetSet(void* pSelf, std::uint64_t) noexcept
        {
            auto& SmartPtr = *reinterpret_cast<T*>(pSelf);
            if (SmartPtr == nullptr)
            {
                return std::tuple<const property::table&, void*>{ property::getTable(empty_v), &empty_v };
            }
            else
            {
                auto& Table = property::getTable(*SmartPtr);
                return std::tuple<const property::table&, void*>{ Table, SmartPtr.get() };
            }
        }
    }

    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< is_specialized_v<std::unique_ptr, std::decay_t<T_VAR>> 
                   || is_specialized_v<std::shared_ptr, std::decay_t<T_VAR>>, property::setup_entry >
    PropertyVar(const char(&pName)[N], int Offset) noexcept
    {
        using var = std::decay_t<T_VAR>;
        return property::setup_entry(pName, details::UniquePtrGetSet<var>, Offset);
    }

    //--------------------------------------------------------------------------------------------------------------------
    // Deal with regular pointers to an object with property::base
    //--------------------------------------------------------------------------------------------------------------------
    namespace details
    {
        template< typename T > constexpr
        std::optional<std::tuple<const property::table&, void*>> RegularPtrGetSet(void* pSelf, std::uint64_t) noexcept
        {
            auto  pRegularPtr = reinterpret_cast<T*>(pSelf);
            if (*pRegularPtr == nullptr)
            {
                return std::tuple<const property::table&, void*>{ property::getTable(empty_v), &empty_v };
            }
            else
            {
                auto& Table = property::getTable(**pRegularPtr);
                return std::tuple<const property::table&, void*>{ Table, * pRegularPtr };
            }
        }
    }

    template< typename T_VAR, std::size_t N > constexpr
    std::enable_if_t< std::is_pointer_v<T_VAR>, property::setup_entry >
    PropertyVar(const char(&pName)[N], int Offset) noexcept
    {
        using var = std::decay_t<T_VAR>;
        return property::setup_entry(pName, details::RegularPtrGetSet<var>, Offset);
    }


    //--------------------------------------------------------------------------------------------
    // Function used to create the parent entry
    //--------------------------------------------------------------------------------------------
    template< typename T_PARENT, typename T_SELF > constexpr
    auto PropertyParent() noexcept
    {
        const property::table& ParentTable = property::getTableByType<T_PARENT>();
        return property::setup_entry
        (
            ParentTable.m_pName
            , ParentTable.m_NameHash
            , []( void* pSelf, std::uint64_t ) vs2017_hack_constepxr noexcept -> std::optional<std::tuple<const property::table&,void*>>
            { 
                auto s = reinterpret_cast<T_SELF>(pSelf);               // C++17 standard says that no reinterpret cast are allowed in constexpr however clang/gcc are fine with this, so am I. :-)
                auto p = static_cast<const T_PARENT*>( s );
                return std::tuple<const property::table&,void*>
                {
                    property::getTableByType<T_PARENT>()
                    , (void*)p
                }; 
            } 
        );
    }

    namespace details
    {
        //--------------------------------------------------------------------------------------------
        // Move the this pointer to the variable as expected
        //--------------------------------------------------------------------------------------------
        constexpr
        void* HandleBasePointer( void* pBase, std::size_t Offset ) noexcept
        {
            if( Offset == table_action_entry::offset_guard ) return pBase;
            return reinterpret_cast<std::byte*>(pBase) + Offset;            
        }

        //--------------------------------------------------------------------------------------------
        // Example of a function that can display all the properties of any class with properties
        //--------------------------------------------------------------------------------------------
        template< bool T_DISPLAY > inline 
        void EnumRecursive( 
              const property::table&    Table
            , void*                     pBase
            , std::array<char, 256>&    NameString
            , int                       StringIndex
            , std::function<enum_callback_fn>& CallBack ) noexcept
        {
            assert( pBase );

            for( size_t i=0; i<Table.m_Count; ++i)
            {
                const auto& Entry = Table.m_pActionEntries[i];
                const auto  Flags = (Entry.m_Flags.m_Value&flags::details::STATIC_MASK.m_Value)==flags::details::STATIC_MASK.m_Value 
                                    ? Entry.m_Flags 
                                    : Entry.m_FunctionDynamicFlags( *reinterpret_cast<std::byte*>(pBase) );

                //
                // Handle Flags
                //
                if constexpr ( T_DISPLAY ) { if( Flags.m_isDontShow ) continue; }
                else                       { if( Flags.m_isDontSave ) continue; }

                //
                // Handle simple entries
                //
                std::uint64_t Index = lists_iterator_ends_v;
                const auto HandleSimpleEntries = [&]( auto&& FunctionGetSet ) constexpr noexcept
                { 
                    using fn_getsettype = std::decay_t<decltype(FunctionGetSet)>;

                    if constexpr ( std::is_same_v<fn_getsettype, std::optional<std::tuple< const property::table&, void*>>(*)( void* pSelf, std::uint64_t Index ) noexcept> )
                    {
                        const auto Optional = FunctionGetSet( HandleBasePointer(pBase, Entry.m_Offset), Index );
                        assert( Optional != std::nullopt );

                        const auto& [ NewTable, pNewBase ] = *Optional;
                        const auto  EntryIndex = Table.getIndexFromEntry( Entry );
                        const auto& TableEntry = Table.m_pEntry[ EntryIndex ];
                        const auto  StrAdded   = ( Index == lists_iterator_ends_v ) ? 
                            snprintf( &NameString[ StringIndex ], NameString.max_size() - StringIndex, "%s/",       TableEntry.m_pName )
                          : snprintf( &NameString[ StringIndex ], NameString.max_size() - StringIndex, "%s[%" PRIu64 "]/", TableEntry.m_pName, Index );

                        if constexpr ( T_DISPLAY ) if ( Index == lists_iterator_ends_v )
                        {
                            // Deal with a new scope let the user know
                            CallBack( { &NameString[ 0 ], static_cast<std::size_t>( StrAdded + StringIndex - 1 ) }, {}, Table, EntryIndex, Flags | flags::details::IS_SCOPE );
                        }

                        EnumRecursive<T_DISPLAY>( NewTable, pNewBase, NameString, StringIndex + StrAdded, CallBack );
                    }
                    else
                    {
                        vartype_from_functiongetset<fn_getsettype> Data;
                        const auto  Ret        = FunctionGetSet( HandleBasePointer(pBase, Entry.m_Offset), Data, true, Index );
                        assert(Ret);

                        const auto  EntryIndex = Table.getIndexFromEntry( Entry );
                        const auto& TableEntry = Table.m_pEntry[ EntryIndex ];
                        const auto  StrAdded   = ( Index == lists_iterator_ends_v ) ? 
                            snprintf( &NameString[ StringIndex ], NameString.max_size() - StringIndex, "%s",       TableEntry.m_pName )
                          : snprintf( &NameString[ StringIndex ], NameString.max_size() - StringIndex, "%s[%" PRIu64 "]", TableEntry.m_pName, Index );
                        CallBack( { &NameString[0], static_cast<std::size_t>(StrAdded) + StringIndex }, data { Data }, Table, EntryIndex, Flags );
                    }
                };

                //
                // Decide if we need to deal with lists or with simple entries
                //
                if( Entry.m_FunctionLists )
                {
                    std::array<uint64_t, 4> MemoryBlock;
                    auto pTheBase = HandleBasePointer( pBase, Entry.m_Offset );

                    // Handle the count
                    {
                        std::uint64_t Count;
                        Entry.m_FunctionLists( pTheBase, Count, lists_cmd::READ_COUNT, MemoryBlock );
                        if( Count )
                        {
                            // Deal with the count property for the list first
                            const auto  EntryIndex = Table.getIndexFromEntry( Entry );
                            const auto& TableEntry = Table.m_pEntry[ EntryIndex ];
                            const auto  StrAdded   = snprintf( &NameString[ StringIndex ], NameString.max_size() - StringIndex, "%s[]", TableEntry.m_pName );
                            CallBack( { &NameString[ 0 ], static_cast<std::size_t>( StrAdded ) + StringIndex }, { static_cast<int>( Count ) }, Table, EntryIndex, Flags | flags::details::IS_SCOPE );
                        }

                        // Go trough all the entries in the list
                        // The iterator is 64bits which allows the property to utilize like an index or like a pointer
                        Entry.m_FunctionLists(pTheBase, Index, lists_cmd::READ_FIRST, MemoryBlock);
                        assert((Count && Index != lists_iterator_ends_v) || (Count == 0 && Index == lists_iterator_ends_v));
                    }

                    while( Index != lists_iterator_ends_v ) 
                    {
                        std::visit( HandleSimpleEntries, Entry.m_FunctionTypeGetSet );
                        Entry.m_FunctionLists( pTheBase, Index, lists_cmd::READ_NEXT, MemoryBlock );
                    }
                }
                else
                {
                    std::visit( HandleSimpleEntries, Entry.m_FunctionTypeGetSet );
                }
            }
        }

        //--------------------------------------------------------------------------------------------

        template< bool T_DISPLAY > inline
        void Enum( const table& Table, void* pClassInstance, std::function<enum_callback_fn>& Callback ) noexcept
        {
            std::array<char, 256> Buffer;
            const int             StringIndex = ( Table.m_pName ) ? snprintf( &Buffer[ 0 ], Buffer.max_size(), "%s/", Table.m_pName ) : 0;

            property::details::EnumRecursive<T_DISPLAY>( Table, pClassInstance, Buffer, StringIndex, Callback );
        }

        //--------------------------------------------------------------------------------------------
        // Example of a function that can display all the properties of any class with properties
        //--------------------------------------------------------------------------------------------
        inline
        void PackRecursive(
          const property::table&    Table
        , void*                     pBase
        , pack&                     WorkingPack ) noexcept
        {
            assert( pBase );

            for ( std::size_t i = 0; i < Table.m_Count; ++i )
            {
                const auto& Entry = Table.m_pActionEntries[ i ];
                const auto  Flags = ((Entry.m_Flags.m_Value&flags::details::STATIC_MASK.m_Value)==flags::details::STATIC_MASK.m_Value)
                                    ? Entry.m_Flags 
                                    : Entry.m_FunctionDynamicFlags( *reinterpret_cast<std::byte*>( pBase ) );

                //
                // Handle flags
                //
                if ( Flags.m_isDontSave ) continue;

                //
                // Handle simple entries
                //
                std::uint64_t   Index               = lists_iterator_ends_v;
                const auto      HandleSimpleEntries = [ & ]( auto&& FunctionGetSet ) constexpr noexcept
                {
                    using fn_getsettype = std::decay_t<decltype( FunctionGetSet )>;

                    if constexpr ( std::is_same_v<fn_getsettype, std::optional<std::tuple< const property::table&, void*>>( *)( void* pSelf, std::uint64_t Index ) noexcept> )
                    {
                        const auto Optional = FunctionGetSet( HandleBasePointer( pBase, Entry.m_Offset ), Index );
                        assert( Optional != std::nullopt );

                        const auto&[ NewTable, pNewBase ] = *Optional;
                        if (NewTable.m_Count)
                        {
                            const auto& TableEntry = Table.m_pEntry[Table.getIndexFromEntry(Entry)];

                            WorkingPack.pushPath(TableEntry.m_NameHash, Index);
                            PackRecursive(NewTable, pNewBase, WorkingPack);

                            // Keep track of how many paths we need to pop
                            WorkingPack.popPath();
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        const auto  Ret = FunctionGetSet( HandleBasePointer( pBase, Entry.m_Offset )
                                                          , WorkingPack.getData().emplace<vartype_from_functiongetset<fn_getsettype>>()
                                                          , true, Index );
                        assert( Ret );

                        const auto& TableEntry = Table.m_pEntry[ Table.getIndexFromEntry( Entry ) ];
                        WorkingPack.pushPath( TableEntry.m_NameHash, Index );
                    }

                    return true;
                };

                //
                // Decide if we need to deal with lists or with simple entries
                //
                if ( Entry.m_FunctionLists )
                {
                    std::array<uint64_t, 4> MemoryBlock;
                    auto                    pTheBase        = HandleBasePointer( pBase, Entry.m_Offset );

                    // Handle the count
                    {
                        std::uint64_t Count;
                        Entry.m_FunctionLists( pTheBase, Count, lists_cmd::READ_COUNT, MemoryBlock );
                        if ( Count != lists_iterator_ends_v )
                        {
                            // Deal with the count property for the list first
                            const auto  EntryIndex = Table.getIndexFromEntry( Entry );
                            const auto& TableEntry = Table.m_pEntry[ EntryIndex ];
                            WorkingPack.pushPath( TableEntry.m_NameHash, lists_iterator_ends_v );
                            WorkingPack.getData() = static_cast<int>( Count );
                            WorkingPack.m_lEntry.back().m_isArrayCount = true;
                            WorkingPack.createEntry();
                        }
                    }

                    // Go trough all the entries in the list
                    // The iterator is 64bits which allows the property to utilize like an index or like a pointer
                    Entry.m_FunctionLists( pTheBase, Index, lists_cmd::READ_FIRST, MemoryBlock );
                    while ( Index != lists_iterator_ends_v )
                    {
                        const auto ientry = WorkingPack.m_lEntry.size();
                        if (std::visit(HandleSimpleEntries, Entry.m_FunctionTypeGetSet))
                        {
                            // If we did not add an entry yet lets add it
                            if (ientry == WorkingPack.m_lEntry.size()) WorkingPack.createEntry();
                        }

                        Entry.m_FunctionLists(pTheBase, Index, lists_cmd::READ_NEXT, MemoryBlock);
                    }
                }
                else
                {
                    const auto icurrent = WorkingPack.m_lEntry.size();
                    if( std::visit( HandleSimpleEntries, Entry.m_FunctionTypeGetSet ) )
                        if( icurrent == WorkingPack.m_lEntry.size() ) WorkingPack.createEntry();
                }
            }
        }

        //--------------------------------------------------------------------------------------------
        // Example of a function that can display all the properties of any class with properties
        //--------------------------------------------------------------------------------------------
        inline
        int UnpackRecursive(
            const property::table&      Table
            , void*                     pBase
            , pack&                     WorkingPack
            , std::size_t&              iCurrentEntry
            , int&                      iCurrentPath ) noexcept
        {
            if (Table.m_Count == 0)
            {
                assert(false);
                iCurrentEntry++;
                return 0;
            }

            do 
            {
                // Variable which allows us to control the state machine of the code
                //  When nPop <   0 means we are ready to move to next node
                //  When nPop ==  0 means we need to deal with the current node
                //  When nPop >   0 means we need to keep popping paths
                int  nPop = -1;

                //
                // Find the entry in the table
                //
                const auto H = WorkingPack.m_lPath[ iCurrentPath ].m_Key;
                assert( Table.find( H ) );
                auto& Entry = *Table.find( H );

                //
                // If the property happens to be disable then out of luck
                // This can cause properties to lose their relative position 
                //
                //TODO: FIx assert -> assert( false == ( Entry.m_FunctionDynamicFlags && Entry.m_FunctionDynamicFlags( *reinterpret_cast<std::byte*>( pBase ) )) );

                //
                // If we are dealing with list and we have account make sure to tell the list
                //
                if ( Entry.m_FunctionLists && WorkingPack.m_lEntry[ iCurrentEntry ].m_isArrayCount )
                {
                    std::array< uint64_t, 4 > MemoryBlock;
                    auto Count = static_cast<std::uint64_t>( std::get<int>( WorkingPack.m_lEntry[ iCurrentEntry ].m_Data ) );
                    Entry.m_FunctionLists( HandleBasePointer( pBase, Entry.m_Offset ), Count, lists_cmd::WRITE_COUNT, MemoryBlock );
                }
                else
                {
                SHORT_CUT:
                    bool bSuccess = true;
                    std::visit( [&]( auto&& FunctionGetSet ) constexpr noexcept
                    {
                        using fnptr_getsettype = std::decay_t<decltype( FunctionGetSet )>;

                        if constexpr ( std::is_same_v<fnptr_getsettype, std::optional<std::tuple< const property::table&, void*>>(*)( void* pSelf, std::uint64_t Index ) noexcept> )
                        {
                            const auto Optional = FunctionGetSet( HandleBasePointer( pBase, Entry.m_Offset ), WorkingPack.m_lPath[ iCurrentPath ].m_Index );
                            if ( Optional == std::nullopt ) bSuccess = false;
                            else
                            {
                                const auto&[ NewTable, pNewBase ] = *Optional;
                                nPop = UnpackRecursive( NewTable, pNewBase, WorkingPack, iCurrentEntry, ++iCurrentPath );
                            }
                        }
                        else
                        {
                            bSuccess = FunctionGetSet( HandleBasePointer( pBase, Entry.m_Offset )
                                    , std::get<vartype_from_functiongetset<fnptr_getsettype>>( WorkingPack.m_lEntry[ iCurrentEntry ].m_Data )
                                    , false
                                    , WorkingPack.m_lPath[ iCurrentPath ].m_Index );
                        }
                    }
                    , Entry.m_FunctionTypeGetSet );

                    // TODO: Report unsuccessful property setters
                    assert(bSuccess);

                    // If we are popping then lets continue to do so
                    if ( nPop > 0 ) return nPop - 1;
                }

                //
                // Next entry
                //
                assert( nPop <= 0 );
                if( nPop < 0 )
                {
                    iCurrentEntry++;
                    if( iCurrentEntry == WorkingPack.m_lEntry.size() ) break;

                    // We still more data to process then update the path
                    iCurrentPath++;

                    // See if we have to pop
                    if ( WorkingPack.m_lEntry[ iCurrentEntry ].m_nPopPaths )
                        return WorkingPack.m_lEntry[ iCurrentEntry ].m_nPopPaths - 1;

                    // if this one is an array/list and we are the same as before
                    if( WorkingPack.m_lPath[iCurrentPath].m_Key == H ) 
                    {
                        assert(WorkingPack.m_lPath[iCurrentPath].m_Index != lists_iterator_ends_v );
                        assert(nPop==-1);
                        goto SHORT_CUT;
                    }
                }
                else if ( iCurrentEntry == WorkingPack.m_lEntry.size() ) break;

            } while( true );

            return 0;
        }

        //--------------------------------------------------------------------------------------------
        // Recursive Query set/get a property
        //--------------------------------------------------------------------------------------------
        template< bool T_IS_READ > inline 
        bool RecursivePropertyQuery( const property::table& Table, void* pBase, const char* pName, data& Data ) noexcept
        {
            assert(pBase && pName);

            // Get the entry
            int e{};
            auto pEntry = Table.find( mm3_x86_32( { pName, e } ) );
            if( pEntry == nullptr ) return false;
            const auto& Entry = *pEntry;

            // If the property happens to be disable then out of luck
            if constexpr ( T_IS_READ )
            {
                // You can always read anything... so far...
            }
            else 
            {
                const auto  Flags = ((Entry.m_Flags.m_Value&flags::details::STATIC_MASK.m_Value)==flags::details::STATIC_MASK.m_Value) 
                                    ? Entry.m_Flags 
                                    : Entry.m_FunctionDynamicFlags( *reinterpret_cast<std::byte*>( pBase ) );

                // When writing you can not do that for read only properties
                if( Flags.m_isShowReadOnly ) return false;
            }

            //
            // Handle simple entries
            //
            bool            bSuccess        = false;
            std::uint64_t   Index           = lists_iterator_ends_v;
            auto            ProcessEntry    = [&]( auto&& FunctionGetSet ) constexpr noexcept
            {
                using fnptr_getsettype = std::decay_t<decltype( FunctionGetSet )>;

                if constexpr ( std::is_same_v<fnptr_getsettype, std::optional<std::tuple< const property::table&, void*>>(*)( void* pSelf, std::uint64_t Index ) noexcept> )
                {
                    const auto Optional = FunctionGetSet( HandleBasePointer(pBase, Entry.m_Offset), Index );
                    if( Optional == std::nullopt ) bSuccess = false;
                    else
                    {
                        const auto&[ NewTable, pNewBase ] = *Optional;
                        bSuccess = RecursivePropertyQuery<T_IS_READ>( NewTable, pNewBase, &pName[ e + 1 ], Data );
                    }
                }
                else
                {
                    using var_type = vartype_from_functiongetset<fnptr_getsettype>;
                    if constexpr ( T_IS_READ ) bSuccess = FunctionGetSet( HandleBasePointer(pBase, Entry.m_Offset), Data.emplace<var_type>(),   T_IS_READ, Index );
                    else                       bSuccess = (variant_t2i_v<var_type, property::settings::data_variant> == Data.index() ) && FunctionGetSet( HandleBasePointer(pBase, Entry.m_Offset), std::get<var_type>( Data ), T_IS_READ, Index );
                }
            };

            //
            // ActionEntry types
            //
            if ( Entry.m_FunctionLists )
            {
                // if it is an array lets get the index number
                assert( pName[ e ] == '[' ); //-V614
                e++;

                // If we have '[]' this means we are dealing with the count itself
                if( pName[ e ] == ']' )
                {
                    assert( pName[ e+1 ] == 0 );
                    std::array< uint64_t,4 > MemoryBlock;
                    if constexpr ( T_IS_READ )
                    {
                        std::uint64_t Count;
                        Entry.m_FunctionLists( HandleBasePointer(pBase, Entry.m_Offset), Count, lists_cmd::READ_COUNT, MemoryBlock );
                        Data.emplace<int>() = static_cast<int>(Count);
                    }
                    else
                    {
                        auto Count = static_cast<std::uint64_t>(std::get<int>( Data ));
                        Entry.m_FunctionLists( HandleBasePointer(pBase, Entry.m_Offset), Count, lists_cmd::WRITE_COUNT, MemoryBlock );
                    }

                    // We are ok here
                    bSuccess = true;
                }
                else
                {
                    assert( std::isdigit( pName[e] ) );
                     
                    Index = pName[ e++ ] - std::uint64_t{'0'};

                    while( std::isdigit( pName[ e ] ) )
                    {
                        Index *= 10;
                        Index += pName[ e++ ] - std::uint64_t{'0'};
                    }

                    assert( pName[ e ] == ']' );    // Make sure we match the close list character
                    e++;
                    assert( pName[ e ] == '/' || pName[ e ] == 0 );

                    std::visit( ProcessEntry, Entry.m_FunctionTypeGetSet );
                }
            }
            else
            {
                std::visit( ProcessEntry, Entry.m_FunctionTypeGetSet );
            }

            return bSuccess;
        }

        //--------------------------------------------------------------------------------------------
        // ActionEntry Query set/get a property
        //--------------------------------------------------------------------------------------------
        template< bool T_IS_READ > constexpr
        bool PropertyQuery( const property::table& Table, void* pClassInstance, const char* pName, property::data& Data ) noexcept
        {
            // Make sure that the root path for the property matches 
            // with the root table.
            int i = 0;
            if ( Table.m_pName )
            {
                for ( ; pName[ i ] == Table.m_pName[ i ]; i++ );
                if ( Table.m_pName[ i ] != 0 ) return false;

                // Get the end of the property
                assert( pName[ i ] == '/' );
                i++;
            }

            return property::details::RecursivePropertyQuery<T_IS_READ>( Table, pClassInstance, &pName[ i ], Data );
        }
    }

    //--------------------------------------------------------------------------------------------
    // List of the properties from a class
    //--------------------------------------------------------------------------------------------

    template< typename T > inline
    void DisplayEnum( T& ClassInstance, std::function<enum_callback_fn> Callback ) noexcept
    {
        details::Enum<true>( getTable( ClassInstance ), &ClassInstance, Callback );
    }

    //--------------------------------------------------------------------------------------------

    inline
    void DisplayEnum( const table& Table, void* pClassInstance, std::function<enum_callback_fn> Callback ) noexcept
    {
        details::Enum<true>( Table, pClassInstance, Callback );
    }

    //--------------------------------------------------------------------------------------------

    template< typename T > inline
    void SerializeEnum( T& ClassInstance, std::function<enum_callback_fn> Callback ) noexcept
    {
        details::Enum<false>( getTable( ClassInstance ), &ClassInstance, Callback );
    }

    //--------------------------------------------------------------------------------------------

    inline
    void SerializeEnum( const table& Table, void* pClassInstance, std::function<enum_callback_fn> Callback ) noexcept
    {
        details::Enum<false>( Table, pClassInstance, Callback );
    }

    //--------------------------------------------------------------------------------------------
    // Allows you to get the value of one of the properties if it finds it
    //--------------------------------------------------------------------------------------------
    inline
    property::data get( const property::table& Table, void* pClassInstance, const char* pName ) noexcept
    {
        assert(pClassInstance);
        property::data Data;
        property::details::PropertyQuery<true>( Table, pClassInstance, pName, Data );
        return Data;
    }

    //--------------------------------------------------------------------------------------------
    template< typename T > inline
    property::data get( T& ClassInstance, const char* pName ) noexcept { return get( property::getTable( ClassInstance ), pName ); }

    //--------------------------------------------------------------------------------------------
    // Will try to set the value of a property if it finds it
    //--------------------------------------------------------------------------------------------

    constexpr
    bool set( const property::table& Table, void* pClassInstance, const char* pName, const property::data& Data ) noexcept
    {
        assert(pClassInstance);
        assert( Data.index() != std::variant_npos );
        return property::details::PropertyQuery<false>( Table, pClassInstance, pName, const_cast<property::data&>( Data ) );
    }

    //--------------------------------------------------------------------------------------------
    template< typename T > constexpr
    bool set( T& ClassInstance, const char* pName, const property::data& Data ) noexcept { return set( property::getTable( ClassInstance ), &ClassInstance, pName, Data ); }

    //--------------------------------------------------------------------------------------------
    // Sets the properties inside a pack into the class instance
    //--------------------------------------------------------------------------------------------
    inline
    bool set( const property::table& Table, void* pClassInstance, const pack& Pack ) noexcept
    {
        // If there is nothing to do exit
        if (Pack.m_lEntry.size() == 0) return true;

        assert(pClassInstance);

        std::size_t     iCurrentEntry = 0;
        int             iCurrentPath  = 0;

        // Lets make sure that the top path matches our root table
        if ( Table.m_NameHash != Pack.m_lPath[ iCurrentPath++ ].m_Key )
            return false;

        int   Ret = property::details::UnpackRecursive( Table, pClassInstance, const_cast<pack&>( Pack ), iCurrentEntry, iCurrentPath );
        assert( Ret == 0 );
        return true;
    }

    //--------------------------------------------------------------------------------------------
    template< typename T > constexpr
    bool set( T& ClassInstance, const pack& Pack ) noexcept { return set( getTable( ClassInstance ), &ClassInstance, Pack ); }

    //--------------------------------------------------------------------------------------------
    // Will try to set the value of a property if it finds it
    //--------------------------------------------------------------------------------------------
    inline
    void Pack( const property::table& Table, void* pClassInstance, pack& ThePack ) noexcept
    {
        if (Table.m_Count == 0) return;

        // Create the first entry
        ThePack.createEntry();

        // Enter as a path the name of the table
        ThePack.pushPath( Table.m_NameHash, lists_iterator_ends_v );

        // Collect all properties
        property::details::PackRecursive( Table, pClassInstance, ThePack );

        // Last is never used (is trash)
        ThePack.m_lEntry.pop_back();
    }

    //--------------------------------------------------------------------------------------------
    template< typename T > inline 
    void Pack( T& ClassInstance, pack& ThePack ) noexcept { Pack( getTable( ClassInstance ), &ClassInstance, ThePack ); }
}

property_begin(property::empty)
{}
property_vend_h(property::empty)

//--------------------------------------------------------------------------------------------
// Re-enable some warnings
//--------------------------------------------------------------------------------------------
#if defined(__clang__)
    // warning: field 'm_Entry' is uninitialized when used here [-Wuninitialized]
    #pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#elif defined(_MSC_VER)
    // warning C4201: nonstandard extension used: nameless struct/union
    #pragma warning( pop ) 
#endif

//--------------------------------------------------------------------------------------------
// END
//--------------------------------------------------------------------------------------------
#endif