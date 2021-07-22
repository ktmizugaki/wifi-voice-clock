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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "include/json_str.h"

#define JSON_STR_STRICT     1
#define JSON_STR_MAX_DATA   65535

enum json_str_status {
    JSON_STR_NONE,
    JSON_STR_RAW,
    JSON_STR_STRING,
    JSON_STR_ARRAY,
    JSON_STR_OBJECT,
};

struct json_str_state {
    enum json_str_status status;
    union {
        size_t keypos;
        int index;
    };
    int keylen;
};

struct json_str {
    struct json_str_state *state;
    int depth;
    int max_depth;
    char *buf;
    size_t pos;
    size_t size;
    size_t pending;
};

json_str_t *new_json_str(size_t required)
{
    struct json_str *str = malloc(sizeof(struct json_str));
    if (str == NULL) return NULL;
    str->depth = 0;
    str->max_depth = 8;
    str->pos = 0;
    str->size = required >= 5? required: 128;
    str->pending = 0;
    str->state = malloc(sizeof(struct json_str_state)*str->max_depth);
    str->buf = malloc(str->size+1);
    if (str->state == NULL || str->buf == NULL) {
        delete_json_str(str);
        return NULL;
    }
    str->state[0].status = JSON_STR_NONE;
    return str;
}

void delete_json_str(json_str_t *str)
{
    if (str != NULL) {
        str->depth = 0;
        str->max_depth = 0;
        str->pos = 0;
        str->size = 0;
        if (str->state != NULL) free(str->state);
        if (str->buf != NULL) free(str->buf);
        free(str);
    }
}

int json_str_get_mem_usage(const json_str_t *str, size_t *used, size_t *allocated)
{
    if (used) *used = str->pos;
    if (allocated) *allocated = str->size;
    return JSON_STR_OK;
}

#define json_str_remaining(str)         ((str)->size - (str)->pos)
#define json_str_current_state(str)     (&(str)->state[(str)->depth])
#define json_str_current_status(str)    json_str_current_state(str)->status

#define ENSURE_SPACE(str, space) do { \
        size_t rem = json_str_remaining(str); \
        if ((size_t)(space)+(str)->pending > rem) { \
            JSON_STR_CHECK(request_buffer(str, (space)), err, return err); \
        } \
    } while(0)

#define CHECK_AND_ADD_KEY(str, key) do { \
        if (!can_add(str)) { \
            return JSON_STR_ERR_NOT_COMPOUND; \
        } \
        JSON_STR_CHECK(add_key(str, key), err, return err); \
    } while(0)

static inline int can_add(const json_str_t *str)
{
    enum json_str_status status;
    if (str->depth < 0) return 0; /* root compound was closed */
    status = json_str_current_status(str);
    return status == JSON_STR_NONE || status == JSON_STR_ARRAY || status == JSON_STR_OBJECT;
}

static int request_buffer(json_str_t *str, size_t needed)
{
    size_t newsize;
    char *newbuf;

    needed += str->pending;
    if (needed < str->size - str->pos) {
        return JSON_STR_OK;
    }
    newsize = (str->pos+needed+127)&(~127);
#if JSON_STR_MAX_DATA
    if (newsize > JSON_STR_MAX_DATA) {
        return JSON_STR_ENOMEM;
    }
#endif

    newbuf = realloc(str->buf, newsize+1);
    if (newbuf == NULL) {
        return JSON_STR_ENOMEM;
    }
    str->buf = newbuf;
    str->size = newsize;
    return JSON_STR_OK;
}

static int append_byte(json_str_t *str, char ch)
{
    assert(str->pos < str->size);
    str->buf[str->pos++] = ch;
    return JSON_STR_OK;
}

/* return length of escaped string if need escape */
static int json_need_escape(char ch)
{
    switch (ch) {
    case '\t':
    case '\n':
    case '\r':
    case '\\':
    case '"':
        return 2;
    }
    if (ch >= 0 && ch <= 0x1f) {
        return 6;   /* \uXXXX */
    }
    return 0;
}

static int json_escape(char ch, char *dst)
{
    static const char hex[] = "0123456789abcdef";
    *dst++ = '\\';
    switch (ch) {
    case '\b': *dst++ = 'b'; return 2;
    case '\t': *dst++ = 't'; return 2;
    case '\n': *dst++ = 'n'; return 2;
    case '\f': *dst++ = 'f'; return 2;
    case '\r': *dst++ = 'r'; return 2;
    case '\\': *dst++ = '\\'; return 2;
    case '"': *dst++ = '"'; return 2;
    }
    *dst++ = 'u';
    *dst++ = '0';
    *dst++ = '0';
    *dst++ = hex[ch>>4];
    *dst++ = hex[ch&0xf];
    return 3;
}

