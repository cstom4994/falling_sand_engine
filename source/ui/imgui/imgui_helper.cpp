#include "imgui_helper.hpp"

#include "core/sdl_wrapper.h"

#pragma region ImString

ImString::ImString() : mData(0), mRefCount(0) {}

ImString::ImString(size_t len) : mData(0), mRefCount(0) { reserve(len); }

ImString::ImString(char *string) : mData(string), mRefCount(0) { ref(); }

ImString::ImString(const char *string) : mData(0), mRefCount(0) {
    if (string) {
        mData = ImStrdup(string);
        ref();
    }
}

ImString::ImString(const ImString &other) {
    mRefCount = other.mRefCount;
    mData = other.mData;
    ref();
}

ImString::~ImString() { unref(); }

char &ImString::operator[](size_t pos) { return mData[pos]; }

ImString::operator char *() { return mData; }

bool ImString::operator==(const char *string) { return strcmp(string, mData) == 0; }

bool ImString::operator!=(const char *string) { return strcmp(string, mData) != 0; }

bool ImString::operator==(ImString &string) { return strcmp(string.c_str(), mData) == 0; }

bool ImString::operator!=(const ImString &string) { return strcmp(string.c_str(), mData) != 0; }

ImString &ImString::operator=(const char *string) {
    if (mData) unref();
    mData = ImStrdup(string);
    ref();
    return *this;
}

ImString &ImString::operator=(const ImString &other) {
    if (mData && mData != other.mData) unref();
    mRefCount = other.mRefCount;
    mData = other.mData;
    ref();
    return *this;
}

void ImString::reserve(size_t len) {
    if (mData) unref();
    mData = (char *)ImGui::MemAlloc(len + 1);
    mData[len] = '\0';
    ref();
}

char *ImString::get() { return mData; }

const char *ImString::c_str() const { return mData; }

bool ImString::empty() const { return mData == 0 || mData[0] == '\0'; }

int ImString::refcount() const { return *mRefCount; }

void ImString::ref() {
    if (!mRefCount) {
        mRefCount = new int();
        (*mRefCount) = 0;
    }
    (*mRefCount)++;
}

void ImString::unref() {
    if (mRefCount) {
        (*mRefCount)--;
        if (*mRefCount == 0) {
            ImGui::MemFree(mData);
            mData = 0;
            delete mRefCount;
        }
        mRefCount = 0;
    }
}

#pragma endregion ImString

ImGuiMarkdown::ImGuiMarkdown() {

    m_table_last_pos = ImVec2(0, 0);

    m_md.abi_version = 0;
    m_md.syntax = nullptr;
    m_md.debug_log = nullptr;

    m_md.flags = MD_FLAG_TABLES | MD_FLAG_UNDERLINE | MD_FLAG_STRIKETHROUGH;
    m_md.enter_block = [](MD_BLOCKTYPE type, void *detail, void *userdata) { return ((ImGuiMarkdown *)userdata)->block(type, detail, true); };

    m_md.leave_block = [](MD_BLOCKTYPE type, void *detail, void *userdata) { return ((ImGuiMarkdown *)userdata)->block(type, detail, false); };

    m_md.enter_span = [](MD_SPANTYPE type, void *detail, void *userdata) { return ((ImGuiMarkdown *)userdata)->span(type, detail, true); };

    m_md.leave_span = [](MD_SPANTYPE type, void *detail, void *userdata) { return ((ImGuiMarkdown *)userdata)->span(type, detail, false); };

    m_md.text = [](MD_TEXTTYPE type, const MD_CHAR *text, MD_SIZE size, void *userdata) { return ((ImGuiMarkdown *)userdata)->text(type, text, text + size); };
}

void ImGuiMarkdown::BLOCK_UL(const MD_BLOCK_UL_DETAIL *d, bool e) {
    if (e) {
        m_list_stack.push_back(list_info{false, d->mark, 0});
    } else {
        m_list_stack.pop_back();
        if (m_list_stack.empty()) ImGui::NewLine();
    }
}

void ImGuiMarkdown::BLOCK_OL(const MD_BLOCK_OL_DETAIL *d, bool e) {
    if (e) {
        m_list_stack.push_back(list_info{true, d->mark_delimiter, d->start});
    } else {
        m_list_stack.pop_back();
        if (m_list_stack.empty()) ImGui::NewLine();
    }
}

void ImGuiMarkdown::BLOCK_LI(const MD_BLOCK_LI_DETAIL *, bool e) {
    if (e) {
        ImGui::NewLine();

        list_info &nfo = m_list_stack.back();
        if (nfo.is_ol) {
            ImGui::Text("%d%c", nfo.cur_ol++, nfo.delim);
            ImGui::SameLine();
        } else {
            if (nfo.delim == '*') {
                float cx = ImGui::GetCursorPosX();
                cx -= ImGui::GetStyle().FramePadding.x * 2;
                ImGui::SetCursorPosX(cx);
                ImGui::Bullet();
            } else {
                ImGui::Text("%c", nfo.delim);
                ImGui::SameLine();
            }
        }

        ImGui::Indent();
    } else {
        ImGui::Unindent();
    }
}

void ImGuiMarkdown::BLOCK_HR(bool e) {
    if (!e) {
        ImGui::NewLine();
        ImGui::Separator();
    }
}

void ImGuiMarkdown::BLOCK_H(const MD_BLOCK_H_DETAIL *d, bool e) {
    if (e) {
        m_hlevel = d->level;
        ImGui::NewLine();
    } else {
        m_hlevel = 0;
    }

    set_font(e);

    if (!e) {
        if (d->level <= 2) {
            ImGui::NewLine();
            ImGui::Separator();
        }
    }
}

void ImGuiMarkdown::BLOCK_DOC(bool) {}

void ImGuiMarkdown::BLOCK_QUOTE(bool) {}
void ImGuiMarkdown::BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool) {}

void ImGuiMarkdown::BLOCK_HTML(bool) {}

void ImGuiMarkdown::BLOCK_P(bool) {
    if (!m_list_stack.empty()) return;
    ImGui::NewLine();
}

void ImGuiMarkdown::BLOCK_TABLE(const MD_BLOCK_TABLE_DETAIL *, bool e) {
    if (e) {
        m_table_row_pos.clear();
        m_table_col_pos.clear();

        m_table_last_pos = ImGui::GetCursorPos();
    } else {

        ImGui::NewLine();
        m_table_last_pos.y = ImGui::GetCursorPos().y;

        m_table_col_pos.push_back(m_table_last_pos.x);
        m_table_row_pos.push_back(m_table_last_pos.y);

        const ImVec2 wp = ImGui::GetWindowPos();
        const ImVec2 sp = ImGui::GetStyle().ItemSpacing;
        const float wx = wp.x + sp.x / 2;
        const float wy = wp.y - sp.y / 2 - ImGui::GetScrollY();

        for (int i = 0; i < m_table_col_pos.size(); ++i) {
            m_table_col_pos[i] += wx;
        }

        for (int i = 0; i < m_table_row_pos.size(); ++i) {
            m_table_row_pos[i] += wy;
        }

        const ImColor c = ImGui::GetStyle().Colors[ImGuiCol_Separator];
        ////////////////////////////////////////////////////////////////////////

        ImDrawList *dl = ImGui::GetWindowDrawList();

        const float xmin = m_table_col_pos.front();
        const float xmax = m_table_col_pos.back();
        for (int i = 0; i < m_table_row_pos.size(); ++i) {
            const float p = m_table_row_pos[i];
            dl->AddLine(ImVec2(xmin, p), ImVec2(xmax, p), c, i == 1 ? 2.0f : 1.0f);
        }

        const float ymin = m_table_row_pos.front();
        const float ymax = m_table_row_pos.back();
        for (int i = 0; i < m_table_col_pos.size(); ++i) {
            const float p = m_table_col_pos[i];
            dl->AddLine(ImVec2(p, ymin), ImVec2(p, ymax), c);
        }
    }
}

void ImGuiMarkdown::BLOCK_THEAD(bool e) {
    m_is_table_header = e;
    set_font(e);
}

void ImGuiMarkdown::BLOCK_TBODY(bool e) { m_is_table_body = e; }

void ImGuiMarkdown::BLOCK_TR(bool e) {
    ImGui::SetCursorPosY(m_table_last_pos.y);

    if (e) {
        m_table_next_column = 0;
        ImGui::NewLine();
        m_table_row_pos.push_back(ImGui::GetCursorPosY());
    }
}

void ImGuiMarkdown::BLOCK_TH(const MD_BLOCK_TD_DETAIL *d, bool e) { BLOCK_TD(d, e); }

