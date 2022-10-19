// Copyright (c) 2022, KaoruXun All rights reserved.

#ifndef _PROPERTY_INSPECTOR_H
#define _PROPERTY_INSPECTOR_H
#pragma once

#define PROPERTY_EDITOR
#ifndef _PROPERTY_H
#include "Properties.h"
#endif
#ifndef IMGUI_API
#include "imgui.h"
#endif

// Microsoft and its macros....
#undef max

// Disable this warnings
#pragma warning( push )
#pragma warning( disable : 4201)                    // warning C4201: nonstandard extension used: nameless struct/union

//-----------------------------------------------------------------------------------
// Different Editor Styles to display the properties, easy to access for the user.
//-----------------------------------------------------------------------------------
namespace property
{
    class inspector;

    //-----------------------------------------------------------------------------------
    // Undo command information
    //-----------------------------------------------------------------------------------
    namespace editor::undo
    {
        //-----------------------------------------------------------------------------------
        // Base class of the cmd class
        struct base_cmd
        {
            string_t                    m_Name;                     // Full name of the property been edited
            const property::table* m_pTable;
            void* m_pInstance;
            union
            {
                std::uint8_t            m_Flags{ 0 };
                struct
                {
                    bool                m_isEditing : 1         // Is Item currently been edited
                        , m_isChange : 1;
                };
            };
        };

        //-----------------------------------------------------------------------------------
        // Undo command based on property types
        template< typename T >
        struct cmd : base_cmd
        {
            T       m_NewValue;                             // Whenever a value changes we put it here
            T       m_Original;                             // This is the value of the property before we made any changes
        };

        namespace details
        {
            template<typename... T>
            std::variant< property::editor::undo::cmd<T> ...> UndoTypesVarient(std::variant< T...>);

            // Actual variant with all the different editing styles
            using undo_variant = decltype(UndoTypesVarient(std::declval<property::data>()));
        }

        //-----------------------------------------------------------------------------------
        // This is the official historical structure of the editor 
        using entry = details::undo_variant;

        //-----------------------------------------------------------------------------------
        struct system
        {
            std::vector<entry>  m_lCmds{};
            int                 m_Index{ 0 };
            int                 m_MaxSteps{ 5 };

            void clear(void) noexcept
            {
                m_Index = 0;
                m_lCmds.clear();
            }
        };
    }

    //-----------------------------------------------------------------------------------
    // Draw prototypes
    //-----------------------------------------------------------------------------------
    template< typename T >
    struct edstyle;

    namespace editor::details
    {
        namespace style
        {
            struct scroll_bar;
            struct drag;
            struct enumeration;
            struct defaulted;
            struct button;
        };

        template< typename T, typename T_STYLE >
        void draw(property::editor::undo::cmd<T>& Cmd, const T& Value, const property::editor::style_info<T>& I, property::flags::type Flags) noexcept;

        template<> void draw<int, style::scroll_bar>(property::editor::undo::cmd<int>& Cmd, const int& Value, const property::editor::style_info<int>& I, property::flags::type Flags) noexcept;
        template<> void draw<int, style::drag>(property::editor::undo::cmd<int>& Cmd, const int& Value, const property::editor::style_info<int>& I, property::flags::type Flags) noexcept;
        template<> void draw<float, style::scroll_bar>(property::editor::undo::cmd<float>& Cmd, const float& Value, const property::editor::style_info<float>& I, property::flags::type Flags) noexcept;
        template<> void draw<float, style::drag>(property::editor::undo::cmd<float>& Cmd, const float& Value, const property::editor::style_info<float>& I, property::flags::type Flags) noexcept;
        template<> void draw<string_t, style::defaulted>(property::editor::undo::cmd<string_t>& Cmd, const string_t& Value, const property::editor::style_info<string_t>& I, property::flags::type Flags) noexcept;
        template<> void draw<string_t, style::enumeration>(property::editor::undo::cmd<string_t>& Cmd, const string_t& Value, const property::editor::style_info<string_t>& I, property::flags::type Flags) noexcept;
        template<> void draw<string_t, style::button>(property::editor::undo::cmd<string_t>& Cmd, const string_t& Value, const property::editor::style_info<string_t>& I, property::flags::type Flags) noexcept;

        template< typename T > inline
            void onRender(property::editor::undo::cmd<T>& Cmd, const T& Value, const property::table_entry& Entry, property::flags::type Flags) noexcept
        {
            const     auto Default = property::edstyle<T>::Default();
            constexpr auto MaxIndex = std::variant_size_v<property::settings::editor::styles_info_variant> -1;
            auto    EdStyle = (Entry.m_EditStylesInfo.index() == MaxIndex) ? Default : Entry.m_EditStylesInfo;
            auto& I = std::get<property::editor::style_info<T>>(EdStyle);
            reinterpret_cast<decltype(draw<T, void>)*>(I.m_pDrawFn)(Cmd, Value, I, Flags);
        }
    };

    //-----------------------------------------------------------------------------------
    // Provides all the setting functions
    //-----------------------------------------------------------------------------------
    template< typename T >
    constexpr editor::style_info<T>             edstyle<T>::Default(void)                                              noexcept
    {
        return editor::style_info<T>{ editor::details::draw<T, editor::details::style::defaulted> };
    }

