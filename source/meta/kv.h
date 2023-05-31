

#ifndef METADOT_KV_H
#define METADOT_KV_H

#include "core/stl/stl.h"

//--------------------------------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * "Key-value", or KV for short, is a serialization API designed to be very similar to JSON. It's a
 * text-based format to store any kind of data in key-value format. This key-value style has some
 * great advantages for serialziation, such as versioning your data, simplicity (human readable),
 * and a low-friction API.
 *
 * You start by opening a KV in either read or write format. For 95% of use cases you can then get
 * away with using the same code for reading and writing data, with only the difference of opening
 * in read/write mode with `metadot_kv_read`/`metadot_kv_write`.
 *
 * Here's a quick example of some KV data.
 *
 *     size = 10,
 *     name = "Clarence",
 *     data = {
 *         x = 5,
 *         z = 15.0,
 *     },
 *     blob = "SGVsbG8gdGhlcmUgOik=",
 *     codes = [3] {
 *         7, -3, 20,
 *     }
 *
 * Here are the supported data types in KV:
 *
 *     - integers
 *     - floats
 *     - strings
 *     - base64 blobs, as strings
 *     - arrays
 *     - objects
 *
 * Reading data with KV is all about calling `metadot_kv_key(kv, "key_name")` to lookup a serialized
 * field. If found, you can call the proper `kv_val_***` function to fetch the data for that key.
 * For example, let's say we have this struct in our game containing some important info.
 *
 *     struct ImportantStuff
 *     {
 *         int state;
 *         float factor;
 *         int value_count;
 *         int values[MAX_ENTRIES];
 *     };
 *
 * We could save the `ImportantStuff` to a KV text file with some code like this:
 *
 *     bool serialize(METADOT_KeyValue* kv, ImportantStuff* stuff)
 *     {
 *         if (metadot_kv_key("state"))       metadot_kv_val_int32(kv, &stuff->state);
 *         if (metadot_kv_key("factor"))      metadot_kv_val_float(kv, &stuff->factor);
 *         if (metadot_kv_key("value_count")) metadot_kv_val_int32(kv, &stuff->value_count);
 *
 *         if (metadot_kv_object_begin(kv, &stuff->value_count, "values")) {
 *             for (int i = 0; i < stuff->value_count; ++i) {
 *                 metadot_kv_val_int32(kv, stuff->values + i);
 *             }
 *             metadot_kv_object_end(kv);
 *         }
 *
 *         return !metadot_is_error(metadot_kv_last_error(kv));
 *     }
 *
 *     bool save(ImportantStuff* stuff, const char* path)
 *     {
 *          METADOT_KeyValue* kv = kv_write();
 *          if (!serialize(kv, stuff)) {
 *              metadot_kv_destroy(kv);
 *              return NULL;
 *          }
 *          bool result = !metadot_is_error(ME_fs_write_entire_buffer_to_file(metadot_kv_buffer(kv), metadot_kv_buffer_size(kv)));
 *          metadot_kv_destroy(kv);
 *          return result;
 *     }
 *
 * The text file would then look something like this:
 *
 *     state = 10
 *     factor = 1.3
 *     value_count = 3
 *     values = [3] {
 *         7, -3, 20,
 *     }
 *
 * The bulk of the work happens in the `serialize` function from the example above. We can
 * reuse this entire function to also read back the KV data into an `ImportantStuff` struct
 * by making a similar function to `save`.
 *
 *     bool load(ImportantStuff* stuff, const char* path)
 *     {
 *          void* data;
 *          size_t size;
 *          if (metadot_is_error(ME_fs_read_entire_file_to_memory(path, &data, &size))) return false;
 *          METADOT_KeyValue* kv = kv_read(data, size);
 *          if (!kv) return false;
 *          bool result = serialize(kv, stuff);
 *          metadot_kv_destroy(kv);
 *          return result;
 *     }
 *
 * In the common case it's possible to reuse most seriaization code for both reading and
 * writing. However, sometimes it's necessary to have entirely different code for reading
 * and writing. Use `metadot_kv_state` to see if the KV is currently in read vs write mode, and then
 * run different code accordingly.
 */