void ImGuiMarkdown::BLOCK_TD(const MD_BLOCK_TD_DETAIL *, bool e) {
    if (e) {

        if (m_table_next_column < m_table_col_pos.size()) {
            ImGui::SetCursorPosX(m_table_col_pos[m_table_next_column]);
        } else {
            m_table_col_pos.push_back(m_table_last_pos.x);
        }

        ++m_table_next_column;

        ImGui::Indent(m_table_col_pos[m_table_next_column - 1]);
        ImGui::SetCursorPos(ImVec2(m_table_col_pos[m_table_next_column - 1], m_table_row_pos.back()));

    } else {
        const ImVec2 p = ImGui::GetCursorPos();
        ImGui::Unindent(m_table_col_pos[m_table_next_column - 1]);
        ImGui::SetCursorPosX(p.x);
        if (p.y > m_table_last_pos.y) m_table_last_pos.y = p.y;
    }
    ImGui::TextUnformatted("");
    ImGui::SameLine();
}

////////////////////////////////////////////////////////////////////////////////
void ImGuiMarkdown::set_href(bool e, const MD_ATTRIBUTE &src) {
    if (e) {
        m_href.assign(src.text, src.size);
    } else {
        m_href.clear();
    }
}

void ImGuiMarkdown::set_font(bool e) {
    if (e) {
        ImGui::PushFont(get_font());
    } else {
        ImGui::PopFont();
    }
}

void ImGuiMarkdown::set_color(bool e) {
    if (e) {
        ImGui::PushStyleColor(ImGuiCol_Text, get_color());
    } else {
        ImGui::PopStyleColor();
    }
}

void ImGuiMarkdown::line(ImColor c, bool under) {
    ImVec2 mi = ImGui::GetItemRectMin();
    ImVec2 ma = ImGui::GetItemRectMax();

    if (!under) {
        ma.y -= ImGui::GetFontSize() / 2;
    }

    mi.y = ma.y;

    ImGui::GetWindowDrawList()->AddLine(mi, ma, c, 1.0f);
}

void ImGuiMarkdown::SPAN_A(const MD_SPAN_A_DETAIL *d, bool e) {
    set_href(e, d->href);
    set_color(e);
}

void ImGuiMarkdown::SPAN_EM(bool e) {
    m_is_em = e;
    set_font(e);
}

void ImGuiMarkdown::SPAN_STRONG(bool e) {
    m_is_strong = e;
    set_font(e);
}

void ImGuiMarkdown::SPAN_IMG(const MD_SPAN_IMG_DETAIL *d, bool e) {
    m_is_image = e;

    set_href(e, d->src);

    if (e) {

        image_info nfo;
        if (get_image(nfo)) {

            const float scale = ImGui::GetIO().FontGlobalScale;
            nfo.size.x *= scale;
            nfo.size.y *= scale;

            ImVec2 const csz = ImGui::GetContentRegionAvail();
            if (nfo.size.x > csz.x) {
                const float r = nfo.size.y / nfo.size.x;
                nfo.size.x = csz.x;
                nfo.size.y = csz.x * r;
            }

            ImGui::Image(nfo.texture_id, nfo.size, nfo.uv0, nfo.uv1, nfo.col_tint, nfo.col_border);

            if (ImGui::IsItemHovered()) {

                // if (d->title.size) {
                //	ImGui::SetTooltip("%.*s", (int)d->title.size, d->title.text);
                // }

                if (ImGui::IsMouseReleased(0)) {
                    open_url();
                }
            }
        }
    }
}

void ImGuiMarkdown::SPAN_CODE(bool) {}

void ImGuiMarkdown::SPAN_LATEXMATH(bool) {}

void ImGuiMarkdown::SPAN_LATEXMATH_DISPLAY(bool) {}

void ImGuiMarkdown::SPAN_WIKILINK(const MD_SPAN_WIKILINK_DETAIL *, bool) {}

void ImGuiMarkdown::SPAN_U(bool e) { m_is_underline = e; }

void ImGuiMarkdown::SPAN_DEL(bool e) { m_is_strikethrough = e; }

void ImGuiMarkdown::render_text(const char *str, const char *str_end) {
    const float scale = ImGui::GetIO().FontGlobalScale;
    const ImGuiStyle &s = ImGui::GetStyle();

    while (!m_is_image && str < str_end) {

        const char *te = str_end;

        if (!m_is_table_header) {

            float wl = ImGui::GetContentRegionAvail().x;

            if (m_is_table_body) {
                wl = (m_table_next_column < m_table_col_pos.size() ? m_table_col_pos[m_table_next_column] : m_table_last_pos.x);
                wl -= ImGui::GetCursorPosX();
            }

            te = ImGui::GetFont()->CalcWordWrapPositionA(scale, str, str_end, wl);

            if (te == str) ++te;
        }

        ImGui::TextUnformatted(str, te);

        if (!m_href.empty()) {

            ImVec4 c;
            if (ImGui::IsItemHovered()) {

                ImGui::SetTooltip("%s", m_href.c_str());

                c = s.Colors[ImGuiCol_ButtonHovered];
                if (ImGui::IsMouseReleased(0)) {
                    open_url();
                }
            } else {
                c = s.Colors[ImGuiCol_Button];
            }
            line(c, true);
        }
        if (m_is_underline) {
            line(s.Colors[ImGuiCol_Text], true);
        }
        if (m_is_strikethrough) {
            line(s.Colors[ImGuiCol_Text], false);
        }

        str = te;

        while (str < str_end && *str == ' ') ++str;
    }

    ImGui::SameLine(0.0f, 0.0f);
}

bool ImGuiMarkdown::render_entity(const char *str, const char *str_end) {
    const size_t sz = str_end - str;
    if (strncmp(str, "&nbsp;", sz) == 0) {
        ImGui::TextUnformatted("");
        ImGui::SameLine();
        return true;
    }
    return false;
}

bool ImGuiMarkdown::render_html(const char *str, const char *str_end) {
    const size_t sz = str_end - str;

    if (strncmp(str, "<br>", sz) == 0) {
        ImGui::NewLine();
        return true;
    }
    if (strncmp(str, "<hr>", sz) == 0) {
        ImGui::Separator();
        return true;
    }
    if (strncmp(str, "<u>", sz) == 0) {
        m_is_underline = true;
        return true;
    }
    if (strncmp(str, "</u>", sz) == 0) {
        m_is_underline = false;
        return true;
    }
    return false;
}

int ImGuiMarkdown::text(MD_TEXTTYPE type, const char *str, const char *str_end) {
    switch (type) {
        case MD_TEXT_NORMAL:
            render_text(str, str_end);
            break;
        case MD_TEXT_CODE:
            render_text(str, str_end);
            break;
        case MD_TEXT_NULLCHAR:
            break;
        case MD_TEXT_BR:
            ImGui::NewLine();
            break;
        case MD_TEXT_SOFTBR:
            soft_break();
            break;
        case MD_TEXT_ENTITY:
            if (!render_entity(str, str_end)) {
                render_text(str, str_end);
            };
            break;
        case MD_TEXT_HTML:
            if (!render_html(str, str_end)) {
                render_text(str, str_end);
            }
            break;
        case MD_TEXT_LATEXMATH:
            render_text(str, str_end);
            break;
        default:
            break;
    }

    if (m_is_table_header) {
        const float x = ImGui::GetCursorPosX();
        if (x > m_table_last_pos.x) m_table_last_pos.x = x;
    }

    return 0;
}

int ImGuiMarkdown::block(MD_BLOCKTYPE type, void *d, bool e) {
    switch (type) {
        case MD_BLOCK_DOC:
            BLOCK_DOC(e);
            break;
        case MD_BLOCK_QUOTE:
            BLOCK_QUOTE(e);
            break;
        case MD_BLOCK_UL:
            BLOCK_UL((MD_BLOCK_UL_DETAIL *)d, e);
            break;
        case MD_BLOCK_OL:
            BLOCK_OL((MD_BLOCK_OL_DETAIL *)d, e);
            break;
        case MD_BLOCK_LI:
            BLOCK_LI((MD_BLOCK_LI_DETAIL *)d, e);
            break;
        case MD_BLOCK_HR:
            BLOCK_HR(e);
            break;
        case MD_BLOCK_H:
            BLOCK_H((MD_BLOCK_H_DETAIL *)d, e);
            break;
        case MD_BLOCK_CODE:
            BLOCK_CODE((MD_BLOCK_CODE_DETAIL *)d, e);
            break;
        case MD_BLOCK_HTML:
            BLOCK_HTML(e);
            break;
        case MD_BLOCK_P:
            BLOCK_P(e);
            break;
        case MD_BLOCK_TABLE:
            BLOCK_TABLE((MD_BLOCK_TABLE_DETAIL *)d, e);
            break;
        case MD_BLOCK_THEAD:
            BLOCK_THEAD(e);
            break;
        case MD_BLOCK_TBODY:
            BLOCK_TBODY(e);
            break;
        case MD_BLOCK_TR:
            BLOCK_TR(e);
            break;
        case MD_BLOCK_TH:
            BLOCK_TH((MD_BLOCK_TD_DETAIL *)d, e);
            break;
        case MD_BLOCK_TD:
            BLOCK_TD((MD_BLOCK_TD_DETAIL *)d, e);
            break;
        default:
            assert(false);
            break;
    }

    return 0;
}

