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

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include "api.h"
#include "endpoint.h"

#define API_HASH_TABLE_SIZE 1024
#define MAX_ROUTE_DEPTH 16

static struct api_hash_node *api_hash_table[API_HASH_TABLE_SIZE] = {0};

static unsigned long api_hash_route(const char *route);
static void api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size);


static void api_endpoint_insert_into_table(Endpoint *ep)
{
    char route[255];
    api_endpoint_get_route(ep, route, 255);

    unsigned long hash_i = api_hash_route(route);
    APIHashNode *new = calloc(1, sizeof(struct api_hash_node));
    new->route = strdup(route);
    new->ep = ep;
    new->index = hash_i;
    ep->hash_node = new;
    APIHashNode *ahn = NULL;
    if ((ahn = api_hash_table[hash_i])) {
	while (ahn->next) {
	    ahn = ahn->next;
	}
	ahn->next = new;
	new->prev = ahn;
    } else {
	api_hash_table[hash_i] = new;
    }
}

/* Insert an endpoint into the API tree, and into the route hash table */
void api_endpoint_register(Endpoint *ep, APINode *parent)
{
    if (parent->num_endpoints == MAX_API_NODE_ENDPOINTS) {
	fprintf(stderr, "API setup error: node \"%s\" max num endpoints\n", parent->obj_name);
	return;
    }
    parent->endpoints[parent->num_endpoints] = ep;
    parent->num_endpoints++;
    ep->parent = parent;

    api_endpoint_insert_into_table(ep);

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


static void api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size)
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

/* dbj2: http://www.cse.yorku.ca/~oz/hash.html */
static unsigned long api_hash_route(const char *route)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *route++))
	hash = ((hash << 5) + hash) + c;
    return hash % API_HASH_TABLE_SIZE;
}

Endpoint *api_endpoint_get(const char *route)
{
    unsigned long hash = api_hash_route(route);
    APIHashNode *ahn = api_hash_table[hash];
    if (!ahn) return NULL;
    if (!ahn->next) return ahn->ep;
    while (1) {
	if (strcmp(ahn->route, route) == 0) {
	    return ahn->ep;
	} else if (ahn->next) {
	    ahn = ahn->next;
	} else {
	    return NULL;
	}
    }
}

static void api_hash_node_destroy(APIHashNode *ahn);
void api_node_renamed(APINode *an)
{
    for (int i=0; i<an->num_endpoints; i++) {
	Endpoint *ep = an->endpoints[i];
	APIHashNode *to_delete = ep->hash_node;
	unsigned long index = to_delete->index;
	/* APIHashNode *head = api_hash_table[index]; */
	
	if (!to_delete) continue;
	/* Case 2: we are in first slot */
	if (!to_delete->prev) {
	    if (to_delete->next) {
		APIHashNode *next = to_delete->next;
		api_hash_table[index] = next; /* Move 'next' into hash table */
		next->prev = NULL;
	    } else {
		/* api_hash_node_destroy(ahn); */
		api_hash_table[index] = NULL; /* Clear hash table entry */
	    }
	} else { /* Case 3: we are in the linked list */
	    to_delete->prev->next = to_delete->next;
	    if (to_delete->next) to_delete->next->prev = to_delete->prev;
	    /* api_hash_node_destroy(ahn); */
	}
	api_hash_node_destroy(to_delete);
	ep->hash_node = NULL;
	api_endpoint_insert_into_table(ep);
    }
    for (int i=0; i<an->num_children; i++) {
	api_node_renamed(an->children[i]);
    }

}

static void *server_threadfn(void *arg)
{
    int port = *(int *)arg;
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t len = sizeof(servaddr);
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("Socket creation failed");
	exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	perror("Bind failed");
	exit(1);
    }
    char buffer[1024];
    while (1) {
	if (recvfrom(sockfd, (char *)buffer, 1024, 0, (struct sockaddr *) &servaddr, &len) < 0) {
	    perror("recvfrom");
	    exit(1);
	}
	int val_offset = 0;
	for (int i=0; i<strlen(buffer); i++) {
	    if (buffer[i] == ' ') {
		buffer[i] = '\0';
		val_offset = i + 1;	    
	    } else if (buffer[i] == ';') {
		buffer[i] = '\0';
	    }
	}
	
	fprintf(stderr, "RECEIVED route: %s; val: %s\n", buffer, buffer + val_offset);
	Endpoint *ep = api_endpoint_get(buffer);
	double readval = atof(buffer + val_offset);
	Value new_val = {.float_v = readval};
	endpoint_write(ep, new_val, true, true, true, false);
    }


}


/* Server */
int api_start_server(int port)
{
    pthread_t servthread;
    pthread_create(&servthread, NULL, server_threadfn, &port);
    return 0;
}




/* Cleanup */


static void api_hash_node_destroy(APIHashNode *ahn)
{
    if (ahn->route)
	free(ahn->route);
    free(ahn);
}

static void api_hash_table_destroy()
{
    for (int i=0; i<API_HASH_TABLE_SIZE; i++) {
	APIHashNode *ahn = api_hash_table[i];
	while (ahn) {
	    APIHashNode *next = ahn->next;
	    api_hash_node_destroy(ahn);
	    ahn = next;
	}
    }
}


void api_quit()
{
    api_hash_table_destroy();
}




/* Testing & logging */

void api_node_print_all_routes(APINode *node)
{
    int dstlen = 255;
    char dst[dstlen];
    for (int i=0; i<node->num_endpoints; i++) {
	api_endpoint_get_route(node->endpoints[i], dst, dstlen);
	fprintf(stderr, "%s\n", dst);
	fprintf(stderr, "\t->hash: %lu\n", api_hash_route(dst));
    }
    for (int i=0; i<node->num_children; i++) {
        api_node_print_all_routes(node->children[i]);
    }
}

void api_table_print()
{
    for (int i=0; i<API_HASH_TABLE_SIZE; i++) {
	APIHashNode *n = api_hash_table[i];
	bool init = true;
	if (!n) {
	    fprintf(stderr, "%d: NULL\n", i);
	} else while (n) {
		char dst[255];
		api_endpoint_get_route(n->ep, dst, 255);
		if (init) {
		    fprintf(stderr, "%d: %s\n", i, dst);
		}else {
		    fprintf(stderr, "\t%d: %s\n", i, dst);
		}
		n = n->next;
		init = false;
	}
    }
}


