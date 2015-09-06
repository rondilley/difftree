/*****
 *
 * Description: ftw Function Headers
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

#ifndef NOFTW_DOT_H
#define NOFTW_DOT_H

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

#include <dirent.h>
#include <common.h>
#include "util.h"
#include "mem.h"

/****
 *
 * defines
 *
 ****/

/* shamelessly lifted from ftw.h */
#if !defined HAVE_NFTW && !defined HAVE_FTW
/*
 * Valid flags for the 3rd argument to the function that is passed as the
 * second argument to ftw(3) and nftw(3).  Say it three times fast!
 */
#define FTW_F           0       /* File.  */
#define FTW_D           1       /* Directory.  */
#define FTW_DNR         2       /* Directory without read permission.  */
#define FTW_DP          3       /* Directory with subdirectories visited.  */
#define FTW_NS          4       /* Unknown type; stat() failed.  */
#define FTW_SL          5       /* Symbolic link.  */
#define FTW_SLN         6       /* Sym link that names a nonexistent file.  */

/*
 * Flags for use as the 4th argument to nftw(3).  These may be ORed together.
 */
#define FTW_PHYS        0x01    /* Physical walk, don't follow sym links.  */
#define FTW_MOUNT       0x02    /* The walk does not cross a mount point.  */
#define FTW_DEPTH       0x04    /* Subdirs visited before the dir itself. */
#define FTW_CHDIR       0x08    /* Change to a directory before reading it. */

#define FTW_ACTIONRETVAL	16

#define FTW_CONTINUE		0
#define FTW_STOP			1
#define FTW_SKIP_SUBTREE	2
#define FTW_SKIP_SIBLINGS	3

/****
 *
 * typdefs & structs
 *
 ****/

struct FTW {
        int base;
        int level;
};
#endif

/****
 *
 * function prototypes
 *
 ****/

int noftw(const char *path, int (*fn)(const char *, const struct stat *ptr, int flag, struct FTW *), int depth, int flags);

#endif /* end of NOFTW_DOT_H */

