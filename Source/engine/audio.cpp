// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "audio.hpp"

#include "core/core.hpp"
#include "core/macros.h"

#if defined(METADOT_BUILD_AUDIO)

void event_instance::start() { assert(false && "Unimpl"); }

void event_instance::stop() { assert(false && "Unimpl"); }

event_instance event::instance() { assert(false && "Unimpl"); }

bank::bank(FMOD::Studio::Bank *fmod_bank) : fmod_bank(fmod_bank) {
    int num_events;
    check_err(fmod_bank->getEventCount(&num_events));
    std::cout << "Num events = " << num_events << std::endl;
    if (num_events > 0) {
        std::vector<FMOD::Studio::EventDescription *> event_descriptions(num_events);
        check_err(fmod_bank->getEventList(event_descriptions.data(), num_events, &num_events));

        for (int ii = 0; ii < num_events; ++ii) {
            // Get the event name
            int name_len;
            check_err(event_descriptions[ii]->getPath(nullptr, 0, &name_len));
            char *name_chars = new char[name_len];
            check_err(event_descriptions[ii]->getPath(name_chars, name_len, nullptr));
            std::string name(name_chars);
            delete[] name_chars;
            std::cout << "Event name = " << name << std::endl;
            event_map.insert(std::make_pair(name, event{event_descriptions[ii], name}));
        }
    }
}

event *bank::load_event(const char *path) {
    const auto cached = event_map.find(path);
    return (cached != event_map.end()) ? &cached->second : nullptr;
}

bank_manager *bank_manager_instance = nullptr;

bank_manager::bank_manager() {}

bank_manager &bank_manager::instance() {
    if (!bank_manager_instance) {
        bank_manager_instance = new bank_manager();
    }
    return *bank_manager_instance;
}

bank *bank_manager::load(const char *path) {
    const auto cached = bank_map.find(path);
    if (cached != bank_map.end()) {
        return &cached->second;
    }
    const auto system = get_fmod_system();
    FMOD::Studio::Bank *fmod_bank = nullptr;
    check_err(system->loadBankFile(path, FMOD_STUDIO_LOAD_BANK_NORMAL, &fmod_bank));

    // make the bank & insert
    bank b{fmod_bank};
    return &bank_map.insert(std::make_pair(std::string(path), std::move(b))).first->second;
}

FMOD::Studio::System *fmod_system = nullptr;
FMOD::System *lowLevelSystem = nullptr;

void init_fmod_system() {
    check_err(FMOD::Studio::System::create(&fmod_system));

    check_err(fmod_system->getCoreSystem(&lowLevelSystem));
    check_err(lowLevelSystem->setSoftwareFormat(0, FMOD_SPEAKERMODE_5POINT1, 0));

    check_err(fmod_system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0));
}

FMOD::Studio::System *get_fmod_system() {
    assert(fmod_system != nullptr);
    return fmod_system;
}

FMOD::System *get_fmod_core_system() {
    assert(lowLevelSystem != nullptr);
    return lowLevelSystem;
}

Implementation::Implementation() { init_fmod_system(); }

Implementation::~Implementation() {
    Audio::ErrorCheck(get_fmod_system()->unloadAll());
    Audio::ErrorCheck(get_fmod_system()->release());
}

void Implementation::Update() {
    std::vector<ChannelMap::iterator> pStoppedChannels;
    for (auto it = mChannels.begin(), itEnd = mChannels.end(); it != itEnd; ++it) {
        bool bIsPlaying = false;
        it->second->isPlaying(&bIsPlaying);
        if (!bIsPlaying) {
            pStoppedChannels.push_back(it);
        }
    }
    for (auto &it : pStoppedChannels) {
        mChannels.erase(it);
    }
    Audio::ErrorCheck(get_fmod_system()->update());
}

Implementation *sgpImplementation = nullptr;

void Audio::Init() { sgpImplementation = new Implementation; }

void Audio::Update() { sgpImplementation->Update(); }

void Audio::LoadSound(const std::string &strSoundName, bool b3d, bool bLooping, bool bStream) {
    auto tFoundIt = sgpImplementation->mSounds.find(strSoundName);
    if (tFoundIt != sgpImplementation->mSounds.end()) return;
    FMOD_MODE eMode = FMOD_DEFAULT;
    eMode |= b3d ? FMOD_3D : FMOD_2D;
    eMode |= bLooping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    eMode |= bStream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
    FMOD::Sound *pSound = nullptr;
    Audio::ErrorCheck(get_fmod_core_system()->createSound(strSoundName.c_str(), eMode, nullptr, &pSound));
    if (pSound) {
        sgpImplementation->mSounds[strSoundName] = pSound;
    }
}

