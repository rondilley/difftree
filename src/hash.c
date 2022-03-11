/*****
 *
 * Description: Hash Functions
 *
 * Copyright (c) 2008-2022, Ron Dilley
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
#include "mem.h"

/****
 *
 * local variables
 *
 ****/

/* force selection of good primes */
size_t hashPrimes[] = {
    53, 97, 193, 389, 769, 1543, 3079,
    6151, 12289, 24593, 49157, 98317, 196613, 393241,
    786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653,
    100663319, 201326611, 402653189, 805306457, 1610612741, 0};

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

uint32_t calcHash(uint32_t hashSize, const char *keyString)
{
  int32_t val = 0;
  const char *ptr;
  int i, tmp, keyLen = strlen(keyString) + 1;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Calculating hash\n");
#endif

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }

#ifdef DEBUG
  if (config->debug >= 4)
    printf("DEBUG - hash: %d\n", val % hashSize);
#endif

  return val % hashSize;
}

/****
 *
 * initialize the hash
 *
 ****/

struct hash_s *initHash(uint32_t hashSize)
{
  struct hash_s *tmpHash;
  int i;

  if ((tmpHash = (struct hash_s *)XMALLOC(sizeof(struct hash_s))) EQ NULL)
  {
    fprintf(stderr, "ERR - Unable to allocate hash\n");
    return NULL;
  }
  XMEMSET(tmpHash, 0, sizeof(struct hash_s));

  /* pick a good hash size */
  for (i = 0; ((hashSize > hashPrimes[i]) && (hashPrimes[i] > 0)); i++)
    ;

  if (hashPrimes[i] EQ 0)
  {
    /* size too large */
    fprintf(stderr, "ERR - Hash size too large\n");
    XFREE(tmpHash);
    return NULL;
  }

  tmpHash->primeOff = i;
  tmpHash->size = hashPrimes[i];

  if ((tmpHash->lists = (struct hashRecList_s **)XMALLOC(
           sizeof(struct hashRecList_s *) * tmpHash->size)) EQ NULL)
  {
    fprintf(stderr, "ERR - Unable to allocate hash record list\n");
    XFREE(tmpHash);
    return NULL;
  }
  XMEMSET(tmpHash->lists, 0, sizeof(struct hashRecList_s *) * tmpHash->size);

#ifdef DEBUG
  if (config->debug >= 4)
    printf("DEBUG - Hash initialized [%u]\n", tmpHash->size);
#endif

  return tmpHash;
}

/****
 *
 * empty the hash table
 *
 ****/

void freeHash(struct hash_s *hash)
{
  size_t i, key;

  if (hash != NULL)
  {
    for (key = 0; key < hash->size; key++)
    {
      if (hash->lists[key] != NULL)
      {
        for (i = 0; i < hash->lists[key]->count; i++)
        {
          if (hash->lists[key]->records[i] != NULL)
          {
            XFREE(hash->lists[key]->records[i]->keyString);
            if (hash->lists[key]->records[i]->data != NULL)
              XFREE(hash->lists[key]->records[i]->data);
            hash->lists[key]->records[i]->keyString = NULL;
            XFREE(hash->lists[key]->records[i]);
            hash->lists[key]->records[i] = NULL;
          }
        }
        if (hash->lists[key]->records != NULL)
        {
          XFREE(hash->lists[key]->records);
          hash->lists[key]->records = NULL;
        }
        XFREE(hash->lists[key]);
        hash->lists[key] = NULL;
      }
    }
    if (hash->lists != NULL)
    {
      XFREE(hash->lists);
      hash->lists = NULL;
    }
    XFREE(hash);
    hash = NULL;
  }
}

/****
 *
 * traverse all hash records, calling func() for each one
 *
 ****/

int traverseHash(const struct hash_s *hash, int (*fn)(const struct hashRec_s *hashRec))
{
  size_t i, key;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Traversing hash\n");
#endif

  for (key = 0; key < hash->size; key++)
  {
    if (hash->lists[key] != NULL)
    {
      for (i = 0; i < hash->lists[key]->count; i++)
      {
        if (hash->lists[key]->records[i] != NULL)
        {
          if (fn(hash->lists[key]->records[i]))
            return (FAILED);
        }
      }
    }
  }

  return (TRUE);
}

