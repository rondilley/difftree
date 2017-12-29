/*****
 *
 * Description: File Handling Function Headers
 * 
 * Copyright (c) 2010-2017, Ron Dilley
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

#ifndef FILEHANDLERS_DOT_H
#define FILEHANDLERS_DOT_H

/****
 *
 * defines
 *
 ****/

#define FORMAT_VERSION "1"
#define MD5_HASH_LEN 16
#define SHA256_HASH_LEN 32
#define TIME_DATE_LEN 22

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
#include "util.h"
#include "mem.h"
#include "hash.h"
#include "md5.h"
#include "parser.h"
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

int writeRecord2File( const struct hashRec_s *hashRec );
int writeDirHash2File( const struct hash_s *dirHash, const char *base, const char *outFile );
int loadFile( const char *fName );
int loadV1File( FILE *inFile );
int loadV2File( FILE *inFile );
int loadExclusions( char *fName );

#endif /* FILEHANDLERS_DOT_H */
