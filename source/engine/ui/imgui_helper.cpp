#include "imgui_helper.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <vector>

bool ImGuiHelper::color_picker_3U32(const char *label, ImU32 *color, ImGuiColorEditFlags flags) {
    float col[3];
    col[0] = (float)((*color >> 0) & 0xFF) / 255.0f;
    col[1] = (float)((*color >> 8) & 0xFF) / 255.0f;
    col[2] = (float)((*color >> 16) & 0xFF) / 255.0f;

    bool result = ImGui::ColorPicker3(label, col, flags);

    *color = ((ImU32)(col[0] * 255.0f)) | ((ImU32)(col[1] * 255.0f) << 8) | ((ImU32)(col[2] * 255.0f) << 16);

    return result;
}

std::string ImGuiHelper::file_browser(const std::string &path) {

    ImGui::Text("Current Path: %s", path.c_str());
    ImGui::Separator();

    if (ImGui::Button("Parent Directory")) {
        std::filesystem::path currentPath(path);
        if (!currentPath.empty()) {
            currentPath = currentPath.parent_path();
            return file_browser(currentPath.string());
        }
    }

    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        const auto &entryPath = entry.path();
        const auto &entryFilename = entryPath.filename().string();
        if (entry.is_directory()) {
            if (ImGui::Selectable((entryFilename + "/").c_str())) return file_browser(entryPath.string());

        } else {
            if (ImGui::Selectable(entryFilename.c_str())) return entryFilename;
        }
    }

    return {};
}

MEimstr::MEimstr() : m_data(0), m_ref_count(0) {}
MEimstr::MEimstr(size_t len) : m_data(0), m_ref_count(0) { reserve(len); }
MEimstr::MEimstr(char *string) : m_data(string), m_ref_count(0) { ref(); }

MEimstr::MEimstr(const char *string) : m_data(0), m_ref_count(0) {
    if (string) {
        m_data = ImStrdup(string);
        ref();
    }
}

MEimstr::MEimstr(const MEimstr &other) {
    m_ref_count = other.m_ref_count;
    m_data = other.m_data;
    ref();
}

MEimstr::~MEimstr() { unref(); }

char &MEimstr::operator[](size_t pos) { return m_data[pos]; }

MEimstr::operator char *() { return m_data; }

bool MEimstr::operator==(const char *string) { return strcmp(string, m_data) == 0; }

bool MEimstr::operator!=(const char *string) { return strcmp(string, m_data) != 0; }

bool MEimstr::operator==(MEimstr &string) { return strcmp(string.c_str(), m_data) == 0; }

bool MEimstr::operator!=(const MEimstr &string) { return strcmp(string.c_str(), m_data) != 0; }

MEimstr &MEimstr::operator=(const char *string) {
    if (m_data) unref();
    m_data = ImStrdup(string);
    ref();
    return *this;
}

MEimstr &MEimstr::operator=(const MEimstr &other) {
    if (m_data && m_data != other.m_data) unref();
    m_ref_count = other.m_ref_count;
    m_data = other.m_data;
    ref();
    return *this;
}

void MEimstr::reserve(size_t len) {
    if (m_data) unref();
    m_data = (char *)ImGui::MemAlloc(len + 1);
    m_data[len] = '\0';
    ref();
}

char *MEimstr::get() { return m_data; }

const char *MEimstr::c_str() const { return m_data; }

bool MEimstr::empty() const { return m_data == 0 || m_data[0] == '\0'; }

int MEimstr::refcount() const { return *m_ref_count; }

void MEimstr::ref() {
    if (!m_ref_count) {
        m_ref_count = new int();
        (*m_ref_count) = 0;
    }
    (*m_ref_count)++;
}

void MEimstr::unref() {
    if (m_ref_count) {
        (*m_ref_count)--;
        if (*m_ref_count == 0) {
            ImGui::MemFree(m_data);
            m_data = 0;
            delete m_ref_count;
        }
        m_ref_count = 0;
    }
}

#if defined(ME_IMM32)

#include <commctrl.h>
#include <tchar.h>
#include <windows.h>

#pragma comment(linker, \
                "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

