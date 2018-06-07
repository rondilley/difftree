/*****
 *
 * Description: Directory Processing Function Headers
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

#ifndef PROCESSDIR_DOT_H
#define PROCESSDIR_DOT_H

/****
 *
 * defines
 *
 ****/

#define FTW_RECORD 0
#define FILE_RECORD 1

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

#include <stdio.h>
#include <common.h>
#if ! defined HAVE_FTW  && ! defined HAVE_NFTW
# include "noftw.h"
#endif
#include <zlib.h> // need to move to autoconf
#include "util.h"
#include "mem.h"
#include "hash.h"
#include "md5.h"
#include "sha256.h"
#include "parser.h"
#include "fileHandlers.h"

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

typedef struct {
  struct stat sb;
  size_t byteCount;
  size_t lineCount;
  unsigned char digest[32]; // large enough to hold a sha256 digest
} metaData_t;

/****
 *
 * function prototypes
 *
 ****/

PUBLIC int processDir( char *dirStr );
int findMissingFiles( const struct hashRec_s *hashRec );
char *hash2hex(const unsigned char *hash, char *hashStr, int hLen );
int processRecord( const char *fpath, const struct stat *sb, char mode, unsigned char *digest );

#endif /* PROCESSDIR_DOT_H */
