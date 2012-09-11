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

#include <stdio.h>
#include <stdlib.h>

#include "rc.h"
#include "scanner.h"

/* This can be global so it doesn't have to passed
 * into *every* parser function call */
static ScannerInput *scan_input = NULL;
static Token ct;

/* Read the next token from the input */
static void next_token(void)
    {   ct = ScanToken(scan_input);     }

/* Try and continue by looking for the start of the next statement */
static void syntax_error(TokenType type)
{
    fprintf(stderr, "Error in config file line %lu: Expected %s\n",
            scan_input->line_number, TokenName(type));
    do
    {
        next_token();
        if (ct.type == TOK_EOF || ct.type == TOK_NAME)
            return;
    } while (1);
}

/* Should be a TOK_NAME, TOK_EQUALS then a value */
static void assignment(void)
{
    char *name;
    if (ct.type != TOK_NAME)
    {
        syntax_error(TOK_NAME);
        return;
    }
    name = ct.value;
    printf("Name %s!\n", name);
    next_token();
    if (ct.type != TOK_EQUALS)
    {
        syntax_error(TOK_EQUALS);
        return;
    }
    next_token();

    /* Memory leaks? Nah... */
    free(name);
}

/* List of variable assignments, i.e. "name = value" */
static void assign_list(void)
{
    while (ct.type != TOK_EOF)
    {
        assignment();
    }
}

void rcp_parse(struct rc_t *R, ScannerInput *I)
{
    scan_input = I;
    next_token();

    assign_list();
}

