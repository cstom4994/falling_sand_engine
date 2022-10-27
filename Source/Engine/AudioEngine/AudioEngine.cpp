#include "AudioEngine.h"

void CAudioEngine::Init() {
}

void CAudioEngine::Update() {
}

void CAudioEngine::LoadSound(const std::string &strSoundName, bool b3d, bool bLooping, bool bStream) {
}

void CAudioEngine::UnLoadSound(const std::string &strSoundName) {
}

int CAudioEngine::PlaySounds(const std::string &strSoundName, const Vector3 &vPosition, float fVolumedB) {
    return 1;
}

void CAudioEngine::SetChannel3dPosition(int nChannelId, const Vector3 &vPosition) {
}

void CAudioEngine::SetChannelVolume(int nChannelId, float fVolumedB) {
}

void CAudioEngine::LoadEvent(const std::string &strEventName) {
}

void CAudioEngine::PlayEvent(const std::string &strEventName) {
}


void CAudioEngine::StopEvent(const std::string &strEventName, bool bImmediate) {
}

bool CAudioEngine::IsEventPlaying(const std::string &strEventName) const {
    return false;
}

void CAudioEngine::GetEventParameter(const std::string &strEventName, const std::string &strParameterName, float *parameter) {
}

void CAudioEngine::SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue) {
}

void CAudioEngine::SetGlobalParameter(const std::string &strParameterName, float fValue) {
}

void CAudioEngine::GetGlobalParameter(const std::string &strParameterName, float *parameter) {
}

void CAudioEngine::Shutdown() {
}