int ImGuiMarkdown::span(MD_SPANTYPE type, void *d, bool e) {
    switch (type) {
        case MD_SPAN_EM:
            SPAN_EM(e);
            break;
        case MD_SPAN_STRONG:
            SPAN_STRONG(e);
            break;
        case MD_SPAN_A:
            SPAN_A((MD_SPAN_A_DETAIL *)d, e);
            break;
        case MD_SPAN_IMG:
            SPAN_IMG((MD_SPAN_IMG_DETAIL *)d, e);
            break;
        case MD_SPAN_CODE:
            SPAN_CODE(e);
            break;
        case MD_SPAN_DEL:
            SPAN_DEL(e);
            break;
        case MD_SPAN_LATEXMATH:
            SPAN_LATEXMATH(e);
            break;
        case MD_SPAN_LATEXMATH_DISPLAY:
            SPAN_LATEXMATH_DISPLAY(e);
            break;
        case MD_SPAN_WIKILINK:
            SPAN_WIKILINK((MD_SPAN_WIKILINK_DETAIL *)d, e);
            break;
        case MD_SPAN_U:
            SPAN_U(e);
            break;
        default:
            assert(false);
            break;
    }

    return 0;
}

int ImGuiMarkdown::print(const std::string &text) { return md_parse(text.c_str(), text.size() + 1, &m_md, this); }

////////////////////////////////////////////////////////////////////////////////

ImFont *ImGuiMarkdown::get_font() const {
    return nullptr;  // default font

    // Example:
#if 0
    if (m_is_table_header) {
        return g_font_bold;
    }

    switch (m_hlevel)
    {
    case 0:
        return m_is_strong ? g_font_bold : g_font_regular;
    case 1:
        return g_font_bold_large;
    default:
        return g_font_bold;
    }
#endif
};

ImVec4 ImGuiMarkdown::get_color() const {
    if (!m_href.empty()) {
        return ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];
    }
    return ImGui::GetStyle().Colors[ImGuiCol_Text];
}

bool ImGuiMarkdown::get_image(image_info &nfo) const {
    nfo.texture_id = ImGui::GetIO().Fonts->TexID;
    nfo.size = {100, 50};
    nfo.uv0 = {0, 0};
    nfo.uv1 = {1, 1};
    nfo.col_tint = {1, 1, 1, 1};
    nfo.col_border = {0, 0, 0, 0};

    return true;
};

void ImGuiMarkdown::open_url() const {
    if (!m_is_image) {
        SDL_OpenURL(m_href.c_str());
    } else {
    }
}

void ImGuiMarkdown::soft_break() { ImGui::NewLine(); }

#if defined(_METADOT_IMM32)

#include <commctrl.h>
#include <tchar.h>
#include <windows.h>

#include <algorithm>
#include <cassert>

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_internal.h"

#pragma comment(linker, \
                "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

void ImGUIIMMCommunication::operator()() {
    ImGuiIO &io = ImGui::GetIO();

    static ImVec2 window_pos = ImVec2();
    static ImVec2 window_pos_pivot = ImVec2();

    static ImGuiID candidate_window_root_id = 0;

    static ImGuiWindow *lastTextInputNavWindow = nullptr;
    static ImGuiID lastTextInputActiveId = 0;
    static ImGuiID lastTextInputFocusId = 0;

    if (!(candidate_window_root_id && ((ImGui::GetCurrentContext()->NavWindow ? ImGui::GetCurrentContext()->NavWindow->RootWindow->ID : 0u) == candidate_window_root_id))) {

        window_pos = ImVec2(ImGui::GetCurrentContext()->PlatformImeData.InputPos.x + 1.0f,
                            ImGui::GetCurrentContext()->PlatformImeData.InputPos.y);  //
        window_pos_pivot = ImVec2(0.0f, 0.0f);

        const ImGuiContext *const currentContext = ImGui::GetCurrentContext();
        IM_ASSERT(currentContext || !"ImGui::GetCurrentContext() return nullptr.");
        if (currentContext) {
            if (!ImGui::IsMouseClicked(0)) {
                if ((currentContext->WantTextInputNextFrame != -1) ? (!!(currentContext->WantTextInputNextFrame)) : false) {
                    if ((!!currentContext->NavWindow) && (currentContext->NavWindow->RootWindow->ID != candidate_window_root_id) && (ImGui::GetActiveID() != lastTextInputActiveId)) {
                        OutputDebugStringW(L"update lastTextInputActiveId\n");
                        lastTextInputNavWindow = ImGui::GetCurrentContext()->NavWindow;
                        lastTextInputActiveId = ImGui::GetActiveID();
                        lastTextInputFocusId = ImGui::GetFocusID();
                    }
                } else {
                    if (lastTextInputActiveId != 0) {
                        if (currentContext->WantTextInputNextFrame) {
                            OutputDebugStringW(L"update lastTextInputActiveId disabled\n");
                        } else {
                            OutputDebugStringW(L"update lastTextInputActiveId disabled update\n");
                        }
                    }
                    lastTextInputNavWindow = nullptr;
                    lastTextInputActiveId = 0;
                    lastTextInputFocusId = 0;
                }
            }
        }
    }

    ImVec2 target_screen_pos = ImVec2(0.0f, 0.0f);  // IME Candidate List Window position.

    if (this->is_open) {
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin("IME Composition Window", nullptr,
                         ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings)) {

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78125f, 1.0f, 0.1875f, 1.0f));
            ImGui::Text(static_cast<bool>(comp_conved_utf8) ? comp_conved_utf8.get() : "");
            ImGui::PopStyleColor();

            if (static_cast<bool>(comp_target_utf8)) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.203125f, 0.91796875f, 0.35546875f, 1.0f));

                target_screen_pos = ImGui::GetCursorScreenPos();
                target_screen_pos.y += ImGui::GetTextLineHeightWithSpacing();

                ImGui::Text(static_cast<bool>(comp_target_utf8) ? comp_target_utf8.get() : "");
                ImGui::PopStyleColor();
            }
            if (static_cast<bool>(comp_unconv_utf8)) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78125f, 1.0f, 0.1875f, 1.0f));
                ImGui::Text(static_cast<bool>(comp_unconv_utf8) ? comp_unconv_utf8.get() : "");
                ImGui::PopStyleColor();
            }
            /*
              ImGui::SameLine();
              ImGui::Text("%u %u",
              candidate_window_root_id ,
              ImGui::GetCurrentContext()->NavWindow ? ImGui::GetCurrentContext()->NavWindow->RootWindow->ID : 0u);
            */
            ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
        }
        ImGui::End();
        ImGui::PopStyleVar();

        /* Draw Candidate List */
        if (show_ime_candidate_list && !candidate_list.list_utf8.empty()) {

            std::vector<const char *> listbox_items = {};

            IM_ASSERT(candidate_window_num);
            int candidate_page = ((int)candidate_list.selection) / candidate_window_num;
            int candidate_selection = ((int)candidate_list.selection) % candidate_window_num;

            auto begin_ite = std::begin(candidate_list.list_utf8);
            std::advance(begin_ite, candidate_page * candidate_window_num);
            auto end_ite = begin_ite;
            {
                auto the_end = std::end(candidate_list.list_utf8);
                for (int i = 0; end_ite != the_end && i < candidate_window_num; ++i) {
                    std::advance(end_ite, 1);
                }
            }

            std::for_each(begin_ite, end_ite, [&](auto &item) { listbox_items.push_back(item.c_str()); });

            const float candidate_window_height = ((ImGui::GetStyle().FramePadding.y * 2) + ((ImGui::GetTextLineHeightWithSpacing()) * ((int)std::size(listbox_items) + 2)));

            if (io.DisplaySize.y < (target_screen_pos.y + candidate_window_height)) {
                target_screen_pos.y -= ImGui::GetTextLineHeightWithSpacing() + candidate_window_height;
            }

            ImGui::SetNextWindowPos(target_screen_pos, ImGuiCond_Always, window_pos_pivot);

            if (ImGui::Begin("##Overlay-IME-Candidate-List-Window", &show_ime_candidate_list,
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
                if (ImGui::ListBoxHeader("##IMECandidateListWindow", static_cast<int>(std::size(listbox_items)), static_cast<int>(std::size(listbox_items)))) {

                    int i = 0;
                    for (const char *&listbox_item : listbox_items) {
                        if (ImGui::Selectable(listbox_item, (i == candidate_selection))) {

                            /* candidate list selection */

                            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)) {
                                if (lastTextInputActiveId && lastTextInputFocusId) {
                                    ImGui::SetActiveID(lastTextInputActiveId, lastTextInputNavWindow);
                                    ImGui::SetFocusID(lastTextInputFocusId, lastTextInputNavWindow);
                                }
                            }

                            /*
                                我想做 ImmNotifyIME (hImc, NI_SELECTCANDIDATESTR, 0, Candidate_page * Candidate_window_num + i);
                              由于 Vista ImmNotifyIME 不支持 NI_SELECTCANDIDATESTR。
                              @ 请参阅 Microsoft 的 IMM32 兼容性信息.doc

                              因此，DXUTguiIME.cpp（曾经使用过的 DXUT 的 gui IME 处理单元现在被视为已弃用。
                              您可以在 https://github.com/microsoft/DXUT 查看
                              有问题的代码是 https://github.com/microsoft/DXUT/blob/master/Optional/DXUTguiIME.cpp）
                              我确认过了

                              从 L.380 单击候选列表时有一个代码

                              原因是 SendKey 用于通过发送箭头光标键从候选列表中进行选择。
                              （你是什么意思 ？！）

                              基于此，使用 SendKey 创建代码。
                            */
                            {
                                if (candidate_selection == i) {
                                    OutputDebugStringW(L"complete\n");
                                    this->request_candidate_list_str_commit = 1;
                                } else {
                                    const BYTE nVirtualKey = (candidate_selection < i) ? VK_DOWN : VK_UP;
                                    const size_t nNumToHit = abs(candidate_selection - i);
                                    for (size_t hit = 0; hit < nNumToHit; ++hit) {
                                        keybd_event(nVirtualKey, 0, 0, 0);
                                        keybd_event(nVirtualKey, 0, KEYEVENTF_KEYUP, 0);
                                    }
                                    this->request_candidate_list_str_commit = (int)nNumToHit;
                                }
                            }
                        }
                        ++i;
                    }
                    ImGui::ListBoxFooter();
                }
                ImGui::Text("%d/%d", candidate_list.selection + 1, static_cast<int>(std::size(candidate_list.list_utf8)));
#if defined(_DEBUG)
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s",
#if defined(UNICODE)
                                   u8"DEBUG (UNICODE)"
#else
                                   u8"DEBUG (MBCS)"
#endif /* defined( UNICODE ) */
                );
