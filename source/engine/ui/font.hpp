
#ifndef ME_FONTCACHE_HPP
#define ME_FONTCACHE_HPP

#include <string>

#include "engine/core/macros.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/renderer/gpu.hpp"
#include "engine/ui/fontcache.hpp"
// #include "engine/core/property.hpp"
#include "engine/utils/module.hpp"

namespace ME {

using font_index = ve_font_id;

class fontcache final : public module<fontcache> /*: public ME::props_auto_reg<ME_fontcache>*/ {
    // public:
    //     static void reg() {
    //         ME::registry::class_<ME_fontcache>()
    //                 // screen
    //                 .prop("screen_w", &ME_fontcache::screen_w, false)
    //                 .prop("screen_h", &ME_fontcache::screen_h, false);
    //     }

public:
    void drawcmd();
    font_index load(const void* data, size_t data_size, f32 font_size = 42.0f);
    void init();
    void end();
    void push(const std::string& text, const font_index font, const MEvec2 pos);
    void push(const std::string& text, const font_index font, const f32 x, const f32 y);
    void resize(MEvec2 size);
    MEvec2 calc_pos(f32 x, f32 y) const;

private:
    GLuint screen_w;
    GLuint screen_h;
};
}  // namespace ME

#endif