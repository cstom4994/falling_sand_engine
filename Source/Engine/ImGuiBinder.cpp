// Copyright(c) 2022, KaoruXun All rights reserved.

// This source code file contains some stolen things
// https://github.com/rsenn/qjs-imgui (Unlicense) by Roman Senn

#include <cassert>
#include <cctype>
#include <imgui.h>

#include "ImGuiBinder.hpp"

#include "Engine/ImGuiImplement.hpp"

#ifndef _METADOT_IMGUI_JSBIND_CONSTANTS
#define _METADOT_IMGUI_JSBIND_CONSTANTS

static const JSCFunctionListEntry js_imgui_window_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiWindowFlags_None, 0),
        JS_PROP_INT32_DEF("NoTitleBar", ImGuiWindowFlags_NoTitleBar, 0),
        JS_PROP_INT32_DEF("NoResize", ImGuiWindowFlags_NoResize, 0),
        JS_PROP_INT32_DEF("NoMove", ImGuiWindowFlags_NoMove, 0),
        JS_PROP_INT32_DEF("NoScrollbar", ImGuiWindowFlags_NoScrollbar, 0),
        JS_PROP_INT32_DEF("NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse, 0),
        JS_PROP_INT32_DEF("NoCollapse", ImGuiWindowFlags_NoCollapse, 0),
        JS_PROP_INT32_DEF("AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize, 0),
        JS_PROP_INT32_DEF("NoBackground", ImGuiWindowFlags_NoBackground, 0),
        JS_PROP_INT32_DEF("NoSavedSettings", ImGuiWindowFlags_NoSavedSettings, 0),
        JS_PROP_INT32_DEF("NoMouseInputs", ImGuiWindowFlags_NoMouseInputs, 0),
        JS_PROP_INT32_DEF("MenuBar", ImGuiWindowFlags_MenuBar, 0),
        JS_PROP_INT32_DEF("HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar, 0),
        JS_PROP_INT32_DEF("NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing, 0),
        JS_PROP_INT32_DEF("NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus, 0),
        JS_PROP_INT32_DEF("AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar, 0),
        JS_PROP_INT32_DEF("AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar,
                          0),
        JS_PROP_INT32_DEF("AlwaysUseWindowPadding", ImGuiWindowFlags_AlwaysUseWindowPadding, 0),
        JS_PROP_INT32_DEF("NoNavInputs", ImGuiWindowFlags_NoNavInputs, 0),
        JS_PROP_INT32_DEF("NoNavFocus", ImGuiWindowFlags_NoNavFocus, 0),
        JS_PROP_INT32_DEF("UnsavedDocument", ImGuiWindowFlags_UnsavedDocument, 0),
        JS_PROP_INT32_DEF("NoNav", ImGuiWindowFlags_NoNav, 0),
        JS_PROP_INT32_DEF("NoDecoration", ImGuiWindowFlags_NoDecoration, 0),
        JS_PROP_INT32_DEF("NoInputs", ImGuiWindowFlags_NoInputs, 0),
        JS_PROP_INT32_DEF("NavFlattened", ImGuiWindowFlags_NavFlattened, 0),
        JS_PROP_INT32_DEF("ChildWindow", ImGuiWindowFlags_ChildWindow, 0),
        JS_PROP_INT32_DEF("Tooltip", ImGuiWindowFlags_Tooltip, 0),
        JS_PROP_INT32_DEF("Popup", ImGuiWindowFlags_Popup, 0),
        JS_PROP_INT32_DEF("Modal", ImGuiWindowFlags_Modal, 0),
        JS_PROP_INT32_DEF("ChildMenu", ImGuiWindowFlags_ChildMenu, 0),
};

static const JSCFunctionListEntry js_imgui_inputtext_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiInputTextFlags_None, 0),
        JS_PROP_INT32_DEF("CharsDecimal", ImGuiInputTextFlags_CharsDecimal, 0),
        JS_PROP_INT32_DEF("CharsHexadecimal", ImGuiInputTextFlags_CharsHexadecimal, 0),
        JS_PROP_INT32_DEF("CharsUppercase", ImGuiInputTextFlags_CharsUppercase, 0),
        JS_PROP_INT32_DEF("CharsNoBlank", ImGuiInputTextFlags_CharsNoBlank, 0),
        JS_PROP_INT32_DEF("AutoSelectAll", ImGuiInputTextFlags_AutoSelectAll, 0),
        JS_PROP_INT32_DEF("EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue, 0),
        JS_PROP_INT32_DEF("CallbackCompletion", ImGuiInputTextFlags_CallbackCompletion, 0),
        JS_PROP_INT32_DEF("CallbackHistory", ImGuiInputTextFlags_CallbackHistory, 0),
        JS_PROP_INT32_DEF("CallbackAlways", ImGuiInputTextFlags_CallbackAlways, 0),
        JS_PROP_INT32_DEF("CallbackCharFilter", ImGuiInputTextFlags_CallbackCharFilter, 0),
        JS_PROP_INT32_DEF("AllowTabInput", ImGuiInputTextFlags_AllowTabInput, 0),
        JS_PROP_INT32_DEF("CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine, 0),
        JS_PROP_INT32_DEF("NoHorizontalScroll", ImGuiInputTextFlags_NoHorizontalScroll, 0),
        JS_PROP_INT32_DEF("AlwaysOverwrite", ImGuiInputTextFlags_AlwaysOverwrite, 0),
        JS_PROP_INT32_DEF("ReadOnly", ImGuiInputTextFlags_ReadOnly, 0),
        JS_PROP_INT32_DEF("Password", ImGuiInputTextFlags_Password, 0),
        JS_PROP_INT32_DEF("NoUndoRedo", ImGuiInputTextFlags_NoUndoRedo, 0),
        JS_PROP_INT32_DEF("CharsScientific", ImGuiInputTextFlags_CharsScientific, 0),
        JS_PROP_INT32_DEF("CallbackResize", ImGuiInputTextFlags_CallbackResize, 0),
        JS_PROP_INT32_DEF("CallbackEdit", ImGuiInputTextFlags_CallbackEdit, 0),
};

static const JSCFunctionListEntry js_imgui_treenode_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiTreeNodeFlags_None, 0),
        JS_PROP_INT32_DEF("Selected", ImGuiTreeNodeFlags_Selected, 0),
        JS_PROP_INT32_DEF("Framed", ImGuiTreeNodeFlags_Framed, 0),
        JS_PROP_INT32_DEF("AllowItemOverlap", ImGuiTreeNodeFlags_AllowItemOverlap, 0),
        JS_PROP_INT32_DEF("NoTreePushOnOpen", ImGuiTreeNodeFlags_NoTreePushOnOpen, 0),
        JS_PROP_INT32_DEF("NoAutoOpenOnLog", ImGuiTreeNodeFlags_NoAutoOpenOnLog, 0),
        JS_PROP_INT32_DEF("DefaultOpen", ImGuiTreeNodeFlags_DefaultOpen, 0),
        JS_PROP_INT32_DEF("OpenOnDoubleClick", ImGuiTreeNodeFlags_OpenOnDoubleClick, 0),
        JS_PROP_INT32_DEF("OpenOnArrow", ImGuiTreeNodeFlags_OpenOnArrow, 0),
        JS_PROP_INT32_DEF("Leaf", ImGuiTreeNodeFlags_Leaf, 0),
        JS_PROP_INT32_DEF("Bullet", ImGuiTreeNodeFlags_Bullet, 0),
        JS_PROP_INT32_DEF("FramePadding", ImGuiTreeNodeFlags_FramePadding, 0),
        JS_PROP_INT32_DEF("SpanAvailWidth", ImGuiTreeNodeFlags_SpanAvailWidth, 0),
        JS_PROP_INT32_DEF("SpanFullWidth", ImGuiTreeNodeFlags_SpanFullWidth, 0),
        JS_PROP_INT32_DEF("NavLeftJumpsBackHere", ImGuiTreeNodeFlags_NavLeftJumpsBackHere, 0),
        JS_PROP_INT32_DEF("CollapsingHeader", ImGuiTreeNodeFlags_CollapsingHeader, 0),
};

static const JSCFunctionListEntry js_imgui_popup_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiPopupFlags_None, 0),
        JS_PROP_INT32_DEF("MouseButtonLeft", ImGuiPopupFlags_MouseButtonLeft, 0),
        JS_PROP_INT32_DEF("MouseButtonRight", ImGuiPopupFlags_MouseButtonRight, 0),
        JS_PROP_INT32_DEF("MouseButtonMiddle", ImGuiPopupFlags_MouseButtonMiddle, 0),
        JS_PROP_INT32_DEF("MouseButtonMask_", ImGuiPopupFlags_MouseButtonMask_, 0),
        JS_PROP_INT32_DEF("MouseButtonDefault_", ImGuiPopupFlags_MouseButtonDefault_, 0),
        JS_PROP_INT32_DEF("NoOpenOverExistingPopup", ImGuiPopupFlags_NoOpenOverExistingPopup, 0),
        JS_PROP_INT32_DEF("NoOpenOverItems", ImGuiPopupFlags_NoOpenOverItems, 0),
        JS_PROP_INT32_DEF("AnyPopupId", ImGuiPopupFlags_AnyPopupId, 0),
        JS_PROP_INT32_DEF("AnyPopupLevel", ImGuiPopupFlags_AnyPopupLevel, 0),
        JS_PROP_INT32_DEF("AnyPopup", ImGuiPopupFlags_AnyPopup, 0),
};

static const JSCFunctionListEntry js_imgui_selectable_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiSelectableFlags_None, 0),
        JS_PROP_INT32_DEF("DontClosePopups", ImGuiSelectableFlags_DontClosePopups, 0),
        JS_PROP_INT32_DEF("SpanAllColumns", ImGuiSelectableFlags_SpanAllColumns, 0),
        JS_PROP_INT32_DEF("AllowDoubleClick", ImGuiSelectableFlags_AllowDoubleClick, 0),
        JS_PROP_INT32_DEF("Disabled", ImGuiSelectableFlags_Disabled, 0),
        JS_PROP_INT32_DEF("AllowItemOverlap", ImGuiSelectableFlags_AllowItemOverlap, 0),
};

static const JSCFunctionListEntry js_imgui_combo_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiComboFlags_None, 0),
        JS_PROP_INT32_DEF("PopupAlignLeft", ImGuiComboFlags_PopupAlignLeft, 0),
        JS_PROP_INT32_DEF("HeightSmall", ImGuiComboFlags_HeightSmall, 0),
        JS_PROP_INT32_DEF("HeightRegular", ImGuiComboFlags_HeightRegular, 0),
        JS_PROP_INT32_DEF("HeightLarge", ImGuiComboFlags_HeightLarge, 0),
        JS_PROP_INT32_DEF("HeightLargest", ImGuiComboFlags_HeightLargest, 0),
        JS_PROP_INT32_DEF("NoArrowButton", ImGuiComboFlags_NoArrowButton, 0),
        JS_PROP_INT32_DEF("NoPreview", ImGuiComboFlags_NoPreview, 0),
        JS_PROP_INT32_DEF("HeightMask_", ImGuiComboFlags_HeightMask_, 0),
};

static const JSCFunctionListEntry js_imgui_tabbar_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiTabBarFlags_None, 0),
        JS_PROP_INT32_DEF("Reorderable", ImGuiTabBarFlags_Reorderable, 0),
        JS_PROP_INT32_DEF("AutoSelectNewTabs", ImGuiTabBarFlags_AutoSelectNewTabs, 0),
        JS_PROP_INT32_DEF("TabListPopupButton", ImGuiTabBarFlags_TabListPopupButton, 0),
        JS_PROP_INT32_DEF("NoCloseWithMiddleMouseButton",
                          ImGuiTabBarFlags_NoCloseWithMiddleMouseButton, 0),
        JS_PROP_INT32_DEF("NoTabListScrollingButtons", ImGuiTabBarFlags_NoTabListScrollingButtons,
                          0),
        JS_PROP_INT32_DEF("NoTooltip", ImGuiTabBarFlags_NoTooltip, 0),
        JS_PROP_INT32_DEF("FittingPolicyResizeDown", ImGuiTabBarFlags_FittingPolicyResizeDown, 0),
        JS_PROP_INT32_DEF("FittingPolicyScroll", ImGuiTabBarFlags_FittingPolicyScroll, 0),
        JS_PROP_INT32_DEF("FittingPolicyMask_", ImGuiTabBarFlags_FittingPolicyMask_, 0),
        JS_PROP_INT32_DEF("FittingPolicyDefault_", ImGuiTabBarFlags_FittingPolicyDefault_, 0),
};

static const JSCFunctionListEntry js_imgui_tabitem_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiTabItemFlags_None, 0),
        JS_PROP_INT32_DEF("UnsavedDocument", ImGuiTabItemFlags_UnsavedDocument, 0),
        JS_PROP_INT32_DEF("SetSelected", ImGuiTabItemFlags_SetSelected, 0),
        JS_PROP_INT32_DEF("NoCloseWithMiddleMouseButton",
                          ImGuiTabItemFlags_NoCloseWithMiddleMouseButton, 0),
        JS_PROP_INT32_DEF("NoPushId", ImGuiTabItemFlags_NoPushId, 0),
        JS_PROP_INT32_DEF("NoTooltip", ImGuiTabItemFlags_NoTooltip, 0),
        JS_PROP_INT32_DEF("NoReorder", ImGuiTabItemFlags_NoReorder, 0),
        JS_PROP_INT32_DEF("Leading", ImGuiTabItemFlags_Leading, 0),
        JS_PROP_INT32_DEF("Trailing", ImGuiTabItemFlags_Trailing, 0),
};

static const JSCFunctionListEntry js_imgui_table_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiTableFlags_None, 0),
        JS_PROP_INT32_DEF("Resizable", ImGuiTableFlags_Resizable, 0),
        JS_PROP_INT32_DEF("Reorderable", ImGuiTableFlags_Reorderable, 0),
        JS_PROP_INT32_DEF("Hideable", ImGuiTableFlags_Hideable, 0),
        JS_PROP_INT32_DEF("Sortable", ImGuiTableFlags_Sortable, 0),
        JS_PROP_INT32_DEF("NoSavedSettings", ImGuiTableFlags_NoSavedSettings, 0),
        JS_PROP_INT32_DEF("ContextMenuInBody", ImGuiTableFlags_ContextMenuInBody, 0),
        JS_PROP_INT32_DEF("RowBg", ImGuiTableFlags_RowBg, 0),
        JS_PROP_INT32_DEF("BordersInnerH", ImGuiTableFlags_BordersInnerH, 0),
        JS_PROP_INT32_DEF("BordersOuterH", ImGuiTableFlags_BordersOuterH, 0),
        JS_PROP_INT32_DEF("BordersInnerV", ImGuiTableFlags_BordersInnerV, 0),
        JS_PROP_INT32_DEF("BordersOuterV", ImGuiTableFlags_BordersOuterV, 0),
        JS_PROP_INT32_DEF("BordersH", ImGuiTableFlags_BordersH, 0),
        JS_PROP_INT32_DEF("BordersV", ImGuiTableFlags_BordersV, 0),
        JS_PROP_INT32_DEF("BordersInner", ImGuiTableFlags_BordersInner, 0),
        JS_PROP_INT32_DEF("BordersOuter", ImGuiTableFlags_BordersOuter, 0),
        JS_PROP_INT32_DEF("Borders", ImGuiTableFlags_Borders, 0),
        JS_PROP_INT32_DEF("NoBordersInBody", ImGuiTableFlags_NoBordersInBody, 0),
        JS_PROP_INT32_DEF("NoBordersInBodyUntilResize", ImGuiTableFlags_NoBordersInBodyUntilResize,
                          0),
        JS_PROP_INT32_DEF("SizingFixedFit", ImGuiTableFlags_SizingFixedFit, 0),
        JS_PROP_INT32_DEF("SizingFixedSame", ImGuiTableFlags_SizingFixedSame, 0),
        JS_PROP_INT32_DEF("SizingStretchProp", ImGuiTableFlags_SizingStretchProp, 0),
        JS_PROP_INT32_DEF("SizingStretchSame", ImGuiTableFlags_SizingStretchSame, 0),
        JS_PROP_INT32_DEF("NoHostExtendX", ImGuiTableFlags_NoHostExtendX, 0),
        JS_PROP_INT32_DEF("NoHostExtendY", ImGuiTableFlags_NoHostExtendY, 0),
        JS_PROP_INT32_DEF("NoKeepColumnsVisible", ImGuiTableFlags_NoKeepColumnsVisible, 0),
        JS_PROP_INT32_DEF("PreciseWidths", ImGuiTableFlags_PreciseWidths, 0),
        JS_PROP_INT32_DEF("NoClip", ImGuiTableFlags_NoClip, 0),
        JS_PROP_INT32_DEF("PadOuterX", ImGuiTableFlags_PadOuterX, 0),
        JS_PROP_INT32_DEF("NoPadOuterX", ImGuiTableFlags_NoPadOuterX, 0),
        JS_PROP_INT32_DEF("NoPadInnerX", ImGuiTableFlags_NoPadInnerX, 0),
        JS_PROP_INT32_DEF("ScrollX", ImGuiTableFlags_ScrollX, 0),
        JS_PROP_INT32_DEF("ScrollY", ImGuiTableFlags_ScrollY, 0),
        JS_PROP_INT32_DEF("SortMulti", ImGuiTableFlags_SortMulti, 0),
        JS_PROP_INT32_DEF("SortTristate", ImGuiTableFlags_SortTristate, 0),
        JS_PROP_INT32_DEF("SizingMask_", ImGuiTableFlags_SizingMask_, 0),
};

static const JSCFunctionListEntry js_imgui_tablecolumn_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiTableColumnFlags_None, 0),
        JS_PROP_INT32_DEF("Disabled", ImGuiTableColumnFlags_Disabled, 0),
        JS_PROP_INT32_DEF("DefaultHide", ImGuiTableColumnFlags_DefaultHide, 0),
        JS_PROP_INT32_DEF("DefaultSort", ImGuiTableColumnFlags_DefaultSort, 0),
        JS_PROP_INT32_DEF("WidthStretch", ImGuiTableColumnFlags_WidthStretch, 0),
        JS_PROP_INT32_DEF("WidthFixed", ImGuiTableColumnFlags_WidthFixed, 0),
        JS_PROP_INT32_DEF("NoResize", ImGuiTableColumnFlags_NoResize, 0),
        JS_PROP_INT32_DEF("NoReorder", ImGuiTableColumnFlags_NoReorder, 0),
        JS_PROP_INT32_DEF("NoHide", ImGuiTableColumnFlags_NoHide, 0),
        JS_PROP_INT32_DEF("NoClip", ImGuiTableColumnFlags_NoClip, 0),
        JS_PROP_INT32_DEF("NoSort", ImGuiTableColumnFlags_NoSort, 0),
        JS_PROP_INT32_DEF("NoSortAscending", ImGuiTableColumnFlags_NoSortAscending, 0),
        JS_PROP_INT32_DEF("NoSortDescending", ImGuiTableColumnFlags_NoSortDescending, 0),
        JS_PROP_INT32_DEF("NoHeaderLabel", ImGuiTableColumnFlags_NoHeaderLabel, 0),
        JS_PROP_INT32_DEF("NoHeaderWidth", ImGuiTableColumnFlags_NoHeaderWidth, 0),
        JS_PROP_INT32_DEF("PreferSortAscending", ImGuiTableColumnFlags_PreferSortAscending, 0),
        JS_PROP_INT32_DEF("PreferSortDescending", ImGuiTableColumnFlags_PreferSortDescending, 0),
        JS_PROP_INT32_DEF("IndentEnable", ImGuiTableColumnFlags_IndentEnable, 0),
        JS_PROP_INT32_DEF("IndentDisable", ImGuiTableColumnFlags_IndentDisable, 0),
        JS_PROP_INT32_DEF("IsEnabled", ImGuiTableColumnFlags_IsEnabled, 0),
        JS_PROP_INT32_DEF("IsVisible", ImGuiTableColumnFlags_IsVisible, 0),
        JS_PROP_INT32_DEF("IsSorted", ImGuiTableColumnFlags_IsSorted, 0),
        JS_PROP_INT32_DEF("IsHovered", ImGuiTableColumnFlags_IsHovered, 0),
        JS_PROP_INT32_DEF("WidthMask_", ImGuiTableColumnFlags_WidthMask_, 0),
        JS_PROP_INT32_DEF("IndentMask_", ImGuiTableColumnFlags_IndentMask_, 0),
        JS_PROP_INT32_DEF("StatusMask_", ImGuiTableColumnFlags_StatusMask_, 0),
        JS_PROP_INT32_DEF("NoDirectResize_", ImGuiTableColumnFlags_NoDirectResize_, 0),
};

static const JSCFunctionListEntry js_imgui_tablerow_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiTableRowFlags_None, 0),
        JS_PROP_INT32_DEF("Headers", ImGuiTableRowFlags_Headers, 0),
};

static const JSCFunctionListEntry js_imgui_tablebgtarget[] = {
        JS_PROP_INT32_DEF("None", ImGuiTableBgTarget_None, 0),
        JS_PROP_INT32_DEF("RowBg0", ImGuiTableBgTarget_RowBg0, 0),
        JS_PROP_INT32_DEF("RowBg1", ImGuiTableBgTarget_RowBg1, 0),
        JS_PROP_INT32_DEF("CellBg", ImGuiTableBgTarget_CellBg, 0),
};

static const JSCFunctionListEntry js_imgui_focused_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiFocusedFlags_None, 0),
        JS_PROP_INT32_DEF("ChildWindows", ImGuiFocusedFlags_ChildWindows, 0),
        JS_PROP_INT32_DEF("RootWindow", ImGuiFocusedFlags_RootWindow, 0),
        JS_PROP_INT32_DEF("AnyWindow", ImGuiFocusedFlags_AnyWindow, 0),
        JS_PROP_INT32_DEF("RootAndChildWindows", ImGuiFocusedFlags_RootAndChildWindows, 0),
};

static const JSCFunctionListEntry js_imgui_hovered_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiHoveredFlags_None, 0),
        JS_PROP_INT32_DEF("ChildWindows", ImGuiHoveredFlags_ChildWindows, 0),
        JS_PROP_INT32_DEF("RootWindow", ImGuiHoveredFlags_RootWindow, 0),
        JS_PROP_INT32_DEF("AnyWindow", ImGuiHoveredFlags_AnyWindow, 0),
        JS_PROP_INT32_DEF("AllowWhenBlockedByPopup", ImGuiHoveredFlags_AllowWhenBlockedByPopup, 0),
        JS_PROP_INT32_DEF("AllowWhenBlockedByActiveItem",
                          ImGuiHoveredFlags_AllowWhenBlockedByActiveItem, 0),
        JS_PROP_INT32_DEF("AllowWhenOverlapped", ImGuiHoveredFlags_AllowWhenOverlapped, 0),
        JS_PROP_INT32_DEF("AllowWhenDisabled", ImGuiHoveredFlags_AllowWhenDisabled, 0),
        JS_PROP_INT32_DEF("RectOnly", ImGuiHoveredFlags_RectOnly, 0),
        JS_PROP_INT32_DEF("RootAndChildWindows", ImGuiHoveredFlags_RootAndChildWindows, 0),
};

static const JSCFunctionListEntry js_imgui_dragdrop_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiDragDropFlags_None, 0),
        JS_PROP_INT32_DEF("SourceNoPreviewTooltip", ImGuiDragDropFlags_SourceNoPreviewTooltip, 0),
        JS_PROP_INT32_DEF("SourceNoDisableHover", ImGuiDragDropFlags_SourceNoDisableHover, 0),
        JS_PROP_INT32_DEF("SourceNoHoldToOpenOthers", ImGuiDragDropFlags_SourceNoHoldToOpenOthers,
                          0),
        JS_PROP_INT32_DEF("SourceAllowNullID", ImGuiDragDropFlags_SourceAllowNullID, 0),
        JS_PROP_INT32_DEF("SourceExtern", ImGuiDragDropFlags_SourceExtern, 0),
        JS_PROP_INT32_DEF("SourceAutoExpirePayload", ImGuiDragDropFlags_SourceAutoExpirePayload, 0),
        JS_PROP_INT32_DEF("AcceptBeforeDelivery", ImGuiDragDropFlags_AcceptBeforeDelivery, 0),
        JS_PROP_INT32_DEF("AcceptNoDrawDefaultRect", ImGuiDragDropFlags_AcceptNoDrawDefaultRect, 0),
        JS_PROP_INT32_DEF("AcceptNoPreviewTooltip", ImGuiDragDropFlags_AcceptNoPreviewTooltip, 0),
        JS_PROP_INT32_DEF("AcceptPeekOnly", ImGuiDragDropFlags_AcceptPeekOnly, 0),
};

static const JSCFunctionListEntry js_imgui_datatype[] = {
        JS_PROP_INT32_DEF("S8", ImGuiDataType_S8, 0),
        JS_PROP_INT32_DEF("U8", ImGuiDataType_U8, 0),
        JS_PROP_INT32_DEF("S16", ImGuiDataType_S16, 0),
        JS_PROP_INT32_DEF("U16", ImGuiDataType_U16, 0),
        JS_PROP_INT32_DEF("S32", ImGuiDataType_S32, 0),
        JS_PROP_INT32_DEF("U32", ImGuiDataType_U32, 0),
        JS_PROP_INT32_DEF("S64", ImGuiDataType_S64, 0),
        JS_PROP_INT32_DEF("U64", ImGuiDataType_U64, 0),
        JS_PROP_INT32_DEF("Float", ImGuiDataType_Float, 0),
        JS_PROP_INT32_DEF("Double", ImGuiDataType_Double, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiDataType_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_dir[] = {
        JS_PROP_INT32_DEF("None", ImGuiDir_None, 0),
        JS_PROP_INT32_DEF("Left", ImGuiDir_Left, 0),
        JS_PROP_INT32_DEF("Right", ImGuiDir_Right, 0),
        JS_PROP_INT32_DEF("Up", ImGuiDir_Up, 0),
        JS_PROP_INT32_DEF("Down", ImGuiDir_Down, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiDir_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_sortdirection[] = {
        JS_PROP_INT32_DEF("None", ImGuiSortDirection_None, 0),
        JS_PROP_INT32_DEF("Ascending", ImGuiSortDirection_Ascending, 0),
        JS_PROP_INT32_DEF("Descending", ImGuiSortDirection_Descending, 0),
};

static const JSCFunctionListEntry js_imgui_key[] = {
        JS_PROP_INT32_DEF("Tab", ImGuiKey_Tab, 0),
        JS_PROP_INT32_DEF("LeftArrow", ImGuiKey_LeftArrow, 0),
        JS_PROP_INT32_DEF("RightArrow", ImGuiKey_RightArrow, 0),
        JS_PROP_INT32_DEF("UpArrow", ImGuiKey_UpArrow, 0),
        JS_PROP_INT32_DEF("DownArrow", ImGuiKey_DownArrow, 0),
        JS_PROP_INT32_DEF("PageUp", ImGuiKey_PageUp, 0),
        JS_PROP_INT32_DEF("PageDown", ImGuiKey_PageDown, 0),
        JS_PROP_INT32_DEF("Home", ImGuiKey_Home, 0),
        JS_PROP_INT32_DEF("End", ImGuiKey_End, 0),
        JS_PROP_INT32_DEF("Insert", ImGuiKey_Insert, 0),
        JS_PROP_INT32_DEF("Delete", ImGuiKey_Delete, 0),
        JS_PROP_INT32_DEF("Backspace", ImGuiKey_Backspace, 0),
        JS_PROP_INT32_DEF("Space", ImGuiKey_Space, 0),
        JS_PROP_INT32_DEF("Enter", ImGuiKey_Enter, 0),
        JS_PROP_INT32_DEF("Escape", ImGuiKey_Escape, 0),
        JS_PROP_INT32_DEF("KeyPadEnter", ImGuiKey_KeyPadEnter, 0),
        JS_PROP_INT32_DEF("A", ImGuiKey_A, 0),
        JS_PROP_INT32_DEF("C", ImGuiKey_C, 0),
        JS_PROP_INT32_DEF("V", ImGuiKey_V, 0),
        JS_PROP_INT32_DEF("X", ImGuiKey_X, 0),
        JS_PROP_INT32_DEF("Y", ImGuiKey_Y, 0),
        JS_PROP_INT32_DEF("Z", ImGuiKey_Z, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiKey_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_keymod_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiModFlags_None, 0),
        JS_PROP_INT32_DEF("Ctrl", ImGuiModFlags_Ctrl, 0),
        JS_PROP_INT32_DEF("Shift", ImGuiModFlags_Shift, 0),
        JS_PROP_INT32_DEF("Alt", ImGuiModFlags_Alt, 0),
        JS_PROP_INT32_DEF("Super", ImGuiModFlags_Super, 0),
};

static const JSCFunctionListEntry js_imgui_navinput[] = {
        JS_PROP_INT32_DEF("Activate", ImGuiNavInput_Activate, 0),
        JS_PROP_INT32_DEF("Cancel", ImGuiNavInput_Cancel, 0),
        JS_PROP_INT32_DEF("Input", ImGuiNavInput_Input, 0),
        JS_PROP_INT32_DEF("Menu", ImGuiNavInput_Menu, 0),
        JS_PROP_INT32_DEF("DpadLeft", ImGuiNavInput_DpadLeft, 0),
        JS_PROP_INT32_DEF("DpadRight", ImGuiNavInput_DpadRight, 0),
        JS_PROP_INT32_DEF("DpadUp", ImGuiNavInput_DpadUp, 0),
        JS_PROP_INT32_DEF("DpadDown", ImGuiNavInput_DpadDown, 0),
        JS_PROP_INT32_DEF("LStickLeft", ImGuiNavInput_LStickLeft, 0),
        JS_PROP_INT32_DEF("LStickRight", ImGuiNavInput_LStickRight, 0),
        JS_PROP_INT32_DEF("LStickUp", ImGuiNavInput_LStickUp, 0),
        JS_PROP_INT32_DEF("LStickDown", ImGuiNavInput_LStickDown, 0),
        JS_PROP_INT32_DEF("FocusPrev", ImGuiNavInput_FocusPrev, 0),
        JS_PROP_INT32_DEF("FocusNext", ImGuiNavInput_FocusNext, 0),
        JS_PROP_INT32_DEF("TweakSlow", ImGuiNavInput_TweakSlow, 0),
        JS_PROP_INT32_DEF("TweakFast", ImGuiNavInput_TweakFast, 0),
        // JS_PROP_INT32_DEF("KeyLeft_", ImGuiNavInput_KeyLeft_, 0),
        // JS_PROP_INT32_DEF("KeyRight_", ImGuiNavInput_KeyRight_, 0),
        // JS_PROP_INT32_DEF("KeyUp_", ImGuiNavInput_KeyUp_, 0),
        // JS_PROP_INT32_DEF("KeyDown_", ImGuiNavInput_KeyDown_, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiNavInput_COUNT, 0),
        // JS_PROP_INT32_DEF("InternalStart_", ImGuiNavInput_InternalStart_, 0),
};

static const JSCFunctionListEntry js_imgui_config_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiConfigFlags_None, 0),
        JS_PROP_INT32_DEF("NavEnableKeyboard", ImGuiConfigFlags_NavEnableKeyboard, 0),
        JS_PROP_INT32_DEF("NavEnableGamepad", ImGuiConfigFlags_NavEnableGamepad, 0),
        JS_PROP_INT32_DEF("NavEnableSetMousePos", ImGuiConfigFlags_NavEnableSetMousePos, 0),
        JS_PROP_INT32_DEF("NavNoCaptureKeyboard", ImGuiConfigFlags_NavNoCaptureKeyboard, 0),
        JS_PROP_INT32_DEF("NoMouse", ImGuiConfigFlags_NoMouse, 0),
        JS_PROP_INT32_DEF("NoMouseCursorChange", ImGuiConfigFlags_NoMouseCursorChange, 0),
        JS_PROP_INT32_DEF("IsSRGB", ImGuiConfigFlags_IsSRGB, 0),
        JS_PROP_INT32_DEF("IsTouchScreen", ImGuiConfigFlags_IsTouchScreen, 0),
};

static const JSCFunctionListEntry js_imgui_backend_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiBackendFlags_None, 0),
        JS_PROP_INT32_DEF("HasGamepad", ImGuiBackendFlags_HasGamepad, 0),
        JS_PROP_INT32_DEF("HasMouseCursors", ImGuiBackendFlags_HasMouseCursors, 0),
        JS_PROP_INT32_DEF("HasSetMousePos", ImGuiBackendFlags_HasSetMousePos, 0),
        JS_PROP_INT32_DEF("RendererHasVtxOffset", ImGuiBackendFlags_RendererHasVtxOffset, 0),
};

