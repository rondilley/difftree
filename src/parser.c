/*****
 *
 * Description: Line Parser Functions
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

#define LHS 0
#define RHS 1

#define MAX_FIELD_POS 1024
#define MAX_FIELD_LEN 1024

/****
 *
 * includes
 *
 ****/

#include "parser.h"

/****
 *
 * local variables
 *
 ****/

PRIVATE char *fields[MAX_FIELD_POS];

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
 * init parser
 *
 ****/

void initParser( void ) {
  /* make sure the field list of clean */
  XMEMSET( fields, 0, sizeof( char * ) * MAX_FIELD_POS );

  /* XXX it would be faster to init all mem here instead of on-demand */
}

/****
 *
 * de-init parser
 *
 ****/

void deInitParser( void ) {
  int i;

  for( i = 0; i < MAX_FIELD_POS; i++ )
    if ( fields[i] != NULL )
      XFREE( fields[i] );
}

/****
 *
 * parse that line
 *
 * pass a line to the function and the function will
 * return a printf style format string
 *
 ****/

int parseLine( char *line ) {
  int curFormPos = 0;
  int curLinePos = 0;
  int startOfField;
  int curFieldType = FIELD_TYPE_UNDEF;
  int runLen = 0;
  int i;
  char *posPtr;
  char *key = NULL;
  long tmpOffset;
  long offsetList[1024];
  int fieldPos = 0;
  int offsetPos = 0;
  long tmpLongNum = 0;
  int positionStatus = LHS;
  int inQuotes = FALSE;

  while( line[curLinePos] != '\0' ) {

    if ( runLen >= MAX_FIELD_LEN ) {
      fprintf( stderr, "ERR - Field is too long\n" );
      return( fieldPos-1 );
    } else if ( fieldPos >= MAX_FIELD_POS ) {
      fprintf( stderr, "ERR - Too many fields in line\n" );
      return( fieldPos-1 );
    } else if ( curFieldType EQ FIELD_TYPE_STRING ) {

      /****
       *
       * string
       *
       ****/

      if ( isalnum( line[curLinePos] ) ) {

	/****
	 *
	 * add alpha numberic char to string
	 *
	 ****/

	runLen++;
	curLinePos++;
      } else if ( ( line[curLinePos] EQ '.' ) |
		  ( (inQuotes) && (line[curLinePos] EQ ',') ) |
		  ( (inQuotes) && (line[curLinePos] EQ '\'') ) |
		  ( line[curLinePos] EQ '-' ) |
  		  ( line[curLinePos] EQ '+' ) |
		  ( line[curLinePos] EQ '%' ) |
		  ( line[curLinePos] EQ ':' ) |
		  ( line[curLinePos] EQ '/' ) |
		  ( line[curLinePos] EQ '#' ) |
		  ( (inQuotes) && (line[curLinePos] EQ ' ') ) |
		  ( line[curLinePos] EQ '~' ) |
		  ( line[curLinePos] EQ '@' ) |
		  ( line[curLinePos] EQ '&' ) |
		  ( line[curLinePos] EQ '$' ) |
		  ( line[curLinePos] EQ '(' ) |
		  ( line[curLinePos] EQ ')' ) |
		  ( (inQuotes) && (line[curLinePos] EQ '\'' ) ) |
		  ( line[curLinePos] EQ '\\' ) |
		  ( line[curLinePos] EQ '_' )
		  ) {

	/****
	 *
	 * add some printable characters to the string
	 *
	 ****/

	runLen++;
	curLinePos++;

      } else if ( line[curLinePos] EQ '\"' ) {

	/****
	 *
	 * deal with quoted fields, spaces and some printable characters
	 * will be added to the string
	 *
	 ****/

	/* check to see if it is the start or end */

	if ( inQuotes ) {

	  /* at the end */
	  curLinePos++;

	  if ( positionStatus EQ RHS ) {

	    /* extract string */

	    if ( fields[fieldPos] EQ NULL ) {
	      if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
		fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
		return( fieldPos-1 );
	      }
	    }
	    fields[fieldPos][runLen] = '\0';
	    XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	    if ( config->debug >= 6 )
	      printf( "DEBUG - Extracting string [%s]\n", fields[fieldPos] );
#endif

	    //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	    fieldPos++;

	    /* switch field state */
	    curFieldType = FIELD_TYPE_STATIC;
	    runLen = 1;
	    startOfField = curLinePos++;
	  }

	  inQuotes = FALSE;
	} else {

	  /* at the start */
	  inQuotes = TRUE;
	  runLen++;
	  curLinePos++;
	}

      } else if ( ( line[curLinePos] EQ ':' ) |
		  ( line[curLinePos] EQ '{' ) |
		  ( line[curLinePos] EQ '}' ) ) {

	/****
	 *
	 * if the : is on the lhs of an =, treat it as a delimeter, if not, add it to the string
	 *
	 ****/

	if ( ( positionStatus EQ RHS ) | ( inQuotes EQ TRUE ) ) {

	  /* just add it to the string */

	  runLen++;
	  curLinePos++;

	} else {

	  /* treat it as a delimeter */

	  if ( fields[fieldPos] EQ NULL ) {
	    if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	      fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	      return( fieldPos-1 );
	    }
	  }
	  fields[fieldPos][runLen] = '\0';
	  XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	  if ( config->debug >= 6 )
	    printf( "DEBUG - Extracted string [%s]\n", fields[fieldPos] );
#endif

	  fieldPos++;

	  /* switch field state */
	  curFieldType = FIELD_TYPE_STATIC;
	  runLen = 1;
	  startOfField = curLinePos++;
	}

      } else if ( line[curLinePos] EQ '=' ) {

	if ( inQuotes ) {

	  /* just add it to the string */

	  runLen++;
	  curLinePos++;

	} else {

	  /****
	  *
	  * treat the lhs as a key field and the rhs as a value field
	  *
	  ****/

	  /* extract string */

	  if ( fields[fieldPos] EQ NULL ) {
	    if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	      fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	      return( fieldPos-1 );
	    }
	  }
	  fields[fieldPos][runLen] = '\0';
	  XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	  if ( config->debug >= 6 )
	    printf( "DEBUG - Extracting string [%s]\n", fields[fieldPos] );
#endif

	  fieldPos++;

	  if ( positionStatus EQ LHS ) {
	    positionStatus = RHS;
	  } else {
	    /* got another =, something is odd */
	  }

	  /* switch field state */
	  curFieldType = FIELD_TYPE_STATIC;
	  runLen = 1;
	  startOfField = curLinePos++;
	}

      } else if ( ( line[curLinePos] EQ ' ' ) |
		  ( line[curLinePos] EQ '\t' )
		  ) {

	/****
	 *
	 * space characters should added to the string if on the lhs or considered a delimiter on the rhs
	 *
	 ****/

	if ( ( positionStatus EQ LHS ) | ( inQuotes EQ TRUE ) ) {

	  /* just add it to the string */

	  runLen++;
	  curLinePos++;
	  
	} else {

	  /* treat it as a delimiter */

	  if ( curLinePos > 0 ) {
	    if ( ( line[curLinePos-1] EQ ' ' ) | ( line[curLinePos-1] EQ '\t' ) ) {
	      /* last char was a blank */
	      runLen--;
	    }
	  }

	  /*
	   * extract string
	   */

	  if ( fields[fieldPos] EQ NULL ) {
	    if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	      fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	      return( fieldPos-1 );
	    }
	  }
	  fields[fieldPos][runLen] = '\0';
	  XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	  if ( config->debug >= 6 )
	    printf( "DEBUG - Extracting string [%s]\n", fields[fieldPos] );
#endif

	  //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	  fieldPos++;

	  /* switch field state */
	  positionStatus = LHS;
	  curFieldType = FIELD_TYPE_STATIC;
	  runLen = 1;
	  startOfField = curLinePos++;
	}
   
      } else if ( ispunct( line[curLinePos] ) ) {

	/****
	 *
	 * punctuation is a delimeter
	 *
	 ****/

	if ( curLinePos > 0 ) {
	  if ( ( line[curLinePos-1] EQ ' ' ) | ( line[curLinePos-1] EQ '\t' ) ) {
	    /* last char was a blank */
	    runLen--;
	  }
	}

	/* extract string */

	if ( fields[fieldPos] EQ NULL ) {
	  if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	    return( fieldPos-1 );
	  }
	}
	fields[fieldPos][runLen] = '\0';
	XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	if ( config->debug >= 6 )
	  printf( "DEBUG - Extracting string [%s]\n", fields[fieldPos] );
