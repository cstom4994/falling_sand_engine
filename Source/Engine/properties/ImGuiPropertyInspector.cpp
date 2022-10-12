// Copyright (c) 2019 - 2022, KaoruXun All rights reserved.

#include "ImGuiPropertyInspector.h"
#include <windows.h>

void Output(const char* szFormat, ...)
{
    char szBuff[1024];
    va_list arg;
    va_start(arg, szFormat);
    vsnprintf(szBuff, sizeof(szBuff), szFormat, arg);
    va_end(arg);

    WCHAR szBuff2[1024];
    for (int i = 0; true; i++)
    {
        szBuff2[i] = szBuff[i];
        if (!szBuff2[i]) break;
    }

    OutputDebugString(szBuff2);
}

//-----------------------------------------------------------------------------------
// Undo details
//-----------------------------------------------------------------------------------
namespace property::editor::undo::details
{
    //-----------------------------------------------------------------------------------
    template< bool T_ORIGINAL_V, typename T >
    void VariadicToStringValue(string_t& Str, T& V )
    {
        char buff[256];
        std::visit([&](auto&& Value)
        {
            using T = std::decay_t<decltype(Value)>;

                 if constexpr (std::is_same_v<T, property::editor::undo::cmd<int> >)                snprintf(buff, sizeof(buff), "%d",  T_ORIGINAL_V ? Value.m_Original : Value.m_NewValue);
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<float>>)               snprintf(buff, sizeof(buff), "%f",  T_ORIGINAL_V ? Value.m_Original : Value.m_NewValue);
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<bool>>)                snprintf(buff, sizeof(buff), "%s", (T_ORIGINAL_V ? Value.m_Original : Value.m_NewValue) ? "true" : "false");
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<string_t>>)            snprintf(buff, sizeof(buff), "%s",  T_ORIGINAL_V ? Value.m_Original.c_str() : Value.m_NewValue.c_str());
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<oobb>>)
            {
                if constexpr (T_ORIGINAL_V)                                                         snprintf(buff, sizeof(buff), "%f %f", Value.m_Original.m_Min, Value.m_Original.m_Max);
                else                                                                                snprintf(buff, sizeof(buff), "%f %f", Value.m_NewValue.m_Min, Value.m_NewValue.m_Max);
            }
            else static_assert(always_false<T>::value, "We are not covering all the cases!");
        }
        , V);

        Str = buff;
    }

    //-----------------------------------------------------------------------------------
    template< typename T >
    void VariadicToType(string_t& Str, T& V)
    {
        std::visit([&](auto&& Value)
        {
            using T = std::decay_t<decltype(Value)>;

                 if constexpr (std::is_same_v<T, property::editor::undo::cmd<int> >)            Str = "int";
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<float>>)           Str = "float";
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<bool>>)            Str = "bool";
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<string_t>>)        Str = "string";
            else if constexpr (std::is_same_v<T, property::editor::undo::cmd<oobb>>)            Str = "oobb";
            else static_assert(always_false<T>::value, "We are not covering all the cases!");
        }
        , V);
    }

    //-----------------------------------------------------------------------------------
    template< typename T >
    void VariadicToName(string_t& Str, T& V)
    {
        std::visit([&](auto&& Value) {Str.assign( Value.m_Name ); }, V);
    }
} 

//-----------------------------------------------------------------------------------
// define all the properties for the undo system
//-----------------------------------------------------------------------------------
property_begin( property::editor::undo::entry )
{
      property_var_fnbegin("Name", string_t)
      {
        if (isRead) property::editor::undo::details::VariadicToName(InOut, Self);
      } property_var_fnend()
    , property_var_fnbegin("Type", string_t)
      {
        if (isRead) property::editor::undo::details::VariadicToType(InOut, Self);
      } property_var_fnend()
    , property_var_fnbegin("Original", string_t)
      {
        if (isRead) property::editor::undo::details::VariadicToStringValue<true>( InOut, Self );
      } property_var_fnend()
    , property_var_fnbegin("New", string_t)
      {
        if (isRead) property::editor::undo::details::VariadicToStringValue<false>( InOut, Self );
      } property_var_fnend()

} property_end()

//-----------------------------------------------------------------------------------

property_begin( property::editor::undo::system )
{
      property_var  (m_Index).Flags(property::flags::SHOW_READONLY)
    , property_var  (m_MaxSteps)      
    , property_var  (m_lCmds).Flags(property::flags::SHOW_READONLY)

} property_end()


