/* Copyright 2021 Kawashima Teruaki
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

#define TEST_BEGIN(str, required) \
    json_str_t *str = new_json_str(required); \
    const char *json; \
    size_t used, allocated;

#define TEST_END(str) \
    if ((json = json_str_finalize(str)) == NULL) { \
        TEST_FAIL("%s failed", "json_str_finalize(str)"); \
    } \
    puts(json); \
    json_str_get_mem_usage(str, &used, &allocated); \
    fprintf(stderr, "mem usage: %lu/%lu\n", (unsigned long)used, (unsigned long)allocated);

#define TEST_FAIL(format, ...) do {\
        fprintf(stderr, "%s:%d:%s:" format "\n", __FILE__, __LINE__, __func__, __VA_ARGS__); \
        exit(1); \
    } while(0)

#define TEST_OK(expr) do { \
        int err = expr; \
        if (err != JSON_STR_OK) { \
            TEST_FAIL("%s failed: %d", #expr, err); \
        } \
    } while(0)

#define TEST_ERR(expr, expected) do { \
        int err = expr; \
        if (err != expected) { \
            TEST_FAIL("%s failed: %d (expected %d)", #expr, err, expected); \
        } \
    } while(0)

#define RAW_JSON    "{\"key1\":\"value1\"}"

static void test_raw_json(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_json(str, NULL, RAW_JSON));

    TEST_END(str)

    assert(strcmp(json, RAW_JSON) == 0);
}

static void test_raw_null(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_null(str, NULL));

    TEST_END(str)

    assert(strcmp(json, "null") == 0);
}

static void test_raw_false(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_boolean(str, NULL, 0));

    TEST_END(str)

    assert(strcmp(json, "false") == 0);
}

static void test_raw_true(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_boolean(str, NULL, !0));

    TEST_END(str)

    assert(strcmp(json, "true") == 0);
}

static void test_raw_number_pi(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_number(str, NULL, 3.141592));

    TEST_END(str)

    assert(strcmp(json, "3.141592") == 0);
}

static void test_raw_integer_answer(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_integer(str, NULL, 42));

    TEST_END(str)

    assert(strcmp(json, "42") == 0);
}

static void test_raw_string_empty(void)
{
    TEST_BEGIN(str, 80)

    TEST_OK(json_str_add_string(str, NULL, ""));

    TEST_END(str)

    assert(strcmp(json, "\"\"") == 0);
}

static void test_raw_string_escape(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_string(str, NULL, "\"\x01\x02\x03\x1c\x1d\x1e\x1f\""));

    TEST_END(str)

    assert(strcmp(json, "\"\\\"\\u0001\\u0002\\u0003\\u001c\\u001d\\u001e\\u001f\\\"\"") == 0);
}

static void test_raw_string_problem(void)
{
    TEST_BEGIN(str, 80)

    TEST_OK(json_str_add_string(str, NULL, "Answer to the Ultimate Question of Life, the Universe, and Everything"));

    TEST_END(str)

    assert(strcmp(json, "\"Answer to the Ultimate Question of Life, the Universe, and Everything\"") == 0);
}

static void test_string_empty(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_string(str, NULL, 40));
    TEST_OK(json_str_end_string(str));

    TEST_END(str)
    assert(strcmp(json, "\"\"") == 0);
}

static void test_string(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_string(str, NULL, 40));
    TEST_OK(json_str_append_string(str, "Answer"));
    TEST_OK(json_str_append_string(str, " to"));
    TEST_OK(json_str_append_string(str, " the"));
    TEST_OK(json_str_append_string(str, " Ultimate"));
    TEST_OK(json_str_append_string(str, " Question"));
    TEST_OK(json_str_append_string(str, " of"));
    TEST_OK(json_str_append_string(str, " Life"));
    TEST_OK(json_str_append_string(str, ", the Universe"));
    TEST_OK(json_str_append_string(str, ", and"));
    TEST_OK(json_str_append_string(str, " Everything"));
    TEST_OK(json_str_end_string(str));

    TEST_END(str)
    assert(strcmp(json, "\"Answer to the Ultimate Question of Life, the Universe, and Everything\"") == 0);
}

static void test_array_empty(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_array(str, NULL));
    TEST_OK(json_str_end_array(str));

    TEST_END(str)

    assert(strcmp(json, "[]") == 0);
}

static void test_array_with_premitives(void)
{
    TEST_BEGIN(str, 256)

    TEST_OK(json_str_begin_array(str, NULL));
      TEST_OK(json_str_add_json(str, NULL, RAW_JSON));
      TEST_OK(json_str_add_null(str, NULL));
      TEST_OK(json_str_add_boolean(str, NULL, 0));
      TEST_OK(json_str_add_boolean(str, NULL, !0));
      TEST_OK(json_str_add_number(str, NULL, 3.141592));
      TEST_OK(json_str_add_integer(str, NULL, 42));
      TEST_OK(json_str_add_string(str, NULL, ""));
      TEST_OK(json_str_add_string(str, NULL, "\"\x01\x02\x03\x1c\x1d\x1e\x1f\""));
      TEST_OK(json_str_add_string(str, NULL, "Answer to the Ultimate Question of Life, the Universe, and Everything"));
    TEST_OK(json_str_end_array(str));

    TEST_END(str)
}

static void test_object_empty(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_object(str, NULL));
    TEST_OK(json_str_end_object(str));

    TEST_END(str)

    assert(strcmp(json, "{}") == 0);
}

static void test_object_with_premitives(void)
{
    TEST_BEGIN(str, 256)

    TEST_OK(json_str_begin_object(str, NULL));
      TEST_OK(json_str_add_json(str, "keyjson", RAW_JSON));
      TEST_OK(json_str_add_null(str, "keynull"));
      TEST_OK(json_str_add_boolean(str, "keytrue", 0));
      TEST_OK(json_str_add_boolean(str, "keyfalse", !0));
      TEST_OK(json_str_add_number(str, "keypi", 3.141592));
      TEST_OK(json_str_add_integer(str, "keyanswer", 42));
      TEST_OK(json_str_add_string(str, "keyempty", ""));
      TEST_OK(json_str_add_string(str, "keyescape", "\"\x01\x02\x03\x1c\x1d\x1e\x1f\""));
      TEST_OK(json_str_add_string(str, "keyproblem", "Answer to the Ultimate Question of Life, the Universe, and Everything"));
    TEST_OK(json_str_end_object(str));

    TEST_END(str)
}

static void test_compound(void)
{
    TEST_BEGIN(str, 256)

    /* taken from https://jsonapi.org/format/#fetching-resources-responses */
    TEST_OK(json_str_begin_object(str, NULL));

      TEST_OK(json_str_begin_object(str, "links"));
        TEST_OK(json_str_add_string(str, "self", "http://example.com/articles"));
      TEST_OK(json_str_end_object(str));

      TEST_OK(json_str_begin_array(str, "data"));
        TEST_OK(json_str_begin_object(str, NULL));
          TEST_OK(json_str_add_string(str, "type", "articles"));
          TEST_OK(json_str_add_integer(str, "id", 1));
          TEST_OK(json_str_begin_object(str, "attributes"));
            TEST_OK(json_str_add_string(str, "title", "JSON:API paints my bikeshed!"));
          TEST_OK(json_str_end_object(str));
        TEST_OK(json_str_end_object(str));

        TEST_OK(json_str_begin_object(str, NULL));
          TEST_OK(json_str_add_string(str, "type", "articles"));
          TEST_OK(json_str_add_integer(str, "id", 2));
          TEST_OK(json_str_begin_object(str, "attributes"));
            TEST_OK(json_str_add_string(str, "title", "Rails is Omakase"));
          TEST_OK(json_str_end_object(str));
        TEST_OK(json_str_end_object(str));
      TEST_OK(json_str_end_array(str));

    TEST_OK(json_str_end_object(str));

    TEST_END(str)
}

