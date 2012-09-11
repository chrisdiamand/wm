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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"

#define ASSERT_RETURN(COND, MSG)                        \
do                                                      \
{                                                       \
    if (!(COND))                                        \
    {                                                   \
        fprintf(stderr, "%s line %u: %s failed: %s\n",  \
                __FILE__, __LINE__,                     \
                __STRING(COND), MSG);                   \
        return;                                         \
    }                                                   \
} while(0);

struct List *List_new(void)
{
    struct List *L = malloc(sizeof(struct List));
    L->items = NULL;
    L->size = 0;
    L->allocated = 0;
    return L;
}

void List_free(struct List *L)
{
    if (L->items)
        free(L->items);
    free(L);
}

void *List_elem(struct List *L, size_t idx)
{
    assert(idx >= 0 && idx < L->size);
    return L->items[idx];
}

void List_push_back(struct List *L, void *value)
{
    if (L->size + 1 >= L->allocated)
    {
        void **n;
        size_t new_size;
        if (L->allocated == 0)
            new_size = 2;
        else
            new_size = (L->allocated * 3) / 2;
        n = realloc(L->items, new_size * sizeof(void *));

        ASSERT_RETURN(n, "Out of memory!");

        L->items = n;
        L->allocated = new_size;
    }
    L->items[L->size++] = value;
}

