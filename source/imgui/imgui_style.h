

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
    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;

    ImGui::StyleColorsDark();

    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

    // Headers
    colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
    colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

    // Rounding
    // style.WindowPadding = ImVec2(4.0f, 4.0f);
    // style.FramePadding = ImVec2(6.0f, 4.0f);
    style.WindowRounding = 10.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
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
