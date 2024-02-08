#include <stdio.h>
#include <stdlib.h>
#include "layout.h"
#include "parse_xml.h"

#define TABSPACES 4
#define MAX_TOKLEN 255
#define LT_TAGNAME "Layout"

static void write_dimension(FILE *f, Dimension *dim, char dimchar, int indent)
{
    if (dim->type == SCALE) {
        fprintf(f, "%*s<%c>%s %f</%c>\n", indent, "", dimchar, get_dimtype_str(dim->type), dim->value.floatval, dimchar);
    } else {
        fprintf(f, "%*s<%c>%s %d</%c>\n", indent, "", dimchar, get_dimtype_str(dim->type), dim->value.intval, dimchar);
    }
}

static const char *get_bool_str(bool b)
{
    return b ? "true" : "false";
}

bool read_bool_str(char *bstr)
{
    if (strncmp(bstr, "true", 4) == 0) {
        return true;
    } else {
        return false;
    }
}


void write_layout(FILE *f, Layout *lt, int indent)
{
    fprintf(f, "%*s<Layout name=\"%s\" display=\"%s\">\n", indent, "", lt->name, get_bool_str(lt->display));
    // fprintf(f, "%*s<name>%s</name>\n", indent + TABSPACES, "", lt->name);
    write_dimension(f, &(lt->x), 'x', indent + TABSPACES);
    write_dimension(f, &(lt->y), 'y', indent + TABSPACES);
    write_dimension(f, &(lt->w), 'w', indent + TABSPACES);
    write_dimension(f, &(lt->h), 'h', indent + TABSPACES);

    fprintf(f, "%*s<index>%d</index>\n", indent + TABSPACES, "", lt->index);
    fprintf(f, "%*s<children>\n", indent + TABSPACES, "");

    for (uint8_t i=0; i<lt->num_children; i++) {
        write_layout(f, lt->children[i], indent + TABSPACES * 2);
    }
    fprintf(f, "%*s</children>\n", indent + TABSPACES, "");

    fprintf(f, "%*s</Layout>\n", indent, "");
}


// // 0 on success; EOF on EOF; -1 on error
// static int await_token(FILE *f, const char *token) 
// {
//     int c;
//     char buffer[MAX_TOKLEN];
//     int len = strlen(token);

//     while ((c=fgetc(f)) != token[0] && c != EOF) {}
//     if (c == EOF) {
//         return EOF;
//     }
//     fpos_t saved_pos;
//     if (fgetpos(f, &saved_pos) != 0) {
//         fprintf(stderr, "Error saving file stream pos\n");
//         return -1;
//     }
//     buffer[0] = c;
//     for (int i=1; i<len; i++) {
//         buffer[i] = fgetc(f);
//     }
//     buffer[len] = '\0';
//     if (strcmp(buffer, token) != 0) {
//         fsetpos(f, &saved_pos);
//         await_token(f, token);
//     }
//     return 0;
// }

// // 1 on start tag, -1 on end tag, 0 on error, EOF on EOF
// static int find_next_tag(FILE *f, char *start_tag, char *end_tag) 
// {
//     int c;
//     char buffer[MAX_TOKLEN];
//     int startlen = strlen(start_tag);
//     int endlen = strlen(end_tag);

//     int len = startlen > endlen ? startlen : endlen;

//     while ((c=fgetc(f)) != start_tag[0] && c != end_tag[0] && c != EOF) {}
//     if (c == EOF) {
//         return EOF;
//     }
//     fpos_t saved_pos;
//     if (fgetpos(f, &saved_pos) != 0) {
//         fprintf(stderr, "Error saving file stream pos\n");
//         return 0;
//     }
//     buffer[0] = c;
//     for (int i=1; i<len; i++) {
//         buffer[i] = fgetc(f);
//     }
//     buffer[len] = '\0';
//     if (strncmp(buffer, start_tag, startlen) == 0) {
//         return 1;
//     } else if (strncmp(buffer, end_tag, endlen) == 0) {
//         return -1;
//     } else {
//         return find_next_tag(f, start_tag, end_tag);
//     }
// }

// void get_tag_range(FILE *f, const char *tagname, long *end)
// {
//     fpos_t start_pos;
//     fgetpos(f, &start_pos);

//     int taglen = strlen(tagname);
//     char start_tag[taglen + 3];
//     char end_tag[taglen + 4];
//     start_tag[0] = '<';
//     end_tag[0] = '<';
//     end_tag[1] = '/';
//     for (int i=0; i<taglen + 2; i++) {
//         start_tag[i+1] = tagname[i];
//         end_tag[i+2] = tagname[i];
//     }
//     start_tag[taglen + 1] = '>';
//     end_tag[taglen + 2] = '>';
//     start_tag[taglen + 2] = '\0';
//     end_tag[taglen + 3] = '\0';

//     // First start tag puts a 1
//     int tag_sum = find_next_tag(f, start_tag, end_tag);
//     while (tag_sum > 0) {
//         tag_sum += find_next_tag(f, start_tag, end_tag);
//     }
//     *end = ftell(f);
//     fsetpos(f, &start_pos);
// }

// long get_search_end_pos(FILE *f, const char *search_token)
// {
//     fpos_t start_pos;
//     fgetpos(f, &start_pos);
//     await_token(f, search_token);
//     long ret = ftell(f);
//     fsetpos(f, &start_pos);
//     return ret;
// }

