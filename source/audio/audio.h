

#ifndef METAENGINE_AUDIO_H
#define METAENGINE_AUDIO_H

#include "core/core.h"
#include "core/macros.h"
#include "core/stl.h"
#include "engine.h"
#include "mathlib.h"

#if defined(METADOT_BUILD_AUDIO)
#include "internal/builtin_fmod.h"
#endif

#include <math.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#if defined(METADOT_BUILD_AUDIO)

#pragma region FMODWrapper

#include <string>
#include <unordered_map>

class event;

class event_instance {
    event *type;

    void start();
    void stop();
};

namespace FMOD {
namespace Studio {
class EventDescription;
}
}  // namespace FMOD

/**
   Models FMOD event descriptions
*/
struct event {
    FMOD::Studio::EventDescription *fmod_bank;
    std::string path;

    event_instance instance();
};

namespace FMOD {
namespace Studio {
class Bank;
}
}  // namespace FMOD

struct bank {
    /** Instance of the fmod bank object */
    FMOD::Studio::Bank *fmod_bank;

    std::unordered_map<std::string, event> event_map;

    bank(FMOD::Studio::Bank *fmod_bank);

    /** Load an event description, caching if already loaded */
    event *load_event(const char *path);
};

/** Singleton for storing banks */
class bank_manager {
    std::unordered_map<std::string, bank> bank_map;

    bank_manager();

public:
    static bank_manager &instance();

    /** Unload a bank */
    void unload(const char *bank_path);

    /** Returns a pointer to the bank if loaded, or null for a failure (error
      printed to console). Caches, so future calls will load the same bank. */
    bank *load(const char *path);
};

/** Initialise the fmod system */
void init_fmod_system();

/** Get the fmod system - asserts if not initialised */
FMOD::Studio::System *get_fmod_system();
FMOD::System *get_fmod_core_system();

struct fmod_exception {
    const char *message;
};

/** Checks the result, printing it out and throwing an 'fmod_exception' if the
    result is an error. */
inline void check_err(FMOD_RESULT res) {
    if (res != FMOD_OK) {
        throw fmod_exception{FMOD_ErrorString(res)};
    }
}

#pragma endregion FMODWrapper

struct Implementation {
    Implementation();
    ~Implementation();

    void Update();

    int mnNextChannelId;

    typedef std::map<std::string, FMOD::Sound *> SoundMap;
    typedef std::map<int, FMOD::Channel *> ChannelMap;
    typedef std::map<std::string, FMOD::Studio::EventInstance *> EventMap;
    typedef std::map<std::string, FMOD::Studio::Bank *> BankMap;

    BankMap mBanks;
    EventMap mEvents;
    SoundMap mSounds;
    ChannelMap mChannels;
};

class Audio {
public:
    static void Init();
    static void Update();
    static void Shutdown();
    static int ErrorCheck(FMOD_RESULT result);

    void LoadBank(const std::string &strBankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags);
    FMOD::Studio::Bank *GetBank(const std::string &strBankName);
    void LoadEvent(const std::string &strEventName);
    void LoadSound(const std::string &strSoundName, bool b3d = true, bool bLooping = false, bool bStream = false);
    void UnLoadSound(const std::string &strSoundName);
    void Set3dListenerAndOrientation(const metadot_vec3 &vPosition, const metadot_vec3 &vLook, const metadot_vec3 &vUp);
    int PlaySounds(const std::string &strSoundName, const metadot_vec3 &vPos = metadot_vec3{0, 0, 0}, float fVolumedB = 0.0f);
    void PlayEvent(const std::string &strEventName);
    FMOD::Studio::EventInstance *GetEvent(const std::string &strEventName);
    void StopChannel(int nChannelId);
    void StopEvent(const std::string &strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string &strEventName, const std::string &strEventParameter, float *parameter);
    void SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue);
    void SetGlobalParameter(const std::string &strParameterName, float fValue);
    void GetGlobalParameter(const std::string &strEventParameter, float *parameter);
    void StopAllChannels();
    void SetChannel3dPosition(int nChannelId, const metadot_vec3 &vPosition);
    void SetChannelVolume(int nChannelId, float fVolumedB);
    bool IsPlaying(int nChannelId) const;
    bool IsEventPlaying(const std::string &strEventName) const;
    float dbToVolume(float dB);
    float VolumeTodB(float volume);
    FMOD_VECTOR VectorToFmod(const metadot_vec3 &vPosition);
};

#else

class Audio {
public:
    static void Init();
    static void Update();
    static void Shutdown();