void ME_imgui_imm::operator()() {
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
            ImGui::Text("%s", static_cast<bool>(comp_conved_utf8) ? comp_conved_utf8.get() : "");
            ImGui::PopStyleColor();

            if (static_cast<bool>(comp_target_utf8)) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.203125f, 0.91796875f, 0.35546875f, 1.0f));

                target_screen_pos = ImGui::GetCursorScreenPos();
                target_screen_pos.y += ImGui::GetTextLineHeightWithSpacing();

                ImGui::Text("%s", static_cast<bool>(comp_target_utf8) ? comp_target_utf8.get() : "");
                ImGui::PopStyleColor();
            }
            if (static_cast<bool>(comp_unconv_utf8)) {
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78125f, 1.0f, 0.1875f, 1.0f));
                ImGui::Text("%s", static_cast<bool>(comp_unconv_utf8) ? comp_unconv_utf8.get() : "");
                ImGui::PopStyleColor();
            }

            // ImGui::SameLine();
            // ImGui::Text("%u %u", candidate_window_root_id, ImGui::GetCurrentContext()->NavWindow ? ImGui::GetCurrentContext()->NavWindow->RootWindow->ID : 0u);

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

            target_screen_pos.x += 100;

            ImGui::SetNextWindowPos(target_screen_pos, ImGuiCond_Always, window_pos_pivot);

            if (ImGui::Begin("##Overlay-IME-Candidate-List-Window", &show_ime_candidate_list,
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings |
                                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
                if (ImGui::BeginListBox("##IMECandidateListWindow", ImVec2(static_cast<int>(std::size(listbox_items)), static_cast<int>(std::size(listbox_items))))) {

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
                        ++i;
                    }
                    ImGui::EndListBox();
                }
                ImGui::Text("%d/%d", candidate_list.selection + 1, static_cast<int>(std::size(candidate_list.list_utf8)));
#if defined(_DEBUG)
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s",
#if defined(UNICODE)
                                   u8"DEBUG (UNICODE)"
#else
                                   u8"DEBUG (MBCS)"
#endif
                );
#endif
                candidate_window_root_id = ImGui::GetCurrentWindowRead()->RootWindow->ID;
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            }
            ImGui::End();
        }
    }

    //   io.WantTextInput 根本不可靠。
    //   在文本输入小部件中，在另一个窗口上按下鼠标按钮以
    //   其他窗口和小部件在启用 WantTextInput 的情况下处于活动状态
    //   （特别是当涉及到 IME 候选窗口的小部件时，
    //   Press 焦点移动到启用了 WantTextInput 的 IME 候选窗口。
    //   在下一帧
    //   WantTextInput 已关闭，因为 IME 候选窗口未启用文本输入
    //   然后，这里就出现了IME失效的现象。 )

    //   这与直觉行为相反，所以我们会修复它，但是当禁用它时，
    //   如果任何小部件没有获得窗口焦点
    //   当 WantTextInput 为真时启用
    //   不对称的形状减少了不适。

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        if (io.ImeWindowHandle) {
            IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
            (void)(ImmAssociateContext(static_cast<HWND>(io.ImeWindowHandle), HIMC(0)));
        }
    }
    if (io.WantTextInput) {
        if (io.ImeWindowHandle) {
            IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
            ME_ASSERT(ImmAssociateContextEx(static_cast<HWND>(io.ImeWindowHandle), HIMC(0), IACE_DEFAULT));
        }
    }

    return;
}

ME_imgui_imm::imm_candidate_list ME_imgui_imm::imm_candidate_list::cocreate(const CANDIDATELIST *const src, const size_t src_size) {
    IM_ASSERT(nullptr != src);
    IM_ASSERT(sizeof(CANDIDATELIST) <= src->dwSize);
    IM_ASSERT(src->dwSelection < src->dwCount);

    imm_candidate_list dst{};
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

bool ME_imgui_imm::update_candidate_window(HWND hWnd) {
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
                    candidate_list = std::move(imm_candidate_list::cocreate(cl, dwSize));
                    result = true;
                }
            }
        }
        ME_ASSERT(ImmReleaseContext(hWnd, hImc));
    }
    return result;
}

LRESULT
ME_imgui_imm::imm_communication_subclassproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
        case WM_DESTROY: {
            ME_ASSERT(ImmAssociateContextEx(hWnd, HIMC(0), IACE_DEFAULT));
            if (!RemoveWindowSubclass(hWnd, reinterpret_cast<SUBCLASSPROC>(uIdSubclass), uIdSubclass)) {
                IM_ASSERT(!"RemoveWindowSubclass() failed\n");
            }
        } break;
        default:
            if (dwRefData) {
                return imm_communication_subclassproc_implement(hWnd, uMsg, wParam, lParam, uIdSubclass, *reinterpret_cast<ME_imgui_imm *>(dwRefData));
            }
    }
    return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT
ME_imgui_imm::imm_communication_subclassproc_implement(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, ME_imgui_imm &comm) {
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
                ME_ASSERT(ImmReleaseContext(hWnd, hImc));
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
                                    comm.candidate_list = std::move(imm_candidate_list::cocreate(cl, dwSize));
                                }
                            }
                        }
                        ME_ASSERT(ImmReleaseContext(hWnd, hImc));
                    }
                }

                    IM_ASSERT(0 <= comm.request_candidate_list_str_commit);
                    if (comm.request_candidate_list_str_commit) {
                        if (comm.request_candidate_list_str_commit == 1) {
                            ME_ASSERT(PostMessage(hWnd, IMGUI_IMM_COMMAND, IMGUI_IMM_COMMAND_COMPOSITION_COMPLETE, 0));
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

        case IMGUI_IMM_COMMAND: {
            switch (wParam) {
                case IMGUI_IMM_COMMAND_NOP:  // NOP
                    return 1;
                case IMGUI_IMM_COMMAND_SUBCLASSIFY: {
                    ImGuiIO &io = ImGui::GetIO();
                    if (io.ImeWindowHandle) {
                        IM_ASSERT(IsWindow(static_cast<HWND>(io.ImeWindowHandle)));
                        ME_ASSERT(ImmAssociateContextEx(static_cast<HWND>(io.ImeWindowHandle), nullptr, IACE_IGNORENOCONTEXT));
                    }
                }
                    return 1;
                case IMGUI_IMM_COMMAND_COMPOSITION_COMPLETE:
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
                    ME_ASSERT(ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0));
                    ME_ASSERT(ImmReleaseContext(hWnd, hImc));
                }
#else
                        keybd_event(VK_RETURN, 0, 0, 0);
                        keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
#endif
                    } else {
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

BOOL ME_imgui_imm::subclassify_impl(HWND hWnd) {
    IM_ASSERT(IsWindow(hWnd));
    if (!IsWindow(hWnd)) {
        return FALSE;
    }

    /* 在 imgui_imm32_onthespot 中进行 IME 控制，
            因为当 TextWidget 失去焦点时 io.WantTextInput 变为 true-> off
            这时候检查一下IME的状态，如果打开就关闭。
            @see ME_imgui_imm :: operator () () 开头

            亲爱的ImGui的ImGui::IO::ImeWindowHandle原本用来指定CompositionWindow的位置
            我用了它，所以它符合目的

            但是，这种方法不好，因为当有多个目标操作系统窗口时它会崩溃。
    */
    ImGui::GetIO().ImeWindowHandle = static_cast<void *>(hWnd);
    if (::SetWindowSubclass(hWnd, ME_imgui_imm::imm_communication_subclassproc, reinterpret_cast<UINT_PTR>(ME_imgui_imm::imm_communication_subclassproc), reinterpret_cast<DWORD_PTR>(this))) {
        /*
           I want to close IME once by calling imgex::imm_associate_context_disable()

           However, at this point, default IMM Context may not have been initialized by User32.dll yet.
           Therefore, a strategy is to post the message and set the IMMContext after the message pump is turned on.
        */
        return PostMessage(hWnd, IMGUI_IMM_COMMAND, IMGUI_IMM_COMMAND_SUBCLASSIFY, 0u);
    }
    return FALSE;
}

int common_control_initialize() {
    HMODULE comctl32 = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"Comctl32.dll", &comctl32)) {
        return EXIT_FAILURE;
    }

    assert(comctl32 != nullptr);
    if (comctl32) {
        {
            typename std::add_pointer<decltype(InitCommonControlsEx)>::type lpfnInitCommonControlsEx =
                    reinterpret_cast<typename std::add_pointer<decltype(InitCommonControlsEx)>::type>(GetProcAddress(comctl32, "InitCommonControlsEx"));

            if (lpfnInitCommonControlsEx) {
                const INITCOMMONCONTROLSEX initcommoncontrolsex = {sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES};
                if (!lpfnInitCommonControlsEx(&initcommoncontrolsex)) {
                    assert(!" InitCommonControlsEx(&initcommoncontrolsex) ");
                    return EXIT_FAILURE;
                }
                // OutputDebugStringW(L"initCommonControlsEx Enable\n");
                return 0;
            }
        }
        {
            InitCommonControls();
            // OutputDebugStringW(L"initCommonControls Enable\n");
            return 0;
        }
    }
    return 1;
}

#endif

#if 1

