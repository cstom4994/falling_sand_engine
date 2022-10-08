#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include "Engine/lib/md4c.h"

#include <map>
#include <string>
#include <vector>

#include <atomic>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct ImMarkdown
{
    ImMarkdown();
    virtual ~ImMarkdown(){};

    int print(const std::string &text);

protected:
    virtual void BLOCK_DOC(bool);
    virtual void BLOCK_QUOTE(bool);
    virtual void BLOCK_UL(const MD_BLOCK_UL_DETAIL *, bool);
    virtual void BLOCK_OL(const MD_BLOCK_OL_DETAIL *, bool);
    virtual void BLOCK_LI(const MD_BLOCK_LI_DETAIL *, bool);
    virtual void BLOCK_HR(bool e);
    virtual void BLOCK_H(const MD_BLOCK_H_DETAIL *d, bool e);
    virtual void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool);
    virtual void BLOCK_HTML(bool);
    virtual void BLOCK_P(bool);
    virtual void BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL *, bool);
    virtual void BLOCK_THEAD(bool);
    virtual void BLOCK_TBODY(bool);
    virtual void BLOCK_TR(bool);
    virtual void BLOCK_TH(const MD_BLOCK_TD_DETAIL *, bool);
    virtual void BLOCK_TD(const MD_BLOCK_TD_DETAIL *, bool);

    virtual void SPAN_EM(bool e);
    virtual void SPAN_STRONG(bool e);
    virtual void SPAN_A(const MD_SPAN_A_DETAIL *d, bool e);
    virtual void SPAN_IMG(const MD_SPAN_IMG_DETAIL *, bool);
    virtual void SPAN_CODE(bool);
    virtual void SPAN_DEL(bool);
    virtual void SPAN_LATEXMATH(bool);
    virtual void SPAN_LATEXMATH_DISPLAY(bool);
    virtual void SPAN_WIKILINK(const MD_SPAN_WIKILINK_DETAIL *, bool);
    virtual void SPAN_U(bool);

    ////////////////////////////////////////////////////////////////////////////

    struct image_info
    {
        ImTextureID texture_id;
        ImVec2 size;
        ImVec2 uv0;
        ImVec2 uv1;
        ImVec4 col_tint;
        ImVec4 col_border;
    };

    //use m_href to identify image
    virtual bool get_image(image_info &nfo) const;
    virtual ImFont *get_font() const;
    virtual ImVec4 get_color() const;

    //url == m_href
    virtual void open_url() const;

    //returns true if the term has been processed
    virtual bool render_entity(const char *str, const char *str_end);

    //returns true if the term has been processed
    virtual bool render_html(const char *str, const char *str_end);

    //called when '\n' in source text where it is not semantically meaningful
    virtual void soft_break();
    ////////////////////////////////////////////////////////////////////////////

    //current state
    std::string m_href;//empty if no link/image

    bool m_is_underline = false;
    bool m_is_strikethrough = false;
    bool m_is_em = false;
    bool m_is_strong = false;
    bool m_is_table_header = false;
    bool m_is_table_body = false;
    bool m_is_image = false;
    unsigned m_hlevel = 0;//0 - no heading

private:
    int text(MD_TEXTTYPE type, const char *str, const char *str_end);
    int block(MD_BLOCKTYPE type, void *d, bool e);
    int span(MD_SPANTYPE type, void *d, bool e);

    void render_text(const char *str, const char *str_end);

    void set_font(bool e);
    void set_color(bool e);
    void set_href(bool e, const MD_ATTRIBUTE &src);

    //table state
    ImVec2 m_table_last_pos;
    int m_table_next_column = 0;
    ImVector<float> m_table_col_pos;
    ImVector<float> m_table_row_pos;

    //list state
    struct list_info
    {
        bool is_ol;
        char delim;
        unsigned cur_ol;
    };
    ImVector<list_info> m_list_stack;

    static void line(ImColor c, bool under);

    MD_PARSER m_md;
};

namespace ImGuiHelper {

    inline void init_style(const float pixel_ratio, const float dpi_scaling) {
        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();
        style.FrameRounding = 5.0F;
        style.ChildRounding = 5.0F;
        style.GrabRounding = 5.0F;
        style.PopupRounding = 5.0F;
        style.ScrollbarRounding = 5.0F;
        style.TabRounding = 5.0F;
        style.WindowRounding = 5.0F;
        style.WindowTitleAlign = ImVec2(0.5F, 0.5F);
    }

