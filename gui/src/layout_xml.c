/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

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
        fprintf(f, "%*s<%c>%s %f</%c>\n", indent, "", dimchar, layout_get_dimtype_str(dim->type), dim->value.floatval, dimchar);
    } else {
        fprintf(f, "%*s<%c>%s %d</%c>\n", indent, "", dimchar, layout_get_dimtype_str(dim->type), dim->value.intval, dimchar);
    }
}


/* Unused at the moment ? */

/* static const char *get_bool_str(bool b) */
/* { */
/*     return b ? "true" : "false"; */
/* } */

static const char *get_lt_type_str(LayoutType tp)
{
    switch (tp){
    case NORMAL:
	return "NORMAL";
    case PRGRM_INTERNAL:
	return "PRGRM_INTERNAL";
    case TEMPLATE:
	return "TEMPLATE";
    case ITERATION:
	return "ITERATION";
    default:
	return NULL;
    }
}

bool read_bool_str(char *bstr)
{
    if (strncmp(bstr, "true", 4) == 0) {
        return true;
    } else {
        return false;
    }
}


void layout_write(FILE *f, Layout *lt, int indent)
{
    if (lt->type == PRGRM_INTERNAL) {
        return;
    }
    fprintf(f, "%*s<Layout name=\"%s\" type=\"%s\">\n", indent, "", lt->name, get_lt_type_str(lt->type));

    // fprintf(f, "%*s<Layout name=\"%s\" display=\"%s\" internal=\"%s\">\n", indent, "", lt->name, get_bool_str(lt->display), get_bool_str(lt->internal));
    // fprintf(f, "%*s<name>%s</name>\n", indent + TABSPACES, "", lt->name);

    write_dimension(f, &(lt->x), 'x', indent + TABSPACES);
    write_dimension(f, &(lt->y), 'y', indent + TABSPACES);
    write_dimension(f, &(lt->w), 'w', indent + TABSPACES);
    write_dimension(f, &(lt->h), 'h', indent + TABSPACES);


    fprintf(f, "%*s<index>%d</index>\n", indent + TABSPACES, "", lt->index);
    if (lt->type == TEMPLATE) {
        fprintf(f, "%*s<iterator>%s %d</iterator>\n", indent + TABSPACES, "", layout_get_itertype_str(lt->iterator->type), lt->iterator->num_iterations);
    }
    fprintf(f, "%*s<children>\n", indent + TABSPACES, "");

    for (uint8_t i=0; i<lt->num_children; i++) {
        layout_write(f, lt->children[i], indent + TABSPACES * 2);
    }
    fprintf(f, "%*s</children>\n", indent + TABSPACES, "");
    fprintf(f, "%*s</Layout>\n", indent, "");
}

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
    } else if (strncmp(dimstr, "COMPLEMENT", 10) == 0) {
	dim->type = COMPLEMENT;
	val_i = 11;
    } else if (strncmp(dimstr, "STACK", 5) == 0) {
	dim->type = STACK;
	val_i = 6;
    } else if (strncmp(dimstr, "PAD", 3) == 0) {
	dim->type = PAD;
	val_i = 4;
    } else if (strncmp(dimstr, "REVREL", 6) == 0) {
	dim->type = REVREL;
	val_i = 7;
    }
    switch (dim->type) {
    case ABS:
    case REL:
    case COMPLEMENT:
    case STACK:
    case PAD:
    case REVREL:
	dim->value.intval = atoi(dimstr + val_i);
	break;
    case SCALE:
	dim->value.floatval = atof(dimstr + val_i);
	break;
    }
}

