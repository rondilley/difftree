/*****
 *
 * Description: ftw Functions
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

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - noftw: Entering [%s]\n", path );
#endif
  
  if ( ( dir = opendir( path ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to open dir [%s]\n", path );
    return( -1 );
  }

  while( ! quit && ( ( dp = readdir( dir ) ) != NULL ) ) {
    if ( ( strncmp( dp->d_name, ".", 2 ) != 0 ) & ( strncmp( dp->d_name, "..", 3 ) != 0 ) ) {
      snprintf( tmpPathBuf, sizeof( tmpPathBuf ), "%s/%s", path, dp->d_name );
#ifdef DEBUG
      if ( config->debug >= 5 )
        printf( "DEBUG - noftw: %s\n", tmpPathBuf );
#endif
      
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

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - noftw: Leaving [%s]\n", path );
#endif
  
  return( 0 );
}

/****
 *
 * traverse (walk) a file tree
 *
 ****/

int noftw(const char *path, int (*fn)(const char *, const struct stat *ptr, int flag, struct FTW *), int depth, int flags) {
#ifdef DEBUG
  if ( config->debug )
      printf( "DEBUG - noftw starting [%s]\n", path );
#endif
  
  return walkDir( path, fn, depth, flags );
}