    enum class Alignment : unsigned char {
        kHorizontalCenter = 1 << 0,
        kVerticalCenter = 1 << 1,
        kCenter = kHorizontalCenter | kVerticalCenter,
    };

    /**
     * @brief Render text with alignment
     */
    inline void AlignedText(const std::string &text, Alignment align,
                            const float &width = 0.0F) {
        const auto alignment = static_cast<unsigned char>(align);
        const auto text_size = ImGui::CalcTextSize(text.c_str());
        const auto wind_size = ImGui::GetContentRegionAvail();
        if (alignment & static_cast<unsigned char>(Alignment::kHorizontalCenter)) {
            if (width < 0.1F) {
                ImGui::SetCursorPosX((wind_size.x - text_size.x) * 0.5F);
            } else {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                     (width - text_size.x) * 0.5F);
            }
        }
        if (alignment & static_cast<unsigned char>(Alignment::kVerticalCenter)) {
            ImGui::AlignTextToFramePadding();
        }

        ImGui::TextUnformatted(text.c_str());
    }

    inline auto CheckButton(const std::string &label, bool checked,
                            const ImVec2 &size) -> bool {
        if (checked) {
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                  ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        } else {
            ImGui::PushStyleColor(
                    ImGuiCol_ButtonHovered,
                    ImGui::GetStyle().Colors[ImGuiCol_TabUnfocusedActive]);
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
        }
        if (ImGui::Button(label.c_str(), size)) {
            checked = !checked;
        }

        ImGui::PopStyleColor(2);

        return checked;
    }

    inline auto ButtonTab(std::vector<std::string> &tabs, int &index) -> int {
        auto checked = 1 << index;
        std::string tab_names;
        std::for_each(tabs.begin(), tabs.end(),
                      [&tab_names](const auto item) { tab_names += item; });
        const auto tab_width = ImGui::GetContentRegionAvail().x;
        const auto tab_btn_width = tab_width / static_cast<float>(tabs.size());
        const auto h = ImGui::CalcTextSize(tab_names.c_str()).y;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, h);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, h);

        ImGui::PushStyleColor(ImGuiCol_ChildBg,
                              ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
        ImGui::BeginChild(tab_names.c_str(),
                          {tab_width, h + ImGui::GetStyle().FramePadding.y * 2},
                          false,
                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoResize);

        for (int i = 0; i < tabs.size(); ++i) {
            auto &tab = tabs[i];

            // if current tab is checkd, uncheck otheres
            if (CheckButton(tab, checked & (1 << i), ImVec2{tab_btn_width, 0})) {
                checked = 0;
                checked = 1 << i;
            }

            if (i != tabs.size() - 1) {
                ImGui::SameLine();
            }
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        ImGui::EndChild();

        index = 0;
        while (checked / 2) {
            checked = checked / 2;
            ++index;
        }

        return index;
    }

    inline void SwitchButton(std::string &&icon, std::string &&label,
                             bool &checked) {
        float height = ImGui::GetFrameHeight();
        float width = height * 1.55F;
        float radius = height * 0.50F;
        const auto frame_width = ImGui::GetContentRegionAvail().x;

        AlignedText(icon + "    " + label, Alignment::kVerticalCenter);
        ImGui::SameLine();

        ImGui::SetCursorPosX(frame_width - width);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton(label.c_str(), ImVec2(width, height))) {
            checked = !checked;
        }
        ImU32 col_bg = 0;
        if (checked) {
            col_bg = ImColor(ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
        } else {
            col_bg = ImColor(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        }
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), col_bg,
                                 radius);
        draw_list->AddCircleFilled(
                ImVec2(checked ? (pos.x + width - radius) : (pos.x + radius),
                       pos.y + radius),
                radius - 1.5F, IM_COL32_WHITE);
    }

    inline void Comb(std::string &&icon, std::string &&label,
                     const std::vector<const char *> &items, int &index) {
        const auto p_w = ImGui::GetContentRegionAvail().x;
        AlignedText(icon + "    " + label, Alignment::kVerticalCenter);
        ImGui::SameLine();
        ImGui::SetCursorPosX(p_w - 150.0F - ImGui::GetStyle().FramePadding.x);
        ImGui::SetNextItemWidth(150.0F);
        ImGui::Combo((std::string("##") + label).c_str(), &index, items.data(), static_cast<int>(items.size()));
    }

    inline void InputInt(std::string &&icon, std::string &&label, int &value) {
        const auto p_w = ImGui::GetContentRegionAvail().x;
        AlignedText(icon + "    " + label, Alignment::kVerticalCenter);
        ImGui::SameLine();
        ImGui::SetCursorPosX(p_w - 100.0F - ImGui::GetStyle().FramePadding.x);
        ImGui::SetNextItemWidth(100.0F);
        ImGui::InputInt((std::string("##") + label).c_str(), &value);
    }

    inline void ListSeparator(float indent = 30.0F) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems) {
            return;
        }

        ImGuiContext &g = *GImGui;

        float thickness_draw = 1.0F;
        float thickness_layout = 0.0F;
        // Horizontal Separator
        float x1 = window->Pos.x + indent;
        float x2 = window->Pos.x + window->Size.x;

        // FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
        if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID) {
            x1 += window->DC.Indent.x;
        }

        // We don't provide our width to the layout so that it doesn't get feed back into AutoFit
        const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
        ImGui::ItemSize(ImVec2(0.0F, thickness_layout));
        const bool item_visible = ImGui::ItemAdd(bb, 0);
        if (item_visible) {
            // Draw
            window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), ImGui::GetColorU32(ImGuiCol_Separator));
        }
    }

}// namespace ImGuiHelper