#endif /* defined( DEBUG ) */
                // #1 ここで作るウィンドウがフォーカスを持ったときには、ウィンドウの位置を変更してはいけない。
                candidate_window_root_id = ImGui::GetCurrentWindowRead()->RootWindow->ID;
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            }
            ImGui::End();
        }
    }

    /*
      io.WantTextInput 根本不可靠。
      在文本输入小部件中，在另一个窗口上按下鼠标按钮以
      其他窗口和小部件在启用 WantTextInput 的情况下处于活动状态
      （特别是当涉及到 IME 候选窗口的小部件时，
      Press 焦点移动到启用了 WantTextInput 的 IME 候选窗口。
      在下一帧
      WantTextInput 已关闭，因为 IME 候选窗口未启用文本输入
      然后，这里就出现了IME失效的现象。 )

      这与直觉行为相反，所以我们会修复它，但是当禁用它时，
      如果任何小部件没有获得窗口焦点
      当 WantTextInput 为真时启用
      不对称的形状减少了不适。
    */
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        if (io.ImeWindowHandle) {
            IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
            (void)(ImmAssociateContext(static_cast<HWND>(io.ImeWindowHandle), HIMC(0)));
        }
    }
    if (io.WantTextInput) {
        if (io.ImeWindowHandle) {
            IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
            /*
              The second argument of ImmAssociateContextEx is
              Changed from IN to _In_ between 10.0.10586.0-Windows 10 and 10.0.14393.0-Windows.

              https://abi-laboratory.pro/compatibility/Windows_10_1511_10586.494_to_Windows_10_1607_14393.0/x86_64/headers_diff/imm32.dll/diff.html

              This is strange, but HIMC is probably not zero because this change was intentional.
              However, the document states that HIMC is ignored if the flag IACE_DEFAULT is used.

              https://docs.microsoft.com/en-us/windows/win32/api/immdev/nf-immdev-immassociatecontextex

              Passing HIMC appropriately when using IACE_DEFAULT causes an error and returns to 0
            */
            VERIFY(ImmAssociateContextEx(static_cast<HWND>(io.ImeWindowHandle), HIMC(0), IACE_DEFAULT));
        }
    }

    return;
}

ImGUIIMMCommunication::IMMCandidateList ImGUIIMMCommunication::IMMCandidateList::cocreate(const CANDIDATELIST *const src, const size_t src_size) {
    IM_ASSERT(nullptr != src);
    IM_ASSERT(sizeof(CANDIDATELIST) <= src->dwSize);
    IM_ASSERT(src->dwSelection < src->dwCount);

    IMMCandidateList dst{};
    if (!(sizeof(CANDIDATELIST) < src->dwSize)) {
        return dst;
    }
    if (!(src->dwSelection < src->dwCount)) {
        return dst;
    }
    const char *const baseaddr = reinterpret_cast<const char *>(src);

    for (size_t i = 0; i < src->dwCount; ++i) {
        const wchar_t *const item = reinterpret_cast<const wchar_t *>(baseaddr + src->dwOffset[i]);
        const int require_byte = WideCharToMultiByte(CP_UTF8, 0, item, -1, nullptr, 0, NULL, NULL);
        if (0 < require_byte) {
            std::unique_ptr<char[]> utf8buf{new char[require_byte]};
            if (require_byte == WideCharToMultiByte(CP_UTF8, 0, item, -1, utf8buf.get(), require_byte, NULL, NULL)) {
                dst.list_utf8.emplace_back(utf8buf.get());
                continue;
            }
        }
        dst.list_utf8.emplace_back("??");
    }
    dst.selection = src->dwSelection;
    return dst;
}

bool ImGUIIMMCommunication::update_candidate_window(HWND hWnd) {
    IM_ASSERT(IsWindow(hWnd));
    bool result = false;
    HIMC const hImc = ImmGetContext(hWnd);
    if (hImc) {
        DWORD dwSize = ImmGetCandidateListW(hImc, 0, NULL, 0);

        if (dwSize) {
            IM_ASSERT(sizeof(CANDIDATELIST) <= dwSize);
            if (sizeof(CANDIDATELIST) <= dwSize) {

                std::vector<char> candidatelist((size_t)dwSize);
                if ((DWORD)(std::size(candidatelist) * sizeof(typename decltype(candidatelist)::value_type)) ==
                    ImmGetCandidateListW(hImc, 0, reinterpret_cast<CANDIDATELIST *>(candidatelist.data()), (DWORD)(std::size(candidatelist) * sizeof(typename decltype(candidatelist)::value_type)))) {
                    const CANDIDATELIST *const cl = reinterpret_cast<CANDIDATELIST *>(candidatelist.data());
                    candidate_list = std::move(IMMCandidateList::cocreate(cl, dwSize));
                    result = true;
#if 0  /* for IMM candidate window debug BEGIN*/
                    {
                        wchar_t dbgbuf[1024];
                        _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                            L" dwSize = %d , dwCount = %d , dwSelection = %d\n",
                            cl->dwSize,
                            cl->dwCount,
                            cl->dwSelection);
                        OutputDebugStringW(dbgbuf);
                        for (DWORD i = 0; i < cl->dwCount; ++i) {
                            _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                L"%d%s: %s\n",
                                i, (cl->dwSelection == i ? L"*" : L" "),
                                (wchar_t*)(candidatelist.data() + cl->dwOffset[i]));
                            OutputDebugStringW(dbgbuf);
                        }
                    }
#endif /* for IMM candidate window debug END */
                }
            }
        }
        VERIFY(ImmReleaseContext(hWnd, hImc));
    }
    return result;
}

