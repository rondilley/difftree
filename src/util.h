/*****
 *
 * Description: Utility Function Headers
 * 
 * Copyright (c) 2009-2015, Ron Dilley
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

#ifndef UTIL_DOT_H
#define UTIL_DOT_H

#define SYSLOG_MAX 1024

#define DEV_NULL "/dev/null"

/****
 *
 * includes
 *
 ****/

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
#include <sys/types.h>
#include <dirent.h>
#include "mem.h"

#ifndef HAVE_ERRNO_H
#ifndef EOPNOTSUPP
# define EOPNOTSUPP  95  /* Operation not supported on transport endpoint */
#endif
#endif

/****
 *
 * function prototypes
 *
 ****/

int display( int level, char *format, ... );
int open_devnull( int fd );
int is_dir_safe( const char *dir );
int create_pid_file( const char *filename );
static int safe_open( const char *filename );
static void cleanup_pid_file( const char *filename );
void sanitize_environment( void );

#endif /* end of UTIL_DOT_H */