#endif

	if ( positionStatus EQ LHS ) {

	  positionStatus = RHS;

	} else {

	  //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	  positionStatus = LHS;
	}

	fieldPos++;

	/* switch field state */
	curFieldType = FIELD_TYPE_STATIC;
	runLen = 1;
	startOfField = curLinePos++;

      } else if ( ( iscntrl( line[curLinePos] ) ) | !( isprint( line[curLinePos] ) ) ) {

	/****
	 *
	 * ignore control and non-printable characters
	 *
	 ****/

	/* extract string */

	if ( fields[fieldPos] EQ NULL ) {
	  if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	    return( fieldPos-1 );
	  }
	}
	fields[fieldPos][runLen] = '\0';
	XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	if ( config->debug >= 6 )
	  printf( "DEBUG - Extracting string [%s]\n", fields[fieldPos] );
#endif

	if ( positionStatus EQ LHS ) {

	  positionStatus = RHS;

	} else {

	  //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	  positionStatus = LHS;
	}

	fieldPos++;

	/* XXX this will mess up hashing */
	curFieldType = FIELD_TYPE_UNDEF;
	curLinePos++;
      }

    } else if ( curFieldType EQ FIELD_TYPE_CHAR ) {

      /****
       *
       * char field
       *
       ****/

      if ( isalnum( line[curLinePos] ) |
	   ( line[curLinePos] EQ '/' ) |
	   ( line[curLinePos] EQ '@' ) |
	   ( (inQuotes) && (line[curLinePos] EQ ' ') ) |
	   ( line[curLinePos] EQ '\\' ) |
	   ( line[curLinePos] EQ ':' )
	   ) {
	/* convery char to string */
	curFieldType = FIELD_TYPE_STRING;
	runLen++;
	curLinePos++;
#ifdef HAVE_ISBLANK
      } else if ( ( ispunct( line[curLinePos] ) ) | ( isblank( line[curLinePos] ) ) ) {
#else
      } else if ( ( ispunct( line[curLinePos] ) ) | ( line[curLinePos] EQ ' ' ) | ( line[curLinePos] EQ '\t' ) ) {
#endif

	/* extract char */

	if ( fields[fieldPos] EQ NULL ) {
	  if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	    return( fieldPos-1 );
	  }
	}
	fields[fieldPos][runLen] = '\0';
	XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	if ( config->debug >= 6 )
	  printf( "DEBUG - Extracting character [%s]\n", fields[fieldPos] );
#endif

	if ( positionStatus EQ RHS ) {

	  //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	  positionStatus = LHS;
	}

	fieldPos++;

	/* switch field state */
	curFieldType = FIELD_TYPE_STATIC;
	runLen = 1;
	startOfField = curLinePos++;
      } else if ( ( iscntrl( line[curLinePos] ) ) | !( isprint( line[curLinePos] ) ) ) {
	/* not a valid log character, ignore it for now */

	curFieldType = FIELD_TYPE_UNDEF;
	curLinePos++;
      }
    } else if ( curFieldType EQ FIELD_TYPE_NUM_INT ) {

      /****
       *
       * number field
       *
       ****/

      /* XXX need to add code to handle numbers beginning with 0 */
      if ( isdigit( line[curLinePos] ) ) {
	runLen++;
	curLinePos++;
      } else if ( isalpha( line[curLinePos] ) |
		  ( line[curLinePos] EQ '/' ) |
		  ( line[curLinePos] EQ '@' ) |
		  ( ( inQuotes ) && ( line[curLinePos] EQ ' ') ) |
		  ( line[curLinePos] EQ ':' ) |
		  ( line[curLinePos] EQ '\\' )
		  ) {
	/* convert field to string */
	curFieldType = FIELD_TYPE_STRING;
	runLen++;
	curLinePos++;
      } else if ( line[curLinePos] EQ '.' ) {
	/* convert field to float */
	curFieldType = FIELD_TYPE_NUM_FLOAT;
	runLen++;
	curLinePos++;
#ifdef HAVE_ISBLANK
      } else if ( ( ispunct( line[curLinePos] ) ) | ( isblank( line[curLinePos] ) ) ) {
#else
      } else if ( ( ispunct( line[curLinePos] ) ) | ( line[curLinePos] EQ ' ' ) | ( line[curLinePos] EQ '\t' ) ) {
#endif

	/* extract number */

	if ( fields[fieldPos] EQ NULL ) {
	  if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate memory for number\n" );
	    return( fieldPos-1 );
	  }
	}
	fields[fieldPos][runLen] = '\0';
	XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	if ( config->debug >= 6 )
	  printf( "DEBUG - Extracting number [%s]\n", fields[fieldPos] );
#endif

	if ( positionStatus EQ RHS ) {

	  //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	  positionStatus = LHS;
	}

	fieldPos++;

	/* switch field state */
	curFieldType = FIELD_TYPE_STATIC;
	runLen = 1;
	startOfField = curLinePos++;
      } else if ( ( iscntrl( line[curLinePos] ) ) | !( isprint( line[curLinePos] ) ) ) {
	  
	/* extract string */

	if ( fields[fieldPos] EQ NULL ) {
	  if( ( fields[fieldPos] = (char *)XMALLOC( MAX_FIELD_LEN ) ) EQ NULL ) {
	    fprintf( stderr, "ERR - Unable to allocate memory for string\n" );
	    return( fieldPos-1 );
	  }
	}
	fields[fieldPos][runLen] = '\0';
	XMEMCPY( fields[fieldPos], line + startOfField, runLen );

#ifdef DEBUG
	if ( config->debug >= 6 )
	  printf( "DEBUG - Extracting number [%s]\n", fields[fieldPos] );
#endif

	if ( positionStatus EQ RHS ) {

	  //printf( "%s=%s\n", fields[fieldPos-1], fields[fieldPos] );

	  positionStatus = LHS;
	}

	fieldPos++;

	/* switch field state */
	curFieldType = FIELD_TYPE_UNDEF;
	curLinePos++;
      }

    } else if ( curFieldType EQ FIELD_TYPE_NUM_FLOAT ) {

      /****
       *
       * float
       *
       ****/

      if ( isdigit( line[curLinePos] ) ) {
	runLen++;
	curLinePos++;
      } else if ( isalpha( line[curLinePos] ) |
		  ( line[curLinePos] EQ '/' ) |
		  ( line[curLinePos] EQ '@' ) |
		  ( line[curLinePos] EQ '\\' )
		  ) {

	/* convert float to string */
	curFieldType = FIELD_TYPE_STRING;
	runLen++;
	curLinePos++;
      } else if ( line[curLinePos] EQ '.' ) {
	/* convert float to string */
	curFieldType = FIELD_TYPE_STRING;
	runLen++;
	curLinePos++;
#ifdef HAVE_ISBLANK
      } else if ( ( ispunct( line[curLinePos] ) ) | ( isblank( line[curLinePos] ) ) ) {
#else
      } else if ( ( ispunct( line[curLinePos] ) ) | ( line[curLinePos] EQ ' ' ) | ( line[curLinePos] EQ '\t' ) ) {
#endif
	/* insert the digit into the format */
	/* switch field state */
	curFieldType = FIELD_TYPE_STATIC;
	runLen = 1;
	startOfField = curLinePos++;
      } else if ( ( iscntrl( line[curLinePos] ) ) | !( isprint( line[curLinePos] ) ) ) {
	/* not a valid log character, ignore it for now */
	curFieldType = FIELD_TYPE_UNDEF;
	curLinePos++;
      }
    } else if ( curFieldType EQ FIELD_TYPE_NUM_HEX ) {

      /****
       *
       * hex field
       *
       ****/

    } else if ( curFieldType EQ FIELD_TYPE_STATIC ) {

      /****
       *
       * printable, but non-alphanumeric
       *
       ****/

      /* this is a placeholder for figuring out how to handle multiple spaces */

      curFieldType = FIELD_TYPE_UNDEF;

    } else {

      /****
       *
       * begining of new field
       *
       ****/

      if ( isalpha( line[curLinePos] ) |
	   ( line[curLinePos] EQ '/' ) |
	   ( line[curLinePos] EQ '@' ) |
	   ( line[curLinePos] EQ '\\' )
	   ) {
	curFieldType = FIELD_TYPE_CHAR;
	runLen = 1;
	startOfField = curLinePos++;
      } else if ( isdigit( line[curLinePos] ) ) {
	curFieldType = FIELD_TYPE_NUM_INT;
	runLen = 1;
	startOfField = curLinePos++;
      } else if ( line[curLinePos] EQ '\"' ) {
	if ( inQuotes ) {
	  runLen++;
	  curLinePos++;
	  inQuotes = FALSE;
	} else {
	  curFieldType = FIELD_TYPE_STRING;
	  inQuotes = TRUE;
	  runLen = 0;
	  startOfField = ++curLinePos;
	}
#ifdef HAVE_ISBLANK
      } else if ( ( ispunct( line[curLinePos] ) ) | ( isblank( line[curLinePos] ) ) ) {
#else
      } else if ( ( ispunct( line[curLinePos] ) ) | ( line[curLinePos] EQ ' ' ) | ( line[curLinePos] EQ '\t' ) ) {
#endif
	/* printable but not alpha+num */
	curFieldType = FIELD_TYPE_STATIC;
	runLen = 1;
	startOfField = curLinePos++;
      } else if ( ( iscntrl( line[curLinePos] ) ) | !( isprint( line[curLinePos] ) ) ) {
	/* not a valid log character, ignore it for now */
	curLinePos++;
      }
    }
  }

  return( fieldPos );
}

/****
 *
 * return parsed field
 *
 ****/

int getParsedField( char *oBuf, int oBufLen, const unsigned int fieldNum ) {
  if ( ( fieldNum >= MAX_FIELD_POS ) || ( fields[fieldNum] EQ NULL ) ) {
    fprintf( stderr, "ERR - Requested field does not exist [%d]\n", fieldNum );
	oBuf[0] = 0;
	return( FAILED );
  }
  XSTRNCPY( oBuf, fields[fieldNum], oBufLen );
  return( TRUE );
}