LRESULT
ImGUIIMMCommunication::imm_communication_subClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_DESTROY: {
            VERIFY(ImmAssociateContextEx(hWnd, HIMC(0), IACE_DEFAULT));
            if (!RemoveWindowSubclass(hWnd, reinterpret_cast<SUBCLASSPROC>(uIdSubclass), uIdSubclass)) {
                IM_ASSERT(!"RemoveWindowSubclass() failed\n");
            }
        } break;
        default:
            if (dwRefData) {
                return imm_communication_subClassProc_implement(hWnd, uMsg, wParam, lParam, uIdSubclass, *reinterpret_cast<ImGUIIMMCommunication *>(dwRefData));
            }
    }
    return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT
ImGUIIMMCommunication::imm_communication_subClassProc_implement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, ImGUIIMMCommunication &comm) {
    switch (uMsg) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            if (comm.is_open) {
                return 0;
            }
            break;

        case WM_IME_SETCONTEXT: { /* 各ビットを落とす */
            lParam &= ~(ISC_SHOWUICOMPOSITIONWINDOW | (ISC_SHOWUICANDIDATEWINDOW) | (ISC_SHOWUICANDIDATEWINDOW << 1) | (ISC_SHOWUICANDIDATEWINDOW << 2) | (ISC_SHOWUICANDIDATEWINDOW << 3));
        }
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        case WM_IME_STARTCOMPOSITION: {
            /* 此消息通知应用程序请求“IME”显示转换窗口。
            应用程序在处理转换字符串的显示时必须处理此消息。
            如果您让 DefWindowProc 函数处理此消息，则该消息将被传递到默认 IME 窗口。
            （即自己绘制转换窗口时，不要传给DefWindowPrc）
        */
            comm.is_open = true;
        }
            return 1;
        case WM_IME_ENDCOMPOSITION: {
            comm.is_open = false;
        }
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        case WM_IME_COMPOSITION: {
            HIMC const hImc = ImmGetContext(hWnd);
            if (hImc) {
                if (lParam & GCS_RESULTSTR) {
                    comm.comp_conved_utf8 = nullptr;
                    comm.comp_target_utf8 = nullptr;
                    comm.comp_unconv_utf8 = nullptr;
                    comm.show_ime_candidate_list = false;
                }
                if (lParam & GCS_COMPSTR) {

                    const LONG compstr_length_in_byte = ImmGetCompositionStringW(hImc, GCS_COMPSTR, nullptr, 0);
                    switch (compstr_length_in_byte) {
                        case IMM_ERROR_NODATA:
                        case IMM_ERROR_GENERAL:
                            break;
                        default: {
                            size_t const buf_length_in_wchar = (size_t(compstr_length_in_byte) / sizeof(wchar_t)) + 1;
                            IM_ASSERT(0 < buf_length_in_wchar);
                            std::unique_ptr<wchar_t[]> buf{new wchar_t[buf_length_in_wchar]};
                            if (buf) {
                                // std::fill( &buf[0] , &buf[buf_length_in_wchar-1] , L'\0' );
                                const LONG buf_length_in_byte = LONG(buf_length_in_wchar * sizeof(wchar_t));
                                const DWORD l = ImmGetCompositionStringW(hImc, GCS_COMPSTR, (LPVOID)(buf.get()), buf_length_in_byte);

                                const DWORD attribute_size = ImmGetCompositionStringW(hImc, GCS_COMPATTR, NULL, 0);
                                std::vector<char> attribute_vec(attribute_size, 0);
                                const DWORD attribute_end = ImmGetCompositionStringW(hImc, GCS_COMPATTR, attribute_vec.data(), (DWORD)std::size(attribute_vec));
                                IM_ASSERT(attribute_end == (DWORD)(std::size(attribute_vec)));
                                {
                                    std::wstring comp_converted;
                                    std::wstring comp_target;
                                    std::wstring comp_unconveted;
                                    size_t begin = 0;
                                    size_t end = 0;

                                    for (end = begin; end < attribute_end; ++end) {
                                        if ((ATTR_TARGET_CONVERTED == attribute_vec[end] || ATTR_TARGET_NOTCONVERTED == attribute_vec[end])) {
                                            break;
                                        } else {
                                            comp_converted.push_back(buf[end]);
                                        }
                                    }

                                    for (begin = end; end < attribute_end; ++end) {
                                        if (!(ATTR_TARGET_CONVERTED == attribute_vec[end] || ATTR_TARGET_NOTCONVERTED == attribute_vec[end])) {
                                            break;
                                        } else {
                                            comp_target.push_back(buf[end]);
                                        }
                                    }

                                    for (; end < attribute_end; ++end) {
                                        comp_unconveted.push_back(buf[end]);
                                    }

#if 0
                            {
                                wchar_t dbgbuf[1024];
                                _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                    L"attribute_size = %d \"%s[%s]%s\"\n",
                                    attribute_size,
                                    comp_converted.c_str(),
                                    comp_target.c_str(),
                                    comp_unconveted.c_str());
                                OutputDebugStringW(dbgbuf);
                            }
#endif
                                    // 准备一个 lambda 函数以将每个函数转换为 UTF-8
                                    /*
                                将 std::wstring 转换为 std::unique_ptr <char[]> 作为 UTF - 8 空终止字符串
                                如果参数是空字符串，则输入 nullptr
                                */
                                    auto to_utf8_pointer = [](const std::wstring &arg) -> std::unique_ptr<char[]> {
                                        if (arg.empty()) {
                                            return std::unique_ptr<char[]>(nullptr);
                                        }
                                        const int require_byte = WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, nullptr, 0, NULL, NULL);
                                        if (0 == require_byte) {
                                            const DWORD lastError = GetLastError();
                                            (void)(lastError);
                                            IM_ASSERT(ERROR_INSUFFICIENT_BUFFER != lastError);
                                            IM_ASSERT(ERROR_INVALID_FLAGS != lastError);
                                            IM_ASSERT(ERROR_INVALID_PARAMETER != lastError);
                                            IM_ASSERT(ERROR_NO_UNICODE_TRANSLATION != lastError);
                                        }
                                        IM_ASSERT(0 != require_byte);
                                        if (!(0 < require_byte)) {
                                            return std::unique_ptr<char[]>(nullptr);
                                        }

                                        std::unique_ptr<char[]> utf8buf{new char[require_byte]};

                                        const int conversion_result = WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, utf8buf.get(), require_byte, NULL, NULL);
                                        if (conversion_result == 0) {
                                            const DWORD lastError = GetLastError();
                                            (void)(lastError);
                                            IM_ASSERT(ERROR_INSUFFICIENT_BUFFER != lastError);
                                            IM_ASSERT(ERROR_INVALID_FLAGS != lastError);
                                            IM_ASSERT(ERROR_INVALID_PARAMETER != lastError);
                                            IM_ASSERT(ERROR_NO_UNICODE_TRANSLATION != lastError);
                                        }

                                        IM_ASSERT(require_byte == conversion_result);
                                        if (require_byte != conversion_result) {
                                            utf8buf.reset(nullptr);
                                        }
                                        return utf8buf;
                                    };

                                    comm.comp_conved_utf8 = to_utf8_pointer(comp_converted);
                                    comm.comp_target_utf8 = to_utf8_pointer(comp_target);
                                    comm.comp_unconv_utf8 = to_utf8_pointer(comp_unconveted);

                                    /*当 GCS_COMPSTR 更新时，Google IME 当然会 IMN_CHANGECANDIDATE
                                不会像完成一样发送。
                                所以，我请求你在这里更新候选人名单 */

                                    // 如果comp_target为空则删除
                                    if (!static_cast<bool>(comm.comp_target_utf8)) {
                                        comm.candidate_list.clear();
                                    } else {
                                        // candidate_list の取得に失敗したときは消去する
                                        if (!comm.update_candidate_window(hWnd)) {
                                            comm.candidate_list.clear();
                                        }
                                    }
                                }
                            }
                        } break;
                    }
                }
                VERIFY(ImmReleaseContext(hWnd, hImc));
            }

        }  // end of WM_IME_COMPOSITION

#if defined(UNICODE)
           // 在UNICODE配置的情况下，直接用DefWindowProc吸收进IME就可以了
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
            // 在多字节配置中，Window 子类的过程处理它，所以需要 DefSubclassProc。
#else
            return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