    void LoadEvent(const std::string& strEventName);
    void LoadSound(const std::string& strSoundName, bool b3d = true, bool bLooping = false, bool bStream = false);
    void UnLoadSound(const std::string& strSoundName);
    int PlaySounds(const std::string& strSoundName, const metadot_vec3& vPos = metadot_vec3{0, 0, 0}, float fVolumedB = 0.0f);
    void PlayEvent(const std::string& strEventName);
    void StopEvent(const std::string& strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string& strEventName, const std::string& strEventParameter, float* parameter);
    void SetEventParameter(const std::string& strEventName, const std::string& strParameterName, float fValue);
    void SetGlobalParameter(const std::string& strParameterName, float fValue);
    void GetGlobalParameter(const std::string& strEventParameter, float* parameter);
    void SetChannel3dPosition(int nChannelId, const metadot_vec3& vPosition);
    void SetChannelVolume(int nChannelId, float fVolumedB);
    bool IsEventPlaying(const std::string& strEventName) const;
};
#endif

void AudioEngineInit();

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct METAENGINE_Audio METAENGINE_Audio;

METAENGINE_Audio *METADOT_CDECL metadot_audio_load_ogg(const char *path /*= NULL*/);
METAENGINE_Audio *METADOT_CDECL metadot_audio_load_wav(const char *path /*= NULL*/);
METAENGINE_Audio *METADOT_CDECL metadot_audio_load_ogg_from_memory(void *memory, int byte_count);
METAENGINE_Audio *METADOT_CDECL metadot_audio_load_wav_from_memory(void *memory, int byte_count);
void METADOT_CDECL metadot_audio_destroy(METAENGINE_Audio *audio);

// -------------------------------------------------------------------------------------------------

void METADOT_CDECL metadot_audio_set_pan(float pan);
void METADOT_CDECL metadot_audio_set_global_volume(float volume);
void METADOT_CDECL metadot_audio_set_sound_volume(float volume);
void METADOT_CDECL metadot_audio_set_pause(bool true_for_paused);

// -------------------------------------------------------------------------------------------------

void METADOT_CDECL metadot_music_play(METAENGINE_Audio *audio_source, float fade_in_time /*= 0*/);
void METADOT_CDECL metadot_music_stop(float fade_out_time /*= 0*/);
void METADOT_CDECL metadot_music_set_volume(float volume);
void METADOT_CDECL metadot_music_set_loop(bool true_to_loop);
void METADOT_CDECL metadot_music_pause();
void METADOT_CDECL metadot_music_resume();
void METADOT_CDECL metadot_music_switch_to(METAENGINE_Audio *audio_source, float fade_out_time /*= 0*/, float fade_in_time /*= 0*/);
void METADOT_CDECL metadot_music_crossfade(METAENGINE_Audio *audio_source, float cross_fade_time /*= 0*/);
uint64_t METADOT_CDECL metadot_music_get_sample_index();
METAENGINE_Result METADOT_CDECL metadot_music_set_sample_index(uint64_t sample_index);

// -------------------------------------------------------------------------------------------------

typedef struct METAENGINE_SoundParams {
    bool paused;
    bool looped;
    float volume;
    float pan;
    float delay;
} METAENGINE_SoundParams;

typedef struct METAENGINE_Sound {
    uint64_t id;
} METAENGINE_Sound;

METADOT_INLINE METAENGINE_SoundParams METADOT_CDECL metadot_sound_params_defaults() {
    METAENGINE_SoundParams params;
    params.paused = false;
    params.looped = false;
    params.volume = 1.0f;
    params.pan = 0.5f;
    params.delay = 0;
    return params;
}

METAENGINE_Sound METADOT_CDECL metadot_play_sound(METAENGINE_Audio *audio_source, METAENGINE_SoundParams params /*= metadot_sound_params_defaults()*/, METAENGINE_Result *err /*= NULL*/);

bool METADOT_CDECL metadot_sound_is_active(METAENGINE_Sound sound);
bool METADOT_CDECL metadot_sound_get_is_paused(METAENGINE_Sound sound);
bool METADOT_CDECL metadot_sound_get_is_looped(METAENGINE_Sound sound);
float METADOT_CDECL metadot_sound_get_volume(METAENGINE_Sound sound);
uint64_t METADOT_CDECL metadot_sound_get_sample_index(METAENGINE_Sound sound);
void METADOT_CDECL metadot_sound_set_is_paused(METAENGINE_Sound sound, bool true_for_paused);
void METADOT_CDECL metadot_sound_set_is_looped(METAENGINE_Sound sound, bool true_for_looped);
void METADOT_CDECL metadot_sound_set_volume(METAENGINE_Sound sound, float volume);
void METADOT_CDECL metadot_sound_set_sample_index(METAENGINE_Sound sound, uint64_t sample_index);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using Audio = METAENGINE_Audio;