static const JSCFunctionListEntry js_imgui_col[] = {
        JS_PROP_INT32_DEF("Text", ImGuiCol_Text, 0),
        JS_PROP_INT32_DEF("TextDisabled", ImGuiCol_TextDisabled, 0),
        JS_PROP_INT32_DEF("WindowBg", ImGuiCol_WindowBg, 0),
        JS_PROP_INT32_DEF("ChildBg", ImGuiCol_ChildBg, 0),
        JS_PROP_INT32_DEF("PopupBg", ImGuiCol_PopupBg, 0),
        JS_PROP_INT32_DEF("Border", ImGuiCol_Border, 0),
        JS_PROP_INT32_DEF("BorderShadow", ImGuiCol_BorderShadow, 0),
        JS_PROP_INT32_DEF("FrameBg", ImGuiCol_FrameBg, 0),
        JS_PROP_INT32_DEF("FrameBgHovered", ImGuiCol_FrameBgHovered, 0),
        JS_PROP_INT32_DEF("FrameBgActive", ImGuiCol_FrameBgActive, 0),
        JS_PROP_INT32_DEF("TitleBg", ImGuiCol_TitleBg, 0),
        JS_PROP_INT32_DEF("TitleBgActive", ImGuiCol_TitleBgActive, 0),
        JS_PROP_INT32_DEF("TitleBgCollapsed", ImGuiCol_TitleBgCollapsed, 0),
        JS_PROP_INT32_DEF("MenuBarBg", ImGuiCol_MenuBarBg, 0),
        JS_PROP_INT32_DEF("ScrollbarBg", ImGuiCol_ScrollbarBg, 0),
        JS_PROP_INT32_DEF("ScrollbarGrab", ImGuiCol_ScrollbarGrab, 0),
        JS_PROP_INT32_DEF("ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered, 0),
        JS_PROP_INT32_DEF("ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive, 0),
        JS_PROP_INT32_DEF("CheckMark", ImGuiCol_CheckMark, 0),
        JS_PROP_INT32_DEF("SliderGrab", ImGuiCol_SliderGrab, 0),
        JS_PROP_INT32_DEF("SliderGrabActive", ImGuiCol_SliderGrabActive, 0),
        JS_PROP_INT32_DEF("Button", ImGuiCol_Button, 0),
        JS_PROP_INT32_DEF("ButtonHovered", ImGuiCol_ButtonHovered, 0),
        JS_PROP_INT32_DEF("ButtonActive", ImGuiCol_ButtonActive, 0),
        JS_PROP_INT32_DEF("Header", ImGuiCol_Header, 0),
        JS_PROP_INT32_DEF("HeaderHovered", ImGuiCol_HeaderHovered, 0),
        JS_PROP_INT32_DEF("HeaderActive", ImGuiCol_HeaderActive, 0),
        JS_PROP_INT32_DEF("Separator", ImGuiCol_Separator, 0),
        JS_PROP_INT32_DEF("SeparatorHovered", ImGuiCol_SeparatorHovered, 0),
        JS_PROP_INT32_DEF("SeparatorActive", ImGuiCol_SeparatorActive, 0),
        JS_PROP_INT32_DEF("ResizeGrip", ImGuiCol_ResizeGrip, 0),
        JS_PROP_INT32_DEF("ResizeGripHovered", ImGuiCol_ResizeGripHovered, 0),
        JS_PROP_INT32_DEF("ResizeGripActive", ImGuiCol_ResizeGripActive, 0),
        JS_PROP_INT32_DEF("Tab", ImGuiCol_Tab, 0),
        JS_PROP_INT32_DEF("TabHovered", ImGuiCol_TabHovered, 0),
        JS_PROP_INT32_DEF("TabActive", ImGuiCol_TabActive, 0),
        JS_PROP_INT32_DEF("TabUnfocused", ImGuiCol_TabUnfocused, 0),
        JS_PROP_INT32_DEF("TabUnfocusedActive", ImGuiCol_TabUnfocusedActive, 0),
        JS_PROP_INT32_DEF("PlotLines", ImGuiCol_PlotLines, 0),
        JS_PROP_INT32_DEF("PlotLinesHovered", ImGuiCol_PlotLinesHovered, 0),
        JS_PROP_INT32_DEF("PlotHistogram", ImGuiCol_PlotHistogram, 0),
        JS_PROP_INT32_DEF("PlotHistogramHovered", ImGuiCol_PlotHistogramHovered, 0),
        JS_PROP_INT32_DEF("TableHeaderBg", ImGuiCol_TableHeaderBg, 0),
        JS_PROP_INT32_DEF("TableBorderStrong", ImGuiCol_TableBorderStrong, 0),
        JS_PROP_INT32_DEF("TableBorderLight", ImGuiCol_TableBorderLight, 0),
        JS_PROP_INT32_DEF("TableRowBg", ImGuiCol_TableRowBg, 0),
        JS_PROP_INT32_DEF("TableRowBgAlt", ImGuiCol_TableRowBgAlt, 0),
        JS_PROP_INT32_DEF("TextSelectedBg", ImGuiCol_TextSelectedBg, 0),
        JS_PROP_INT32_DEF("DragDropTarget", ImGuiCol_DragDropTarget, 0),
        JS_PROP_INT32_DEF("NavHighlight", ImGuiCol_NavHighlight, 0),
        JS_PROP_INT32_DEF("NavWindowingHighlight", ImGuiCol_NavWindowingHighlight, 0),
        JS_PROP_INT32_DEF("NavWindowingDimBg", ImGuiCol_NavWindowingDimBg, 0),
        JS_PROP_INT32_DEF("ModalWindowDimBg", ImGuiCol_ModalWindowDimBg, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiCol_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_stylevar[] = {
        JS_PROP_INT32_DEF("Alpha", ImGuiStyleVar_Alpha, 0),
        JS_PROP_INT32_DEF("DisabledAlpha", ImGuiStyleVar_DisabledAlpha, 0),
        JS_PROP_INT32_DEF("WindowPadding", ImGuiStyleVar_WindowPadding, 0),
        JS_PROP_INT32_DEF("WindowRounding", ImGuiStyleVar_WindowRounding, 0),
        JS_PROP_INT32_DEF("WindowBorderSize", ImGuiStyleVar_WindowBorderSize, 0),
        JS_PROP_INT32_DEF("WindowMinSize", ImGuiStyleVar_WindowMinSize, 0),
        JS_PROP_INT32_DEF("WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign, 0),
        JS_PROP_INT32_DEF("ChildRounding", ImGuiStyleVar_ChildRounding, 0),
        JS_PROP_INT32_DEF("ChildBorderSize", ImGuiStyleVar_ChildBorderSize, 0),
        JS_PROP_INT32_DEF("PopupRounding", ImGuiStyleVar_PopupRounding, 0),
        JS_PROP_INT32_DEF("PopupBorderSize", ImGuiStyleVar_PopupBorderSize, 0),
        JS_PROP_INT32_DEF("FramePadding", ImGuiStyleVar_FramePadding, 0),
        JS_PROP_INT32_DEF("FrameRounding", ImGuiStyleVar_FrameRounding, 0),
        JS_PROP_INT32_DEF("FrameBorderSize", ImGuiStyleVar_FrameBorderSize, 0),
        JS_PROP_INT32_DEF("ItemSpacing", ImGuiStyleVar_ItemSpacing, 0),
        JS_PROP_INT32_DEF("ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing, 0),
        JS_PROP_INT32_DEF("IndentSpacing", ImGuiStyleVar_IndentSpacing, 0),
        JS_PROP_INT32_DEF("CellPadding", ImGuiStyleVar_CellPadding, 0),
        JS_PROP_INT32_DEF("ScrollbarSize", ImGuiStyleVar_ScrollbarSize, 0),
        JS_PROP_INT32_DEF("ScrollbarRounding", ImGuiStyleVar_ScrollbarRounding, 0),
        JS_PROP_INT32_DEF("GrabMinSize", ImGuiStyleVar_GrabMinSize, 0),
        JS_PROP_INT32_DEF("GrabRounding", ImGuiStyleVar_GrabRounding, 0),
        JS_PROP_INT32_DEF("TabRounding", ImGuiStyleVar_TabRounding, 0),
        JS_PROP_INT32_DEF("ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign, 0),
        JS_PROP_INT32_DEF("SelectableTextAlign", ImGuiStyleVar_SelectableTextAlign, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiStyleVar_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_button_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiButtonFlags_None, 0),
        JS_PROP_INT32_DEF("MouseButtonLeft", ImGuiButtonFlags_MouseButtonLeft, 0),
        JS_PROP_INT32_DEF("MouseButtonRight", ImGuiButtonFlags_MouseButtonRight, 0),
        JS_PROP_INT32_DEF("MouseButtonMiddle", ImGuiButtonFlags_MouseButtonMiddle, 0),
        JS_PROP_INT32_DEF("MouseButtonMask_", ImGuiButtonFlags_MouseButtonMask_, 0),
        JS_PROP_INT32_DEF("MouseButtonDefault_", ImGuiButtonFlags_MouseButtonDefault_, 0),
};

static const JSCFunctionListEntry js_imgui_coloredit_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiColorEditFlags_None, 0),
        JS_PROP_INT32_DEF("NoAlpha", ImGuiColorEditFlags_NoAlpha, 0),
        JS_PROP_INT32_DEF("NoPicker", ImGuiColorEditFlags_NoPicker, 0),
        JS_PROP_INT32_DEF("NoOptions", ImGuiColorEditFlags_NoOptions, 0),
        JS_PROP_INT32_DEF("NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview, 0),
        JS_PROP_INT32_DEF("NoInputs", ImGuiColorEditFlags_NoInputs, 0),
        JS_PROP_INT32_DEF("NoTooltip", ImGuiColorEditFlags_NoTooltip, 0),
        JS_PROP_INT32_DEF("NoLabel", ImGuiColorEditFlags_NoLabel, 0),
        JS_PROP_INT32_DEF("NoSidePreview", ImGuiColorEditFlags_NoSidePreview, 0),
        JS_PROP_INT32_DEF("NoDragDrop", ImGuiColorEditFlags_NoDragDrop, 0),
        JS_PROP_INT32_DEF("NoBorder", ImGuiColorEditFlags_NoBorder, 0),
        JS_PROP_INT32_DEF("AlphaBar", ImGuiColorEditFlags_AlphaBar, 0),
        JS_PROP_INT32_DEF("AlphaPreview", ImGuiColorEditFlags_AlphaPreview, 0),
        JS_PROP_INT32_DEF("AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf, 0),
        JS_PROP_INT32_DEF("HDR", ImGuiColorEditFlags_HDR, 0),
        JS_PROP_INT32_DEF("DisplayRGB", ImGuiColorEditFlags_DisplayRGB, 0),
        JS_PROP_INT32_DEF("DisplayHSV", ImGuiColorEditFlags_DisplayHSV, 0),
        JS_PROP_INT32_DEF("DisplayHex", ImGuiColorEditFlags_DisplayHex, 0),
        JS_PROP_INT32_DEF("Uint8", ImGuiColorEditFlags_Uint8, 0),
        JS_PROP_INT32_DEF("Float", ImGuiColorEditFlags_Float, 0),
        JS_PROP_INT32_DEF("PickerHueBar", ImGuiColorEditFlags_PickerHueBar, 0),
        JS_PROP_INT32_DEF("PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel, 0),
        JS_PROP_INT32_DEF("InputRGB", ImGuiColorEditFlags_InputRGB, 0),
        JS_PROP_INT32_DEF("InputHSV", ImGuiColorEditFlags_InputHSV, 0),
        JS_PROP_INT32_DEF("DefaultOptions_", ImGuiColorEditFlags_DefaultOptions_, 0),
        JS_PROP_INT32_DEF("DisplayMask_", ImGuiColorEditFlags_DisplayMask_, 0),
        JS_PROP_INT32_DEF("DataTypeMask_", ImGuiColorEditFlags_DataTypeMask_, 0),
        JS_PROP_INT32_DEF("PickerMask_", ImGuiColorEditFlags_PickerMask_, 0),
        JS_PROP_INT32_DEF("InputMask_", ImGuiColorEditFlags_InputMask_, 0),

        JS_PROP_INT32_DEF("DisplayRGB", ImGuiColorEditFlags_DisplayRGB, 0),
        JS_PROP_INT32_DEF("DisplayHSV", ImGuiColorEditFlags_DisplayHSV, 0),
        JS_PROP_INT32_DEF("DisplayHEX", ImGuiColorEditFlags_DisplayHex, 0),

        // TODO Input~
};

static const JSCFunctionListEntry js_imgui_slider_flags[] = {
        JS_PROP_INT32_DEF("None", ImGuiSliderFlags_None, 0),
        JS_PROP_INT32_DEF("AlwaysClamp", ImGuiSliderFlags_AlwaysClamp, 0),
        JS_PROP_INT32_DEF("Logarithmic", ImGuiSliderFlags_Logarithmic, 0),
        JS_PROP_INT32_DEF("NoRoundToFormat", ImGuiSliderFlags_NoRoundToFormat, 0),
        JS_PROP_INT32_DEF("NoInput", ImGuiSliderFlags_NoInput, 0),
        JS_PROP_INT32_DEF("InvalidMask_", ImGuiSliderFlags_InvalidMask_, 0),
        JS_PROP_INT32_DEF("ClampOnInput", ImGuiSliderFlags_ClampOnInput, 0),
};

static const JSCFunctionListEntry js_imgui_mousebutton[] = {
        JS_PROP_INT32_DEF("Left", ImGuiMouseButton_Left, 0),
        JS_PROP_INT32_DEF("Right", ImGuiMouseButton_Right, 0),
        JS_PROP_INT32_DEF("Middle", ImGuiMouseButton_Middle, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiMouseButton_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_mousecursor[] = {
        JS_PROP_INT32_DEF("None", ImGuiMouseCursor_None, 0),
        JS_PROP_INT32_DEF("Arrow", ImGuiMouseCursor_Arrow, 0),
        JS_PROP_INT32_DEF("TextInput", ImGuiMouseCursor_TextInput, 0),
        JS_PROP_INT32_DEF("ResizeAll", ImGuiMouseCursor_ResizeAll, 0),
        JS_PROP_INT32_DEF("ResizeNS", ImGuiMouseCursor_ResizeNS, 0),
        JS_PROP_INT32_DEF("ResizeEW", ImGuiMouseCursor_ResizeEW, 0),
        JS_PROP_INT32_DEF("ResizeNESW", ImGuiMouseCursor_ResizeNESW, 0),
        JS_PROP_INT32_DEF("ResizeNWSE", ImGuiMouseCursor_ResizeNWSE, 0),
        JS_PROP_INT32_DEF("Hand", ImGuiMouseCursor_Hand, 0),
        JS_PROP_INT32_DEF("NotAllowed", ImGuiMouseCursor_NotAllowed, 0),
        JS_PROP_INT32_DEF("COUNT", ImGuiMouseCursor_COUNT, 0),
};

static const JSCFunctionListEntry js_imgui_cond[] = {
        JS_PROP_INT32_DEF("None", ImGuiCond_None, 0),
        JS_PROP_INT32_DEF("Always", ImGuiCond_Always, 0),
        JS_PROP_INT32_DEF("Once", ImGuiCond_Once, 0),
        JS_PROP_INT32_DEF("FirstUseEver", ImGuiCond_FirstUseEver, 0),
        JS_PROP_INT32_DEF("Appearing", ImGuiCond_Appearing, 0),
};

#endif

#ifndef _METADOT_IMGUI_JSBIND_PAYLOAD
#define _METADOT_IMGUI_JSBIND_PAYLOAD

thread_local JSClassID js_imgui_payload_class_id = 0;
thread_local JSValue imgui_payload_proto = {JS_TAG_UNDEFINED},
                     imgui_payload_ctor = {JS_TAG_UNDEFINED};

enum {
    PAYLOAD_CLEAR,
    PAYLOAD_IS_DATA_TYPE,
    PAYLOAD_IS_PREVIEW,
    PAYLOAD_IS_DELIVERY,
};

static inline ImGuiPayload *js_imgui_payload_data2(JSContext *ctx, JSValueConst value) {
    return static_cast<ImGuiPayload *>(JS_GetOpaque2(ctx, value, js_imgui_payload_class_id));
}

static JSValue js_imgui_payload_wrap(JSContext *ctx, ImGuiPayload *payload) {
    JSValue obj = JS_NewObjectProtoClass(ctx, imgui_payload_proto, js_imgui_payload_class_id);
    JS_SetOpaque(obj, payload);
    return obj;
}

static JSValue js_imgui_payload_constructor(JSContext *ctx, JSValueConst new_target, int argc,
                                            JSValueConst argv[]) {
    JSValue proto, obj = JS_UNDEFINED;
    ImGuiPayload *payload = new ImGuiPayload();
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_imgui_payload_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;
    JS_SetOpaque(obj, payload);
    return obj;
fail:
    js_free(ctx, payload);
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static void js_imgui_payload_finalizer(JSRuntime *rt, JSValue val) {
    ImGuiPayload *payload =
            static_cast<ImGuiPayload *>(JS_GetOpaque(val, js_imgui_payload_class_id));
    if (payload) { delete payload; }
    JS_FreeValueRT(rt, val);
}

static JSValue js_imgui_payload_functions(JSContext *ctx, JSValueConst this_val, int argc,
                                          JSValueConst argv[], int magic) {
    ImGuiPayload *payload;
    JSValue ret = JS_UNDEFINED;

    if (!(payload = js_imgui_payload_data2(ctx, this_val))) return ret;

    switch (magic) {
        case PAYLOAD_CLEAR: {
            payload->Clear();
            break;
        }
        case PAYLOAD_IS_DATA_TYPE: {
            const char *type = JS_ToCString(ctx, argv[0]);
            ret = JS_NewBool(ctx, payload->IsDataType(type));
            JS_FreeCString(ctx, type);
            break;
        }
        case PAYLOAD_IS_PREVIEW: {
            ret = JS_NewBool(ctx, payload->IsPreview());
            break;
        }
        case PAYLOAD_IS_DELIVERY: {
            ret = JS_NewBool(ctx, payload->IsDelivery());
            break;
        }
    }

    return ret;
}

static JSClassDef js_imgui_payload_class = {
        .class_name = "ImGuiPayload",
        .finalizer = js_imgui_payload_finalizer,
};

static const JSCFunctionListEntry js_imgui_payload_funcs[] = {
        JS_CFUNC_MAGIC_DEF("Clear", 0, js_imgui_payload_functions, PAYLOAD_CLEAR),
        JS_CFUNC_MAGIC_DEF("IsDataType", 1, js_imgui_payload_functions, PAYLOAD_IS_DATA_TYPE),
        JS_CFUNC_MAGIC_DEF("IsPreview", 0, js_imgui_payload_functions, PAYLOAD_IS_PREVIEW),
        JS_CFUNC_MAGIC_DEF("IsDelivery", 0, js_imgui_payload_functions, PAYLOAD_IS_DELIVERY),
};

#endif

#ifndef _METADOT_IMGUI_JSBIND_IO
#define _METADOT_IMGUI_JSBIND_IO

thread_local JSClassID js_imgui_io_class_id = 0;
thread_local JSValue imgui_io_proto = {JS_TAG_UNDEFINED}, imgui_io_ctor = {JS_TAG_UNDEFINED};

enum {
    IO_ADD_INPUT_CHARACTER,
    IO_ADD_INPUT_CHARACTERS,
    IO_CLEAR_INPUT_CHARACTERS,
    IO_ADD_FOCUS_EVENT,
};

static inline ImGuiIO *js_imgui_io_data2(JSContext *ctx, JSValueConst value) {
    return static_cast<ImGuiIO *>(JS_GetOpaque2(ctx, value, js_imgui_io_class_id));
}

static JSValue js_imgui_io_wrap(JSContext *ctx, ImGuiIO *io) {
    JSValue obj = JS_NewObjectProtoClass(ctx, imgui_io_proto, js_imgui_io_class_id);
    JS_SetOpaque(obj, io);
    return obj;
}

static JSValue js_imgui_io_constructor(JSContext *ctx, JSValueConst new_target, int argc,
                                       JSValueConst argv[]) {
    ImGuiIO *io;
    JSValue proto, obj = JS_UNDEFINED;

    /* if(!(io = static_cast<ImGuiIO*>(js_mallocz(ctx, sizeof(ImGuiIO)))))
     return JS_EXCEPTION;

   new (io)  ImGuiIO();*/
    io = new ImGuiIO();

    /* using new_target to get the prototype is necessary when the class is extended. */
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_imgui_io_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;

    JS_SetOpaque(obj, io);

    return obj;
fail:
    js_free(ctx, io);
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static void js_imgui_io_finalizer(JSRuntime *rt, JSValue val) {
    ImGuiIO *io = static_cast<ImGuiIO *>(JS_GetOpaque(val, js_imgui_io_class_id));
    if (io) { delete io; }
    JS_FreeValueRT(rt, val);
}

static JSValue js_imgui_io_functions(JSContext *ctx, JSValueConst this_val, int argc,
                                     JSValueConst argv[], int magic) {
    ImGuiIO *io;
    JSValue ret = JS_UNDEFINED;

    if (!(io = js_imgui_io_data2(ctx, this_val))) return ret;

    switch (magic) {
        case IO_ADD_INPUT_CHARACTER: {
            if (JS_IsString(argv[0])) {
                size_t len;
                const char *str = JS_ToCStringLen(ctx, &len, argv[0]);
                const uint8_t *ptr = reinterpret_cast<const uint8_t *>(str);
                int codepoint;
                if ((codepoint = unicode_from_utf8(ptr, len, &ptr)) != -1)
                    io->AddInputCharacter(codepoint);
                JS_FreeCString(ctx, str);
            } else if (JS_IsNumber(argv[0])) {
                uint32_t n;
                JS_ToUint32(ctx, &n, argv[0]);
                io->AddInputCharacter(n);
            }
            break;
        }
        case IO_ADD_INPUT_CHARACTERS: {
            const char *str = JS_ToCString(ctx, argv[0]);
            io->AddInputCharactersUTF8(str);
            JS_FreeCString(ctx, str);
            break;
        }
        case IO_CLEAR_INPUT_CHARACTERS: {
            io->ClearInputCharacters();
            break;
        }
        case IO_ADD_FOCUS_EVENT: {
            bool focused = JS_ToBool(ctx, argv[0]);
            io->AddFocusEvent(focused);
            break;
        }
    }

    return ret;
}

static JSClassDef js_imgui_io_class = {
        .class_name = "ImGuiIO",
        .finalizer = js_imgui_io_finalizer,
};

static const JSCFunctionListEntry js_imgui_io_funcs[] = {
        JS_CFUNC_MAGIC_DEF("AddInputCharacter", 1, js_imgui_io_functions, IO_ADD_INPUT_CHARACTER),
        JS_CFUNC_MAGIC_DEF("AddInputCharacters", 1, js_imgui_io_functions, IO_ADD_INPUT_CHARACTERS),
        JS_CFUNC_MAGIC_DEF("ClearInputCharacters", 0, js_imgui_io_functions,
                           IO_CLEAR_INPUT_CHARACTERS),
        JS_CFUNC_MAGIC_DEF("AddFocusEvent", 1, js_imgui_io_functions, IO_ADD_FOCUS_EVENT),
};

#endif

#ifndef _METADOT_IMGUI_JSBIND_IMFONT
#define _METADOT_IMGUI_JSBIND_IMFONT

thread_local JSClassID js_imfont_class_id = 0;
thread_local JSValue imfont_proto = {JS_TAG_UNDEFINED}, imfont_ctor = {JS_TAG_UNDEFINED};

enum {
    FONT_FIND_GLYPH,
    FONT_FIND_GLYPH_NO_FALLBACK,
    FONT_CALC_TEXT_SIZE_A,
    FONT_CALC_WORD_WRAP_POSITION_A,
    FONT_RENDER_CHAR,
    FONT_RENDER_TEXT,
    FONT_BUILD_LOOKUP_TABLE,
    FONT_CLEAR_OUTPUT_DATA,
    FONT_GROW_INDEX,
    FONT_ADD_GLYPH,
    FONT_ADD_REMAP_CHAR,
    FONT_SET_GLYPH_VISIBLE,
    FONT_IS_GLYPH_RANGE_UNUSED,
};

static inline ImFont *js_imfont_data2(JSContext *ctx, JSValueConst value) {
    return static_cast<ImFont *>(JS_GetOpaque2(ctx, value, js_imfont_class_id));
}

static JSValue js_imfont_wrap(JSContext *ctx, ImFont *payload) {
    JSValue obj = JS_NewObjectProtoClass(ctx, imfont_proto, js_imfont_class_id);
    JS_SetOpaque(obj, payload);
    return obj;
}

static JSValue js_imfont_constructor(JSContext *ctx, JSValueConst new_target, int argc,
                                     JSValueConst argv[]) {
    JSValue proto, obj = JS_UNDEFINED;
    ImFont *payload = new ImFont();
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_imfont_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;
    JS_SetOpaque(obj, payload);
    return obj;
fail:
    js_free(ctx, payload);
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static void js_imfont_finalizer(JSRuntime *rt, JSValue val) {
    ImFont *payload = static_cast<ImFont *>(JS_GetOpaque(val, js_imfont_class_id));
    if (payload) { delete payload; }
    JS_FreeValueRT(rt, val);
}

static JSValue js_imfont_functions(JSContext *ctx, JSValueConst this_val, int argc,
                                   JSValueConst argv[], int magic) {
    ImFont *payload;
    JSValue ret = JS_UNDEFINED;

    if (!(payload = js_imfont_data2(ctx, this_val))) return ret;

    switch (magic) {}

    return ret;
}

static JSClassDef js_imfont_class = {
        .class_name = "ImFont",
        .finalizer = js_imfont_finalizer,
};

static const JSCFunctionListEntry js_imfont_funcs[] = {
        JS_CFUNC_MAGIC_DEF("FindGlyph", 1, js_imfont_functions, FONT_FIND_GLYPH),
        JS_CFUNC_MAGIC_DEF("FindGlyphNoFallback", 1, js_imfont_functions,
                           FONT_FIND_GLYPH_NO_FALLBACK),
        JS_CFUNC_MAGIC_DEF("CalcTextSizeA", 4, js_imfont_functions, FONT_CALC_TEXT_SIZE_A),
        JS_CFUNC_MAGIC_DEF("CalcWordWrapPositionA", 4, js_imfont_functions,
                           FONT_CALC_WORD_WRAP_POSITION_A),
        JS_CFUNC_MAGIC_DEF("RenderChar", 5, js_imfont_functions, FONT_RENDER_CHAR),
        JS_CFUNC_MAGIC_DEF("RenderText", 7, js_imfont_functions, FONT_RENDER_TEXT),
        JS_CFUNC_MAGIC_DEF("BuildLookupTable", 0, js_imfont_functions, FONT_BUILD_LOOKUP_TABLE),
        JS_CFUNC_MAGIC_DEF("ClearOutputData", 0, js_imfont_functions, FONT_CLEAR_OUTPUT_DATA),
        JS_CFUNC_MAGIC_DEF("GrowIndex", 1, js_imfont_functions, FONT_GROW_INDEX),
        JS_CFUNC_MAGIC_DEF("AddGlyph", 11, js_imfont_functions, FONT_ADD_GLYPH),
        JS_CFUNC_MAGIC_DEF("AddRemapChar", 2, js_imfont_functions, FONT_ADD_REMAP_CHAR),
        JS_CFUNC_MAGIC_DEF("SetGlyphVisible", 2, js_imfont_functions, FONT_SET_GLYPH_VISIBLE),
        JS_CFUNC_MAGIC_DEF("IsGlyphRangeUnused", 2, js_imfont_functions,
                           FONT_IS_GLYPH_RANGE_UNUSED),

};

#endif

#ifndef _METADOT_IMGUI_JSBIND_IMFONTATLAS
#define _METADOT_IMGUI_JSBIND_IMFONTATLAS

thread_local JSClassID js_imfontatlas_class_id = 0;
thread_local JSValue imfontatlas_proto = {JS_TAG_UNDEFINED}, imfontatlas_ctor = {JS_TAG_UNDEFINED};

enum {
    FONTATLAS_ADD_FONT,
    FONTATLAS_ADD_FONT_DEFAULT,
    FONTATLAS_ADD_FONT_FROM_FILE_TTF,
    FONTATLAS_ADD_FONT_FROM_MEMORY_TTF,
    FONTATLAS_ADD_FONT_FROM_MEMORY_COMPRESSED_TTF,
    FONTATLAS_ADD_FONT_FROM_MEMORY_COMPRESSED_BASE85_TTF,
    FONTATLAS_CLEAR_INPUT_DATA,
    FONTATLAS_CLEAR_TEX_DATA,
    FONTATLAS_CLEAR_FONTS,
    FONTATLAS_CLEAR,
    FONTATLAS_BUILD,
    FONTATLAS_GET_TEX_DATA_AS_ALPHA8,
    FONTATLAS_GET_TEX_DATA_ASRGBA32,
    FONTATLAS_GET_GLYPH_RANGES_DEFAULT,
    FONTATLAS_GET_GLYPH_RANGES_KOREAN,
    FONTATLAS_GET_GLYPH_RANGES_JAPANESE,
    FONTATLAS_GET_GLYPH_RANGES_CHINESE_FULL,
    FONTATLAS_GET_GLYPH_RANGES_CHINESE_SIMPLIFIED_COMMON,
    FONTATLAS_GET_GLYPH_RANGES_CYRILLIC,
    FONTATLAS_GET_GLYPH_RANGES_THAI,
    FONTATLAS_GET_GLYPH_RANGES_VIETNAMESE,
    FONTATLAS_ADD_CUSTOM_RECT_REGULAR,
    FONTATLAS_ADD_CUSTOM_RECT_FONT_GLYPH,
    FONTATLAS_CALC_CUSTOM_RECT_UV,
    FONTATLAS_GET_MOUSE_CURSOR_TEX_DATA,
};

static inline ImFontAtlas *js_imfontatlas_data2(JSContext *ctx, JSValueConst value) {
    return static_cast<ImFontAtlas *>(JS_GetOpaque2(ctx, value, js_imfontatlas_class_id));
}

static JSValue js_imfontatlas_wrap(JSContext *ctx, ImFontAtlas *payload) {
    JSValue obj = JS_NewObjectProtoClass(ctx, imfontatlas_proto, js_imfontatlas_class_id);
    JS_SetOpaque(obj, payload);
    return obj;
}

static JSValue js_imfontatlas_constructor(JSContext *ctx, JSValueConst new_target, int argc,
                                          JSValueConst argv[]) {
    JSValue proto, obj = JS_UNDEFINED;
    ImFontAtlas *payload = new ImFontAtlas();
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_imfontatlas_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;
    JS_SetOpaque(obj, payload);
    return obj;
fail:
    js_free(ctx, payload);
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static void js_imfontatlas_finalizer(JSRuntime *rt, JSValue val) {
    ImFontAtlas *payload = static_cast<ImFontAtlas *>(JS_GetOpaque(val, js_imfontatlas_class_id));
    if (payload) { delete payload; }
    JS_FreeValueRT(rt, val);
}

static JSValue js_imfontatlas_functions(JSContext *ctx, JSValueConst this_val, int argc,
                                        JSValueConst argv[], int magic) {
    ImFontAtlas *payload;
    JSValue ret = JS_UNDEFINED;

    if (!(payload = js_imfontatlas_data2(ctx, this_val))) return ret;

    switch (magic) {}

    return ret;
}

static JSClassDef js_imfontatlas_class = {
        .class_name = "ImFontAtlas",
        .finalizer = js_imfontatlas_finalizer,
};

static const JSCFunctionListEntry js_imfontatlas_funcs[] = {
        JS_CFUNC_MAGIC_DEF("AddFont", 0, js_imfont_functions, FONTATLAS_ADD_FONT),
        JS_CFUNC_MAGIC_DEF("AddFontDefault", 0, js_imfont_functions, FONTATLAS_ADD_FONT_DEFAULT),
        JS_CFUNC_MAGIC_DEF("AddFontFromFileTTF", 0, js_imfont_functions,
                           FONTATLAS_ADD_FONT_FROM_FILE_TTF),
        JS_CFUNC_MAGIC_DEF("AddFontFromMemoryTTF", 0, js_imfont_functions,
                           FONTATLAS_ADD_FONT_FROM_MEMORY_TTF),
        JS_CFUNC_MAGIC_DEF("AddFontFromMemoryCompressedTTF", 0, js_imfont_functions,
                           FONTATLAS_ADD_FONT_FROM_MEMORY_COMPRESSED_TTF),
        JS_CFUNC_MAGIC_DEF("AddFontFromMemoryCompressedBase85TTF", 0, js_imfont_functions,
                           FONTATLAS_ADD_FONT_FROM_MEMORY_COMPRESSED_BASE85_TTF),
        JS_CFUNC_MAGIC_DEF("ClearInputData", 0, js_imfont_functions, FONTATLAS_CLEAR_INPUT_DATA),
        JS_CFUNC_MAGIC_DEF("ClearTexData", 0, js_imfont_functions, FONTATLAS_CLEAR_TEX_DATA),
        JS_CFUNC_MAGIC_DEF("ClearFonts", 0, js_imfont_functions, FONTATLAS_CLEAR_FONTS),
        JS_CFUNC_MAGIC_DEF("Clear", 0, js_imfont_functions, FONTATLAS_CLEAR),
        JS_CFUNC_MAGIC_DEF("Build", 0, js_imfont_functions, FONTATLAS_BUILD),
        JS_CFUNC_MAGIC_DEF("GetTexDataAsAlpha8", 0, js_imfont_functions,
                           FONTATLAS_GET_TEX_DATA_AS_ALPHA8),
        JS_CFUNC_MAGIC_DEF("GetTexDataAsRGBA32", 0, js_imfont_functions,
                           FONTATLAS_GET_TEX_DATA_ASRGBA32),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesDefault", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_DEFAULT),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesKorean", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_KOREAN),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesJapanese", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_JAPANESE),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesChineseFull", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_CHINESE_FULL),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesChineseSimplifiedCommon", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_CHINESE_SIMPLIFIED_COMMON),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesCyrillic", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_CYRILLIC),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesThai", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_THAI),
        JS_CFUNC_MAGIC_DEF("GetGlyphRangesVietnamese", 0, js_imfont_functions,
                           FONTATLAS_GET_GLYPH_RANGES_VIETNAMESE),
        JS_CFUNC_MAGIC_DEF("AddCustomRectRegular", 0, js_imfont_functions,
                           FONTATLAS_ADD_CUSTOM_RECT_REGULAR),
        JS_CFUNC_MAGIC_DEF("AddCustomRectFontGlyph", 0, js_imfont_functions,
                           FONTATLAS_ADD_CUSTOM_RECT_FONT_GLYPH),
        JS_CFUNC_MAGIC_DEF("CalcCustomRectUV", 0, js_imfont_functions,
                           FONTATLAS_CALC_CUSTOM_RECT_UV),
        JS_CFUNC_MAGIC_DEF("GetMouseCursorTexData", 0, js_imfont_functions,
                           FONTATLAS_GET_MOUSE_CURSOR_TEX_DATA),
};

#endif

enum {
    IMGUI_GET_IO,
    IMGUI_GET_STYLE,
    IMGUI_SHOW_DEMO_WINDOW,
    IMGUI_SHOW_ABOUT_WINDOW,
    IMGUI_SHOW_METRICS_WINDOW,
    IMGUI_SHOW_STYLE_EDITOR,
    IMGUI_SHOW_STYLE_SELECTOR,
    IMGUI_SHOW_FONT_SELECTOR,
    IMGUI_SHOW_USER_GUIDE,
    IMGUI_GET_VERSION,
    IMGUI_STYLE_COLORS_DARK,
    IMGUI_STYLE_COLORS_CLASSIC,
    IMGUI_STYLE_COLORS_LIGHT,
    IMGUI_BEGIN,
    IMGUI_END,
    IMGUI_BEGIN_CHILD,
    IMGUI_END_CHILD,
    IMGUI_IS_WINDOW_APPEARING,
    IMGUI_IS_WINDOW_COLLAPSED,
    IMGUI_IS_WINDOW_FOCUSED,
    IMGUI_IS_WINDOW_HOVERED,
    IMGUI_GET_WINDOW_DRAW_LIST,
    IMGUI_GET_WINDOW_POS,
    IMGUI_GET_WINDOW_SIZE,
    IMGUI_GET_WINDOW_WIDTH,
    IMGUI_GET_WINDOW_HEIGHT,
    IMGUI_GET_CONTENT_REGION_MAX,
    IMGUI_GET_CONTENT_REGION_AVAIL,
    IMGUI_GET_WINDOW_CONTENT_REGION_MIN,
    IMGUI_GET_WINDOW_CONTENT_REGION_MAX,
    IMGUI_SET_NEXT_WINDOW_POS,
    IMGUI_SET_NEXT_WINDOW_SIZE,
    IMGUI_SET_NEXT_WINDOW_SIZE_CONSTRAINTS,
    IMGUI_SET_NEXT_WINDOW_CONTENT_SIZE,
    IMGUI_SET_NEXT_WINDOW_COLLAPSED,
    IMGUI_SET_NEXT_WINDOW_FOCUS,
    IMGUI_SET_NEXT_WINDOW_BG_ALPHA,
    IMGUI_SET_WINDOW_POS,
    IMGUI_SET_WINDOW_SIZE,
    IMGUI_SET_WINDOW_COLLAPSED,
    IMGUI_SET_WINDOW_FOCUS,
    IMGUI_SET_WINDOW_FONT_SCALE,
    IMGUI_GET_SCROLL_X,
    IMGUI_GET_SCROLL_Y,
    IMGUI_GET_SCROLL_MAX_X,
    IMGUI_GET_SCROLL_MAX_Y,
    IMGUI_SET_SCROLL_X,
    IMGUI_SET_SCROLL_Y,
    IMGUI_SET_SCROLL_HERE_X,
    IMGUI_SET_SCROLL_HERE_Y,
    IMGUI_SET_SCROLL_FROM_POS_X,
    IMGUI_SET_SCROLL_FROM_POS_Y,
    IMGUI_PUSH_FONT,
    IMGUI_POP_FONT,
    IMGUI_PUSH_STYLE_COLOR,
    IMGUI_POP_STYLE_COLOR,
    IMGUI_PUSH_STYLE_VAR,
    IMGUI_POP_STYLE_VAR,
    IMGUI_GET_STYLE_COLOR_VEC4,
    IMGUI_GET_FONT,
    IMGUI_GET_FONT_SIZE,
    IMGUI_GET_FONT_TEX_UV_WHITE_PIXEL,
    IMGUI_GET_COLORU32,
    IMGUI_PUSH_ITEM_WIDTH,
    IMGUI_POP_ITEM_WIDTH,
    IMGUI_CALC_ITEM_WIDTH,
    IMGUI_PUSH_TEXT_WRAP_POS,
    IMGUI_POP_TEXT_WRAP_POS,
    IMGUI_PUSH_ALLOW_KEYBOARD_FOCUS,
    IMGUI_POP_ALLOW_KEYBOARD_FOCUS,
    IMGUI_PUSH_BUTTON_REPEAT,
    IMGUI_POP_BUTTON_REPEAT,
    IMGUI_SEPARATOR,
    IMGUI_SAME_LINE,
    IMGUI_NEW_LINE,
    IMGUI_SPACING,
    IMGUI_DUMMY,
    IMGUI_INDENT,
    IMGUI_UNINDENT,
    IMGUI_BEGIN_GROUP,
    IMGUI_END_GROUP,
    IMGUI_GET_CURSOR_POS,
    IMGUI_GET_CURSOR_POS_X,
    IMGUI_GET_CURSOR_POS_Y,
    IMGUI_SET_CURSOR_POS,
    IMGUI_SET_CURSOR_POS_X,
    IMGUI_SET_CURSOR_POS_Y,
    IMGUI_GET_CURSOR_START_POS,
    IMGUI_GET_CURSOR_SCREEN_POS,
    IMGUI_SET_CURSOR_SCREEN_POS,
    IMGUI_ALIGN_TEXT_TO_FRAME_PADDING,
    IMGUI_GET_TEXT_LINE_HEIGHT,
    IMGUI_GET_TEXT_LINE_HEIGHT_WITH_SPACING,
    IMGUI_GET_FRAME_HEIGHT,
    IMGUI_GET_FRAME_HEIGHT_WITH_SPACING,
    IMGUI_PUSH_ID,
    IMGUI_POP_ID,
    IMGUI_GET_ID,
    IMGUI_TEXT_UNFORMATTED,
    IMGUI_TEXT,
    IMGUI_TEXT_COLORED,
    IMGUI_TEXT_DISABLED,
    IMGUI_TEXT_WRAPPED,
    IMGUI_LABEL_TEXT,
    IMGUI_BULLET_TEXT,
    IMGUI_BUTTON,
    IMGUI_SMALL_BUTTON,
    IMGUI_INVISIBLE_BUTTON,
    IMGUI_ARROW_BUTTON,
    IMGUI_IMAGE,
    IMGUI_IMAGE_BUTTON,
    IMGUI_CHECKBOX,
    IMGUI_CHECKBOX_FLAGS,
    IMGUI_RADIO_BUTTON,
    IMGUI_PROGRESS_BAR,
    IMGUI_BULLET,
    IMGUI_BEGIN_COMBO,
    IMGUI_END_COMBO,
    IMGUI_COMBO,
    IMGUI_DRAG_FLOAT,
    IMGUI_DRAG_FLOAT2,
    IMGUI_DRAG_FLOAT3,
    IMGUI_DRAG_FLOAT4,
    IMGUI_DRAG_FLOAT_RANGE2,
    IMGUI_DRAG_INT,
    IMGUI_DRAG_INT2,
    IMGUI_DRAG_INT3,
    IMGUI_DRAG_INT4,
    IMGUI_DRAG_INT_RANGE2,
    IMGUI_DRAG_SCALAR,
    IMGUI_DRAG_SCALAR_N,
    IMGUI_SLIDER_FLOAT,
    IMGUI_SLIDER_FLOAT2,
    IMGUI_SLIDER_FLOAT3,
    IMGUI_SLIDER_FLOAT4,
    IMGUI_SLIDER_ANGLE,
    IMGUI_SLIDER_INT,
    IMGUI_SLIDER_INT2,
    IMGUI_SLIDER_INT3,
    IMGUI_SLIDER_INT4,
    IMGUI_SLIDER_SCALAR,
    IMGUI_SLIDER_SCALAR_N,
    IMGUI_V_SLIDER_FLOAT,
    IMGUI_V_SLIDER_INT,
    IMGUI_V_SLIDER_SCALAR,
    IMGUI_INPUT_TEXT,
    IMGUI_INPUT_TEXT_MULTILINE,
    IMGUI_INPUT_TEXT_WITH_HINT,
    IMGUI_INPUT_FLOAT,
    IMGUI_INPUT_FLOAT2,
    IMGUI_INPUT_FLOAT3,
    IMGUI_INPUT_FLOAT4,
    IMGUI_INPUT_INT,
    IMGUI_INPUT_INT2,
    IMGUI_INPUT_INT3,
    IMGUI_INPUT_INT4,
    IMGUI_INPUT_DOUBLE,
    IMGUI_INPUT_SCALAR,
    IMGUI_INPUT_SCALAR_N,
    IMGUI_COLOR_EDIT3,
    IMGUI_COLOR_EDIT4,
    IMGUI_COLOR_PICKER3,
    IMGUI_COLOR_PICKER4,
    IMGUI_COLOR_BUTTON,
    IMGUI_SET_COLOR_EDIT_OPTIONS,
    IMGUI_TREE_NODE,
    IMGUI_TREE_NODE_EX,
    IMGUI_TREE_PUSH,
    IMGUI_TREE_POP,
    IMGUI_GET_TREE_NODE_TO_LABEL_SPACING,
    IMGUI_COLLAPSING_HEADER,
    IMGUI_SET_NEXT_ITEM_OPEN,
    IMGUI_SELECTABLE,
    IMGUI_BEGIN_LIST_BOX,
    IMGUI_LIST_BOX,
    IMGUI_LIST_BOX_HEADER,
    IMGUI_LIST_BOX_FOOTER,
    IMGUI_END_LIST_BOX,
    IMGUI_PLOT_LINES,
    IMGUI_PLOT_HISTOGRAM,
    IMGUI_VALUE,
    IMGUI_BEGIN_MAIN_MENU_BAR,
    IMGUI_END_MAIN_MENU_BAR,
    IMGUI_BEGIN_MENU_BAR,
    IMGUI_END_MENU_BAR,
    IMGUI_BEGIN_MENU,
    IMGUI_END_MENU,
    IMGUI_MENU_ITEM,
    IMGUI_BEGIN_TOOLTIP,
    IMGUI_END_TOOLTIP,
    IMGUI_SET_TOOLTIP,
    IMGUI_OPEN_POPUP,
    IMGUI_BEGIN_POPUP,
    IMGUI_BEGIN_POPUP_CONTEXT_ITEM,
    IMGUI_BEGIN_POPUP_CONTEXT_WINDOW,
    IMGUI_BEGIN_POPUP_CONTEXT_VOID,
    IMGUI_BEGIN_POPUP_MODAL,
    IMGUI_END_POPUP,
    IMGUI_OPEN_POPUP_ON_ITEM_CLICK,
    IMGUI_IS_POPUP_OPEN,
    IMGUI_BEGIN_TABLE,
    IMGUI_END_TABLE,
    IMGUI_TABLE_NEXT_ROW,
    IMGUI_TABLE_NEXT_COLUMN,
    IMGUI_TABLE_SET_COLUMN_INDEX,
    IMGUI_TABLE_SETUP_COLUMN,
    IMGUI_TABLE_SETUP_SCROLL_FREEZE,
    IMGUI_TABLE_HEADERS_ROW,
    IMGUI_TABLE_HEADER,
    IMGUI_TABLE_GET_SORT_SPECS,
    IMGUI_TABLE_GET_COLUMN_COUNT,
    IMGUI_TABLE_GET_COLUMN_INDEX,
    IMGUI_TABLE_GET_ROW_INDEX,
    IMGUI_TABLE_GET_COLUMN_NAME,
    IMGUI_TABLE_GET_COLUMN_FLAGS,
    IMGUI_TABLE_SET_COLUMN_ENABLED,
    IMGUI_TABLE_SET_BG_COLOR,
    IMGUI_BUILD_LOOKUP_TABLE,
    IMGUI_CLOSE_CURRENT_POPUP,
    IMGUI_COLUMNS,
    IMGUI_NEXT_COLUMN,
    IMGUI_GET_COLUMN_INDEX,
    IMGUI_GET_COLUMN_WIDTH,
    IMGUI_SET_COLUMN_WIDTH,
    IMGUI_GET_COLUMN_OFFSET,
    IMGUI_SET_COLUMN_OFFSET,
    IMGUI_GET_COLUMNS_COUNT,
    IMGUI_BEGIN_TAB_BAR,
    IMGUI_END_TAB_BAR,
    IMGUI_BEGIN_TAB_ITEM,
    IMGUI_END_TAB_ITEM,
    IMGUI_TAB_ITEM_BUTTON,
    IMGUI_SET_TAB_ITEM_CLOSED,
    IMGUI_LOG_TO_TTY,
    IMGUI_LOG_TO_FILE,
    IMGUI_LOG_TO_CLIPBOARD,
    IMGUI_LOG_FINISH,
    IMGUI_LOG_BUTTONS,
    IMGUI_LOG_TEXT,
    IMGUI_BEGIN_DRAG_DROP_SOURCE,
    IMGUI_SET_DRAG_DROP_PAYLOAD,
    IMGUI_END_DRAG_DROP_SOURCE,
    IMGUI_BEGIN_DRAG_DROP_TARGET,
    IMGUI_ACCEPT_DRAG_DROP_PAYLOAD,
    IMGUI_END_DRAG_DROP_TARGET,
    IMGUI_GET_DRAG_DROP_PAYLOAD,
    IMGUI_BEGIN_DISABLED,
    IMGUI_END_DISABLED,
    IMGUI_PUSH_CLIP_RECT,
    IMGUI_POP_CLIP_RECT,
    IMGUI_SET_ITEM_DEFAULT_FOCUS,
    IMGUI_SET_KEYBOARD_FOCUS_HERE,
    IMGUI_IS_ITEM_HOVERED,
    IMGUI_IS_ITEM_ACTIVE,
    IMGUI_IS_ITEM_FOCUSED,
    IMGUI_IS_ITEM_CLICKED,
    IMGUI_IS_ITEM_VISIBLE,
    IMGUI_IS_ITEM_EDITED,
    IMGUI_IS_ITEM_ACTIVATED,
    IMGUI_IS_ITEM_DEACTIVATED,
    IMGUI_IS_ITEM_DEACTIVATED_AFTER_EDIT,
    IMGUI_IS_ANY_ITEM_HOVERED,
    IMGUI_IS_ANY_ITEM_ACTIVE,
    IMGUI_IS_ANY_ITEM_FOCUSED,
    IMGUI_GET_ITEM_RECT_MIN,
    IMGUI_GET_ITEM_RECT_MAX,
    IMGUI_GET_ITEM_RECT_SIZE,
    IMGUI_SET_ITEM_ALLOW_OVERLAP,
    IMGUI_IS_RECT_VISIBLE,
    IMGUI_GET_TIME,
    IMGUI_GET_FRAME_COUNT,
    IMGUI_GET_BACKGROUND_DRAW_LIST,
    IMGUI_GET_FOREGROUND_DRAW_LIST,
    IMGUI_GET_DRAW_LIST_SHARED_DATA,
    IMGUI_GET_STYLE_COLOR_NAME,
    IMGUI_SET_STATE_STORAGE,
    IMGUI_GET_STATE_STORAGE,
    IMGUI_CALC_TEXT_SIZE,
    IMGUI_CALC_LIST_CLIPPING,
    IMGUI_BEGIN_CHILD_FRAME,
    IMGUI_END_CHILD_FRAME,
    IMGUI_COLOR_CONVERTU32_TO_FLOAT4,
    IMGUI_COLOR_CONVERT_FLOAT4_TOU32,
    IMGUI_COLOR_CONVERT_RGB_TO_HSV,
    IMGUI_COLOR_CONVERT_HSV_TO_RGB,
    IMGUI_GET_KEY_INDEX,
    IMGUI_IS_KEY_DOWN,
    IMGUI_IS_KEY_PRESSED,
    IMGUI_IS_KEY_RELEASED,
    IMGUI_GET_KEY_PRESSED_AMOUNT,
    IMGUI_IS_MOUSE_DOWN,
    IMGUI_IS_ANY_MOUSE_DOWN,
    IMGUI_IS_MOUSE_CLICKED,
    IMGUI_IS_MOUSE_DOUBLE_CLICKED,
    IMGUI_IS_MOUSE_RELEASED,
    IMGUI_IS_MOUSE_DRAGGING,
    IMGUI_IS_MOUSE_HOVERING_RECT,
    IMGUI_IS_MOUSE_POS_VALID,
    IMGUI_GET_MOUSE_POS,
    IMGUI_GET_MOUSE_POS_ON_OPENING_CURRENT_POPUP,
    IMGUI_GET_MOUSE_DRAG_DELTA,
    IMGUI_RESET_MOUSE_DRAG_DELTA,
    IMGUI_GET_MOUSE_CURSOR,
    IMGUI_SET_MOUSE_CURSOR,
    IMGUI_CAPTURE_KEYBOARD_FROM_APP,
    IMGUI_CAPTURE_MOUSE_FROM_APP,
    IMGUI_GET_CLIPBOARD_TEXT,
    IMGUI_SET_CLIPBOARD_TEXT,
    IMGUI_LOAD_INI_SETTINGS_FROM_DISK,
    IMGUI_LOAD_INI_SETTINGS_FROM_MEMORY,
    IMGUI_SAVE_INI_SETTINGS_TO_DISK,
    IMGUI_SAVE_INI_SETTINGS_TO_MEMORY,
    IMGUI_SET_ALLOCATOR_FUNCTIONS,
    IMGUI_MEM_ALLOC,
    IMGUI_MEM_FREE,
    IMGUI_POINTER,
};

static void js_imgui_free_func(JSRuntime *rt, void *opaque, void *ptr) { ImGui::MemFree(ptr); }

static ImVec2 js_imgui_getimvec2(JSContext *ctx, JSValueConst value) {
    JSValue xval = JS_UNDEFINED, yval = JS_UNDEFINED;
    double x, y;
    if (JS_IsArray(ctx, value)) {
        xval = JS_GetPropertyUint32(ctx, value, 0);
        yval = JS_GetPropertyUint32(ctx, value, 1);
    } else if (JS_IsObject(value)) {
        xval = JS_GetPropertyStr(ctx, value, "x");
        yval = JS_GetPropertyStr(ctx, value, "y");
    }
    JS_ToFloat64(ctx, &x, xval);
    JS_ToFloat64(ctx, &y, yval);
    return ImVec2(x, y);
}

static ImVec4 js_imgui_getimvec4(JSContext *ctx, JSValueConst value) {
    JSValue xval = JS_UNDEFINED, yval = JS_UNDEFINED, zval = JS_UNDEFINED, wval = JS_UNDEFINED;
    double x, y, z, w;
    if (JS_IsArray(ctx, value)) {
        xval = JS_GetPropertyUint32(ctx, value, 0);
        yval = JS_GetPropertyUint32(ctx, value, 1);
        zval = JS_GetPropertyUint32(ctx, value, 2);
        wval = JS_GetPropertyUint32(ctx, value, 3);
    } else if (JS_IsObject(value)) {
        xval = JS_GetPropertyStr(ctx, value, "x");
        yval = JS_GetPropertyStr(ctx, value, "y");
        zval = JS_GetPropertyStr(ctx, value, "z");
        wval = JS_GetPropertyStr(ctx, value, "w");
    }
    JS_ToFloat64(ctx, &x, xval);
    JS_ToFloat64(ctx, &y, yval);
    JS_ToFloat64(ctx, &x, zval);
    JS_ToFloat64(ctx, &y, wval);
    return ImVec4(x, y, z, w);
}

static ImVec4 js_imgui_getcolor(JSContext *ctx, JSValueConst value) {
    ImVec4 vec = {0, 0, 0, 0};
    if (JS_IsObject(value)) {
        vec = js_imgui_getimvec4(ctx, value);
    } else if (JS_IsNumber(value)) {
        uint32_t color = 0;
        JS_ToUint32(ctx, &color, value);
        vec = ImGui::ColorConvertU32ToFloat4(color);
    } else if (JS_IsString(value)) {
        const char *p, *str = JS_ToCString(ctx, value);
        uint32_t color;
        for (p = str; *p; p++)
            if (isxdigit(*p)) break;
        color = strtoul(p, 0, 16);
        vec = ImGui::ColorConvertU32ToFloat4(color);
        JS_FreeCString(ctx, str);
    }
    return vec;
}

static JSValue js_imgui_newptr(JSContext *ctx, void *ptr) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%p", ptr);
    return JS_NewString(ctx, buf);
}

template<class T>
static T *js_imgui_getptr(JSContext *ctx, JSValueConst value) {
    const char *str = JS_ToCString(ctx, value);
    void *ptr = 0;
    sscanf(str, "%p", &ptr);
    JS_FreeCString(ctx, str);
    return static_cast<T *>(ptr);
}

static ImTextureID js_imgui_gettexture(JSContext *ctx, JSValueConst value) {
    if (JS_IsNumber(value)) {
        uint64_t id;
        JS_ToIndex(ctx, &id, value);
        return ImTextureID(id);
    }

    return js_imgui_getptr<void>(ctx, value);
}

static JSValue js_imgui_newimvec2(JSContext *ctx, const ImVec2 &vec) {
    JSValue ret = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, ret, 0, JS_NewFloat64(ctx, vec.x));
    JS_SetPropertyUint32(ctx, ret, 1, JS_NewFloat64(ctx, vec.y));
    return ret;
}

static JSValue js_imgui_newimvec4(JSContext *ctx, const ImVec4 &vec) {
    JSValue ret = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, ret, 0, JS_NewFloat64(ctx, vec.x));
    JS_SetPropertyUint32(ctx, ret, 1, JS_NewFloat64(ctx, vec.y));
    JS_SetPropertyUint32(ctx, ret, 2, JS_NewFloat64(ctx, vec.z));
    JS_SetPropertyUint32(ctx, ret, 3, JS_NewFloat64(ctx, vec.w));
    return ret;
}

static int js_imgui_formatcount(JSContext *ctx, int argc, JSValueConst argv[]) {
    const char *p, *end, *fmt;
    int i = 0;
    fmt = JS_ToCString(ctx, argv[i++]);
    end = fmt + strlen(fmt);
    for (p = fmt; p < end; p++) {
        if (*p == '%' && p[1] != '%') {
            ++p;
            while (isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == '*') ++p;
            ++i;
        }
    }
    JS_FreeCString(ctx, fmt);
    return i;
}

static void js_imgui_formatargs(JSContext *ctx, int argc, JSValueConst argv[], void *output[]) {
    const char *p, *end, *fmt;
    int i = 0;

    fmt = JS_ToCString(ctx, argv[i]);
    end = fmt + strlen(fmt);

    output[i++] = (void *) fmt;

    for (p = fmt; p < end; p++) {
        if (*p == '%' && p[1] != '%') {
            ++p;

            while (isdigit(*p) || *p == '.' || *p == '-' || *p == '+' || *p == '*') ++p;

            switch (*p) {
                case 'd':
                case 'i': {
                    int64_t i = 0;
                    JS_ToInt64(ctx, &i, argv[i]);
                    output[i] = (void *) (intptr_t) i;
                    break;
                }
                case 'o':
                case 'x':
                case 'u': {
                    uint32_t i = 0;
                    JS_ToUint32(ctx, &i, argv[i]);
                    output[i] = (void *) (uintptr_t) i;
                    break;
                }
                case 's': {
                    const char *str = JS_ToCString(ctx, argv[i]);
                    output[i] = (void *) str;
                    break;
                }
            }
            ++i;
        }
    }
    // JS_FreeCString(ctx, fmt);
}

#ifndef _METADOT_IMGUI_JSBIND_STYLE
#define _METADOT_IMGUI_JSBIND_STYLE

thread_local JSClassID js_imgui_style_class_id = 0;
thread_local JSValue imgui_style_proto = {JS_TAG_UNDEFINED}, imgui_style_ctor = {JS_TAG_UNDEFINED};

enum {
    STYLE_SCALE_ALL_SIZES,
    STYLE_ALPHA,
    STYLE_DISABLED_ALPHA,
    STYLE_WINDOW_PADDING,
    STYLE_WINDOW_ROUNDING,
    STYLE_WINDOW_BORDER_SIZE,
    STYLE_WINDOW_MIN_SIZE,
    STYLE_WINDOW_TITLE_ALIGN,
    STYLE_WINDOW_MENU_BUTTON_POSITION,
    STYLE_CHILD_ROUNDING,
    STYLE_CHILD_BORDER_SIZE,
    STYLE_POPUP_ROUNDING,
    STYLE_POPUP_BORDER_SIZE,
    STYLE_FRAME_PADDING,
    STYLE_FRAME_ROUNDING,
    STYLE_FRAME_BORDER_SIZE,
    STYLE_ITEM_SPACING,
    STYLE_ITEM_INNER_SPACING,
    STYLE_CELL_PADDING,
    STYLE_TOUCH_EXTRA_PADDING,
    STYLE_INDENT_SPACING,
    STYLE_COLUMNS_MIN_SPACING,
    STYLE_SCROLLBAR_SIZE,
    STYLE_SCROLLBAR_ROUNDING,
    STYLE_GRAB_MIN_SIZE,
    STYLE_GRAB_ROUNDING,
    STYLE_LOG_SLIDER_DEADZONE,
    STYLE_TAB_ROUNDING,
    STYLE_TAB_BORDER_SIZE,
    STYLE_TAB_MIN_WIDTH_FOR_CLOSE_BUTTON,
    STYLE_COLOR_BUTTON_POSITION,
    STYLE_BUTTON_TEXT_ALIGN,
    STYLE_SELECTABLE_TEXT_ALIGN,
    STYLE_DISPLAY_WINDOW_PADDING,
    STYLE_DISPLAY_SAFE_AREA_PADDING,
    STYLE_MOUSE_CURSOR_SCALE,
    STYLE_ANTI_ALIASED_LINES,
    STYLE_ANTI_ALIASED_LINES_USE_TEX,
    STYLE_ANTI_ALIASED_FILL,
    STYLE_CURVE_TESSELLATION_TOL,
    STYLE_CIRCLE_TESSELLATION_MAX_ERROR,
    STYLE_COLORS,
};

static inline ImGuiStyle *js_imgui_style_data2(JSContext *ctx, JSValueConst value) {
    return static_cast<ImGuiStyle *>(JS_GetOpaque2(ctx, value, js_imgui_style_class_id));
}

static JSValue js_imgui_style_wrap(JSContext *ctx, ImGuiStyle *style) {
    JSValue obj = JS_NewObjectProtoClass(ctx, imgui_style_proto, js_imgui_style_class_id);
    JS_SetOpaque(obj, style);
    return obj;
}

static JSValue js_imgui_style_constructor(JSContext *ctx, JSValueConst new_target, int argc,
                                          JSValueConst argv[]) {
    JSValue proto, obj = JS_UNDEFINED;
    ImGuiStyle *style = new ImGuiStyle();

    /* using new_target to get the prototype is necessary when the class is extended. */
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_imgui_style_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;

    JS_SetOpaque(obj, style);

    return obj;
fail:
    js_free(ctx, style);
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static void js_imgui_style_finalizer(JSRuntime *rt, JSValue val) {
    ImGuiStyle *style = static_cast<ImGuiStyle *>(JS_GetOpaque(val, js_imgui_style_class_id));
    if (style) { delete style; }
    JS_FreeValueRT(rt, val);
}

static JSValue js_imgui_style_get(JSContext *ctx, JSValueConst this_val, int magic) {
    ImGuiStyle *style;
    JSValue ret = JS_UNDEFINED;

    if (!(style = js_imgui_style_data2(ctx, this_val))) return ret;

    switch (magic) {
        case STYLE_ALPHA: {
            ret = JS_NewFloat64(ctx, style->Alpha);
            break;
        }
        case STYLE_DISABLED_ALPHA: {
            ret = JS_NewFloat64(ctx, style->DisabledAlpha);
            break;
        }
        case STYLE_WINDOW_PADDING: {
            ret = js_imgui_newimvec2(ctx, style->WindowPadding);
            break;
        }
        case STYLE_WINDOW_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->WindowRounding);
            break;
        }
        case STYLE_WINDOW_BORDER_SIZE: {
            ret = JS_NewFloat64(ctx, style->WindowBorderSize);
            break;
        }
        case STYLE_WINDOW_MIN_SIZE: {
            ret = js_imgui_newimvec2(ctx, style->WindowMinSize);
            break;
        }
        case STYLE_WINDOW_TITLE_ALIGN: {
            ret = js_imgui_newimvec2(ctx, style->WindowTitleAlign);
            break;
        }
        case STYLE_WINDOW_MENU_BUTTON_POSITION: {
            ret = JS_NewInt32(ctx, style->WindowMenuButtonPosition);
            break;
        }
        case STYLE_CHILD_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->ChildRounding);
            break;
        }
        case STYLE_CHILD_BORDER_SIZE: {
            ret = JS_NewFloat64(ctx, style->ChildBorderSize);
            break;
        }
        case STYLE_POPUP_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->PopupRounding);
            break;
        }
        case STYLE_POPUP_BORDER_SIZE: {
            ret = JS_NewFloat64(ctx, style->PopupBorderSize);
            break;
        }
        case STYLE_FRAME_PADDING: {
            ret = js_imgui_newimvec2(ctx, style->FramePadding);
            break;
        }
        case STYLE_FRAME_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->FrameRounding);
            break;
        }
        case STYLE_FRAME_BORDER_SIZE: {
            ret = JS_NewFloat64(ctx, style->FrameBorderSize);
            break;
        }
        case STYLE_ITEM_SPACING: {
            ret = js_imgui_newimvec2(ctx, style->ItemSpacing);
            break;
        }
        case STYLE_ITEM_INNER_SPACING: {
            ret = js_imgui_newimvec2(ctx, style->ItemInnerSpacing);
            break;
        }
        case STYLE_CELL_PADDING: {
            ret = js_imgui_newimvec2(ctx, style->CellPadding);
            break;
        }
        case STYLE_TOUCH_EXTRA_PADDING: {
            ret = js_imgui_newimvec2(ctx, style->TouchExtraPadding);
            break;
        }
        case STYLE_INDENT_SPACING: {
            ret = JS_NewFloat64(ctx, style->IndentSpacing);
            break;
        }
        case STYLE_COLUMNS_MIN_SPACING: {
            ret = JS_NewFloat64(ctx, style->ColumnsMinSpacing);
            break;
        }
        case STYLE_SCROLLBAR_SIZE: {
            ret = JS_NewFloat64(ctx, style->ScrollbarSize);
            break;
        }
        case STYLE_SCROLLBAR_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->ScrollbarRounding);
            break;
        }
        case STYLE_GRAB_MIN_SIZE: {
            ret = JS_NewFloat64(ctx, style->GrabMinSize);
            break;
        }
        case STYLE_GRAB_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->GrabRounding);
            break;
        }
        case STYLE_LOG_SLIDER_DEADZONE: {
            ret = JS_NewFloat64(ctx, style->LogSliderDeadzone);
            break;
        }
        case STYLE_TAB_ROUNDING: {
            ret = JS_NewFloat64(ctx, style->TabRounding);
            break;
        }
        case STYLE_TAB_BORDER_SIZE: {
            ret = JS_NewFloat64(ctx, style->TabBorderSize);
            break;
        }
        case STYLE_TAB_MIN_WIDTH_FOR_CLOSE_BUTTON: {
            ret = JS_NewFloat64(ctx, style->TabMinWidthForCloseButton);
            break;
        }
        case STYLE_COLOR_BUTTON_POSITION: {
            ret = JS_NewInt32(ctx, style->ColorButtonPosition);
            break;
        }
        case STYLE_BUTTON_TEXT_ALIGN: {
            ret = js_imgui_newimvec2(ctx, style->ButtonTextAlign);
            break;
        }
        case STYLE_SELECTABLE_TEXT_ALIGN: {
            ret = js_imgui_newimvec2(ctx, style->SelectableTextAlign);
            break;
        }
        case STYLE_DISPLAY_WINDOW_PADDING: {
            ret = js_imgui_newimvec2(ctx, style->DisplayWindowPadding);
            break;
        }
        case STYLE_DISPLAY_SAFE_AREA_PADDING: {
            ret = js_imgui_newimvec2(ctx, style->DisplaySafeAreaPadding);
            break;
        }
        case STYLE_MOUSE_CURSOR_SCALE: {
            ret = JS_NewFloat64(ctx, style->MouseCursorScale);
            break;
        }
        case STYLE_ANTI_ALIASED_LINES: {
            ret = JS_NewBool(ctx, style->AntiAliasedLines);
            break;
        }
        case STYLE_ANTI_ALIASED_LINES_USE_TEX: {
            ret = JS_NewBool(ctx, style->AntiAliasedLinesUseTex);
            break;
        }
        case STYLE_ANTI_ALIASED_FILL: {
            ret = JS_NewBool(ctx, style->AntiAliasedFill);
            break;
        }
        case STYLE_CURVE_TESSELLATION_TOL: {
            ret = JS_NewFloat64(ctx, style->CurveTessellationTol);
            break;
        }
        case STYLE_CIRCLE_TESSELLATION_MAX_ERROR: {
            ret = JS_NewFloat64(ctx, style->CircleTessellationMaxError);
            break;
        }
        case STYLE_COLORS: {
            uint32_t i;
            ret = JS_NewArray(ctx);
            for (i = 0; i < ImGuiCol_COUNT; i++)
                JS_SetPropertyUint32(ctx, ret, i, js_imgui_newimvec4(ctx, style->Colors[i]));
            break;
        }
    }

    return ret;
}

