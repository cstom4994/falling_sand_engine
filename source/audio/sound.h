

#ifndef hashtable_h
#define hashtable_h

#ifndef HASHTABLE_U64
#define HASHTABLE_U64 unsigned long long
#endif

typedef struct hashtable_t hashtable_t;

void hashtable_init(hashtable_t* table, int item_size, int initial_capacity, void* memctx);
void hashtable_term(hashtable_t* table);

void* hashtable_insert(hashtable_t* table, HASHTABLE_U64 key, void const* item);
void hashtable_remove(hashtable_t* table, HASHTABLE_U64 key);
void hashtable_clear(hashtable_t* table);

void* hashtable_find(hashtable_t const* table, HASHTABLE_U64 key);

int hashtable_count(hashtable_t const* table);
void* hashtable_items(hashtable_t const* table);
HASHTABLE_U64 const* hashtable_keys(hashtable_t const* table);

void hashtable_swap(hashtable_t* table, int index_a, int index_b);

#endif /* hashtable_h */

/*
----------------------
    IMPLEMENTATION
----------------------
*/

#ifndef hashtable_t_h
#define hashtable_t_h

#ifndef HASHTABLE_U32
#define HASHTABLE_U32 unsigned int
#endif

struct hashtable_internal_slot_t {
    HASHTABLE_U32 key_hash;
    int item_index;
    int base_count;
};

struct hashtable_t {
    void* memctx;
    int count;
    int item_size;

    struct hashtable_internal_slot_t* slots;
    int slot_capacity;

    HASHTABLE_U64* items_key;
    int* items_slot;
    void* items_data;
    int item_capacity;

    void* swap_temp;
};

#endif /* hashtable_t_h */

// end hashtable.h

#define STB_VORBIS_HEADER_ONLY
#include "libs/external/stb_vorbis.c"

#if !defined(METAENGINE_SOUND_H)

#define METAENGINE_SOUND_FORCE_SDL

#if defined(_WIN32)
#if !defined _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#if !defined _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// -------------------------------------------------------------------------------------------------
// Error handling.

typedef enum cs_error_t {
    METAENGINE_SOUND_ERROR_NONE,
    METAENGINE_SOUND_ERROR_IMPLEMENTATION_ERROR_PLEASE_REPORT_THIS_ON_GITHUB,  // https://github.com/RandyGaul/cute_headers/issues
    METAENGINE_SOUND_ERROR_FILE_NOT_FOUND,
    METAENGINE_SOUND_ERROR_INVALID_SOUND,
    METAENGINE_SOUND_ERROR_HWND_IS_NULL,
    METAENGINE_SOUND_ERROR_DIRECTSOUND_CREATE_FAILED,
    METAENGINE_SOUND_ERROR_CREATESOUNDBUFFER_FAILED,
    METAENGINE_SOUND_ERROR_SETFORMAT_FAILED,
    METAENGINE_SOUND_ERROR_AUDIOCOMPONENTFINDNEXT_FAILED,
    METAENGINE_SOUND_ERROR_AUDIOCOMPONENTINSTANCENEW_FAILED,
    METAENGINE_SOUND_ERROR_FAILED_TO_SET_STREAM_FORMAT,
    METAENGINE_SOUND_ERROR_FAILED_TO_SET_RENDER_CALLBACK,
    METAENGINE_SOUND_ERROR_AUDIOUNITINITIALIZE_FAILED,
    METAENGINE_SOUND_ERROR_AUDIOUNITSTART_FAILED,
    METAENGINE_SOUND_ERROR_CANT_OPEN_AUDIO_DEVICE,
    METAENGINE_SOUND_ERROR_CANT_INIT_SDL_AUDIO,
    METAENGINE_SOUND_ERROR_THE_FILE_IS_NOT_A_WAV_FILE,
    METAENGINE_SOUND_ERROR_WAV_FILE_FORMAT_CHUNK_NOT_FOUND,
    METAENGINE_SOUND_ERROR_WAV_DATA_CHUNK_NOT_FOUND,
    METAENGINE_SOUND_ERROR_ONLY_PCM_WAV_FILES_ARE_SUPPORTED,
    METAENGINE_SOUND_ERROR_WAV_ONLY_MONO_OR_STEREO_IS_SUPPORTED,
    METAENGINE_SOUND_ERROR_WAV_ONLY_16_BITS_PER_SAMPLE_SUPPORTED,
    METAENGINE_SOUND_ERROR_CANNOT_SWITCH_MUSIC_WHILE_PAUSED,
    METAENGINE_SOUND_ERROR_CANNOT_CROSSFADE_WHILE_MUSIC_IS_PAUSED,
    METAENGINE_SOUND_ERROR_CANNOT_FADEOUT_WHILE_MUSIC_IS_PAUSED,
    METAENGINE_SOUND_ERROR_TRIED_TO_SET_SAMPLE_INDEX_BEYOND_THE_AUDIO_SOURCES_SAMPLE_COUNT,
    METAENGINE_SOUND_ERROR_STB_VORBIS_DECODE_FAILED,
    METAENGINE_SOUND_ERROR_OGG_UNSUPPORTED_CHANNEL_COUNT,
} cs_error_t;