static void read_dimension(char *dimstr, Dimension *dim)
{
    int val_i;
    if (strncmp(dimstr, "ABS", 3) == 0) {
        dim->type = ABS;
        val_i = 4;
    } else if (strncmp(dimstr, "REL", 3) == 0) {
        dim->type = REL;
        val_i = 4;
    } else if (strncmp(dimstr, "SCALE", 5) == 0) {
        dim->type = SCALE;
        val_i = 6;
    }
    switch (dim->type) {
        case ABS:
            dim->value.intval = atoi(dimstr + val_i);
            break;
        case REL:
            dim->value.intval = atoi(dimstr + val_i);
            break;
        case SCALE:
            dim->value.floatval = atof(dimstr + val_i);
            break;
    }
}


// /* return attribute string length */
// static int get_xml_attribute(FILE *f, char *dst, const char *attr_name)
// {
//     await_token(f, attr_name);
//     await_token(f, "=");
//     await_token(f, "\"");
//     char c;
//     int i=0;
//     while ((c = fgetc(f)) != '"' && i < MAX_TOKLEN - 1) {
//         dst[i] = c;
//         i++;
//     }
//     dst[i] = '\0';
//     return i + 1;
// }

static Layout *get_layout_from_tag(Tag *lt_tag)
{
    Layout *lt = create_layout();
    for (int i=0; i<lt_tag->num_attrs; i++) {
        Attr *attr = lt_tag->attrs[i];
        if (strcmp(attr->name, "name") == 0) {
            strcpy(lt->name, attr->value);
        } else if (strcmp(attr->name, "display") == 0) {
            lt->display = read_bool_str(attr->value);
        }
    }
    for (int i=0; i<lt_tag->num_children; i++) {
        Tag *childtag = lt_tag->children[i];
        fprintf(stderr, "CHECK CHILDTAG: %s\n", childtag->tagname);
        if (strcmp(childtag->tagname, "x") == 0) {
            fprintf(stderr, "Reading dim %s\n", childtag->inner_text);
            read_dimension(childtag->inner_text, &(lt->x));
        } else if (strcmp(childtag->tagname, "y") == 0) {
            read_dimension(childtag->inner_text, &(lt->y));
        } else if (strcmp(childtag->tagname, "w") == 0) {
            read_dimension(childtag->inner_text, &(lt->w));
        } else if (strcmp(childtag->tagname, "h") == 0) {
            read_dimension(childtag->inner_text, &(lt->h));
        } else if (strcmp(childtag->tagname, "children") == 0) {
            fprintf(stderr, "\tDoing children\n");
            for (int j=0; j<childtag->num_children; j++) {
                fprintf(stderr, "\t%d / %d\n", j, childtag->num_children);
                Tag *child_lt_tag = childtag->children[j];
                Layout *child_lt = get_layout_from_tag(child_lt_tag);
                fprintf(stderr, "CHILD %s of %s\n", child_lt->name, lt->name);
                reparent(child_lt, lt);
            }
        }
        fprintf(stderr, "Completed if statements\n");
    }
    return lt;
}

static Layout *read_layout(FILE *f, long endrange)
{
    fprintf(stderr, "Reading lt\n");
    Tag *lt_tag = store_next_tag(f, -1);
    fprintf(stderr, "Stored tag!\n");
    if (strcmp(lt_tag->tagname, "Layout") != 0) {
        fprintf(stderr, "Error reading xml file: root tag is not a 'Layout' tag.");
        return NULL;
    }

    Layout *lt = get_layout_from_tag(lt_tag);

    fprintf(stderr, "Done reading; destroying tag\n");
    destroy_tag(lt_tag);
    fprintf(stderr, "Destroyed tag.\n");

    fprintf(stderr, "Children of the lt:\n");
    for (int i=0; i<lt->num_children; i++) {
        fprintf(stderr, "\t%d/%d: %s\n", i, lt->num_children, lt->children[i]->name);
    }
    return lt;
}

Layout *read_layout_from_xml(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: unable to open file at %s\n", filename);
        return NULL;
    }
    return read_layout(f, 0);
}

Layout *read_xml_to_lt(Layout *dst, const char *filename)
{
    Layout *opened = read_layout_from_xml(filename);
    if (!opened) {
        return dst;
    }
    // opened->x = dst->x;
    // opened->y = dst->y;
    // opened->h = dst->h;
    // opened->w = dst->w;

    if (dst->parent) {
        reparent(opened, dst->parent);
    }
    // opened->parent = dst->parent;

    for (uint8_t i=0; i<dst->num_children; i++) {
        Layout *child = dst->children[i];
        reparent(child, opened);
        dst->children[i] = NULL;
    }

    dst->num_children = 0;
    opened->display = true;
    delete_layout(dst);
    reset_layout(opened);
    return opened;

    
}

// void open_lt_xml_in_rect(SDL_Rect *dst, const char *filename)
// {
//     Layout *opened = read_layout_from_xml(filename);
//     resize_layout(opened, dst->w, dst->h);
// }

void write_layout_to_file(Layout *lt)
{
    char filename[MAX_LT_NAMELEN + 4];
    strcpy(filename, lt->name);
    strcat(filename, ".xml");
    FILE *f = fopen(filename, "w");
    fprintf(stderr, "Attempting to write to %s\n", filename);
    if (!f) {
        fprintf(stderr, "Unable to write file at %s\n", filename);
        return;
    }
    write_layout(f, lt, 0);
    fclose(f);
}