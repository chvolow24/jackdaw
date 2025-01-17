/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
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

/*****************************************************************************************************************
    api.c

    * create and maintain data structures related to UDP API
    * setup and teardown UDP server
    * triage messages sent via UDP
 *****************************************************************************************************************/

#include "api.h"
#include "endpoint.h"

#define API_HASH_TABLE_SIZE 1024
#define MAX_ROUTE_DEPTH 16

void api_endpoint_register(Endpoint *ep, APINode *parent)
{
    if (parent->num_endpoints == MAX_API_NODE_ENDPOINTS) {
	fprintf(stderr, "API setup error: node \"%s\" max num endpoints\n", parent->obj_name);
	return;
    }
    parent->endpoints[parent->num_endpoints] = ep;
    parent->num_endpoints++;
    ep->parent = parent;
    
}


void api_node_register(APINode *node, APINode *parent, char *obj_name)
{
    if (parent->num_children == MAX_API_NODE_CHILDREN) {
	fprintf(stderr, "API setup error: node \"%s\" max num children\n", parent->obj_name);
	return;
    }
    parent->children[parent->num_children] = node;
    parent->num_children++;
    node->obj_name = obj_name;
    node->parent = parent;
}


static char make_idchar(char c)
{
    if (c >= 'a' && c <= 'z') return c;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    if (c >= '0' && c <= '9') return c;
    return '_';
}

void api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size)
{
    char *components[MAX_ROUTE_DEPTH];
    int num_components = 0;

    components[num_components] = (char *)ep->local_id;
    num_components++;
    APINode *node = ep->parent;
    while (node && node->parent) {
	components[num_components] = node->obj_name;
	num_components++;
	node = node->parent;
    }

    num_components--;
    int offset = 0;
    while (num_components >= 0) {
	char *str = components[num_components];
	char lowered[dst_size];
	for (int i=0; i<strlen(str); i++) {
	    lowered[i] = make_idchar(str[i]);
	}
	lowered[strlen(str)] = '\0';
	offset += snprintf(dst + offset, dst_size - offset, "/%s", lowered);
	num_components--;
    }   
}

void api_node_print_all_routes(APINode *node)
{
    int dstlen = 255;
    char dst[dstlen];
    for (int i=0; i<node->num_endpoints; i++) {
	api_endpoint_get_route(node->endpoints[i], dst, dstlen);
	fprintf(stderr, "%s\n", dst);
    }
    for (int i=0; i<node->num_children; i++) {
        api_node_print_all_routes(node->children[i]);
    }
}