struct test_markdown : public ImMarkdown
{
    void open_url() const override {
        //HyperlinkHelper::OpenUrl(m_href.c_str());
        system(std::string("start msedge " + m_href).c_str());
    }

    bool get_image(image_info &nfo) const override {
        //use m_href to identify images
        nfo.size = {40, 20};
        nfo.uv0 = {0, 0};
        nfo.uv1 = {1, 1};
        nfo.col_tint = {1, 1, 1, 1};
        nfo.col_border = {0, 0, 0, 0};
        return true;
    }
};


#define MAX_FILE_DIALOG_NAME_BUFFER 1024

typedef void *UserDatas;

struct FileInfoStruct
{
    char type = ' ';
    std::string filePath;
    std::string fileName;
    std::string ext;
};

class ImGuiFileDialog {
private:
    std::vector<FileInfoStruct> m_FileList;
    std::map<std::string, ImVec4> m_FilterColor;
    //std::string m_SelectedFileName;
    std::map<std::string, bool> m_SelectedFileNames;// map for have binary search
    std::string m_SelectedExt;
    std::string m_CurrentPath;
    std::vector<std::string> m_CurrentPath_Decomposition;
    std::string m_Name;
    bool m_ShowDialog = false;
    bool m_ShowDrives = false;
    std::string m_LastSelectedFileName;// for shift multi selectio

public:
    static char FileNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
    static char DirectoryNameBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
    static char SearchBuffer[MAX_FILE_DIALOG_NAME_BUFFER];
    static int FilterIndex;
    bool IsOk = false;
    bool m_AnyWindowsHovered = false;
    bool m_CreateDirectoryMode = false;

private:
    std::string dlg_key;
    std::string dlg_name;
    const char *dlg_filters{};
    std::string dlg_path;
    std::string dlg_defaultFileName;
    std::string dlg_defaultExt;
    std::function<void(std::string, UserDatas, bool *)> dlg_optionsPane;
    size_t dlg_optionsPaneWidth = 0;
    std::string searchTag;
    UserDatas dlg_userDatas;
    size_t dlg_CountSelectionMax = 1;// 0 for infinite

public:
    static ImGuiFileDialog *Instance() {
        static auto *_instance = new ImGuiFileDialog();
        return _instance;
    }

protected:
    ImGuiFileDialog();                                                    // Prevent construction
    ImGuiFileDialog(const ImGuiFileDialog &){};                           // Prevent construction by copying
    ImGuiFileDialog &operator=(const ImGuiFileDialog &) { return *this; };// Prevent assignment
    ~ImGuiFileDialog();                                                   // Prevent unwanted destruction

public:
    void OpenDialog(const std::string &vKey, const char *vName, const char *vFilters,
                    const std::string &vPath, const std::string &vDefaultFileName,
                    const std::function<void(std::string, UserDatas, bool *)> &vOptionsPane, const size_t &vOptionsPaneWidth = 250,
                    const int &vCountSelectionMax = 1, UserDatas vUserDatas = 0);
    void OpenDialog(const std::string &vKey, const char *vName, const char *vFilters,
                    const std::string &vDefaultFileName,
                    const std::function<void(std::string, UserDatas, bool *)> &vOptionsPane, const size_t &vOptionsPaneWidth = 250,
                    const int &vCountSelectionMax = 1, UserDatas vUserDatas = 0);
    void OpenDialog(const std::string &vKey, const char *vName, const char *vFilters,
                    const std::string &vPath, const std::string &vDefaultFileName,
                    const int &vCountSelectionMax = 1, UserDatas vUserDatas = 0);
    void OpenDialog(const std::string &vKey, const char *vName, const char *vFilters,
                    const std::string &vFilePathName, const int &vCountSelectionMax = 1,
                    UserDatas vUserDatas = 0);

