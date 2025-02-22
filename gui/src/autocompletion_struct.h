#ifndef JDAW_AUTOCOMPLETION_STRUCT_H
#define JDAW_AUTOCOMPLETION_STRUCT_H

#define AUTOCOMPLETE_ENTRY_BUFLEN 64


struct autocompletion_item {
    const char *str;
    void *obj;
};

typedef struct layout Layout;
typedef struct text_entry TextEntry;
typedef struct text_lines TextLines;
typedef struct autocompletion AutoCompletion;
typedef int (*TlinesFilter)(void *, void *);

typedef struct autocompletion {
    Layout *layout;
    char entry_buf[AUTOCOMPLETE_ENTRY_BUFLEN];
    TextEntry *entry;
    TextLines *lines;
    int (*update_records)(AutoCompletion *self, struct autocompletion_item **items_arr_p);
    TlinesFilter tline_filter;
    int selection; /* -1 if in textentry, otherwise index of selected line */
} AutoCompletion;


#endif