static int append_string(json_str_t *str, const char *string)
{
    size_t len = strlen(string);
    char ch;
    int err;
    ENSURE_SPACE(str, len);
    while ((ch = *string++) != '\0') {
        int esc = json_need_escape(ch);
        if (esc) {
            ENSURE_SPACE(str, esc);
            json_escape(ch, str->buf+str->pos);
            str->pos += esc;
        } else {
            append_byte(str, ch);
        }
    }
    return JSON_STR_OK;
}

static int add_key(json_str_t *str, const char *key)
{
    size_t needed;
    int err;
    struct json_str_state *state;
    state = json_str_current_state(str);
    if (state->status == JSON_STR_OBJECT) {
        if (key == NULL) return JSON_STR_ERR_KEY_REQUIRED;

        needed = 2+strlen(key)+2+5;
        ENSURE_SPACE(str, needed);
        if (state->keypos != 0) {
            append_byte(str, ',');
        }
        append_byte(str, '"');
        state->keypos = str->pos+1;
        str->pending += 2;
        err = append_string(str, key);
        state->keylen = str->pos - state->keypos;
        str->pending -= 2;
        append_byte(str, '"');
        append_byte(str, ':');
    } else if (state->status == JSON_STR_ARRAY) {
#if JSON_STR_STRICT
        if (key != NULL) return JSON_STR_ERR_KEY_NOTALLOWED;
#endif
        if (state->index > 0) {
            ENSURE_SPACE(str, 1);
            append_byte(str, ',');
        }
        state->index++;
        err = JSON_STR_OK;
    } else {
#if JSON_STR_STRICT
        if (key != NULL) return JSON_STR_ERR_KEY_NOTALLOWED;
#endif
        err = JSON_STR_OK;
    }
    return err;
}

const char *json_str_finalize(json_str_t *str)
{
    while (str->depth >= 0) {
        enum json_str_status status = json_str_current_status(str);
        switch (status) {
        case JSON_STR_NONE:
            return NULL;
            break;
        case JSON_STR_RAW:
            /* should not occur */
            return NULL;
            break;
        case JSON_STR_STRING:
            json_str_end_string(str);
            break;
        case JSON_STR_ARRAY:
            json_str_end_array(str);
            break;
        case JSON_STR_OBJECT:
            json_str_end_object(str);
            break;
        default:
            /* should not occur */
            return NULL;
            break;
        }
    }
    str->buf[str->pos] = '\0';
    return str->buf;
}

int json_str_add_json(json_str_t *str, const char *key, const char *value)
{
    int size;
    int err;
    CHECK_AND_ADD_KEY(str, key);
    size = strlen(value);
    ENSURE_SPACE(str, size);
    memcpy(str->buf+str->pos, value, size);
    str->pos += size;
    if (json_str_current_state(str)->status == JSON_STR_NONE) {
        json_str_current_state(str)->status = JSON_STR_RAW;
        str->depth--;
    }
    return JSON_STR_OK;
}

int json_str_add_null(json_str_t *str, const char *key)
{
    return json_str_add_json(str, key, "null");
}

extern int json_str_add_boolean(json_str_t *str, const char *key, int value)
{
    return json_str_add_json(str, key, value? "true": "false");
}

int json_str_add_number(json_str_t *str, const char *key, double value)
{
    int size, needed;
    int err;
    CHECK_AND_ADD_KEY(str, key);
    ENSURE_SPACE(str, 6);
    size = json_str_remaining(str)+1;
    needed = snprintf(str->buf+str->pos, size, "%.10g", value);
    if (needed < 0 || needed >= size) {
        ENSURE_SPACE(str, needed < 0 ? 24: needed);
        needed = snprintf(str->buf+str->pos, json_str_remaining(str)+1, "%.10g", value);
        if (needed < 0) {
            needed = strlen(str->buf+str->pos);
        }
    }
    str->pos += needed;
    if (json_str_current_state(str)->status == JSON_STR_NONE) {
        json_str_current_state(str)->status = JSON_STR_RAW;
        str->depth--;
    }
    return JSON_STR_OK;
}

