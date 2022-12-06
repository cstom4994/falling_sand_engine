#ifndef _AUDIO_ENGINE_H_
#define _AUDIO_ENGINE_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Engine/Math.hpp"

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
