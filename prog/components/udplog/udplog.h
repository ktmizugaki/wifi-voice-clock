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

#include <stdarg.h>
#include <stdarg.h>
#include <lwip/ip4_addr.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */
#ifndef LOG_EMERG
/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)syslog.h	8.1 (Berkeley) 6/2/93
 */

/*
 * priorities/facilities are encoded into a single 32-bit quantity, where the
 * bottom 3 bits are the priority (0-7) and the top 28 bits are the facility
 * (0-big number).  Both the priorities and the facilities map roughly
 * one-to-one to strings in the syslogd(8) source code.  This mapping is
 * included in this file.
 *
 * priorities (these are ordered)
 */
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

#define LOG_PRIMASK     0x07    /* mask to extract priority part (internal) */
                                /* extract priority */
#define LOG_PRI(p)      ((p) & LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri)   ((fac) | (pri))

/* facility codes */
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */

        /* other codes through 15 reserved for system use */
#define LOG_LOCAL0      (16<<3) /* reserved for local use */
#define LOG_LOCAL1      (17<<3) /* reserved for local use */
#define LOG_LOCAL2      (18<<3) /* reserved for local use */
#define LOG_LOCAL3      (19<<3) /* reserved for local use */
#define LOG_LOCAL4      (20<<3) /* reserved for local use */
#define LOG_LOCAL5      (21<<3) /* reserved for local use */
#define LOG_LOCAL6      (22<<3) /* reserved for local use */
#define LOG_LOCAL7      (23<<3) /* reserved for local use */

#define LOG_NFACILITIES 24      /* current number of facilities */
#define LOG_FACMASK     0x03f8  /* mask to extract facility part */

#define LOG_FAC(p)      (((p) & LOG_FACMASK) >> 3)

/*
 * arguments to setlogmask.
 */
#define LOG_MASK(pri)   (1 << (pri))            /* mask for one priority */
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)  /* all priorities through pri */

#endif /* LOG_EMERG */
/** @endcond */

/**
 * @file
 * syslog like logging functions which sends messages to external syslog server.
 */

/** @brief maximum length of ident this library supports. */
#define MAX_UDPLOG_IDENT    16
/** @brief maximum length of message this library supports. */
#define MAX_UDPLOG_MESSAGE  128

/**
 * @brief configure where udplogs are sent and port.
 * @param[in] ident         a prefix string prepended to every message.
 * @param[in] facility      default facility to be used if none is specified in udplog.
 * @param[in] destination   ipv4 address of syslog server.
 * @param[in] port          port of syslog server, usually 514.
 */
extern void udplog_init(const char *ident, int facility, const ip4_addr_t *destination, int port);
/**
 * @brief send message to server configured with @ref udplog_init.
 * @param[in] priority      ORed facility and level. this may be level alone and then default facility is used.
 * @param[in] format        printf style format string.
 * @param[in] ...           arguments for format string.
 */
extern void udplog(int priority, const char *format, ...) __attribute__(( format(printf, 2, 3) ));
/**
 * @brief same as @ref udplog except it uses argument list from stdarg.
 * @param[in] priority      ORed facility and level. this may be level alone and then default facility is used.
 * @param[in] format        printf style format string.
 * @param[in] ap            argument list abtained from va_start.
 */
extern void vudplog(int priority, const char *format, va_list ap);

/**
 * @brief sets logmask that determines which log levels will be logged.
 * @param[in] mask          set bits in this value enables correspanding log
 *                          levels and unset bits in this value disables
 *                          correspanding log levels.
 */
extern int setudplogmask(int mask);

#ifdef __cplusplus
}
#endif