void Audio::UnLoadSound(const std::string &strSoundName) {
    auto tFoundIt = sgpImplementation->mSounds.find(strSoundName);
    if (tFoundIt == sgpImplementation->mSounds.end()) return;
    Audio::ErrorCheck(tFoundIt->second->release());
    sgpImplementation->mSounds.erase(tFoundIt);
}

int Audio::PlaySounds(const std::string &strSoundName, const Vector3 &vPosition, float fVolumedB) {
    int nChannelId = sgpImplementation->mnNextChannelId++;
    auto tFoundIt = sgpImplementation->mSounds.find(strSoundName);
    if (tFoundIt == sgpImplementation->mSounds.end()) {
        LoadSound(strSoundName);
        tFoundIt = sgpImplementation->mSounds.find(strSoundName);
        if (tFoundIt == sgpImplementation->mSounds.end()) {
            return nChannelId;
        }
    }
    FMOD::Channel *pChannel = nullptr;
    Audio::ErrorCheck(get_fmod_core_system()->playSound(tFoundIt->second, nullptr, true, &pChannel));
    if (pChannel) {
        FMOD_MODE currMode;
        tFoundIt->second->getMode(&currMode);
        if (currMode & FMOD_3D) {
            FMOD_VECTOR position = VectorToFmod(vPosition);
            Audio::ErrorCheck(pChannel->set3DAttributes(&position, nullptr));
        }
        Audio::ErrorCheck(pChannel->setVolume(dbToVolume(fVolumedB)));
        Audio::ErrorCheck(pChannel->setPaused(false));
        sgpImplementation->mChannels[nChannelId] = pChannel;
    }
    return nChannelId;
}

void Audio::SetChannel3dPosition(int nChannelId, const Vector3 &vPosition) {
    auto tFoundIt = sgpImplementation->mChannels.find(nChannelId);
    if (tFoundIt == sgpImplementation->mChannels.end()) return;

    FMOD_VECTOR position = VectorToFmod(vPosition);
    Audio::ErrorCheck(tFoundIt->second->set3DAttributes(&position, NULL));
}

void Audio::SetChannelVolume(int nChannelId, float fVolumedB) {
    auto tFoundIt = sgpImplementation->mChannels.find(nChannelId);
    if (tFoundIt == sgpImplementation->mChannels.end()) return;

    Audio::ErrorCheck(tFoundIt->second->setVolume(dbToVolume(fVolumedB)));
}

void Audio::LoadBank(const std::string &strBankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags) {
    auto tFoundIt = sgpImplementation->mBanks.find(strBankName);
    if (tFoundIt != sgpImplementation->mBanks.end()) return;
    FMOD::Studio::Bank *pBank;
    Audio::ErrorCheck(get_fmod_system()->loadBankFile(strBankName.c_str(), flags, &pBank));
    if (pBank) {
        sgpImplementation->mBanks[strBankName] = pBank;
    }
}

FMOD::Studio::Bank *Audio::GetBank(const std::string &strBankName) { return sgpImplementation->mBanks[strBankName]; }

void Audio::LoadEvent(const std::string &strEventName) {
    auto tFoundit = sgpImplementation->mEvents.find(strEventName);
    if (tFoundit != sgpImplementation->mEvents.end()) return;
    FMOD::Studio::EventDescription *pEventDescription = NULL;
    Audio::ErrorCheck(get_fmod_system()->getEvent(strEventName.c_str(), &pEventDescription));
    if (pEventDescription) {
        FMOD::Studio::EventInstance *pEventInstance = NULL;
        Audio::ErrorCheck(pEventDescription->createInstance(&pEventInstance));
        if (pEventInstance) {
            sgpImplementation->mEvents[strEventName] = pEventInstance;
        }
    }
}

void Audio::PlayEvent(const std::string &strEventName) {
    auto tFoundit = sgpImplementation->mEvents.find(strEventName);
    if (tFoundit == sgpImplementation->mEvents.end()) {
        LoadEvent(strEventName);
        tFoundit = sgpImplementation->mEvents.find(strEventName);
        if (tFoundit == sgpImplementation->mEvents.end()) return;
    }
    tFoundit->second->start();
}