    bool FileDialog(const std::string &vKey, ImGuiWindowFlags vFlags = ImGuiWindowFlags_NoCollapse);
    void CloseDialog(const std::string &vKey);

    std::string GetFilepathName();
    std::string GetCurrentPath();
    std::string GetCurrentFileName();
    std::string GetCurrentFilter();
    UserDatas GetUserDatas();
    std::map<std::string, std::string> GetSelection();// return map<FileName, FilePathName>

    void SetFilterColor(const std::string &vFilter, ImVec4 vColor);
    bool GetFilterColor(const std::string &vFilter, ImVec4 *vColor);
    void ClearFilterColor();

private:
    bool SelectDirectory(const FileInfoStruct &vInfos);

    void SelectFileName(const FileInfoStruct &vInfos);
    void RemoveFileNameInSelection(const std::string &vFileName);
    void AddFileNameInSelection(const std::string &vFileName, bool vSetLastSelectionFileName);

    void CheckFilter();

    void SetPath(const std::string &vPath);
    void ScanDir(const std::string &vPath);
    void SetCurrentDir(const std::string &vPath);
    bool CreateDir(const std::string &vPath);
    void ComposeNewPath(std::vector<std::string>::iterator vIter);
    void GetDrives();
};


/**
   Dear ImGui with IME on-the-spot translation routines.
   author: TOGURO Mikito , mit@shalab.net
*/


#if !defined(imgex_hpp_HEADER_GUARD_7556619d_62b7_4f3b_b364_f02af36a3bbc)
#define imgex_hpp_HEADER_GUARD_7556619d_62b7_4f3b_b364_f02af36a3bbc 1

#if defined(_WIN32)
#include <Windows.h>
#include <tchar.h>
#endif /* defined( _WIN32 ) */

#include "imgui.h"

#if defined(__cplusplus)
#include <cassert>
#else /* defined( __cplusplus ) */
#include <assert.h>
#endif /* defined( __cplusplus ) */

#if !defined(VERIFY_ASSERT)
#if defined(IM_ASSERT)
#define VERIFY_ASSERT(exp) IM_ASSERT(exp)
#else /* defined(IM_ASSERT) */
#define VERIFY_ASSERT(exp) assert(exp)
#endif /* defined(IM_ASSERT) */
#endif /* !defined( VERIFY_ASSERT ) */

#if !defined(VERIFY)
#if defined(NDEBUG)
#define VERIFY(exp) \
    do { (void) (exp); } while (0)
#else /* defined( NDEBUG ) */
#define VERIFY(exp) VERIFY_ASSERT(exp)
#endif /* defined( NDEBUG ) */
#endif /* !defined( VERIFY ) */

#if defined(__cplusplus)

namespace imgex {
    // composition of flags
    namespace implements {
        template<typename first_t>
        constexpr inline unsigned int
        composite_flags_0(first_t first) {
            return static_cast<unsigned int>(first);
        }
        template<typename first_t, typename... tail_t>
        constexpr inline unsigned int
        composite_flags_0(first_t first, tail_t... tail) {
            return static_cast<unsigned int>(first) | composite_flags_0(tail...);
        }
    }// namespace implements

    template<typename require_t, typename... tail_t>
    constexpr inline require_t
    composite_flags(tail_t... tail) {
        return static_cast<require_t>(implements::composite_flags_0(tail...));
    }
}// namespace imgex

#endif /* defined( __cplusplus ) */
#endif /* !defined( imgex_hpp_HEADER_GUARD_7556619d_62b7_4f3b_b364_f02af36a3bbc ) */


#if !defined(IMGUI_IMM32_ONTHESPOT_H_UUID_ccfbd514_0a94_4888_a8b8_f065c57c1e70_HEADER_GUARD)
#define IMGUI_IMM32_ONTHESPOT_H_UUID_ccfbd514_0a94_4888_a8b8_f065c57c1e70_HEADER_GUARD 1

//#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <commctrl.h>
#include <tchar.h>
#endif /* defined( _WIN32 ) */

#if !defined(WM_IMGUI_IMM32_COMMAND_BEGIN)
#define WM_IMGUI_IMM32_COMMAND_BEGIN (WM_APP + 0x200)
#endif /* !defined( WM_IMGUI_IMM32_COMMAND_BEGIN ) */

