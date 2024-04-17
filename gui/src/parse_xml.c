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

#include "parse_xml.h"

#define not_whitespace_char(c) (c != ' ' && c != '\n' && c != '\t')
#define is_whitespace_char(c) (c == ' ' || c == '\n' || c == '\t')

static void clear_whitespace(FILE *f) 
{
    char c;
    while ((c = fgetc(f)) == ' ' || c == '\n' || c == '\t') {}
    ungetc(c, f);
}

static long xml_get_tag_endrange(FILE *f, char *tagname);

/*  get_next_tag:

    Search for the next xml tag, starting at the current file position.
    'startrange' is populated with the file offset of the character after the '>' in the opening tag;
    'endrange' is populated with the file offset of the character after the '>' in the end tag (</tagname)
    
    File position is set to the end of the opening tag.
    
    Returns:
        1 if a tag is found
        -1 if there's an error or no tag found.
 */
int xml_get_next_tag(FILE *f, char *buf, int maxlen, long *startrange, long *endrange)
{

    char c;
    int i=0;
    while ((c = fgetc(f)) != '<' && c != EOF) {}
    while ((c = fgetc(f)) != '>' && not_whitespace_char(c)) {
        if (c == EOF || i == maxlen-1) {
            return -1;
        }
        buf[i] = c;
        i++;
    }
    buf[i] = '\0';
    // ungetc(c, f);
    long saved_pos = ftell(f) - 1;

    /* Get the start of the tag range */
    long start;
    if (c == '>') {
        start = ftell(f);
    } else {
        while (c != '>') {
            c = fgetc(f);
        }
        start = ftell(f);

    }
    if (startrange) {
        *startrange = start;
    }

    fseek(f, saved_pos, 0);
    if (endrange) {
        *endrange = xml_get_tag_endrange(f, buf);
    }
    return 1;
}

static long xml_get_tag_endrange(FILE *f, char *tagname) 
{
    long saved_pos = ftell(f);
    long offset_end = -1;
    char *endtag = malloc(strlen(tagname) + 2);
    endtag[0] = '/';
    strcpy(endtag + 1, tagname);
    char buf[255];
    int tagsum = 1;
    while (tagsum > 0) {
        int t = xml_get_next_tag(f, buf, 255, &offset_end, NULL);
        if (t > 0) {
            if (strcmp(buf, tagname) == 0) {
                tagsum++;
            } else if (strcmp(buf, endtag) == 0) {
                tagsum--;
            }
        } else {
            break;
        }

    }
    if (tagsum != 0) {
        offset_end = -1;
    } else {
\
        offset_end -= strlen(endtag) + 2;
    }
    fseek(f, saved_pos, 0);
    free(endtag);
    return offset_end;
}

/*  get_tag_attribute:

    Assumes a tagname has just been found with the function above. 
    Returns:
        1 if an attribute is found
        -1 on error or no additional attributes (closing brace found).
*/
int xml_get_tag_attr(FILE *f, char *attr_name_buf, char *attr_value_buf, int maxlen)
{
    char c;
    int i=0;
    clear_whitespace(f);
    while ((c = fgetc(f)) != '=') {
        if (c == EOF || c == '>' || i == maxlen-1) {
            return -1;
        }
        if (not_whitespace_char(c)) {
            attr_name_buf[i] = c;
            i++;
        }
    }
    attr_name_buf[i] = '\0';
    clear_whitespace(f);
    i=0;
    bool open_quote = false;
    while ((c = fgetc(f)) != '>' && !(is_whitespace_char(c) && !open_quote)) {
        if (c == EOF || i == maxlen-1) {
            return -1;
        } else if (c == '"') {
            open_quote = open_quote ? false : true;
        } else if (open_quote) {
            attr_value_buf[i] = c;
            i++;
        }
    }
    ungetc(c, f);
    attr_value_buf[i] = '\0';
    return 1;
}


/* Tag structure */

Tag *xml_create_tag(void)
{
    Tag *tag = malloc(sizeof(Tag));
    tag->num_attrs = 0;
    tag->num_children = 0;
    tag->inner_text = NULL;
    tag->tagname[0] = '\0';
    return tag;
}

Attr *xml_create_attr(void)
{
    Attr *attr = malloc(sizeof(Attr));
    attr->name = NULL;
    attr->value = NULL;
    return attr;
}

Tag *xml_store_next_tag(FILE *f, long endrange)
{
    char *buf = malloc(256);
    char *buf2 = malloc(256);
    long start,end;
    if (xml_get_next_tag(f, buf, 256, &start, &end) == -1 || (start > endrange && endrange != -1) || end - start < 0) {
        free(buf);
        free(buf2);
        fseek(f, start, 0);
        return NULL;
    } 
    Tag *ret = xml_create_tag();
    strncpy(ret->tagname, buf, 32);

    while (xml_get_tag_attr(f, buf, buf2, 256) > 0) {
        if (buf[0] == '/') {
            continue;
        }
        Attr *new_attr = xml_create_attr();
        new_attr->name = malloc(strlen(buf) + 1);
        new_attr->value = malloc(strlen(buf2) + 1);
        strcpy(new_attr->name, buf);
        strcpy(new_attr->value, buf2);
        ret->attrs[ret->num_attrs] = new_attr;
        ret->num_attrs++;
    }

    long saved_pos = ftell(f);

    ret->inner_text = malloc(1 + end - start);
    fseek(f, start, 0);
    int i=0;
    while (i<(end-start)) {
        ret->inner_text[i] = fgetc(f);
        i++;
    }
    ret->inner_text[i] = '\0';

    fseek(f, saved_pos, 0);
    Tag *child = NULL;

    while ((child = xml_store_next_tag(f, end)) != NULL) {
        ret->children[ret->num_children] = child;
        ret->num_children++;
    }
    free(buf);
    free(buf2);
    return ret;
}

void xml_destroy_attr(Attr *attr)
{
    if (attr->name) {
        free(attr->name);
    }
    if (attr->value) {
        free(attr->value);
    }
    free(attr);
}
void xml_destroy_tag(Tag *tag)
{
    for (int i=0; i<tag->num_children; i++) {
        xml_destroy_tag(tag->children[i]);
    }
    for (int i=0; i<tag->num_attrs; i++) {
        xml_destroy_attr(tag->attrs[i]);
    }
    free(tag->inner_text);
    free(tag);
}

void xml_fprint_tag_recursive(FILE *f, Tag *tag, int indent)
{
    fprintf(f, "%*c<%s", indent * TABSPACES, ' ', tag->tagname);
    for (int i=0; i<tag->num_attrs; i++) {
        fprintf(f, " %s=\"%s\"", tag->attrs[i]->name, tag->attrs[i]->value);
    }
    if (tag->num_children == 0) {
        fprintf(f, ">%s</%s>\n", tag->inner_text, tag->tagname);
    } else {
        fprintf(f, ">\n");
        for (int i=0; i<tag->num_children; i++) {
            xml_fprint_tag_recursive(f, tag->children[i], indent+1);
        }
        fprintf(f, "%*c</%s>\n", indent * TABSPACES, ' ', tag->tagname);
    }
}