#endif

        case WM_IME_NOTIFY: {
            switch (wParam) {

                case IMN_OPENCANDIDATE:
                    // 通常，在发送 IMN_OPENCANDIDATE 时会设置 show_ime_candidate_list 标志。
                    // Google IME 不发送 IMN_OPENCANDIDATE（IMN_CHANGECANDIDATE 发送）
                    // 为此，在启动 IMN_CHANGECANDIDATE 时进行更改
                    comm.show_ime_candidate_list = true;
#if 0
            if (IMN_OPENCANDIDATE == wParam) {
                OutputDebugStringW(L"IMN_OPENCANDIDATE\n");
            }
#endif
                    ;  // tear down;
                case IMN_CHANGECANDIDATE: {
#if 0
            if (IMN_CHANGECANDIDATE == wParam) {
                OutputDebugStringW(L"IMN_CHANGECANDIDATE\n");
            }
#endif

                    // Google IME BEGIN 的代码 有关详细信息，请参阅有关 IMN_OPENCANDIDATE 的评论
                    if (!comm.show_ime_candidate_list) {
                        comm.show_ime_candidate_list = true;
                    }
                    // 谷歌输入法结束代码

                    HIMC const hImc = ImmGetContext(hWnd);
                    if (hImc) {
                        DWORD dwSize = ImmGetCandidateListW(hImc, 0, NULL, 0);
                        if (dwSize) {
                            IM_ASSERT(sizeof(CANDIDATELIST) <= dwSize);
                            if (sizeof(CANDIDATELIST) <= dwSize) {  // dwSize は最低でも struct CANDIDATELIST より大きくなければならない

                                (void)(lParam);
                                std::vector<char> candidatelist((size_t)dwSize);
                                if ((DWORD)(std::size(candidatelist) * sizeof(typename decltype(candidatelist)::value_type)) ==
                                    ImmGetCandidateListW(hImc, 0, reinterpret_cast<CANDIDATELIST *>(candidatelist.data()),
                                                         (DWORD)(std::size(candidatelist) * sizeof(typename decltype(candidatelist)::value_type)))) {
                                    const CANDIDATELIST *const cl = reinterpret_cast<CANDIDATELIST *>(candidatelist.data());
                                    comm.candidate_list = std::move(IMMCandidateList::cocreate(cl, dwSize));

#if 0  /* for IMM candidate window debug BEGIN*/
                            {
                                wchar_t dbgbuf[1024];
                                _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                    L"lParam = %lld, dwSize = %d , dwCount = %d , dwSelection = %d\n",
                                    lParam,
                                    cl->dwSize,
                                    cl->dwCount,
                                    cl->dwSelection);
                                OutputDebugStringW(dbgbuf);
                                for (DWORD i = 0; i < cl->dwCount; ++i) {
                                    _snwprintf_s(dbgbuf, sizeof(dbgbuf) / sizeof(dbgbuf[0]),
                                        L"%d%s: %s\n",
                                        i, (cl->dwSelection == i ? L"*" : L" "),
                                        (wchar_t*)(candidatelist.data() + cl->dwOffset[i]));
                                    OutputDebugStringW(dbgbuf);
                                }
                            }
#endif /* for IMM candidate window debug END */
                                }
                            }
                        }
                        VERIFY(ImmReleaseContext(hWnd, hImc));
                    }
                }

                    IM_ASSERT(0 <= comm.request_candidate_list_str_commit);
                    if (comm.request_candidate_list_str_commit) {
                        if (comm.request_candidate_list_str_commit == 1) {
                            VERIFY(PostMessage(hWnd, WM_IMGUI_IMM32_COMMAND, WM_IMGUI_IMM32_COMMAND_COMPOSITION_COMPLETE, 0));
                        }
                        --(comm.request_candidate_list_str_commit);
                    }

                    break;
                case IMN_CLOSECANDIDATE: {
                    // OutputDebugStringW(L"IMN_CLOSECANDIDATE\n");
                    comm.show_ime_candidate_list = false;
                } break;
                default:
                    break;
            }
        }
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_IME_REQUEST:
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_INPUTLANGCHANGE:
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);

        case WM_IMGUI_IMM32_COMMAND: {
            switch (wParam) {
                case WM_IMGUI_IMM32_COMMAND_NOP:  // NOP
                    return 1;
                case WM_IMGUI_IMM32_COMMAND_SUBCLASSIFY: {
                    ImGuiIO &io = ImGui::GetIO();
                    if (io.ImeWindowHandle) {
                        IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
                        VERIFY(ImmAssociateContextEx(static_cast<HWND>(io.ImeWindowHandle), nullptr, IACE_IGNORENOCONTEXT));
                    }
                }
                    return 1;
                case WM_IMGUI_IMM32_COMMAND_COMPOSITION_COMPLETE:
                    if (!static_cast<bool>(comm.comp_unconv_utf8) || '\0' == *(comm.comp_unconv_utf8.get())) {
                        /*
                   There is probably no unconverted string after the conversion target.
                   However, since there is now a cursor operation in the
                   keyboard buffer, the confirmation operation needs to be performed
                   after the cursor operation is completed.  For this purpose, an enter
                   key, which is a decision operation, is added to the end of the
                   keyboard buffer.
                */
#if 0
                /*
                  Here, the expected behavior is to perform the conversion completion
                  operation.
                  However, there is a bug that the TextInput loses the focus
                  and the conversion result is lost when the conversion
                  complete operation is performed.

                  To work around this bug, disable it.
                  This is because the movement of the focus of the widget and
                  the accompanying timing are complicated relationships.
                */
                HIMC const hImc = ImmGetContext(hWnd);
                if (hImc) {
                    VERIFY(ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0));
                    VERIFY(ImmReleaseContext(hWnd, hImc));
                }
#else
                        keybd_event(VK_RETURN, 0, 0, 0);
                        keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
#endif
                    } else {
                        /* Do this to close the candidate window without ending composition. */
                        /*
                  keybd_event (VK_RIGHT, 0, 0, 0);
                  keybd_event (VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);

                  keybd_event (VK_LEFT, 0, 0, 0);
                  keybd_event (VK_LEFT, 0, KEYEVENTF_KEYUP, 0);
                */
                        /*
                   Since there is an unconverted string after the conversion
                   target, press the right key of the keyboard to convert the
                   next clause to IME.
                */
                        keybd_event(VK_RIGHT, 0, 0, 0);
                        keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);
                    }
                    return 1;
                default:
                    break;
            }
        }
            return 1;
        default:
            break;
    }
    return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL ImGUIIMMCommunication::subclassify_impl(HWND hWnd) {
    IM_ASSERT(IsWindow(hWnd));
    if (!IsWindow(hWnd)) {
        return FALSE;
    }

    /* 在 imgui_imm32_onthespot 中进行 IME 控制，
            因为当 TextWidget 失去焦点时 io.WantTextInput 变为 true-> off
            这时候检查一下IME的状态，如果打开就关闭。
            @see ImGUIIMMCommunication :: operator () () 开头

            亲爱的ImGui的ImGui::IO::ImeWindowHandle原本用来指定CompositionWindow的位置
            我用了它，所以它符合目的

            但是，这种方法不好，因为当有多个目标操作系统窗口时它会崩溃。
    */
    ImGui::GetIO().ImeWindowHandle = static_cast<void *>(hWnd);
    if (::SetWindowSubclass(hWnd, ImGUIIMMCommunication::imm_communication_subClassProc, reinterpret_cast<UINT_PTR>(ImGUIIMMCommunication::imm_communication_subClassProc),
                            reinterpret_cast<DWORD_PTR>(this))) {
        /*
           I want to close IME once by calling imgex::imm_associate_context_disable()

           However, at this point, default IMM Context may not have been initialized by User32.dll yet.
           Therefore, a strategy is to post the message and set the IMMContext after the message pump is turned on.
        */
        return PostMessage(hWnd, WM_IMGUI_IMM32_COMMAND, WM_IMGUI_IMM32_COMMAND_SUBCLASSIFY, 0u);
    }
    return FALSE;
}

#endif

