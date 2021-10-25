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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "queryparser.h"

#define ST_START    0
#define ST_HASKEY   1
#define ST_END      2

struct queryparser {
    int state;
    int key_len;
    int scratch_size;
    char *query;
    char *end;
    char *key;
    void *user_data;
    queryparser_handler_t handler;
    char scratch[];
};

#define PARSE_DONE  QUERY_PARSER_DONE
#define NEED_MORE   QUERY_PARSER_NEED_MORE
#define MALFORMED   QUERY_PARSER_MALFORMED

static int urldecode(char *s)
{
    int state = 0, c;
    int i, len;
    for (i = 0, len = 0; s[i]; i++) {
        if (state <= 1 && s[i] == '%') {
            state = 2;
            s[len] = 0;
        } else if (state == 2 || state == 3) {
            c = s[i];
            if (c >= '0' && c <= '9') {
                c = c-'0';
            } else if (c >= 'A' && c <= 'F') {
                c = c-'A'+10;
            } else if (c >= 'a' && c <= 'f') {
                c = c-'a'+10;
            } else {
                return MALFORMED;
            }
            if (state == 2) {
                s[len] = c << 4;
                state = 3;
            } else {
                s[len] |= c;
                len++;
                state = 1;
            }
        } else {
            if (state) {
                s[len] = s[i];
            }
            len++;
        }
    }
    if (state == 2 || state == 3) {
        return MALFORMED;
    }
    s[len] = 0;
    return len;
}

static int parse_segment(char *s, char *end, char **pnext, int *c, const char *accept, int isend)
{
    char *next;
    int len;
    next = strpbrk(s, accept);
    if (next == NULL) {
        if (!isend) {
            return NEED_MORE;
        }
        *c = 0;
        next = end? end: s + strlen(s);
    } else {
        *c = *next;
        *next++ = 0;
    }
    *pnext = next;
    return len = urldecode(s);
    if (len < 0) {
        return MALFORMED;
    }
    return len;
}

static int parse_query(struct queryparser *parser, int isend)
{
    while (1) {
        if (parser->state == ST_START) {
            char *key = parser->query;
            char *next;
            int c;
            int key_len = parse_segment(key, parser->end, &next, &c, "&=", isend);
            if (key_len < 0) {
                return key_len;
            }
            if (c == '=') {
                parser->state = ST_HASKEY;
                parser->query = next;
                parser->key = key;
                parser->key_len = key_len;
            } else {
                /* '=' is missing: key only segment */
                parser->handler(parser, key, key_len, NULL, 0, parser->user_data);
                parser->state = c=='&'? ST_START: ST_END;
                parser->query = next;
                parser->key = NULL;
                parser->key_len = 0;
            }
        }
        if (parser->state == ST_HASKEY) {
            char *value = parser->query;
            char *next;
            int c;
            int value_len = parse_segment(value, parser->end, &next, &c, "&", isend);
            if (value_len < 0) {
                return value_len;
            }
            parser->handler(parser, parser->key, parser->key_len, value, value_len, parser->user_data);
            parser->state = c=='&'? ST_START: ST_END;
            parser->query = next;
            parser->key = NULL;
            parser->key_len = 0;
        }
        if (parser->state == ST_END) {
            return PARSE_DONE;
        }
    }
}

static int compact_parser(struct queryparser *parser)
{
    int ret, len;
    if (parser->end == NULL) {
        /* static parser cannot be compacted. */
        return 0;
    }
    if (parser->state == ST_START) {
        len = parser->end-parser->query;
        ret = len;
        parser->query = memmove(parser->scratch, parser->query, len);
        parser->end = parser->query+len;
    } else if (parser->state == ST_HASKEY) {
        len = parser->end-parser->query;
        ret = parser->key_len+1+len;
        parser->key = memmove(parser->scratch, parser->key, parser->key_len+1);
        parser->query = memmove(parser->scratch+parser->key_len+1, parser->query, len);
        parser->end = parser->query+len;
    } else {
        ret = 0;
    }
    return ret;
}

int queryparser_parse(char *query, queryparser_handler_t handler, void *user_data)
{
    struct queryparser parser = {
        .state = ST_START,
        .query = query,
        .end = NULL,
        .key = NULL,
        .handler = handler,
        .user_data = user_data,
        .key_len = 0,
        .scratch_size = 0,
    };
    if (handler == NULL) {
        return PARSE_DONE;
    }
    return parse_query(&parser, 1);
}

queryparser_t *queryparser_new(size_t scratch_size, queryparser_handler_t handler, void *user_data)
{
    queryparser_t *parser;
    if (handler == NULL) {
        return NULL;
    }
    parser = malloc(sizeof(struct queryparser)+scratch_size);
    if (parser == NULL) {
        return NULL;
    }
    parser->state = ST_START;
    parser->query = parser->scratch;
    parser->end = parser->scratch;
    parser->key = NULL;
    parser->handler = handler;
    parser->user_data = user_data;
    parser->key_len = 0;
    parser->scratch_size = scratch_size;
    return parser;
}

void queryparser_get_scratch(queryparser_t *parser, void **scratch, size_t *size)
{
    if (parser == NULL || scratch == NULL || size == NULL) {
        /* invalid argument */
        return;
    }
    if (parser->end == NULL || parser->state == ST_END) {
        *scratch = NULL;
        *size = 0;
    } else {
        *scratch = parser->end;
        *size = parser->scratch+parser->scratch_size - parser->end-1;
    }
}

int queryparser_update(queryparser_t *parser, size_t size)
{
    int ret;
    if (parser->end == NULL || parser->state == ST_END) {
        return QUERY_PARSER_INVALID_STATE;
    }
    if (size > (size_t)(parser->scratch+parser->scratch_size - parser->end-1)) {
        return QUERY_PARSER_INVALID_STATE;
    }
    parser->end += size;
    ret = parse_query(parser, 0);
    compact_parser(parser);
    return ret;
}

int queryparser_finish(queryparser_t *parser)
{
    int ret;
    if (parser->state == ST_END) {
        return PARSE_DONE;
    }
    ret = parse_query(parser, 1);
    parser->state = ST_END;
    return ret;
}