/****
 *
 * add a unique record to the hash
 *
 ****/

struct hashRec_s *addUniqueHashRec(struct hash_s *hash, const char *keyString, int keyLen, void *data)
{
  uint32_t key;
  int32_t val = 0;
  const char *ptr;
  char oBuf[4096];
  char nBuf[4096];
  int i, tmp, ret, cmpLen, low, high;
  register int mid;
  struct hashRec_s **tmpHashArrayPtr;
  struct hashRec_s *tmpHashRecPtr;

  if (keyLen EQ 0)
    keyLen = strlen(keyString) + 1;

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

  if (key > hash->size)
  {
    fprintf(stderr, "ERR - Key outside of valid record range [%d]\n", key);
  }

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Adding hash [%d] (%s)\n", key,
           hexConvert(keyString, keyLen, nBuf, sizeof(nBuf)));
#endif

  if (hash->lists[key] EQ NULL)
  {
    /* add new list entry to hash pointer buffer */
    if ((hash->lists[key] = (struct hashRecList_s *)XMALLOC(sizeof(struct hashRecList_s))) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate space for hash list record\n");
      return NULL;
    }
    /* add pointer buffer */
    hash->lists[key]->count = 1;
    if ((hash->lists[key]->records = (struct hashRec_s **)XMALLOC(sizeof(struct hashRec_s *) * hash->lists[key]->count)) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate space for hash record list\n");
      hash->lists[key]->count--;
      XFREE(hash->lists[key]);
      hash->lists[key] = NULL;
      return NULL;
    }
    /* add record to pointer buffer */
    if ((hash->lists[key]->records[0] = (struct hashRec_s *)XMALLOC(sizeof(struct hashRec_s))) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate space for hash record\n");
      XFREE(hash->lists[key]->records);
      XFREE(hash->lists[key]);
      hash->lists[key] = NULL;
      return NULL;
    }
    XMEMSET((struct hashRec_s *)hash->lists[key]->records[0], 0,
            sizeof(struct hashRec_s));
    if ((hash->lists[key]->records[0]->keyString = (char *)XMALLOC(keyLen)) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate space for hash label\n");
      XFREE(hash->lists[key]);
      hash->lists[key] = NULL;
      return NULL;
    }
    XMEMCPY((void *)hash->lists[key]->records[0]->keyString, (void *)keyString, keyLen);
    hash->lists[key]->records[0]->keyLen = keyLen;
    if (data != NULL)
      hash->lists[key]->records[0]->data = data;
    hash->lists[key]->records[0]->lastSeen = hash->lists[key]->records[0]->createTime = config->current_time;
  }
  else
  {
    /* search for keyString and insert in sorted hash list if not found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

    do
    {
#ifdef DEBUG
      if (config->debug >= 7)
        printf("DEBUG - snoop hashrec Count: %d L: %d M: %d H: %d\n", hash->lists[key]->count, low, mid, high);
#endif
      if ((ret = strcmp(keyString, hash->lists[key]->records[mid]->keyString)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
        /* existing record found */
#ifdef DEBUG
        if (config->debug >= 4)
        {
          if (keyString[keyLen - 1] EQ 0) // it is a null terminated key string
            printf("DEBUG - Found (%s) in hash table at [%d] at depth [%d]\n", (char *)keyString, key, mid);
          else
            printf("DEBUG - Found (%s) in hash table at [%d] at depth [%d]\n", hexConvert(keyString, keyLen, nBuf, sizeof(nBuf)), key, mid);
        }
#endif
        return NULL;
      }
      mid = low + ((high - low) / 2);
    } while (low < high);

    /* grow the hash list buffer */
    if ((tmpHashArrayPtr = (struct hashRec_s **)XREALLOC(hash->lists[key]->records, sizeof(struct hashRec_s *) * (hash->lists[key]->count + 1))) EQ NULL)
    {
      /* return without adding record, keep existing list */
      fprintf(stderr, "ERR - Unable to allocate space for hash record list\n");
      return NULL;
    }
    hash->lists[key]->records = tmpHashArrayPtr;

    /* create hash record */
    if ((tmpHashRecPtr = (struct hashRec_s *)XMALLOC(sizeof(struct hashRec_s))) EQ NULL)
    {
      /* XXX need better cleanup than this */
      fprintf(stderr, "ERR - Unable to allocate space for hash record\n");
      return NULL;
    }
    XMEMSET((struct hashRec_s *)tmpHashRecPtr, 0, sizeof(struct hashRec_s));
    if ((tmpHashRecPtr->keyString = (char *)XMALLOC(keyLen)) EQ NULL)
    {
      /* XXX need better cleanup than this */
      /* remove the partial record from the list */
      fprintf(stderr, "ERR - Unable to allocate space for hash label\n");
      return NULL;
    }
    XMEMCPY((void *)tmpHashRecPtr->keyString, (void *)keyString, keyLen);
    tmpHashRecPtr->keyLen = keyLen;
    tmpHashRecPtr->data = data;
    tmpHashRecPtr->lastSeen = tmpHashRecPtr->createTime = config->current_time;

    if (mid < hash->lists[key]->count)
    {
      hash->lists[key]->records[hash->lists[key]->count] = NULL;
      memmove(&hash->lists[key]->records[mid + 1], &hash->lists[key]->records[mid], sizeof(struct hashRec_s *) * (hash->lists[key]->count - mid));
      hash->lists[key]->records[mid] = tmpHashRecPtr;
      hash->lists[key]->count++;
    }
    else
      hash->lists[key]->records[hash->lists[key]->count++] = tmpHashRecPtr;
  }

