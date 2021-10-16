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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <lwip/udp.h>
#include <lwip/ip4_addr.h>
#include <lwip/pbuf.h>

#include "udplog.h"

static char *s_ident = NULL;
static int s_header_len = 0;
static int s_facility = LOG_USER;
static ip_addr_t s_destination;
static int s_port = -1;
static int s_logmask = LOG_UPTO(LOG_DEBUG);

void udplog_init(const char *ident, int facility, const ip4_addr_t *destination, int port)
{
    if (s_ident != NULL) {
        free(s_ident);
        s_ident = NULL;
    }
    if (ident != NULL) {
        s_ident = strdup(ident);
    } else {
        s_ident = strdup("");
    }
    if (s_ident == NULL) {
        s_port = -1;
        return;
    }
    s_header_len = strlen(s_ident);
    if (s_header_len > MAX_UDPLOG_IDENT) {
        s_header_len = MAX_UDPLOG_IDENT;
        s_ident[s_header_len] = '\0';
    }
    s_header_len += 12;
    s_facility = facility&LOG_FACMASK;
    if (s_facility == 0) {
        s_facility = LOG_USER;
    }
    ip_addr_copy_from_ip4(s_destination, *destination);
    if (port > 0 && port <= 65535) {
        s_port = port;
    } else {
        s_port = 514;
    }
}

void udplog(int priority, const char *format, ...)
{
    va_list args;

    priority &= LOG_PRIMASK;

    if (s_port < 0) {
        return;
    }
    if ((s_logmask & LOG_MASK(priority)) == 0) {
        return;
    }

    va_start(args, format);
    vudplog(priority, format, args);
    va_end(args);
}

void vudplog(int priority, const char *format, va_list ap)
{
    struct udp_pcb *pcb;
    struct pbuf *p;
    size_t hdrlen, msglen;

    priority &= (LOG_PRIMASK|LOG_FACMASK);

    if (s_port < 0) {
        return;
    }
    if ((s_logmask & LOG_MASK(LOG_PRI(priority))) == 0) {
        return;
    }
    if ((priority&LOG_FACMASK) == 0) {
        priority = LOG_MAKEPRI(s_facility, priority);
    }

    pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    p = pbuf_alloc(PBUF_TRANSPORT, s_header_len+MAX_UDPLOG_MESSAGE, PBUF_RAM);
    hdrlen = snprintf((char*)p->payload, s_header_len, "<%d>%s: ", priority, s_ident);
    msglen = vsnprintf((char*)p->payload+hdrlen, MAX_UDPLOG_MESSAGE, format, ap);
    if (msglen > MAX_UDPLOG_MESSAGE) {
        msglen = MAX_UDPLOG_MESSAGE;
    }
    ((char*)p->payload)[hdrlen+msglen] = '\0';
    p->tot_len = hdrlen+msglen+1;
    p->len = hdrlen+msglen+1;
    udp_sendto(pcb, p, &s_destination, s_port);
    pbuf_free(p);
    udp_remove(pcb);
}

int setudplogmask(int mask)
{
    int prevmask = s_logmask;
    if (mask != 0) {
        s_logmask = mask;
    }
    return prevmask;
}
