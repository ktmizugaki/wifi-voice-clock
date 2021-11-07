/* json_str.c - simple JSON string builder in C
 *
 * Copyright 2021 Kawashima Teruaki
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JSON_STR_H
#define JSON_STR_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * simple JSON string builder in C
 */

/** obfuscated type building JSON string */
typedef struct json_str json_str_t;

#define JSON_STR_OK                 0   /**< success */
#define JSON_STR_FAIL               -1  /**< generic error */
#define JSON_STR_ENOMEM             1   /**< failed to extend internal buffer */
#define JSON_STR_ERR_KEY_REQUIRED   2   /**< key is NULL when required. key is required for object. */
#define JSON_STR_ERR_KEY_NOTALLOWED 3   /**< key is not NULL when not allowed. key is not allowed for array. */
#define JSON_STR_ERR_TOO_DEEP       4   /**< object or array is nested too deeply */
#define JSON_STR_ERR_NOT_COMPOUND   10  /**< state is not appropreate for adding value. */
#define JSON_STR_ERR_NOT_STRING     11  /**< tryied to append/end string without beginning string */
#define JSON_STR_ERR_NOT_ARRAY      12  /**< tryied to end array without beginning array */
#define JSON_STR_ERR_NOT_OBJECT     13  /**< tryied to end object without beginning object */

#define JSON_STR_CHECK(expr, err, onerror) do {\
        if ((err = expr) != JSON_STR_OK) { onerror; } \
    } while(0)

#define JSON_STR_OBJECT_WITH(str, key, err, onerror, block) \
    JSON_STR_CHECK(json_str_begin_object(str, key), err, onerror); \
    block; \
    JSON_STR_CHECK(json_str_end_object(str), err, onerror)

#define JSON_STR_ARRAY_WITH(str, key, err, onerror, block) \
    JSON_STR_CHECK(json_str_begin_array(str, key), err, onerror); \
    block; \
    JSON_STR_CHECK(json_str_end_array(str), err, onerror)

/**
 * @brief create new JSON string buffer.
 * @param[in]   required    specify approximate size of resulting JSON string.
 *                          specify 0 for internal default value.
 * @return new JSON string buffer or NULL if allocation failed.
 */
extern json_str_t *new_json_str(size_t required);
/**
 * @brief release memory of JSON string buffer.
 * @param[in]   str     JSON string buffer returned from @ref new_json_str.
 */
extern void delete_json_str(json_str_t *str);
/**
 * @brief null terminate JSON string buffer.
 * @note This can be called multiple times to get JSON string.
 * @param[in]   str     constructrd JSON string buffer.
 * @return null-termibated JSON string or NULL if invalid state.
 */
extern const char *json_str_finalize(json_str_t *str);
/**
 * @brief get memory usage information of JSON string buffer.
 * @param[in]   str         JSON string buffer.
 * @param[out]  used        (optional) size of used memory is stored.
 * @param[out]  allocated   (optional) size of allocated memory is stored.
 * @return JSON_STR_OK on success.
 */
extern int json_str_get_mem_usage(const json_str_t *str, size_t *used, size_t *allocated);
/**
 * @brief add pre composed JSON string to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @param[in]   value   valid JSON string.
 * @return JSON_STR_OK on success.
 * @note this function does not check validity of value.
 */
extern int json_str_add_json(json_str_t *str, const char *key, const char *value);
/**
 * @brief add "null" to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @return JSON_STR_OK on success.
 */
extern int json_str_add_null(json_str_t *str, const char *key);
/**
 * @brief add "true" or "false" to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @param[in]   value   0 indicates false, non-0 value indicates true.
 * @return JSON_STR_OK on success.
 */
extern int json_str_add_boolean(json_str_t *str, const char *key, int value);
/**
 * @brief add number to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @param[in]   value   number.
 * @return JSON_STR_OK on success.
 */
extern int json_str_add_number(json_str_t *str, const char *key, double value);
/**
 * @brief add number to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @param[in]   value   number.
 * @return JSON_STR_OK on success.
 */
extern int json_str_add_integer(json_str_t *str, const char *key, int value);
/**
 * @brief add string to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @param[in]   value   string.
 * @return JSON_STR_OK on success.
 * @note string will be escaped while adding to buffer if necessary.
 */
extern int json_str_add_string(json_str_t *str, const char *key, const char *value);
/**
 * @brief begins bulk adding of string to JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @param[in]   required    specify approximate size of string which is going to be added.
 *                          specify 0 if not known.
 * @return JSON_STR_OK on success.
 * @note call @ref json_str_end_string when done.
 */
extern int json_str_begin_string(json_str_t *str, const char *key, size_t required);
/**
 * @brief append string to current string in JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   value   string.
 * @return JSON_STR_OK on success.
 * @note must be called between @ref json_str_begin_string and @ref json_str_end_string.
 * @note string will be escaped while adding to buffer if necessary.
 */
extern int json_str_append_string(json_str_t *str, const char *value);
/**
 * @brief ends bulk adding of string started by @ref json_str_begin_string.
 * @param[in]   str     JSON string buffer.
 * @return JSON_STR_OK on success.
 */
extern int json_str_end_string(json_str_t *str);
/**
 * @brief open array in JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @return JSON_STR_OK on success.
 * @note call @ref json_str_end_array to close array.
 */
extern int json_str_begin_array(json_str_t *str, const char *key);
/**
 * @brief close array opened by @ref json_str_begin_array.
 * @param[in]   str     JSON string buffer.
 * @return JSON_STR_OK on success.
 */
extern int json_str_end_array(json_str_t *str);
/**
 * @brief open object in JSON string buffer.
 * @param[in]   str     JSON string buffer.
 * @param[in]   key     specify key string when adding value to object.
                        must be NULL when not adding to object.
 * @return JSON_STR_OK on success.
 * @note call @ref json_str_end_object to close object.
 */
extern int json_str_begin_object(json_str_t *str, const char *key);
/**
 * @brief close object opened by @ref json_str_begin_object.
 * @param[in]   str     JSON string buffer.
 * @return JSON_STR_OK on success.
 */
extern int json_str_end_object(json_str_t *str);

#ifdef __cplusplus
}
#endif
#endif /* JSON_STR_H */