#ifdef DEBUG
  if (config->debug >= 4)
  {
    if (keyString[keyLen - 1] EQ 0) // it is a null terminated key string
      printf("DEBUG - Added hash [%d] (%s) in record list [%d]\n", key, keyString, hash->lists[key]->count);
    else
      printf("DEBUG - Added hash [%d] (%s) in record list [%d]\n", key, hexConvert(keyString, keyLen, nBuf, sizeof(nBuf)), hash->lists[key]->count);
  }
#endif

  hash->totalRecords++;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Record Count: %d\n", hash->totalRecords);
#endif

  return tmpHashRecPtr;
}

/****
 *
 * insert an existing record into the hash
 *
 ****/

int insertUniqueHashRec(struct hash_s *hash, struct hashRec_s *hashRec)
{
  uint32_t key;
  int32_t val = 0;
  const char *ptr;
  char oBuf[4096];
  char nBuf[4096];
  int i, tmp, ret, cmpLen, low, high;
  register int mid;
  struct hashRec_s **tmpHashArrayPtr;

  /* generate the lookup hash */
  for (i = 0; i < hashRec->keyLen; i++)
  {
    val = (val << 4) + (hashRec->keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

  if (key > hash->size)
  {
    fprintf(stderr, "ERR - Key outside of valid record range [%d]\n", key);
  }

#ifdef DEBUG
  if (config->debug >= 3)
  {
    if (hashRec->keyString[hashRec->keyLen - 1] EQ 0) // it is a null terminated key string
      printf("DEBUG - Inserting hash [%d] (%s)\n", key, hashRec->keyString);
    else
      printf("DEBUG - Inserting hash [%d] (%s)\n", key, hexConvert(hashRec->keyString, hashRec->keyLen, nBuf, sizeof(nBuf)));
  }
#endif

  if (hash->lists[key] EQ NULL)
  {
    /* add new list entry to hash pointer buffer */
    if ((hash->lists[key] = (struct hashRecList_s *)XMALLOC(sizeof(struct hashRecList_s))) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate space for hash list record\n");
      return FAILED;
    }
    /* add pointer buffer */
    hash->lists[key]->count = 1;
    if ((hash->lists[key]->records = (struct hashRec_s **)XMALLOC(sizeof(struct hashRec_s *) * hash->lists[key]->count)) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate space for hash record list\n");
      hash->lists[key]->count--;
      XFREE(hash->lists[key]);
      hash->lists[key] = NULL;
      return FAILED;
    }
    hash->lists[key]->records[0] = hashRec;
  }
  else
  {
    /* search for keyString and insert in sorted hash list if not found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

    do
    {
#ifdef DEBUG
      if (config->debug >= 7)
        printf("DEBUG - snoop hashrec Count: %d L: %d M: %d H: %d\n", hash->lists[key]->count, low, mid, high);
#endif

      if ((ret = strcmp(hashRec->keyString, hash->lists[key]->records[mid]->keyString)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
        /* strings match */
        if (hashRec->keyString[hashRec->keyLen - 1] EQ 0) // it is a null terminated key string
          fprintf(stderr, "ERR - Found duplicate hash record [%s][%s] at [%d] in record list [%d]\n", hashRec->keyString, hash->lists[key]->records[mid]->keyString, key, mid);
        else
          fprintf(stderr, "ERR - Found duplicate hash record at [%d] in record list [%d]\n", key, mid);

        return FALSE;
      }
      mid = low + ((high - low) / 2);
    } while (low < high);

    /* grow the hash list buffer */
    if ((tmpHashArrayPtr = (struct hashRec_s **)XREALLOC(hash->lists[key]->records, sizeof(struct hashRec_s *) * (hash->lists[key]->count + 1))) EQ NULL)
    {
      /* return without adding record, keep existing list */
      fprintf(stderr, "ERR - Unable to allocate space for hash record list\n");
      return FAILED;
    }
    hash->lists[key]->records = tmpHashArrayPtr;

    if (mid < hash->lists[key]->count)
    {
      hash->lists[key]->records[hash->lists[key]->count] = NULL;
      memmove(&hash->lists[key]->records[mid + 1], &hash->lists[key]->records[mid], sizeof(struct hashRec_s *) * (hash->lists[key]->count - mid));
      hash->lists[key]->records[mid] = hashRec;
      hash->lists[key]->count++;
    }
    else
      hash->lists[key]->records[hash->lists[key]->count++] = hashRec;
  }

#ifdef DEBUG
  if (config->debug >= 4)
  {
    if (hashRec->keyString[hashRec->keyLen - 1] EQ 0) // it is a null terminated key string
      printf("DEBUG - Added hash [%d] (%s) in record list [%d]\n", key, hashRec->keyString, hash->lists[key]->count);
    else
      printf("DEBUG - Added hash [%d] (%s) in record list [%d]\n", key, hexConvert(hashRec->keyString, hashRec->keyLen, nBuf, sizeof(nBuf)), hash->lists[key]->count);
  }
#endif

  hash->totalRecords++;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Record Count: %d\n", hash->totalRecords);
#endif

  return TRUE;
}

