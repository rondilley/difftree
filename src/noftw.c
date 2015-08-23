/****
 *
 * Copyright (c) 2012-2014, Ron Dilley
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

/****
 *
 * defines
 *
 ****/

/****
 *
 * includes
 *
 ****/

#include "noftw.h"

/****
 *
 * local variables
 *
 ****/

/****
 *
 * external global variables
 *
 ****/

extern Config_t *config;
extern int quit;

/****
 *
 * functions
 *
 ****/

/****
 *
 * re-entrant dir walker
 *
 ****/

int walkDir(const char *path, int (*fn)(const char *, const struct stat *ptr, int flag, struct FTW *), int depth, int flags) {
  DIR *dir;
  struct dirent *dp;
  struct stat s;
  char tmpPathBuf[PATH_MAX];
  int status;

  //printf( "Entering [%s]\n", path );

  if ( ( dir = opendir( path ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to open dir [%s]\n", path );
    return( -1 );
  }

  while( ! quit & ( dp = readdir( dir ) ) != NULL ) {
    if ( ( strncmp( dp->d_name, ".", 2 ) != 0 ) & ( strncmp( dp->d_name, "..", 3 ) != 0 ) ) {
      snprintf( tmpPathBuf, sizeof( tmpPathBuf ), "%s/%s", path, dp->d_name );
      //printf( "%s\n", tmpPathBuf );
      if ( ( status = lstat( tmpPathBuf, &s ) ) EQ 0 ) {
	fn( tmpPathBuf, &s, s.st_mode, NULL );
	if ( S_ISDIR( s.st_mode ) ) {
	  walkDir( tmpPathBuf, fn, depth, flags );
	}
      } else {
	fn( tmpPathBuf, &s, FTW_NS, NULL );
      }
    }
  }

  closedir( dir );

  //printf( "Leaving [%s]\n", path );

  return( 0 );
}

/****
 *
 * traverse (walk) a file tree
 *
 ****/

int noftw(const char *path, int (*fn)(const char *, const struct stat *ptr, int flag, struct FTW *), int depth, int flags) {

  return walkDir( path, fn, depth, flags );
}
