/*****
 *
 * Description: Line Parser Function Headers
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

#ifndef PARSER_DOT_H
#define PARSER_DOT_H

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
#include "util.h"
#include "mem.h"

/****
 *
 * defines
 *
 ****/

#define FIELD_TYPE_UNDEF 0
#define FIELD_TYPE_STRING 1
#define FIELD_TYPE_CHAR 2
#define FIELD_TYPE_NUM_INT 3
#define FIELD_TYPE_NUM_FLOAT 4
#define FIELD_TYPE_NUM_HEX 5
#define FIELD_TYPE_STATIC 6

#define PARSER_MIN_FIELD_LEN 0

/****
 *
 * typdefs & structs
 *
 ****/

/****
 *
 * function prototypes
 *
 ****/

void initParser( void );
void deInitParser( void );
int parseLine( char *line );
int getParsedField( char *oBuf, int oBufLen, const unsigned int fieldNum );

#endif /* end of PARSER_DOT_H */

