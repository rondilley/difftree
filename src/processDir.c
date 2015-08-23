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

/****
 *
 * includes
 *.
 ****/

#include "processDir.h"

/****
 *
 * local variables
 *
 ****/

FILE *out;

/****
 *
 * global variables
 *
 ****/

/****
 *
 * external variables
 *
 ****/

extern int errno;
extern char **environ;
extern struct hash_s *baseDirHash;
extern struct hash_s *compDirHash;
extern int quit;
extern int baseDirLen;
extern int compDirLen;
extern char *baseDir;
extern char *compDir;
extern Config_t *config;

/****
 * 
 * functions
 *
 ****/

/****
 *
 * process FTW() record
 *
 * NOTE: This is a callback function
 *
 ****/

#define FTW_RECORD 0
#define FILE_RECORD 1

static int processFtwRecord(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
  return processRecord( fpath, sb, FTW_RECORD, NULL );
}

/****
 *
 * process record
 *
 ****/

inline int processRecord( const char *fpath, const struct stat *sb, char mode, unsigned char *digestPtr ) {
  struct hashRec_s *tmpRec;
  struct stat *tmpSb;
  metaData_t *tmpMD;
  char tmpBuf[1024];
  char diffBuf[4096];
  struct tm *tmPtr;
  MD5_CTX ctx;
  FILE *inFile;
  size_t rCount;
  unsigned char rBuf[16384];
  unsigned char digest[16];
  int tflag = sb->st_mode & S_IFMT;

  bzero( &ctx, sizeof( ctx ) );
  bzero( digest, sizeof( digest ) );
  bzero( rBuf, sizeof( rBuf ) );
  bzero( tmpBuf, sizeof( tmpBuf ) );
  bzero( diffBuf, sizeof( diffBuf ) );

  if ( strlen( fpath ) <= compDirLen ) {
    /* ignore the root */
    return( FTW_CONTINUE );
  }

  if ( quit ) {
    /* graceful shutdown */
    return( FTW_STOP );
  }

  if ( baseDirHash != NULL ) {
    /* compare */
    if ( config->debug >=  5 )
      printf( "DEBUG - [%s]\n", fpath+compDirLen );

    /* hash regular files */
    if ( config->hash && ( tflag EQ S_IFREG ) ) {
      if ( mode EQ FTW_RECORD ) {
#ifdef DEBUG
	if ( config->debug >= 3 )
	  printf( "DEBUG - Generating MD5 of file [%s]\n", fpath );
#endif
	MD5_Init( &ctx );
	if ( ( inFile = fopen( fpath, "r" ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to open file [%s]\n", fpath );
	} else {
	  while( ( rCount = fread( rBuf, 1, sizeof( rBuf ), inFile ) ) > 0 ) {
#ifdef DEBUG
	    if ( config->debug >= 6 )
	      printf( "DEBUG - Read [%ld] bytes from [%s]\n", (long int)rCount, fpath );
#endif
	    MD5_Update( &ctx, rBuf, rCount );
	  }
	  fclose( inFile );
	  MD5_Final( digest, &ctx );
	}
      } else if ( mode EQ FILE_RECORD ) {
	if ( digestPtr != NULL ) {
#ifdef DEBUG
	  if ( config->debug >= 3 )
	    printf( "DEBUG - Loading previously generated MD5 of file [%s]\n", fpath );
#endif
	  XMEMCPY( digest, digestPtr, sizeof( digest ) );
	}
      } else {
	/* unknown record type */
      }
    }

    if ( ( tmpRec = getHashRecord( baseDirHash, fpath+compDirLen ) ) EQ NULL ) {
      printf( "+ %s [%s]\n",
	      (tflag == S_IFDIR) ?   "d"   : (tflag == S_IFCHR) ? "chr" :
	      (tflag == S_IFBLK) ?  "blk"  : (tflag == S_IFREG) ?   "f" :
#ifndef MINGW
	      (tflag == S_IFSOCK) ?  "sok"  : (tflag == S_IFLNK) ?  "sl" :
#endif
	      (tflag == S_IFIFO) ? "fifo" : "???",
	      fpath+compDirLen );
    } else {
#ifdef DEBUG
      if ( config->debug >= 3 )
	printf( "DEBUG - Found match, comparing metadata\n" );
#endif
      tmpMD = (metaData_t *)tmpRec->data;
      tmpSb = (struct stat *)&tmpMD->sb;

      if ( sb->st_size != tmpSb->st_size ) {
#ifdef HAVE_SNPRINTF
	snprintf( tmpBuf, sizeof( tmpBuf ), "s[%ld->%ld] ", (long int)tmpSb->st_size, (long int)sb->st_size );
#else
	sprintf( tmpBuf, "s[%ld->%ld] ", (long int)tmpSb->st_size, (long int)sb->st_size );
#endif

#ifdef HAVE_STRNCAT
	strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
      } else {
	if ( config->hash && ( tflag EQ S_IFREG ) ) {
	  if ( memcmp( tmpMD->digest, digest, sizeof( digest ) ) != 0 ) {
	    /* MD5 does not match */
#ifdef HAVE_SNPRINTF
	    snprintf( tmpBuf, sizeof( tmpBuf ), "md5[%s->", hash2hex( tmpMD->digest, rBuf, sizeof( digest ) ) );
#else
	    sprintf( tmpBuf, "md5[%s->", hash2hex( tmpMD->digest, rBuf, sizeof( digest ) ) );
#endif
#ifdef HAVE_STRNCAT
	    strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	    strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
#ifdef HAVE_SNPRINTF
	    snprintf( tmpBuf, sizeof( tmpBuf ), "%s] ", hash2hex( digest, rBuf, sizeof( digest ) ) );
#else
	    sprintf( tmpBuf, "%s] ", hash2hex( digest, rBuf, sizeof( digest ) ) );
#endif
#ifdef HAVE_STRNCAT
	    strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	    strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	  }
	}
      }

      if ( !config->quick ) {
	/* do more testing */

	/* uid/gid */
	if ( sb->st_uid != tmpSb->st_uid ) {
#ifdef HAVE_SNPRINTF
	  snprintf( tmpBuf, sizeof( tmpBuf ), "u[%d>%d] ", tmpSb->st_uid, sb->st_uid );
#else
	  sprintf( tmpBuf, "u[%d>%d] ", tmpSb->st_uid, sb->st_uid );
#endif
#ifdef HAVE_STRNCAT
          strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	  strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	}
	if ( sb->st_gid != tmpSb->st_gid ) {
#ifdef HAVE_SNPRINTF
	  snprintf( tmpBuf, sizeof( tmpBuf ), "g[%d>%d] ", tmpSb->st_gid, sb->st_gid );
#else
	  sprintf( tmpBuf, "g[%d>%d] ", tmpSb->st_gid, sb->st_gid );
#endif

#ifdef HAVE_STRNCAT
          strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	  strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	}

	/* file type */
	if ( ( sb->st_mode & S_IFMT ) != ( tmpSb->st_mode & S_IFMT ) ) {
#ifdef HAVE_SNPRINTF
	  snprintf( tmpBuf, sizeof( tmpBuf ), "t[%s->%s] ",
#else
	  sprintf( tmpBuf, "t[%s->%s] ",
#endif
		   ((tmpSb->st_mode & S_IFMT) == S_IFDIR) ?   "d"   :
		   ((tmpSb->st_mode & S_IFMT) == S_IFBLK) ?  "blk"  :
		   ((tmpSb->st_mode & S_IFMT) == S_IFREG) ?   "f" :
		   ((tmpSb->st_mode & S_IFMT) == S_IFCHR) ?  "chr" :
#ifndef MINGW
		   ((tmpSb->st_mode & S_IFMT) == S_IFSOCK) ? "sok" :
		   ((tmpSb->st_mode & S_IFMT) == S_IFLNK) ?  "sl" :
#endif
		   ((tmpSb->st_mode & S_IFMT) == S_IFIFO) ? "fifo" : "???",
		   ((sb->st_mode & S_IFMT) == S_IFDIR) ?   "d"   :
		   ((sb->st_mode & S_IFMT) == S_IFBLK) ?  "blk"  :
		   ((sb->st_mode & S_IFMT) == S_IFREG) ?   "f" :
		   ((sb->st_mode & S_IFMT) == S_IFCHR) ?  "chr"  :
#ifndef MINGW
		   ((sb->st_mode & S_IFMT) == S_IFSOCK) ? "sok" :
		   ((sb->st_mode & S_IFMT) == S_IFLNK) ?  "sl" :
#endif
		   ((sb->st_mode & S_IFMT) == S_IFIFO) ? "fifo" : "???" );

#ifdef HAVE_STRNCAT
          strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	  strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	}

	/* permissions */
	if ( ( sb->st_mode & 0xffff ) != ( tmpSb->st_mode & 0xffff ) ) {
#ifdef HAVE_SNPRINTF
	  snprintf( tmpBuf, sizeof( tmpBuf ), "p[%c%c%c%c%c%c%c%c%c->%c%c%c%c%c%c%c%c%c] ",
#else
	  sprintf( tmpBuf, "p[%c%c%c%c%c%c%c%c%c->%c%c%c%c%c%c%c%c%c] ",
#endif
#ifdef MINGW
		   (tmpSb->st_mode & S_IREAD ) ? 'r' : '-',
		   (tmpSb->st_mode & S_IWRITE ) ? 'w' : '-',
		   (tmpSb->st_mode & S_IEXEC ) ? 'x' : '-',
		   '-','-','-','-','-','-',
#else
		   (tmpSb->st_mode & S_IRUSR ) ? 'r' : '-',
		   (tmpSb->st_mode & S_IWUSR ) ? 'w' : '-',
		   (tmpSb->st_mode & S_ISUID ) ? 's' : (tmpSb->st_mode & S_IXUSR) ? 'x' : '-',
		   (tmpSb->st_mode & S_IRGRP ) ? 'r' : '-',
		   (tmpSb->st_mode & S_IWGRP ) ? 'w' : '-',
		   (tmpSb->st_mode & S_ISGID ) ? 's' : (tmpSb->st_mode & S_IXGRP) ? 'x' : '-',
		   (tmpSb->st_mode & S_IROTH ) ? 'r' : '-',
		   (tmpSb->st_mode & S_IWOTH ) ? 'w' : '-',
		   (tmpSb->st_mode & S_ISVTX ) ? 's' : (tmpSb->st_mode & S_IXOTH) ? 'x' : '-',
#endif
#ifdef MINGW
		   (sb->st_mode & S_IREAD ) ? 'r' : '-',
		   (sb->st_mode & S_IWRITE ) ? 'w' : '-',
		   (sb->st_mode & S_IEXEC ) ? 'x' : '-',
		   '-','-','-','-','-','-'
#else
		   (sb->st_mode & S_IRUSR ) ? 'r' : '-',
		   (sb->st_mode & S_IWUSR ) ? 'w' : '-',
		   (sb->st_mode & S_ISUID ) ? 's' : (sb->st_mode & S_IXUSR) ? 'x' : '-',
		   (sb->st_mode & S_IRGRP ) ? 'r' : '-',
		   (sb->st_mode & S_IWGRP ) ? 'w' : '-',
		   (sb->st_mode & S_ISGID ) ? 's' : (sb->st_mode & S_IXGRP) ? 'x' : '-',
		   (sb->st_mode & S_IROTH ) ? 'r' : '-',
		   (sb->st_mode & S_IWOTH ) ? 'w' : '-',
		   (sb->st_mode & S_ISVTX ) ? 's' : (sb->st_mode & S_IXOTH) ? 'x' : '-'
#endif
		   );
#ifdef HAVE_STRNCAT
          strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	  strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	}

	if ( sb->st_mtime != tmpSb->st_mtime ) {
	  tmPtr = localtime( &tmpSb->st_mtime );
#ifdef HAVE_SNPRINTF
	  snprintf( tmpBuf, sizeof( tmpBuf ), "mt[%04d/%02d/%02d@%02d:%02d:%02d->",
#else
	  sprintf( tmpBuf, "mt[%04d/%02d/%02d@%02d:%02d:%02d->",
#endif
		   tmPtr->tm_year+1900, tmPtr->tm_mon+1, tmPtr->tm_mday,
		   tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec );
#ifdef HAVE_STRNCAT
          strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	  strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	  tmPtr = localtime( &sb->st_mtime );
#ifdef HAVE_SNPRINTF
	  snprintf( tmpBuf, sizeof( tmpBuf ), "%04d/%02d/%02d@%02d:%02d:%02d] ",
#else
	  sprintf( tmpBuf, "%04d/%02d/%02d@%02d:%02d:%02d] ",
#endif
		   tmPtr->tm_year+1900, tmPtr->tm_mon+1, tmPtr->tm_mday,
		   tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec );
#ifdef HAVE_STRNCAT
          strncat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#else
	  strlcat( diffBuf, tmpBuf, sizeof( diffBuf ) - 1 );
#endif
	}
      }

      if ( strlen( diffBuf ) ) {
	printf( "%s%s [%s]\n", diffBuf,
		(tflag == S_IFDIR) ?   "d"   :
		(tflag == S_IFBLK) ?  "blk"  :
		(tflag == S_IFREG) ?   "f" :
		(tflag == S_IFCHR) ?  "chr"  :
#ifndef MINGW
		(tflag == S_IFSOCK) ? "sok" :
		(tflag == S_IFLNK) ?  "sl" :
#endif
		(tflag == S_IFIFO) ? "fifo" : "???",
		fpath );
      }
#ifdef DEBUG
      else {
	if ( config->debug > 4 )
	  printf( "DEBUG - Record matches\n" );
      }
#endif

    }

    /* store file metadata */
    tmpMD = (metaData_t *)XMALLOC( sizeof( metaData_t ) );
    XMEMSET( tmpMD, 0, sizeof( metaData_t ) );
    /* save the hash for regular files */
    if ( config->hash && ( tflag EQ S_IFREG ) ) {
#ifdef DEBUG
      if ( config->debug >= 3 )
	printf( "DEBUG - Adding hash [%s] for file [%s]\n", hash2hex( digest, rBuf, sizeof( digest ) ), fpath );
#endif
      XMEMCPY( tmpMD->digest, digest, sizeof( digest ) );
    }
    XMEMCPY( (void *)&tmpMD->sb, (void *)sb, sizeof( struct stat ) );
    addUniqueHashRec( compDirHash, fpath+compDirLen, strlen( fpath+compDirLen ), tmpMD );
    /* check to see if the hash should be grown */
    compDirHash = dyGrowHash( compDirHash );
  } else {
    /* store file metadata */
    tmpMD = (metaData_t *)XMALLOC( sizeof( metaData_t ) );
    XMEMSET( tmpMD, 0, sizeof( metaData_t ) );

    /* hash regular files */
    if ( config->hash && ( tflag EQ S_IFREG ) ) {
      if ( mode EQ FTW_RECORD ) {
#ifdef DEBUG
	if ( config->debug >= 3 )
	  printf( "DEBUG - Generating MD5 of file [%s]\n", fpath );
#endif
	MD5_Init( &ctx );
	if ( ( inFile = fopen( fpath, "r" ) ) EQ NULL ) {
	  fprintf( stderr, "ERR - Unable to open file [%s]\n", fpath );
	} else {
	  while( ( rCount = fread( rBuf, 1, sizeof( rBuf ), inFile ) ) > 0 ) {
#ifdef DEBUG
	    if ( config->debug >= 6 )
	      printf( "DEBUG - Read [%ld] bytes from [%s]\n", (long int)rCount, fpath );
#endif
	    MD5_Update( &ctx, rBuf, rCount );
	  }
	  fclose( inFile );
	  MD5_Final( digest, &ctx );
	}
      } else if ( mode EQ FILE_RECORD ) {
	if ( digestPtr != NULL ) {
#ifdef DEBUG
	  if ( config->debug >= 3 )
	    printf( "DEBUG - Loading previously generated MD5 of file [%s]\n", fpath );
#endif
	  XMEMCPY( digest, digestPtr, sizeof( digest ) );
	}
      } else {
	/* unknown record type */
      }
      XMEMCPY( tmpMD->digest, digest, sizeof( digest ) );
    }

    XMEMCPY( (void *)&tmpMD->sb, (void *)sb, sizeof( struct stat ) );
#ifdef DEBUG
    if ( config->debug >= 5 )
      printf( "DEBUG - Adding RECORD\n" );
#endif
    addUniqueHashRec( compDirHash, fpath+compDirLen, strlen( fpath+compDirLen ), tmpMD );
    /* check to see if the hash should be grown */
    compDirHash = dyGrowHash( compDirHash );
  }

  return( FTW_CONTINUE );
}

/****
 *
 * process directory tree
 *
 ****/

PUBLIC int processDir( char *dirStr ) {
  struct stat sb;
  char realDirStr[PATH_MAX];
  int tflag, ret;

  if ( realpath( dirStr, realDirStr ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to determine real path\n" );
    return( FAILED );
  }

  if ( lstat( realDirStr, &sb ) EQ 0 ) {
    /* file exists, make sure it is a file */
    tflag = sb.st_mode & S_IFMT;
    if ( tflag EQ S_IFREG ) {
      /* process file */
      printf( "Processing file [%s]\n", realDirStr );
      if ( ( ret = loadFile( realDirStr ) ) EQ (-1) ) {
		fprintf( stderr, "ERR - Problem while loading file\n" );
		return( FAILED );
	  } else if ( ret EQ FTW_STOP ) {
		fprintf( stderr, "ERR - loadFile() was interrupted by a signal\n");
		return( FAILED );
	  }	
    } else if ( tflag EQ S_IFDIR ) {
      /* process directory */
      printf( "Processing dir [%s]\n", realDirStr );
      
#ifdef HAVE_NFTW
      if ( ( ret = nftw( realDirStr, processFtwRecord, 20, FTW_PHYS | FTW_ACTIONRETVAL ) ) EQ (-1) ) {
#else
      if ( ( ret = noftw( realDirStr, processFtwRecord, 20, FTW_PHYS | FTW_ACTIONRETVAL ) ) EQ (-1) ) {
#endif
	fprintf( stderr, "ERR - Unable to open dir [%s]\n", realDirStr );
	return ( FAILED );
      } else if ( ret EQ FTW_STOP ) {
	fprintf( stderr, "ERR - nftw() was interrupted by a signal\n" );
	return( FAILED );
      }

      /* write data to file */
      if ( config->outfile != NULL ) {
	writeDirHash2File( compDirHash, compDir, config->outfile );
	/* only write out the first read dir */
	XFREE( config->outfile );
	config->outfile = NULL;
      }

#ifdef DEBUG
      if ( config->debug >= 2 )
	printf( "DEBUG - Finished processing dir [%s]\n", realDirStr );
#endif
    } else {
      fprintf( stderr, "ERR - [%s] is not a regular file or a directory\n", realDirStr );
      return( FAILED );
    }
  } else {
    fprintf( stderr, "ERR - Unable to stat file [%s]\n", realDirStr );
    return( FAILED );
  }

  return( TRUE );
}

/****
 *
 * check for missing files
 *
 ****/

int findMissingFiles( const struct hashRec_s *hashRec ) {
  struct hashRec_s *tmpRec;
  metaData_t *tmpMD;
  struct stat *tmpSb;
  int tflag;

#ifdef DEBUG
  if ( config->debug >= 3 )
    printf( "DEBUG - Searching for [%s]\n", hashRec->keyString );
#endif

  if ( ( tmpRec = getHashRecord( compDirHash, hashRec->keyString ) ) EQ NULL ) {
    tmpMD = (metaData_t *)hashRec->data;
    tmpSb = (struct stat *)&tmpMD->sb;
    tflag = tmpSb->st_mode & S_IFMT;
    printf( "- %s [%s]\n", 
	    (tflag == S_IFDIR) ?   "d"   :
	    (tflag == S_IFBLK) ?  "blk"  :
		(tflag == S_IFREG) ?   "f" :
	    (tflag == S_IFCHR) ?  "chr"  :
#ifndef MINGW
		(tflag == S_IFSOCK) ? "sok" :
		(tflag == S_IFLNK) ?  "sl" :
#endif
	    (tflag == S_IFIFO) ? "fifo" : "???",
	    hashRec->keyString );
  }
#ifdef DEBUG
  else
    if ( config->debug >= 4 )
      printf( "DEBUG - Record found [%s]\n", hashRec->keyString );
#endif

  /* can use this later to interrupt traversing the hash */
  if ( quit )
    return( TRUE );
  return( FALSE );
}

/****
 *
 * convert hash to hex
 *
 ****/

char *hash2hex(const char *hash, char *hashStr, int hLen ) {
  int i;
  char hByte[3];
  bzero( hByte, sizeof( hByte ) );
	hashStr[0] = 0;
	
  for( i = 0; i < hLen; i++ ) {
    snprintf( hByte, sizeof(hByte), "%02x", hash[i] & 0xff );
#ifdef HAVE_STRNCAT
    strncat( hashStr, hByte, hLen*2 );
#else
    strlcat( hashStr, hByte, hLen*2 );
#endif
  }

  return hashStr;
}
