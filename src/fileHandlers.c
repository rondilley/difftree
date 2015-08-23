/*****
 *
 * Copyright (c) 2014, Ron Dilley
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
 *
 ****/

#include "fileHandlers.h"

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

/*****
 *
 * write directory tree record to file
 *
 *****/

int writeRecord2File( const struct hashRec_s *hashRec ) {
  metaData_t *tmpMD;
  struct stat *tmpSb;
  char tmpBuf[1024];
  int tflag;
  struct tm *tmPtr;

  tmpMD = (metaData_t *)hashRec->data;
  tmpSb = (struct stat *)&tmpMD->sb;
  tflag = tmpSb->st_mode & S_IFMT;

  if ( strlen( hashRec->keyString ) <= 0 )
    return FAILED;

  fprintf( out, "KEY=\"%s\"|", hashRec->keyString );
  fprintf( out, "TYPE=%s|",
	   (tflag == S_IFDIR) ?   "d"   :
	   (tflag == S_IFBLK) ?  "blk"  :
	   (tflag == S_IFREG) ?   "f" :
	   (tflag == S_IFCHR) ?  "chr"  :
#ifndef MINGW
	   (tflag == S_IFSOCK) ? "sok" :
	   (tflag == S_IFLNK) ?  "sl" :
#endif
	   (tflag == S_IFIFO) ? "fifo" : "???" );
  fprintf( out, "SIZE=%ld|", (long int)tmpSb->st_size );
  fprintf( out, "UID=%d|", tmpSb->st_uid );
  fprintf( out, "GID=%d|", tmpSb->st_gid );
  fprintf( out, "PERM=%x%x%x%x|",
#ifdef MINGW
		tmpSb->st_mode & S_IRWXU >> 9,
		0,0,0
#else
	   tmpSb->st_mode >> 9 & S_IRWXO,
	   tmpSb->st_mode >> 6 & S_IRWXO,
	   tmpSb->st_mode >> 3 & S_IRWXO,
	   tmpSb->st_mode & S_IRWXO
#endif	   
	   );
  fprintf( out, "MTIME=%ld|", tmpSb->st_mtime );
  fprintf( out, "ATIME=%ld|", tmpSb->st_atime );
  fprintf( out, "CTIME=%ld|", tmpSb->st_ctime );
  fprintf( out, "INODE=%ld|", (long int)tmpSb->st_ino );
  fprintf( out, "HLINKS=%d|", (int)tmpSb->st_nlink );
#ifndef MINGW
  fprintf( out, "BLOCKS=%ld|", (long int)tmpSb->st_blocks );
#endif
  if ( config->hash && ( tflag EQ S_IFREG ) )
    fprintf( out, "MD5=\"%s\"|", hash2hex( tmpMD->digest, tmpBuf, MD5_HASH_LEN ) );
  fprintf( out, "\n" );

  /* can use this later to interrupt traversing the hash */
  return FALSE;
}

/*****
 *
 * write contents of directory hash to file
 *
 *****/

int writeDirHash2File( const struct hash_s *dirHash, const char *base, const char *outFile ) {
  struct stat sb;
  int tflag;
  struct tm *tmPtr;

  if ( lstat( outFile, &sb ) EQ 0 ) {
    /* file exists, make sure it is a file */
    tflag = sb.st_mode & S_IFMT;
    if ( tflag != S_IFREG ) {
      fprintf( stderr, "ERR - Unable to overwrite non-file [%s]\n", outFile);
      return( FAILED );
    }
  }
	
  /* open the file for writing */
  if ( ( out = fopen( outFile, "w" ) ) EQ (FILE *)0 ) {
    fprintf( stderr, "ERR - Unable to open file [%s] to write\n", outFile );
    return FAILED;
  }

  /* write the header info */
  fprintf( out, "%%DIFFTREE-%s\n", VERSION );
  fprintf( out, "VER=%s\n", FORMAT_VERSION );
  fprintf( out, "BASE=%s\n", base );
  fprintf( out, "MODE=%s\n", (config->hash) ? "HASH" : ( config->quick ) ? "QUICK" : "NORMAL" );
  tmPtr = localtime( &config->current_time );
  fprintf( out, "START=\"%04d/%02d/%02d@%02d:%02d:%02d\"\n",
	   tmPtr->tm_year+1900, tmPtr->tm_mon+1, tmPtr->tm_mday,
	   tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec );

  /* dump all directory tree records to a file */
  traverseHash( dirHash, writeRecord2File );

  fprintf( out, "RECORDS=%ld\n", (long int)dirHash->totalRecords );

  /* done */
  fclose( out );

  return TRUE;
}