FMOD::Studio::EventInstance *Audio::GetEvent(const std::string &strEventName) {
    auto tFoundit = sgpImplementation->mEvents.find(strEventName);
    if (tFoundit == sgpImplementation->mEvents.end()) {
        LoadEvent(strEventName);
        tFoundit = sgpImplementation->mEvents.find(strEventName);
        if (tFoundit == sgpImplementation->mEvents.end()) return nullptr;
    }
    return tFoundit->second;
}

void Audio::StopEvent(const std::string &strEventName, bool bImmediate) {
    auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
    if (tFoundIt == sgpImplementation->mEvents.end()) return;
    FMOD_STUDIO_STOP_MODE eMode;
    eMode = bImmediate ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT;
    Audio::ErrorCheck(tFoundIt->second->stop(eMode));
}

bool Audio::IsEventPlaying(const std::string &strEventName) const {
    auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
    if (tFoundIt == sgpImplementation->mEvents.end()) return false;

    FMOD_STUDIO_PLAYBACK_STATE *state = NULL;
    if (tFoundIt->second->getPlaybackState(state) == FMOD_STUDIO_PLAYBACK_PLAYING) {
        return true;
    }
    return false;
}

void Audio::GetEventParameter(const std::string &strEventName, const std::string &strParameterName, float *parameter) {
    auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
    if (tFoundIt == sgpImplementation->mEvents.end()) return;
    Audio::ErrorCheck(tFoundIt->second->getParameterByName(strParameterName.c_str(), parameter));
    // CAudioEngine::ErrorCheck(pParameter->getValue(parameter));
}

void Audio::SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue) {
    auto tFoundIt = sgpImplementation->mEvents.find(strEventName);
    if (tFoundIt == sgpImplementation->mEvents.end()) return;
    Audio::ErrorCheck(tFoundIt->second->setParameterByName(strParameterName.c_str(), fValue));
}

void Audio::SetGlobalParameter(const std::string &strParameterName, float fValue) { get_fmod_system()->setParameterByName(strParameterName.c_str(), fValue); }

void Audio::GetGlobalParameter(const std::string &strParameterName, float *parameter) { get_fmod_system()->getParameterByName(strParameterName.c_str(), parameter); }

FMOD_VECTOR Audio::VectorToFmod(const Vector3 &vPosition) {
    FMOD_VECTOR fVec;
    fVec.x = vPosition.x;
    fVec.y = vPosition.y;
    fVec.z = vPosition.z;
    return fVec;
}

int Audio::ErrorCheck(FMOD_RESULT result) {
    if (result != FMOD_OK) {
        METADOT_ERROR("FMOD Error: %d", result);
        return METADOT_FAILED;
    }
    // cout << "FMOD all good" << endl;
    return METADOT_OK;
}

float Audio::dbToVolume(float dB) { return powf(10.0f, 0.05f * dB); }

float Audio::VolumeTodB(float volume) { return 20.0f * log10f(volume); }

void Audio::Shutdown() { delete sgpImplementation; }

#else

void Audio::Init() {}

void Audio::Update() {}

void Audio::LoadSound(const std::string &strSoundName, bool b3d, bool bLooping, bool bStream) {}

void Audio::UnLoadSound(const std::string &strSoundName) {}

int Audio::PlaySounds(const std::string &strSoundName, const Vector3 &vPosition, float fVolumedB) { return 1; }

void Audio::SetChannel3dPosition(int nChannelId, const Vector3 &vPosition) {}

void Audio::SetChannelVolume(int nChannelId, float fVolumedB) {}

void Audio::LoadEvent(const std::string &strEventName) {}

void Audio::PlayEvent(const std::string &strEventName) {}

void Audio::StopEvent(const std::string &strEventName, bool bImmediate) {}

bool Audio::IsEventPlaying(const std::string &strEventName) const { return false; }

void Audio::GetEventParameter(const std::string &strEventName, const std::string &strParameterName, float *parameter) {}

void Audio::SetEventParameter(const std::string &strEventName, const std::string &strParameterName, float fValue) {}

void Audio::SetGlobalParameter(const std::string &strParameterName, float fValue) {}

void Audio::GetGlobalParameter(const std::string &strParameterName, float *parameter) {}

void Audio::Shutdown() {}

#endif
