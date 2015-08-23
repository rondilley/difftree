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

#ifndef HASH_DOT_H
#define HASH_DOT_H

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
#include "mem.h"

/****
 *
 * typedefs and enums
 *
 ****/

struct hashRec_s {
  char *keyString;
  int keyLen;
  void *data;
  time_t lastSeen;
  time_t createTime;
  uint32_t accessCount;
  uint16_t modifyCount;
  struct hashRec_s *prev;
  struct hashRec_s *next;
};

struct hash_s {
  uint32_t size;
  uint32_t totalRecords;
  uint16_t maxDepth;
  uint8_t primeOff;
  struct hashRec_s **records;
};

/****
 *
 * function prototypes
 *
 ****/

uint32_t calcHash( uint32_t hashSize, const void *keyString );
void freeHash( struct hash_s *hash );
int addHashRec( struct hash_s *hash, uint32_t key, char *keyString, void *data, time_t lastSeen );
int addUniqueHashRec( struct hash_s *hash, const  char *keyString, int keyLen, void *data );
struct hash_s *initHash( uint32_t hashSize );
uint32_t searchHash( struct hash_s *hash, const void *keyString );
void updateData( struct hash_s *hash, const void *keyString, const void *data );
void dumpHash( struct hash_s *hash );
struct hashRec_s *getHashRecord( struct hash_s *hash, const void *keyString );
void *getHashData( struct hash_s *hash, const void *keyString );
struct hashRec_s *snoopHashRecord( struct hash_s *hash, const  char *keyString, int keyLen );
struct hashRec_s *snoopHashRecWithKey( struct hash_s *hash, const  char *keyString, int keyLen, uint32_t key );
void *getDataByKey( struct hash_s *hash, uint32_t key, void *keyString );
struct hash_s *dyGrowHash( struct hash_s *oldHash );
struct hash_s *dyShrinkHash( struct hash_s *oldHash );
void *purgeOldHashData( struct hash_s *hash, time_t age );
void *popHash( struct hash_s *hash );
char *hexConvert( const char *keyString, int keyLen, char *buf, const int bufLen );
char *utfConvert( const char *keyString, int keyLen, char *buf, const int bufLen );
uint32_t getHashSize( struct hash_s *hash );
int traverseHash( const struct hash_s *hash, int (*fn) (const struct hashRec_s *hashRec) );

#endif /* end of HASH_DOT_H */