/****
 *
 * load file and process each record
 *
 ****/

int loadFile( const char *fName ) {
  FILE *inFile;
  char inBuf[8192];
  int i, ret, fileVersion;

  if ( ( inFile = fopen( fName, "r" ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to open file [%s] for reading\n", fName );
    return( FAILED );
  }

  /****
   *
   * read the headers
   *
   ****/
  /* get to parsing */
  initParser();

  /* DIFFTREE preamble */
  if ( fscanf( inFile, "%%DIFFTREE-%s\n", inBuf ) != 1 ) {
    fprintf( stderr, "ERR - File does not appear to be a DIFFTREE file\n" );
    fclose( inFile );
    return( FAILED );
  }

  if ( fscanf( inFile, "VER=%d\n", &fileVersion ) EQ 1 ) {
#ifdef DEBUG
    if ( config->debug >= 2 ) {
      printf( "DEBUG - DT Version: %s\n", inBuf );
      printf( "DEBUG - File Version: %d\n", fileVersion );
    }
#endif
    if ( fileVersion EQ 1 ) {
      loadV1File( inFile );
    } else if ( fileVersion EQ 2 ) {
      loadV2File( inFile );
    } else {
      fprintf( stderr, "ERR - File version format [%s] is not compatible with this version of difftree\n", inBuf );
      deInitParser();
      fclose( inFile );
      return( FAILED );
    }
  } else {
    fprintf( stderr, "ERR - File version missing, file may be corrupt\n" );
    deInitParser();
    fclose( inFile );
    return( FAILED );
  }
  
#ifdef DEBUG
  if ( config->debug >= 7 )
    printf( "DEBUG - Read [%s]\n", inBuf );
#endif

  deInitParser();

  fclose( inFile );

  return( FTW_CONTINUE );
}

/****
 * 
 * load v1 file
 * 
 ****/

int loadV1File( FILE *inFile ) {
  char inBuf[8192];
  char keyString[MAXPATHLEN+1];
  char startDate[128];
  char hByte[3];
  char *strPtr;
  unsigned char digest[16]; /* MD5 */
  struct stat sb;
  size_t count = 0, rCount;
  int i, dPos, lPos, ret, tflag;
  int tMon = 0, tDay = 0, tYear = 0, tHour = 0, tMin = 0, tSec = 0, tType = 0, tPerm = 0;
  struct tm tmb;
  struct tm *tmPtr;

  XMEMSET( hByte, 0, sizeof( hByte ) );
  XMEMSET( inBuf, 0, sizeof( inBuf ) );
  XMEMSET( startDate, 0, sizeof( startDate ) );
  
  /****
   *
   * read the headers
   *
   ****/
  
  if ( fscanf( inFile, "BASE=%s\n", inBuf ) EQ 1 ) {
    if ( strlen( inBuf ) < MAXPATHLEN ) {
      if ( ( compDir = XMALLOC( MAXPATHLEN + 1 ) ) != NULL ) {
        XSTRNCPY( compDir, inBuf, MAXPATHLEN );
#ifdef DEBUG
	if ( config->debug >= 3 )
	  printf( "DEBUG - Base: %s\n", compDir );
#endif
      }
    }

  } else {
    fprintf( stderr, "ERR - Base Dir missing, file may be corrupt\n" );
    deInitParser();
    fclose( inFile );
    return( FAILED );
  }

  /* Scan mode */
  if ( fgets( inBuf, sizeof( inBuf ), inFile ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to read line from input file\n" );
    deInitParser();
    fclose( inFile );
    return( FAILED );
  }
  if ( ( ret = parseLine( inBuf ) ) != 2 ) {
    printf( "ERR - Malformed record line - [%d] fields\n", ret );
  } else {
    for( i = 0; i < ret; i=i+2 ) {
      /* load the fields */
      getParsedField( inBuf, sizeof( inBuf ), i );
      if ( strlen( inBuf ) <= 0 ) {
	/* LHS (key) is zero length */
	fprintf( stderr, "ERR - Key is corrupt\n" );
      } else if ( strcmp( inBuf, "MODE" ) EQ 0 ) {
	getParsedField( inBuf, sizeof( inBuf ), i+1 );
	if ( strncmp( inBuf, "QUICK", sizeof( inBuf ) ) EQ 0 ) {
	  if ( ! config->quick ) {
	    fprintf( stderr, "ERR - The file's mode is not compatible with the current mode [%s->%s]\n",
		     inBuf, (config->hash) ? "HASH" : "NORMAL" );
	    deInitParser();
	    fclose( inFile );
	    return( FAILED );
	  }
	} else if ( strncmp( inBuf, "NORMAL", sizeof( inBuf ) ) EQ 0 ) {
	  if ( config->hash ) {
	    fprintf( stderr, "ERR - The file's mode is not compatible with the current mode [%s->HASH]\n", inBuf );
	    deInitParser();
	    fclose( inFile );
	    return( FAILED );
	  }
	} else if ( strncmp( inBuf, "HASH", sizeof( inBuf ) ) EQ 0 ) {
	  /* compatible with other modes */
	} else {
	  /* unknown hash mode */
	  fprintf( stderr, "ERR - Unknown mode, file may be corrupt\n" );
	  deInitParser();
	  fclose( inFile );
	  return( FAILED );
	}
#ifdef DEBUG
	if ( config->debug >= 2 )
	  printf( "DEBUG - Mode: %s\n", inBuf );
#endif
      } else {
	fprintf( stderr, "ERR - Mode is corrupted\n" );
	deInitParser();
	fclose( inFile );
	return( FAILED );
      }
    }
  }

  /* Start time */
  if ( fgets( inBuf, sizeof( inBuf ), inFile ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to read line from input file\n" );
    deInitParser();
    fclose( inFile );
    return( FAILED );
  }
  if ( ( ret = parseLine( inBuf ) ) != 2 ) {
    printf( "ERR - Malformed record line - [%d] fields\n", ret );
  } else {
    for( i = 0; i < ret; i=i+2 ) {
      /* load the fields */
      getParsedField( inBuf, sizeof( inBuf ), i );
      if ( strlen( inBuf ) <= 0 ) {
	/* LHS (key) is zero length */
	fprintf( stderr, "ERR - Key is corrupt\n" );
      } else if ( strcmp( inBuf, "START" ) EQ 0 ) {
	getParsedField( inBuf, sizeof( inBuf ), i+1 );
	if ( strlen( inBuf ) > 0 ) {
	  XSTRNCPY( startDate, inBuf, sizeof( startDate ) );
	} else {
	  fprintf( stderr, "ERR - Start time is corrupted\n" );
	  deInitParser();
	  fclose( inFile );
	  return( FAILED );
	}
#ifdef DEBUG
	if ( config->debug >= 2 )
	  printf( "DEBUG - Start: %s\n", startDate );
#endif
      } else {
	fprintf( stderr, "ERR - Start is corrupted\n" );
	deInitParser();
	fclose( inFile );
	return( FAILED );
      }
    }
  }

  /****
   *
   * read all of the file records
   *
   ****/

  while( fgets( inBuf, sizeof( inBuf ), inFile ) != NULL ) {
    if ( quit ) {
      deInitParser();
      fclose( inFile );
      return( FTW_STOP );
    }

    /**** get to work on that line ****/
    if ( ( ret = parseLine( inBuf ) ) EQ 2 ) {
      for ( i = 0; i < ret; i+=2 ) {
	getParsedField( inBuf, sizeof( inBuf ), i );
	if ( strlen( inBuf ) <= 0 ) {
	  /* LHS (key) is zero length */
	  fprintf( stderr, "ERR - Key is corrupt\n" );
	} else if ( strcmp( inBuf, "RECORDS" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  if ( strlen( inBuf ) > 0 ) {
	    if ( ( rCount = atol( inBuf ) ) != compDirHash->totalRecords )
	      fprintf( stderr, "ERR - [%ld] records in file, [%ld] records in hash\n", (long int)rCount, (long int)compDirHash->totalRecords );
#ifdef DEBUG
	    if ( config->debug >= 2 )
	    printf( "DEBUG - Records: %ld\n", rCount );
#endif
	  } else {
	    fprintf( stderr, "ERR - Record count is corrupted\n" );
	    deInitParser();
	    fclose( inFile );
	    return( FAILED );
	  }
	} else {
	  fprintf( stderr, "ERR - Record is corrupted\n" );
	  deInitParser();
	  fclose( inFile );
	  return( FAILED );
	}
      }
    } else if ( ( ret != 24 ) && ( ret != 26 ) ) {
      printf( "ERR - Malformed record line - [%d] fields\n", ret );
    } else {
      count++;
      for( i = 0; i < ret; i=i+2 ) {
	/* load the fields */
	getParsedField( inBuf, sizeof( inBuf ), i );
	if ( strlen( inBuf ) <= 0 ) {
	  /* LHS (key) is zero length */
	  fprintf( stderr, "ERR - Key is corrupt\n" );
	} else if ( strcmp( inBuf, "KEY" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  if ( strlen( inBuf ) > 0 ) {
	    snprintf( keyString, sizeof( keyString ), "%s%s", compDir, inBuf );
#ifdef DEBUG
	    if ( config->debug >= 5 )
	      printf( "DEBUG - KEY=%s\n", keyString );
#endif
	  }
	} else if ( strcmp( inBuf, "TYPE" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  if ( inBuf[0] EQ 'f' )
	    if ( inBuf[1] EQ 'i' )	  
	      tType = S_IFIFO;
	    else
	      tType = S_IFREG;
	  else if ( inBuf[0] EQ 'd' )
	    tType =  S_IFDIR;
	  else if ( inBuf[0] EQ 'b' )
	    tType = S_IFBLK;
	  else if ( inBuf[0] EQ 'c' )
	    tType =  S_IFCHR;
	  else if ( inBuf[0] EQ 's' ) {
#ifndef MINGW
	    if ( inBuf[1] EQ 'l' )
	      tType = S_IFLNK;
	    else
	      tType = S_IFSOCK;
#endif
	  }
	} else if ( strcmp( inBuf, "SIZE" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_size = (off_t)atol( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - SIZE=%ld\n", (long int)sb.st_size );
#endif
	} else if ( strcmp( inBuf, "UID" ) EQ 0 ) {
 	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_uid = (uid_t)atoi( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - UID=%d\n", sb.st_uid );
#endif
	} else if ( strcmp( inBuf, "GID" ) EQ 0 ) {
 	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_gid = (gid_t)atoi( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - GID=%d\n", sb.st_gid );
#endif
	} else if ( strcmp( inBuf, "PERM" ) EQ 0 ) {
 	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  tPerm = strtoul( inBuf, NULL, 8 );
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBIG - PERM=%x%x%x%x\n",
		    tPerm >> 9 & S_IRWXO,
		    tPerm >> 6 & S_IRWXO,
		    tPerm >> 3 & S_IRWXO,
		    tPerm & S_IRWXO );
#endif
	} else if ( strcmp( inBuf, "MTIME" ) EQ 0 ) {
 	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_mtime = atol( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 ) {
	    tmPtr = localtime( &sb.st_mtime );
	    printf( "DEBUG - MTIME=%04d/%02d/%02d@%02d:%02d:%02d%s\n",
		    tmPtr->tm_year+1900, tmPtr->tm_mon+1, tmPtr->tm_mday,
		    tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec, (tmPtr->tm_isdst) ? "DST" : "---" );
	  }
#endif
	} else if ( strcmp( inBuf, "ATIME" ) EQ 0 ) {
 	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_atime = atol( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 ) {
	    tmPtr = localtime( &sb.st_atime );
	    printf( "DEBUG - ATIME=%04d/%02d/%02d@%02d:%02d:%02d%s\n",
		    tmPtr->tm_year+1900, tmPtr->tm_mon+1, tmPtr->tm_mday,
		    tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec, (tmPtr->tm_isdst) ? "DST" : "---" );
	  }
#endif
	} else if ( strcmp( inBuf, "CTIME" ) EQ 0 ) {
 	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_ctime = atol( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 ) {
	    tmPtr = localtime( &sb.st_ctime );
	    printf( "DEBUG - CTIME=%04d/%02d/%02d@%02d:%02d:%02d%s\n",
		    tmPtr->tm_year+1900, tmPtr->tm_mon+1, tmPtr->tm_mday,
		    tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec, (tmPtr->tm_isdst) ? "DST" : "---" );
	  }
#endif
	} else if ( strcmp( inBuf, "INODE" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  sb.st_ino = (ino_t)atol( inBuf );
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - INODE=%ld\n", (long int)sb.st_ino );
#endif
	} else if ( strcmp( inBuf, "HLINKS" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  #ifndef MINGW
	  sb.st_nlink = (nlink_t)atoi( inBuf );
	  #endif
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - HLINKS=%d\n", (int)sb.st_nlink );
#endif
	} else if ( strcmp( inBuf, "BLOCKS" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  #ifndef MINGW
#ifdef OPENBSD
	  sb.st_blocks = (int64_t)atol( inBuf );
#else
	  sb.st_blocks = (blkcnt_t)atol( inBuf );
#endif
#endif

#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - BLOCKS=%ld\n", (long int)sb.st_blocks );
#endif
	} else if ( strcmp( inBuf, "MD5" ) EQ 0 ) {
	  getParsedField( inBuf, sizeof( inBuf ), i+1 );
	  /* convert hash string to binary */
	  for( dPos = 0, lPos = 0; dPos < MD5_HASH_LEN; dPos++, lPos+=2 ) {
	    hByte[0] = inBuf[lPos];
	    hByte[1] = inBuf[lPos+1];
	    digest[dPos] = strtoul( hByte, NULL, 16 );
	  }
#ifdef DEBUG
	  if ( config->debug >= 5 )
	    printf( "DEBUG - MD5=%s\n", hash2hex( digest, inBuf, MD5_HASH_LEN ) );
#endif
	} else {
	  fprintf( stderr, "ERR - Unknown field key [%s]\n", inBuf );
	}
      }

      /* merge the mode bits */
      sb.st_mode = tType | tPerm;

      /* load record into hash */
      processRecord( keyString, &sb, FILE_RECORD, digest );
    }
    /********* done with line *********/

#ifdef DEBUG
    if ( config->debug >= 7 )
      printf( "DEBUG - Read [%s]\n", inBuf );
#endif

  }

  printf( "Read [%ld] and loaded [%ld] lines from file about [%s] dated [%s]\n", (long int)count, (long int)compDirHash->totalRecords, compDir, startDate );

  return( FTW_CONTINUE );
}

/****
 * 
 * load v2 file
 * 
 ****/

int loadV2File( FILE *inFile ) {
  char inBuf[8192];
  int inNum;
  int i, ret, fileVersion;

  /* DIFFTREE preamble */
  if ( fscanf( inFile, "VER=%d\n", &inNum ) != 1 ) {
    fprintf( stderr, "ERR - File does not appear to be a DIFFTREE file\n" );
    fclose( inFile );
    return( FAILED );
  }

  /* get to parsing */
  initParser();
  
  /* File format version */
  if ( fgets( inBuf, sizeof( inBuf ), inFile ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to read line from input file\n" );
    deInitParser();
    fclose( inFile );
    return( FAILED );
  }
  if ( ( ret = parseLine( inBuf ) ) != 2 ) {
    printf( "ERR - Malformed record line - [%d] fields\n", ret );
  }
  
  return (FTW_CONTINUE);
}