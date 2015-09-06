/*****
 *
 * Description: Memory Helper Function Headers
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

#ifndef MEM_DOT_H
#define MEM_DOT_H

#define XMALLOC(c)      xmalloc_(c,__FILE__,__LINE__)
#define XFREE(ptr)      xfree_(ptr,__FILE__,__LINE__)
#define XFREE_ALL()     xfree_all_(__FILE__,__LINE__)
#define XMEMSET(s,c,n)  xmemset_(s,c,n,__FILE__,__LINE__)
#define XMEMCPY(s,d,n)  xmemcpy_(s,d,n,__FILE__,__LINE__)
#define XREALLOC(s,n)   xrealloc_(s,n,__FILE__,__LINE__)
#define XSTRDUP(s)      xstrdup_(x,__FILE__,__LINE__)
#define XSTRCPY(s,d)    xstrcpy_(s,d,__FILE__,__LINE__)
#define XSTRNCPY(s,d,n) xstrncpy_(s,d,n,__FILE__,__LINE__)

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

/****
 *
 * defines
 *
 ****/

#define MEM_D_STAT_DIRTY 0
#define MEM_D_STAT_CLEAN 1
#define MEM_D_STAT_DE    2

/****
 *
 * typedefs and structs
 *
 ****/

struct Mem_s {
  void *buf_ptr;
  unsigned int buf_size;
  int status;
  struct Mem_s *prev;
  struct Mem_s *next;
};

/****
 *
 * function prototypes
 *
 ****/

char *copy_argv(char *argv[]);
void *xmalloc_( int size, const char *filename, const int linenumber );
void *xrealloc_( void *ptr, int size, const char *filename, const int linenumber);
void *xmemset_( void *ptr, const char value, const int size, const char *filename, const int linenumber );
void *xmemcpy_( void *d_ptr, void *s_ptr, const int size, const char *filename, const int linenumber );
void xfree_( void *ptr, const char *filename, const int linenumber );
void xfree_all_( const char *filename, const int linenumber );
char *xstrdup_( const char *str, const char *filename, const int linenumber );
void xgrow_( void **old, int elementSize, int *oldCount, int newCount, char *filename, const int linenumber );
char *xstrcpy_( char *d_ptr, const char *s_ptr, const char *filename, const int linenumber );
char *xstrncpy_( char *d_ptr, const char *s_ptr, const size_t len, const char *filename, const int linenumber );

#endif /* end of UTIL_DOT_H */
