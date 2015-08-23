/****
*
* Copyright (c) 2012, Ron Dilley
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