const char* cs_error_as_string(cs_error_t error);

// -------------------------------------------------------------------------------------------------
// MetaEngine sound context functions.

/**
 * Pass in NULL for `os_handle`, except for the DirectSound backend this should be hwnd.
 * play_frequency_in_Hz depends on your audio file, 44100 seems to be fine.
 * buffered_samples is clamped to be at least 1024.
 */
cs_error_t cs_init(void* os_handle, unsigned play_frequency_in_Hz, int buffered_samples, void* user_allocator_context /* = NULL */);
void cs_shutdown();

/**
 * Call this function once per game-tick.
 */
void cs_update(float dt);
void cs_set_global_volume(float volume_0_to_1);
void cs_set_global_pan(float pan_0_to_1);
void cs_set_global_pause(bool true_for_paused);

/**
 * Spawns a mixing thread dedicated to mixing audio in the background.
 * If you don't call this function mixing will happen on the main-thread when you call `cs_update`.
 */
void cs_spawn_mix_thread();

/**
 * In cases where the mixing thread takes up extra CPU attention doing nothing, you can force
 * it to sleep manually. You can tune this as necessary, but it's probably not necessary for you.
 */
void cs_mix_thread_sleep_delay(int milliseconds);

/**
 * Sometimes useful for dynamic library shenanigans.
 */
void* cs_get_context_ptr();
void cs_set_context_ptr(void* ctx);

// -------------------------------------------------------------------------------------------------
// Loaded sounds.

typedef struct cs_audio_source_t cs_audio_source_t;

cs_audio_source_t* cs_load_wav(const char* path, cs_error_t* err /* = NULL */);
cs_audio_source_t* cs_read_mem_wav(const void* memory, size_t size, cs_error_t* err /* = NULL */);
void cs_free_audio_source(cs_audio_source_t* audio);

// If stb_vorbis was included *before* cute_sound go ahead and create
// some functions for dealing with OGG files.
#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H

cs_audio_source_t* cs_load_ogg(const char* path, cs_error_t* err /* = NULL */);
cs_audio_source_t* cs_read_mem_ogg(const void* memory, size_t size, cs_error_t* err /* = NULL */);

#endif

// SDL_RWops specific functions
#if defined(SDL_rwops_h_) && defined(METAENGINE_SOUND_SDL_RWOPS)

// Provides the ability to use cs_load_wav with an SDL_RWops object.
cs_audio_source_t* cs_load_wav_rw(SDL_RWops* context, cs_error_t* err /* = NULL */);

#ifdef STB_VORBIS_INCLUDE_STB_VORBIS_H

// Provides the ability to use cs_load_ogg with an SDL_RWops object.
cs_audio_source_t* cs_load_ogg_rw(SDL_RWops* rw, cs_error_t* err /* = NULL */);

#endif

#endif  // SDL_rwops_h_

// -------------------------------------------------------------------------------------------------
// Music sounds.

void cs_music_play(cs_audio_source_t* audio, float fade_in_time /* = 0 */);
void cs_music_stop(float fade_out_time /* = 0 */);
void cs_music_pause();
void cs_music_resume();
void cs_music_set_volume(float volume_0_to_1);
void cs_music_set_loop(bool true_to_loop);
void cs_music_switch_to(cs_audio_source_t* audio, float fade_out_time /* = 0 */, float fade_in_time /* = 0 */);
void cs_music_crossfade(cs_audio_source_t* audio, float cross_fade_time /* = 0 */);
uint64_t cs_music_get_sample_index();
cs_error_t cs_music_set_sample_index(uint64_t sample_index);

// -------------------------------------------------------------------------------------------------
// Playing sounds.

typedef struct cs_playing_sound_t {
    uint64_t id;
} cs_playing_sound_t;

typedef struct cs_sound_params_t {
    bool paused /* = false */;
    bool looped /* = false */;
    float volume /* = 1.0f */;
    float pan /* = 0.5f */;  // Can be from 0 to 1.
    float delay /* = 0 */;
} cs_sound_params_t;

cs_sound_params_t cs_sound_params_default();

cs_playing_sound_t cs_play_sound(cs_audio_source_t* audio, cs_sound_params_t params);

bool cs_sound_is_active(cs_playing_sound_t sound);
bool cs_sound_get_is_paused(cs_playing_sound_t sound);
bool cs_sound_get_is_looped(cs_playing_sound_t sound);
float cs_sound_get_volume(cs_playing_sound_t sound);
uint64_t cs_sound_get_sample_index(cs_playing_sound_t sound);
void cs_sound_set_is_paused(cs_playing_sound_t sound, bool true_for_paused);
void cs_sound_set_is_looped(cs_playing_sound_t sound, bool true_for_looped);
void cs_sound_set_volume(cs_playing_sound_t sound, float volume_0_to_1);
cs_error_t cs_sound_set_sample_index(cs_playing_sound_t sound, uint64_t sample_index);

void cs_set_playing_sounds_volume(float volume_0_to_1);
void cs_stop_all_playing_sounds();

// -------------------------------------------------------------------------------------------------
// C++ overloads.

#ifdef __cplusplus
#endif

#define METAENGINE_SOUND_H
#endif
