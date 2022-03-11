/*****
 *
 * Description: Difftree Function Headers
 * 
 * Copyright (c) 2010-2022, Ron Dilley
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****/

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