struct ImGUIIMMCommunication
{

    enum {
        WM_IMGUI_IMM32_COMMAND = WM_IMGUI_IMM32_COMMAND_BEGIN,
        WM_IMGUI_IMM32_END
    };

    enum {
        WM_IMGUI_IMM32_COMMAND_NOP = 0u,
        WM_IMGUI_IMM32_COMMAND_SUBCLASSIFY,
        WM_IMGUI_IMM32_COMMAND_COMPOSITION_COMPLETE,
        WM_IMGUI_IMM32_COMMAND_CLEANUP
    };

    struct IMMCandidateList
    {
        std::vector<std::string> list_utf8;
        std::vector<std::string>::size_type selection;

        IMMCandidateList()
            : list_utf8{}, selection(0) {
        }
        IMMCandidateList(const IMMCandidateList &rhv) = default;
        IMMCandidateList(IMMCandidateList &&rhv) noexcept
            : list_utf8(), selection(0) {
            *this = std::move(rhv);
        }

        ~IMMCandidateList() = default;

        inline IMMCandidateList &
        operator=(const IMMCandidateList &rhv) = default;

        inline IMMCandidateList &
        operator=(IMMCandidateList &&rhv) noexcept {
            if (this == &rhv) {
                return *this;
            }
            std::swap(list_utf8, rhv.list_utf8);
            std::swap(selection, rhv.selection);
            return *this;
        }
        inline void clear() {
            list_utf8.clear();
            selection = 0;
        }
        static IMMCandidateList cocreate(const CANDIDATELIST *const src, const size_t src_size);
    };

    static constexpr int candidate_window_num = 9;

    bool is_open;
    std::unique_ptr<char[]> comp_conved_utf8;
    std::unique_ptr<char[]> comp_target_utf8;
    std::unique_ptr<char[]> comp_unconv_utf8;
    bool show_ime_candidate_list;
    int request_candidate_list_str_commit;// 1の時に candidate list が更新された後に、次の変換候補へ移る要請をする
    IMMCandidateList candidate_list;

    ImGUIIMMCommunication()
        : is_open(false),
          comp_conved_utf8(nullptr),
          comp_target_utf8(nullptr),
          comp_unconv_utf8(nullptr),
          show_ime_candidate_list(false),
          request_candidate_list_str_commit(false),
          candidate_list() {
    }

    ~ImGUIIMMCommunication() = default;
    void operator()();

private:
    bool update_candidate_window(HWND hWnd);

    static LRESULT
            WINAPI
            imm_communication_subClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT
    imm_communication_subClassProc_implement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR uIdSubclass, ImGUIIMMCommunication &comm);
    BOOL subclassify_impl(HWND hWnd);

public:
    template<typename type_t>
    inline BOOL subclassify(type_t hWnd);

    template<>
    inline BOOL subclassify<HWND>(HWND hWnd) {
        return subclassify_impl(hWnd);
    }
};

//#if defined( _WIN32 )
//
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3.h>
//#include <GLFW/glfw3native.h>
//
//template<>
//inline BOOL
//ImGUIIMMCommunication::subclassify<GLFWwindow*>(GLFWwindow* window)
//{
//    return this->subclassify(glfwGetWin32Window(window));
//}
//#endif


#include <SDL.h>
#include <SDL_syswm.h>

#if defined(__cplusplus)

#if defined(_WIN32)
template<>
inline BOOL
ImGUIIMMCommunication::subclassify<SDL_Window *>(SDL_Window *window) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        IM_ASSERT(IsWindow(info.info.win.window));
        return this->subclassify(info.info.win.window);
    }
    return FALSE;
}

#endif /* defined( _WIN32 ) */
#endif /* defined( __cplusplus ) */

#endif


#ifndef __IM_STRING__
#define __IM_STRING__

class ImString {

public:
    ImString();

    ImString(size_t len);

    ImString(char *string);

    explicit ImString(const char *string);

    ImString(const ImString &other);

    ~ImString();

    char &operator[](size_t pos);

    operator char *();

    bool operator==(const char *string);

    bool operator!=(const char *string);

    bool operator==(const ImString &string);

    bool operator!=(const ImString &string);

    ImString &operator=(const char *string);

    ImString &operator=(const ImString &other);

    inline size_t size() const { return mData ? strlen(mData) + 1 : 0; }

    void reserve(size_t len);

    char *get();

    const char *c_str() const;

    bool empty() const;

    int refcount() const;

    void ref();

    void unref();

private:
    char *mData;
    int *mRefCount;
};

#endif
