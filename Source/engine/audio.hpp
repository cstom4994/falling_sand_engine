// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _AUDIO_ENGINE_H_
#define _AUDIO_ENGINE_H_

#include "core/macros.h"
#include "engine/engine_cpp.h"

#if defined(METADOT_BUILD_AUDIO)
#include "engine/internal/builtin_fmod.h"
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
    void Set3dListenerAndOrientation(const Vector3 &vPosition, const Vector3 &vLook, const Vector3 &vUp);
    int PlaySounds(const std::string &strSoundName, const Vector3 &vPos = Vector3{0, 0, 0}, float fVolumedB = 0.0f);
    void PlayEvent(const std::string &strEventName);
    FMOD::Studio::EventInstance *GetEvent(const std::string &strEventName);
    void StopChannel(int nChannelId);
    void StopEvent(const std::string &strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string &strEventName, const std::string &strEventParameter, float *parameter);
    void SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue);
    void SetGlobalParameter(const std::string &strParameterName, float fValue);
    void GetGlobalParameter(const std::string &strEventParameter, float *parameter);
    void StopAllChannels();
    void SetChannel3dPosition(int nChannelId, const Vector3 &vPosition);
    void SetChannelVolume(int nChannelId, float fVolumedB);
    bool IsPlaying(int nChannelId) const;
    bool IsEventPlaying(const std::string &strEventName) const;
    float dbToVolume(float dB);
    float VolumeTodB(float volume);
    FMOD_VECTOR VectorToFmod(const Vector3 &vPosition);
};

#else

class Audio {
public:
    static void Init();
    static void Update();
    static void Shutdown();

    void LoadEvent(const std::string &strEventName);
    void LoadSound(const std::string &strSoundName, bool b3d = true, bool bLooping = false, bool bStream = false);
    void UnLoadSound(const std::string &strSoundName);
    int PlaySounds(const std::string &strSoundName, const Vector3 &vPos = Vector3{0, 0, 0}, float fVolumedB = 0.0f);
    void PlayEvent(const std::string &strEventName);
    void StopEvent(const std::string &strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string &strEventName, const std::string &strEventParameter, float *parameter);
    void SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue);
    void SetGlobalParameter(const std::string &strParameterName, float fValue);
    void GetGlobalParameter(const std::string &strEventParameter, float *parameter);
    void SetChannel3dPosition(int nChannelId, const Vector3 &vPosition);
    void SetChannelVolume(int nChannelId, float fVolumedB);
    bool IsEventPlaying(const std::string &strEventName) const;
};
#endif

#endif