/****
 *
 * get hash record pointer
 *
 ****/

struct hashRec_s *getHashRecord(struct hash_s *hash, const char *keyString, int keyLen)
{
  uint32_t key;
  int32_t val = 0;
  int i, tmp, ret, cmpLen, low, high;
  register int mid;

  if (keyLen EQ 0)
    keyLen = strlen(keyString) + 1;

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Getting record from hash table\n");
#endif

  if (hash->lists[key] != NULL)
  {
    /* search for keyString and hash record if found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

    do
    {

#ifdef DEBUG
      if (config->debug >= 7)
        printf("DEBUG - snoop hashrec Count: %d L: %d M: %d H: %d\n", hash->lists[key]->count, low, mid, high);
#endif

      if ((ret = strcmp(keyString, hash->lists[key]->records[mid]->keyString)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
#ifdef DEBUG
        if (config->debug >= 4)
          printf("DEBUG - Found (%s) in hash table at [%d] in record list [%d]\n",
                 (char *)keyString, key, mid);
#endif
        hash->lists[key]->records[mid]->lastSeen = config->current_time;
        return hash->lists[key]->records[mid];
      }
      mid = low + ((high - low) / 2);
    } while (low < high);
  }

  return NULL;
}

/****
 *
 * snoop hash record pointer
 *
 ****/

struct hashRec_s *snoopHashRecord(struct hash_s *hash, const char *keyString, int keyLen)
{
  uint32_t key;
  int32_t val = 0;
  int i, tmp, ret, cmpLen, low, high;
  register int mid;

  if (keyLen EQ 0)
    keyLen = strlen(keyString) + 1;

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Snooping record from hash table\n");
#endif

  if (hash->lists[key] != NULL)
  {
    /* search for keyString and hash record if found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

    /* use the shortest string */
    if (hash->lists[key]->records[mid]->keyLen < keyLen)
      cmpLen = hash->lists[key]->records[mid]->keyLen;
    else
      cmpLen = keyLen;

#ifdef DEBUG
    if (config->debug >= 7)
      printf("DEBUG - snoop hashrec L: %d M: %d H: %d\n", low, mid, high);
#endif

    do
    {
      if ((ret = XMEMCMP(keyString, hash->lists[key]->records[mid]->keyString, cmpLen)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
        if (keyLen > hash->lists[key]->records[mid]->keyLen)
          low = mid + 1;
        else if (keyLen < hash->lists[key]->records[mid]->keyLen)
          high = mid;
        else
        {
#ifdef DEBUG
          if (config->debug >= 4)
            printf("DEBUG - Found (%s) in hash table at [%d] in record list [%d]\n", (char *)keyString, key, mid);
#endif
          return hash->lists[key]->records[mid];
        }
      }
      mid = low + ((high - low) / 2);
    } while (low < high);
  }

  return NULL;
}

/****
 *
 * get data in hash record
 *
 ****/

void *getHashData(struct hash_s *hash, const char *keyString, int keyLen)
{
  uint32_t key;
  int32_t val = 0;
  int i, tmp, ret, cmpLen, low, high;
  register int mid;

  if (keyLen EQ 0)
    keyLen = strlen(keyString) + 1;

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Getting data from hash table\n");
#endif

  if (hash->lists[key] != NULL)
  {
    /* search for keyString and hash record if found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

    /* use the shortest string */
    if (hash->lists[key]->records[mid]->keyLen < keyLen)
      cmpLen = hash->lists[key]->records[mid]->keyLen;
    else
      cmpLen = keyLen;

#ifdef DEBUG
    if (config->debug >= 7)
      printf("DEBUG - snoop hashrec L: %d M: %d H: %d\n", low, mid, high);
#endif

    do
    {
      if ((ret = XMEMCMP(keyString, hash->lists[key]->records[mid]->keyString, cmpLen)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
        if (keyLen > hash->lists[key]->records[mid]->keyLen)
          low = mid + 1;
        else if (keyLen < hash->lists[key]->records[mid]->keyLen)
          high = mid;
        else
        {
#ifdef DEBUG
          if (config->debug >= 4)
            printf("DEBUG - Found (%s) in hash table at [%d] in record list [%d]\n", (char *)keyString, key, mid);
#endif
          hash->lists[key]->records[mid]->lastSeen = config->current_time;
          return hash->lists[key]->records[mid]->data;
        }
      }
      mid = low + ((high - low) / 2);
    } while (low < high);
  }

  return NULL;
}

/****
 *
 * snoop hash record data
 *
 ****/

void *snoopHashData(struct hash_s *hash, const char *keyString, int keyLen)
{
  uint32_t key;
  int32_t val = 0;
  int i, tmp, ret, cmpLen, low, high;
  register int mid;

  if (keyLen EQ 0)
    keyLen = strlen(keyString) + 1;

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Snooping data from hash table\n");
#endif

  if (hash->lists[key] != NULL)
  {
    /* search for keyString and hash record if found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

    /* use the shortest string */
    if (hash->lists[key]->records[mid]->keyLen < keyLen)
      cmpLen = hash->lists[key]->records[mid]->keyLen;
    else
      cmpLen = keyLen;

#ifdef DEBUG
    if (config->debug >= 7)
      printf("DEBUG - snoop hashrec L: %d M: %d H: %d\n", low, mid, high);
#endif

    do
    {
      if ((ret = XMEMCMP(keyString, hash->lists[key]->records[mid]->keyString, cmpLen)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
        if (keyLen > hash->lists[key]->records[mid]->keyLen)
          low = mid + 1;
        else if (keyLen < hash->lists[key]->records[mid]->keyLen)
          high = mid;
        else
        {
#ifdef DEBUG
          if (config->debug >= 4)
            printf("DEBUG - Found (%s) in hash table at [%d] in record list [%d]\n", (char *)keyString, key, mid);
#endif
          return hash->lists[key]->records[mid]->data;
        }
      }
      mid = low + ((high - low) / 2);
    } while (low < high);
  }

  return NULL;
}

/****
 *
 * remove hash record
 *
 ****/

void *deleteHashRecord(struct hash_s *hash, const char *keyString, int keyLen)
{
  uint32_t key;
  int32_t val = 0;
  int i, tmp, ret, low, high, cmpLen;
  register int mid;
  void *tmpDataPtr;

  if (keyLen EQ 0)
    keyLen = strlen(keyString) + 1;

  /* generate the lookup hash */
  for (i = 0; i < keyLen; i++)
  {
    val = (val << 4) + (keyString[i] & 0xff);
    if ((tmp = (val & 0xf0000000)))
    {
      val = val ^ (tmp >> 24);
      val = val ^ tmp;
    }
  }
  key = val % hash->size;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Searching for record to delete from the hash table\n");
#endif

  if (hash->lists[key] != NULL)
  {
    /* search for keyString and hash record if found */
    low = 0;
    high = hash->lists[key]->count;
    mid = high / 2;

#ifdef DEBUG
    if (config->debug >= 7)
      printf("DEBUG - snoop hashrec L: %d M: %d H: %d\n", low, mid, high);
#endif

    do
    {
      if ((ret = strcmp(keyString, hash->lists[key]->records[mid]->keyString)) > 0)
        low = mid + 1;
      else if (ret < 0)
        high = mid;
      else
      {
#ifdef DEBUG
        if (config->debug >= 4)
          printf("DEBUG - Found (%s) in hash table at [%d] in record list [%d]\n", (char *)keyString, key, mid);
#endif
        tmpDataPtr = hash->lists[key]->records[mid]->data;
        XFREE(hash->lists[key]->records[mid]->keyString);
        XFREE(hash->lists[key]->records[mid]);

        if (hash->lists[key]->count EQ 1)
        {
          /* last record in list */
          XFREE(hash->lists[key]->records);
          XFREE(hash->lists[key]);
          hash->lists[key] = NULL;
          return tmpDataPtr;
        }
        else if (mid < hash->lists[key]->count)
        {
          /* move mem up to fill the hole */
          /* XXX need to add a wrapper in mem.c for memmove */
          memmove(&hash->lists[key]->records[mid], &hash->lists[key]->records[mid + 1], sizeof(struct hashRec_s *) * (hash->lists[key]->count - (mid + 1)));
        }
        hash->lists[key]->count--;
        /* shrink the buffer */
        if ((hash->lists[key]->records = (struct hashRec_s **)XREALLOC(hash->lists[key]->records, sizeof(struct hashRec_s *) * (hash->lists[key]->count))) EQ NULL)
        {
          /* XXX need better cleanup than this */
          fprintf(stderr, "ERR - Unable to allocate space for hash record list\n");
          return NULL;
        }
        /* return the data */
        return tmpDataPtr;
      }
      mid = low + ((high - low) / 2);
    } while (low < high);
  }

  return NULL;
}

/****
 *
 * dynamic hash grow
 *
 ****/

struct hash_s *dyGrowHash(struct hash_s *oldHash)
{
  struct hash_s *tmpHash;
  int i;
  uint32_t tmpKey;

  if (((float)oldHash->totalRecords / (float)oldHash->size) > 0.8)
  {
    /* the hash should be grown */

#ifdef DEBUG
    if (config->debug >= 3)
      printf("DEBUG - R: %d T: %d\n", oldHash->totalRecords, oldHash->size);
#endif

    if (hashPrimes[oldHash->primeOff + 1] EQ 0)
    {
      fprintf(stderr, "ERR - Hash at maximum size already\n");
      return oldHash;
    }
#ifdef DEBUG
    if (config->debug >= 4)
      printf("DEBUG - HASH: Growing\n");
#endif
    if ((tmpHash = initHash(hashPrimes[oldHash->primeOff + 1])) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate new hash\n");
      return oldHash;
    }

    for (tmpKey = 0; tmpKey < oldHash->size; tmpKey++)
    {
      if (oldHash->lists[tmpKey] != NULL)
      {
        for (i = 0; i < oldHash->lists[tmpKey]->count; i++)
        {
          if (insertUniqueHashRec(tmpHash, oldHash->lists[tmpKey]->records[i]) != TRUE)
          {
            fprintf(stderr, "ERR - Failed to insert hash record while growing\n");
            /* XXX need to do properly handle this error */
          }
        }
        /* free old hash record list buffer */
        XFREE(oldHash->lists[tmpKey]->records);
        XFREE(oldHash->lists[tmpKey]);
      }
    }

#ifdef DEBUG
    if (config->debug >= 5)
      printf("DEBUG - Old [RC: %d T: %d] New [RC: %d T: %d]\n", oldHash->totalRecords, oldHash->size, tmpHash->totalRecords, tmpHash->size);
#endif

    if (tmpHash->totalRecords != oldHash->totalRecords)
    {
      fprintf(stderr,
              "ERR - New hash is not the same size as the old hash [%d->%d]\n", oldHash->totalRecords, tmpHash->totalRecords);
    }

    /* free the rest of the old hash buffers */
    XFREE(oldHash->lists);
    XFREE(oldHash);

#ifdef DEBUG
    if (config->debug >= 5)
      printf("DEBUG - HASH: Grew\n");
#endif
    return tmpHash;
  }

  return oldHash;
}

/****
 *
 * dynamic hash shrink
 *
 ****/

struct hash_s *dyShrinkHash(struct hash_s *oldHash)
{
  struct hash_s *tmpHash;
  int i;
  uint32_t tmpKey;

  if ((oldHash->totalRecords / oldHash->size) < 0.3)
  {
    /* the hash should be shrunk */

#ifdef DEBUG
    if (config->debug >= 3)
      printf("DEBUG - R: %d T: %d\n", oldHash->totalRecords, oldHash->size);
#endif

    if (oldHash->primeOff EQ 0)
    {
      fprintf(stderr, "ERR - Hash at minimum size already\n");
      return oldHash;
    }
#ifdef DEBUG
    if (config->debug >= 4)
      printf("DEBUG - HASH: Shrinking\n");
#endif
    if ((tmpHash = initHash(hashPrimes[oldHash->primeOff - 1])) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate new hash\n");
      return oldHash;
    }

    for (tmpKey = 0; tmpKey < oldHash->size; tmpKey++)
    {
      if (oldHash->lists[tmpKey] != NULL)
      {
        for (i = 0; i < oldHash->lists[tmpKey]->count; i++)
        {
          if (insertUniqueHashRec(tmpHash, oldHash->lists[tmpKey]->records[i]) EQ FAILED)
          {
            fprintf(stderr, "ERR - Failed to insert hash record while shrinking\n");
            /* XXX need to do properly handle this error */
          }
        }
        /* free old hash record list buffer */
        XFREE(oldHash->lists[tmpKey]->records);
      }
    }

#ifdef DEBUG
    if (config->debug >= 5)
      printf("DEBUG - Old [RC: %d T: %d] New [RC: %d T: %d]\n",
             oldHash->totalRecords, oldHash->size, tmpHash->totalRecords,
             tmpHash->size);
#endif

    if (tmpHash->totalRecords != oldHash->totalRecords)
    {
      fprintf(stderr,
              "ERR - New hash is not the same size as the old hash [%d->%d]\n",
              oldHash->totalRecords, tmpHash->totalRecords);
    }

    /* free the rest of the old hash buffers */
    XFREE(oldHash->lists);
    XFREE(oldHash);

#ifdef DEBUG
    if (config->debug >= 5)
      printf("DEBUG - HASH: Grew\n");
#endif
    return tmpHash;
  }

  return oldHash;
}

/****
 *
 * get rid of old hash records
 *
 ****/

void **purgeOldHashRecords(struct hash_s *hash, time_t age, void **dataList)
{
  uint32_t i, key;
  size_t count = 0;

  if (dataList EQ NULL)
  {
    if ((dataList = (void **)XMALLOC(sizeof(void *) * (count + 1))) EQ NULL)
    {
      fprintf(stderr, "ERR - Unable to allocate memory for purged data list\n");
      return NULL;
    }
  }
  dataList[count] = NULL;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Purging hash records older than [%ld]\n", age);
#endif

  for (key = 0; key < hash->size; key++)
  {
    if (hash->lists[key] != NULL)
    {
      for (i = 0; i < hash->lists[key]->count; i++)
      {
        if (hash->lists[key]->records[i] != NULL)
        {
          if (hash->lists[key]->records[i]->lastSeen < age)
          {
            /* old record, remove it */
            count++;
            if ((dataList = (void **)XREALLOC(dataList, count)) EQ NULL)
            {
              fprintf(stderr, "ERR - Unable to grow memory for purged data list\n");
              return NULL;
            }
            dataList[count - 1] = hash->lists[key]->records[i]->data;
            dataList[count] = NULL;
            XFREE(hash->lists[key]->records[i]->keyString);
            XFREE(hash->lists[key]->records[i]);

            if (hash->lists[key]->count EQ 1)
            {
              /* last record in list */
              XFREE(hash->lists[key]->records);
              XFREE(hash->lists[key]);
              hash->lists[key] = NULL;
            }
            else if (i < hash->lists[key]->count)
            {
              /* move mem up to fill the hole */
              /* XXX need to add a wrapper in mem.c for memmove */
              memmove(&hash->lists[key]->records[i], &hash->lists[key]->records[i + 1], sizeof(struct hashRec_s *) * (hash->lists[key]->count - (i + 1)));
            }
            hash->lists[key]->count--;
            /* shrink the buffer */
            if ((hash->lists[key]->records = (struct hashRec_s **)XREALLOC(hash->lists[key]->records, sizeof(struct hashRec_s *) * (hash->lists[key]->count))) EQ NULL)
            {
              /* XXX need better cleanup than this */
              fprintf(stderr, "ERR - Unable to allocate space for hash record list\n");
              return NULL;
            }
          }
        }
      }
    }
  }
  return (dataList);
}

/****
 *
 * print key string in hex (just in case it is not ascii)
 *
 ****/

char *hexConvert(const char *keyString, int keyLen, char *buf,
                 const int bufLen)
{
  int i;
  char *ptr = buf;
  for (i = 0; i < keyLen & i < (bufLen / 2) - 1; i++)
  {
    snprintf(ptr, bufLen, "%02x", keyString[i] & 0xff);
    ptr += 2;
  }
  return buf;
}

/****
 *
 * deal with UTF strings
 * XXX this is a terrible way to handle this
 *
 ****/

char *utfConvert(const char *keyString, int keyLen, char *buf,
                 const int bufLen)
{
  int i;
  char *ptr = buf;
  /* XXX should check for buf len */
  for (i = 0; i < (keyLen / 2); i++)
  {
    buf[i] = keyString[(i * 2)];
  }
  buf[i] = '\0';

  return buf;
}

/****
 *
 * return size of hash
 *
 ****/

uint32_t getHashSize(struct hash_s *hash)
{
  if (hash != NULL)
    return hash->size;
  return FAILED;
}
