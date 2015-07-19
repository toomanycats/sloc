/*
 *
 * sloc.c
 *      implements functions for counting the lines of source code in a given
 *      directory (including all subdirectories). code is easily modified to
 *      add new languages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>

#include "sloc.h"
#include "languages.h"

#define NUM_LANGS sizeof(langs) / sizeof(lang_t)

int main(int argc, char **argv)
{
    int     i;
    int     numcounts = 0;
    char *  pwd;
    sloc_t  counts[NUM_LANGS];
    int     print_tots = 1;

    /* initilize all the count values to 0 */
    for (i = 0; i < NUM_LANGS; i++)
    {
        counts[i].tot = 0;
        counts[i].code = 0;
        counts[i].com = 0;
        counts[i].blank = 0;
        counts[i].files = 0;
    }

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            /* version info */
            disp_version();
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            /* help message */
            disp_usage(argv[0]);
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            /* don't print totals */
            print_tots = 0;
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            /* check that the next argument exists */
            if (++i == argc)
            {
                disp_usage(argv[0]);
            }
            /* tell program not to count pwd */
            numcounts++;
            /* count lines from stdin using the given language */
            count_stdin(argv[i], counts);
        }
        else if (strcmp(argv[i], "-") == 0)
        {
            /* don't count pwd */
            numcounts++;
            /* get file list from stdin */
            get_stdin_filenames(counts);
        }
        else
        {
            /* tell program not to count pwd */
            numcounts++;
            /* count lines from the given file */
            count_lines(argv[i], counts);
        }
    }

    /* if no counts were performed, count the pwd */
    if (numcounts == 0)
    {
        /* pwd is dynamically allocated */
        pwd = getcwd(NULL, 0);
        count_lines(pwd, counts);
        free(pwd);
    }

    /* print the results */
    print_sloc(counts, print_tots);

    return EXIT_SUCCESS;
}

void disp_version(void)
{
    printf("sloc-"VERSION", (c) 2013-14 Brian Kubisiak, "
           "see LICENSE for details\n");
    exit(EXIT_SUCCESS);
}

void disp_usage(char *prog)
{
    printf("usage: %s [-v] [-h] [-n] [-t lang] [-] [file] [...]\n", prog);
    exit(EXIT_SUCCESS);
}