static void test_no_item_after_null(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_null(str, NULL));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "null") == 0);
}

static void test_no_item_after_boolean(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_boolean(str, NULL, 0));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "false") == 0);
}

static void test_no_item_after_number(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_number(str, NULL, 3.141592));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "3.141592") == 0);
}

static void test_no_item_after_integer(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_integer(str, NULL, 42));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "42") == 0);
}

static void test_no_item_after_string(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_add_string(str, NULL, ""));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "\"\"") == 0);
}

static void test_no_item_after_string2(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_string(str, NULL, 4));
    TEST_OK(json_str_end_string(str));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "\"\"") == 0);
}

static void test_no_item_in_string2(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_string(str, NULL, 4));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_ARRAY);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_OBJECT);

    TEST_OK(json_str_end_string(str));

    TEST_END(str)

    assert(strcmp(json, "\"\"") == 0);
}

static void test_no_item_after_array(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_array(str, NULL));
    TEST_OK(json_str_end_array(str));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "[]") == 0);
}

static void test_no_item_after_object(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_object(str, NULL));
    TEST_OK(json_str_end_object(str));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_string(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_array(str), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_NOT_COMPOUND);
    TEST_ERR(json_str_end_object(str), JSON_STR_ERR_NOT_COMPOUND);

    TEST_END(str)

    assert(strcmp(json, "{}") == 0);
}

