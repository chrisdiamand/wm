
#ifndef LIST_H
#define LIST_H

struct List
{
    void    **items;
    size_t  size;
    size_t  allocated;
};

struct List *List_new(void);
void List_free(struct List *);
void *List_elem(struct List *, size_t);
void List_push_back(struct List *, void *);

#endif

