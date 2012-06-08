
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct df_t
{
    FILE        *fp;
    char        *fname;
    int         lineno;
};

void skip_to_next_line(struct df_t *D);

static int has_extension(char *fname, char *ext)
{
    int i = strlen(fname) - 1, j = strlen(ext) - 1;
    while (i >= 0 && j >= 0)
    {
        if (fname[i] != ext[j])
            return 0;
        i--;
        j--;
    }
    return 1;
}

static char next_char(struct df_t *D)
{
    char c = fgetc(D->fp);
    if (c == '\n')
        D->lineno++;
    /* Comments start with a hash */
    if (c == '#')
    {
        skip_to_next_line(D);
        c = '\n';
    }
    /* Escape sequences */
    if (c == '\\')
        c = fgetc(D->fp);
    return c;
}

void skip_to_next_line(struct df_t *D)
{
    char c = next_char(D);
    while (c != '\n' && !feof(D->fp))
        c = next_char(D);
}

static void error(struct df_t *D, char *msg)
{
    fprintf(stderr, "Error: %s - Line %d: %s\n", D->fname, D->lineno, msg);
    skip_to_next_line(D);
}

/* Skip whitespace, return the next non-whitespace character */
static char skip_whitespace(struct df_t *D, int skip_newlines)
{
    char c = next_char(D);
    if (skip_newlines)
    {
        while (isspace(c))
            c = next_char(D);
    }
    else
    {
        while (isspace(c) && c != '\n')
            c = next_char(D);
    }
    return c;
}

char *keyname(struct df_t *D, char c)
{
    int i = 0;
    char key[64];
    while ( (i < sizeof(key)) && (isalnum(c)
                || c == '-' || c == '_' || c == '@'
                || c == '[' || c == ']') )
    {
        key[i++] = c;
        c = next_char(D);
    }
    key[i] = '\0';
    ungetc(c, D->fp);
    if (i > 0)
        return strdup(key);
    return NULL;
}

void read_groupname(struct df_t *D)
{
    char c = '[';
    while (c != ']')
    {
        c = next_char(D);
    }
}

char *read_rest_of_line(struct df_t *D, char c)
{
    int i = 0;
    char str[256]; /* Easier than doing loads of realloc() stuff */
    while ( (i < sizeof(str)) && (c != '\n') && !feof(D->fp) )
    {
        str[i++] = c;
        /* If the line is too long just ignore the extra. It's probably something
         * boring like MimeType that we don't care about anyway */
        if (i >= sizeof(str))
        {
            skip_to_next_line(D);
            break;
        }
        c = next_char(D);
    }
    str[i] = '\0';
    return strdup(str);
}

/* Read a key=value pair from a .desktop file */
void read_line(struct df_t *D, char **key, char **val)
{
    char c;
    *key = NULL;
    *val = NULL;
    c = skip_whitespace(D, 1);
    if (c == '[')
    {
        read_groupname(D);
        return;
    }

    *key = keyname(D, c);

    if (!(*key)) /* Syntax error? */
    {
        error(D, "Expected key or group name");
        return;
    }

    c = skip_whitespace(D, 0);

    if (c != '=')
    {
        error(D, "Expected \'=\' after key");
        free(*key);
        *key = NULL;
        return;
    }

    /* Allow whitespace after '=' but don't skip newlines
     * If newlines are eaten by skip_whitespace and there is a line
     * like "MimeType=" with no value, then read_rest_of_line
     * will get the entire *next* line, not the end of the current one */
    c = skip_whitespace(D, 0);

    *val = read_rest_of_line(D, c);

    /* If there is an EOF after the end of the line, make sure
     * it's picked up by the while(!feof(D.fp)) in read_desktop_file */
    ungetc(skip_whitespace(D, 1), D->fp);
}

void read_desktop_file(char *fname, char **app_name, char **app_exec)
{
    struct df_t D;
    char *key = NULL, *value = NULL;
    D.fname = fname;
    D.lineno = 1;

    /* Check it's a ".desktop" file */
    if (!has_extension(fname, ".desktop"))
        return;

    D.fp = fopen(fname, "r");
    if (!D.fp)
    {
        perror(fname);
        return;
    }

    *app_name = NULL;
    *app_exec = NULL;
    while (!feof(D.fp))
    {
        read_line(&D, &key, &value);
        if (key && value)
        {
            if (!strcmp(key, "Name"))
                *app_name = value;
            else if (!strcmp(key, "Exec"))
                *app_exec = value;
            else
                {   free(key);  free(value);    }
        }
    }
    if (!(*app_name))
        error(&D, "Does not have a \'Name=\' field");
    if (!(*app_exec))
        error(&D, "Does not have a \'Exec=\' field");
}

void scan_applications_dir(char *path)
{
    DIR *dp = opendir(path);
    struct dirent *entry;
    char *name, *exec;
    if (!dp)
    {
        perror(path);
        return;
    }
    while ( (entry = readdir(dp)) )
    {
        char *n = entry->d_name, *fullpath;
        if (!strcmp(n, "..") || !strcmp(n, "."))
            continue;
        fullpath = malloc( sizeof(char) * (strlen(n) + strlen(path) + 2) );
        sprintf(fullpath, "%s/%s", path, n);
        switch (entry->d_type)
        {
            case DT_DIR: /* Recursively scan directories */
                scan_applications_dir(fullpath);
                break;
            case DT_REG: /* A regular file */
                read_desktop_file(fullpath, &name, &exec);
                break;
        }
        free(fullpath);
    }
    closedir(dp);
}
/*
int main(int argc, char **argv)
{
    int i;
    char *dirs[] = {"/usr/share/applications", "/usr/local/share/applications", NULL};
    
    for (i = 0; dirs[i]; i++)
    {
        scan_applications_dir(dirs[i]);
    }
    return 0;
}
*/
