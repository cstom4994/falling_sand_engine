#ifndef _AUDIO_ENGINE_H_
#define _AUDIO_ENGINE_H_

#include "Core/Macros.hpp"
#include "Engine/Math.hpp"

#if defined(METADOT_BUILD_AUDIO)
#include "Engine/Internal/BuiltinFMOD.hpp"
#endif

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <vector>

#if defined(METADOT_BUILD_AUDIO)

struct Implementation
{
    Implementation();
    ~Implementation();

    void Update();

    FMOD::Studio::System *mpStudioSystem;
    FMOD::System *mpSystem;

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
    void LoadSound(const std::string &strSoundName, bool b3d = true, bool bLooping = false,
                   bool bStream = false);
    void UnLoadSound(const std::string &strSoundName);
    void Set3dListenerAndOrientation(const Vector3 &vPosition, const Vector3 &vLook,
                                     const Vector3 &vUp);
    int PlaySounds(const std::string &strSoundName, const Vector3 &vPos = Vector3{0, 0, 0},
                   float fVolumedB = 0.0f);
    void PlayEvent(const std::string &strEventName);
    FMOD::Studio::EventInstance *GetEvent(const std::string &strEventName);
    void StopChannel(int nChannelId);
    void StopEvent(const std::string &strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string &strEventName, const std::string &strEventParameter,
                           float *parameter);
    void SetEventParameter(const std::string &strEventName, const std::string &strParameterName,
                           float fValue);
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
    void LoadSound(const std::string &strSoundName, bool b3d = true, bool bLooping = false,
                   bool bStream = false);
    void UnLoadSound(const std::string &strSoundName);
    int PlaySounds(const std::string &strSoundName, const Vector3 &vPos = Vector3{0, 0, 0},
                   float fVolumedB = 0.0f);
    void PlayEvent(const std::string &strEventName);
    void StopEvent(const std::string &strEventName, bool bImmediate = false);
    void GetEventParameter(const std::string &strEventName, const std::string &strEventParameter,
                           float *parameter);
    void SetEventParameter(const std::string &strEventName, const std::string &strParameterName,
                           float fValue);
    void SetGlobalParameter(const std::string &strParameterName, float fValue);
    void GetGlobalParameter(const std::string &strEventParameter, float *parameter);
    void SetChannel3dPosition(int nChannelId, const Vector3 &vPosition);
    void SetChannelVolume(int nChannelId, float fVolumedB);
    bool IsEventPlaying(const std::string &strEventName) const;
};
#endif

#endif