struct SoundParams : public METAENGINE_SoundParams {
    SoundParams() { *(METAENGINE_SoundParams *)this = metadot_sound_params_defaults(); }
    SoundParams(METAENGINE_SoundParams sp) { *(METAENGINE_SoundParams *)this = sp; }
};

struct Sound : public METAENGINE_Sound {
    Sound() { id = -1; }
    Sound(METAENGINE_Sound s) { *(METAENGINE_Sound *)this = s; }
};

METADOT_INLINE Audio *audio_load_ogg(const char *path = NULL) { return metadot_audio_load_ogg(path); }
METADOT_INLINE Audio *audio_load_wav(const char *path = NULL) { return metadot_audio_load_wav(path); }
METADOT_INLINE Audio *audio_load_ogg_from_memory(void *memory, int byte_count) { return metadot_audio_load_ogg_from_memory(memory, byte_count); }
METADOT_INLINE Audio *audio_load_wav_from_memory(void *memory, int byte_count) { return metadot_audio_load_wav_from_memory(memory, byte_count); }
METADOT_INLINE void audio_destroy(Audio *audio) { metadot_audio_destroy(audio); }

// -------------------------------------------------------------------------------------------------

METADOT_INLINE void audio_set_pan(float pan) { metadot_audio_set_pan(pan); }
METADOT_INLINE void audio_set_global_volume(float volume) { metadot_audio_set_global_volume(volume); }
METADOT_INLINE void audio_set_sound_volume(float volume) { metadot_audio_set_sound_volume(volume); }
METADOT_INLINE void audio_set_pause(bool true_for_paused) { metadot_audio_set_pause(true_for_paused); }

// -------------------------------------------------------------------------------------------------

METADOT_INLINE void music_play(Audio *audio_source, float fade_in_time = 0) { metadot_music_play(audio_source, fade_in_time); }
METADOT_INLINE void music_stop(float fade_out_time = 0) { metadot_music_stop(fade_out_time = 0); }
METADOT_INLINE void music_set_volume(float volume) { metadot_music_set_volume(volume); }
METADOT_INLINE void music_set_loop(bool true_to_loop) { metadot_music_set_loop(true_to_loop); }
METADOT_INLINE void music_pause() { metadot_music_pause(); }
METADOT_INLINE void music_resume() { metadot_music_resume(); }
METADOT_INLINE void music_switch_to(Audio *audio_source, float fade_out_time = 0, float fade_in_time = 0) { metadot_music_switch_to(audio_source, fade_out_time, fade_in_time); }
METADOT_INLINE void music_crossfade(Audio *audio_source, float cross_fade_time = 0) { metadot_music_crossfade(audio_source, cross_fade_time); }
METADOT_INLINE void music_set_sample_index(uint64_t sample_index) { metadot_music_set_sample_index(sample_index); }
METADOT_INLINE uint64_t music_get_sample_index() { return metadot_music_get_sample_index(); }

// -------------------------------------------------------------------------------------------------

METADOT_INLINE Sound sound_play(Audio *audio_source, SoundParams params = SoundParams(), Result *err = NULL) { return metadot_play_sound(audio_source, params, err); }

METADOT_INLINE bool sound_is_active(Sound sound) { return metadot_sound_is_active(sound); }
METADOT_INLINE bool sound_get_is_paused(Sound sound) { return metadot_sound_get_is_paused(sound); }
METADOT_INLINE bool sound_get_is_looped(Sound sound) { return metadot_sound_get_is_looped(sound); }
METADOT_INLINE float sound_get_volume(Sound sound) { return metadot_sound_get_volume(sound); }
METADOT_INLINE uint64_t sound_get_sample_index(Sound sound) { return metadot_sound_get_sample_index(sound); }
METADOT_INLINE void sound_set_is_paused(Sound sound, bool true_for_paused) { metadot_sound_set_is_paused(sound, true_for_paused); }
METADOT_INLINE void sound_set_is_looped(Sound sound, bool true_for_looped) { metadot_sound_set_is_looped(sound, true_for_looped); }
METADOT_INLINE void sound_set_volume(Sound sound, float volume) { metadot_sound_set_volume(sound, volume); }
METADOT_INLINE void sound_set_sample_index(Sound sound, int sample_index) { metadot_sound_set_sample_index(sound, sample_index); }

}  // namespace MetaEngine

#endif  // METAENGINE_AUDIO_H
