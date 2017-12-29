/*****
 *
 * Description: Hash Functions
 * 
 * Copyright (c) 2008-2017, Ron Dilley
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

#include "hash.h"

/****
 *
 * local variables
 *
 ****/

/* force selection of good primes */
size_t hashPrimes[] = { 53,97,193,389,769,1543,3079,6151,12289,24593,49157,98317,196613,393241,786433,1572869,3145739,6291469,12582917,25165843,50331653,100663319,201326611,402653189,805306457,1610612741,0 };

/****
 *
 * external global variables
 *
 ****/

extern Config_t *config;

/****
 *
 * functions
 *
 ****/

/****
 *
 * calculate hash
 *
 ****/

uint32_t calcHash( uint32_t hashSize, const void *keyString ) {
  int32_t val = 0;
  const char *ptr;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Calculating hash\n" );
#endif
  
  ptr = (char *)keyString;
  while (*ptr != '\0') {
    int tmp;
    val = (val << 4) + (*ptr);
    if ((tmp = (val & 0xf0000000))) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
    ptr++;
  }

#ifdef DEBUG
  if ( config->debug >= 4 )
    printf( "DEBUG - hash: %d\n", val % hashSize );
#endif

  return val % hashSize;
}

/****
 *
 * empty the hash table
 *
 ****/

void freeHash( struct hash_s *hash ) {
  struct hashRec_s *tmpHashRec;
  struct hashRec_s *curHashRec;
  size_t key;
  
  for ( key = 0; key < hash->size; key++ ) {
    curHashRec = hash->records[key];
    while ( curHashRec != NULL ) {
      tmpHashRec = curHashRec;
      curHashRec = curHashRec->next;
      if ( tmpHashRec->data != NULL )
	XFREE( tmpHashRec->data );
      XFREE( tmpHashRec->keyString );
      XFREE( tmpHashRec );
    }
  }
  XFREE( hash->records );
  XFREE( hash );
}

/****
 *
 * traverse all hash records, calling func() for each one
 *
 ****/


int traverseHash( const struct hash_s *hash, int (*fn) (const struct hashRec_s *hashRec) ) {
  struct hashRec_s *curHashRec;
  size_t key;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Traversing hash\n" );
#endif

  for ( key = 0; key < hash->size; key++ ) {
    curHashRec = hash->records[key];
    while ( curHashRec != NULL ) {
      /* external callback */
      if ( fn( curHashRec ) )
	return( FAILED );
      curHashRec = curHashRec->next;
    }
  }

  return( TRUE );
}

/****
 *
 * add a record to the hash
 *
 ****/

