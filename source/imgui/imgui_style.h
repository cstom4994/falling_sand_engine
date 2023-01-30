

#ifndef _METADOT_IMGUICSS_STYLE_H__
#define _METADOT_IMGUICSS_STYLE_H__

#if __APPLE__
#define DEFAULT_DPI 72.0f
#else
#define DEFAULT_DPI 96.0f
#endif

extern "C" {
#ifndef restrict
#define restrict
#endif
#include <libcss/libcss.h>
}

#include <iostream>

#include "imgui/imgui_impl.hpp"

inline void ImGuiInitStyle(const float pixel_ratio, const float dpi_scaling) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.3f;
    style.FrameRounding = 2.3f;
    style.ScrollbarRounding = 0;

    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.99f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

struct css_computed_style;
struct css_select_ctx;

namespace ImGuiCSS {

ImGuiWindow* GetCurrentWindowNoDefault();  // like GetCurrentWindowRead, but returns NULL if current window is fallback window

class Element;
class ComputedStyle;
class Context;

typedef bool (*styleCallback)(ComputedStyle& style);

inline ImU32 parseColor(css_color color) {
    return (((color >> 16) & 0xFF) << IM_COL32_R_SHIFT) | (((color >> 8) & 0xFF) << IM_COL32_G_SHIFT) | (((color >> 0) & 0xFF) << IM_COL32_B_SHIFT) | (((color >> 24) & 0xFF) << IM_COL32_A_SHIFT);
}

enum ParseUnitsAxis { X, Y };

float parseUnits(css_fixed value, css_unit unit, ComputedStyle& style, const ImVec2& parentSize, ParseUnitsAxis axis);

bool setDimensions(ComputedStyle& style);

bool setPosition(ComputedStyle& style);

bool setColor(ComputedStyle& style);

bool setBackgroundColor(ComputedStyle& style);

bool setPadding(ComputedStyle& style);

bool setMargins(ComputedStyle& style);

bool setFont(ComputedStyle& style);

bool setBorderRadius(ComputedStyle& style);

bool setBorder(ComputedStyle& style);

class Style;
/**
 * Style state
 */
class ComputedStyle {
public:
    ComputedStyle(Element* target);
    ~ComputedStyle();
    /**
     * Compute style for an element
     *
     * @param element Target element
     */
    bool compute(Element* element);

    /**
     * Apply computed style
     */
    void begin(Element* element);

    /**
     * Roll back any changes
     */
    void end();

    void destroy();

    void updateClassCache();

    void setInlineStyle(const char* data);

    ImVector<lwc_string*>& getClasses();

    // override copy constructor
    ComputedStyle(ComputedStyle& other)
        : libcssData(0), fontName(0), fontSize(0), element(other.element), style(0), context(0), nCol(0), nStyle(0), nFonts(0), fontScale(0), mSelectCtx(0), mInlineStyle(0), mAutoSize(false) {
        compute(element);
    }

    void* libcssData;
    char* fontName;
    float fontSize;
    uint16_t position;

    Element* element;

    css_select_results* style;

    ImVec2 elementScreenPosition;
    ImVec2 contentRegion;
    ImVec2 parentSize;
    ImVec4 borderRadius;

    Context* context;

    struct Decoration {
        void renderBackground(ImRect rect);
        void render(ImRect rect);

        void drawJoint(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p1i, const ImVec2& p2i, const ImVec2& p3i, ImU32 startColor, ImU32 endColor, int numSegments);

        void drawJoint(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& center, ImU32 startColor, ImU32 endColor, const ImVec2& startPoint, const ImVec2& endPoint, int numSegments);

        float thickness[4];
        float rounding[4];
        ImU32 col[4];
        ImU32 bgCol;
    };

    Decoration decoration;
    // style stack
    int nCol;
    int nStyle;
    int nFonts;
    float fontScale;

private:
    void initFonts(css_media* media);

    void cleanupClassCache();

    friend class Style;
    ImVector<styleCallback> mStyleCallbacks;
    css_select_ctx* mSelectCtx;
    ImVector<lwc_string*> mClasses;
    uint16_t mWidthMode;
    uint16_t mHeightMode;
    css_stylesheet* mInlineStyle;
    bool mAutoSize;
};

/**
 * Wrapper for libcss
 */
class Style {
public:
    struct Sheet {
        css_stylesheet* sheet;
        bool scoped;
    };

    Style(Style* parent = 0);
    ~Style();

    css_select_ctx* select();

    /**
     * Parse sheet and append it to sheet list
     *
     * @param data raw sheet data
     * @param scoped adds list to local
     */
    void load(const char* data, bool scoped = false);

    void parse(const char* data, css_stylesheet** dest, bool isInline = false);

private:
    css_stylesheet* mBase;
    ImVector<Sheet> mSheets;

    Style* mParent;

    void appendSheets(css_select_ctx* ctx, bool scoped = false);
};
}  // namespace ImGuiCSS

#endif
