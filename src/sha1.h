/*
  This file is part of libextractor.
  (C) 2002, 2003 Christian Grothoff (and other contributing authors)
  
  libextractor is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 2, or (at your
  option) any later version.
  
  libextractor is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with libextractor; see the file COPYING.  If not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/
 
#ifndef SHA1_H
#define SHA1_H

/* includes */
#include <string.h>
#include <common.h>

/* typedefs and structs */
typedef struct {
  char data[20];
} HashCode160;

struct SHA1Context {
  unsigned int total[2];
  unsigned int state[5];
  unsigned char buffer[64];
};

/* function prototypes */
#ifdef __cplusplus
extern "C"
{
#endif
  PUBLIC void SHA1Init( struct SHA1Context *ctx );
  PUBLIC void SHA1Final( unsigned char digest[20], struct SHA1Context *ctx );
  PUBLIC void SHA1Update( struct SHA1Context *ctx, unsigned char *input, unsigned int length );
  PUBLIC void hash(void *data, int size, HashCode160 *hc);
#ifdef __cplusplus
}
#endif

#endif /* end of SHA1_H */