static JSValue js_imgui_style_set(JSContext *ctx, JSValueConst this_val, JSValueConst value,
                                  int magic) {
    ImGuiStyle *style;
    JSValue ret = JS_UNDEFINED;

    if (!(style = js_imgui_style_data2(ctx, this_val))) return ret;

    switch (magic) {
        case STYLE_ALPHA: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->Alpha = d;
            break;
        }
        case STYLE_DISABLED_ALPHA: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->DisabledAlpha = d;
            break;
        }
        case STYLE_WINDOW_PADDING: {
            style->WindowPadding = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_WINDOW_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->WindowRounding = d;
            break;
        }
        case STYLE_WINDOW_BORDER_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->WindowBorderSize = d;
            break;
        }
        case STYLE_WINDOW_MIN_SIZE: {
            style->WindowMinSize = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_WINDOW_TITLE_ALIGN: {
            style->WindowTitleAlign = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_WINDOW_MENU_BUTTON_POSITION: {
            int32_t i;
            JS_ToInt32(ctx, &i, value);
            style->WindowMenuButtonPosition = i;
            break;
        }
        case STYLE_CHILD_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->ChildRounding = d;
            break;
        }
        case STYLE_CHILD_BORDER_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->ChildBorderSize = d;
            break;
        }
        case STYLE_POPUP_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->PopupRounding = d;
            break;
        }
        case STYLE_POPUP_BORDER_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->PopupBorderSize = d;
            break;
        }
        case STYLE_FRAME_PADDING: {
            style->FramePadding = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_FRAME_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->FrameRounding = d;
            break;
        }
        case STYLE_FRAME_BORDER_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->FrameBorderSize = d;
            break;
        }
        case STYLE_ITEM_SPACING: {
            style->ItemSpacing = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_ITEM_INNER_SPACING: {
            style->ItemInnerSpacing = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_CELL_PADDING: {
            style->CellPadding = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_TOUCH_EXTRA_PADDING: {
            style->TouchExtraPadding = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_INDENT_SPACING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->IndentSpacing = d;
            break;
        }
        case STYLE_COLUMNS_MIN_SPACING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->ColumnsMinSpacing = d;
            break;
        }
        case STYLE_SCROLLBAR_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->ScrollbarSize = d;
            break;
        }
        case STYLE_SCROLLBAR_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->ScrollbarRounding = d;
            break;
        }
        case STYLE_GRAB_MIN_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->GrabMinSize = d;
            break;
        }
        case STYLE_GRAB_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->GrabRounding = d;
            break;
        }
        case STYLE_LOG_SLIDER_DEADZONE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->LogSliderDeadzone = d;
            break;
        }
        case STYLE_TAB_ROUNDING: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->TabRounding = d;
            break;
        }
        case STYLE_TAB_BORDER_SIZE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->TabBorderSize = d;
            break;
        }
        case STYLE_TAB_MIN_WIDTH_FOR_CLOSE_BUTTON: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->TabMinWidthForCloseButton = d;
            break;
        }
        case STYLE_COLOR_BUTTON_POSITION: {
            int32_t i;
            JS_ToInt32(ctx, &i, value);
            style->ColorButtonPosition = i;
            break;
        }
        case STYLE_BUTTON_TEXT_ALIGN: {
            style->ButtonTextAlign = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_SELECTABLE_TEXT_ALIGN: {
            style->SelectableTextAlign = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_DISPLAY_WINDOW_PADDING: {
            style->DisplayWindowPadding = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_DISPLAY_SAFE_AREA_PADDING: {
            style->DisplaySafeAreaPadding = js_imgui_getimvec2(ctx, value);
            break;
        }
        case STYLE_MOUSE_CURSOR_SCALE: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->MouseCursorScale = d;
            break;
        }
        case STYLE_ANTI_ALIASED_LINES: {
            style->AntiAliasedLines = JS_ToBool(ctx, value);
            break;
        }
        case STYLE_ANTI_ALIASED_LINES_USE_TEX: {
            style->AntiAliasedLinesUseTex = JS_ToBool(ctx, value);
            break;
        }
        case STYLE_ANTI_ALIASED_FILL: {
            style->AntiAliasedFill = JS_ToBool(ctx, value);
            break;
        }
        case STYLE_CURVE_TESSELLATION_TOL: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->CurveTessellationTol = d;
            break;
        }
        case STYLE_CIRCLE_TESSELLATION_MAX_ERROR: {
            double d;
            JS_ToFloat64(ctx, &d, value);
            style->CircleTessellationMaxError = d;
            break;
        }
        case STYLE_COLORS: {
            uint32_t i;
            for (i = 0; i < ImGuiCol_COUNT; i++) {
                JSValue color = JS_GetPropertyUint32(ctx, value, i);
                style->Colors[i] = js_imgui_getimvec4(ctx, color);
                JS_FreeValue(ctx, color);
            }
            break;
        }
    }

    return ret;
}