static void read_iterator(char *iterstr, IteratorType *type, int *num_iterations)
{
    int val_i = 0;
    if (strncmp(iterstr, "VERTICAL", 8) == 0) {
        *type = VERTICAL;
        val_i = 9;
    } else if (strncmp(iterstr, "HORIZONTAL", 10) == 0) {
        *type = HORIZONTAL;
        val_i = 11;
    }
    *num_iterations = atoi(iterstr + val_i);

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

LayoutType read_lt_type_str(char *lt_type_str)
{
    if (strcmp(lt_type_str, "NORMAL") == 0) {
        return NORMAL;
    } else if (strcmp(lt_type_str, "PRGRM_INTERNAL") == 0) {
        return PRGRM_INTERNAL;
    } else if (strcmp(lt_type_str, "TEMPLATE") == 0) {
        return TEMPLATE;
    } else if (strcmp(lt_type_str, "ITERATION") == 0) {
        return ITERATION;
    } else {
        fprintf(stderr, "Error: unable to read lt type string: \"%s\"\n", lt_type_str);
        return NORMAL;
    }
}

static Layout *get_layout_from_tag(Tag *lt_tag)
{
    Layout *lt = layout_create();
    for (int i=0; i<lt_tag->num_attrs; i++) {
        Attr *attr = lt_tag->attrs[i];
        if (strcmp(attr->name, "name") == 0) {
            strcpy(lt->name, attr->value);
        } else if (strcmp(attr->name, "type") == 0) {
            lt->type = read_lt_type_str(attr->value);
        }
        // } else if (strcmp(attr->name, "display") == 0) {
        //     lt->display = read_bool_str(attr->value);
        // } else if (strcmp(attr->name, "internal") == 0) {
        //     lt->internal = read_bool_str(attr->value);
        // }
    }
    for (int i=0; i<lt_tag->num_children; i++) {
        Tag *childtag = lt_tag->children[i];
        if (strcmp(childtag->tagname, "x") == 0) {
            read_dimension(childtag->inner_text, &(lt->x));
        } else if (strcmp(childtag->tagname, "y") == 0) {
            read_dimension(childtag->inner_text, &(lt->y));
        } else if (strcmp(childtag->tagname, "w") == 0) {
            read_dimension(childtag->inner_text, &(lt->w));
        } else if (strcmp(childtag->tagname, "h") == 0) {
            read_dimension(childtag->inner_text, &(lt->h));
        } else if (strcmp(childtag->tagname, "children") == 0) {
            for (int j=0; j<childtag->num_children; j++) {
                Tag *child_lt_tag = childtag->children[j];
                Layout *child_lt = get_layout_from_tag(child_lt_tag);
                // fprintf(stderr, "NUM CHILDREN: %d/%d\n", j, childtag->num_children);
                layout_reparent(child_lt, lt);
            }
        } else if (strcmp(childtag->tagname, "iterator") == 0) {
            IteratorType iter_type;
            int num_iterations;
            read_iterator(childtag->inner_text, &iter_type, &num_iterations);
            layout_create_iter_from_template(lt, iter_type, num_iterations, true);
        
        }
    }
    return lt;
}

static Layout *read_layout(FILE *f, long endrange)
{
    Tag *lt_tag = xml_store_next_tag(f, -1);
    if (strcmp(lt_tag->tagname, "Layout") != 0) {
        fprintf(stderr, "Error reading xml file: root tag is not a 'Layout' tag.");
	xml_destroy_tag(lt_tag);
        return NULL;
    }

    Layout *lt = get_layout_from_tag(lt_tag);

    xml_destroy_tag(lt_tag);
    return lt;
}

Layout *layout_read_from_xml(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: unable to open file at %s\n", filename);
	perror("(Error in fopen)");
        return NULL;
    }
    Layout *ret = read_layout(f, 0);
    fclose(f);
    return ret;
}

Layout *layout_read_xml_to_lt(Layout *dst, const char *filename)
{
    Layout *opened = layout_read_from_xml(filename);
    if (!opened) {
        return dst;
    }

    layout_reparent(opened, dst);
    /* fprintf(stderr, "\"%s\" now a child of \"%s\"\n", opened->name, opened->parent->name); */
    // if (dst->parent) {
    //     reparent(opened, dst->parent);
    // }

    // for (uint8_t i=0; i<dst->num_children; i++) {
    //     Layout *child = dst->children[i];
    //     reparent(child, opened);
    //     dst->children[i] = NULL;
    // }

    // dst->num_children = 0;
    // opened->display = true;
    // delete_layout(dst);
    layout_reset(opened);
    return opened;
}

// void open_lt_xml_in_rect(SDL_Rect *dst, const char *filename)
// {
//     Layout *opened = read_layout_from_xml(filename);
//     resize_layout(opened, dst->w, dst->h);
// }

void layout_write_to_file(Layout *lt)
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
    fprintf(stdout, "Arguments to lt write: %p, %p\n", f, lt);
    layout_write(f, lt, 0);
    fclose(f);
}
