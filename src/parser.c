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

void initParser(void)
{
	/* make sure the field list of clean */
	XMEMSET(fields, 0, sizeof(char *) * MAX_FIELD_POS);

	/* XXX it would be faster to init all mem here instead of on-demand */
}

/****
 *
 * de-init parser
 *
 ****/

void deInitParser(void)
{
	int i;

	for (i = 0; i < MAX_FIELD_POS; i++)
		if (fields[i] != NULL)
			XFREE(fields[i]);
}

/****
 *
 * parse that line
 *
 * pass a line to the function and the function will
 * return a printf style format string
 *
 ****/

int parseLine(char *line)
{
	int curFormPos = 0;
	int curLinePos = 0;
	int startOfField = 0;
	int curFieldType = FIELD_TYPE_UNDEF;
	int runLen = 0;
	int i;
	char *posPtr;
	char *key = NULL;
	char *endPtr;
	long tmpOffset;
	long offsetList[1024];
	int fieldPos = 0;
	int offsetPos = 0;
	long tmpLongNum = 0;
	int positionStatus = LHS;
	int inQuotes = FALSE;
	int lineLen = strlen(line);

	while (curLinePos <= lineLen)
	{
#ifdef DEBUG
		if (config->debug >= 3)
			printf("DEBUG - [%c] runLen: %d curLinePos: %d startOfField: %d currentFieldType: %d position: %d [%s]\n",
						 line[curLinePos],
						 runLen,
						 curLinePos,
						 startOfField,
						 curFieldType,
						 positionStatus,
						 line);
#endif

		if (runLen >= MAX_FIELD_LEN)
		{
			fprintf(stderr, "ERR - Field is too long\n");
			return (fieldPos - 1);
		}

		if (fieldPos >= MAX_FIELD_POS)
		{
			fprintf(stderr, "ERR - Too many fields in line\n");
			return (fieldPos - 1);
		}

		/* XXX this can be optimized, only happens a few times on each line */
		if (curFieldType EQ FIELD_TYPE_UNDEF)
		{
			if (line[curLinePos] EQ '\"')
			{
				/* at the start */
				if ((endPtr = strstr(line + curLinePos, "\"|")) != NULL)
				{
					/* found terminating double-quote, fast forward */
					startOfField = ++curLinePos;
					runLen = (endPtr - (line + curLinePos));
					//printf("DEBUG - runLen: %d\n", runLen);
					/* extract string */
					if (fields[fieldPos] EQ NULL)
					{
						if ((fields[fieldPos] = (char *)XMALLOC(MAX_FIELD_LEN)) EQ NULL)
						{
							fprintf(stderr, "ERR - Unable to allocate memory for string\n");
							return (fieldPos - 1);
						}
					}
					fields[fieldPos][runLen] = '\0';
					XMEMCPY(fields[fieldPos], line + startOfField, runLen);

#ifdef DEBUG
					if (config->debug >= 6)
						printf("DEBUG - Extracting string [%s]\n", fields[fieldPos]);
#endif

					fieldPos++;
					positionStatus = LHS;

					/* switch field state */
					curFieldType = FIELD_TYPE_UNDEF;
					curLinePos += runLen + 2;
					runLen = 0;
					inQuotes = FALSE;
				}
				else
				{
					curFieldType = FIELD_TYPE_STRING;
					inQuotes = TRUE;
					runLen = 0;
					startOfField = ++curLinePos;
				}
			}
			//else if ((ispunct(line[curLinePos])) | (line[curLinePos] EQ ' ') | (line[curLinePos] EQ '\t'))
			//{
				/* ignore though there is something wrong */
			//	curLinePos++;
			//}
			//else if ((iscntrl(line[curLinePos])) | !(isprint(line[curLinePos])))
			//{
				/* not a valid log character, ignore it for now */
			//	curLinePos++;
			//}
			else
			{
				curFieldType = FIELD_TYPE_STRING;
				runLen = 1;
				startOfField = curLinePos++;
			}
			
		}
		else if (positionStatus EQ RHS)
		{
			/* RHS */
			if ((line[curLinePos] EQ '\"') && inQuotes)
			{
				/* at the end */
				curLinePos++;

				/* extract string */
				if (fields[fieldPos] EQ NULL)
				{
					if ((fields[fieldPos] = (char *)XMALLOC(MAX_FIELD_LEN)) EQ NULL)
					{
						fprintf(stderr, "ERR - Unable to allocate memory for string\n");
						return (fieldPos - 1);
					}
				}
				fields[fieldPos][runLen] = '\0';
				XMEMCPY(fields[fieldPos], line + startOfField, runLen);

#ifdef DEBUG
				if (config->debug >= 6)
					printf("DEBUG - Extracting string [%s]\n", fields[fieldPos]);
#endif

				fieldPos++;
				positionStatus = LHS;

				/* switch field state */
				curFieldType = FIELD_TYPE_UNDEF;
				runLen = 1;
				startOfField = curLinePos++;
				inQuotes = FALSE;
			}
			else if (line[curLinePos] EQ '\"')
			{
				/* at the start */
				if ((endPtr = strstr(line + curLinePos, "\"|")) != NULL)
				{
					/* found terminating double-quote, fast forward */
					curLinePos += (endPtr - (line + curLinePos));
					runLen = (endPtr - (line + curLinePos));

					/* extract string */
					if (fields[fieldPos] EQ NULL)
					{
						if ((fields[fieldPos] = (char *)XMALLOC(MAX_FIELD_LEN)) EQ NULL)
						{
							fprintf(stderr, "ERR - Unable to allocate memory for string\n");
							return (fieldPos - 1);
						}
					}
					fields[fieldPos][runLen] = '\0';
					XMEMCPY(fields[fieldPos], line + startOfField, runLen);

#ifdef DEBUG
					if (config->debug >= 6)
						printf("DEBUG - Extracting string [%s]\n", fields[fieldPos]);
#endif

					fieldPos++;
					positionStatus = LHS;

					/* switch field state */
					curFieldType = FIELD_TYPE_UNDEF;
					runLen = 1;
					startOfField = curLinePos++;
					inQuotes = FALSE;
				}
				else
				{
					inQuotes = TRUE;
					runLen++;
					curLinePos++;
				}
			}
			else if ((line[curLinePos] EQ '|') || (iscntrl(line[curLinePos])) || !(isprint(line[curLinePos])))
			{
				if (fields[fieldPos] EQ NULL)
				{
					if ((fields[fieldPos] = (char *)XMALLOC(MAX_FIELD_LEN)) EQ NULL)
					{
						fprintf(stderr, "ERR - Unable to allocate memory for string\n");
						return (fieldPos - 1);
					}
				}
				fields[fieldPos][runLen] = '\0';
				XMEMCPY(fields[fieldPos], line + startOfField, runLen);

#ifdef DEBUG
				if (config->debug >= 6)
					printf("DEBUG - Extracted string [%s]\n", fields[fieldPos]);
#endif

				fieldPos++;
				positionStatus = LHS;

				/* switch field state */
				curFieldType = FIELD_TYPE_UNDEF;
				runLen = 1;
				startOfField = curLinePos++;
			}
			else
			{
				runLen++;
				curLinePos++;
			}
		}
		else
		{
			/* LHS */
			if (curFieldType EQ FIELD_TYPE_STRING)
			{
				if (isalnum(line[curLinePos]))
				{
					runLen++;
					curLinePos++;
				}
				else if (line[curLinePos] EQ '=')
				{
					if (fields[fieldPos] EQ NULL)
					{
						if ((fields[fieldPos] = (char *)XMALLOC(MAX_FIELD_LEN)) EQ NULL)
						{
							fprintf(stderr, "ERR - Unable to allocate memory for string\n");
							return (fieldPos - 1);
						}
					}
					fields[fieldPos][runLen] = '\0';
					XMEMCPY(fields[fieldPos], line + startOfField, runLen);

#ifdef DEBUG
					if (config->debug >= 6)
						printf("DEBUG - Extracting string [%s]\n", fields[fieldPos]);
#endif

					fieldPos++;
					positionStatus = RHS;

					/* switch field state */
					curFieldType = FIELD_TYPE_UNDEF;
					curLinePos++;
				}
				else
				{
					/* something is wrong */
					curLinePos++;
				}
			}
		}
	}

	return (fieldPos);
}

/****
 *
 * return parsed field
 *
 ****/

int getParsedField(char *oBuf, int oBufLen, const unsigned int fieldNum)
{
	if ((fieldNum >= MAX_FIELD_POS) || (fields[fieldNum] EQ NULL))
	{
		fprintf(stderr, "ERR - Requested field does not exist [%d]\n", fieldNum);
		oBuf[0] = 0;
		return (FAILED);
	}
	XSTRNCPY(oBuf, fields[fieldNum], oBufLen);
	return (TRUE);
}
