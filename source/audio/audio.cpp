

#include "audio.h"

#include "core/alloc.hpp"
#include "core/core.h"
#include "filesystem.h"
#include "sound.h"

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

int Audio::PlaySounds(const std::string &strSoundName, const metadot_vec3 &vPosition, float fVolumedB) {
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

void Audio::SetChannel3dPosition(int nChannelId, const metadot_vec3 &vPosition) {
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

FMOD_VECTOR Audio::VectorToFmod(const metadot_vec3 &vPosition) {
    FMOD_VECTOR fVec;
    fVec.x = vPosition.X;
    fVec.y = vPosition.Y;
    fVec.z = vPosition.Z;
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

int Audio::PlaySounds(const std::string &strSoundName, const metadot_vec3 &vPosition, float fVolumedB) { return 1; }

void Audio::SetChannel3dPosition(int nChannelId, const metadot_vec3 &vPosition) {}

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

using namespace MetaEngine;

void AudioEngineInit() {
    int more_on_emscripten = 1;
    cs_error_t err = cs_init(NULL, 44100, 1024 * more_on_emscripten, NULL);
    if (err == METAENGINE_SOUND_ERROR_NONE) {
        cs_spawn_mix_thread();
        // app->spawned_mix_thread = true;
    } else {
        METADOT_ASSERT_E(false);
    }
}

METAENGINE_Audio *metadot_audio_load_ogg(const char *path) {
    size_t size;
    void *data = fs_read_entire_file_to_memory(path, &size);
    if (data) {
        auto src = metadot_audio_load_ogg_from_memory(data, (int)size);
        METAENGINE_FW_FREE(data);
        return (METAENGINE_Audio *)src;
    } else {
        return NULL;
    }
}

METAENGINE_Audio *metadot_audio_load_wav(const char *path) {
    size_t size;
    void *data = fs_read_entire_file_to_memory(path, &size);
    if (data) {
        auto src = metadot_audio_load_wav_from_memory(data, (int)size);
        METAENGINE_FW_FREE(data);
        return (METAENGINE_Audio *)src;
    } else {
        return NULL;
    }
}

METAENGINE_Audio *metadot_audio_load_ogg_from_memory(void *memory, int byte_count) {
    auto src = cs_read_mem_ogg(memory, (size_t)byte_count, NULL);
    return (METAENGINE_Audio *)src;
}

METAENGINE_Audio *metadot_audio_load_wav_from_memory(void *memory, int byte_count) {
    auto src = cs_read_mem_wav(memory, (size_t)byte_count, NULL);
    return (METAENGINE_Audio *)src;
}

void metadot_audio_destroy(METAENGINE_Audio *audio) { cs_free_audio_source((cs_audio_source_t *)audio); }

// -------------------------------------------------------------------------------------------------

void metadot_audio_set_pan(float pan) { cs_set_global_pan(pan); }

void metadot_audio_set_global_volume(float volume) { cs_set_global_volume(volume); }

void metadot_audio_set_sound_volume(float volume) { cs_set_playing_sounds_volume(volume); }

void metadot_audio_set_pause(bool true_for_paused) { cs_set_global_pause(true_for_paused); }

// -------------------------------------------------------------------------------------------------

static inline METAENGINE_Result s_result(cs_error_t err) {
    if (err == METAENGINE_SOUND_ERROR_NONE)
        return result_success();
    else {
        Result result;
        result.code = RESULT_ERROR;
        result.details = cs_error_as_string(err);
        return result;
    }
}

void metadot_music_play(METAENGINE_Audio *audio_source, float fade_in_time) { cs_music_play((cs_audio_source_t *)audio_source, fade_in_time); }

void metadot_music_stop(float fade_out_time) { cs_music_stop(fade_out_time); }

void metadot_music_set_volume(float volume) { cs_music_set_volume(volume); }

void metadot_music_set_loop(bool true_to_loop) { cs_music_set_loop(true_to_loop); }

void metadot_music_pause() { cs_music_pause(); }

void metadot_music_resume() { cs_music_resume(); }

void metadot_music_switch_to(METAENGINE_Audio *audio_source, float fade_out_time, float fade_in_time) { return cs_music_switch_to((cs_audio_source_t *)audio_source, fade_out_time, fade_in_time); }

void metadot_music_crossfade(METAENGINE_Audio *audio_source, float cross_fade_time) { return cs_music_crossfade((cs_audio_source_t *)audio_source, cross_fade_time); }

uint64_t metadot_music_get_sample_index() { return cs_music_get_sample_index(); }

METAENGINE_Result metadot_music_set_sample_index(uint64_t sample_index) { return s_result(cs_music_set_sample_index(sample_index)); }

// -------------------------------------------------------------------------------------------------

METAENGINE_Sound metadot_play_sound(METAENGINE_Audio *audio_source, METAENGINE_SoundParams params /*= metadot_sound_params_defaults()*/, METAENGINE_Result *err) {
    cs_sound_params_t csparams;
    csparams.paused = params.paused;
    csparams.looped = params.looped;
    csparams.volume = params.volume;
    csparams.pan = params.pan;
    csparams.delay = params.delay;
    METAENGINE_Sound result;
    cs_playing_sound_t csresult = cs_play_sound((cs_audio_source_t *)audio_source, csparams);
    result.id = csresult.id;
    return result;
}

bool metadot_sound_is_active(METAENGINE_Sound sound) {
    cs_playing_sound_t cssound = {sound.id};
    return cs_sound_is_active(cssound);
}

bool metadot_sound_get_is_paused(METAENGINE_Sound sound) {
    cs_playing_sound_t cssound = {sound.id};
    return cs_sound_get_is_paused(cssound);
}

bool metadot_sound_get_is_looped(METAENGINE_Sound sound) {
    cs_playing_sound_t cssound = {sound.id};
    return cs_sound_get_is_looped(cssound);
}

float metadot_sound_get_volume(METAENGINE_Sound sound) {
    cs_playing_sound_t cssound = {sound.id};
    return cs_sound_get_volume(cssound);
}

uint64_t metadot_sound_get_sample_index(METAENGINE_Sound sound) {
    cs_playing_sound_t cssound = {sound.id};
    return cs_sound_get_sample_index(cssound);
}

void metadot_sound_set_is_paused(METAENGINE_Sound sound, bool true_for_paused) {
    cs_playing_sound_t cssound = {sound.id};
    cs_sound_set_is_paused(cssound, true_for_paused);
}

void metadot_sound_set_is_looped(METAENGINE_Sound sound, bool true_for_looped) {
    cs_playing_sound_t cssound = {sound.id};
    cs_sound_set_is_looped(cssound, true_for_looped);
}

void metadot_sound_set_volume(METAENGINE_Sound sound, float volume) {
    cs_playing_sound_t cssound = {sound.id};
    cs_sound_set_volume(cssound, volume);
}

void metadot_sound_set_sample_index(METAENGINE_Sound sound, uint64_t sample_index) {
    cs_playing_sound_t cssound = {sound.id};
    cs_sound_set_sample_index(cssound, sample_index);
}