int addHashRec( struct hash_s *hash, uint32_t key, char *keyString, void *data, time_t lastSeen ) {
  struct hashRec_s *tmpHashRec;
  struct hashRec_s *curHashRec;
  int tmpDepth = 0;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Adding hash [%d] (%s)\n", key, keyString );
#endif

  if ( hash->records[key] EQ NULL ) {
    /* nope, add it in the current slot */
    if ( ( hash->records[key] = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
      fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
      return FAILED;
    }
    XMEMSET( (struct hashRec_s *)hash->records[key], 0, sizeof( struct hashRec_s ) );
    if ( ( hash->records[key]->keyString = (char *)XMALLOC( strlen( keyString ) + 1 ) ) EQ NULL ) {
      fprintf( stderr, "ERR - Unable to allocate space for hash label\n" );
      XFREE( hash->records[key] );
      return FAILED;
    }
    XSTRCPY( hash->records[key]->keyString, keyString );
    hash->records[key]->data = data;
    hash->records[key]->lastSeen = hash->records[key]->createTime = lastSeen;
    tmpDepth++;
  } else {
    /* yup, traverse the linked list and stick it at the end */

    /* XXX we should make this dynamically optimizing, so the most accessed is at the top of the list */

    /* advance to the end of the chain */
    curHashRec = hash->records[key];
    while( curHashRec != NULL ) {
      if ( curHashRec->next != NULL ) {
	curHashRec = curHashRec->next;
	tmpDepth++;
      } else {
	/* at the end of the chain */
	if ( ( curHashRec->next = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
	  return FAILED;
	}
	XMEMSET( (struct hashRec_s *)curHashRec->next, 0, sizeof( struct hashRec_s ) );
	if ( ( curHashRec->next->keyString = (char *)XMALLOC( strlen( keyString ) + 1 ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to allocate space for hash label\n" );
	  XFREE( curHashRec->next );
	  curHashRec->next = NULL;
	  return FAILED;
	}
	XSTRCPY( curHashRec->next->keyString, keyString );
	curHashRec->next->data = data;
	curHashRec->next->lastSeen = curHashRec->next->createTime = lastSeen;
	curHashRec->next->prev = curHashRec;
	curHashRec = NULL;
      }
    }
  }

  if ( hash->maxDepth < tmpDepth )
    hash->maxDepth = tmpDepth;

  hash->totalRecords++;

  return TRUE;
}

/****
 *
 * add a record to the hash
 *
 ****/

int addUniqueHashRec( struct hash_s *hash, const char *keyString, int keyLen, void *data ) {
  struct hashRec_s *tmpHashRec;
  struct hashRec_s *curHashRec;
  int tmpDepth = 0;
  uint32_t key;
  int32_t val = 0;
  const char *ptr;
  char oBuf[4096];
  char nBuf[4096];
  int i, tmp, ret;
  int done = FALSE;

  if ( keyLen EQ 0 )
    keyLen = strlen( keyString );

  /* generate the lookup hash */
  for( i = 0; i < keyLen; i++ ) {
    val = (val << 4) + ( keyString[i] & 0xff );
    if ((tmp = (val & 0xf0000000))) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

  if ( key > hash->size ) {
    fprintf( stderr, "ERR - Key outside of valid record range [%d]\n", key );
  }

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Adding hash [%d] (%s)\n", key, hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ) );
#endif

  if ( hash->records[key] EQ NULL ) {
    /* nope, add it in the current slot */
    if ( ( hash->records[key] = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
      fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
      return FAILED;
    }
    XMEMSET( (struct hashRec_s *)hash->records[key], 0, sizeof( struct hashRec_s ) );
    if ( ( hash->records[key]->keyString = (char *)XMALLOC( keyLen ) ) EQ NULL ) {
      fprintf( stderr, "ERR - Unable to allocate space for hash label [%s]\n", hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ) );
      XFREE( hash->records[key] );
      hash->records[key] = NULL;
      return FAILED;
    }
    XMEMCPY( (void *)hash->records[key]->keyString, (void *)keyString, keyLen );
    hash->records[key]->keyLen = keyLen;
    if ( data != NULL )
      hash->records[key]->data = data;
    hash->records[key]->lastSeen = hash->records[key]->createTime = time( NULL );
  } else {
    /* yup, traverse the linked list and stick it at the end */

    /* XXX we should make this dynamically optimizing, so the most accessed is at the top of the list */

    /* advance to the end of the chain */
    curHashRec = hash->records[key];
    while( curHashRec != NULL && ! done ) {
      if ( curHashRec->keyLen EQ keyLen ) {
#ifdef DEBUG
	if ( config->debug >= 4 )
	  printf( "DEBUG - keyLen Match\n" );
#endif
	if ( ( ret = memcmp( curHashRec->keyString, keyString, keyLen ) ) EQ 0 ) {
	  /* duplicate, ignore */
#ifdef DEBUG
	  if ( config->debug >= 1 ) {
	    printf( "DEBUG - Ignoring duplicate hash [%s] X [%s]\n", hexConvert( curHashRec->keyString, keyLen, nBuf, sizeof( nBuf )/2 ), hexConvert( keyString, keyLen, nBuf+2048, sizeof( nBuf )/2 ) );
	  }
#endif
	  return FAILED;
	} else if ( ret < 0 ) {
#ifdef DEBUG
	  if ( config->debug >= 2 )
	    printf( "DEBUG - new cmp (%s) > old cmp (%s)\n", hexConvert( keyString, keyLen, nBuf+2048, sizeof( nBuf )/2 ), hexConvert( curHashRec->keyString, keyLen, nBuf, sizeof( nBuf )/2 ) );
#endif
	  if ( curHashRec->next != NULL ) {
#ifdef DEBUG
	    if ( config->debug >= 5 )
	      printf( "DEBUG - Desending\n" );
#endif
	    curHashRec = curHashRec->next;
	    tmpDepth++;
	  } else {
#ifdef DEBUG
	    if ( config->debug >= 5 )
	      printf( "DEBUG - Adding to the end of the chain\n" );
#endif
	    /* at the end of the chain */
	    tmpDepth++;
	    if ( ( curHashRec->next = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
	      fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
	      return FAILED;
	    }
	    XMEMSET( (struct hashRec_s *)curHashRec->next, 0, sizeof( struct hashRec_s ) );
	    if ( ( curHashRec->next->keyString = (char *)XMALLOC( keyLen ) ) EQ NULL ) {
	      fprintf( stderr, "ERR - Unable to allocate space for hash label\n" );
	      XFREE( curHashRec->next );
	      curHashRec->next = NULL;
	      return FAILED;
	    }
	    XMEMCPY( (void *)curHashRec->next->keyString, (void *)keyString, keyLen );
	    curHashRec->next->keyLen = keyLen;
	    curHashRec->next->data = data;
	    curHashRec->next->lastSeen = curHashRec->next->createTime = time( NULL );
	    curHashRec->next->prev = curHashRec;
	    done = TRUE;
	  }
	} else {
#ifdef DEBUG
	  if ( config->debug >= 2 )
	    printf( "DEBUG - old cmp (%s) > new cmp (%s)\n", hexConvert( curHashRec->keyString, keyLen, nBuf, sizeof( nBuf )/2 ), hexConvert( keyString, keyLen, nBuf+2048, sizeof( nBuf )/2 )  );
#endif
	  if ( ( tmpHashRec = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
	    return FAILED;
	  }
	  XMEMSET( (struct hashRec_s *)tmpHashRec, 0, sizeof( struct hashRec_s ) );
	  if ( ( tmpHashRec->keyString = (char *)XMALLOC( keyLen ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate space for hash label\n" );
	    XFREE( tmpHashRec );
	    return FAILED;
	  }
	  XMEMCPY( (void *)tmpHashRec->keyString, (void *)keyString, keyLen );
	  tmpHashRec->keyLen = keyLen;
	  tmpHashRec->data = data;
	  tmpHashRec->lastSeen = tmpHashRec->createTime = time( NULL );
	  tmpHashRec->next = curHashRec;
	  if ( curHashRec->prev EQ NULL ) {
	    /* top of the list */
	    hash->records[key] = tmpHashRec;
	  } else {
	    curHashRec->prev->next = tmpHashRec;
	  }
	  tmpHashRec->prev = curHashRec->prev;
	  curHashRec->prev = tmpHashRec;
	  done = TRUE;
	}
      } else if ( curHashRec->keyLen < keyLen ) {
#ifdef DEBUG
	if ( config->debug >= 2 )
	  printf( "DEBUG - new len (%s) > old len (%s)\n", hexConvert( keyString, keyLen, nBuf+2048, sizeof( nBuf )/2 ), hexConvert( curHashRec->keyString, keyLen, nBuf, sizeof( nBuf )/2 ) );
#endif
	if ( curHashRec->next != NULL ) {
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - Descending\n" );
#endif
	  curHashRec = curHashRec->next;
	  tmpDepth++;
	} else {
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - Adding to the end of the list\n" );
#endif
	  /* at the end of the chain */
	  tmpDepth++;
	  if ( ( curHashRec->next = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
	    return FAILED;
	  }
	  XMEMSET( (struct hashRec_s *)curHashRec->next, 0, sizeof( struct hashRec_s ) );
	  if ( ( curHashRec->next->keyString = (char *)XMALLOC( keyLen ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate space for hash label\n" );
	    XFREE( curHashRec->next );
	    curHashRec->next = NULL;
	    return FAILED;
	  }
	  XMEMCPY( (void *)curHashRec->next->keyString, (void *)keyString, keyLen );
	  curHashRec->next->keyLen = keyLen;
	  curHashRec->next->data = data;
	  curHashRec->next->lastSeen = curHashRec->next->createTime = time( NULL );
	  curHashRec->next->prev = curHashRec;
	  done = TRUE;
	}
      } else {
#ifdef DEBUG
	if ( config->debug >= 2 )
	  printf( "DEBUG - old len (%s) > new len (%s)\n", hexConvert( curHashRec->keyString, keyLen, nBuf, sizeof( nBuf )/2 ), hexConvert( keyString, keyLen, nBuf+2048, sizeof( nBuf )/2 ) );
#endif
	if ( ( tmpHashRec = (struct hashRec_s *)XMALLOC( sizeof( struct hashRec_s ) ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to allocate space for hash\n" );
	  return FAILED;
	}
	XMEMSET( (struct hashRec_s *)tmpHashRec, 0, sizeof( struct hashRec_s ) );
	if ( ( tmpHashRec->keyString = (char *)XMALLOC( keyLen + 1 ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to allocate space for hash label\n" );
	  XFREE( tmpHashRec );
	  return FAILED;
	}
	XMEMCPY( (void *)tmpHashRec->keyString, (void *)keyString, keyLen );
	tmpHashRec->keyLen = keyLen;
	if ( data != NULL )
	  tmpHashRec->data = data;
	tmpHashRec->lastSeen = tmpHashRec->createTime = time( NULL );
	tmpHashRec->next = curHashRec;
	if ( curHashRec->prev EQ NULL ) {
	  /* top of the list */
	  hash->records[key] = tmpHashRec;
	} else {
	  curHashRec->prev->next = tmpHashRec;
	}
	tmpHashRec->prev = curHashRec->prev;
	curHashRec->prev = tmpHashRec;
	done = TRUE;
      }
    }
  }

#ifdef DEBUG
  if ( config->debug >= 4 )
    printf( "DEBUG - Added hash [%d] (%s) at depth [%d]\n", key, hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ), tmpDepth );
#endif

  if ( hash->maxDepth < tmpDepth )
    hash->maxDepth = tmpDepth;

  hash->totalRecords++;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Record Count: %d\n", hash->totalRecords );
#endif

  return TRUE;
}

/****
 *
 * initialize the hash
 *
 ****/

struct hash_s *initHash( uint32_t hashSize ) {
  struct hash_s *tmpHash;
  int i = 0;

  /* nope, alloc my own */
  if ( ( tmpHash = (struct hash_s *)XMALLOC( sizeof( struct hash_s ) ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to allocate hash\n" );
    return NULL;
  }
  XMEMSET( tmpHash, 0, sizeof( struct hash_s ) );

  /* pick a good hash size */
  if ( hashSize EQ 0 ) {
    tmpHash->primeOff = 0;
    tmpHash->size = hashPrimes[0];
  } else {
    while( hashPrimes[i++] != 0 ) {
      if ( hashSize <= hashPrimes[i] ) {
	tmpHash->primeOff = i;
	tmpHash->size = hashPrimes[i];
#ifdef DEBUG
	if ( config->debug >= 4 )
	  printf( "DEBUG - Hash initialized [%u]\n", tmpHash->size );
#endif
	
	/* XXX clean this up, repetative */
	if ( ( tmpHash->records = (struct hashRec_s **)XMALLOC( sizeof( struct hashRec_s * ) * tmpHash->size ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to allocate hash record list\n" );
	  XFREE( tmpHash );
	  return NULL;
	}
	XMEMSET( tmpHash->records, 0, sizeof( struct hashRec_s * ) * tmpHash->size );
	return tmpHash;
      }
    }
    fprintf( stderr, "ERR - Hash size too large\n" );
    XFREE( tmpHash->records );
    XFREE( tmpHash );
    return NULL;
  }

  if ( ( tmpHash->records = (struct hashRec_s **)XMALLOC( sizeof( struct hashRec_s * ) * tmpHash->size ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to allocate hash record list\n" );
    XFREE( tmpHash );
    return NULL;
  }
  XMEMSET( tmpHash->records, 0, sizeof( struct hashRec_s * ) * tmpHash->size );

#ifdef DEBUG
  if ( config->debug >= 4 )
    printf( "DEBUG - Hash initialized [%u]\n", tmpHash->size );
#endif

  return tmpHash;
}

/****
 *
 * find a hash
 *
 ****/

uint32_t searchHash( struct hash_s *hash, const void *keyString ) {
  struct hashRec_s *tmpHashRec = NULL;
  uint32_t key = calcHash( hash->size, keyString );
  int depth = 0;
  int keyLen = strlen( keyString );
  int i;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Searching for (%s) in hash table at [%d]\n", (char *)keyString, key );
#endif

  /* check to see if the hash slot is allocated */
  if ( hash->records[key] EQ NULL ) {
    /* empty hash slot */
#ifdef DEBUG
    if ( config->debug >= 5 )
      printf( "DEBUG - (%s) not found in hash table\n", (char *)keyString );
#endif
    return hash->size+1;
  }

  /* XXX switch to a single while loop */
  tmpHashRec = hash->records[key];
  while( tmpHashRec != NULL ) {
    if ( tmpHashRec->keyLen EQ keyLen ) {
      if ( strcmp( (char *)tmpHashRec->keyString, (char *)keyString ) EQ 0 ) {
#ifdef DEBUG
	if ( config->debug >= 5 )
	  printf( "DEBUG - Found (%s) in hash table at [%d] at depth [%d]\n", (char *)keyString, key, depth );
#endif
	tmpHashRec->lastSeen = time( NULL );
	tmpHashRec->accessCount++;
	return key;
      }
    }
    tmpHashRec = tmpHashRec->next;
  }

#ifdef DEBUG
  if ( config->debug >= 4 )
    printf( "DEBUG - (%s) not found in hash table\n", (char *)keyString );
#endif

  return hash->size+1;
}

/****
 *
 * get hash record pointer
 *
 ****/

struct hashRec_s *getHashRecord( struct hash_s *hash, const void *keyString ) {
  uint32_t key = calcHash( hash->size, keyString );
  int depth = 0;
  int keyLen = strlen( keyString );
  struct hashRec_s *tmpHashRec;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Getting data from hash table\n" );
#endif

  /* XXX switch to a single while loop */
  tmpHashRec = hash->records[key];
  while( tmpHashRec != NULL ) {
    if ( tmpHashRec->keyLen EQ keyLen ) {
      if ( strcmp( tmpHashRec->keyString, (char *)keyString ) EQ 0 ) {
#ifdef DEBUG
	if ( config->debug >= 4 )
	  printf( "DEBUG - Found (%s) in hash table at [%d] at depth [%d]\n", (char *)keyString, key, depth );
#endif
	/* XXX global time, updated periodically would be faster */
	tmpHashRec->lastSeen = time( NULL );
	tmpHashRec->accessCount++;
	return tmpHashRec;
      }
    }
    depth++;
    tmpHashRec = tmpHashRec->next;
  }

  return NULL;
}

/****
 *
 * snoop hash record pointer with precomputed key
 *
 ****/

inline struct hashRec_s *snoopHashRecWithKey( struct hash_s *hash, const  char *keyString, int keyLen, uint32_t key ) {
  struct hashRec_s *tmpHashRec;
  const char *ptr;
  int depth = 0;
#ifdef DEBUG
  char oBuf[4096];
  char nBuf[4096];
#endif

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Searching for [%s]\n",  hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ) );
#endif

  //if ( keyLen EQ 0 )
  //  keyLen = strlen( keyString );

  /* XXX switch to a single while loop */
  
  tmpHashRec = hash->records[key];
  while( tmpHashRec != NULL ) {
    if ( bcmp( tmpHashRec->keyString, keyString, keyLen ) EQ 0 ) {
#ifdef DEBUG
      if ( config->debug >= 4 )
	printf( "DEBUG - Found (%s) in hash table at [%d] at depth [%d] [%s]\n", hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ), key, depth, hexConvert( tmpHashRec->keyString, tmpHashRec->keyLen, oBuf, sizeof( oBuf ) ) );
#endif
      return tmpHashRec;
    }
    tmpHashRec = tmpHashRec->next;
    depth++;
  }

  return NULL;
}

/****
 *
 * snoop hash record pointer
 *
 ****/

struct hashRec_s *snoopHashRecord( struct hash_s *hash, const  char *keyString, int keyLen ) {
  uint32_t key;
  int depth = 0;
  struct hashRec_s *tmpHashRec;
  uint32_t val = 0;
  const char *ptr;
  char oBuf[4096];
  char nBuf[4096];
  int i = 0;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Searching for [%s]\n",  hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ) );
#endif

  if ( keyLen EQ 0 )
    keyLen = strlen( keyString );

  /* generate the lookup hash */
  for( i = 0; i < keyLen; i++ ) {
    int tmp;
    val = (val << 4) + ( keyString[i] & 0xff );
    if ((tmp = (val & 0xf0000000))) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

  /* XXX switch to a single while loop */
  
  tmpHashRec = hash->records[key];
  while( tmpHashRec != NULL ) {
    if ( bcmp( tmpHashRec->keyString, keyString, keyLen ) EQ 0 ) {
#ifdef DEBUG
      if ( config->debug >= 4 )
	printf( "DEBUG - Found (%s) in hash table at [%d] at depth [%d] [%s]\n", hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ), key, depth, hexConvert( tmpHashRec->keyString, tmpHashRec->keyLen, oBuf, sizeof( oBuf ) ) );
#endif
      return tmpHashRec;
    }
    depth++;
    tmpHashRec = tmpHashRec->next;
  }

  return NULL;
}

/****
 *
 * get data in hash record
 *
 ****/

void *getHashData( struct hash_s *hash, const void *keyString ) {
  uint32_t key = calcHash( hash->size, keyString );
  struct hashRec_s *tmpHashRec;

  return getDataByKey( hash, key, (void *)keyString );
}

void *getDataByKey( struct hash_s *hash, uint32_t key, void *keyString ) {
  int depth = 0;
  struct hashRec_s *tmpHashRec;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Getting data from hash table\n" );
#endif

  tmpHashRec = hash->records[key];
  while( tmpHashRec != NULL ) {
    if ( strcmp( tmpHashRec->keyString, (char *)keyString ) EQ 0 ) {
#ifdef DEBUG
      if ( config->debug >= 4 )
	printf( "DEBUG - Found (%s) in hash table at [%d] at depth [%d]\n", (char *)keyString, key, depth );
#endif
      /* XXX would be faster with a global current time updated periodically */
      tmpHashRec->lastSeen = time( NULL );
      tmpHashRec->accessCount++;
      return tmpHashRec->data;
    }
    depth++;
    tmpHashRec = tmpHashRec->next;
  }

  return NULL;
}

/****
 *
 * dump the hash
 *
 ****/

void dumpHash( struct hash_s *hash ) {
  uint32_t key = 0;
  int count = 0;
  struct hashRec_s *tmpHashRec;

  for ( key = 0; key < hash->size; key++ ) {
    tmpHashRec = hash->records[key];
    while( tmpHashRec != NULL ) {
      if ( tmpHashRec->keyString != NULL ) {
	//fprintf( stderr, "%d: %s\n", key, tmpHashRec->keyString );
	count++;
      }
      tmpHashRec = tmpHashRec->next;
    }
  }

  //fprintf( stderr, "%d total items\n", count );
}

/****
 *
 * grow the hash
 *
 ****/

struct hash_s *growHash( struct hash_s *oldHash, size_t newHashSize ) {
  return NULL;
}

/****
 *
 * shrink the hash
 *
 ****/

struct hash_s *shrinkHash( struct hash_s *oldHash, size_t newHashSize ) {
  return NULL;
}

/****
 *
 * dynamic hash grow
 *
 ****/

struct hash_s *dyGrowHash( struct hash_s *oldHash ) {
  struct hash_s *tmpHash;
  struct hashRec_s *curHashRec;
  struct hashRec_s *tmpHashRec;
  int i;
  uint32_t tmpKey;

  if ( ( (float)oldHash->totalRecords / (float)oldHash->size ) > 0.8 ) {
    /* the hash should be grown */
    
#ifdef DEBUG
    if ( config->debug >= 3 )
      printf( "DEBUG - R: %d T: %d\n", oldHash->totalRecords, oldHash->size );
#endif

    if ( hashPrimes[oldHash->primeOff+1] EQ 0 ) {
      fprintf( stderr, "ERR - Hash at maximum size already\n" );
      return oldHash;
    }
#ifdef DEBUG
    if ( config->debug >= 4 )
      printf( "DEBUG - HASH: Growing\n" );
#endif
    if ( ( tmpHash = initHash( hashPrimes[oldHash->primeOff+1] ) ) EQ NULL ) {
      fprintf( stderr, "ERR - Unable to allocate new hash\n" );
      return oldHash;
    }
  
    for ( tmpKey = 0; tmpKey < oldHash->size; tmpKey++ ) {
      curHashRec = oldHash->records[tmpKey];
      
      while ( curHashRec != NULL ) {
	tmpHashRec = curHashRec;
	addUniqueHashRec( tmpHash, curHashRec->keyString, curHashRec->keyLen, curHashRec->data );
	curHashRec = curHashRec->next;
        
	XFREE( tmpHashRec->keyString );
	XFREE( tmpHashRec );
      }
    }

#ifdef DEBUG
    if ( config->debug >= 5 )
      printf( "DEBUG - Old [RC: %d T: %d] New [RC: %d T: %d]\n",
	      oldHash->totalRecords, oldHash->size,
	      tmpHash->totalRecords, tmpHash->size );
#endif

    if ( tmpHash->totalRecords != oldHash->totalRecords ) {
      fprintf( stderr, "ERR - New hash is not the same size as the old hash [%d->%d]\n", oldHash->totalRecords, tmpHash->totalRecords );
    }
    XFREE( oldHash->records );
    XFREE( oldHash );
#ifdef DEBUG
    if ( config->debug >= 5 )
      printf( "DEBUG - HASH: Grew\n" );
#endif
    return tmpHash;
  }

  return oldHash;
}

/****
 *
 * dynamic hash shring
 *
 ****/

struct hash_s *dyShrinkHash( struct hash_s *oldHash ) {
  struct hash_s *tmpHash;
  int i;
  uint32_t tmpKey;

  if ( ( oldHash->totalRecords / oldHash->size ) < 0.3 ) {
    /* the hash should be shrunk */
    if ( oldHash->primeOff EQ 0 )
      return oldHash;

    if ( ( tmpHash = initHash( hashPrimes[oldHash->primeOff-1] ) ) EQ NULL ) {
      fprintf( stderr, "ERR - Unable to allocate new hash\n" );
      return oldHash;
    }

    for( i = 0; i < oldHash->size; i++ ) {
      if ( oldHash->records[i] != NULL ) {
	/* move hash records */
	tmpKey = calcHash( tmpHash->size, oldHash->records[i]->keyString );
	tmpHash->records[tmpKey] = oldHash->records[i];
	oldHash->records[i] = NULL;
      }
    }

    tmpHash->totalRecords = oldHash->totalRecords;
    tmpHash->maxDepth = oldHash->maxDepth;
    freeHash( oldHash );
    return tmpHash;
  }

  return oldHash;
}

/****
 * 
 * remove hash record
 * 
 ****/

void *deleteHashRecord( struct hash_s *hash, const char *keyString, int keyLen ) {
  uint32_t key;
  int depth = 0;
  struct hashRec_s *tmpHashRec;
  uint32_t val = 0;
  const char *ptr;
  char oBuf[4096];
  char nBuf[4096];
  int i = 0;
  void *data;
  
#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Searching for [%s]\n",  hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ) );
#endif

  if ( keyLen EQ 0 )
    keyLen = strlen( keyString );

  /* generate the lookup hash */
  for( i = 0; i < keyLen; i++ ) {
    int tmp;
    val = (val << 4) + ( keyString[i] & 0xff );
    if ((tmp = (val & 0xf0000000))) {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

  /* XXX switch to a single while loop */
  
  tmpHashRec = hash->records[key];
  while( tmpHashRec != NULL ) {
    if ( XMEMCMP( tmpHashRec->keyString, keyString, keyLen ) EQ 0 ) {
#ifdef DEBUG
      if ( config->debug >= 4 )
	printf( "DEBUG - Found (%s) in hash table at [%d] at depth [%d] [%s]\n", hexConvert( keyString, keyLen, nBuf, sizeof( nBuf ) ), key, depth, hexConvert( tmpHashRec->keyString, tmpHashRec->keyLen, oBuf, sizeof( oBuf ) ) );
#endif
      
#ifdef DEBUG
      if ( config->debug >= 3 )
	printf( "DEBUG - Removing hash record\n" );
#endif
      if ( tmpHashRec->prev != NULL )
        tmpHashRec->prev->next = tmpHashRec->next;
      else
        hash->records[key] = tmpHashRec->next;

      if ( tmpHashRec->next != NULL )
        tmpHashRec->next->prev = tmpHashRec->prev;

      data = tmpHashRec->data;
      XFREE( tmpHashRec->keyString );
      XFREE( tmpHashRec );
      hash->totalRecords--;
      
      return data;
    }
    depth++;
    tmpHashRec = tmpHashRec->next;
  }

  return NULL;
}

/****
 *
 * get an old record
 *
 ****/

/* XXX really inefficient, should generate a list of old records and return a linked list */

void *purgeOldHashData( struct hash_s *hash, time_t age ) {
  int i;
  struct hashRec_s *tmpHashRec;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Purging hash records older than [%u]\n", (unsigned int)age );
#endif

  for ( i = 0; i < hash->size; i++ ) {
    tmpHashRec = hash->records[i];
    while ( tmpHashRec != NULL ) {
      if ( tmpHashRec->lastSeen EQ 0 ) {
	fprintf( stderr, "ERR - hash rec with bad time\n" );
      } else if ( tmpHashRec->lastSeen < age ) {
#ifdef DEBUG
	if ( config->debug >= 4 )
	  printf( "DEBUG - Removing old hash record\n" );
#endif
	/* hash is old, remove it */
	if ( tmpHashRec->prev != NULL )
	  tmpHashRec->prev->next = tmpHashRec->next;
	else
	  hash->records[i] = NULL;

	if ( tmpHashRec->next != NULL ) {
	  tmpHashRec->next->prev = tmpHashRec->prev;
	}
	hash->totalRecords--;
	/* if there is data, return for cleanup */
	if ( tmpHashRec->data != NULL )
	  return tmpHashRec->data;
      }
      tmpHashRec = tmpHashRec->next;
    }
  }

  /* no old records */
  return NULL;
}

/****
 *
 * pop data out of hash
 *
 ****/

/* XXX inefficient, should generate a list of records to pop and return a linked list */

void *popHash( struct hash_s *hash ) {
  int i;
  struct hashRec_s *tmpHashRec;

#ifdef DEBUG
  printf( "DEBUG - POPing hash record\n" );
#endif

  for ( i = 0; i < hash->size; i++ ) {
    tmpHashRec = hash->records[i];
    while ( tmpHashRec != NULL ) {
#ifdef DEBUG
      printf( "DEBUG - Popping hash record\n" );
#endif
      if ( tmpHashRec->prev != NULL )
	tmpHashRec->prev->next = tmpHashRec->next;
      else
	hash->records[i] = NULL;

      if ( tmpHashRec->next != NULL ) {
	tmpHashRec->next->prev = tmpHashRec->prev;
      }
      hash->totalRecords--;
      /* if there is data, return for cleanup */
      if ( tmpHashRec->data != NULL )
	return tmpHashRec->data;
    }
    tmpHashRec = tmpHashRec->next;
  }

  /* no old records */
  return NULL;
}

/****
 *
 * print key string in hex (just in case it is not ascii)
 *
 ****/

char *hexConvert( const char *keyString, int keyLen, char *buf, const int bufLen ) {
  int i;
  char *ptr = buf;
  for ( i = 0; i < keyLen & i < (bufLen/2)-1; i++ ) {
    snprintf( ptr, bufLen, "%02x", keyString[i] & 0xff );
    ptr +=2;
  }
  return buf;
}

/****
 *
 * print key string in hex (just in case it is not ascii)
 *
 ****/

char *utfConvert( const char *keyString, int keyLen, char *buf, const int bufLen ) {
  int i;
  char *ptr = buf;
  /* XXX should check for buf len */
  for ( i = 0; i < ( keyLen / 2 ); i++ ) {
    buf[i] =  keyString[(i*2)];
  }
  buf[i] = '\0';

  return buf;
}

/****
 *
 * return size of hash
 *
 ****/

uint32_t getHashSize( struct hash_s *hash ) {
  if ( hash != NULL )
    return hash->size;
  return FAILED;
}
