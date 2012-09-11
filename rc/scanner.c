/*
 * Copyright (c) 2012 Chris Diamand
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
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "scanner.h"

/* This could be done with a table but it's not really worth bothering */
char *TokenName(TokenType t)
{
    /* No default: case so the compiler will warn when a type is missing */
    switch (t)
    {
        case TOK_NAME:              return "name";
        case TOK_INT:               return "integer";
        case TOK_STRING:            return "string";
        case TOK_EQUALS:            return "\'=\'";
        case TOK_COMMA:             return "\',\'";
        case TOK_EOF:               return "<eof>";
    }
    return "<?>";
}

/* Return an input source that reads from a file */
ScannerInput *ScannerInputFile(FILE *fp)
{
    ScannerInput *i = malloc(sizeof(ScannerInput));

    if (fp == NULL)
    {
        fprintf(stderr, "Null file pointer passed to ScannerInputFile\n");
        return NULL;
    }

    i->fp = fp;
    i->filename = "<unknown>";
    i->str = NULL;
    i->pos = 0;
    return i;
}

ScannerInput *ScannerInputString(char *s)
{
    ScannerInput *i = malloc(sizeof(ScannerInput));

    if (s == NULL)
    {
        fprintf(stderr, "Null string passed to ScannerInputFile()\n");
        return NULL;
    }

    i->str = s;
    i->pos = 0;
    i->fp = NULL;
    return i;
}

/* Reads the next character from the input source */
static char retrieve_next_char(ScannerInput *I)
{
    if (I->str != NULL)
    {
        char c = I->str[(I->pos)++];
        if (c == '\0')
            return 0;
        return c;
    }
    /* FIXME: Add some sort of buffering instead of calling getc() loads */
    else if (I->fp != NULL)
    {
        int i;
        i = getc(I->fp);
        if (i == EOF)
        {
            return 0;
        }
        return (char) i;
    }
    else
    {
        fprintf(stderr, "Input source has no sources set\n");
    }
    return 0;
}

/* Wrapper for retrieve_next_char() that counts the line numbers */
static char next_char(ScannerInput *I)
{
    char c = retrieve_next_char(I);
    if (c == '\n')
        I->line_number++;
    return c;
}


static void put_back(ScannerInput *I, char c)
{
    if (c == '\n')
        I->line_number--;
    if (I->str != NULL)
    {
        I->pos =- 1;
    }
    else if (I->fp != NULL)
    {
        /* FIXME: This won't work with a buffer (see above, next_char() ) */
        ungetc(c, I->fp);
    }
}

/* FIXME: Add escape sequences \n etc */
static char *read_quoted_string(ScannerInput *I)
{
    unsigned int len = 32, pos = 0;
    char *s = malloc(len), c;
    if (s == NULL)
    {
        fprintf(stderr, "read_quoted_string(): malloc() failed!\n");
        return "";
    }
    do
    {
        c = next_char(I);
        if (pos >= len - 1)
        {
            char *NewStr;
            len *= 2;
            NewStr = realloc(s, len);
            if (NewStr == NULL)
            {
                fprintf(stderr, "read_quoted_string: realloc() failed!\n");
                return s;
            }
            s = NewStr;
        }
        s[pos++] = c;
    } while (c != '\"');
    /* Use pos - 1 to remove the final quote */
    s[pos - 1] = '\0';
    return s;
}

Token ScanToken(ScannerInput *I)
{
    char c = 1;
    int i; 
    Token T;
    while (c != 0)
    {
        c = next_char(I);
        if (isalpha(c)) /* An identifier */
        {
            char name[512]; /* Arbitrary limit to name length */
            name[0] = c;
            i = 1;
            c = next_char(I);
            while (isalnum(c) || c == '_')
            {
                name[i++] = c;
                c = next_char(I);
            }
            name[i] = 0;
            put_back(I, c);
            T.type = TOK_NAME;
            T.value = strdup(name);
            return T;
        }
        /* FIXME: Need to check that there is only 1 '.' and 'e' in a number.
         * Also maybe make 'e' an actual operator instead because that would be cool */
        if (isdigit(c))
        {
            char num[512];
            num[0] = c;
            i = 1;
            c = next_char(I);
            while (isdigit(c))
            {
                num[i++] = c;
                c = next_char(I);
            }
            num[i] = 0;
            put_back(I, c);
            T.type = TOK_INT;
            /* It must be strdup'd to stop it being destroyed as soon as this loop returns */
            T.value = strdup(num);
            return T;
        }
        if (c == '\"')
        {
            char *s = read_quoted_string(I);
            T.value = s;
            T.type = TOK_STRING;
            return T;
        }
        if (c == '#')
        {
            while (c != '\n')
                c = next_char(I);
        }
        switch (c)
        {
            case ',':   T.type = TOK_COMMA;         return T;
            case '=':   T.type = TOK_EQUALS;        return T;
            default:    break;
        }
    }
    T.type = TOK_EOF;
    return T;
}