    constexpr editor::style_info<int>           edstyle<int>::ScrollBar(int Min, int Max, const char* pFormat)             noexcept
    {
        return editor::style_info<int>{ editor::details::draw<int, editor::details::style::scroll_bar >, Min, Max, pFormat, 1     };
    }

    constexpr editor::style_info<int>           edstyle<int>::Drag(float Speed, int Min, int Max, const char* pFormat)  noexcept
    {
        return editor::style_info<int>{ editor::details::draw<int, editor::details::style::drag       >, Min, Max, pFormat, Speed };
    }

    constexpr editor::style_info<float>         edstyle<float>::ScrollBar(float Min, float Max, const char* pFormat, float Power) noexcept
    {
        return editor::style_info<float>{ editor::details::draw<float, editor::details::style::scroll_bar >, Min, Max, pFormat, 1.0f, Power   };
    }

    constexpr editor::style_info<float>         edstyle<float>::Drag(float Speed, float Min, float Max, const char* pFormat, float Power)  noexcept
    {
        return editor::style_info<float>{ editor::details::draw<float, editor::details::style::drag       >, Min, Max, pFormat, Speed, Power   };
    }

    template< std::size_t N >
    constexpr editor::style_info<string_t>   edstyle<string_t>::Enumeration(const std::array<std::pair<const char*, int>, N>& Array)  noexcept
    {
        return editor::style_info<string_t>{ editor::details::draw<string_t, editor::details::style::enumeration >, Array.data(), Array.size() };
    }

    constexpr editor::style_info<string_t>   edstyle<string_t>::Button(void)  noexcept
    {
        return editor::style_info<string_t>{ editor::details::draw<string_t, editor::details::style::button >, nullptr, 0 };
    }

    constexpr editor::style_info<string_t>   edstyle<string_t>::Default(void)                                              noexcept
    {
        return editor::style_info<string_t>{ editor::details::draw<string_t, editor::details::style::defaulted>, nullptr, 0 };
    }
}

//-----------------------------------------------------------------------------------
// Inspector to display the properties
//-----------------------------------------------------------------------------------

class property::inspector : public property::base
{
public:

    struct settings
    {
        ImVec2      m_WindowPadding{ 0, 3 };
        ImVec2      m_FramePadding{ 0, 2 };
        ImVec2      m_ItemSpacing{ 2, 1 };
        float       m_IndentSpacing{ 3 };
        ImVec2      m_TableFramePadding{ 2, 6 };

        bool        m_bRenderLeftBackground{ true };
        bool        m_bRenderRightBackground{ true };
        bool        m_bRenderBackgroundDepth{ true };
        float       m_ColorVScalar1{ 0.5f };
        float       m_ColorVScalar2{ 0.4f };
        float       m_ColorSScalar{ 0.4f };

        ImVec2      m_HelpWindowPadding{ 10, 10 };
        int         m_HelpWindowSizeInChars{ 50 };
    };

public:

    property_vtable();

    inline                  inspector(const char* pName, bool isOpen = true)               noexcept : m_pName{ pName }, m_bWindowOpen{ isOpen } {}
    virtual                ~inspector(void)                                                noexcept = default;
    void        clear(void)                                                noexcept;
    void        AppendEntity(void)                                                noexcept;
    void        AppendEntityComponent(const property::table& Table, void* pBase)           noexcept;
    void        Undo(void)                                                noexcept;
    void        Redo(void)                                                noexcept;
    void        Show(std::function<void(void)> Callback)                  noexcept;
    inline      bool        isValid(void)                                        const   noexcept { return m_lEntities.empty() == false; }
    inline      void        setupWindowSize(int Width, int Height)                               noexcept { m_Width = Width; m_Height = Height; }
    inline      void        setOpenWindow(bool b)                                              noexcept { m_bWindowOpen = b; }
    constexpr   bool        isWindowOpen(void)                                        const   noexcept { return m_bWindowOpen; }

    vs2017_hack_protected

        settings                                            m_Settings{};

protected:

    struct entry
    {
        std::string                                     m_FullName;
        property::data                                  m_Data;
        const property::table_entry* m_pUserData;
        property::flags::type                           m_Flags;
    };

    struct component
    {
        std::pair<const property::table*, void*>        m_Base{ nullptr,nullptr };
        std::vector<std::unique_ptr<entry>>             m_List{};
    };

    struct entity
    {
        std::vector<std::unique_ptr<component>>     m_lComponents{};
    };

protected:

    void        RefreshAllProperties(void)                                        noexcept;
    void        RefactorComponents(void)                                        noexcept;
    void        Render(component& C, int& GlobalIndex)              noexcept;
    void        Show(void)                                        noexcept;
    void        DrawBackground(int Depth, int GlobalIndex)          const   noexcept;
    void        HelpMarker(const char* desc)                    const   noexcept;
    void        Help(const entry& Entry)                  const   noexcept;

protected:

    const char* m_pName{ nullptr };
    std::vector<std::unique_ptr<entity>>        m_lEntities{};
    int                                         m_Width{ 430 };
    int                                         m_Height{ 450 };
    bool                                        m_bWindowOpen{ true };
    property::editor::undo::system              m_UndoSystem {};
};

#pragma warning( pop ) 

#endif