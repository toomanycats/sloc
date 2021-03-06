#ifndef _SLOC_H_
#define _SLOC_H_

#define VERSION "1.1"

#define MAX(a, b) ((a) > (b)) ? (a) : (b)

/* length array indices */
#define IDX_LANG    0
#define IDX_TOT     1
#define IDX_CODE    2
#define IDX_COM     3
#define IDX_BLANK   4
#define IDX_FILE    5
#define NUM_MEMBERS 6

/* label strings */
#define STR_LANG    "Language"
#define STR_TOT     "Total"
#define STR_CODE    "Code"
#define STR_COM     "Comment"
#define STR_BLANK   "Blank"
#define STR_FILE    "Files"

/* number of spaces for print formatting */
#define SPACES "  "

typedef struct _sloc_t
{
    int tot;
    int code;
    int com;
    int blank;
    int files;
} sloc_t;

typedef struct _sloc_list_t
{
    int idx;
    char *name;
    char *tot;
    char *code;
    char *com;
    char *blank;
    char *files;
    struct _sloc_list_t *next;
} sloc_list_t;

/*
 *  disp_version
 *      display version information about the program, then exit
 *      successfully.
 */
void disp_version(void);

/*
 *  disp_usage
 *      display usage information about the program, then exit successfully.
 *  args:
 *      @prog   : the name of the program (taken from argv[0])
 */
void disp_usage(char *prog);

/*
 *  get_lang_idx
 *      get the index of the language with the given name
 *  args:
 *      @name   : the name of the language to find
 *  return:
 *      returns the index of the given language, or -1 if it is not found
 */
int get_lang_idx(char *name);

/*
 *  count_lines
 *      count the total number of sloc in the given file. if the given file
 *      is a directory, count the total number of lines in each file in the
 *      directory.
 *  args:
 *      @filename   : the name of the file or folder to count
 *      @counts     : the location to store the line counts
 */
void count_lines(char *filename, sloc_t *counts);

/*
 *  get_stdin_filenames
 *      get the list of files to count from standard input, one per line.
 *      simple reads in each line and passes the string to the count_lines
 *      function.
 *  args:
 *      @counts : the location to store the line counts
 */
void get_stdin_filenames(sloc_t *counts);

/*
 *  strends_with
 *      if haystack ends with needle, return nonzero. else return 0.
 *  args:
 *      @haystack   : the string to look at
 *      @needle     : the string to look for
 *  return:
 *      returns 0 if haystack ends with needle. else, returns nonzero.
 */
int strends_with(char *haystack, char *needle);

/*
 *  get_file_lang
 *      gets the language of the specified file
 *  args:
 *      @filename   : the name of the file
 *  return:
 *      returns an index to langs of the language to use, or -1 if the
 *      language is not found.
 */
int get_file_lang(char *filename);

/*
 *  count_file
 *      converts the given filename into a buffered file stream, then counts
 *      the lines in the stream
 *  args:
 *      @filename   : the name of the file to count
 *      @counts     : the sloc counter to add to
 *      @lang       : the language to use
 */
void count_file(char *filename, sloc_t *counts, int lang);

/*
 *  count_stdin
 *      counts the number of lines of code from the stdin stream using the
 *      given language.
 *  args:
 *      @lang   : a string representing the language to use
 *      @counts : the sloc counter to add to
 */
void count_stdin(char *lang, sloc_t *counts);

/*
 *  count_stream
 *      counts the number of lines of code in the given file stream
 *  args:
 *      @fp     : a pointer to the stream to count
 *      @counts : the sloc counter to add to
 *      @lang   : the language to use
 */
void count_stream(FILE *fp, sloc_t *counts, int lang);

/*
 *  count_folder
 *      count the lines of code in every file in the given directory
 *  args:
 *      @dirname    : the name of the directory to count
 *      @counts     : the location to store all the file counts
 */
void count_folder(char *dirname, sloc_t *counts);

/*
 *  print_sloc
 *      prints the total number of sloc counted in a neat table, sorted
 *      according to the number of lines of code.
 *  args:
 *      counts      : the counted lines of code
 *      print_tots  : whether or not to print the totals
 */
void print_sloc(sloc_t *counts, int print_tots);

/*
 *  set_max_lens
 *      sets the maximum lengths if the given strings are longer than the
 *      given lengths
 *  args:
 *      @mems   : the member strings
 *      @maxn   : the maximum lengths
 */
void set_max_lens(char mems[][BUFSIZ], int *maxn);

/*
 *  add_sloc_item
 *      adds a new print-friendly sloc structure to the given list of sloc
 *      counters.
 *  args:
 *      @lst    : the list to add to
 *      @idx    : the index to sort by
 *      @s      : the strings to use
 */
void add_sloc_item(sloc_list_t **lst, int idx, char s[][BUFSIZ]);

/*
 *  free_sloc_list
 *      frees all the memory from the given sloc list
 *  args:
 *      @lst    : the list to free
 */
void free_sloc_list(sloc_list_t *lst);

/*
 *  print_sloc_list
 *      print the sloc list using the given lengths for each member
 *  args:
 *      @list   : the list to print
 *      @lens   : the lengths for each member
 */
void print_sloc_list(sloc_list_t *list, int *lens);

/*
 *  print_member
 *      print the member of the sloc list with the given length
 *  args:
 *      @s  : the string to print
 *      @n  : the total number of characters to print
 */
void print_member(char *s, int n);


int scan_for_code_before_comment(char *s, int stop_index);

#endif /* _SLOC_H_ */