static JSValue js_imgui_style_functions(JSContext *ctx, JSValueConst this_val, int argc,
                                        JSValueConst argv[], int magic) {
    ImGuiStyle *style;
    JSValue ret = JS_UNDEFINED;

    if (!(style = js_imgui_style_data2(ctx, this_val))) return ret;

    switch (magic) {
        case STYLE_SCALE_ALL_SIZES: {
            double factor;
            JS_ToFloat64(ctx, &factor, argv[0]);
            style->ScaleAllSizes(factor);
            break;
        }
    }

    return ret;
}

static JSClassDef js_imgui_style_class = {
        .class_name = "ImGuiStyle",
        .finalizer = js_imgui_style_finalizer,
};

static const JSCFunctionListEntry js_imgui_style_funcs[] = {
        JS_CFUNC_MAGIC_DEF("ScaleAllSizes", 1, js_imgui_style_functions, STYLE_SCALE_ALL_SIZES),
        JS_CGETSET_MAGIC_DEF("Alpha", js_imgui_style_get, js_imgui_style_set, STYLE_ALPHA),
        JS_CGETSET_MAGIC_DEF("WindowPadding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_WINDOW_PADDING),
        JS_CGETSET_MAGIC_DEF("WindowRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_WINDOW_ROUNDING),
        JS_CGETSET_MAGIC_DEF("WindowBorderSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_WINDOW_BORDER_SIZE),
        JS_CGETSET_MAGIC_DEF("WindowMinSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_WINDOW_MIN_SIZE),
        JS_CGETSET_MAGIC_DEF("WindowTitleAlign", js_imgui_style_get, js_imgui_style_set,
                             STYLE_WINDOW_TITLE_ALIGN),
        JS_CGETSET_MAGIC_DEF("ChildRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_CHILD_ROUNDING),
        JS_CGETSET_MAGIC_DEF("ChildBorderSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_CHILD_BORDER_SIZE),
        JS_CGETSET_MAGIC_DEF("PopupRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_POPUP_ROUNDING),
        JS_CGETSET_MAGIC_DEF("PopupBorderSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_POPUP_BORDER_SIZE),
        JS_CGETSET_MAGIC_DEF("FramePadding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_FRAME_PADDING),
        JS_CGETSET_MAGIC_DEF("FrameRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_FRAME_ROUNDING),
        JS_CGETSET_MAGIC_DEF("FrameBorderSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_FRAME_BORDER_SIZE),
        JS_CGETSET_MAGIC_DEF("ItemSpacing", js_imgui_style_get, js_imgui_style_set,
                             STYLE_ITEM_SPACING),
        JS_CGETSET_MAGIC_DEF("ItemInnerSpacing", js_imgui_style_get, js_imgui_style_set,
                             STYLE_ITEM_INNER_SPACING),
        JS_CGETSET_MAGIC_DEF("TouchExtraPadding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_TOUCH_EXTRA_PADDING),
        JS_CGETSET_MAGIC_DEF("IndentSpacing", js_imgui_style_get, js_imgui_style_set,
                             STYLE_INDENT_SPACING),
        JS_CGETSET_MAGIC_DEF("ColumnsMinSpacing", js_imgui_style_get, js_imgui_style_set,
                             STYLE_COLUMNS_MIN_SPACING),
        JS_CGETSET_MAGIC_DEF("ScrollbarSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_SCROLLBAR_SIZE),
        JS_CGETSET_MAGIC_DEF("ScrollbarRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_SCROLLBAR_ROUNDING),
        JS_CGETSET_MAGIC_DEF("GrabMinSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_GRAB_MIN_SIZE),
        JS_CGETSET_MAGIC_DEF("GrabRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_GRAB_ROUNDING),
        JS_CGETSET_MAGIC_DEF("TabRounding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_TAB_ROUNDING),
        JS_CGETSET_MAGIC_DEF("TabBorderSize", js_imgui_style_get, js_imgui_style_set,
                             STYLE_TAB_BORDER_SIZE),
        JS_CGETSET_MAGIC_DEF("ButtonTextAlign", js_imgui_style_get, js_imgui_style_set,
                             STYLE_BUTTON_TEXT_ALIGN),
        JS_CGETSET_MAGIC_DEF("SelectableTextAlign", js_imgui_style_get, js_imgui_style_set,
                             STYLE_SELECTABLE_TEXT_ALIGN),
        JS_CGETSET_MAGIC_DEF("DisplayWindowPadding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_DISPLAY_WINDOW_PADDING),
        JS_CGETSET_MAGIC_DEF("DisplaySafeAreaPadding", js_imgui_style_get, js_imgui_style_set,
                             STYLE_DISPLAY_SAFE_AREA_PADDING),
        JS_CGETSET_MAGIC_DEF("MouseCursorScale", js_imgui_style_get, js_imgui_style_set,
                             STYLE_MOUSE_CURSOR_SCALE),
        JS_CGETSET_MAGIC_DEF("AntiAliasedLines", js_imgui_style_get, js_imgui_style_set,
                             STYLE_ANTI_ALIASED_LINES),
        JS_CGETSET_MAGIC_DEF("AntiAliasedFill", js_imgui_style_get, js_imgui_style_set,
                             STYLE_ANTI_ALIASED_FILL),
        JS_CGETSET_MAGIC_DEF("CurveTessellationTol", js_imgui_style_get, js_imgui_style_set,
                             STYLE_CURVE_TESSELLATION_TOL),
        JS_CGETSET_MAGIC_DEF("Colors", js_imgui_style_get, js_imgui_style_set, STYLE_COLORS),

};

#endif

static JSValue js_imgui_functions(JSContext *ctx, JSValueConst this_val, int argc,
                                  JSValueConst argv[], int magic) {
    JSValue ret = JS_UNDEFINED;

    switch (magic) {
        case IMGUI_GET_IO: {
            ret = js_imgui_io_wrap(ctx, new ImGuiIO(ImGui::GetIO()));
            break;
        }
        case IMGUI_GET_STYLE: {
            ret = js_imgui_style_wrap(ctx, new ImGuiStyle(ImGui::GetStyle()));
            break;
        }
        case IMGUI_SHOW_DEMO_WINDOW:
            break;
        case IMGUI_SHOW_ABOUT_WINDOW:
            break;
        case IMGUI_SHOW_METRICS_WINDOW:
            break;
        case IMGUI_SHOW_STYLE_EDITOR: {
            ImGuiStyle *style = js_imgui_style_data2(ctx, argv[0]);
            ImGui::ShowStyleEditor(style);
            break;
        }
        case IMGUI_SHOW_STYLE_SELECTOR: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ret = JS_NewBool(ctx, ImGui::ShowStyleSelector(label));
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_SHOW_FONT_SELECTOR: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ImGui::ShowFontSelector(label);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_SHOW_USER_GUIDE: {
            ImGui::ShowUserGuide();
            break;
        }
        case IMGUI_GET_VERSION: {
            ret = JS_NewString(ctx, ImGui::GetVersion());
            break;
        }
        case IMGUI_STYLE_COLORS_DARK: {
            ImGui::StyleColorsDark(js_imgui_style_data2(ctx, argv[0]));
            break;
        }
        case IMGUI_STYLE_COLORS_CLASSIC: {
            ImGui::StyleColorsClassic(js_imgui_style_data2(ctx, argv[0]));
            break;
        }
        case IMGUI_STYLE_COLORS_LIGHT: {
            ImGui::StyleColorsLight(js_imgui_style_data2(ctx, argv[0]));
            break;
        }
        case IMGUI_BEGIN: {
            const char *name = JS_ToCString(ctx, argv[0]);
            int32_t window_flags = 0;
            OutputArg<bool> p_open(ctx, argc >= 2 ? argv[1] : JS_NULL);
            if (argc >= 3) JS_ToInt32(ctx, &window_flags, argv[2]);

            ret = JS_NewBool(ctx, ImGui::Begin(name, p_open, (ImGuiWindowFlags) window_flags));
            JS_FreeCString(ctx, name);
            break;
        }
        case IMGUI_END: {
            ImGui::End();
            break;
        }
        case IMGUI_BEGIN_CHILD: {
            const char *str_id = JS_IsString(argv[0]) ? JS_ToCString(ctx, argv[0]) : 0;
            int32_t id = -1;
            ImVec2 size(0, 0);
            bool border = false;
            int32_t flags = 0;
            if (argc >= 1) JS_ToInt32(ctx, &id, argv[0]);
            if (argc >= 2) size = js_imgui_getimvec2(ctx, argv[1]);
            if (argc >= 3) border = JS_ToBool(ctx, argv[2]);
            if (argc >= 4) JS_ToInt32(ctx, &flags, argv[3]);
            ret = JS_NewBool(ctx, str_id ? ImGui::BeginChild(str_id, size, border, flags)
                                         : ImGui::BeginChild(id, size, border, flags));
            if (str_id) JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_END_CHILD: {
            ImGui::EndChild();
            break;
        }
        case IMGUI_IS_WINDOW_APPEARING: {
            ret = JS_NewBool(ctx, ImGui::IsWindowAppearing());
            break;
        }
        case IMGUI_IS_WINDOW_COLLAPSED: {
            ret = JS_NewBool(ctx, ImGui::IsWindowCollapsed());
            break;
        }
        case IMGUI_IS_WINDOW_FOCUSED: {
            int32_t flags = 0;
            if (argc >= 1) JS_ToInt32(ctx, &flags, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsWindowFocused(flags));
            break;
        }
        case IMGUI_IS_WINDOW_HOVERED: {
            int32_t flags = 0;
            if (argc >= 1) JS_ToInt32(ctx, &flags, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsWindowHovered(flags));
            break;
        }
        case IMGUI_GET_WINDOW_DRAW_LIST:
            break;
        case IMGUI_GET_WINDOW_POS: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetWindowPos());
            break;
        }
        case IMGUI_GET_WINDOW_SIZE: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetWindowSize());
            break;
        }
        case IMGUI_GET_WINDOW_WIDTH: {
            ret = JS_NewFloat64(ctx, ImGui::GetWindowWidth());
            break;
        }
        case IMGUI_GET_WINDOW_HEIGHT: {
            ret = JS_NewFloat64(ctx, ImGui::GetWindowHeight());
            break;
        }
        case IMGUI_GET_CONTENT_REGION_MAX: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetContentRegionMax());
            break;
        }
        case IMGUI_GET_CONTENT_REGION_AVAIL: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetContentRegionAvail());
            break;
        }
        case IMGUI_GET_WINDOW_CONTENT_REGION_MIN: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetWindowContentRegionMin());
            break;
        }
        case IMGUI_GET_WINDOW_CONTENT_REGION_MAX: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetWindowContentRegionMax());
            break;
        }
        case IMGUI_SET_NEXT_WINDOW_POS: {
            ImVec2 pos = js_imgui_getimvec2(ctx, argv[0]), pivot(0, 0);
            int32_t cond = 0;
            if (argc >= 2) JS_ToInt32(ctx, &cond, argv[1]);
            if (argc >= 3) pivot = js_imgui_getimvec2(ctx, argv[2]);
            ImGui::SetNextWindowPos(pos, cond, pivot);
            break;
        }
        case IMGUI_SET_NEXT_WINDOW_SIZE: {
            ImVec2 size = js_imgui_getimvec2(ctx, argv[0]);
            int32_t cond = 0;
            if (argc >= 2) JS_ToInt32(ctx, &cond, argv[1]);
            ImGui::SetNextWindowSize(size, cond);
            break;
        }
        case IMGUI_SET_NEXT_WINDOW_SIZE_CONSTRAINTS:
            break;
        case IMGUI_SET_NEXT_WINDOW_CONTENT_SIZE: {
            ImGui::SetNextWindowContentSize(js_imgui_getimvec2(ctx, argv[0]));
            break;
        }
        case IMGUI_SET_NEXT_WINDOW_COLLAPSED: {
            bool collapsed = JS_ToBool(ctx, argv[0]);
            int32_t cond = 0;
            if (argc >= 2) JS_ToInt32(ctx, &cond, argv[1]);
            ImGui::SetNextWindowCollapsed(collapsed, cond);
        }
        case IMGUI_SET_NEXT_WINDOW_FOCUS: {
            ImGui::SetNextWindowFocus();
            break;
        }
        case IMGUI_SET_NEXT_WINDOW_BG_ALPHA: {
            double alpha;
            JS_ToFloat64(ctx, &alpha, argv[0]);
            ImGui::SetNextWindowBgAlpha(alpha);
            break;
        }
        case IMGUI_SET_WINDOW_POS: {
            ImVec2 pos = js_imgui_getimvec2(ctx, argv[0]);
            int32_t cond = 0;
            if (argc >= 2) JS_ToInt32(ctx, &cond, argv[1]);
            ImGui::SetWindowPos(pos, cond);
            break;
        }
        case IMGUI_SET_WINDOW_SIZE: {
            ImVec2 size = js_imgui_getimvec2(ctx, argv[0]);
            int32_t cond = 0;
            if (argc >= 2) JS_ToInt32(ctx, &cond, argv[1]);
            ImGui::SetWindowSize(size, cond);
            break;
        }
        case IMGUI_SET_WINDOW_COLLAPSED: {
            bool collapsed = JS_ToBool(ctx, argv[0]);
            int32_t cond = 0;
            if (argc >= 2) JS_ToInt32(ctx, &cond, argv[1]);
            ImGui::SetWindowCollapsed(collapsed, cond);
        }
        case IMGUI_SET_WINDOW_FOCUS: {
            if (argc >= 1) {
                const char *name = JS_ToCString(ctx, argv[0]);
                ImGui::SetWindowFocus(name);
                JS_FreeCString(ctx, name);
            } else {
                ImGui::SetWindowFocus();
            }
            break;
        }
        case IMGUI_SET_WINDOW_FONT_SCALE: {
            double scale;
            JS_ToFloat64(ctx, &scale, argv[0]);
            ImGui::SetWindowFontScale(scale);
            break;
        }
        case IMGUI_GET_SCROLL_X: {
            ret = JS_NewFloat64(ctx, ImGui::GetScrollX());
            break;
        }
        case IMGUI_GET_SCROLL_Y: {
            ret = JS_NewFloat64(ctx, ImGui::GetScrollY());
            break;
        }
        case IMGUI_GET_SCROLL_MAX_X: {
            ret = JS_NewFloat64(ctx, ImGui::GetScrollMaxX());
            break;
        }
        case IMGUI_GET_SCROLL_MAX_Y: {
            ret = JS_NewFloat64(ctx, ImGui::GetScrollMaxY());
            break;
        }
        case IMGUI_SET_SCROLL_X: {
            double x;
            JS_ToFloat64(ctx, &x, argv[0]);
            ImGui::SetScrollX(x);
        }
        case IMGUI_SET_SCROLL_Y: {
            double y;
            JS_ToFloat64(ctx, &y, argv[0]);
            ImGui::SetScrollX(y);
        }
        case IMGUI_SET_SCROLL_HERE_X: {
            double center_x_ratio = 0.5;
            if (argc >= 1) JS_ToFloat64(ctx, &center_x_ratio, argv[0]);
            ImGui::SetScrollHereX(center_x_ratio);
        }
        case IMGUI_SET_SCROLL_HERE_Y: {
            double center_y_ratio = 0.5;
            if (argc >= 1) JS_ToFloat64(ctx, &center_y_ratio, argv[0]);
            ImGui::SetScrollHereX(center_y_ratio);
        }
        case IMGUI_SET_SCROLL_FROM_POS_X: {
            double local_x, center_x_ratio = 0.5;
            JS_ToFloat64(ctx, &local_x, argv[0]);
            if (argc >= 2) JS_ToFloat64(ctx, &center_x_ratio, argv[1]);
            ImGui::SetScrollFromPosX(local_x, center_x_ratio);
        }
        case IMGUI_SET_SCROLL_FROM_POS_Y: {
            double local_y, center_y_ratio = 0.5;
            JS_ToFloat64(ctx, &local_y, argv[0]);
            if (argc >= 2) JS_ToFloat64(ctx, &center_y_ratio, argv[1]);
            ImGui::SetScrollFromPosX(local_y, center_y_ratio);
        }
        case IMGUI_PUSH_FONT: {
            ImFont *font = js_imfont_data2(ctx, argv[0]);
            ImGui::PushFont(font);
            break;
        }
        case IMGUI_POP_FONT: {
            ImGui::PopFont();
            break;
        }
        case IMGUI_PUSH_STYLE_COLOR: {
            int32_t idx;
            JS_ToInt32(ctx, &idx, argv[0]);
            if (JS_IsNumber(argv[1])) {
                uint32_t col;
                JS_ToUint32(ctx, &col, argv[1]);
                ImGui::PushStyleColor(idx, col);
            } else {
                ImVec4 vec = js_imgui_getimvec4(ctx, argv[1]);
                ImGui::PushStyleColor(idx, vec);
            }
            break;
        }
        case IMGUI_POP_STYLE_COLOR: {
            uint32_t count = 1;
            if (argc >= 1) JS_ToUint32(ctx, &count, argv[0]);
            ImGui::PopStyleColor(count);
            break;
        }
        case IMGUI_PUSH_STYLE_VAR: {
            int32_t idx;
            JS_ToInt32(ctx, &idx, argv[0]);
            if (JS_IsNumber(argv[1])) {
                double val;
                JS_ToFloat64(ctx, &val, argv[1]);
                ImGui::PushStyleVar(idx, val);
            } else {
                ImVec2 vec = js_imgui_getimvec2(ctx, argv[1]);
                ImGui::PushStyleVar(idx, vec);
            }
            break;
        }
        case IMGUI_POP_STYLE_VAR: {
            uint32_t count = 1;
            if (argc >= 1) JS_ToUint32(ctx, &count, argv[0]);
            ImGui::PopStyleVar(count);
            break;
        }
        case IMGUI_GET_STYLE_COLOR_VEC4: {
            uint32_t col;
            JS_ToUint32(ctx, &col, argv[0]);
            ret = js_imgui_newimvec4(ctx, ImGui::GetStyleColorVec4(col));
            break;
        }
        case IMGUI_GET_FONT: {
            ret = js_imfont_wrap(ctx, ImGui::GetFont());
            break;
        }
        case IMGUI_GET_FONT_SIZE: {
            ret = JS_NewFloat64(ctx, ImGui::GetFontSize());
            break;
        }
        case IMGUI_GET_FONT_TEX_UV_WHITE_PIXEL: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetFontTexUvWhitePixel());
            break;
        }
        case IMGUI_GET_COLORU32: {
            if (JS_IsArray(ctx, argv[0])) {
                ImVec4 color = js_imgui_getimvec4(ctx, argv[0]);
                ret = JS_NewUint32(ctx, ImGui::GetColorU32(color));
            } else if (argc >= 2) {
                int32_t idx;
                double alpha_mul = 1.0f;
                JS_ToInt32(ctx, &idx, argv[0]);
                JS_ToFloat64(ctx, &alpha_mul, argv[1]);
                ret = JS_NewUint32(ctx, ImGui::GetColorU32(idx, alpha_mul));
            } else {
                uint32_t col;
                JS_ToUint32(ctx, &col, argv[0]);
                ret = JS_NewUint32(ctx, ImGui::GetColorU32(col));
            }
            break;
        }
        case IMGUI_PUSH_ITEM_WIDTH: {
            int32_t width;
            JS_ToInt32(ctx, &width, argv[0]);
            ImGui::PushItemWidth(width);
            break;
        }
        case IMGUI_POP_ITEM_WIDTH: {
            ImGui::PopItemWidth();
            break;
        }
        case IMGUI_CALC_ITEM_WIDTH: {
            ret = JS_NewFloat64(ctx, ImGui::CalcItemWidth());
            break;
        }
        case IMGUI_PUSH_TEXT_WRAP_POS: {
            double wrap_local_pos_x = 0;
            JS_ToFloat64(ctx, &wrap_local_pos_x, argv[0]);
            ImGui::PushTextWrapPos(wrap_local_pos_x);
            break;
        }
        case IMGUI_POP_TEXT_WRAP_POS: {
            ImGui::PopTextWrapPos();
            break;
        }
        case IMGUI_PUSH_ALLOW_KEYBOARD_FOCUS: {
            ImGui::PushAllowKeyboardFocus(JS_ToBool(ctx, argv[0]));
            break;
        }
        case IMGUI_POP_ALLOW_KEYBOARD_FOCUS: {
            ImGui::PopAllowKeyboardFocus();
            break;
        }
        case IMGUI_PUSH_BUTTON_REPEAT: {
            ImGui::PushButtonRepeat(JS_ToBool(ctx, argv[0]));
            break;
        }
        case IMGUI_POP_BUTTON_REPEAT: {
            ImGui::PopButtonRepeat();
            break;
        }
        case IMGUI_SEPARATOR: {
            ImGui::Separator();
            break;
        }
        case IMGUI_SAME_LINE: {
            double offset_from_start_x = 0, spacing = -1.0;
            JS_ToFloat64(ctx, &offset_from_start_x, argv[0]);
            if (argc >= 2) JS_ToFloat64(ctx, &spacing, argv[1]);
            ImGui::SameLine(offset_from_start_x, spacing);
            break;
        }
        case IMGUI_NEW_LINE: {
            ImGui::NewLine();
            break;
        }
        case IMGUI_SPACING: {
            ImGui::Spacing();
            break;
        }
        case IMGUI_DUMMY: {
            ImGui::Dummy(js_imgui_getimvec2(ctx, argv[0]));
            break;
        }
        case IMGUI_INDENT: {
            double indent_w = 0;
            JS_ToFloat64(ctx, &indent_w, argv[0]);
            ImGui::Indent(indent_w);
            break;
        }
        case IMGUI_UNINDENT: {
            double indent_w = 0;
            JS_ToFloat64(ctx, &indent_w, argv[0]);
            ImGui::Unindent(indent_w);
            break;
        }
        case IMGUI_BEGIN_GROUP: {
            ImGui::BeginGroup();
            break;
        }
        case IMGUI_END_GROUP: {
            ImGui::EndGroup();
            break;
        }
        case IMGUI_GET_CURSOR_POS: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetCursorPos());
            break;
        }
        case IMGUI_GET_CURSOR_POS_X: {
            ret = JS_NewFloat64(ctx, ImGui::GetCursorPosX());
            break;
        }
        case IMGUI_GET_CURSOR_POS_Y: {
            ret = JS_NewFloat64(ctx, ImGui::GetCursorPosY());
            break;
        }
        case IMGUI_SET_CURSOR_POS: {
            ImGui::SetCursorPos(js_imgui_getimvec2(ctx, argv[0]));
            break;
        }
        case IMGUI_SET_CURSOR_POS_X: {
            double x;
            JS_ToFloat64(ctx, &x, argv[0]);
            ImGui::SetCursorPosX(x);
            break;
        }
        case IMGUI_SET_CURSOR_POS_Y: {
            double y;
            JS_ToFloat64(ctx, &y, argv[0]);
            ImGui::SetCursorPosY(y);
            break;
        }
        case IMGUI_GET_CURSOR_START_POS: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetCursorStartPos());
            break;
        }
        case IMGUI_GET_CURSOR_SCREEN_POS: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetCursorScreenPos());
            break;
        }
        case IMGUI_SET_CURSOR_SCREEN_POS: {
            ImGui::SetCursorScreenPos(js_imgui_getimvec2(ctx, argv[0]));
            break;
        }
        case IMGUI_ALIGN_TEXT_TO_FRAME_PADDING: {
            ImGui::AlignTextToFramePadding();
            break;
        }
        case IMGUI_GET_TEXT_LINE_HEIGHT: {
            ret = JS_NewFloat64(ctx, ImGui::GetTextLineHeight());
            break;
        }
        case IMGUI_GET_TEXT_LINE_HEIGHT_WITH_SPACING: {
            ret = JS_NewFloat64(ctx, ImGui::GetTextLineHeightWithSpacing());
            break;
        }
        case IMGUI_GET_FRAME_HEIGHT: {
            ret = JS_NewFloat64(ctx, ImGui::GetFrameHeight());
            break;
        }
        case IMGUI_GET_FRAME_HEIGHT_WITH_SPACING: {
            ret = JS_NewFloat64(ctx, ImGui::GetFrameHeightWithSpacing());
            break;
        }
        case IMGUI_PUSH_ID: {
            if (argc >= 2) {
                const char *id_begin = JS_ToCString(ctx, argv[0]),
                           *id_end = JS_ToCString(ctx, argv[1]);
                ImGui::PushID(id_begin, id_end);
                JS_FreeCString(ctx, id_begin);
                JS_FreeCString(ctx, id_end);
            } else if (JS_IsString(argv[0])) {
                const char *id = JS_ToCString(ctx, argv[0]);
                ImGui::PushID(id);
                JS_FreeCString(ctx, id);
            } else if (JS_IsNumber(argv[0])) {
                int32_t id;
                JS_ToInt32(ctx, &id, argv[0]);
                ImGui::PushID(id);
            } else {
                ret = JS_ThrowInternalError(ctx, "ImGui::PushID() invalid argument");
            }
            break;
        }
        case IMGUI_POP_ID: {
            ImGui::PopID();
            break;
        }
        case IMGUI_GET_ID: {
            ImGuiID id = -1;
            if (argc >= 2) {
                const char *id_begin = JS_ToCString(ctx, argv[0]),
                           *id_end = JS_ToCString(ctx, argv[1]);
                id = ImGui::GetID(id_begin, id_end);
                JS_FreeCString(ctx, id_begin);
                JS_FreeCString(ctx, id_end);
            } else if (JS_IsString(argv[0])) {
                const char *id_str = JS_ToCString(ctx, argv[0]);
                id = ImGui::GetID(id_str);
                JS_FreeCString(ctx, id_str);
            } else {
                ret = JS_ThrowInternalError(ctx, "ImGui::GetID() invalid argument");
                break;
            }
            ret = JS_NewInt32(ctx, id);
            break;
        }
        case IMGUI_TEXT_UNFORMATTED: {
            const char *text = JS_ToCString(ctx, argv[0]), *text_end = 0;
            if (argc >= 2) text_end = JS_ToCString(ctx, argv[1]);
            ImGui::TextUnformatted(text, text_end);
            JS_FreeCString(ctx, text);
            if (text_end) JS_FreeCString(ctx, text_end);
            break;
        }
        case IMGUI_TEXT: {
            int count = js_imgui_formatcount(ctx, argc, argv);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc, argv, a);
            ImGui::Text((const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9],
                        a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18], a[19], a[20],
                        a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28], a[29], a[30],
                        a[31]);
            break;
        }
        case IMGUI_TEXT_COLORED: {
            ImVec4 color = js_imgui_getcolor(ctx, argv[0]);
            int count = js_imgui_formatcount(ctx, argc - 1, argv + 1);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc - 1, argv + 1, a);
            ImGui::TextColored(color, (const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
                               a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17],
                               a[18], a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27],
                               a[28], a[29], a[30], a[31]);
            break;
        }
        case IMGUI_TEXT_DISABLED: {
            int count = js_imgui_formatcount(ctx, argc, argv);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc, argv, a);
            ImGui::TextDisabled((const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                                a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18],
                                a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27],
                                a[28], a[29], a[30], a[31]);
            break;
        }
        case IMGUI_TEXT_WRAPPED: {
            int count = js_imgui_formatcount(ctx, argc, argv);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc, argv, a);
            ImGui::TextWrapped((const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                               a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18],
                               a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28],
                               a[29], a[30], a[31]);
            break;
        }
        case IMGUI_LABEL_TEXT: {
            const char *label = JS_ToCString(ctx, argv[0]);
            int count = js_imgui_formatcount(ctx, argc - 1, argv + 1);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc - 1, argv + 1, a);
            ImGui::LabelText(label, (const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
                             a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17],
                             a[18], a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27],
                             a[28], a[29], a[30], a[31]);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_BULLET_TEXT: {
            int count = js_imgui_formatcount(ctx, argc, argv);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc, argv, a);
            ImGui::BulletText((const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                              a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18],
                              a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28],
                              a[29], a[30], a[31]);
            break;
        }
        case IMGUI_BUTTON: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ImVec2 size(0, 0);
            if (argc >= 2) size = js_imgui_getimvec2(ctx, argv[1]);
            ret = JS_NewBool(ctx, ImGui::Button(label, size));
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_SMALL_BUTTON: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ret = JS_NewBool(ctx, ImGui::SmallButton(label));
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_INVISIBLE_BUTTON: {
            const char *str_id = JS_ToCString(ctx, argv[0]);
            ImVec2 size(0, 0);
            int32_t flags = 0;
            if (argc >= 2) size = js_imgui_getimvec2(ctx, argv[1]);
            if (argc >= 3) JS_ToInt32(ctx, &flags, argv[2]);
            ret = JS_NewBool(ctx, ImGui::InvisibleButton(str_id, size, flags));
            JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_ARROW_BUTTON: {
            const char *str_id = JS_ToCString(ctx, argv[0]);
            int32_t dir = 0;
            if (argc >= 2) JS_ToInt32(ctx, &dir, argv[1]);
            ret = JS_NewBool(ctx, ImGui::ArrowButton(str_id, dir));
            JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_IMAGE: {
            ImTextureID tex = js_imgui_gettexture(ctx, argv[0]);
            ImVec2 size = js_imgui_getimvec2(ctx, argv[1]);
            ImVec2 uv0(0, 0), uv1(1, 1);
            ImVec4 tint_col(1, 1, 1, 1), border_col(0, 0, 0, 0);
            if (argc >= 3) uv0 = js_imgui_getimvec2(ctx, argv[2]);
            if (argc >= 4) uv1 = js_imgui_getimvec2(ctx, argv[3]);
            if (argc >= 5) tint_col = js_imgui_getimvec4(ctx, argv[4]);
            if (argc >= 6) border_col = js_imgui_getimvec4(ctx, argv[5]);
            ImGui::Image(tex, size, uv0, uv1, tint_col, border_col);
            break;
        }
        case IMGUI_IMAGE_BUTTON: {
            ImTextureID tex = js_imgui_gettexture(ctx, argv[0]);
            ImVec2 size = js_imgui_getimvec2(ctx, argv[1]);
            ImVec2 uv0(0, 0), uv1(1, 1);
            int32_t frame_padding = -1;
            ImVec4 tint_col(1, 1, 1, 1), border_col(0, 0, 0, 0);
            if (argc >= 3) uv0 = js_imgui_getimvec2(ctx, argv[2]);
            if (argc >= 4) uv1 = js_imgui_getimvec2(ctx, argv[3]);
            if (argc >= 5) JS_ToInt32(ctx, &frame_padding, argv[4]);
            if (argc >= 6) tint_col = js_imgui_getimvec4(ctx, argv[5]);
            if (argc >= 7) border_col = js_imgui_getimvec4(ctx, argv[6]);
            ret = JS_NewBool(ctx, ImGui::ImageButton(tex, size, uv0, uv1, frame_padding, tint_col,
                                                     border_col));
            break;
        }
        case IMGUI_CHECKBOX:
            break;
        case IMGUI_CHECKBOX_FLAGS:
            break;
        case IMGUI_RADIO_BUTTON: {
            const char *label = JS_ToCString(ctx, argv[0]);
            bool active = JS_ToBool(ctx, argv[1]);
            ret = JS_NewBool(ctx, ImGui::RadioButton(label, active));
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_PROGRESS_BAR: {
            double fraction;
            ImVec2 size_arg(-FLT_MIN, 0);
            const char *overlay = 0;
            JS_ToFloat64(ctx, &fraction, argv[0]);
            if (argc >= 2) size_arg = js_imgui_getimvec2(ctx, argv[1]);
            if (argc >= 3) overlay = JS_ToCString(ctx, argv[2]);
            ImGui::ProgressBar(fraction, size_arg, overlay);
            if (overlay) JS_FreeCString(ctx, overlay);
            break;
        }
        case IMGUI_BULLET: {
            ImGui::Bullet();
            break;
        }
        case IMGUI_BEGIN_COMBO: {
            const char *label = JS_ToCString(ctx, argv[0]),
                       *preview_value = JS_ToCString(ctx, argv[1]);
            int32_t flags = 0;
            if (argc >= 3) JS_ToInt32(ctx, &flags, argv[2]);
            ImGui::BeginCombo(label, preview_value, flags);
            JS_FreeCString(ctx, label);
            JS_FreeCString(ctx, preview_value);
            break;
        }
        case IMGUI_END_COMBO: {
            ImGui::EndCombo();
            break;
        }
        case IMGUI_COMBO:
            break;
        case IMGUI_DRAG_FLOAT:
            break;
        case IMGUI_DRAG_FLOAT2:
            break;
        case IMGUI_DRAG_FLOAT3:
            break;
        case IMGUI_DRAG_FLOAT4:
            break;
        case IMGUI_DRAG_FLOAT_RANGE2:
            break;
        case IMGUI_DRAG_INT:
            break;
        case IMGUI_DRAG_INT2:
            break;
        case IMGUI_DRAG_INT3:
            break;
        case IMGUI_DRAG_INT4:
            break;
        case IMGUI_DRAG_INT_RANGE2:
            break;
        case IMGUI_DRAG_SCALAR:
            break;
        case IMGUI_DRAG_SCALAR_N:
            break;
        case IMGUI_SLIDER_FLOAT:
            break;
        case IMGUI_SLIDER_FLOAT2:
            break;
        case IMGUI_SLIDER_FLOAT3:
            break;
        case IMGUI_SLIDER_FLOAT4:
            break;
        case IMGUI_SLIDER_ANGLE:
            break;
        case IMGUI_SLIDER_INT:
            break;
        case IMGUI_SLIDER_INT2:
            break;
        case IMGUI_SLIDER_INT3:
            break;
        case IMGUI_SLIDER_INT4:
            break;
        case IMGUI_SLIDER_SCALAR:
            break;
        case IMGUI_SLIDER_SCALAR_N:
            break;
        case IMGUI_V_SLIDER_FLOAT:
            break;
        case IMGUI_V_SLIDER_INT:
            break;
        case IMGUI_V_SLIDER_SCALAR:
            break;
        case IMGUI_INPUT_TEXT:
            break;
        case IMGUI_INPUT_TEXT_MULTILINE:
            break;
        case IMGUI_INPUT_TEXT_WITH_HINT:
            break;
        case IMGUI_INPUT_FLOAT:
            break;
        case IMGUI_INPUT_FLOAT2:
            break;
        case IMGUI_INPUT_FLOAT3:
            break;
        case IMGUI_INPUT_FLOAT4:
            break;
        case IMGUI_INPUT_INT:
            break;
        case IMGUI_INPUT_INT2:
            break;
        case IMGUI_INPUT_INT3:
            break;
        case IMGUI_INPUT_INT4:
            break;
        case IMGUI_INPUT_DOUBLE:
            break;
        case IMGUI_INPUT_SCALAR:
            break;
        case IMGUI_INPUT_SCALAR_N:
            break;
        case IMGUI_COLOR_EDIT3:
            break;
        case IMGUI_COLOR_EDIT4:
            break;
        case IMGUI_COLOR_PICKER3:
            break;
        case IMGUI_COLOR_PICKER4:
            break;
        case IMGUI_COLOR_BUTTON:
            break;
        case IMGUI_SET_COLOR_EDIT_OPTIONS: {
            int32_t options;
            JS_ToInt32(ctx, &options, argv[0]);
            ImGui::SetColorEditOptions(options);
            break;
        }
        case IMGUI_TREE_NODE: {
            if (argc > 1 /* && JS_IsString(argv[0])*/) {
                const char *str_id = JS_ToCString(ctx, argv[0]);
                int count = js_imgui_formatcount(ctx, argc - 1, argv + 1);
                assert(count <= 32);
                void *a[count];
                js_imgui_formatargs(ctx, argc - 1, argv + 1, a);
                ret = JS_NewBool(ctx,
                                 ImGui::TreeNode(str_id, (const char *) a[0], a[1], a[2], a[3],
                                                 a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11],
                                                 a[12], a[13], a[14], a[15], a[16], a[17], a[18],
                                                 a[19], a[20], a[21], a[22], a[23], a[24], a[25],
                                                 a[26], a[27], a[28], a[29], a[30], a[31]));
                JS_FreeCString(ctx, str_id);
            } else {
                const char *label = JS_ToCString(ctx, argv[0]);
                ret = JS_NewBool(ctx, ImGui::TreeNode(label));
                JS_FreeCString(ctx, label);
            }
            break;
        }
        case IMGUI_TREE_NODE_EX: {
            int32_t flags = 0;
            JS_ToInt32(ctx, &flags, argv[1]);

            if (argc > 1 /* && JS_IsString(argv[0])*/) {
                const char *str_id = JS_ToCString(ctx, argv[0]);
                int count = js_imgui_formatcount(ctx, argc - 2, argv + 2);
                assert(count <= 32);
                void *a[count];
                js_imgui_formatargs(ctx, argc - 2, argv + 2, a);
                ret = JS_NewBool(ctx, ImGui::TreeNodeEx(str_id, flags, (const char *) a[0], a[1],
                                                        a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                                                        a[9], a[10], a[11], a[12], a[13], a[14],
                                                        a[15], a[16], a[17], a[18], a[19], a[20],
                                                        a[21], a[22], a[23], a[24], a[25], a[26],
                                                        a[27], a[28], a[29], a[30], a[31]));
                JS_FreeCString(ctx, str_id);
            } else {
                const char *label = JS_ToCString(ctx, argv[0]);
                ret = JS_NewBool(ctx, ImGui::TreeNodeEx(label, flags));
                JS_FreeCString(ctx, label);
            }
            break;
        }
        case IMGUI_TREE_PUSH: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ImGui::TreePush(label);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_TREE_POP: {
            ImGui::TreePop();
            break;
        }
        case IMGUI_GET_TREE_NODE_TO_LABEL_SPACING: {
            ret = JS_NewFloat64(ctx, ImGui::GetTreeNodeToLabelSpacing());
            break;
        }
        case IMGUI_COLLAPSING_HEADER: {
            const char *label = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0;
            OutputArg<bool> p_visible(ctx, argc >= 2 ? argv[1] : JS_NULL);
            if (argc >= 3) JS_ToInt32(ctx, &flags, argv[2]);

            ImGui::CollapsingHeader(label, p_visible, flags);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_SET_NEXT_ITEM_OPEN: {
            bool is_open = JS_ToBool(ctx, argv[0]);
            int32_t flags = 0;
            JS_ToInt32(ctx, &flags, argv[1]);
            ImGui::SetNextItemOpen(is_open, flags);
            break;
        }
        case IMGUI_SELECTABLE: {
            const char *label = JS_ToCString(ctx, argv[0]);
            OutputArg<bool> selected(ctx, argc >= 2 ? argv[1] : JS_NULL);
            int32_t flags = 0;
            ImVec2 size(0, 0);
            if (argc >= 3) JS_ToInt32(ctx, &flags, argv[2]);
            if (argc >= 4) size = js_imgui_getimvec2(ctx, argv[3]);
            ImGui::Selectable(label, selected, flags, size);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_BEGIN_LIST_BOX: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ImVec2 size(0, 0);
            if (argc >= 2) size = js_imgui_getimvec2(ctx, argv[1]);
            ImGui::BeginListBox(label, size);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_END_LIST_BOX: {
            ImGui::EndListBox();
            break;
        }
        case IMGUI_LIST_BOX:
            break;
        case IMGUI_LIST_BOX_HEADER:
            break;
        case IMGUI_LIST_BOX_FOOTER:
            break;

        case IMGUI_PLOT_LINES:
            break;
        case IMGUI_PLOT_HISTOGRAM:
            break;
        case IMGUI_VALUE: {
            const char *prefix = JS_ToCString(ctx, argv[0]);
            if (JS_IsBool(argv[1])) {
                ImGui::Value(prefix, static_cast<bool>(JS_ToBool(ctx, argv[1])));
            } else if (JS_IsNumber(argv[1])) {
                if (JS_VALUE_GET_TAG(argv[1]) == JS_TAG_FLOAT64) {
                    double f;
                    JS_ToFloat64(ctx, &f, argv[1]);
                    ImGui::Value(prefix, static_cast<float>(f));
                } else {
                    int64_t i64;
                    JS_ToInt64(ctx, &i64, argv[1]);
                    if (i64 >= 0) ImGui::Value(prefix, static_cast<unsigned int>(i64));
                    else
                        ImGui::Value(prefix, static_cast<int>(i64));
                }
            }
            JS_FreeCString(ctx, prefix);
            break;
        }
        case IMGUI_BEGIN_MAIN_MENU_BAR: {
            ret = JS_NewBool(ctx, ImGui::BeginMainMenuBar());
            break;
        }
        case IMGUI_END_MAIN_MENU_BAR: {
            ImGui::EndMainMenuBar();
            break;
        }
        case IMGUI_BEGIN_MENU_BAR: {
            ret = JS_NewBool(ctx, ImGui::BeginMenuBar());
            break;
        }
        case IMGUI_END_MENU_BAR: {
            ImGui::EndMenuBar();
            break;
        }
        case IMGUI_BEGIN_MENU: {
            const char *label = JS_ToCString(ctx, argv[0]);
            bool enabled = true;
            if (argc >= 2) enabled = JS_ToBool(ctx, argv[1]);
            ImGui::BeginMenu(label, enabled);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_END_MENU: {
            ImGui::EndMenu();
            break;
        }
        case IMGUI_MENU_ITEM: {
            const char *label = JS_ToCString(ctx, argv[0]), *shortcut = 0;
            OutputArg<bool> selected(ctx, argc >= 3 ? argv[2] : JS_NULL);
            if (argc >= 2) shortcut = JS_ToCString(ctx, argv[1]);
            ret = JS_NewBool(ctx, ImGui::MenuItem(label, shortcut, selected,
                                                  argc >= 4 ? JS_ToBool(ctx, argv[3]) : true));
            JS_FreeCString(ctx, label);
            if (shortcut) JS_FreeCString(ctx, shortcut);
            break;
        }
        case IMGUI_BEGIN_TOOLTIP: {
            ImGui::BeginTooltip();
            break;
        }
        case IMGUI_END_TOOLTIP: {
            ImGui::EndTooltip();
            break;
        }
        case IMGUI_SET_TOOLTIP: {
            int count = js_imgui_formatcount(ctx, argc, argv);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc, argv, a);
            ImGui::SetTooltip((const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                              a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18],
                              a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28],
                              a[29], a[30], a[31]);
            break;
        }
        case IMGUI_OPEN_POPUP: {
            if (JS_IsString(argv[0])) {
                const char *str_id = JS_ToCString(ctx, argv[0]);
                int32_t flags = 0;
                if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
                ImGui::OpenPopup(str_id, flags);
                JS_FreeCString(ctx, str_id);
            } else {
                int32_t id;
                JS_ToInt32(ctx, &id, argv[0]);
                int32_t flags = 0;
                if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
                ImGui::OpenPopup(id, flags);
            }
            break;
        }
        case IMGUI_BEGIN_POPUP: {
            const char *str_id = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0;
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ret = JS_NewBool(ctx, ImGui::BeginPopup(str_id, flags));
            JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_BEGIN_POPUP_CONTEXT_ITEM: {
            const char *str_id = 0;
            int32_t flags = 1;
            if (argc >= 1) str_id = JS_ToCString(ctx, argv[0]);
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ImGui::BeginPopupContextItem(str_id, flags);
            if (str_id) JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_BEGIN_POPUP_CONTEXT_WINDOW: {
            const char *str_id = 0;
            int32_t flags = 1;
            if (argc >= 1) str_id = JS_ToCString(ctx, argv[0]);
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ImGui::BeginPopupContextWindow(str_id, flags);
            if (str_id) JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_BEGIN_POPUP_CONTEXT_VOID: {
            const char *str_id = 0;
            int32_t flags = 1;
            if (argc >= 1) str_id = JS_ToCString(ctx, argv[0]);
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ImGui::BeginPopupContextVoid(str_id, flags);
            if (str_id) JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_BEGIN_POPUP_MODAL:
            break;
        case IMGUI_END_POPUP: {
            ImGui::EndPopup();
            break;
        }
        case IMGUI_OPEN_POPUP_ON_ITEM_CLICK: {
            const char *str_id = 0;
            int32_t flags = 1;
            if (argc >= 1) str_id = JS_ToCString(ctx, argv[0]);
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ImGui::OpenPopupOnItemClick(str_id, flags);
            if (str_id) JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_IS_POPUP_OPEN: {
            const char *str_id = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0;
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ImGui::IsPopupOpen(str_id, flags);
            JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_BEGIN_TABLE: {
            const char *str_id = JS_ToCString(ctx, argv[0]);
            int32_t column, flags = 0;
            ImVec2 outer_size(0, 0);
            double inner_width = 0.0f;
            JS_ToInt32(ctx, &column, argv[1]);
            if (argc >= 3) JS_ToInt32(ctx, &flags, argv[2]);
            if (argc >= 4) outer_size = js_imgui_getimvec2(ctx, argv[3]);
            if (argc >= 5) JS_ToFloat64(ctx, &inner_width, argv[4]);
            ret = JS_NewBool(ctx,
                             ImGui::BeginTable(str_id, column, flags, outer_size, inner_width));
            JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_END_TABLE: {
            ImGui::EndTable();
            break;
        }
        case IMGUI_TABLE_NEXT_ROW: {
            int32_t row_flags = 0;
            double min_row_height = 0.0f;

            if (argc >= 1) JS_ToInt32(ctx, &row_flags, argv[0]);
            if (argc >= 2) JS_ToFloat64(ctx, &min_row_height, argv[1]);
            ImGui::TableNextRow(row_flags, min_row_height);
            break;
        }
        case IMGUI_TABLE_NEXT_COLUMN: {
            ret = JS_NewBool(ctx, ImGui::TableNextColumn());
            break;
        }
        case IMGUI_TABLE_SET_COLUMN_INDEX: {
            int32_t column_index;
            JS_ToInt32(ctx, &column_index, argv[0]);
            ret = JS_NewBool(ctx, ImGui::TableSetColumnIndex(column_index));
            break;
        }
        case IMGUI_TABLE_SETUP_COLUMN: {
            const char *label = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0, user_id = 0;
            double init_width_or_weight = 0.0f;
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            if (argc >= 3) JS_ToFloat64(ctx, &init_width_or_weight, argv[2]);
            if (argc >= 4) JS_ToInt32(ctx, &user_id, argv[3]);
            ImGui::TableSetupColumn(label, flags, init_width_or_weight, user_id);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_TABLE_SETUP_SCROLL_FREEZE: {
            int32_t cols, rows;
            JS_ToInt32(ctx, &cols, argv[0]);
            JS_ToInt32(ctx, &rows, argv[1]);
            ImGui::TableSetupScrollFreeze(cols, rows);
            break;
        }
        case IMGUI_TABLE_HEADERS_ROW: {
            ImGui::TableHeadersRow();
            break;
        }
        case IMGUI_TABLE_HEADER: {
            const char *label = JS_ToCString(ctx, argv[0]);

            ImGui::TableHeader(label);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_TABLE_GET_SORT_SPECS:
            break;
        case IMGUI_TABLE_GET_COLUMN_COUNT: {
            ret = JS_NewUint32(ctx, ImGui::TableGetColumnCount());
            break;
        }
        case IMGUI_TABLE_GET_COLUMN_INDEX: {
            ret = JS_NewInt32(ctx, ImGui::TableGetColumnIndex());
            break;
        }
        case IMGUI_TABLE_GET_ROW_INDEX: {
            ret = JS_NewInt32(ctx, ImGui::TableGetRowIndex());
            break;
        }
        case IMGUI_TABLE_GET_COLUMN_NAME: {
            int32_t col = -1;
            if (argc >= 1) JS_ToInt32(ctx, &col, argv[0]);
            ret = JS_NewString(ctx, ImGui::TableGetColumnName(col));
            break;
        }
        case IMGUI_TABLE_GET_COLUMN_FLAGS: {
            int32_t col = -1;
            if (argc >= 1) JS_ToInt32(ctx, &col, argv[0]);
            ret = JS_NewInt32(ctx, ImGui::TableGetColumnFlags(col));
            break;
        }
        case IMGUI_TABLE_SET_COLUMN_ENABLED: {
            int32_t col;
            JS_ToInt32(ctx, &col, argv[0]);
            ImGui::TableSetColumnEnabled(col, JS_ToBool(ctx, argv[1]));
            break;
        }
        case IMGUI_TABLE_SET_BG_COLOR:
            break;
        case IMGUI_BUILD_LOOKUP_TABLE:
            break;
        case IMGUI_CLOSE_CURRENT_POPUP: {
            ImGui::CloseCurrentPopup();
            break;
        }
        case IMGUI_COLUMNS: {
            int32_t count = 1;
            const char *id = 0;
            bool border = true;
            if (argc >= 1) JS_ToInt32(ctx, &count, argv[0]);
            if (argc >= 2) id = JS_ToCString(ctx, argv[1]);
            if (argc >= 3) border = JS_ToBool(ctx, argv[2]);
            ImGui::Columns(count, id, border);
            if (id) JS_FreeCString(ctx, id);
            break;
        }
        case IMGUI_NEXT_COLUMN: {
            ImGui::NextColumn();
            break;
        }
        case IMGUI_GET_COLUMN_INDEX: {
            ret = JS_NewInt32(ctx, ImGui::GetColumnIndex());
            break;
        }
        case IMGUI_GET_COLUMN_WIDTH: {
            int32_t col = -1;
            if (argc >= 1) JS_ToInt32(ctx, &col, argv[0]);
            ret = JS_NewFloat64(ctx, ImGui::GetColumnWidth(col));
            break;
        }
        case IMGUI_SET_COLUMN_WIDTH: {
            int32_t col;
            double width;
            JS_ToInt32(ctx, &col, argv[0]);
            JS_ToFloat64(ctx, &width, argv[1]);
            ImGui::SetColumnWidth(col, width);
            break;
        }
        case IMGUI_GET_COLUMN_OFFSET: {
            int32_t col = -1;
            if (argc >= 1) JS_ToInt32(ctx, &col, argv[0]);
            ret = JS_NewFloat64(ctx, ImGui::GetColumnOffset(col));
            break;
        }
        case IMGUI_SET_COLUMN_OFFSET: {
            int32_t col;
            double offset_x;
            JS_ToInt32(ctx, &col, argv[0]);
            JS_ToFloat64(ctx, &offset_x, argv[1]);
            ImGui::SetColumnOffset(col, offset_x);
            break;
        }
        case IMGUI_GET_COLUMNS_COUNT: {
            ret = JS_NewUint32(ctx, ImGui::GetColumnsCount());
            break;
        }
        case IMGUI_BEGIN_TAB_BAR: {
            const char *str_id = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0;
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ret = JS_NewBool(ctx, ImGui::BeginTabBar(str_id, flags));
            JS_FreeCString(ctx, str_id);
            break;
        }
        case IMGUI_END_TAB_BAR: {
            ImGui::EndTabBar();
            break;
        }
        case IMGUI_BEGIN_TAB_ITEM: {
            const char *label = JS_ToCString(ctx, argv[0]);
            OutputArg<bool> p_open(ctx, argc >= 2 ? argv[1] : JS_NULL);
            int32_t flags = 0;
            if (argc >= 3) JS_ToInt32(ctx, &flags, argv[2]);
            ret = JS_NewBool(ctx, ImGui::BeginTabItem(label, p_open, flags));
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_END_TAB_ITEM: {
            ImGui::EndTabItem();
            break;
        }
        case IMGUI_TAB_ITEM_BUTTON: {
            const char *label = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0;
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ret = JS_NewBool(ctx, ImGui::TabItemButton(label, flags));
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_SET_TAB_ITEM_CLOSED: {
            const char *label = JS_ToCString(ctx, argv[0]);
            ImGui::SetTabItemClosed(label);
            JS_FreeCString(ctx, label);
            break;
        }
        case IMGUI_LOG_TO_TTY: {
            int32_t auto_open_depth = -1;
            if (argc >= 1) JS_ToInt32(ctx, &auto_open_depth, argv[0]);
            ImGui::LogToTTY(auto_open_depth);
            break;
        }
        case IMGUI_LOG_TO_FILE: {
            int32_t auto_open_depth = -1;
            const char *filename = 0;
            if (argc >= 1) JS_ToInt32(ctx, &auto_open_depth, argv[0]);
            if (argc >= 2) filename = JS_ToCString(ctx, argv[1]);
            ImGui::LogToFile(auto_open_depth, filename);
            if (filename) JS_FreeCString(ctx, filename);
            break;
        }
        case IMGUI_LOG_TO_CLIPBOARD: {
            int32_t auto_open_depth = -1;
            if (argc >= 1) JS_ToInt32(ctx, &auto_open_depth, argv[0]);
            ImGui::LogToClipboard(auto_open_depth);
            break;
        }
        case IMGUI_LOG_FINISH: {
            ImGui::LogFinish();
            break;
        }
        case IMGUI_LOG_BUTTONS: {
            ImGui::LogButtons();
            break;
        }
        case IMGUI_LOG_TEXT: {
            int count = js_imgui_formatcount(ctx, argc, argv);
            assert(count <= 32);
            void *a[count];
            js_imgui_formatargs(ctx, argc, argv, a);
            ImGui::LogText((const char *) a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8],
                           a[9], a[10], a[11], a[12], a[13], a[14], a[15], a[16], a[17], a[18],
                           a[19], a[20], a[21], a[22], a[23], a[24], a[25], a[26], a[27], a[28],
                           a[29], a[30], a[31]);
            break;
        }
        case IMGUI_BEGIN_DRAG_DROP_SOURCE: {
            int32_t flags = 0;
            if (argc >= 1) JS_ToInt32(ctx, &flags, argv[0]);
            ret = JS_NewBool(ctx, ImGui::BeginDragDropSource(flags));
            break;
        }
        case IMGUI_SET_DRAG_DROP_PAYLOAD: {
            const char *type = JS_ToCString(ctx, argv[0]);
            uint8_t *ptr;
            size_t len;
            int32_t cond = 0;
            if (argc >= 3) JS_ToInt32(ctx, &cond, argv[2]);
            ptr = JS_GetArrayBuffer(ctx, &len, argv[1]);
            ret = JS_NewBool(ctx, ImGui::SetDragDropPayload(type, ptr, len, cond));
            JS_FreeCString(ctx, type);
            break;
        }
        case IMGUI_END_DRAG_DROP_SOURCE: {
            ImGui::EndDragDropSource();
            break;
        }
        case IMGUI_BEGIN_DRAG_DROP_TARGET: {
            ret = JS_NewBool(ctx, ImGui::BeginDragDropTarget());
            break;
        }
        case IMGUI_ACCEPT_DRAG_DROP_PAYLOAD: {
            const char *type = JS_ToCString(ctx, argv[0]);
            int32_t flags = 0;
            if (argc >= 2) JS_ToInt32(ctx, &flags, argv[1]);
            ret = js_imgui_payload_wrap(
                    ctx, new ImGuiPayload(*ImGui::AcceptDragDropPayload(type, flags)));
            JS_FreeCString(ctx, type);
            break;
        }
        case IMGUI_END_DRAG_DROP_TARGET: {
            ImGui::EndDragDropTarget();
            break;
        }
        case IMGUI_GET_DRAG_DROP_PAYLOAD: {
            ret = js_imgui_payload_wrap(ctx, new ImGuiPayload(*ImGui::GetDragDropPayload()));
            break;
        }
        case IMGUI_BEGIN_DISABLED: {
            bool disabled = true;
            if (argc >= 1) disabled = JS_ToBool(ctx, argv[0]);
            ImGui::BeginDisabled(disabled);
            break;
        }
        case IMGUI_END_DISABLED: {
            ImGui::EndDisabled();
            break;
        }
        case IMGUI_PUSH_CLIP_RECT: {
            ImVec2 clip_rect_min = js_imgui_getimvec2(ctx, argv[0]),
                   clip_rect_max = js_imgui_getimvec2(ctx, argv[1]);
            bool intersect_with_current_clip_rect = JS_ToBool(ctx, argv[2]);
            ImGui::PushClipRect(clip_rect_min, clip_rect_max, intersect_with_current_clip_rect);
            break;
        }
        case IMGUI_POP_CLIP_RECT: {
            ImGui::PopClipRect();
            break;
        }
        case IMGUI_SET_ITEM_DEFAULT_FOCUS: {
            ImGui::SetItemDefaultFocus();
            break;
        }
        case IMGUI_SET_KEYBOARD_FOCUS_HERE: {
            int32_t offset = 0;
            if (argc >= 1) JS_ToInt32(ctx, &offset, argv[0]);
            ImGui::SetKeyboardFocusHere(offset);
            break;
        }
        case IMGUI_IS_ITEM_HOVERED: {
            int32_t flags = 0;
            if (argc >= 1) JS_ToInt32(ctx, &flags, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsItemHovered(flags));
            break;
        }
        case IMGUI_IS_ITEM_ACTIVE: {
            ret = JS_NewBool(ctx, ImGui::IsItemActive());
            break;
        }
        case IMGUI_IS_ITEM_FOCUSED: {
            ret = JS_NewBool(ctx, ImGui::IsItemFocused());
            break;
        }
        case IMGUI_IS_ITEM_CLICKED: {
            int32_t mouse_button = 0;
            if (argc >= 1) JS_ToInt32(ctx, &mouse_button, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsItemClicked(mouse_button));
            break;
        }
        case IMGUI_IS_ITEM_VISIBLE: {
            ret = JS_NewBool(ctx, ImGui::IsItemVisible());
            break;
        }
        case IMGUI_IS_ITEM_EDITED: {
            ret = JS_NewBool(ctx, ImGui::IsItemEdited());
            break;
        }
        case IMGUI_IS_ITEM_ACTIVATED: {
            ret = JS_NewBool(ctx, ImGui::IsItemActivated());
            break;
        }
        case IMGUI_IS_ITEM_DEACTIVATED: {
            ret = JS_NewBool(ctx, ImGui::IsItemDeactivated());
            break;
        }
        case IMGUI_IS_ITEM_DEACTIVATED_AFTER_EDIT: {
            ret = JS_NewBool(ctx, ImGui::IsItemDeactivatedAfterEdit());
            break;
        }
        case IMGUI_IS_ANY_ITEM_HOVERED: {
            ret = JS_NewBool(ctx, ImGui::IsAnyItemHovered());
            break;
        }
        case IMGUI_IS_ANY_ITEM_ACTIVE: {
            ret = JS_NewBool(ctx, ImGui::IsAnyItemActive());
            break;
        }
        case IMGUI_IS_ANY_ITEM_FOCUSED: {
            ret = JS_NewBool(ctx, ImGui::IsAnyItemFocused());
            break;
        }
        case IMGUI_GET_ITEM_RECT_MIN: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetItemRectMin());
            break;
        }
        case IMGUI_GET_ITEM_RECT_MAX: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetItemRectMax());
            break;
        }
        case IMGUI_GET_ITEM_RECT_SIZE: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetItemRectSize());
            break;
        }
        case IMGUI_SET_ITEM_ALLOW_OVERLAP: {
            ImGui::SetItemAllowOverlap();
            break;
        }
        case IMGUI_IS_RECT_VISIBLE: {
            if (argc >= 2) {
                ImVec2 rect_min = js_imgui_getimvec2(ctx, argv[0]),
                       rect_max = js_imgui_getimvec2(ctx, argv[1]);
                ret = JS_NewBool(ctx, ImGui::IsRectVisible(rect_min, rect_max));
            } else {
                ImVec2 size = js_imgui_getimvec2(ctx, argv[0]);
                ret = JS_NewBool(ctx, ImGui::IsRectVisible(size));
            }
            break;
        }
        case IMGUI_GET_TIME: {
            ret = JS_NewFloat64(ctx, ImGui::GetTime());
            break;
        }
        case IMGUI_GET_FRAME_COUNT: {
            ret = JS_NewInt64(ctx, ImGui::GetFrameCount());
            break;
        }
        case IMGUI_GET_BACKGROUND_DRAW_LIST:
            break;
        case IMGUI_GET_FOREGROUND_DRAW_LIST:
            break;
        case IMGUI_GET_DRAW_LIST_SHARED_DATA:
            break;
        case IMGUI_GET_STYLE_COLOR_NAME: {
            int32_t idx;
            JS_ToInt32(ctx, &idx, argv[0]);
            ret = JS_NewString(ctx, ImGui::GetStyleColorName(idx));
            break;
        }
        case IMGUI_SET_STATE_STORAGE:
            break;
        case IMGUI_GET_STATE_STORAGE:
            break;
        case IMGUI_CALC_TEXT_SIZE: {
            const char *text = JS_ToCString(ctx, argv[0]), *text_end = 0;
            bool hide_text_after_double_hash = false;
            double wrap_width = -1.0f;
            if (argc >= 2) text_end = JS_ToCString(ctx, argv[1]);
            if (argc >= 3) hide_text_after_double_hash = JS_ToBool(ctx, argv[2]);
            if (argc >= 4) JS_ToFloat64(ctx, &wrap_width, argv[3]);
            ret = js_imgui_newimvec2(
                    ctx,
                    ImGui::CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width));
            JS_FreeCString(ctx, text);
            if (text_end) JS_FreeCString(ctx, text_end);
            break;
        }
        case IMGUI_CALC_LIST_CLIPPING:
            break;
        case IMGUI_BEGIN_CHILD_FRAME: {
            int32_t id, window_flags = 0;
            ImVec2 size = js_imgui_getimvec2(ctx, argv[1]);
            JS_ToInt32(ctx, &id, argv[0]);
            if (argc >= 3) JS_ToInt32(ctx, &window_flags, argv[2]);
            ret = JS_NewBool(ctx, ImGui::BeginChildFrame(id, size, window_flags));
            break;
        }
        case IMGUI_END_CHILD_FRAME: {
            ImGui::EndChildFrame();
            break;
        }
        case IMGUI_COLOR_CONVERTU32_TO_FLOAT4: {
            uint32_t in;
            JS_ToUint32(ctx, &in, argv[0]);
            ret = js_imgui_newimvec4(ctx, ImGui::ColorConvertU32ToFloat4(in));
            break;
        }
        case IMGUI_COLOR_CONVERT_FLOAT4_TOU32: {
            ImVec4 in = js_imgui_getimvec4(ctx, argv[0]);
            ret = JS_NewUint32(ctx, ImGui::ColorConvertFloat4ToU32(in));
            break;
        }
        case IMGUI_COLOR_CONVERT_RGB_TO_HSV: {
            double r, g, b;
            float h, s, v;
            JS_ToFloat64(ctx, &r, argv[0]);
            JS_ToFloat64(ctx, &g, argv[1]);
            JS_ToFloat64(ctx, &b, argv[2]);
            ImGui::ColorConvertRGBtoHSV(r, g, b, h, s, v);
            ret = JS_NewArray(ctx);
            JS_SetPropertyUint32(ctx, ret, 0, JS_NewFloat64(ctx, h));
            JS_SetPropertyUint32(ctx, ret, 1, JS_NewFloat64(ctx, s));
            JS_SetPropertyUint32(ctx, ret, 2, JS_NewFloat64(ctx, v));
            break;
        }
        case IMGUI_COLOR_CONVERT_HSV_TO_RGB: {
            double h, s, v;
            float r, g, b;
            JS_ToFloat64(ctx, &h, argv[0]);
            JS_ToFloat64(ctx, &s, argv[1]);
            JS_ToFloat64(ctx, &v, argv[2]);
            ImGui::ColorConvertHSVtoRGB(h, s, v, r, g, b);
            ret = JS_NewArray(ctx);
            JS_SetPropertyUint32(ctx, ret, 0, JS_NewFloat64(ctx, r));
            JS_SetPropertyUint32(ctx, ret, 1, JS_NewFloat64(ctx, g));
            JS_SetPropertyUint32(ctx, ret, 2, JS_NewFloat64(ctx, b));
            break;
        }
        case IMGUI_GET_KEY_INDEX: {
            int32_t imgui_key;
            JS_ToInt32(ctx, &imgui_key, argv[0]);
            ret = JS_NewInt32(ctx, ImGui::GetKeyIndex((ImGuiKey) imgui_key));
            break;
        }
        case IMGUI_IS_KEY_DOWN: {
            int32_t user_key_index;
            JS_ToInt32(ctx, &user_key_index, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsKeyDown((ImGuiKey) user_key_index));
            break;
        }
        case IMGUI_IS_KEY_PRESSED: {
            int32_t user_key_index;
            bool repeat = true;
            if (argc >= 2) repeat = JS_ToBool(ctx, argv[1]);
            JS_ToInt32(ctx, &user_key_index, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsKeyPressed((ImGuiKey) user_key_index, repeat));
            break;
        }
        case IMGUI_IS_KEY_RELEASED: {
            int32_t user_key_index;
            JS_ToInt32(ctx, &user_key_index, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsKeyReleased((ImGuiKey) user_key_index));
            break;
        }
        case IMGUI_GET_KEY_PRESSED_AMOUNT: {
            int32_t key_index;
            double repeat_delay, rate;
            JS_ToInt32(ctx, &key_index, argv[0]);
            JS_ToFloat64(ctx, &repeat_delay, argv[1]);
            JS_ToFloat64(ctx, &rate, argv[2]);
            ret = JS_NewInt32(ctx,
                              ImGui::GetKeyPressedAmount((ImGuiKey) key_index, repeat_delay, rate));
            break;
        }
        case IMGUI_IS_MOUSE_DOWN: {
            int32_t button;
            JS_ToInt32(ctx, &button, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsMouseDown(button));
            break;
        }
        case IMGUI_IS_ANY_MOUSE_DOWN: {
            ret = JS_NewBool(ctx, ImGui::IsAnyMouseDown());
            break;
        }
        case IMGUI_IS_MOUSE_CLICKED: {
            int32_t button;
            bool repeat = true;
            if (argc >= 2) repeat = JS_ToBool(ctx, argv[1]);
            JS_ToInt32(ctx, &button, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsMouseClicked(button, repeat));
            break;
        }
        case IMGUI_IS_MOUSE_DOUBLE_CLICKED: {
            int32_t button;
            JS_ToInt32(ctx, &button, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsMouseDoubleClicked(button));
            break;
        }
        case IMGUI_IS_MOUSE_RELEASED: {
            int32_t button;
            JS_ToInt32(ctx, &button, argv[0]);
            ret = JS_NewBool(ctx, ImGui::IsMouseReleased(button));
            break;
        }
        case IMGUI_IS_MOUSE_DRAGGING: {
            int32_t button;
            double lock_threshold = -1.0;
            JS_ToInt32(ctx, &button, argv[0]);
            if (argc >= 2) JS_ToFloat64(ctx, &lock_threshold, argv[1]);
            ret = JS_NewBool(ctx, ImGui::IsMouseDragging(button, lock_threshold));
            break;
        }
        case IMGUI_IS_MOUSE_HOVERING_RECT: {
            ImVec2 r_min = js_imgui_getimvec2(ctx, argv[0]),
                   r_max = js_imgui_getimvec2(ctx, argv[1]);
            bool clip = true;
            if (argc >= 3) clip = JS_ToBool(ctx, argv[2]);
            ImGui::IsMouseHoveringRect(r_min, r_max, clip);
            break;
        }
        case IMGUI_IS_MOUSE_POS_VALID: {
            const ImVec2 *ptr = 0;
            ImVec2 pos;
            if (argc >= 1) {
                pos = js_imgui_getimvec2(ctx, argv[0]);
                ptr = &pos;
            }
            ret = JS_NewBool(ctx, ImGui::IsMousePosValid(ptr));
            break;
        }
        case IMGUI_GET_MOUSE_POS: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetMousePos());
            break;
        }
        case IMGUI_GET_MOUSE_POS_ON_OPENING_CURRENT_POPUP: {
            ret = js_imgui_newimvec2(ctx, ImGui::GetMousePosOnOpeningCurrentPopup());
            break;
        }
        case IMGUI_GET_MOUSE_DRAG_DELTA: {
            int32_t mouse_button = 0;
            double lock_threshold = -1.0;
            if (argc >= 1) JS_ToInt32(ctx, &mouse_button, argv[0]);
            if (argc >= 2) JS_ToFloat64(ctx, &lock_threshold, argv[1]);
            ret = js_imgui_newimvec2(ctx, ImGui::GetMouseDragDelta(mouse_button, lock_threshold));
            break;
        }
        case IMGUI_RESET_MOUSE_DRAG_DELTA: {
            int32_t mouse_button = 0;
            if (argc >= 1) JS_ToInt32(ctx, &mouse_button, argv[0]);
            ImGui::ResetMouseDragDelta(mouse_button);
            break;
        }
        case IMGUI_GET_MOUSE_CURSOR: {
            ret = JS_NewInt32(ctx, ImGui::GetMouseCursor());
            break;
        }
        case IMGUI_SET_MOUSE_CURSOR: {
            int32_t mouse_cursor;
            JS_ToInt32(ctx, &mouse_cursor, argv[0]);
            ImGui::SetMouseCursor(mouse_cursor);
            break;
        }
        case IMGUI_CAPTURE_KEYBOARD_FROM_APP: {
            bool want_capture = true;
            if (argc >= 1) want_capture = JS_ToBool(ctx, argv[0]);
            ImGui::CaptureKeyboardFromApp(want_capture);
            break;
        }
        case IMGUI_CAPTURE_MOUSE_FROM_APP: {
            bool want_capture = true;
            if (argc >= 1) want_capture = JS_ToBool(ctx, argv[0]);
            ImGui::CaptureMouseFromApp(want_capture);
            break;
        }
        case IMGUI_GET_CLIPBOARD_TEXT: {
            ret = JS_NewString(ctx, ImGui::GetClipboardText());
            break;
        }
        case IMGUI_SET_CLIPBOARD_TEXT: {
            const char *text = JS_ToCString(ctx, argv[0]);
            ImGui::SetClipboardText(text);
            JS_FreeCString(ctx, text);

            break;
        }
        case IMGUI_LOAD_INI_SETTINGS_FROM_DISK: {
            const char *file = JS_ToCString(ctx, argv[0]);
            ImGui::LoadIniSettingsFromDisk(file);
            JS_FreeCString(ctx, file);
            break;
        }
        case IMGUI_LOAD_INI_SETTINGS_FROM_MEMORY: {
            uint8_t *ptr;
            size_t len;
            if (JS_IsString(argv[0])) {
                const char *data = JS_ToCStringLen(ctx, &len, argv[0]);
                ImGui::LoadIniSettingsFromMemory(data, len);
                JS_FreeCString(ctx, data);
            } else if ((ptr = JS_GetArrayBuffer(ctx, &len, argv[0]))) {
                ImGui::LoadIniSettingsFromMemory(reinterpret_cast<const char *>(ptr), len);
            } else {
                ret = JS_ThrowInternalError(ctx, "argument 1 is neither String nor ArrayBuffer");
            }
            break;
        }
        case IMGUI_SAVE_INI_SETTINGS_TO_DISK: {
            const char *file = JS_ToCString(ctx, argv[0]);
            ImGui::SaveIniSettingsToDisk(file);
            JS_FreeCString(ctx, file);
            break;
        }
        case IMGUI_SAVE_INI_SETTINGS_TO_MEMORY: {
            size_t len;
            const char *data = ImGui::SaveIniSettingsToMemory(&len);

            ret = JS_NewArrayBufferCopy(ctx, reinterpret_cast<const uint8_t *>(data), len);
            break;
        }
        case IMGUI_SET_ALLOCATOR_FUNCTIONS:
            break;
        case IMGUI_MEM_ALLOC: {
            uint64_t len;
            void *data;
            JS_ToIndex(ctx, &len, argv[0]);
            data = ImGui::MemAlloc(len);
            ret = JS_NewArrayBuffer(ctx, static_cast<uint8_t *>(data), len, js_imgui_free_func,
                                    (void *) len, _FALSE);
            break;
        }
        case IMGUI_MEM_FREE: {
            JS_DetachArrayBuffer(ctx, argv[0]);
            break;
        }
    }

    return ret;
}

typedef enum {
    PROPERTY = 0,
    CALL = 1,
    INVOKE = 2,
    GETSET = 3
} pointer_closure_type;

struct imgui_pointer_closure
{
    int ref_count;
    JSContext *ctx;
    union {
        JSValue obj;
        JSValue fns[2];
    };
    JSAtom prop;
    pointer_closure_type type;
};

static const JSCFunctionListEntry js_imgui_static_funcs[] = {
        JS_CFUNC_MAGIC_DEF("GetIO", 0, js_imgui_functions, IMGUI_GET_IO),
        JS_CFUNC_MAGIC_DEF("GetStyle", 0, js_imgui_functions, IMGUI_GET_STYLE),
        JS_CFUNC_MAGIC_DEF("ShowDemoWindow", 1, js_imgui_functions, IMGUI_SHOW_DEMO_WINDOW),
        JS_CFUNC_MAGIC_DEF("ShowAboutWindow", 1, js_imgui_functions, IMGUI_SHOW_ABOUT_WINDOW),
        JS_CFUNC_MAGIC_DEF("ShowMetricsWindow", 1, js_imgui_functions, IMGUI_SHOW_METRICS_WINDOW),
        JS_CFUNC_MAGIC_DEF("ShowStyleEditor", 1, js_imgui_functions, IMGUI_SHOW_STYLE_EDITOR),
        JS_CFUNC_MAGIC_DEF("ShowStyleSelector", 1, js_imgui_functions, IMGUI_SHOW_STYLE_SELECTOR),
        JS_CFUNC_MAGIC_DEF("ShowFontSelector", 1, js_imgui_functions, IMGUI_SHOW_FONT_SELECTOR),
        JS_CFUNC_MAGIC_DEF("ShowUserGuide", 0, js_imgui_functions, IMGUI_SHOW_USER_GUIDE),
        JS_CFUNC_MAGIC_DEF("GetVersion", 0, js_imgui_functions, IMGUI_GET_VERSION),
        JS_CFUNC_MAGIC_DEF("StyleColorsDark", 1, js_imgui_functions, IMGUI_STYLE_COLORS_DARK),
        JS_CFUNC_MAGIC_DEF("StyleColorsClassic", 1, js_imgui_functions, IMGUI_STYLE_COLORS_CLASSIC),
        JS_CFUNC_MAGIC_DEF("StyleColorsLight", 1, js_imgui_functions, IMGUI_STYLE_COLORS_LIGHT),
        JS_CFUNC_MAGIC_DEF("Begin", 3, js_imgui_functions, IMGUI_BEGIN),
        JS_CFUNC_MAGIC_DEF("End", 0, js_imgui_functions, IMGUI_END),
        JS_CFUNC_MAGIC_DEF("BeginChild", 4, js_imgui_functions, IMGUI_BEGIN_CHILD),
        JS_CFUNC_MAGIC_DEF("EndChild", 0, js_imgui_functions, IMGUI_END_CHILD),
        JS_CFUNC_MAGIC_DEF("IsWindowAppearing", 0, js_imgui_functions, IMGUI_IS_WINDOW_APPEARING),
        JS_CFUNC_MAGIC_DEF("IsWindowCollapsed", 0, js_imgui_functions, IMGUI_IS_WINDOW_COLLAPSED),
        JS_CFUNC_MAGIC_DEF("IsWindowFocused", 1, js_imgui_functions, IMGUI_IS_WINDOW_FOCUSED),
        JS_CFUNC_MAGIC_DEF("IsWindowHovered", 1, js_imgui_functions, IMGUI_IS_WINDOW_HOVERED),
        JS_CFUNC_MAGIC_DEF("GetWindowDrawList", 0, js_imgui_functions, IMGUI_GET_WINDOW_DRAW_LIST),
        JS_CFUNC_MAGIC_DEF("GetWindowPos", 0, js_imgui_functions, IMGUI_GET_WINDOW_POS),
        JS_CFUNC_MAGIC_DEF("GetWindowSize", 0, js_imgui_functions, IMGUI_GET_WINDOW_SIZE),
        JS_CFUNC_MAGIC_DEF("GetWindowWidth", 0, js_imgui_functions, IMGUI_GET_WINDOW_WIDTH),
        JS_CFUNC_MAGIC_DEF("GetWindowHeight", 0, js_imgui_functions, IMGUI_GET_WINDOW_HEIGHT),
        JS_CFUNC_MAGIC_DEF("GetContentRegionMax", 0, js_imgui_functions,
                           IMGUI_GET_CONTENT_REGION_MAX),
        JS_CFUNC_MAGIC_DEF("GetContentRegionAvail", 0, js_imgui_functions,
                           IMGUI_GET_CONTENT_REGION_AVAIL),
        JS_CFUNC_MAGIC_DEF("GetWindowContentRegionMin", 0, js_imgui_functions,
                           IMGUI_GET_WINDOW_CONTENT_REGION_MIN),
        JS_CFUNC_MAGIC_DEF("GetWindowContentRegionMax", 0, js_imgui_functions,
                           IMGUI_GET_WINDOW_CONTENT_REGION_MAX),
        JS_CFUNC_MAGIC_DEF("SetNextWindowPos", 3, js_imgui_functions, IMGUI_SET_NEXT_WINDOW_POS),
        JS_CFUNC_MAGIC_DEF("SetNextWindowSize", 2, js_imgui_functions, IMGUI_SET_NEXT_WINDOW_SIZE),
        JS_CFUNC_MAGIC_DEF("SetNextWindowSizeConstraints", 4, js_imgui_functions,
                           IMGUI_SET_NEXT_WINDOW_SIZE_CONSTRAINTS),
        JS_CFUNC_MAGIC_DEF("SetNextWindowContentSize", 1, js_imgui_functions,
                           IMGUI_SET_NEXT_WINDOW_CONTENT_SIZE),
        JS_CFUNC_MAGIC_DEF("SetNextWindowCollapsed", 2, js_imgui_functions,
                           IMGUI_SET_NEXT_WINDOW_COLLAPSED),
        JS_CFUNC_MAGIC_DEF("SetNextWindowFocus", 0, js_imgui_functions,
                           IMGUI_SET_NEXT_WINDOW_FOCUS),
        JS_CFUNC_MAGIC_DEF("SetNextWindowBgAlpha", 1, js_imgui_functions,
                           IMGUI_SET_NEXT_WINDOW_BG_ALPHA),
        JS_CFUNC_MAGIC_DEF("SetWindowPos", 3, js_imgui_functions, IMGUI_SET_WINDOW_POS),
        JS_CFUNC_MAGIC_DEF("SetWindowSize", 3, js_imgui_functions, IMGUI_SET_WINDOW_SIZE),
        JS_CFUNC_MAGIC_DEF("SetWindowCollapsed", 3, js_imgui_functions, IMGUI_SET_WINDOW_COLLAPSED),
        JS_CFUNC_MAGIC_DEF("SetWindowFocus", 1, js_imgui_functions, IMGUI_SET_WINDOW_FOCUS),
        JS_CFUNC_MAGIC_DEF("SetWindowFontScale", 1, js_imgui_functions,
                           IMGUI_SET_WINDOW_FONT_SCALE),
        JS_CFUNC_MAGIC_DEF("GetScrollX", 0, js_imgui_functions, IMGUI_GET_SCROLL_X),
        JS_CFUNC_MAGIC_DEF("GetScrollY", 0, js_imgui_functions, IMGUI_GET_SCROLL_Y),
        JS_CFUNC_MAGIC_DEF("GetScrollMaxX", 0, js_imgui_functions, IMGUI_GET_SCROLL_MAX_X),
        JS_CFUNC_MAGIC_DEF("GetScrollMaxY", 0, js_imgui_functions, IMGUI_GET_SCROLL_MAX_Y),
        JS_CFUNC_MAGIC_DEF("SetScrollX", 1, js_imgui_functions, IMGUI_SET_SCROLL_X),
        JS_CFUNC_MAGIC_DEF("SetScrollY", 1, js_imgui_functions, IMGUI_SET_SCROLL_Y),
        JS_CFUNC_MAGIC_DEF("SetScrollHereY", 1, js_imgui_functions, IMGUI_SET_SCROLL_HERE_Y),
        JS_CFUNC_MAGIC_DEF("SetScrollFromPosY", 2, js_imgui_functions, IMGUI_SET_SCROLL_FROM_POS_Y),
        JS_CFUNC_MAGIC_DEF("PushFont", 1, js_imgui_functions, IMGUI_PUSH_FONT),
        JS_CFUNC_MAGIC_DEF("PopFont", 0, js_imgui_functions, IMGUI_POP_FONT),
        JS_CFUNC_MAGIC_DEF("PushStyleColor", 2, js_imgui_functions, IMGUI_PUSH_STYLE_COLOR),
        JS_CFUNC_MAGIC_DEF("PopStyleColor", 1, js_imgui_functions, IMGUI_POP_STYLE_COLOR),
        JS_CFUNC_MAGIC_DEF("PushStyleVar", 2, js_imgui_functions, IMGUI_PUSH_STYLE_VAR),
        JS_CFUNC_MAGIC_DEF("PopStyleVar", 1, js_imgui_functions, IMGUI_POP_STYLE_VAR),
        JS_CFUNC_MAGIC_DEF("GetStyleColorVec4", 1, js_imgui_functions, IMGUI_GET_STYLE_COLOR_VEC4),
        JS_CFUNC_MAGIC_DEF("GetFont", 0, js_imgui_functions, IMGUI_GET_FONT),
        JS_CFUNC_MAGIC_DEF("GetFontSize", 0, js_imgui_functions, IMGUI_GET_FONT_SIZE),
        JS_CFUNC_MAGIC_DEF("GetFontTexUvWhitePixel", 0, js_imgui_functions,
                           IMGUI_GET_FONT_TEX_UV_WHITE_PIXEL),
        JS_CFUNC_MAGIC_DEF("GetColorU32", 1, js_imgui_functions, IMGUI_GET_COLORU32),
        JS_CFUNC_MAGIC_DEF("PushItemWidth", 1, js_imgui_functions, IMGUI_PUSH_ITEM_WIDTH),
        JS_CFUNC_MAGIC_DEF("PopItemWidth", 0, js_imgui_functions, IMGUI_POP_ITEM_WIDTH),
        JS_CFUNC_MAGIC_DEF("CalcItemWidth", 0, js_imgui_functions, IMGUI_CALC_ITEM_WIDTH),
        JS_CFUNC_MAGIC_DEF("PushTextWrapPos", 1, js_imgui_functions, IMGUI_PUSH_TEXT_WRAP_POS),
        JS_CFUNC_MAGIC_DEF("PopTextWrapPos", 0, js_imgui_functions, IMGUI_POP_TEXT_WRAP_POS),
        JS_CFUNC_MAGIC_DEF("PushAllowKeyboardFocus", 1, js_imgui_functions,
                           IMGUI_PUSH_ALLOW_KEYBOARD_FOCUS),
        JS_CFUNC_MAGIC_DEF("PopAllowKeyboardFocus", 0, js_imgui_functions,
                           IMGUI_POP_ALLOW_KEYBOARD_FOCUS),
        JS_CFUNC_MAGIC_DEF("PushButtonRepeat", 1, js_imgui_functions, IMGUI_PUSH_BUTTON_REPEAT),
        JS_CFUNC_MAGIC_DEF("PopButtonRepeat", 0, js_imgui_functions, IMGUI_POP_BUTTON_REPEAT),
        JS_CFUNC_MAGIC_DEF("Separator", 0, js_imgui_functions, IMGUI_SEPARATOR),
        JS_CFUNC_MAGIC_DEF("SameLine", 2, js_imgui_functions, IMGUI_SAME_LINE),
        JS_CFUNC_MAGIC_DEF("NewLine", 0, js_imgui_functions, IMGUI_NEW_LINE),
        JS_CFUNC_MAGIC_DEF("Spacing", 0, js_imgui_functions, IMGUI_SPACING),
        JS_CFUNC_MAGIC_DEF("Dummy", 1, js_imgui_functions, IMGUI_DUMMY),
        JS_CFUNC_MAGIC_DEF("Indent", 1, js_imgui_functions, IMGUI_INDENT),
        JS_CFUNC_MAGIC_DEF("Unindent", 1, js_imgui_functions, IMGUI_UNINDENT),
        JS_CFUNC_MAGIC_DEF("BeginGroup", 0, js_imgui_functions, IMGUI_BEGIN_GROUP),
        JS_CFUNC_MAGIC_DEF("EndGroup", 0, js_imgui_functions, IMGUI_END_GROUP),
        JS_CFUNC_MAGIC_DEF("GetCursorPos", 0, js_imgui_functions, IMGUI_GET_CURSOR_POS),
        JS_CFUNC_MAGIC_DEF("GetCursorPosX", 0, js_imgui_functions, IMGUI_GET_CURSOR_POS_X),
        JS_CFUNC_MAGIC_DEF("GetCursorPosY", 0, js_imgui_functions, IMGUI_GET_CURSOR_POS_Y),
        JS_CFUNC_MAGIC_DEF("SetCursorPos", 1, js_imgui_functions, IMGUI_SET_CURSOR_POS),
        JS_CFUNC_MAGIC_DEF("SetCursorPosX", 1, js_imgui_functions, IMGUI_SET_CURSOR_POS_X),
        JS_CFUNC_MAGIC_DEF("SetCursorPosY", 1, js_imgui_functions, IMGUI_SET_CURSOR_POS_Y),
        JS_CFUNC_MAGIC_DEF("GetCursorStartPos", 0, js_imgui_functions, IMGUI_GET_CURSOR_START_POS),
        JS_CFUNC_MAGIC_DEF("GetCursorScreenPos", 0, js_imgui_functions,
                           IMGUI_GET_CURSOR_SCREEN_POS),
        JS_CFUNC_MAGIC_DEF("SetCursorScreenPos", 1, js_imgui_functions,
                           IMGUI_SET_CURSOR_SCREEN_POS),
        JS_CFUNC_MAGIC_DEF("AlignTextToFramePadding", 0, js_imgui_functions,
                           IMGUI_ALIGN_TEXT_TO_FRAME_PADDING),
        JS_CFUNC_MAGIC_DEF("GetTextLineHeight", 0, js_imgui_functions, IMGUI_GET_TEXT_LINE_HEIGHT),
        JS_CFUNC_MAGIC_DEF("GetTextLineHeightWithSpacing", 0, js_imgui_functions,
                           IMGUI_GET_TEXT_LINE_HEIGHT_WITH_SPACING),
        JS_CFUNC_MAGIC_DEF("GetFrameHeight", 0, js_imgui_functions, IMGUI_GET_FRAME_HEIGHT),
        JS_CFUNC_MAGIC_DEF("GetFrameHeightWithSpacing", 0, js_imgui_functions,
                           IMGUI_GET_FRAME_HEIGHT_WITH_SPACING),
        JS_CFUNC_MAGIC_DEF("PushID", 1, js_imgui_functions, IMGUI_PUSH_ID),
        JS_CFUNC_MAGIC_DEF("PopID", 0, js_imgui_functions, IMGUI_POP_ID),
        JS_CFUNC_MAGIC_DEF("GetID", 1, js_imgui_functions, IMGUI_GET_ID),
        JS_CFUNC_MAGIC_DEF("TextUnformatted", 2, js_imgui_functions, IMGUI_TEXT_UNFORMATTED),
        JS_CFUNC_MAGIC_DEF("Text", 1, js_imgui_functions, IMGUI_TEXT),
        JS_CFUNC_MAGIC_DEF("TextColored", 2, js_imgui_functions, IMGUI_TEXT_COLORED),
        JS_CFUNC_MAGIC_DEF("TextDisabled", 1, js_imgui_functions, IMGUI_TEXT_DISABLED),
        JS_CFUNC_MAGIC_DEF("TextWrapped", 1, js_imgui_functions, IMGUI_TEXT_WRAPPED),
        JS_CFUNC_MAGIC_DEF("LabelText", 2, js_imgui_functions, IMGUI_LABEL_TEXT),
        JS_CFUNC_MAGIC_DEF("BulletText", 1, js_imgui_functions, IMGUI_BULLET_TEXT),
        JS_CFUNC_MAGIC_DEF("Button", 2, js_imgui_functions, IMGUI_BUTTON),
        JS_CFUNC_MAGIC_DEF("SmallButton", 1, js_imgui_functions, IMGUI_SMALL_BUTTON),
        JS_CFUNC_MAGIC_DEF("InvisibleButton", 2, js_imgui_functions, IMGUI_INVISIBLE_BUTTON),
        JS_CFUNC_MAGIC_DEF("ArrowButton", 2, js_imgui_functions, IMGUI_ARROW_BUTTON),
        JS_CFUNC_MAGIC_DEF("Image", 6, js_imgui_functions, IMGUI_IMAGE),
        JS_CFUNC_MAGIC_DEF("ImageButton", 7, js_imgui_functions, IMGUI_IMAGE_BUTTON),
        JS_CFUNC_MAGIC_DEF("Checkbox", 2, js_imgui_functions, IMGUI_CHECKBOX),
        JS_CFUNC_MAGIC_DEF("CheckboxFlags", 3, js_imgui_functions, IMGUI_CHECKBOX_FLAGS),
        JS_CFUNC_MAGIC_DEF("RadioButton", 3, js_imgui_functions, IMGUI_RADIO_BUTTON),
        JS_CFUNC_MAGIC_DEF("ProgressBar", 3, js_imgui_functions, IMGUI_PROGRESS_BAR),
        JS_CFUNC_MAGIC_DEF("Bullet", 0, js_imgui_functions, IMGUI_BULLET),
        JS_CFUNC_MAGIC_DEF("BeginCombo", 3, js_imgui_functions, IMGUI_BEGIN_COMBO),
        JS_CFUNC_MAGIC_DEF("EndCombo", 0, js_imgui_functions, IMGUI_END_COMBO),
        JS_CFUNC_MAGIC_DEF("Combo", 6, js_imgui_functions, IMGUI_COMBO),
        JS_CFUNC_MAGIC_DEF("DragFloat", 7, js_imgui_functions, IMGUI_DRAG_FLOAT),
        JS_CFUNC_MAGIC_DEF("DragFloat2", 7, js_imgui_functions, IMGUI_DRAG_FLOAT2),
        JS_CFUNC_MAGIC_DEF("DragFloat3", 7, js_imgui_functions, IMGUI_DRAG_FLOAT3),
        JS_CFUNC_MAGIC_DEF("DragFloat4", 7, js_imgui_functions, IMGUI_DRAG_FLOAT4),
        JS_CFUNC_MAGIC_DEF("DragFloatRange2", 9, js_imgui_functions, IMGUI_DRAG_FLOAT_RANGE2),
        JS_CFUNC_MAGIC_DEF("DragInt", 6, js_imgui_functions, IMGUI_DRAG_INT),
        JS_CFUNC_MAGIC_DEF("DragInt2", 6, js_imgui_functions, IMGUI_DRAG_INT2),
        JS_CFUNC_MAGIC_DEF("DragInt3", 6, js_imgui_functions, IMGUI_DRAG_INT3),
        JS_CFUNC_MAGIC_DEF("DragInt4", 6, js_imgui_functions, IMGUI_DRAG_INT4),
        JS_CFUNC_MAGIC_DEF("DragIntRange2", 8, js_imgui_functions, IMGUI_DRAG_INT_RANGE2),
        JS_CFUNC_MAGIC_DEF("DragScalar", 8, js_imgui_functions, IMGUI_DRAG_SCALAR),
        JS_CFUNC_MAGIC_DEF("DragScalarN", 9, js_imgui_functions, IMGUI_DRAG_SCALAR_N),
        JS_CFUNC_MAGIC_DEF("SliderFloat", 6, js_imgui_functions, IMGUI_SLIDER_FLOAT),
        JS_CFUNC_MAGIC_DEF("SliderFloat2", 6, js_imgui_functions, IMGUI_SLIDER_FLOAT2),
        JS_CFUNC_MAGIC_DEF("SliderFloat3", 6, js_imgui_functions, IMGUI_SLIDER_FLOAT3),
        JS_CFUNC_MAGIC_DEF("SliderFloat4", 6, js_imgui_functions, IMGUI_SLIDER_FLOAT4),
        JS_CFUNC_MAGIC_DEF("SliderAngle", 5, js_imgui_functions, IMGUI_SLIDER_ANGLE),
        JS_CFUNC_MAGIC_DEF("SliderInt", 5, js_imgui_functions, IMGUI_SLIDER_INT),
        JS_CFUNC_MAGIC_DEF("SliderInt2", 5, js_imgui_functions, IMGUI_SLIDER_INT2),
        JS_CFUNC_MAGIC_DEF("SliderInt3", 5, js_imgui_functions, IMGUI_SLIDER_INT3),
        JS_CFUNC_MAGIC_DEF("SliderInt4", 5, js_imgui_functions, IMGUI_SLIDER_INT4),
        JS_CFUNC_MAGIC_DEF("SliderScalar", 7, js_imgui_functions, IMGUI_SLIDER_SCALAR),
        JS_CFUNC_MAGIC_DEF("SliderScalarN", 8, js_imgui_functions, IMGUI_SLIDER_SCALAR_N),
        JS_CFUNC_MAGIC_DEF("VSliderFloat", 7, js_imgui_functions, IMGUI_V_SLIDER_FLOAT),
        JS_CFUNC_MAGIC_DEF("VSliderInt", 6, js_imgui_functions, IMGUI_V_SLIDER_INT),
        JS_CFUNC_MAGIC_DEF("VSliderScalar", 8, js_imgui_functions, IMGUI_V_SLIDER_SCALAR),
        JS_CFUNC_MAGIC_DEF("InputText", 6, js_imgui_functions, IMGUI_INPUT_TEXT),
        JS_CFUNC_MAGIC_DEF("InputTextMultiline", 7, js_imgui_functions, IMGUI_INPUT_TEXT_MULTILINE),
        JS_CFUNC_MAGIC_DEF("InputTextWithHint", 7, js_imgui_functions, IMGUI_INPUT_TEXT_WITH_HINT),
        JS_CFUNC_MAGIC_DEF("InputFloat", 6, js_imgui_functions, IMGUI_INPUT_FLOAT),
        JS_CFUNC_MAGIC_DEF("InputFloat2", 4, js_imgui_functions, IMGUI_INPUT_FLOAT2),
        JS_CFUNC_MAGIC_DEF("InputFloat3", 4, js_imgui_functions, IMGUI_INPUT_FLOAT3),
        JS_CFUNC_MAGIC_DEF("InputFloat4", 4, js_imgui_functions, IMGUI_INPUT_FLOAT4),
        JS_CFUNC_MAGIC_DEF("InputInt", 5, js_imgui_functions, IMGUI_INPUT_INT),
        JS_CFUNC_MAGIC_DEF("InputInt2", 3, js_imgui_functions, IMGUI_INPUT_INT2),
        JS_CFUNC_MAGIC_DEF("InputInt3", 3, js_imgui_functions, IMGUI_INPUT_INT3),
        JS_CFUNC_MAGIC_DEF("InputInt4", 3, js_imgui_functions, IMGUI_INPUT_INT4),
        JS_CFUNC_MAGIC_DEF("InputDouble", 6, js_imgui_functions, IMGUI_INPUT_DOUBLE),
        JS_CFUNC_MAGIC_DEF("InputScalar", 7, js_imgui_functions, IMGUI_INPUT_SCALAR),
        JS_CFUNC_MAGIC_DEF("InputScalarN", 8, js_imgui_functions, IMGUI_INPUT_SCALAR_N),
        JS_CFUNC_MAGIC_DEF("ColorEdit3", 3, js_imgui_functions, IMGUI_COLOR_EDIT3),
        JS_CFUNC_MAGIC_DEF("ColorEdit4", 3, js_imgui_functions, IMGUI_COLOR_EDIT4),
        JS_CFUNC_MAGIC_DEF("ColorPicker3", 3, js_imgui_functions, IMGUI_COLOR_PICKER3),
        JS_CFUNC_MAGIC_DEF("ColorPicker4", 4, js_imgui_functions, IMGUI_COLOR_PICKER4),
        JS_CFUNC_MAGIC_DEF("ColorButton", 4, js_imgui_functions, IMGUI_COLOR_BUTTON),
        JS_CFUNC_MAGIC_DEF("SetColorEditOptions", 1, js_imgui_functions,
                           IMGUI_SET_COLOR_EDIT_OPTIONS),
        JS_CFUNC_MAGIC_DEF("TreeNode", 2, js_imgui_functions, IMGUI_TREE_NODE),
        JS_CFUNC_MAGIC_DEF("TreeNodeEx", 3, js_imgui_functions, IMGUI_TREE_NODE_EX),
        JS_CFUNC_MAGIC_DEF("TreePush", 1, js_imgui_functions, IMGUI_TREE_PUSH),
        JS_CFUNC_MAGIC_DEF("TreePop", 0, js_imgui_functions, IMGUI_TREE_POP),
        JS_CFUNC_MAGIC_DEF("GetTreeNodeToLabelSpacing", 0, js_imgui_functions,
                           IMGUI_GET_TREE_NODE_TO_LABEL_SPACING),
        JS_CFUNC_MAGIC_DEF("CollapsingHeader", 3, js_imgui_functions, IMGUI_COLLAPSING_HEADER),
        JS_CFUNC_MAGIC_DEF("Selectable", 4, js_imgui_functions, IMGUI_SELECTABLE),
        JS_CFUNC_MAGIC_DEF("ListBox", 6, js_imgui_functions, IMGUI_LIST_BOX),
        JS_CFUNC_MAGIC_DEF("ListBoxHeader", 3, js_imgui_functions, IMGUI_LIST_BOX_HEADER),
        JS_CFUNC_MAGIC_DEF("ListBoxFooter", 0, js_imgui_functions, IMGUI_LIST_BOX_FOOTER),
        JS_CFUNC_MAGIC_DEF("EndListBox", 0, js_imgui_functions, IMGUI_END_LIST_BOX),
        JS_CFUNC_MAGIC_DEF("PlotLines", 9, js_imgui_functions, IMGUI_PLOT_LINES),
        JS_CFUNC_MAGIC_DEF("PlotHistogram", 9, js_imgui_functions, IMGUI_PLOT_HISTOGRAM),
        JS_CFUNC_MAGIC_DEF("Value", 3, js_imgui_functions, IMGUI_VALUE),
        JS_CFUNC_MAGIC_DEF("BeginMainMenuBar", 0, js_imgui_functions, IMGUI_BEGIN_MAIN_MENU_BAR),
        JS_CFUNC_MAGIC_DEF("EndMainMenuBar", 0, js_imgui_functions, IMGUI_END_MAIN_MENU_BAR),
        JS_CFUNC_MAGIC_DEF("BeginMenuBar", 0, js_imgui_functions, IMGUI_BEGIN_MENU_BAR),
        JS_CFUNC_MAGIC_DEF("EndMenuBar", 0, js_imgui_functions, IMGUI_END_MENU_BAR),
        JS_CFUNC_MAGIC_DEF("BeginMenu", 2, js_imgui_functions, IMGUI_BEGIN_MENU),
        JS_CFUNC_MAGIC_DEF("EndMenu", 0, js_imgui_functions, IMGUI_END_MENU),
        JS_CFUNC_MAGIC_DEF("MenuItem", 4, js_imgui_functions, IMGUI_MENU_ITEM),
        JS_CFUNC_MAGIC_DEF("BeginTooltip", 0, js_imgui_functions, IMGUI_BEGIN_TOOLTIP),
        JS_CFUNC_MAGIC_DEF("EndTooltip", 0, js_imgui_functions, IMGUI_END_TOOLTIP),
        JS_CFUNC_MAGIC_DEF("SetTooltip", 1, js_imgui_functions, IMGUI_SET_TOOLTIP),
        JS_CFUNC_MAGIC_DEF("OpenPopup", 1, js_imgui_functions, IMGUI_OPEN_POPUP),
        JS_CFUNC_MAGIC_DEF("BeginPopup", 2, js_imgui_functions, IMGUI_BEGIN_POPUP),
        JS_CFUNC_MAGIC_DEF("BeginPopupContextItem", 2, js_imgui_functions,
                           IMGUI_BEGIN_POPUP_CONTEXT_ITEM),
        JS_CFUNC_MAGIC_DEF("BeginPopupContextWindow", 3, js_imgui_functions,
                           IMGUI_BEGIN_POPUP_CONTEXT_WINDOW),
        JS_CFUNC_MAGIC_DEF("BeginPopupContextVoid", 2, js_imgui_functions,
                           IMGUI_BEGIN_POPUP_CONTEXT_VOID),
        JS_CFUNC_MAGIC_DEF("BeginPopupModal", 3, js_imgui_functions, IMGUI_BEGIN_POPUP_MODAL),
        JS_CFUNC_MAGIC_DEF("EndPopup", 0, js_imgui_functions, IMGUI_END_POPUP),
        JS_CFUNC_MAGIC_DEF("OpenPopupOnItemClick", 2, js_imgui_functions,
                           IMGUI_OPEN_POPUP_ON_ITEM_CLICK),
        JS_CFUNC_MAGIC_DEF("IsPopupOpen", 1, js_imgui_functions, IMGUI_IS_POPUP_OPEN),
        JS_CFUNC_MAGIC_DEF("CloseCurrentPopup", 0, js_imgui_functions, IMGUI_CLOSE_CURRENT_POPUP),
        JS_CFUNC_MAGIC_DEF("Columns", 3, js_imgui_functions, IMGUI_COLUMNS),
        JS_CFUNC_MAGIC_DEF("NextColumn", 0, js_imgui_functions, IMGUI_NEXT_COLUMN),
        JS_CFUNC_MAGIC_DEF("GetColumnIndex", 0, js_imgui_functions, IMGUI_GET_COLUMN_INDEX),
        JS_CFUNC_MAGIC_DEF("GetColumnWidth", 1, js_imgui_functions, IMGUI_GET_COLUMN_WIDTH),
        JS_CFUNC_MAGIC_DEF("SetColumnWidth", 2, js_imgui_functions, IMGUI_SET_COLUMN_WIDTH),
        JS_CFUNC_MAGIC_DEF("GetColumnOffset", 1, js_imgui_functions, IMGUI_GET_COLUMN_OFFSET),
        JS_CFUNC_MAGIC_DEF("SetColumnOffset", 2, js_imgui_functions, IMGUI_SET_COLUMN_OFFSET),
        JS_CFUNC_MAGIC_DEF("GetColumnsCount", 0, js_imgui_functions, IMGUI_GET_COLUMNS_COUNT),
        JS_CFUNC_MAGIC_DEF("BeginTabBar", 2, js_imgui_functions, IMGUI_BEGIN_TAB_BAR),
        JS_CFUNC_MAGIC_DEF("EndTabBar", 0, js_imgui_functions, IMGUI_END_TAB_BAR),
        JS_CFUNC_MAGIC_DEF("BeginTabItem", 3, js_imgui_functions, IMGUI_BEGIN_TAB_ITEM),
        JS_CFUNC_MAGIC_DEF("EndTabItem", 0, js_imgui_functions, IMGUI_END_TAB_ITEM),
        JS_CFUNC_MAGIC_DEF("TabItemButton", 0, js_imgui_functions, IMGUI_TAB_ITEM_BUTTON),
        JS_CFUNC_MAGIC_DEF("SetTabItemClosed", 1, js_imgui_functions, IMGUI_SET_TAB_ITEM_CLOSED),
        JS_CFUNC_MAGIC_DEF("LogToTTY", 1, js_imgui_functions, IMGUI_LOG_TO_TTY),
        JS_CFUNC_MAGIC_DEF("LogToFile", 2, js_imgui_functions, IMGUI_LOG_TO_FILE),
        JS_CFUNC_MAGIC_DEF("LogToClipboard", 1, js_imgui_functions, IMGUI_LOG_TO_CLIPBOARD),
        JS_CFUNC_MAGIC_DEF("LogFinish", 0, js_imgui_functions, IMGUI_LOG_FINISH),
        JS_CFUNC_MAGIC_DEF("LogButtons", 0, js_imgui_functions, IMGUI_LOG_BUTTONS),
        JS_CFUNC_MAGIC_DEF("LogText", 1, js_imgui_functions, IMGUI_LOG_TEXT),
        JS_CFUNC_MAGIC_DEF("BeginDragDropSource", 1, js_imgui_functions,
                           IMGUI_BEGIN_DRAG_DROP_SOURCE),
        JS_CFUNC_MAGIC_DEF("SetDragDropPayload", 4, js_imgui_functions,
                           IMGUI_SET_DRAG_DROP_PAYLOAD),
        JS_CFUNC_MAGIC_DEF("EndDragDropSource", 0, js_imgui_functions, IMGUI_END_DRAG_DROP_SOURCE),
        JS_CFUNC_MAGIC_DEF("BeginDragDropTarget", 0, js_imgui_functions,
                           IMGUI_BEGIN_DRAG_DROP_TARGET),
        JS_CFUNC_MAGIC_DEF("AcceptDragDropPayload", 2, js_imgui_functions,
                           IMGUI_ACCEPT_DRAG_DROP_PAYLOAD),
        JS_CFUNC_MAGIC_DEF("EndDragDropTarget", 0, js_imgui_functions, IMGUI_END_DRAG_DROP_TARGET),
        JS_CFUNC_MAGIC_DEF("GetDragDropPayload", 0, js_imgui_functions,
                           IMGUI_GET_DRAG_DROP_PAYLOAD),
        JS_CFUNC_MAGIC_DEF("PushClipRect", 3, js_imgui_functions, IMGUI_PUSH_CLIP_RECT),
        JS_CFUNC_MAGIC_DEF("PopClipRect", 0, js_imgui_functions, IMGUI_POP_CLIP_RECT),
        JS_CFUNC_MAGIC_DEF("SetItemDefaultFocus", 0, js_imgui_functions,
                           IMGUI_SET_ITEM_DEFAULT_FOCUS),
        JS_CFUNC_MAGIC_DEF("SetKeyboardFocusHere", 1, js_imgui_functions,
                           IMGUI_SET_KEYBOARD_FOCUS_HERE),
        JS_CFUNC_MAGIC_DEF("IsItemHovered", 1, js_imgui_functions, IMGUI_IS_ITEM_HOVERED),
        JS_CFUNC_MAGIC_DEF("IsItemActive", 0, js_imgui_functions, IMGUI_IS_ITEM_ACTIVE),
        JS_CFUNC_MAGIC_DEF("IsItemFocused", 0, js_imgui_functions, IMGUI_IS_ITEM_FOCUSED),
        JS_CFUNC_MAGIC_DEF("IsItemClicked", 1, js_imgui_functions, IMGUI_IS_ITEM_CLICKED),
        JS_CFUNC_MAGIC_DEF("IsItemVisible", 0, js_imgui_functions, IMGUI_IS_ITEM_VISIBLE),
        JS_CFUNC_MAGIC_DEF("IsItemEdited", 0, js_imgui_functions, IMGUI_IS_ITEM_EDITED),
        JS_CFUNC_MAGIC_DEF("IsItemActivated", 0, js_imgui_functions, IMGUI_IS_ITEM_ACTIVATED),
        JS_CFUNC_MAGIC_DEF("IsItemDeactivated", 0, js_imgui_functions, IMGUI_IS_ITEM_DEACTIVATED),
        JS_CFUNC_MAGIC_DEF("IsItemDeactivatedAfterEdit", 0, js_imgui_functions,
                           IMGUI_IS_ITEM_DEACTIVATED_AFTER_EDIT),
        JS_CFUNC_MAGIC_DEF("IsAnyItemHovered", 0, js_imgui_functions, IMGUI_IS_ANY_ITEM_HOVERED),
        JS_CFUNC_MAGIC_DEF("IsAnyItemActive", 0, js_imgui_functions, IMGUI_IS_ANY_ITEM_ACTIVE),
        JS_CFUNC_MAGIC_DEF("IsAnyItemFocused", 0, js_imgui_functions, IMGUI_IS_ANY_ITEM_FOCUSED),
        JS_CFUNC_MAGIC_DEF("GetItemRectMin", 0, js_imgui_functions, IMGUI_GET_ITEM_RECT_MIN),
        JS_CFUNC_MAGIC_DEF("GetItemRectMax", 0, js_imgui_functions, IMGUI_GET_ITEM_RECT_MAX),
        JS_CFUNC_MAGIC_DEF("GetItemRectSize", 0, js_imgui_functions, IMGUI_GET_ITEM_RECT_SIZE),
        JS_CFUNC_MAGIC_DEF("SetItemAllowOverlap", 0, js_imgui_functions,
                           IMGUI_SET_ITEM_ALLOW_OVERLAP),
        JS_CFUNC_MAGIC_DEF("IsRectVisible", 2, js_imgui_functions, IMGUI_IS_RECT_VISIBLE),
        JS_CFUNC_MAGIC_DEF("GetTime", 0, js_imgui_functions, IMGUI_GET_TIME),
        JS_CFUNC_MAGIC_DEF("GetFrameCount", 0, js_imgui_functions, IMGUI_GET_FRAME_COUNT),
        JS_CFUNC_MAGIC_DEF("GetBackgroundDrawList", 0, js_imgui_functions,
                           IMGUI_GET_BACKGROUND_DRAW_LIST),
        JS_CFUNC_MAGIC_DEF("GetForegroundDrawList", 0, js_imgui_functions,
                           IMGUI_GET_FOREGROUND_DRAW_LIST),
        JS_CFUNC_MAGIC_DEF("GetDrawListSharedData", 0, js_imgui_functions,
                           IMGUI_GET_DRAW_LIST_SHARED_DATA),
        JS_CFUNC_MAGIC_DEF("GetStyleColorName", 1, js_imgui_functions, IMGUI_GET_STYLE_COLOR_NAME),
        JS_CFUNC_MAGIC_DEF("SetStateStorage", 1, js_imgui_functions, IMGUI_SET_STATE_STORAGE),
        JS_CFUNC_MAGIC_DEF("GetStateStorage", 0, js_imgui_functions, IMGUI_GET_STATE_STORAGE),
        JS_CFUNC_MAGIC_DEF("CalcTextSize", 4, js_imgui_functions, IMGUI_CALC_TEXT_SIZE),
        JS_CFUNC_MAGIC_DEF("CalcListClipping", 4, js_imgui_functions, IMGUI_CALC_LIST_CLIPPING),
        JS_CFUNC_MAGIC_DEF("BeginChildFrame", 3, js_imgui_functions, IMGUI_BEGIN_CHILD_FRAME),
        JS_CFUNC_MAGIC_DEF("EndChildFrame", 0, js_imgui_functions, IMGUI_END_CHILD_FRAME),
        JS_CFUNC_MAGIC_DEF("ColorConvertU32ToFloat4", 1, js_imgui_functions,
                           IMGUI_COLOR_CONVERTU32_TO_FLOAT4),
        JS_CFUNC_MAGIC_DEF("ColorConvertFloat4ToU32", 1, js_imgui_functions,
                           IMGUI_COLOR_CONVERT_FLOAT4_TOU32),
        JS_CFUNC_MAGIC_DEF("ColorConvertRGBtoHSV", 6, js_imgui_functions,
                           IMGUI_COLOR_CONVERT_RGB_TO_HSV),
        JS_CFUNC_MAGIC_DEF("ColorConvertHSVtoRGB", 6, js_imgui_functions,
                           IMGUI_COLOR_CONVERT_HSV_TO_RGB),
        JS_CFUNC_MAGIC_DEF("GetKeyIndex", 1, js_imgui_functions, IMGUI_GET_KEY_INDEX),
        JS_CFUNC_MAGIC_DEF("IsKeyDown", 1, js_imgui_functions, IMGUI_IS_KEY_DOWN),
        JS_CFUNC_MAGIC_DEF("IsKeyPressed", 2, js_imgui_functions, IMGUI_IS_KEY_PRESSED),
        JS_CFUNC_MAGIC_DEF("IsKeyReleased", 1, js_imgui_functions, IMGUI_IS_KEY_RELEASED),
        JS_CFUNC_MAGIC_DEF("GetKeyPressedAmount", 3, js_imgui_functions,
                           IMGUI_GET_KEY_PRESSED_AMOUNT),
        JS_CFUNC_MAGIC_DEF("IsMouseDown", 1, js_imgui_functions, IMGUI_IS_MOUSE_DOWN),
        JS_CFUNC_MAGIC_DEF("IsAnyMouseDown", 0, js_imgui_functions, IMGUI_IS_ANY_MOUSE_DOWN),
        JS_CFUNC_MAGIC_DEF("IsMouseClicked", 2, js_imgui_functions, IMGUI_IS_MOUSE_CLICKED),
        JS_CFUNC_MAGIC_DEF("IsMouseDoubleClicked", 1, js_imgui_functions,
                           IMGUI_IS_MOUSE_DOUBLE_CLICKED),
        JS_CFUNC_MAGIC_DEF("IsMouseReleased", 1, js_imgui_functions, IMGUI_IS_MOUSE_RELEASED),
        JS_CFUNC_MAGIC_DEF("IsMouseDragging", 2, js_imgui_functions, IMGUI_IS_MOUSE_DRAGGING),
        JS_CFUNC_MAGIC_DEF("IsMouseHoveringRect", 3, js_imgui_functions,
                           IMGUI_IS_MOUSE_HOVERING_RECT),
        JS_CFUNC_MAGIC_DEF("IsMousePosValid", 1, js_imgui_functions, IMGUI_IS_MOUSE_POS_VALID),
        JS_CFUNC_MAGIC_DEF("GetMousePos", 0, js_imgui_functions, IMGUI_GET_MOUSE_POS),
        JS_CFUNC_MAGIC_DEF("GetMousePosOnOpeningCurrentPopup", 0, js_imgui_functions,
                           IMGUI_GET_MOUSE_POS_ON_OPENING_CURRENT_POPUP),
        JS_CFUNC_MAGIC_DEF("GetMouseDragDelta", 2, js_imgui_functions, IMGUI_GET_MOUSE_DRAG_DELTA),
        JS_CFUNC_MAGIC_DEF("ResetMouseDragDelta", 1, js_imgui_functions,
                           IMGUI_RESET_MOUSE_DRAG_DELTA),
        JS_CFUNC_MAGIC_DEF("GetMouseCursor", 0, js_imgui_functions, IMGUI_GET_MOUSE_CURSOR),
        JS_CFUNC_MAGIC_DEF("SetMouseCursor", 1, js_imgui_functions, IMGUI_SET_MOUSE_CURSOR),
        JS_CFUNC_MAGIC_DEF("CaptureKeyboardFromApp", 1, js_imgui_functions,
                           IMGUI_CAPTURE_KEYBOARD_FROM_APP),
        JS_CFUNC_MAGIC_DEF("CaptureMouseFromApp", 1, js_imgui_functions,
                           IMGUI_CAPTURE_MOUSE_FROM_APP),
        JS_CFUNC_MAGIC_DEF("GetClipboardText", 0, js_imgui_functions, IMGUI_GET_CLIPBOARD_TEXT),
        JS_CFUNC_MAGIC_DEF("SetClipboardText", 1, js_imgui_functions, IMGUI_SET_CLIPBOARD_TEXT),
        JS_CFUNC_MAGIC_DEF("LoadIniSettingsFromDisk", 1, js_imgui_functions,
                           IMGUI_LOAD_INI_SETTINGS_FROM_DISK),
        JS_CFUNC_MAGIC_DEF("LoadIniSettingsFromMemory", 2, js_imgui_functions,
                           IMGUI_LOAD_INI_SETTINGS_FROM_MEMORY),
        JS_CFUNC_MAGIC_DEF("SaveIniSettingsToDisk", 1, js_imgui_functions,
                           IMGUI_SAVE_INI_SETTINGS_TO_DISK),
        JS_CFUNC_MAGIC_DEF("SaveIniSettingsToMemory", 1, js_imgui_functions,
                           IMGUI_SAVE_INI_SETTINGS_TO_MEMORY),
        JS_CFUNC_MAGIC_DEF("SetAllocatorFunctions", 3, js_imgui_functions,
                           IMGUI_SET_ALLOCATOR_FUNCTIONS),
        JS_CFUNC_MAGIC_DEF("MemAlloc", 1, js_imgui_functions, IMGUI_MEM_ALLOC),
        JS_CFUNC_MAGIC_DEF("MemFree", 1, js_imgui_functions, IMGUI_MEM_FREE),
        JS_PROP_STRING_DEF("IMGUI_VERSION", IMGUI_VERSION, JS_PROP_ENUMERABLE),
        JS_PROP_INT32_DEF("IMGUI_VERSION_NUM", IMGUI_VERSION_NUM, JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("WindowFlags", js_imgui_window_flags, countof(js_imgui_window_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("InputTextFlags", js_imgui_inputtext_flags, countof(js_imgui_inputtext_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TreeNodeFlags", js_imgui_treenode_flags, countof(js_imgui_treenode_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("PopupFlags", js_imgui_popup_flags, countof(js_imgui_popup_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("SelectableFlags", js_imgui_selectable_flags,
                      countof(js_imgui_selectable_flags), JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("ComboFlags", js_imgui_combo_flags, countof(js_imgui_combo_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TabBarFlags", js_imgui_tabbar_flags, countof(js_imgui_tabbar_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TabItemFlags", js_imgui_tabitem_flags, countof(js_imgui_tabitem_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TableFlags", js_imgui_table_flags, countof(js_imgui_table_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TableColumnFlags", js_imgui_tablecolumn_flags,
                      countof(js_imgui_tablecolumn_flags), JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TableRowFlags", js_imgui_tablerow_flags, countof(js_imgui_tablerow_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("TableBgTarget", js_imgui_tablebgtarget, countof(js_imgui_tablebgtarget),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("FocusedFlags", js_imgui_focused_flags, countof(js_imgui_focused_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("HoveredFlags", js_imgui_hovered_flags, countof(js_imgui_hovered_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("DragDropFlags", js_imgui_dragdrop_flags, countof(js_imgui_dragdrop_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("DataType", js_imgui_datatype, countof(js_imgui_datatype),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("Dir", js_imgui_dir, countof(js_imgui_dir), JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("SortDirection", js_imgui_sortdirection, countof(js_imgui_sortdirection),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("Key", js_imgui_key, countof(js_imgui_key), JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("KeyModFlags", js_imgui_keymod_flags, countof(js_imgui_keymod_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("NavInput", js_imgui_navinput, countof(js_imgui_navinput),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("ConfigFlags", js_imgui_config_flags, countof(js_imgui_config_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("BackendFlags", js_imgui_backend_flags, countof(js_imgui_backend_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("Col", js_imgui_col, countof(js_imgui_col), JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("StyleVar", js_imgui_stylevar, countof(js_imgui_stylevar),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("ButtonFlags", js_imgui_button_flags, countof(js_imgui_button_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("ColorEditFlags", js_imgui_coloredit_flags, countof(js_imgui_coloredit_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("SliderFlags", js_imgui_slider_flags, countof(js_imgui_slider_flags),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("MouseButton", js_imgui_mousebutton, countof(js_imgui_mousebutton),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("MouseCursor", js_imgui_mousecursor, countof(js_imgui_mousecursor),
                      JS_PROP_ENUMERABLE),
        JS_OBJECT_DEF("Cond", js_imgui_cond, countof(js_imgui_cond), JS_PROP_ENUMERABLE),
};

#ifndef _METADOT_IMGUI_JSBIND_INPUTTEXTCALLBACKDATA
#define _METADOT_IMGUI_JSBIND_INPUTTEXTCALLBACKDATA

thread_local JSClassID js_imgui_inputtextcallbackdata_class_id = 0;
thread_local JSValue imgui_inputtextcallbackdata_proto = {JS_TAG_UNDEFINED},
                     imgui_inputtextcallbackdata_ctor = {JS_TAG_UNDEFINED};

enum {
    INPUTTEXTCALLBACKDATA_DELETE_CHARS,
    INPUTTEXTCALLBACKDATA_INSERT_CHARS,
    INPUTTEXTCALLBACKDATA_SELECT_ALL,
    INPUTTEXTCALLBACKDATA_CLEAR_SELECTION,
    INPUTTEXTCALLBACKDATA_HAS_SELECTION,
};

static inline ImGuiInputTextCallbackData *js_imgui_inputtextcallbackdata_data2(JSContext *ctx,
                                                                               JSValueConst value) {
    return static_cast<ImGuiInputTextCallbackData *>(
            JS_GetOpaque2(ctx, value, js_imgui_inputtextcallbackdata_class_id));
}

static JSValue js_imgui_inputtextcallbackdata_constructor(JSContext *ctx, JSValueConst new_target,
                                                          int argc, JSValueConst argv[]) {
    JSValue proto, obj = JS_UNDEFINED;
    ImGuiInputTextCallbackData *itcd = new ImGuiInputTextCallbackData();
    /* using new_target to get the prototype is necessary when the class is extended. */
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto)) goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_imgui_inputtextcallbackdata_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj)) goto fail;
    JS_SetOpaque(obj, itcd);
    return obj;
fail:
    js_free(ctx, itcd);
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static void js_imgui_inputtextcallbackdata_finalizer(JSRuntime *rt, JSValue val) {
    ImGuiInputTextCallbackData *itcd = static_cast<ImGuiInputTextCallbackData *>(
            JS_GetOpaque(val, js_imgui_inputtextcallbackdata_class_id));
    if (itcd) { delete itcd; }
    JS_FreeValueRT(rt, val);
}

static JSValue js_imgui_inputtextcallbackdata_functions(JSContext *ctx, JSValueConst this_val,
                                                        int argc, JSValueConst argv[], int magic) {
    ImGuiInputTextCallbackData *itcd;
    JSValue ret = JS_UNDEFINED;

    if (!(itcd = js_imgui_inputtextcallbackdata_data2(ctx, this_val))) return ret;

    switch (magic) {

        case INPUTTEXTCALLBACKDATA_DELETE_CHARS: {
            int32_t pos, bytes_count;
            JS_ToInt32(ctx, &pos, argv[0]);
            JS_ToInt32(ctx, &bytes_count, argv[1]);
            itcd->DeleteChars(pos, bytes_count);
            break;
        }
        case INPUTTEXTCALLBACKDATA_INSERT_CHARS: {
            int32_t pos;
            const char *text = JS_ToCString(ctx, argv[1]), *text_end = 0;
            if (argc >= 3) text_end = JS_ToCString(ctx, argv[2]);
            JS_ToInt32(ctx, &pos, argv[0]);
            itcd->InsertChars(pos, text, text_end);
            JS_FreeCString(ctx, text);
            if (text_end) JS_FreeCString(ctx, text_end);
            break;
        }
        case INPUTTEXTCALLBACKDATA_SELECT_ALL: {
            itcd->SelectAll();
            break;
        }
        case INPUTTEXTCALLBACKDATA_CLEAR_SELECTION: {
            itcd->ClearSelection();
            break;
        }
        case INPUTTEXTCALLBACKDATA_HAS_SELECTION: {
            ret = JS_NewBool(ctx, itcd->HasSelection());
            break;
        }
    }

    return ret;
}

static JSClassDef js_imgui_inputtextcallbackdata_class = {
        .class_name = "ImGuiInputTextCallbackData",
        .finalizer = js_imgui_inputtextcallbackdata_finalizer,
};

static const JSCFunctionListEntry js_imgui_inputtextcallbackdata_funcs[] = {
        JS_CFUNC_MAGIC_DEF("DeleteChars", 2, js_imgui_inputtextcallbackdata_functions,
                           INPUTTEXTCALLBACKDATA_DELETE_CHARS),
        JS_CFUNC_MAGIC_DEF("InsertChars", 2, js_imgui_inputtextcallbackdata_functions,
                           INPUTTEXTCALLBACKDATA_INSERT_CHARS),
        JS_CFUNC_MAGIC_DEF("SelectAll", 0, js_imgui_inputtextcallbackdata_functions,
                           INPUTTEXTCALLBACKDATA_SELECT_ALL),
        JS_CFUNC_MAGIC_DEF("ClearSelection", 0, js_imgui_inputtextcallbackdata_functions,
                           INPUTTEXTCALLBACKDATA_CLEAR_SELECTION),
        JS_CFUNC_MAGIC_DEF("HasSelection", 0, js_imgui_inputtextcallbackdata_functions,
                           INPUTTEXTCALLBACKDATA_HAS_SELECTION),
};

#endif

int js_imgui_init(JSContext *ctx, JSModuleDef *m) {

    JS_NewClassID(&js_imgui_io_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_imgui_io_class_id, &js_imgui_io_class);

    imgui_io_ctor =
            JS_NewCFunction2(ctx, js_imgui_io_constructor, "ImGuiIO", 1, JS_CFUNC_constructor, 0);
    imgui_io_proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, imgui_io_proto, js_imgui_io_funcs, countof(js_imgui_io_funcs));
    JS_SetClassProto(ctx, js_imgui_io_class_id, imgui_io_proto);

    JS_NewClassID(&js_imgui_style_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_imgui_style_class_id, &js_imgui_style_class);

    imgui_style_ctor = JS_NewCFunction2(ctx, js_imgui_style_constructor, "ImGuiStyle", 1,
                                        JS_CFUNC_constructor, 0);
    imgui_style_proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, imgui_style_proto, js_imgui_style_funcs,
                               countof(js_imgui_style_funcs));
    JS_SetClassProto(ctx, js_imgui_style_class_id, imgui_style_proto);

    JS_NewClassID(&js_imgui_inputtextcallbackdata_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_imgui_inputtextcallbackdata_class_id,
                &js_imgui_inputtextcallbackdata_class);

    imgui_inputtextcallbackdata_ctor =
            JS_NewCFunction2(ctx, js_imgui_inputtextcallbackdata_constructor, "ImGuiPayload", 1,
                             JS_CFUNC_constructor, 0);
    imgui_inputtextcallbackdata_proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, imgui_inputtextcallbackdata_proto,
                               js_imgui_inputtextcallbackdata_funcs,
                               countof(js_imgui_inputtextcallbackdata_funcs));
    JS_SetClassProto(ctx, js_imgui_inputtextcallbackdata_class_id,
                     imgui_inputtextcallbackdata_proto);

    JS_NewClassID(&js_imgui_payload_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_imgui_payload_class_id, &js_imgui_payload_class);

    imgui_payload_ctor = JS_NewCFunction2(ctx, js_imgui_payload_constructor, "ImGuiPayload", 1,
                                          JS_CFUNC_constructor, 0);
    imgui_payload_proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, imgui_payload_proto, js_imgui_payload_funcs,
                               countof(js_imgui_payload_funcs));
    JS_SetClassProto(ctx, js_imgui_payload_class_id, imgui_payload_proto);

    JS_NewClassID(&js_imfont_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_imfont_class_id, &js_imfont_class);

    imfont_ctor =
            JS_NewCFunction2(ctx, js_imfont_constructor, "ImFont", 1, JS_CFUNC_constructor, 0);
    imfont_proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, imfont_proto, js_imfont_funcs, countof(js_imfont_funcs));
    JS_SetClassProto(ctx, js_imfont_class_id, imfont_proto);

    JS_NewClassID(&js_imfontatlas_class_id);
    JS_NewClass(JS_GetRuntime(ctx), js_imfontatlas_class_id, &js_imfontatlas_class);

    imfontatlas_ctor = JS_NewCFunction2(ctx, js_imfontatlas_constructor, "ImFontAtlas", 1,
                                        JS_CFUNC_constructor, 0);
    imfontatlas_proto = JS_NewObject(ctx);

    JS_SetPropertyFunctionList(ctx, imfontatlas_proto, js_imfontatlas_funcs,
                               countof(js_imfontatlas_funcs));
    JS_SetClassProto(ctx, js_imfontatlas_class_id, imfontatlas_proto);

    if (m) {
        JS_SetModuleExport(ctx, m, "ImGuiIO", imgui_io_ctor);
        JS_SetModuleExport(ctx, m, "ImGuiStyle", imgui_style_ctor);
        JS_SetModuleExport(ctx, m, "ImGuiInputTextCallbackData", imgui_inputtextcallbackdata_ctor);
        JS_SetModuleExport(ctx, m, "ImGuiPayload", imgui_payload_ctor);
        JS_SetModuleExport(ctx, m, "ImFont", imfont_ctor);
        JS_SetModuleExport(ctx, m, "ImFontAtlas", imfontatlas_ctor);
        JS_SetModuleExportList(ctx, m, js_imgui_static_funcs, countof(js_imgui_static_funcs));
    }

    return 0;
}

JSModuleDef *init_imgui_module(JSContext *ctx, const char *module_name) {
    JSModuleDef *m;
    if (!(m = JS_NewCModule(ctx, module_name, &js_imgui_init))) return m;
    JS_AddModuleExport(ctx, m, "ImGuiIO");
    JS_AddModuleExport(ctx, m, "ImGuiStyle");
    JS_AddModuleExport(ctx, m, "ImGuiInputTextCallbackData");
    JS_AddModuleExport(ctx, m, "ImGuiPayload");
    JS_AddModuleExport(ctx, m, "ImFont");
    JS_AddModuleExport(ctx, m, "ImFontAtlas");
    JS_AddModuleExportList(ctx, m, js_imgui_static_funcs, countof(js_imgui_static_funcs));
    return m;
}