void ImGuiWidget::PlotFlame(const char *label, void (*values_getter)(float *start, float *end, ImU8 *level, const char **caption, const void *data, int idx), const void *data, int values_count,
                            int values_offset, const char *overlay_text, float scale_min, float scale_max, ImVec2 graph_size) {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return;

    ImGuiContext &g = *GImGui;
    const ImGuiStyle &style = g.Style;

    // Find the maximum depth
    ImU8 maxDepth = 0;
    for (int i = values_offset; i < values_count; ++i) {
        ImU8 depth;
        values_getter(nullptr, nullptr, &depth, nullptr, data, i);
        maxDepth = ImMax(maxDepth, depth);
    }

    const auto blockHeight = ImGui::GetTextLineHeight() + (style.FramePadding.y * 2);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    if (graph_size.x == 0.0f) graph_size.x = ImGui::CalcItemWidth();
    if (graph_size.y == 0.0f) graph_size.y = label_size.y + (style.FramePadding.y * 3) + blockHeight * (maxDepth + 1);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + graph_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, 0, &frame_bb)) return;

    // Determine scale from values if not specified
    if (scale_min == FLT_MAX || scale_max == FLT_MAX) {
        float v_min = FLT_MAX;
        float v_max = -FLT_MAX;
        for (int i = values_offset; i < values_count; i++) {
            float v_start, v_end;
            values_getter(&v_start, &v_end, nullptr, nullptr, data, i);
            if (v_start == v_start)  // Check non-NaN values
                v_min = ImMin(v_min, v_start);
            if (v_end == v_end)  // Check non-NaN values
                v_max = ImMax(v_max, v_end);
        }
        if (scale_min == FLT_MAX) scale_min = v_min;
        if (scale_max == FLT_MAX) scale_max = v_max;
    }

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    bool any_hovered = false;
    if (values_count - values_offset >= 1) {
        const ImU32 col_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x77FFFFFF;
        const ImU32 col_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x77FFFFFF;
        const ImU32 col_outline_base = ImGui::GetColorU32(ImGuiCol_PlotHistogram) & 0x7FFFFFFF;
        const ImU32 col_outline_hovered = ImGui::GetColorU32(ImGuiCol_PlotHistogramHovered) & 0x7FFFFFFF;

        for (int i = values_offset; i < values_count; ++i) {
            float stageStart, stageEnd;
            ImU8 depth;
            const char *caption;

            values_getter(&stageStart, &stageEnd, &depth, &caption, data, i);

            auto duration = scale_max - scale_min;
            if (duration == 0) {
                return;
            }

            auto start = stageStart - scale_min;
            auto end = stageEnd - scale_min;

            auto startX = static_cast<float>(start / (double)duration);
            auto endX = static_cast<float>(end / (double)duration);

            float width = inner_bb.Max.x - inner_bb.Min.x;
            float height = blockHeight * (maxDepth - depth + 1) - style.FramePadding.y;

            auto pos0 = inner_bb.Min + ImVec2(startX * width, height);
            auto pos1 = inner_bb.Min + ImVec2(endX * width, height + blockHeight);

            bool v_hovered = false;
            if (ImGui::IsMouseHoveringRect(pos0, pos1)) {
                ImGui::SetTooltip("%s: %8.4g", caption, stageEnd - stageStart);
                v_hovered = true;
                any_hovered = v_hovered;
            }

            window->DrawList->AddRectFilled(pos0, pos1, v_hovered ? col_hovered : col_base);
            window->DrawList->AddRect(pos0, pos1, v_hovered ? col_outline_hovered : col_outline_base);
            auto textSize = ImGui::CalcTextSize(caption);
            auto boxSize = (pos1 - pos0);
            auto textOffset = ImVec2(0.0f, 0.0f);
            if (textSize.x < boxSize.x) {
                textOffset = ImVec2(0.5f, 0.5f) * (boxSize - textSize);
                ImGui::RenderText(pos0 + textOffset, caption);
            }
        }

        // Text overlay
        if (overlay_text) ImGui::RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));

        if (label_size.x > 0.0f) ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
    }

    if (!any_hovered && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Total: %8.4g", scale_max - scale_min);
    }
}

/**
    Define ME_GUI_DISABLE_TEST_WINDOW to disable this feature.
*/

#ifndef ME_GUI_DISABLE_TEST_WINDOW
#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <vector>
#endif

#if 0

