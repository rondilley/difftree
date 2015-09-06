/*****
 *
 * Description: Hash Function Headers
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

