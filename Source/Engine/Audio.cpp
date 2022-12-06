#include "Audio.h"

void Audio::Init() {}

void Audio::Update() {}

void Audio::LoadSound(const std::string &strSoundName, bool b3d, bool bLooping,
                             bool bStream) {}

void Audio::UnLoadSound(const std::string &strSoundName) {}

int Audio::PlaySounds(const std::string &strSoundName, const Vector3 &vPosition,
                             float fVolumedB) {
    return 1;
}

void Audio::SetChannel3dPosition(int nChannelId, const Vector3 &vPosition) {}

void Audio::SetChannelVolume(int nChannelId, float fVolumedB) {}

void Audio::LoadEvent(const std::string &strEventName) {}

void Audio::PlayEvent(const std::string &strEventName) {}

void Audio::StopEvent(const std::string &strEventName, bool bImmediate) {}

bool Audio::IsEventPlaying(const std::string &strEventName) const { return false; }

void Audio::GetEventParameter(const std::string &strEventName,
                                     const std::string &strParameterName, float *parameter) {}

void Audio::SetEventParameter(const std::string &strEventName,
                                     const std::string &strParameterName, float fValue) {}

void Audio::SetGlobalParameter(const std::string &strParameterName, float fValue) {}

void Audio::GetGlobalParameter(const std::string &strParameterName, float *parameter) {}

void Audio::Shutdown() {}