typedef struct METADOT_KeyValue METADOT_KeyValue;

#define METADOT_KV_STATE_DEFS       \
    METADOT_ENUM(KV_STATE_WRITE, 0) \
    METADOT_ENUM(KV_STATE_READ, 1)

typedef enum METADOT_KeyValueState {
#define METADOT_ENUM(K, V) METADOT_##K = V,
    METADOT_KV_STATE_DEFS
#undef METADOT_ENUM
} METADOT_KeyValueState;

ME_INLINE const char* metadot_key_value_state_to_string(METADOT_KeyValueState state) {
    switch (state) {
#define METADOT_ENUM(K, V) \
    case METADOT_##K:      \
        return METAENGINE_STRINGIZE(METADOT_##K);
        METADOT_KV_STATE_DEFS
#undef METADOT_ENUM
        default:
            return NULL;
    }
}

/**
 * Parses a buffer of kv data. All data is loaded up into memory at once. You can fetch out values
 * as-needed by using `metadot_kv_key` and `metadot_kv_val_***` functions.
 *
 * Example:
 *
 *     const char* data =
 *         "a = 10,\n"
 *         "b = 13,\n"
 *     ;
 *     size_t len = METAENGINE_STRLEN(string);
 *
 *     METADOT_KeyValue* kv = metadot_kv_parse((void*)string, len, NULL);
 *
 *     int val;
 *     if (metadot_kv_key(kv, "a")) {
 *         metadot_kv_val(kv, &val);
 *         printf("a was %d\n", val);
 *     }
 *     if (metadot_kv_key(kv, "b")) {
 *         metadot_kv_val(kv, &val);
 *         printf("b was %d\n", val);
 *     }
 *
 *     metadot_kv_destroy(kv);
 *
 * Which prints:
 *
 *     a was 10
 *     b was 13
 */
METADOT_KeyValue* metadot_kv_read(const void* data, size_t size, METAENGINE_Result* result_out /* = NULL */);

/**
 * Creates a new kv ready for writing. You can write values as-needed by using `metadot_kv_key` and
 * `metadot_kv_val***` functions.
 *
 * Example:
 *
 *     int a = 10;
 *     int b = 12;
 *
 *     METADOT_KeyValue* kv = kv_write();
 *     metadot_kv_key(kv, "a");
 *     metadot_kv_val(kv, &a);
 *
 *     metadot_kv_key(kv, "b");
 *     metadot_kv_val(kv, &b);
 *
 *     printf("%s", metadot_kv_buffer());
 *     metadot_kv_destroy(kv);
 *
 * Which prints:
 *
 *     a = 10,
 *     b = 12,
 */
METADOT_KeyValue* metadot_kv_write();

/**
 * Frees up all resources using by the kv instance.
 */
void metadot_kv_destroy(METADOT_KeyValue* kv);

/**
 * Returns the mode this KV was opened in. Either KV_STATE_READ or KV_STATE_WRITE. You can use this
 * function in your serialization routines to do specific things for reading vs writing, whereas for
 * most cases you can use the same code for both reading and writing.
 */
METADOT_KeyValueState metadot_kv_state(METADOT_KeyValue* kv);

/**
 * Fetches the write buffer pointer containing any data serialized so far. This function will return
 * NULL if the kv is not opened for write mode.
 */
const char* metadot_kv_buffer(METADOT_KeyValue* kv);

/**
 * Returns the size written to the write buffer so far. This size does not include the nul-terminator.
 */
size_t metadot_kv_buffer_size(METADOT_KeyValue* kv);

/**
 * Resets the read position of `kv`. `kv` must be in read mode to use this function. After calling
 * `metadot_kv_key` and various combinations of `metadot_kv_object_begin` or `metadot_kv_array_begin` you might want
 * to stop reading and reset back to the top-level object in your serialized file. This function does
 * not do any re-parsing, and merely clears/resets a few internal variables.
 */
void metadot_read_reset(METADOT_KeyValue* kv);

/**
 * The base must be in read mode. This function is used to support data inheritence and delta encoding.
 * Depending on whether `kv` is in read or write mode this function behaves very differently. Read mode
 * is for data inheritence, while write mode is for delta encoding.
 *
 * Data Inheritence
 *
 *     If `kv` is in read mode any value missing will be fetched recursively from `base`.
 *
 * Delta Encoding
 *
 *     If `kv` is in write mode any value will first be recursively looked up in `base`. If found, it
 *     is only written if the new value is different from the value to be written.
 *
 * For a more in-depth description on how to use this function see the tutotorial page from the Cute
 * Framework docs here: https://randygaul.github.io/cute_framework/#/serialization/
 */
void metadot_kv_set_base(METADOT_KeyValue* kv, METADOT_KeyValue* base);

/**
 * Returns the error state of the kv instance. You can use this try and get a more useful description
 * of what may have went wrong. These errors are not fatal. For example if you search for a key with
 * `metadot_kv_key` and it's non-existent a potentially useful error message may be generated, but you can
 * still keep going and look for other keys freely.
 */
METAENGINE_Result metadot_kv_last_error(METADOT_KeyValue* kv);

#define METADOT_KV_TYPE_DEFS        \
    METADOT_ENUM(KV_TYPE_NULL, 0)   \
    METADOT_ENUM(KV_TYPE_INT64, 1)  \
    METADOT_ENUM(KV_TYPE_DOUBLE, 2) \
    METADOT_ENUM(KV_TYPE_STRING, 3) \
    METADOT_ENUM(KV_TYPE_ARRAY, 4)  \
    METADOT_ENUM(KV_TYPE_OBJECT, 5)

typedef enum METADOT_KeyValueType {
#define METADOT_ENUM(K, V) METADOT_##K = V,
    METADOT_KV_TYPE_DEFS
#undef METADOT_ENUM
} METADOT_KeyValueType;

ME_INLINE const char* metadot_key_value_type_to_string(METADOT_KeyValueType type) {
    switch (type) {
#define METADOT_ENUM(K, V) \
    case METADOT_##K:      \
        return METAENGINE_STRINGIZE(METADOT_##K);
        METADOT_KV_TYPE_DEFS
#undef METADOT_ENUM
        default:
            return NULL;
    }
}

bool metadot_kv_key(METADOT_KeyValue* kv, const char* key, METADOT_KeyValueType* type /*= NULL*/);

bool metadot_kv_val_uint8(METADOT_KeyValue* kv, uint8_t* val);
bool metadot_kv_val_uint16(METADOT_KeyValue* kv, uint16_t* val);
bool metadot_kv_val_uint32(METADOT_KeyValue* kv, uint32_t* val);
bool metadot_kv_val_uint64(METADOT_KeyValue* kv, uint64_t* val);

bool metadot_kv_val_int8(METADOT_KeyValue* kv, int8_t* val);
bool metadot_kv_val_int16(METADOT_KeyValue* kv, int16_t* val);
bool metadot_kv_val_int32(METADOT_KeyValue* kv, int32_t* val);
bool metadot_kv_val_int64(METADOT_KeyValue* kv, int64_t* val);

bool metadot_kv_val_float(METADOT_KeyValue* kv, float* val);
bool metadot_kv_val_double(METADOT_KeyValue* kv, double* val);
bool metadot_kv_val_bool(METADOT_KeyValue* kv, bool* val);

bool metadot_kv_val_string(METADOT_KeyValue* kv, const char** str, size_t* size);
bool metadot_kv_val_blob(METADOT_KeyValue* kv, void* data, size_t data_capacity, size_t* data_len);

bool metadot_kv_object_begin(METADOT_KeyValue* kv, const char* key /*= NULL*/);
bool metadot_kv_object_end(METADOT_KeyValue* kv);

bool metadot_kv_array_begin(METADOT_KeyValue* kv, int* count, const char* key /*= NULL*/);
bool metadot_kv_array_end(METADOT_KeyValue* kv);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using KeyValue = METADOT_KeyValue;

using KeyValueState = METADOT_KeyValueState;
#define METADOT_ENUM(K, V) ME_INLINE constexpr KeyValueState K = METADOT_##K;
METADOT_KV_STATE_DEFS
#undef METADOT_ENUM

ME_INLINE const char* to_string(KeyValueState state) {
    switch (state) {
#define METADOT_ENUM(K, V) \
    case METADOT_##K:      \
        return #K;
        METADOT_KV_STATE_DEFS
#undef METADOT_ENUM
        default:
            return NULL;
    }
}

using KeyValueType = METADOT_KeyValueType;
#define METADOT_ENUM(K, V) ME_INLINE constexpr KeyValueType K = METADOT_##K;
METADOT_KV_TYPE_DEFS
#undef METADOT_ENUM

ME_INLINE const char* to_string(KeyValueType type) {
    switch (type) {
#define METADOT_ENUM(K, V) \
    case METADOT_##K:      \
        return #K;
        METADOT_KV_TYPE_DEFS
#undef METADOT_ENUM
        default:
            return NULL;
    }
}

ME_INLINE KeyValue* kv_read(const void* data, size_t size, Result* result_out = NULL) { return metadot_kv_read(data, size, result_out); }
ME_INLINE KeyValue* kv_write(KeyValue* kv) { return metadot_kv_write(); }
ME_INLINE void kv_destroy(KeyValue* kv) { metadot_kv_destroy(kv); }
ME_INLINE KeyValueState kv_state(KeyValue* kv) { return (KeyValueState)metadot_kv_state(kv); }
ME_INLINE const char* kv_buffer(KeyValue* kv) { return metadot_kv_buffer(kv); }
ME_INLINE size_t kv_buffer_size(KeyValue* kv) { return metadot_kv_buffer_size(kv); }
ME_INLINE void kv_set_base(KeyValue* kv, KeyValue* base) { metadot_kv_set_base(kv, base); }
ME_INLINE Result kv_error_state(KeyValue* kv) { return metadot_kv_last_error(kv); }
ME_INLINE bool kv_key(KeyValue* kv, const char* key, KeyValueType* type = NULL) { return metadot_kv_key(kv, key, type); }
ME_INLINE bool kv_val(KeyValue* kv, uint8_t* val) { return metadot_kv_val_uint8(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, uint16_t* val) { return metadot_kv_val_uint16(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, uint32_t* val) { return metadot_kv_val_uint32(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, uint64_t* val) { return metadot_kv_val_uint64(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, int8_t* val) { return metadot_kv_val_int8(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, int16_t* val) { return metadot_kv_val_int16(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, int32_t* val) { return metadot_kv_val_int32(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, int64_t* val) { return metadot_kv_val_int64(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, float* val) { return metadot_kv_val_float(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, double* val) { return metadot_kv_val_double(kv, val); }
ME_INLINE bool kv_val(KeyValue* kv, bool* val) { return metadot_kv_val_bool(kv, val); }
ME_INLINE bool kv_val_string(KeyValue* kv, const char** str, size_t* size) { return metadot_kv_val_string(kv, str, size); }
ME_INLINE bool kv_val_blob(KeyValue* kv, void* data, size_t data_capacity, size_t* data_len) { return metadot_kv_val_blob(kv, data, data_capacity, data_len); }
ME_INLINE bool kv_object_begin(KeyValue* kv, const char* key = NULL) { return metadot_kv_object_begin(kv, key); }
ME_INLINE bool kv_object_end(KeyValue* kv) { return metadot_kv_object_end(kv); }
ME_INLINE bool kv_array_begin(KeyValue* kv, int* count, const char* key = NULL) { return metadot_kv_array_begin(kv, count, key); }
ME_INLINE bool kv_array_end(KeyValue* kv) { return metadot_kv_array_end(kv); }

}  // namespace MetaEngine

#endif  // METADOT_KV_H