void ShowAutoTestWindow() {

    auto myCollapsingHeader = [](const char *name) -> bool {
        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::PushStyleColor(ImGuiCol_Header, style.Colors[ImGuiCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, style.Colors[ImGuiCol_ButtonHovered]);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, style.Colors[ImGuiCol_ButtonActive]);
        bool b = ImGui::CollapsingHeader(name);
        ImGui::PopStyleColor(3);
        return b;
    };
    if (myCollapsingHeader("About ImGui::Auto()")) {
        ImGui::Auto(R"comment(
ImGui::Auto() is one simple function that can create GUI for almost any data structure using ImGui functions.
a. Your data is presented in tree-like structure that is defined by your type.
    The generated code is highly efficient.
b. Const types are display only, the user cannot modify them.
    Use ImGui::as_cost() to permit the user to modify a non-const type.
The following types are supported (with nesting):
1 Strings. Flavours of std::string and char*. Use std::string for input.
2 Numbers. Integers, Floating points, ImVec{2,4}
3 STL Containers. Standard containers are supported. (std::vector, std::map,...)
The contained type has to be supported. If it is not, see 8.
4 Pointers, arrays. Pointed type must be supported.
5 std::pair, std::tuple. Types used must be supported.
6 structs and simple classes! The struct is converted to a tuple, and displayed as such.
    * Requirements: C++14 compiler (GCC-5.0+, Clang, Visual Studio 2017 with /std:c++17, ...)
    * How? I'm using this libary https://github.com/apolukhin/magic_get (ImGui::Auto() is only VS2017 C++17 tested.)
7 Functions. A void(void) type function becomes simple button.
For other types, you can input the arguments and calculate the result.
Not yet implemented!
8 You can define ImGui::Auto() for your own type! Use the following macros:
    * ME_GUI_DEFINE_INLINE(template_spec, type_spec, code)	single line definition, teplates shouldn't have commas inside!
    * ME_GUI_DEFINE_BEGIN(template_spec, type_spec)        start multiple line definition, no commas in arguments!
    * ME_GUI_DEFINE_BEGIN_P(template_spec, type_spec)      start multiple line definition, can have commas, use parentheses!
    * ME_GUI_DEFINE_END                                    end multiple line definition with this.
where
    * template_spec   describes how the type is templated. For fully specialized, use "template<>" only
    * type_spec       is the type for witch you define the ImGui::Auto() function.
	* var             will be the generated function argument of type type_spec.
	* name            is the const std::string& given by the user, and/or generated by the caller ImGui::Auto function
Example:           ME_GUI_DEFINE_INLINE(template<>, bool, ImGui::Checkbox(name.c_str(), &var);)
Tipps: - You may use ImGui::Auto_t<type>::Auto(var, name) functions directly.
        - Check imgui::detail namespace for other helper functions.

The libary uses partial template specialization, so definitions can overlap, the most specialized will be chosen.
However, using large, nested structs can lead to slow compilation.
Container data, large structs and tuples are hidden by default.
)comment");
        if (ImGui::TreeNode("TODO-s")) {
            ImGui::Auto(R"todos(
	1	Insert items to (non-const) set, map, list, ect
	2	Call any function or function object.
	3	Deduce function arguments as a tuple. User can edit the arguments, call the function (button), and view return value.
	4	All of the above needs self ImGui::Auto() allocated memmory. Current plan is to prioritize low memory usage
		over user experiance. (Beacuse if you want good UI, you code it, not generate it.)
		Plan A:
			a)	Data is stored in a map. Key can be the presneted object's address.
			b)	Data is allocated when not already present in the map
			c)	std::unique_ptr<void, deleter> is used in the map with deleter function manually added (or equivalent something)
			d)	The first call for ImGui::Auto at every few frames should delete
				(some of, or) all data that was not accesed in the last (few) frames. How?
		This means after closing a TreeNode, and opening it, the temporary values might be deleted and recreated.
)todos");
            ImGui::TreePop();
        }
    }
    if (myCollapsingHeader("1. String")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	ImGui::Auto("Hello Imgui::Auto() !"); //This is how this text is written as well.)code");
        ImGui::Auto("Hello Imgui::Auto() !");  // This is how this text is written as well.

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str = "Hello ImGui::Auto() for strings!";
	ImGui::Auto(str, "asd");)code");
        static std::string str = "Hello ImGui::Auto() for strings!";
        ImGui::Auto(str, "str");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str2 = "ImGui::Auto()\n Automatically uses multiline input for strings!\n:)";
	ImGui::Auto(str2, "str2");)code");
        static std::string str2 = "ImGui::Auto()\n Automatically uses multiline input for strings!\n:)";
        ImGui::Auto(str2, "str2");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::string conststr = "Const types are not to be changed!";
	ImGui::Auto(conststr, "conststr");)code");
        static const std::string conststr = "Const types are not to be changed!";
        ImGui::Auto(conststr, "conststr");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	char * buffer = "To edit a string use std::string. Manual buffers are unwelcome here.";
	ImGui::Auto(buffer, "buffer");)code");
        char *buffer = "To edit a string use std::string. Manual buffers are unwelcome here.";
        ImGui::Auto(buffer, "buffer");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("2. Numbers")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static int i = 42;
	ImGui::Auto(i, "i");)code");
        static int i = 42;
        ImGui::Auto(i, "i");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static float f = 3.14;
	ImGui::Auto(f, "f");)code");
        static float f = 3.14;
        ImGui::Auto(f, "f");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static ImVec4 f4 = {1.5f,2.1f,3.4f,4.3f};
	ImGui::Auto(f4, "f4");)code");
        static ImVec4 f4 = {1.5f, 2.1f, 3.4f, 4.3f};
        ImGui::Auto(f4, "f4");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const ImVec2 f2 = {1.f,2.f};
	ImGui::Auto(f2, "f2");)code");
        static const ImVec2 f2 = {1.f, 2.f};
        ImGui::Auto(f2, "f2");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("3. Containers")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static std::vector<std::string> vec = { "First string","Second str",":)" };
	ImGui::Auto(vec,"vec");)code");
        static std::vector<std::string> vec = {"First string", "Second str", ":)"};
        ImGui::Auto(vec, "vec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::vector<float> constvec = { 3,1,2.1f,4,3,4,5 };
	ImGui::Auto(constvec,"constvec");	//Cannot change vector, nor values)code");
        static const std::vector<float> constvec = {3, 1, 2.1f, 4, 3, 4, 5};
        ImGui::Auto(constvec, "constvec");  // Cannot change vector, nor values

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::vector<bool> bvec = { false, true, false, false };
	ImGui::Auto(bvec,"bvec");)code");
        static std::vector<bool> bvec = {false, true, false, false};
        ImGui::Auto(bvec, "bvec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::vector<bool> constbvec = { false, true, false, false };
	ImGui::Auto(constbvec,"constbvec");
	)code");
        static const std::vector<bool> constbvec = {false, true, false, false};
        ImGui::Auto(constbvec, "constbvec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::map<int, float> map = { {3,2},{1,2} };
	ImGui::Auto(map, "map");	// insert and other operations)code");
        static std::map<int, float> map = {{3, 2}, {1, 2}};
        ImGui::Auto(map, "map");  // insert and other operations

        if (ImGui::TreeNode("All cases")) {
            ImGui::Auto(R"code(
	static std::deque<bool> deque = { false, true, false, false };
	ImGui::Auto(deque,"deque");)code");
            static std::deque<bool> deque = {false, true, false, false};
            ImGui::Auto(deque, "deque");

            ImGui::NewLine();
            ImGui::Separator();

            ImGui::Auto(R"code(
	static std::set<char*> set = { "set","with","char*" };
	ImGui::Auto(set,"set");)code");
            static std::set<char *> set = {"set", "with", "char*"};  // for some reason, this does not work
            ImGui::Auto(set, "set");                                 // the problem is with the const iterator, but

            ImGui::NewLine();
            ImGui::Separator();

            ImGui::Auto(R"code(
	static std::map<char*, std::string> map = { {"asd","somevalue"},{"bsd","value"} };
	ImGui::Auto(map, "map");	// insert and other operations)code");
            static std::map<char *, std::string> map = {{"asd", "somevalue"}, {"bsd", "value"}};
            ImGui::Auto(map, "map");  // insert and other operations

            ImGui::TreePop();
        }
        ImGui::Unindent();
    }
    if (myCollapsingHeader("4. Pointers and Arrays")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static float *pf = nullptr;
	ImGui::Auto(pf, "pf");)code");
        static float *pf = nullptr;
        ImGui::Auto(pf, "pf");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static int i=10, *pi=&i;
	ImGui::Auto(pi, "pi");)code");
        static int i = 10, *pi = &i;
        ImGui::Auto(pi, "pi");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static const std::string cs= "I cannot be changed!", * cps=&cs;
	ImGui::Auto(cps, "cps");)code");
        static const std::string cs = "I cannot be changed!", *cps = &cs;
        ImGui::Auto(cps, "cps");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::string str = "I can be changed! (my pointee cannot)";
	static std::string *const strpc = &str;)code");
        static std::string str = "I can be changed! (my pointee cannot)";
        static std::string *const strpc = &str;
        ImGui::Auto(strpc, "strpc");

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::Auto(R"code(
	static std::array<float,5> farray = { 1.2, 3.4, 5.6, 7.8, 9.0 };
	ImGui::Auto(farray, "std::array");)code");
        static std::array<float, 5> farray = {1.2, 3.4, 5.6, 7.8, 9.0};
        ImGui::Auto(farray, "std::array");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static float farr[5] = { 1.2, 3.4, 5.6, 7.8, 9.0 };
	ImGui::Auto(farr, "float[5]");)code");
        static float farr[5] = {11.2, 3.4, 5.6, 7.8, 911.0};
        ImGui::Auto(farr, "float[5]");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("5. Pairs and Tuples")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	static std::pair<bool, ImVec2> pair = { true,{2.1f,3.2f} };
	ImGui::Auto(pair, "pair");)code");
        static std::pair<bool, ImVec2> pair = {true, {2.1f, 3.2f}};
        ImGui::Auto(pair, "pair");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::pair<int, std::string> pair2 = { -3,"simple types appear next to each other in a pair" };
	ImGui::Auto(pair2, "pair2");)code");
        static std::pair<int, std::string> pair2 = {-3, "simple types appear next to each other in a pair"};
        ImGui::Auto(pair2, "pair2");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	ImGui::Auto(ImGui::as_const(pair), "as_const(pair)"); //easy way to view as const)code");
        ImGui::Auto(ImGui::as_const(pair), "as_const(pair)");  // easy way to view as const

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	std::tuple<const int, std::string, ImVec2> tuple = { 42, "string in tuple", {3.1f,3.2f} };
	ImGui::Auto(tuple, "tuple");)code");
        std::tuple<const int, std::string, ImVec2> tuple = {42, "string in tuple", {3.1f, 3.2f}};
        ImGui::Auto(tuple, "tuple");

        ImGui::NewLine();
        ImGui::Separator();

        // 	ImGui::Auto(R"code(
        // const std::tuple<int, const char*, ImVec2> consttuple = { 42, "smaller tuples are inlined", {3.1f,3.2f} };
        // ImGui::Auto(consttuple, "consttuple");)code");
        // 	const std::tuple<int, const char*, ImVec2> consttuple = { 42, "Smaller tuples are inlined", {3.1f,3.2f} };
        // 	ImGui::Auto(consttuple, "consttuple");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("6. Structs!!")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	struct A //Structs are automagically converted to tuples!
	{
		int i = 216;
		bool b = true;
	};
	static A a;
	ImGui::Auto("a", a);)code");
        struct A {
            int i = 216;
            bool b = true;
        };
        static A a;
        ImGui::Auto(a, "a");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	ImGui::Auto(ImGui::as_const(a), "as_const(a)");// const structs are possible)code");
        ImGui::Auto(ImGui::as_const(a), "as_const(a)");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	struct B
	{
		std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
		const A a = A();
	};
	static B b;
	ImGui::Auto(b, "b");)code");
        struct B {
            std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
            const A a = A();
        };
        static B b;
        ImGui::Auto(b, "b");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	static std::vector<B> vec = { {"vector of structs!", A()}, B() };
	ImGui::Auto(vec, "vec");)code");
        static std::vector<B> vec = {{"vector of structs!", A()}, B()};
        ImGui::Auto(vec, "vec");

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::Auto(R"code(
	struct C
	{
		std::list<B> vec;
		A *a;
	};
	static C c = { {{"Container inside a struct!", A() }}, &a };
	ImGui::Auto(c, "c");)code");
        struct C {
            std::list<B> vec;
            A *a;
        };
        static C c = {{{"Container inside a struct!", A()}}, &a};
        ImGui::Auto(c, "c");

        ImGui::Unindent();
    }
    if (myCollapsingHeader("Functions")) {
        ImGui::Indent();

        ImGui::Auto(R"code(
	void (*func)() = []() { ImGui::TextColored(ImVec4(0.5, 1, 0.5, 1.0), "Button pressed, function called :)"); };
	ImGui::Auto(func, "void(void) function");)code");
        void (*func)() = []() {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5, 1, 0.5, 1.0), "Button pressed, function called :)");
        };
        ImGui::Auto(func, "void(void) function");

        ImGui::Unindent();
    }
}

#endif