//-----------------------------------------------------------------------------------
// All the render functions
//-----------------------------------------------------------------------------------
namespace property::editor::details
{
    //-----------------------------------------------------------------------------------
    template< typename T>
    bool ReadOnly( const char* pFmt, T Value )
    {
        std::array<char, 128> Buff;
        snprintf( Buff.data(), Buff.size(), pFmt, Value );
        ImGui::Button( Buff.data(), ImVec2( -1, 0 ) );
        return false;
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<int, style::scroll_bar>( undo::cmd<int>& Cmd, const int& Value, const property::editor::style_info<int>& I, property::flags::type Flags ) noexcept
    {
        if ( Flags.m_isShowReadOnly ) editor::details::ReadOnly( I.m_pFormat, Value );
        else
        {
            int V = Value;
            Cmd.m_isChange = ImGui::SliderInt( "##value", &V, I.m_Min, I.m_Max, I.m_pFormat );
            if( Cmd.m_isChange )
            {
                if (Cmd.m_isEditing == false) Cmd.m_Original = Value;
                Cmd.m_isEditing = true;
                Cmd.m_NewValue  = V;
            }
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<int, style::drag>( undo::cmd<int>& Cmd, const int& Value, const property::editor::style_info<int>& I, property::flags::type Flags ) noexcept
    {
        if ( Flags.m_isShowReadOnly ) editor::details::ReadOnly( I.m_pFormat, Value );
        else
        {
            int V = Value;
            Cmd.m_isChange = ImGui::DragInt( "##value", &V, I.m_Speed, I.m_Min, I.m_Max, I.m_pFormat );
            if( Cmd.m_isChange )
            {
                if (Cmd.m_isEditing == false) Cmd.m_Original = Value;
                Cmd.m_isEditing = true;
                Cmd.m_NewValue  = V;
            }
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<float, style::scroll_bar>( undo::cmd<float>& Cmd, const float& Value, const property::editor::style_info<float>& I, property::flags::type Flags ) noexcept
    {
        if ( Flags.m_isShowReadOnly ) editor::details::ReadOnly( I.m_pFormat, Value );
        else
        {
            float V = Value;
            Cmd.m_isChange = ImGui::SliderFloat("##value", &V, I.m_Min, I.m_Max, I.m_pFormat, I.m_Power);
            if( Cmd.m_isChange )
            {
                if( Cmd.m_isEditing == false ) Cmd.m_Original = Value;
                Cmd.m_isEditing = true;
                Cmd.m_NewValue  = V;
            }
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<float, style::drag>( undo::cmd<float>& Cmd, const float& Value, const property::editor::style_info<float>& I, property::flags::type Flags ) noexcept
    {
        if ( Flags.m_isShowReadOnly ) editor::details::ReadOnly( I.m_pFormat, Value );
        else
        {
            float V = Value;
            Cmd.m_isChange = ImGui::DragFloat("##value", &V, I.m_Speed, I.m_Min, I.m_Max, I.m_pFormat, I.m_Power);
            if( Cmd.m_isChange )
            {
                if (Cmd.m_isEditing == false) Cmd.m_Original = Value;
                Cmd.m_isEditing = true;
                Cmd.m_NewValue  = V;
            }
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<bool, style::defaulted>( undo::cmd<bool>& Cmd, const bool& Value, const property::editor::style_info<bool>&, property::flags::type Flags ) noexcept
    {
        bool V = Value;
        if ( Flags.m_isShowReadOnly )
        {
            ImGui::Checkbox("##value", &V);
            V = Value;
        }
        else 
        {
            Cmd.m_isChange = ImGui::Checkbox("##value", &V);
            if ( Cmd.m_isChange )
            {
                if(Cmd.m_isEditing == false) Cmd.m_Original = Value;
                Cmd.m_isEditing = true;
                Cmd.m_NewValue  = V;
            } 
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }

        ImGui::SameLine();
        if (V) ImGui::Text("True");
        else   ImGui::Text("False");
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<string_t, style::defaulted>(undo::cmd<string_t>& Cmd, const string_t& Value, const property::editor::style_info<string_t>&, property::flags::type Flags) noexcept
    {
        if ( Flags.m_isShowReadOnly ) ImGui::InputText( "##value", (char*)Value.c_str(), Value.length(), ImGuiInputTextFlags_ReadOnly );
        else
        {
            std::array<char, 256> buff;
            Value.copy( buff.data(), Value.length() );
            buff[ Value.length() ] = 0;
            Cmd.m_isChange = ImGui::InputText( "##value", buff.data(), buff.size(), ImGuiInputTextFlags_EnterReturnsTrue );
            if ( Cmd.m_isChange )
            {
                if( Cmd.m_isEditing == false ) Cmd.m_Original.assign(Value);
                Cmd.m_isEditing = true;
                Cmd.m_NewValue.assign( buff.data() );
            }
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<string_t, style::button>(property::editor::undo::cmd<string_t>& Cmd, const string_t& Value, const property::editor::style_info<string_t>&, property::flags::type Flags) noexcept
    {
        if ( Flags.m_isShowReadOnly ) ImGui::Button( Value.c_str(), ImVec2(-1,16) );
        else
        {
            Cmd.m_isChange = ImGui::Button( Value.c_str(), ImVec2(-1,16) );
            if ( Cmd.m_isChange )
            {
                if( Cmd.m_isEditing == false ) Cmd.m_Original.assign(Value);
                Cmd.m_isEditing = true;
                Cmd.m_NewValue.assign( Value );
            }
            if( Cmd.m_isEditing && ImGui::IsItemDeactivatedAfterEdit() ) Cmd.m_isEditing = false;
        }
        
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<string_t, style::enumeration>(undo::cmd<string_t>& Cmd, const string_t& Value, const property::editor::style_info<string_t>& Info, property::flags::type Flags) noexcept
    {
        if (Flags.m_isShowReadOnly) ImGui::InputText("##value", (char*)Value.c_str(), Value.length(), ImGuiInputTextFlags_ReadOnly);
        else
        {
            std::size_t current_item = 0;
            for (; current_item < Info.m_EnumCount; ++current_item)
            {
                if (Value == Info.m_pEnumList[current_item].first )
                {
                    break;
                }
            }

            Cmd.m_isChange = false;
            if (ImGui::BeginCombo("##combo", Info.m_pEnumList[current_item].first )) // The second parameter is the label previewed before opening the combo.
            {
                for (std::size_t n = 0; n < Info.m_EnumCount; n++)
                {
                    bool is_selected = (current_item == n); // You can store your selection however you want, outside or inside your objects
                   
                    if (ImGui::Selectable(Info.m_pEnumList[n].first, is_selected))
                    {
                        if (Cmd.m_isEditing == false) Cmd.m_Original.assign(Value);
                        Cmd.m_NewValue.assign(Info.m_pEnumList[n].first);
                        Cmd.m_isChange = true;
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }
        }
    }

    //-----------------------------------------------------------------------------------

    template<>
    void draw<oobb, style::defaulted>(undo::cmd<oobb>& Cmd, const oobb& Value, const property::editor::style_info<oobb>&, property::flags::type Flags) noexcept
    {
        ImGuiStyle * style   = &ImGui::GetStyle();
        const auto   Width   = (ImGui::GetContentRegionAvail().x - style->ItemInnerSpacing.x ) / 2;
        const auto   Height  = ImGui::GetFrameHeight();
        ImVec2       pos     = ImGui::GetCursorScreenPos();
        oobb         Temp    = Value;

        ImGui::PushItemWidth( Width );

        // Min
        bool bChange      = ImGui::DragFloat( "##value1", &Temp.m_Min, 0.01f, -1000.0f, 1000.0f );
        bool bDoneEditing = ImGui::IsItemDeactivatedAfterEdit();
        ImGui::GetWindowDrawList()->AddRectFilled( pos, ImVec2( pos.x + Width, pos.y + Height ), ImU32( 0x440000ff ) );

        // Max
        ImGui::SameLine( 0, 2 );
        pos = ImGui::GetCursorScreenPos();

        bChange      |= ImGui::DragFloat( "##value2", &Temp.m_Max, 0.01f, -1000.0f, 1000.0f );
        bDoneEditing |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::GetWindowDrawList()->AddRectFilled( pos, ImVec2( pos.x + Width, pos.y + Height ), ImU32( 0x4400ff00 ) );

        // Done
        ImGui::PopItemWidth();

        if( bChange )
        {
            if ( Flags.m_isShowReadOnly ) return;

            if (Cmd.m_isEditing == false) Cmd.m_Original = Value;

            Cmd.m_isEditing     = true;
            Cmd.m_isChange      = true;
            Cmd.m_NewValue      = Temp;
        }
        if( bDoneEditing )
        {
            Cmd.m_isEditing = false;
        }
    }
}


//-------------------------------------------------------------------------------------------------
// Inspector
//-------------------------------------------------------------------------------------------------

static std::array<ImColor, 20> s_ColorCategories =
{
    ImColor{ 0xffe8c7ae },
    ImColor{ 0xffb4771f },
    ImColor{ 0xff0e7fff },
    ImColor{ 0xff2ca02c },
    ImColor{ 0xff78bbff },
    ImColor{ 0xff8adf98 },
    ImColor{ 0xff2827d6 },
    ImColor{ 0xff9698ff },
    ImColor{ 0xffbd6794 },
    ImColor{ 0xffd5b0c5 },
    ImColor{ 0xff4b568c },
    ImColor{ 0xff949cc4 },
    ImColor{ 0xffc277e3 },
    ImColor{ 0xffd2b6f7 },
    ImColor{ 0xff7f7f7f },
    ImColor{ 0xffc7c7c7 },
    ImColor{ 0xff22bdbc },
    ImColor{ 0xff8ddbdb },
    ImColor{ 0xffcfbe17 },
    ImColor{ 0xffe5da9e }
};

//-------------------------------------------------------------------------------------------------

void property::inspector::clear(void) noexcept
{
    m_lEntities.clear();
    m_UndoSystem.clear();
}

//-------------------------------------------------------------------------------------------------
void property::inspector::AppendEntity(void) noexcept
{
    m_lEntities.push_back( std::make_unique<entity>() );
}

//-------------------------------------------------------------------------------------------------
void property::inspector::AppendEntityComponent(const property::table& Table, void* pBase) noexcept
{
    auto Component = std::make_unique<component>();

    // Cache the information
    Component->m_Base = { &Table, pBase };

    m_lEntities.back()->m_lComponents.push_back(std::move(Component));

}

//-------------------------------------------------------------------------------------------------

void property::inspector::Undo(void) noexcept
{
    if (m_UndoSystem.m_Index == 0 || m_UndoSystem.m_lCmds.size() == 0)
        return;

    std::visit([&](auto& Value) noexcept
    {
        property::set(*Value.m_pTable, Value.m_pInstance, Value.m_Name.c_str(), property::data { Value.m_Original });
    }, m_UndoSystem.m_lCmds[--m_UndoSystem.m_Index]);
}

//-------------------------------------------------------------------------------------------------

void property::inspector::Redo(void) noexcept
{
    if (m_UndoSystem.m_Index == static_cast<int>(m_UndoSystem.m_lCmds.size()))
        return;

    std::visit([&](auto& Value) noexcept
    {
        property::set(*Value.m_pTable, Value.m_pInstance, Value.m_Name.c_str(), property::data { Value.m_NewValue });
    }, m_UndoSystem.m_lCmds[m_UndoSystem.m_Index++]);
}

//-------------------------------------------------------------------------------------------------
    
void property::inspector::RefreshAllProperties( void ) noexcept
{
    for ( auto& E : m_lEntities )
    {
        for ( auto& C : E->m_lComponents )
        {
            C->m_List.clear();
            property::DisplayEnum( *C->m_Base.first, C->m_Base.second, [&]( std::string_view PropertyName, property::data&& Data, const property::table& Table, std::size_t Index, property::flags::type Flags )
            {
                C->m_List.push_back( std::make_unique<entry>(std::string{ PropertyName }, Data, &Table.m_pEntry[Index], Flags) );
            } );
        }
    }
}

//-------------------------------------------------------------------------------------------------
// This generates the intersection of all the components
//-------------------------------------------------------------------------------------------------
void property::inspector::RefactorComponents( void ) noexcept
{
    /*
    auto& ReferenceEntity = *m_lEntities[0];
    for( int iC = 0; ReferenceEntity.m_lComponents.size(); iC++ )
    {
        auto RefCompCRC = ReferenceEntity.m_lComponents[iC]->m_Base.first->m_NameHash;
        for ( int iE = 1; iE < m_lEntities.size(); iE++ )
        {
            auto& Entity = *m_lEntities[iE];
            int iFound = -1;
            for ( int i=0 ; i<Entity.m_lComponents.size(); ++i )
            {
                auto& Component = *Entity.m_lComponents[i];
                if( RefCompCRC == Component.m_Base.first->m_NameHash )
                {
                    iFound = i;
                    break;
                }
            }
        }
    }
    */
}

//-------------------------------------------------------------------------------------------------

void property::inspector::Show( std::function<void(void)> Callback ) noexcept
{
    if( m_bWindowOpen == false ) return;

    //
    // Key styles 
    //
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding,   m_Settings.m_WindowPadding );
    ImGui::PushStyleVar( ImGuiStyleVar_FramePadding,    m_Settings.m_FramePadding );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing,     m_Settings.m_ItemSpacing );
    ImGui::PushStyleVar( ImGuiStyleVar_IndentSpacing,   m_Settings.m_IndentSpacing );
    ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0 );

    //
    // Open the window
    //
    ImGui::SetNextWindowSize( ImVec2( static_cast<float>(m_Width), static_cast<float>(m_Height) ), ImGuiCond_FirstUseEver );
    if ( !ImGui::Begin( m_pName, &m_bWindowOpen ) )
    {
        ImGui::PopStyleVar( 5 );
        ImGui::End();
        return;
    }

    //
    // Let the user inject something at the top of the window
    //
    Callback();

    //
    // Display the properties
    //
    ImGui::Columns( 2 );
    ImGui::Separator();

    Show();

    ImGui::Columns( 1 );
    ImGui::Separator();
    ImGui::PopStyleVar( 5 );
    ImGui::End();
}

//-------------------------------------------------------------------------------------------------

void property::inspector::Render( component& C, int& GlobalIndex ) noexcept
{
    struct element
    {
        std::uint32_t   m_CRC;
        int             m_iArray;
        std::size_t     m_iStart;
        std::size_t     m_iEnd;
        int             m_OpenAll;
        bool            m_isOpen        : 1
                        , m_isAtomicArray : 1
                        , m_isReadOnly    : 1;
    };

    int                         iDepth   = -1;
    std::array<element,32>      Tree;
    auto                        PushTree = [&]( std::uint32_t UID, const char* pName, std::size_t iStart, std::size_t iEnd, bool isReadOnly, bool bArray = false, bool bAtomic = false )
    {
        bool Open = iDepth<0? true : Tree[ iDepth ].m_isOpen;
        if( Open )
        {
            if ( iDepth >0 && Tree[iDepth-1].m_OpenAll ) ImGui::SetNextItemOpen( Tree[iDepth-1].m_OpenAll > 0 );
            Open = ImGui::TreeNodeEx( pName, ImGuiTreeNodeFlags_DefaultOpen | ( ( iDepth == -1 ) ? ImGuiTreeNodeFlags_Framed : 0 ) );
        }
        auto& L = Tree[ ++iDepth ];

        L.m_CRC             = UID;
        L.m_iArray          = bArray ? 0 : -1;
        L.m_iStart          = iStart;
        L.m_iEnd            = iEnd;
        L.m_OpenAll         = 0;
        L.m_isOpen          = Open;
        L.m_isAtomicArray   = bAtomic;
        L.m_isReadOnly      = isReadOnly || ((iDepth>0)?Tree[ iDepth -1 ].m_isReadOnly : false);

        return Open;
    };
    auto                                            PopTree = [ & ]( std::size_t& iStart, std::size_t& iEnd )
    {
        const auto& E = Tree[ iDepth-- ];
        if( E.m_isOpen )
        {
            ImGui::TreePop();
        }

        iStart = E.m_iStart;
        iEnd   = E.m_iEnd;
    };

    std::size_t iStart = 0;
    std::size_t iEnd   = strlen( C.m_Base.first->m_pName );

    //
    // Deal with the top most tree
    //
    {
        ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, m_Settings.m_TableFramePadding );
        ImGui::AlignTextToFramePadding();

        // If the main tree is Close then forget about it
        PushTree( C.m_Base.first->m_NameHash, C.m_Base.first->m_pName, iStart, iEnd, false );

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled( pos, ImVec2( pos.x + ImGui::GetContentRegionAvail().x, pos.y + ImGui::GetFrameHeight() ), ImGui::GetColorU32( ImGuiCol_Header ) );
        ImGui::PopStyleVar();
    }
        
    if( Tree[iDepth].m_isOpen == false )
    {
        PopTree( iStart, iEnd );
        return;
    }

    //
    // Do all properties
    //
    for ( std::size_t iE = 0; iE<C.m_List.size(); ++iE )
    {
        auto& E = *C.m_List[iE];

        //
        // If we have a close tree skip same level entries
        //
        bool bSameLevel = [&]
        {
            if ( E.m_FullName.size() < iStart || E.m_FullName.size() < iEnd ) return false;
            return mm3_x86_32( { &E.m_FullName.c_str()[ iStart ], static_cast<std::uint32_t>( iEnd - iStart + 1 ) } ) == Tree[ iDepth ].m_CRC;
        }();

        if( Tree[iDepth].m_isOpen == false && bSameLevel )
            continue;

        //
        // Do we need to pop scopes?
        //
        while( bSameLevel == false )
        {
            PopTree( iStart, iEnd );
            bSameLevel = [&]
            {
                if ( E.m_FullName.size() < iStart || E.m_FullName.size() < iEnd ) return false;
                return mm3_x86_32( { &E.m_FullName.c_str()[ iStart ], static_cast<std::uint32_t>( iEnd - iStart + 1 ) } ) == Tree[ iDepth ].m_CRC;
            }();
        }

        // Make sure at this point everything is open
        assert( Tree[iDepth].m_isOpen );

        //
        // Render the left column
        //
        ++GlobalIndex;
        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        ImVec2 lpos = ImGui::GetCursorScreenPos();
        if( Tree[iDepth].m_iArray >= 0 ) ImGui::PushID( E.m_pUserData->m_NameHash + Tree[iDepth].m_iArray + iDepth * 1000 );
        else                             ImGui::PushID( E.m_pUserData->m_NameHash + iDepth * 1000 );
        if ( m_Settings.m_bRenderLeftBackground ) DrawBackground( iDepth, GlobalIndex );

        bool bRenderBlankRight = false;

        // Create a new tree
        if( E.m_Flags.m_isScope && Tree[iDepth].m_iArray < 0 )
        {
            // Is an array?
            if( E.m_FullName.back() == ']' )
            {
                std::array<char, 128> Name;
                snprintf( Name.data(), Name.size(), "%s[..%d]", E.m_pUserData->m_pName, std::get<int>(E.m_Data) );
                PushTree( E.m_pUserData->m_NameHash, Name.data(), iStart, iEnd, E.m_Flags.m_isShowReadOnly, true, C.m_List[iE+1]->m_FullName.back() == ']' );
                iStart = iEnd + 1;
                iEnd   = E.m_FullName.size() - 2;
            }
            else
            {
                PushTree( E.m_pUserData->m_NameHash, E.m_pUserData->m_pName, iStart, iEnd, E.m_Flags.m_isShowReadOnly );
                iStart = iEnd + 1;
                iEnd   = E.m_FullName.size();
            }
        }
        else
        {
            // if it is an array...
            if( Tree[iDepth].m_iArray >= 0 )
            {
                std::array<char, 128> Name;
                snprintf( Name.data(), Name.size(), "[%d]", Tree[ iDepth ].m_iArray++ );

                // Atomic array
                if ( Tree[iDepth].m_isAtomicArray )
                {
                    ImGui::TreeNodeEx( reinterpret_cast<void*>(static_cast<std::size_t>(E.m_pUserData->m_NameHash)), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", Name.data() );
                }
                else
                {
                    auto NewEnd = iEnd;
                    while( E.m_FullName[NewEnd] != '/' ) NewEnd++;

                    auto CRC = mm3_x86_32( { &E.m_FullName.c_str()[ iStart ], static_cast<std::uint32_t>( NewEnd - iStart + 1 ) } );

                    PushTree(CRC, Name.data(), iStart, iEnd, E.m_Flags.m_isShowReadOnly );
                    iEnd = NewEnd;

                    bRenderBlankRight = true;

                    // We need to redo this entry
                    iE--;
                }
            }
            else
            {
                ImGui::TreeNodeEx( reinterpret_cast<void*>(static_cast<std::size_t>(E.m_pUserData->m_NameHash)), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s", E.m_pUserData->m_pName );
            }
        }

        if ( E.m_Flags.m_isShowReadOnly || Tree[iDepth].m_isReadOnly )
        {
            ImColor     CC = ImVec4(0.7f, 0.7f, 1.0f, 0.35f);
            ImGui::GetWindowDrawList()->AddRectFilled(lpos, ImVec2(lpos.x + ImGui::GetContentRegionAvail().x, lpos.y + ImGui::GetFrameHeight()), CC);
        }

        // Print the help
        if ( ImGui::IsItemHovered() && bRenderBlankRight == false )
        {
            Help( E );
        }

        //
        // Render the right column
        //
        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::PushItemWidth( -1 );
        ImVec2 rpos = ImGui::GetCursorScreenPos();

        if( E.m_Flags.m_isScope || bRenderBlankRight )
        {
            if ( m_Settings.m_bRenderRightBackground ) DrawBackground( iDepth-1, GlobalIndex );

            if( iDepth>0 && Tree[iDepth].m_isAtomicArray == false && Tree[iDepth].m_isOpen )
            {
                if ( ImGui::Button( " O " ) ) Tree[iDepth-1].m_OpenAll = 1;
                HelpMarker( "Open/Expands all entries in the list" );
                ImGui::SameLine();
                if ( ImGui::Button( " C " ) ) Tree[iDepth-1].m_OpenAll = -1;
                HelpMarker( "Closes/Collapses all entries in the list" );
            }

            if (E.m_Flags.m_isShowReadOnly || Tree[iDepth].m_isReadOnly)
            {
                ImColor     CC = ImVec4(0.7f, 0.7f, 1.0f, 0.35f);
                ImGui::GetWindowDrawList()->AddRectFilled(rpos, ImVec2(rpos.x + ImGui::GetContentRegionAvail().x, rpos.y + ImGui::GetFrameHeight()), CC);
            }
        }
        else
        {
            if ( m_Settings.m_bRenderRightBackground ) DrawBackground( iDepth, GlobalIndex );

            if ( E.m_Flags.m_isShowReadOnly || Tree[iDepth].m_isReadOnly )
            {
                E.m_Flags |= property::flags::SHOW_READONLY;

                ImGuiStyle* style = &ImGui::GetStyle();
                ImVec2      pos   = ImGui::GetCursorScreenPos();
                ImColor     CC    = ImVec4( 0.7f, 0.7f, 1.0f, 0.35f );
                ImVec4      CC2f  = style->Colors[ ImGuiCol_Text ];

                CC2f.x *= 1.1f;
                CC2f.y *= 0.8f;
                CC2f.z *= 0.8f;

                ImGui::PushStyleColor( ImGuiCol_Text, CC2f );
                std::visit( [&]( auto&& Value ) 
                { 
                    using t = std::decay_t<decltype(Value)>;
                    if constexpr (std::is_same_v< t, property::settings::editor::empty> )
                    {
                        assert(false);
                    }
                    else
                    {
                        using T = std::decay_t<decltype(Value)>;
                        property::editor::undo::cmd<T> Cmd;
                        property::editor::details::onRender(Cmd, Value, *E.m_pUserData, E.m_Flags);
                        assert(Cmd.m_isChange == false);
                        assert(Cmd.m_isEditing == false);
                    }
                }, E.m_Data );
                ImGui::PopStyleColor();

                ImGui::GetWindowDrawList()->AddRectFilled( pos, ImVec2( pos.x + ImGui::GetContentRegionAvail().x, pos.y + ImGui::GetFrameHeight() ), CC );
            }
            else
            {
                // Determine if we are dealing with the same entry we are editing
                std::visit( [&]( auto&& Value ) 
                { 
                    using T = std::decay_t<decltype(Value)>;

                    // Check if we are editing an entry already
                    if( m_UndoSystem.m_lCmds.size() > 0 && m_UndoSystem.m_Index < static_cast<int>(m_UndoSystem.m_lCmds.size() ) )
                    {
                        auto& CmdVariant = m_UndoSystem.m_lCmds[m_UndoSystem.m_Index];

                        // Same data type?
                        if( E.m_Data.index() == CmdVariant.index() )
                        {
                            auto& UndoCmd = std::get<property::editor::undo::cmd<T>>( CmdVariant );
                            if( ( UndoCmd.m_isEditing || UndoCmd.m_isChange ) && std::strcmp( UndoCmd.m_Name.c_str(), E.m_FullName.c_str() ) == 0 )
                            {
                                property::editor::details::onRender( UndoCmd, Value, *E.m_pUserData, E.m_Flags );

                                if( UndoCmd.m_isChange )
                                    property::set( *C.m_Base.first, C.m_Base.second, E.m_FullName.c_str(), UndoCmd.m_NewValue );

                                if( UndoCmd.m_isEditing == false )
                                {
                                    // Make sure to mark this cmd is done
                                    UndoCmd.m_isChange = false;

                                    // Make it an official undo step
                                    m_UndoSystem.m_Index++;
                                    assert( m_UndoSystem.m_Index == static_cast<int>(m_UndoSystem.m_lCmds.size()) );
                                }
                                return;
                            }
                        }
                    }
                
                    // Any other entry except the editing entry gets handle here
                    property::editor::undo::cmd<T> Cmd;
                    property::editor::details::onRender(Cmd, Value, *E.m_pUserData, E.m_Flags);
                    if( Cmd.m_isEditing || Cmd.m_isChange )
                    {
                        assert( m_UndoSystem.m_Index <= static_cast<int>(m_UndoSystem.m_lCmds.size()) );

                        // Set the property value
                        if( Cmd.m_isChange )
                            property::set(*C.m_Base.first, C.m_Base.second, E.m_FullName.c_str(), Cmd.m_NewValue);

                        // Make sure we reset the undo buffer to current entry
                        if( m_UndoSystem.m_Index < static_cast<int>(m_UndoSystem.m_lCmds.size()) )
                            m_UndoSystem.m_lCmds.erase( m_UndoSystem.m_lCmds.begin() + m_UndoSystem.m_Index, m_UndoSystem.m_lCmds.end()  );

                        // Make sure we don't have more entries than we should
                        if( m_UndoSystem.m_Index > m_UndoSystem.m_MaxSteps )
                        {
                            m_UndoSystem.m_lCmds.erase(m_UndoSystem.m_lCmds.begin());
                            m_UndoSystem.m_Index--;
                        }

                        // Insert the cmd into the list
                        Cmd.m_Name.assign( E.m_FullName.c_str() );
                        Cmd.m_pTable    = C.m_Base.first;
                        Cmd.m_pInstance = C.m_Base.second;

                        m_UndoSystem.m_lCmds.push_back( std::move(Cmd) );
                    }

                }, E.m_Data );
            }
        }

        ImGui::PopItemWidth();
        ImGui::PopID();
    }

    //
    // Pop any scope
    //
    while( iDepth >= 0 ) 
        PopTree( iStart, iEnd );
}

//-------------------------------------------------------------------------------------------------

void property::inspector::Show( void ) noexcept
{
    // Anything to render?
    if( m_lEntities.size() == 0 ) 
        return;

    //
    // Refresh all the properties
    //
    RefreshAllProperties();

    //
    // If we have multiple Entities refactor components
    //

    //
    // Render each of the components
    //
    for ( auto& E : m_lEntities )
    {
        int GlobalIndex = 0;
        for ( auto& C : E->m_lComponents )
        {
            ImGui::Columns( 2 );
            Render( *C, GlobalIndex );
            ImGui::Columns( 1 );
        }
    }
}

//-------------------------------------------------------------------------------------------------

void property::inspector::DrawBackground( int Depth, int GlobalIndex ) const noexcept
{
    if( m_Settings.m_bRenderBackgroundDepth == false ) 
        Depth = 0;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    auto Color = s_ColorCategories[Depth];

    float h, s, v;
    ImVec4 C = Color;
    ImGui::ColorConvertRGBtoHSV( C.x, C.y, C.z, h, s, v );

    if(GlobalIndex&1)
    {
        Color.SetHSV( h, s*m_Settings.m_ColorSScalar, v*m_Settings.m_ColorVScalar1 );
    }
    else
    {
        Color.SetHSV( h, s*m_Settings.m_ColorSScalar, v*m_Settings.m_ColorVScalar2 );
    }


    ImGui::GetWindowDrawList()->AddRectFilled(
        pos
        , ImVec2( pos.x + ImGui::GetContentRegionAvail().x
                , pos.y + ImGui::GetFrameHeight() )
        , Color );
}

//-----------------------------------------------------------------------------------

void property::inspector::HelpMarker( const char* desc ) const noexcept
{
    if ( ImGui::IsItemHovered() )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, m_Settings.m_HelpWindowPadding );
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos( ImGui::GetFontSize() * m_Settings.m_HelpWindowSizeInChars );
        ImGui::TextUnformatted( desc );
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
        ImGui::PopStyleVar();
    }
}

//-----------------------------------------------------------------------------------

void property::inspector::Help( const entry& Entry ) const noexcept
{
    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, m_Settings.m_HelpWindowPadding );
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos( ImGui::GetFontSize() * m_Settings.m_HelpWindowSizeInChars );

    ImGui::TextDisabled( "FullName: ");
    ImGui::SameLine();
    ImGui::Text( "%s", Entry.m_FullName.c_str() );

    ImGui::TextDisabled( "Name:     " );
    ImGui::SameLine();
    ImGui::Text( "%s", Entry.m_pUserData->m_pName );
       
    ImGui::TextDisabled( "Hash:     " );
    ImGui::SameLine();
    ImGui::Text( "0x%x", Entry.m_pUserData->m_NameHash );

    if( Entry.m_pUserData->m_pHelp )
    {
        ImGui::Separator();
        ImGui::TextUnformatted( Entry.m_pUserData->m_pHelp );
    }
    else
    {
        ImGui::TextDisabled( "Help:     " );
        ImGui::SameLine();
        ImGui::Text( "none provided" );
    }

    ImGui::EndTooltip();
    ImGui::PopStyleVar();
}

//-----------------------------------------------------------------------------------

property_begin( property::inspector::settings )
{
      property_var  ( m_WindowPadding.x                             )
          .EDStyle  ( edstyle<float>::ScrollBar( 0.0f, 20.0f )      )
          .Help     ( "Blank Border for the property window in X"   )
    , property_var  ( m_WindowPadding.y                             )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
          .Help     ( "Blank Border for the property window in Y"   )
    , property_var  ( m_TableFramePadding.x                         )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
          .Help     ( "Main/Top Property Border size in X"          )
    , property_var  ( m_TableFramePadding.y                         )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 40.0f)        )
          .Help     ( "Main/Top Property Border size in Y"          )
    , property_var  ( m_FramePadding.x                              )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
    , property_var  ( m_FramePadding.y                              )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
    , property_var  ( m_ItemSpacing.x                               )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
    , property_var  ( m_ItemSpacing.y                               )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
    , property_var  ( m_IndentSpacing                               )
          .EDStyle  ( edstyle<float>::ScrollBar(0.0f, 20.0f)        )
    , property_scope_begin( "Background" )
      {
          property_var  ( m_bRenderLeftBackground                               )
            .Name       ( "RenderLeft"                                          )
            .Help       ( "Disable the rendering of the background on the left" )
        , property_var  ( m_bRenderRightBackground                              )
            .Name       ( "RenderRight"                                         )
            .Help       ( "Disable the rendering of the background on the right")
        , property_var  ( m_bRenderBackgroundDepth                              )
            .Name		( "Depth"                                               )
            .Help       ( "Disable the rendering of multiple color background"  )
#if !defined(_MSC_VER) || (_MSC_VER >= 1920)                                    // This should work, only visual studio 2017 has issues with this
            .DynamicFlags([](const std::byte & Bytes) noexcept->property::flags::type
            {
                auto& Self = reinterpret_cast<const t_self&>(Bytes);
                // Disable Y property when m_X is equal to 5
                if ( Self.m_bRenderLeftBackground == false && Self.m_bRenderRightBackground == false ) return property::flags::DISABLE;
                return {};
            })
#endif
        , property_var  ( m_ColorVScalar1                                           )
            .EDStyle    ( edstyle<float>::ScrollBar( 0.0f, 2.0f )                   )
            .Help       ( "Changes the Luminosity of one of the alternate colors for the background" )
        , property_var  ( m_ColorVScalar2                                           )
            .EDStyle    ( edstyle<float>::ScrollBar( 0.0f, 2.0f )                   )
            .Help       ( "Changes the Luminosity of one of the alternate colors for the background" )
        , property_var  ( m_ColorSScalar                                            )
            .EDStyle    ( edstyle<float>::ScrollBar( 0.0f, 2.0f )                   )
            .Help       ( "Changes the Saturation for all the colors in the background" )

      } property_scope_end() 
    , property_scope_begin( "Help Popup" )
      {
          property_var  ( m_HelpWindowPadding.x                         )
            .Name       ( "WindowPadding.X"                             )
            .EDStyle    ( edstyle<float>::ScrollBar( 0.0f, 20.0f )      )
            .Help       ( "Border size in X for the help popup"         )
        , property_var  ( m_HelpWindowPadding.y                         )
            .Name       ( "WindowPadding.Y"                             )
            .EDStyle    ( edstyle<float>::ScrollBar( 0.0f, 20.0f )      )
            .Help       ( "Border size in Y for the help popup"         )
        , property_var  ( m_HelpWindowSizeInChars                       )
            .Name       ( "MaxSizeInChars"                              )
            .EDStyle    ( edstyle<int>::ScrollBar( 1, 200 )             )
            .Help       ( "Max Size of the help window popup when it opens" )
      } property_scope_end()
}
property_end()

//-----------------------------------------------------------------------------------

property_begin_name(property::inspector, "Inspector")
{
      property_var  (m_Settings)
    , property_var  (m_UndoSystem)
}
property_vend_cpp(property::inspector)
