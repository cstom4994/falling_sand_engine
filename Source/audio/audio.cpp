

#include "audio.h"

#include "core/alloc.h"
#include "core/core.h"
#include "filesystem.h"
#include "sound.h"

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

METAENGINE_Audio* metadot_audio_load_ogg(const char* path) {
    size_t size;
    void* data = fs_read_entire_file_to_memory(path, &size);
    if (data) {
        auto src = metadot_audio_load_ogg_from_memory(data, (int)size);
        METAENGINE_FW_FREE(data);
        return (METAENGINE_Audio*)src;
    } else {
        return NULL;
    }
}

METAENGINE_Audio* metadot_audio_load_wav(const char* path) {
    size_t size;
    void* data = fs_read_entire_file_to_memory(path, &size);
    if (data) {
        auto src = metadot_audio_load_wav_from_memory(data, (int)size);
        METAENGINE_FW_FREE(data);
        return (METAENGINE_Audio*)src;
    } else {
        return NULL;
    }
}

METAENGINE_Audio* metadot_audio_load_ogg_from_memory(void* memory, int byte_count) {
    auto src = cs_read_mem_ogg(memory, (size_t)byte_count, NULL);
    return (METAENGINE_Audio*)src;
}

METAENGINE_Audio* metadot_audio_load_wav_from_memory(void* memory, int byte_count) {
    auto src = cs_read_mem_wav(memory, (size_t)byte_count, NULL);
    return (METAENGINE_Audio*)src;
}

void metadot_audio_destroy(METAENGINE_Audio* audio) { cs_free_audio_source((cs_audio_source_t*)audio); }

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

void metadot_music_play(METAENGINE_Audio* audio_source, float fade_in_time) { cs_music_play((cs_audio_source_t*)audio_source, fade_in_time); }

void metadot_music_stop(float fade_out_time) { cs_music_stop(fade_out_time); }

void metadot_music_set_volume(float volume) { cs_music_set_volume(volume); }

void metadot_music_set_loop(bool true_to_loop) { cs_music_set_loop(true_to_loop); }

void metadot_music_pause() { cs_music_pause(); }

void metadot_music_resume() { cs_music_resume(); }

void metadot_music_switch_to(METAENGINE_Audio* audio_source, float fade_out_time, float fade_in_time) { return cs_music_switch_to((cs_audio_source_t*)audio_source, fade_out_time, fade_in_time); }

void metadot_music_crossfade(METAENGINE_Audio* audio_source, float cross_fade_time) { return cs_music_crossfade((cs_audio_source_t*)audio_source, cross_fade_time); }

uint64_t metadot_music_get_sample_index() { return cs_music_get_sample_index(); }

METAENGINE_Result metadot_music_set_sample_index(uint64_t sample_index) { return s_result(cs_music_set_sample_index(sample_index)); }

// -------------------------------------------------------------------------------------------------

METAENGINE_Sound metadot_play_sound(METAENGINE_Audio* audio_source, METAENGINE_SoundParams params /*= metadot_sound_params_defaults()*/, METAENGINE_Result* err) {
    cs_sound_params_t csparams;
    csparams.paused = params.paused;
    csparams.looped = params.looped;
    csparams.volume = params.volume;
    csparams.pan = params.pan;
    csparams.delay = params.delay;
    METAENGINE_Sound result;
    cs_playing_sound_t csresult = cs_play_sound((cs_audio_source_t*)audio_source, csparams);
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
