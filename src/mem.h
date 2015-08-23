/*****
 *
 * Copyright (c) 2009-2014, Ron Dilley
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
 *****/

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