void ShowAutoTestWindow() {

    if (ImGui::Begin("自动序列UI")) {

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
    * ME_GUI_DEFINE_INLINE(template_spec, type_spec, code)  single line definition, teplates shouldn't have commas inside!
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
        - Check imdetail namespace for other helper functions.

The libary uses partial template specialization, so definitions can overlap, the most specialized will be chosen.
However, using large, nested structs can lead to slow compilation.
Container data, large structs and tuples are hidden by default.
)comment");
            if (ImGui::TreeNode("TODO-s")) {
                ImGui::Auto(R"todos(
    1   Insert items to (non-const) set, map, list, ect
    2   Call any function or function object.
    3   Deduce function arguments as a tuple. User can edit the arguments, call the function (button), and view return value.
    4   All of the above needs self ImGui::Auto() allocated memmory. Current plan is to prioritize low memory usage
        over user experiance. (Beacuse if you want good UI, you code it, not generate it.)
        Plan A:
            a)  Data is stored in a map. Key can be the presneted object's address.
            b)  Data is allocated when not already present in the map
            c)  std::unique_ptr<void, deleter> is used in the map with deleter function manually added (or equivalent something)
            d)  The first call for ImGui::Auto at every few frames should delete
                (some of, or) all data that was not accesed in the last (few) frames. How?
        This means after closing a TreeNode, and opening it, the temporary values might be deleted and recreated.
)todos");
                ImGui::TreePop();
            }
        }
        if (myCollapsingHeader("1. String")) {
            ImGui::Indent();

            ImGui::Auto(R"code(
    ImGui::Auto("Hello ImAuto() !"); //This is how this text is written as well.)code");
            ImGui::Auto("Hello ImAuto() !");  // This is how this text is written as well.

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
    ImGui::Auto(constvec,"constvec");   //Cannot change vector, nor values)code");
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
    ImGui::Auto(map, "map");    // insert and other operations)code");
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
    ImGui::Auto(map, "map");    // insert and other operations)code");
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

            //  ImGui::Auto(R"code(
            // const std::tuple<int, const char*, ImVec2> consttuple = { 42, "smaller tuples are inlined", {3.1f,3.2f} };
            // ImGui::Auto(consttuple, "consttuple");)code");
            //  const std::tuple<int, const char*, ImVec2> consttuple = { 42, "Smaller tuples are inlined", {3.1f,3.2f} };
            //  ImGui::Auto(consttuple, "consttuple");

            ImGui::Unindent();
        }
        if (myCollapsingHeader("6. Structs!!")) {
            ImGui::Indent();

            //     ImGui::Auto(R"code(
            // struct A //Structs are automagically converted to tuples!
            // {
            //  int i = 216;
            //  bool b = true;
            // };
            // static A a;
            // ImGui::Auto("a", a);)code");
            //     struct A {
            //         int i = 216;
            //         bool b = true;
            //     };
            //     static A a;
            //     ImGui::Auto(a, "a");

            //     ImGui::NewLine();
            //     ImGui::Separator();

            //     ImGui::Auto(R"code(
            // ImGui::Auto(ImGui::as_const(a), "as_const(a)");// const structs are possible)code");
            //     ImGui::Auto(ImGui::as_const(a), "as_const(a)");

            //     ImGui::NewLine();
            //     ImGui::Separator();

            //     ImGui::Auto(R"code(
            // struct B
            // {
            //  std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
            //  const A a = A();
            // };
            // static B b;
            // ImGui::Auto(b, "b");)code");
            //     struct B {
            //         std::string str = "Unfortunatelly, cannot deduce const-ness from within a struct";
            //         const A a = A();
            //     };
            //     static B b;
            //     ImGui::Auto(b, "b");

            //     ImGui::NewLine();
            //     ImGui::Separator();

            //     ImGui::Auto(R"code(
            // static std::vector<B> vec = { {"vector of structs!", A()}, B() };
            // ImGui::Auto(vec, "vec");)code");
            //     static std::vector<B> vec = {{"vector of structs!", A()}, B()};
            //     ImGui::Auto(vec, "vec");

            //     ImGui::NewLine();
            //     ImGui::Separator();

            //     ImGui::Auto(R"code(
            // struct C
            // {
            //  std::list<B> vec;
            //  A *a;
            // };
            // static C c = { {{"Container inside a struct!", A() }}, &a };
            // ImGui::Auto(c, "c");)code");
            //     struct C {
            //         std::list<B> vec;
            //         A *a;
            //     };
            //     static C c = {{{"Container inside a struct!", A()}}, &a};
            //     ImGui::Auto(c, "c");

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
    ImGui::End();
}

#endif