/*****
 *
 * Copyright (c) 2009-2014, Ron Dilley
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   - Neither the name of Uberadmin/BaraCUDA/Nightingale nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****/

#ifndef DT_DOT_H
#define DT_DOT_H

/****
 *
 * defines
 *
 ****/

#define PROGNAME "dt"
#define MODE_DAEMON 0
#define MODE_INTERACTIVE 1
#define MODE_DEBUG 2

/* arg len boundary */
#define MAX_ARG_LEN 1024

#define LOGDIR "/var/log/dt"
#define MAX_LOG_LINE 2048
#define MAX_SYSLOG_LINE 4096 /* it should be 1024 but some clients are stupid */
#define SYSLOG_SOCKET "/dev/log"
#define MAX_FILE_DESC 256

/* user and group defaults */
#define MAX_USER_LEN 16
#define MAX_GROUP_LEN 16

#ifndef PATH_MAX
# ifdef MAXPATHLEN
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 1024
# endif
#endif

/****
 *
 * includes
 *
 ****/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sysdep.h>

#ifndef __SYSDEP_H__
# error something is messed up
#endif

#include <common.h>
#include "util.h"
#include "mem.h"
#include "getopt.h"
#include "hash.h"
#include "processDir.h"
#if ! defined HAVE_FTW  && ! defined HAVE_NFTW
# include "noftw.h"
#endif

/****
 *
 * consts & enums
 *
 ****/

/****
 *
 * typedefs & structs
 *
 ****/

/****
 *
 * function prototypes
 *
 ****/

int main(int argc, char *argv[]);
PRIVATE void cleanup( void );
PRIVATE void show_info( void );
PRIVATE void print_version( void );
PRIVATE void print_help( void );
void sigint_handler( int signo );
void sighup_handler( int signo );
void sigterm_handler( int signo );
void sigfpe_handler( int signo );
void sigbus_handler( int signo );
void sigsegv_handler( int signo );
void sigill_handler( int signo );

#endif /* DT_DOT_H */
