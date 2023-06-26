

#ifndef ME_AUDIO_H
#define ME_AUDIO_H

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/macros.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/engine.h"

//--------------------------------------------------------------------------------------------------
// C API

typedef struct ME_Audio ME_Audio;

ME_Audio *METADOT_CDECL ME_audio_load_ogg(const char *path /*= NULL*/);
ME_Audio *METADOT_CDECL ME_audio_load_wav(const char *path /*= NULL*/);
ME_Audio *METADOT_CDECL ME_audio_load_ogg_from_memory(void *memory, int byte_count);
ME_Audio *METADOT_CDECL ME_audio_load_wav_from_memory(void *memory, int byte_count);
void METADOT_CDECL ME_audio_destroy(ME_Audio *audio);

// -------------------------------------------------------------------------------------------------

void METADOT_CDECL ME_audio_set_pan(float pan);
void METADOT_CDECL ME_audio_set_global_volume(float volume);
void METADOT_CDECL ME_audio_set_sound_volume(float volume);
void METADOT_CDECL ME_audio_set_pause(bool true_for_paused);

// -------------------------------------------------------------------------------------------------

void METADOT_CDECL metadot_music_play(ME_Audio *audio_source, float fade_in_time /*= 0*/);
void METADOT_CDECL metadot_music_stop(float fade_out_time /*= 0*/);
void METADOT_CDECL metadot_music_set_volume(float volume);
void METADOT_CDECL metadot_music_set_loop(bool true_to_loop);
void METADOT_CDECL metadot_music_pause();
void METADOT_CDECL metadot_music_resume();
void METADOT_CDECL metadot_music_switch_to(ME_Audio *audio_source, float fade_out_time /*= 0*/, float fade_in_time /*= 0*/);
void METADOT_CDECL metadot_music_crossfade(ME_Audio *audio_source, float cross_fade_time /*= 0*/);
uint64_t METADOT_CDECL metadot_music_get_sample_index();
void METADOT_CDECL metadot_music_set_sample_index(uint64_t sample_index);

// -------------------------------------------------------------------------------------------------

typedef struct ME_SoundParams {
    bool paused;
    bool looped;
    float volume;
    float pan;
    float delay;
} ME_SoundParams;

typedef struct ME_Sound {
    uint64_t id;
} ME_Sound;

ME_INLINE ME_SoundParams METADOT_CDECL metadot_sound_params_defaults() {
    ME_SoundParams params;
    params.paused = false;
    params.looped = false;
    params.volume = 1.0f;
    params.pan = 0.5f;
    params.delay = 0;
    return params;
}

ME_Sound METADOT_CDECL metadot_play_sound(ME_Audio *audio_source, ME_SoundParams params /*= metadot_sound_params_defaults()*/, int *err /*= NULL*/);

bool METADOT_CDECL metadot_sound_is_active(ME_Sound sound);
bool METADOT_CDECL metadot_sound_get_is_paused(ME_Sound sound);
bool METADOT_CDECL metadot_sound_get_is_looped(ME_Sound sound);
float METADOT_CDECL metadot_sound_get_volume(ME_Sound sound);
uint64_t METADOT_CDECL metadot_sound_get_sample_index(ME_Sound sound);
void METADOT_CDECL metadot_sound_set_is_paused(ME_Sound sound, bool true_for_paused);
void METADOT_CDECL metadot_sound_set_is_looped(ME_Sound sound, bool true_for_looped);
void METADOT_CDECL metadot_sound_set_volume(ME_Sound sound, float volume);
void METADOT_CDECL metadot_sound_set_sample_index(ME_Sound sound, uint64_t sample_index);

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using Audio = ME_Audio;

struct SoundParams : public ME_SoundParams {
    SoundParams() { *(ME_SoundParams *)this = metadot_sound_params_defaults(); }
    SoundParams(ME_SoundParams sp) { *(ME_SoundParams *)this = sp; }
};

struct Sound : public ME_Sound {
    Sound() { id = -1; }
    Sound(ME_Sound s) { *(ME_Sound *)this = s; }
};

ME_INLINE Audio *audio_load_ogg(const char *path = NULL) { return ME_audio_load_ogg(path); }
ME_INLINE Audio *audio_load_wav(const char *path = NULL) { return ME_audio_load_wav(path); }
ME_INLINE Audio *audio_load_ogg_from_memory(void *memory, int byte_count) { return ME_audio_load_ogg_from_memory(memory, byte_count); }
ME_INLINE Audio *audio_load_wav_from_memory(void *memory, int byte_count) { return ME_audio_load_wav_from_memory(memory, byte_count); }
ME_INLINE void audio_destroy(Audio *audio) { ME_audio_destroy(audio); }

