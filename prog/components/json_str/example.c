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

#include <stdio.h>
#include "include/json_str.h"

int main(void)
{
    /* taken from https://jsonapi.org/format/#fetching-resources-responses */
    int err;
    json_str_t *str = new_json_str(224);

    /* start root object */
    JSON_STR_CHECK(json_str_begin_object(str, NULL), err, goto end);
      /* start "links" object */
      JSON_STR_CHECK(json_str_begin_object(str, "links"), err, goto end);
        JSON_STR_CHECK(json_str_add_string(str, "self", "http://example.com/articles"), err, goto end);
      /* close "links" object */
      JSON_STR_CHECK(json_str_end_object(str), err, goto end);

      /* start "data" array */
      JSON_STR_CHECK(json_str_begin_array(str, "data"), err, goto end);
        /* start data[0] object */
        JSON_STR_CHECK(json_str_begin_object(str, NULL), err, goto end);
          JSON_STR_CHECK(json_str_add_string(str, "type", "articles"), err, goto end);
          JSON_STR_CHECK(json_str_add_integer(str, "id", 1), err, goto end);
          JSON_STR_CHECK(json_str_begin_object(str, "attributes"), err, goto end);
            JSON_STR_CHECK(json_str_add_string(str, "title", "JSON:API paints my bikeshed!"), err, goto end);
          JSON_STR_CHECK(json_str_end_object(str), err, goto end);
        /* close data[0] object */
        JSON_STR_CHECK(json_str_end_object(str), err, goto end);

        JSON_STR_CHECK(json_str_begin_object(str, NULL), err, goto end);
          JSON_STR_CHECK(json_str_add_string(str, "type", "articles"), err, goto end);
          JSON_STR_CHECK(json_str_add_integer(str, "id", 2), err, goto end);
          JSON_STR_CHECK(json_str_begin_object(str, "attributes"), err, goto end);
            JSON_STR_CHECK(json_str_add_string(str, "title", "Rails is Omakase"), err, goto end);
          JSON_STR_CHECK(json_str_end_object(str), err, goto end);
        JSON_STR_CHECK(json_str_end_object(str), err, goto end);
      /* close "data" array */
      JSON_STR_CHECK(json_str_end_array(str), err, goto end);

    /* close root object */
    JSON_STR_CHECK(json_str_end_object(str), err, goto end);

    /* print built json string */
    if (json_str_finalize(str) == NULL) {
        err = JSON_STR_FAIL;
    }
    puts(json_str_finalize(str));

end:
    /* release buffer */
    delete_json_str(str);
    if (err != JSON_STR_OK) {
        fprintf(stderr, "json_str failed: %d\n", err);
    }
    return 0;
}