int json_str_add_integer(json_str_t *str, const char *key, int value)
{
    int size, needed;
    int err;
    CHECK_AND_ADD_KEY(str, key);
    ENSURE_SPACE(str, 6);
    size = json_str_remaining(str)+1;
    needed = snprintf(str->buf+str->pos, size, "%d", value);
    if (needed < 0 || needed >= size) {
        ENSURE_SPACE(str, needed < 0 ? 32: needed);
        needed = snprintf(str->buf+str->pos, json_str_remaining(str)+1, "%d", value);
    }
    str->pos += needed;
    if (json_str_current_state(str)->status == JSON_STR_NONE) {
        json_str_current_state(str)->status = JSON_STR_RAW;
        str->depth--;
    }
    return JSON_STR_OK;
}

int json_str_add_string(json_str_t *str, const char *key, const char *value)
{
    int needed;
    int err;
    CHECK_AND_ADD_KEY(str, key);
    needed = 2+strlen(value);
    ENSURE_SPACE(str, needed);
    append_byte(str, '"');
    str->pending += 1;
    err = append_string(str, value);
    append_byte(str, '"');
    str->pending -= 1;
    if (json_str_current_state(str)->status == JSON_STR_NONE) {
        json_str_current_state(str)->status = JSON_STR_RAW;
        str->depth--;
    }
    return err;
}

int json_str_begin_string(json_str_t *str, const char *key, size_t required)
{
    int err;
    struct json_str_state *state;
    if (str->depth+1 >= str->max_depth) {
        return JSON_STR_ERR_TOO_DEEP;
    }
    CHECK_AND_ADD_KEY(str, key);
    ENSURE_SPACE(str, required+2);
    append_byte(str, '"');
    if (json_str_current_status(str) != JSON_STR_NONE) {
        str->depth++;
    }
    state = json_str_current_state(str);
    state->status = JSON_STR_STRING;
    state->keypos = str->pos;
    str->pending++;
    return JSON_STR_OK;
}

int json_str_append_string(json_str_t *str, const char *value)
{
    int err;
    if (str->depth < 0) {
        return JSON_STR_ERR_NOT_COMPOUND;
    }
    if (json_str_current_status(str) != JSON_STR_STRING) {
        return JSON_STR_ERR_NOT_STRING;
    }
    err = append_string(str, value);
    return err;
}

int json_str_end_string(json_str_t *str)
{
    if (str->depth < 0) {
        return JSON_STR_ERR_NOT_COMPOUND;
    }
    if (json_str_current_status(str) != JSON_STR_STRING) {
        return JSON_STR_ERR_NOT_STRING;
    }
    str->pending--;
    append_byte(str, '"');
    str->depth--;
    return JSON_STR_OK;
}

int json_str_begin_array(json_str_t *str, const char *key)
{
    int err;
    struct json_str_state *state;
    if (str->depth+1 >= str->max_depth) {
        return JSON_STR_ERR_TOO_DEEP;
    }
    CHECK_AND_ADD_KEY(str, key);
    ENSURE_SPACE(str, 2);
    append_byte(str, '\x5B');
    if (json_str_current_status(str) != JSON_STR_NONE) {
        str->depth++;
    }
    state = json_str_current_state(str);
    state->status = JSON_STR_ARRAY;
    state->index = 0;
    str->pending++;
    return JSON_STR_OK;
}

int json_str_end_array(json_str_t *str)
{
    if (str->depth < 0) {
        return JSON_STR_ERR_NOT_COMPOUND;
    }
    if (json_str_current_status(str) != JSON_STR_ARRAY) {
        return JSON_STR_ERR_NOT_ARRAY;
    }
    str->pending--;
    append_byte(str, '\x5D');
    str->depth--;
    return JSON_STR_OK;
}

int json_str_begin_object(json_str_t *str, const char *key)
{
    int err;
    struct json_str_state *state;
    if (str->depth+1 >= str->max_depth) {
        return JSON_STR_ERR_TOO_DEEP;
    }
    CHECK_AND_ADD_KEY(str, key);
    ENSURE_SPACE(str, 2);
    append_byte(str, '\x7B');
    if (json_str_current_status(str) != JSON_STR_NONE) {
        str->depth++;
    }
    state = json_str_current_state(str);
    state->status = JSON_STR_OBJECT;
    state->keypos = 0;
    str->pending++;
    return JSON_STR_OK;
}

int json_str_end_object(json_str_t *str)
{
    if (str->depth < 0) {
        return JSON_STR_ERR_NOT_COMPOUND;
    }
    if (json_str_current_status(str) != JSON_STR_OBJECT) {
        return JSON_STR_ERR_NOT_OBJECT;
    }
    str->pending--;
    append_byte(str, '\x7D');
    str->depth--;
    return JSON_STR_OK;
}