// -------------------------------------------------------------------------------------------------

ME_INLINE void audio_set_pan(float pan) { ME_audio_set_pan(pan); }
ME_INLINE void audio_set_global_volume(float volume) { ME_audio_set_global_volume(volume); }
ME_INLINE void audio_set_sound_volume(float volume) { ME_audio_set_sound_volume(volume); }
ME_INLINE void audio_set_pause(bool true_for_paused) { ME_audio_set_pause(true_for_paused); }

// -------------------------------------------------------------------------------------------------

ME_INLINE void music_play(Audio *audio_source, float fade_in_time = 0) { metadot_music_play(audio_source, fade_in_time); }
ME_INLINE void music_stop(float fade_out_time = 0) { metadot_music_stop(fade_out_time = 0); }
ME_INLINE void music_set_volume(float volume) { metadot_music_set_volume(volume); }
ME_INLINE void music_set_loop(bool true_to_loop) { metadot_music_set_loop(true_to_loop); }
ME_INLINE void music_pause() { metadot_music_pause(); }
ME_INLINE void music_resume() { metadot_music_resume(); }
ME_INLINE void music_switch_to(Audio *audio_source, float fade_out_time = 0, float fade_in_time = 0) { metadot_music_switch_to(audio_source, fade_out_time, fade_in_time); }
ME_INLINE void music_crossfade(Audio *audio_source, float cross_fade_time = 0) { metadot_music_crossfade(audio_source, cross_fade_time); }
ME_INLINE void music_set_sample_index(uint64_t sample_index) { metadot_music_set_sample_index(sample_index); }
ME_INLINE uint64_t music_get_sample_index() { return metadot_music_get_sample_index(); }

// -------------------------------------------------------------------------------------------------

ME_INLINE Sound sound_play(Audio *audio_source, SoundParams params = SoundParams(), int *err = NULL) { return metadot_play_sound(audio_source, params, err); }

ME_INLINE bool sound_is_active(Sound sound) { return metadot_sound_is_active(sound); }
ME_INLINE bool sound_get_is_paused(Sound sound) { return metadot_sound_get_is_paused(sound); }
ME_INLINE bool sound_get_is_looped(Sound sound) { return metadot_sound_get_is_looped(sound); }
ME_INLINE float sound_get_volume(Sound sound) { return metadot_sound_get_volume(sound); }
ME_INLINE uint64_t sound_get_sample_index(Sound sound) { return metadot_sound_get_sample_index(sound); }
ME_INLINE void sound_set_is_paused(Sound sound, bool true_for_paused) { metadot_sound_set_is_paused(sound, true_for_paused); }
ME_INLINE void sound_set_is_looped(Sound sound, bool true_for_looped) { metadot_sound_set_is_looped(sound, true_for_looped); }
ME_INLINE void sound_set_volume(Sound sound, float volume) { metadot_sound_set_volume(sound, volume); }
ME_INLINE void sound_set_sample_index(Sound sound, int sample_index) { metadot_sound_set_sample_index(sound, sample_index); }

}  // namespace MetaEngine

class AudioEngine {
private:
    std::map<std::string, MetaEngine::Audio *> audio_list = {};
    std::map<std::string, MetaEngine::Sound> sound_list = {};

public:
    void InitAudio();
    void EndAudio();

    void LoadEvent(const std::string &strEventName, const std::string &filepath);
    void LoadSound(const std::string &strSoundName, bool b3d = true, bool bLooping = false, bool bStream = false);
    void UnLoadSound(const std::string &strSoundName);
    int PlaySounds(const std::string &strSoundName, const MEvec3 &vPos = MEvec3{0, 0, 0}, float fVolumedB = 0.0f);
    void PlayEvent(const std::string &strEventName);
    void StopEvent(const std::string &strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string &strEventName, const std::string &strEventParameter, float *parameter);
    void SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue);
    void SetGlobalParameter(const std::string &strParameterName, float fValue);
    void GetGlobalParameter(const std::string &strEventParameter, float *parameter);
    void SetChannel3dPosition(int nChannelId, const MEvec3 &vPosition);
    void SetChannelVolume(int nChannelId, float fVolumedB);
    bool IsEventPlaying(const std::string &strEventName) const;
};

#endif  // ME_AUDIO_H
