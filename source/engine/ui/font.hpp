
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

class fontcache final : public module<fontcache> /*: public ME::props_auto_reg<ME_fontcache>*/ {
    // public:
    //     static void reg() {
    //         ME::registry::class_<ME_fontcache>()
    //                 // screen
    //                 .prop("screen_w", &ME_fontcache::screen_w, false)
    //                 .prop("screen_h", &ME_fontcache::screen_h, false);
    //     }

public:
    void ME_fontcache_drawcmd();
    void ME_fontcache_load(const void* data, size_t data_size);
    int ME_fontcache_init();
    int ME_fontcache_end();
    void ME_fontcache_push(std::string& text, MEvec2 pos);
    void resize(MEvec2 size);

private:
    GLuint screen_w;
    GLuint screen_h;
};
}  // namespace ME

#endif