int get_lang_idx(char *name)
{
    int i;

    for (i = 0; i < NUM_LANGS; i++)
    {
        if (strcmp(name, langs[i].name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void count_lines(char *filename, sloc_t *counts)
{
    struct stat sb;
    int         lang;

    if (stat(filename, &sb) == -1)
    {
        return;
    }

    if (S_ISDIR(sb.st_mode) != 0)
    {
        count_folder(filename, counts);
    }
    else if ( S_ISREG(sb.st_mode) != 0 &&
              (lang = get_file_lang(filename)) != -1)
    {
        count_file(filename, counts + lang, lang);
    }

    else if ( S_ISREG(sb.st_mode) != 0 &&
            (lang = get_file_lang(filename)) == -1)
    {
        count_file(filename, counts + 1, lang);
    }
}

void get_stdin_filenames(sloc_t *counts)
{
    char    name[BUFSIZ];
    char *  nl;

    while (fgets(name, BUFSIZ, stdin) != NULL)
    {
        nl = strchr(name, '\n');
        if (nl != NULL)
        {
            *nl = '\0';
        }
        count_lines(name, counts);
    }
}

int strends_with(char *haystack, char *needle)
{
    char *  end = haystack + strlen(haystack) - strlen(needle);

    if (strcmp(end, needle) == 0)
    {
        return 1;
    }
    return 0;
}

int get_file_lang(char *filename)
{
    int i;
    int j;

    for (i = 0; i < NUM_LANGS; i++)
    {
        for (j = 0; langs[i].ext[j] != NULL; j++)
        {
            if (strends_with(filename, langs[i].ext[j]) != 0)
            {
                return i;
            }
        }
    }

    return -1;
}

void count_file(char *filename, sloc_t *counter, int lang)
{
    FILE *  fp;

    fp = fopen(filename, "r");
    if (fp != NULL)
    {
        count_stream(fp, counter, lang);
    }
}

void count_stdin(char *lang, sloc_t *counts)
{
    int idxlang;

    idxlang = get_lang_idx(lang);
    if (idxlang == -1)
    {
        fprintf(stderr, "error: '%s' is not a known language!\n", lang);
        exit(EXIT_FAILURE);
    }
    count_stream(stdin, &counts[idxlang], idxlang);
}

int scan_for_code_before_comment(char *s, int stop_index)
{
    int i;
    int codeline = 0;

    for (i=0; i < stop_index; i++)
    {
        if (!isspace(s[i]))
        {
            codeline = 1;
            break;
        }
    }
    return codeline;
}

void count_stream(FILE *fp, sloc_t *counter, int lang)
{
    char    s[BUFSIZ];
    char    codeline = 0;
    char    comline = 0;
    int     blankline = 0;
    char    *found_blk, *found_inline, *end_blk;
    int     ind_blk, ind_inline, ind_end, in_blk_com_flag=0;

    counter->files++;

    /* loop through doc */
    while (fgets(s, BUFSIZ, fp) != NULL)
    {
        /* if the lang has inline and bulk comments */
        if (langs[lang].eol != NULL && langs[lang].startblk != NULL)
        {

            found_blk = strstr(s, langs[lang].startblk);
            if (found_blk != NULL)
                ind_blk = found_blk - s;
            else
                ind_blk = -1;

            found_inline = strstr(s, langs[lang].eol);
            if (found_inline != NULL)
                ind_inline = found_inline - s;
            else
                ind_inline = -1;

            end_blk = strstr(s, langs[lang].endblk);
            if (end_blk != NULL)
                ind_end = end_blk - s;
            else
                ind_end = -1;

            /* switch on the cases */
            if (ind_inline == 0)
            {
                comline = 1;
            }

            else if (ind_blk == 0 && ind_end > 0)
            {
                comline = 1;
            }

            else if (ind_blk == 0 && ind_end < 0)
            {
                in_blk_com_flag = 1;
                comline = 1;
            }

            else if (ind_inline < 0 && ind_blk < 0 && ind_end < 0 && in_blk_com_flag == 0 && strcmp(s, "\n") != 0 && strcmp(s, "\r") != 0)
            {
                codeline = 1;
            }

            else if (ind_inline > 0 )
            {
                comline = 1;
                codeline = scan_for_code_before_comment(s, ind_inline);
            }


            else if (ind_blk > 0 && ind_end < 0)
            {
                in_blk_com_flag = 1;
                comline = 1;
                codeline = scan_for_code_before_comment(s, ind_blk);
            }

            else if (ind_blk > 0 && ind_end > 0)
            {
                comline = 1;
                codeline = scan_for_code_before_comment(s, ind_blk);
            }

            else if (in_blk_com_flag == 1 && ind_end < 0)
            {
                    comline = 1;
            }

            else if (in_blk_com_flag == 1  && ind_end >= 0)
            {
                in_blk_com_flag = 0;
                comline = 1;
            }

            else if (in_blk_com_flag == 0 && (strcmp(s, "\n") == 0 || strcmp(s, "\r") == 0))
                blankline = 1;
        }

        /* the lang has only inline */
        else if (langs[lang].eol != NULL && langs[lang].startblk == NULL)
        {
            found_inline = strstr(s, langs[lang].eol);
            if (found_inline != NULL)
                ind_inline = found_inline - s;
            else
                ind_inline = -1;

            if (ind_inline == 0)
            {
                comline = 1;
            }

            else if (ind_inline < 0 && strcmp(s, "\n") != 0 && strcmp(s, "\r") != 0)
            {
                codeline = 1;
            }

            else if (ind_inline < 0 && (strcmp(s, "\n") == 0 || strcmp(s, "\r") == 0))
                blankline = 1;

            else if (ind_inline > 0 )
            {
                comline = 1;
                codeline = scan_for_code_before_comment(s, ind_inline);
            }
        }

        /* file ext unknown */
        else if (lang == -1)
        {
            counter->tot++;
            continue;
        }

        /* in while loop */
        counter->tot++;
        counter->code += codeline;
        counter->com += comline;
        counter->blank += blankline;

        blankline = 0;
        codeline = 0;
        comline = 0;
    }

fclose(fp);
}

void count_folder(char *dirname, sloc_t *counts)
{
    char            path[BUFSIZ];   /* path to the next file */
    int             idx;            /* index of the NULL byte in 'path' */
    DIR *           dp;
    struct dirent * next;

    strncpy(path, dirname, BUFSIZ - 1);
    path[BUFSIZ - 2] = '\0';
    idx = strlen(path);
    path[idx] = '/';
    idx++;

    if ((dp = opendir(dirname)) == NULL)
    {
        return;
    }

    while ((next = readdir(dp)) != NULL)
    {
        if (*(next->d_name) != '.')
        {
            strncpy(path + idx, next->d_name, BUFSIZ - idx - 1);
            count_lines(path, counts);
        }
    }

    closedir(dp);
}

void print_sloc(sloc_t *counts, int print_tots)
{
    int     i;
    sloc_t  tots;
    char    s[NUM_MEMBERS][BUFSIZ];
    int     maxn[NUM_MEMBERS] =
    {
        sizeof(STR_LANG) - 1,
        sizeof(STR_TOT) - 1,
        sizeof(STR_CODE) - 1,
        sizeof(STR_COM) - 1,
        sizeof(STR_BLANK) - 1,
        sizeof(STR_FILE) - 1,
    };
    sloc_list_t *   lst = NULL;

    tots.tot = 0;
    tots.code = 0;
    tots.com = 0;
    tots.blank = 0;
    tots.files = 0;

    /* put the data in a list that is suited for printing */
    for (i = 0; i < NUM_LANGS; i++)
    {
        if (counts[i].files != 0)
        {
            tots.tot += counts[i].tot;
            tots.code += counts[i].code;
            tots.com += counts[i].com;
            tots.blank += counts[i].blank;
            tots.files += counts[i].files;

            /* check lengths of the entries */
            strcpy(s[IDX_LANG], langs[i].name);
            snprintf(s[IDX_TOT], BUFSIZ, "%d", counts[i].tot);
            snprintf(s[IDX_CODE], BUFSIZ, "%d", counts[i].code);
            snprintf(s[IDX_COM], BUFSIZ, "%d", counts[i].com);
            snprintf(s[IDX_BLANK], BUFSIZ, "%d", counts[i].blank);
            snprintf(s[IDX_FILE], BUFSIZ, "%d", counts[i].files);
            set_max_lens(s, maxn);

            add_sloc_item(&lst, counts[i].code, s);
        }
    }

    if (print_tots != 0)
    {
        strcpy(s[IDX_LANG], STR_TOT);
        snprintf(s[IDX_TOT], BUFSIZ, "%d", tots.tot);
        snprintf(s[IDX_CODE], BUFSIZ, "%d", tots.code);
        snprintf(s[IDX_COM], BUFSIZ, "%d", tots.com);
        snprintf(s[IDX_BLANK], BUFSIZ, "%d", tots.blank);
        snprintf(s[IDX_FILE], BUFSIZ, "%d", tots.files);
        set_max_lens(s, maxn);

        add_sloc_item(&lst, tots.tot, s);
    }


    strcpy(s[IDX_LANG], STR_LANG);
    strcpy(s[IDX_TOT], STR_TOT);
    strcpy(s[IDX_CODE], STR_CODE);
    strcpy(s[IDX_COM], STR_COM);
    strcpy(s[IDX_BLANK], STR_BLANK);
    strcpy(s[IDX_FILE], STR_FILE);

    add_sloc_item(&lst, INT_MAX, s);

    print_sloc_list(lst, maxn);

    free_sloc_list(lst);
}

void set_max_lens(char mems[][BUFSIZ], int *maxn)
{
    int i;

    for (i = 0; i < NUM_MEMBERS; i++)
    {
        maxn[i] = MAX(maxn[i], strlen(mems[i]));
    }
}

void add_sloc_item(sloc_list_t **lst, int idx, char s[][BUFSIZ])
{
    sloc_list_t **  cur = lst;
    sloc_list_t *   new;

    new = (sloc_list_t *)malloc(sizeof(sloc_list_t));
    if (new == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new->idx = idx;
    new->name = strdup(s[IDX_LANG]);
    new->tot = strdup(s[IDX_TOT]);
    new->code = strdup(s[IDX_CODE]);
    new->com = strdup(s[IDX_COM]);
    new->blank = strdup(s[IDX_BLANK]);
    new->files = strdup(s[IDX_FILE]);

    for (;;)
    {
        if (*cur == NULL || (*cur)->idx < new->idx)
        {
            new->next = *cur;
            *cur = new;
            return;
        }
        cur = &((*cur)->next);
    }
}

void free_sloc_list(sloc_list_t *lst)
{
    sloc_list_t *cur;
    sloc_list_t *next;

    cur = lst;
    while (cur != NULL)
    {
        next = cur->next;
        free(cur->name);
        free(cur->tot);
        free(cur->code);
        free(cur->com);
        free(cur->blank);
        free(cur->files);
        free(cur);
        cur = next;
    }
}

void print_sloc_list(sloc_list_t *list, int *lens)
{
    sloc_list_t *   cur = list;

    while (cur != NULL)
    {
        print_member(cur->name, lens[IDX_LANG]);
        print_member(cur->files, lens[IDX_FILE]);
        print_member(cur->code, lens[IDX_CODE]);
        print_member(cur->com, lens[IDX_COM]);
        print_member(cur->blank, lens[IDX_BLANK]);
        print_member(cur->tot, lens[IDX_TOT]);

        putchar('\n');

        cur = cur->next;
    }
}

void print_member(char *s, int n)
{
    int i;

    for (i = 0; i < n - strlen(s); i++)
    {
        putchar(' ');
    }
    printf("%s"SPACES, s);
}