static void test_key_not_allowed_in_root(void)
{
    TEST_BEGIN(str, 8)

    TEST_ERR(json_str_add_json(str, "key", RAW_JSON), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_null(str, "key"), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_boolean(str, "key", 0), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_number(str, "key", 3.141592), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_integer(str, "key", 42), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_string(str, "key", ""), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_begin_string(str, "key", 4), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_begin_array(str, "key"), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_begin_object(str, "key"), JSON_STR_ERR_KEY_NOTALLOWED);

    json_str_add_null(str, NULL);
    TEST_END(str)

    assert(strcmp(json, "null") == 0);
}

static void test_key_not_allowed_in_array(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_array(str, NULL));

    TEST_ERR(json_str_add_json(str, "keyy", RAW_JSON), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_null(str, "key"), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_boolean(str, "key", 0), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_number(str, "key", 3.141592), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_integer(str, "key", 42), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_add_string(str, "key", ""), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_begin_string(str, "key", 4), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_begin_array(str, "key"), JSON_STR_ERR_KEY_NOTALLOWED);
    TEST_ERR(json_str_begin_object(str, "key"), JSON_STR_ERR_KEY_NOTALLOWED);

    TEST_OK(json_str_end_array(str));

    TEST_END(str)

    assert(strcmp(json, "[]") == 0);
}

static void test_key_required_in_object(void)
{
    TEST_BEGIN(str, 8)

    TEST_OK(json_str_begin_object(str, NULL));

    TEST_ERR(json_str_add_json(str, NULL, RAW_JSON), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_add_null(str, NULL), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_add_boolean(str, NULL, 0), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_add_number(str, NULL, 3.141592), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_add_integer(str, NULL, 42), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_add_string(str, NULL, ""), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_begin_string(str, NULL, 4), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_begin_array(str, NULL), JSON_STR_ERR_KEY_REQUIRED);
    TEST_ERR(json_str_begin_object(str, NULL), JSON_STR_ERR_KEY_REQUIRED);

    TEST_OK(json_str_end_object(str));

    TEST_END(str)

    assert(strcmp(json, "{}") == 0);
}

static void test_end(void)
{
    puts("\"All test passed!\"");
}

static void (*const tests[])(void) = {
    test_raw_json,
    test_raw_null,
    test_raw_false,
    test_raw_true,
    test_raw_number_pi,
    test_raw_integer_answer,
    test_raw_string_empty,
    test_raw_string_escape,
    test_raw_string_problem,
    test_string_empty,
    test_string,
    test_array_empty,
    test_array_with_premitives,
    test_object_empty,
    test_object_with_premitives,
    test_compound,
    test_no_item_after_null,
    test_no_item_after_boolean,
    test_no_item_after_number,
    test_no_item_after_integer,
    test_no_item_after_string,
    test_no_item_after_string2,
    test_no_item_in_string2,
    test_no_item_after_array,
    test_no_item_after_object,
    test_key_not_allowed_in_root,
    test_key_not_allowed_in_array,
    test_key_required_in_object,
    test_end,
};
static const int num_test = sizeof(tests)/sizeof(tests[0]);

int main(int argc, char **argv)
{
    if (argc > 1) {
        int test_id = atoi(argv[1]);
        if (test_id >= 0 && test_id < num_test) {
            tests[test_id]();
        } else {
            fprintf(stderr, "invalid test id: %s\n", argv[1]);
            return 1;
        }
    } else {
        int test_id;
        for (test_id = 0; test_id < num_test; test_id++) {
            tests[test_id]();
        }
    }
    return 0;
}
