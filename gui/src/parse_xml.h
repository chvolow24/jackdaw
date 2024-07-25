#ifndef JDAW_GUI_PARSE_XML_H
#define JDAW_GUI_PARSE_XML_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAGNAME_LEN 32
#define MAX_SUBTAGS 256
#define MAX_ATTRS 32
#define TABSPACES 4

typedef struct attr {
    char *name;
    char *value;
} Attr;

typedef struct tag Tag;
typedef struct tag {
    char tagname[MAX_TAGNAME_LEN];
    Attr *attrs[MAX_ATTRS];
    int num_attrs;
    Tag *children[MAX_SUBTAGS];
    int num_children;
    char *inner_text;
} Tag;

/*  get_next_tag:

    Search for the next xml tag, starting at the current file position.
    'startrange' is populated with the file offset of the character after the '>' in the opening tag;
    'endrange' is populated with the file offset of the character after the '>' in the end tag (</tagname)
    
    File position is set to the end of the opening tag.
    
    Returns:
        1 if a tag is found
        -1 if there's an error or no tag found.
 */
int xml_get_next_tag(FILE *f, char *tagname_buf, int maxlen, long *startrange, long *endrange);


/*  get_tag_attribute:

    Assumes a tagname has just been found with the function above. 
    Returns:
        1 if an attribute is found
        -1 on error or no additional attributes (closing brace found).
*/
int xml_get_tag_attr(FILE *f, char *attr_name_buf, char *attr_value_buf, int maxlen);


Tag *xml_store_next_tag(FILE *f, long endrange);
void xml_destroy_tag(Tag *tag);
void xml_fprint_tag_recursive(FILE *f, Tag *tag, int indent);

#endif
