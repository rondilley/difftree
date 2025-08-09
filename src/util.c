/*****
 *
 * Description: Utility Functions
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

/* turn on priority names */
#define SYSLOG_NAMES

/****
 *
 * includes
 *
 ****/

#include "util.h"

/****
 *
 * local variables
 *
 ****/

PRIVATE char *restricted_environ[] = {
  "IFS= \t\n",
  "PATH=/bin:/usr/bin:/usr/local/bin",
  "SHELL=/bin/sh",
  0
};
PRIVATE char *preserve_environ[] = {
  "TZ",
  "LANG",
  "LC_ALL",
  "LC_CTYPE",
  "USER",
  "HOME",
  0
};

/****
 *
 * external global variables
 *
 ****/

extern Config_t *config;
extern char **environ;

/****
 *
 * functions
 *
 ****/

/****
 *
 * check to see if dir is safe
 *
 ****/
int is_dir_safe( const char *dir ) {
  DIR *fd, *start;
  int rc = FAILED;
  char new_dir[PATH_MAX+1];
  uid_t uid;
  struct stat f, l;

  if ( !( start = opendir( "." ) ) ) return FAILED;
  if ( lstat( dir, &l ) == FAILED ) {
    closedir( start );
    return FAILED;
  }
  uid = geteuid();

  do {
    if ( chdir( dir ) EQ FAILED ) break;
    if ( !( fd = opendir( "." ) ) ) break;

#ifdef LINUX
    if ( fstat( dirfd( fd ), &f ) EQ FAILED ) {
#elif defined MACOS
    if ( fstat( fd->__dd_fd, &f ) EQ FAILED ) {
#elif defined CYGWIN
    if ( fstat( fd->__d_fd, &f ) EQ FAILED ) {
#elif defined MINGW
    if ( fstat( fd->dd_handle, &f ) EQ FAILED ) {
#elif defined FREEBSD
    if ( fstat( dirfd( fd ), &f ) EQ FAILED ) { 
#elif defined OPENBSD
    if ( fstat( dirfd( fd ), &f ) EQ FAILED ) { 
#else
    if ( fstat( fd->dd_fd, &f ) EQ FAILED ) {
#endif

      closedir( fd );
      break;
    }
    closedir( fd );

    if ( l.st_mode != f.st_mode || l.st_ino != f.st_ino || l.st_dev != f.st_dev )
      break;
#ifdef MINGW
	if ( f.st_uid && f.st_uid != uid ) {
#else
    if ( ( f.st_mode & ( S_IWOTH | S_IWGRP ) ) || ( f.st_uid && f.st_uid != uid ) ) {
#endif
      rc = 0;
      break;
    }
    dir = "..";
    if ( lstat( dir, &l ) EQ FAILED ) break;
    if ( !getcwd( new_dir, PATH_MAX + 1 ) ) break;
  } while ( new_dir[1] ); /* new_dir[0] will always be a slash */
  if ( !new_dir[1] ) rc = 1;

#ifdef LINUX
  rc = fchdir( dirfd( start ) );
#elif defined MACOS
  rc = fchdir( start->__dd_fd );
#elif defined CYGWIN
  rc = fchdir( start->__d_fd );
#elif defined MINGW
  rc = fchdir( start->dd_handle );
#elif defined FREEBSD
  rc = fchdir( dirfd( start ) );
#elif defined OPENBSD
  rc = fchdir( dirfd( start ) );
#else
  rc = fchdir( start->dd_fd );
#endif

  closedir( start );
  return rc;
}

/****
 *
 * safely open a file for writing
 *
 ****/

#ifdef UNUSED
static int safe_open( const char *filename ) {
  int fd;
  struct stat sb;
  XMEMSET( &sb, 0, sizeof( struct stat ) );
                                                                 
  if ( lstat(filename, &sb) EQ FAILED ) {
    if (errno != ENOENT)
      return( FAILED );
  } else if ( ( sb.st_mode & S_IFREG) EQ 0 ) {
    errno = EOPNOTSUPP;
    return ( FAILED );
  }

  unlink( filename );
#ifdef MINGW
  fd = open( filename, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR );
#else
  fd = open( filename, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH );
#endif

  return (fd);
}
#endif

/****
 *
 * cleaup pid file
 *
 ****/

#ifdef UNUSED
static void cleanup_pid_file( const char *filename ) {
  if ( strlen( filename ) > 0 ) {
    unlink( filename );
  }
}
#endif

/****
 *
 * sanitize environment
 *
 ****/

void sanitize_environment( void ) {
  int i;
  char **new_environ;
  char *ptr, *value, *var;
  size_t arr_size = 1;
  size_t arr_ptr = 0;
  size_t len, value_len;
  size_t new_size = 0;
  size_t max_env_len = 32768; /* Maximum length for any environment variable */

  /* Calculate size needed for restricted environment variables */
  for( i = 0; (var = restricted_environ[i]) != 0; i++ ) {
    len = strlen( var );
    if (len > max_env_len) {
      fprintf(stderr, "ERR - Environment variable too long: %s\n", var);
      continue;
    }
    new_size += len + 1;
    arr_size++;
  }

  /* Calculate size needed for preserved environment variables */
  for ( i = 0; (var = preserve_environ[i]) != 0; i++ ) {
    if ( !(value = getenv(var))) continue;
    
    /* Validate environment variable name and value */
    len = strlen( var );
    value_len = strlen( value );
    
    if (len == 0 || len > 256 || value_len > max_env_len) {
      fprintf(stderr, "ERR - Invalid environment variable: %s\n", var);
      continue;
    }
    
    /* Check for suspicious characters in variable name */
    for (size_t j = 0; j < len; j++) {
      if (!isalnum(var[j]) && var[j] != '_') {
        fprintf(stderr, "ERR - Invalid character in env var name: %s\n", var);
        goto skip_var;
      }
    }
    
    /* Check for null bytes and other dangerous characters in value */
    for (size_t j = 0; j < value_len; j++) {
      if (value[j] == '\0' || value[j] == '\n' || value[j] == '\r') {
        fprintf(stderr, "ERR - Dangerous character in env var value: %s\n", var);
        goto skip_var;
      }
    }
    
    new_size += len + value_len + 2; /* var + '=' + value + '\0' */
    arr_size++;
    
    skip_var:
    continue;
  }

  /* Allocate memory for new environment */
  new_size += ( arr_size * sizeof( char * ) );
  new_environ = (char **)XMALLOC( new_size );
  XMEMSET(new_environ, 0, new_size);
  
  /* Set up pointer to string data area */
  ptr = ( char * )new_environ + (arr_size * sizeof(char *));

  /* Copy restricted environment variables */
  for ( i = 0; ( var = restricted_environ[i] ) != 0; i++ ) {
    len = strlen( var );
    if (len > max_env_len) continue;
    
    new_environ[arr_ptr++] = ptr;
    XMEMCPY( ptr, var, len + 1 );
    ptr += len + 1;
  }

  /* Copy preserved environment variables */
  for ( i = 0; ( var = preserve_environ[i] ) != 0; i++ ) {
    if ( !( value = getenv( var ) ) ) continue;
    
    len = strlen( var );
    value_len = strlen( value );
    
    /* Re-validate lengths and characters */
    if (len == 0 || len > 256 || value_len > max_env_len) continue;
    
    /* Validate variable name again */
    int valid = 1;
    for (size_t j = 0; j < len; j++) {
      if (!isalnum(var[j]) && var[j] != '_') {
        valid = 0;
        break;
      }
    }
    if (!valid) continue;
    
    /* Validate value again */
    for (size_t j = 0; j < value_len; j++) {
      if (value[j] == '\0' || value[j] == '\n' || value[j] == '\r') {
        valid = 0;
        break;
      }
    }
    if (!valid) continue;
    
    new_environ[arr_ptr++] = ptr;
    
    /* Copy variable name */
    XMEMCPY( ptr, var, len );
    ptr += len;
    
    /* Add equals sign */
    *ptr = '=';
    ptr++;
    
    /* Copy value */
    XMEMCPY( ptr, value, value_len + 1 );
    ptr += value_len + 1;
  }

  /* Null terminate the environment array */
  new_environ[arr_ptr] = NULL;
  
  /* Replace global environment */
  environ = new_environ;
}

/****
 *
 * show environment for debugging
 *
 ****/

void show_environment( void ) {
  extern char **environ;
  char **env = environ;
  int count = 0;
  
  printf("DEBUG - Sanitized environment variables:\n");
  while (*env) {
    printf("DEBUG - ENV[%d]: %s\n", count++, *env);
    env++;
  }
  printf("DEBUG - Total environment variables: %d\n", count);
}

/****
 *
 * is it an odd number
 *
 ****/

inline int isodd(const int n) {
  return n % 2;
}

/****
 *
 * is it an even number
 *
 ****/

inline int iseven(const int n) {
  return !isodd(n);
}
