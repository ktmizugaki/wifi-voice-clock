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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define QUERY_PARSER_DONE           0
#define QUERY_PARSER_NEED_MORE      -1
#define QUERY_PARSER_MALFORMED      -2
#define QUERY_PARSER_INVALID_STATE  -3

#define QUEYR_PARSER_SIZE   (sizeof(void*)*10)
typedef struct queryparser queryparser_t;

/**
 * @brief type of callback function called each time key and value are decoded.
 * @param[in] key       null-terimated key string.
 * @param[in] key_len   lenth of key, provided for convenience.
 * @param[in] value     NULL for key-only parameter or null-terimated value string.
 * @param[in] value_len lenth of value, provided for convenience.
 */
typedef void (*queryparser_handler_t)(queryparser_t *, char *key, size_t key_len, char *value, size_t value_len, void *user_data);

/**
 * @brief parse full query string.
 * @param[in] query     full query string. query will be modified in place.
 * @param[in] handler   callback function to receive parameter.
 * @param[in] user_data user specified data passed to handler.
 */
extern int queryparser_parse(char *query, queryparser_handler_t handler, void *user_data);
/**
 * @brief allocate dynaimc query parser.
 * @param[in] scratch_size  specify size of scratch buffer.
 *                      this affect maximum parameter size
 *                      (length of single key=value pair).
 * @param[in] handler   callback function to receive parameter.
 * @param[in] user_data user specified data passed to handler.
 */
extern queryparser_t *queryparser_new(size_t scratch_size, queryparser_handler_t handler, void *user_data);
/**
 * @brief get part of scratch buffer to fill query string.
 * @param[in] parser    parser created with queryparser_new.
 * @param[out] scratch  pointer to scratch buffer where user can fill query
 *                      string will be stored.
 * @param[out] size     remaining size of scratch will be stored.
 */
extern void queryparser_get_scratch(queryparser_t *parser, void **scratch, size_t *size);
/**
 * @brief update parser.
 * @param[in] parser    parser created with queryparser_new.
 * @param[in] size      filled size of scratch which is obtained by queryparser_get_scratch.
 */
extern int queryparser_update(queryparser_t *parser, size_t size);
/**
 * @brief finish parser.
 * @param[in] parser    parser created with queryparser_new.
 */
extern int queryparser_finish(queryparser_t *parser);

#ifdef __cplusplus
}
#endif
