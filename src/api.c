/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    api.c

    * create and maintain data structures related to UDP API
    * setup and teardown UDP server
    * triage messages sent via UDP
 *****************************************************************************************************************/

#include <errno.h>
#include "api.h"
#include "endpoint.h"
#include "project.h"


#define API_HASH_TABLE_SIZE 1024
#define MAX_ROUTE_DEPTH 16

extern volatile bool CANCEL_THREADS;

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

/* extern Project *proj; */
static void *server_threadfn(void *arg)
{
    /* int port = *(int *)arg; */
    Project *proj = (Project *)arg;
    int port = proj->server.port;
    /* socklen_t len = sizeof(proj->server.servaddr); */
    if ((proj->server.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("Socket creation failed");
	exit(1);
    }

    memset(&proj->server.servaddr, 0, sizeof(proj->server.servaddr));

    proj->server.servaddr.sin_family = AF_INET;
    proj->server.servaddr.sin_addr.s_addr = INADDR_ANY;
    proj->server.servaddr.sin_port = htons(port);

    if (bind(proj->server.sockfd, (const struct sockaddr *)&proj->server.servaddr, sizeof(proj->server.servaddr)) < 0) {
	/* perror("Bind failed"); */
	proj->server.active = false;
	fprintf(stderr, "SERVERTHREAD: unlocking due to error\n");
        pthread_mutex_unlock(&proj->server.setup_lock);
	return NULL;
    }
    char buffer[1024];
    proj->server.active = true;
    fprintf(stderr, "SERVERTHREAD: unlocking due to success\n");
    pthread_mutex_unlock(&proj->server.setup_lock);
    while (proj->server.active) {
	struct sockaddr_in cliaddr;
	memset(&cliaddr, '\0', sizeof(cliaddr));
	socklen_t len = sizeof(cliaddr);
	if (recvfrom(proj->server.sockfd, (char *)buffer, 1024, 0, (struct sockaddr *)&cliaddr, &len) < 0) {
	    perror("recvfrom");
	    exit(1);
	}


	/* struct sockaddr_in client_info = *(struct sockaddr_in *)&sa; */
	/* fprintf(stderr, "CLI: %d, %d\n", ntohs(client_info.sin_port), client_info.sin_addr.s_addr); */
	/* fprintf(stderr, "HOST: %d, %d\n", ntohs(proj->server.servaddr.sin_port), proj->server.servaddr.sin_addr.s_addr); */
						 
						 
	/* fprintf(stderr, "SA family: %d", sa.sa_family); */
	/* fprintf(stderr, "HOST family: %d\n", ); */
	/* fprintf(stderr, "Rec: %s\n", buffer); */
	int val_offset = 0;
	for (int i=0; i<strlen(buffer); i++) {
	    if (buffer[i] == ' ') {
		buffer[i] = '\0';
		val_offset = i + 1;	    
	    } else if (buffer[i] == ';') {
		buffer[i] = '\0';
	    }
	}

	Endpoint *ep = api_endpoint_get(buffer);
	if (ep) {
	    Value new_val = jdaw_val_from_str(buffer + val_offset, ep->val_type);
	    endpoint_write(ep, new_val, true, true, true, false);
	}


	/* Send response */
	char *msg = ep ? "200 OK;" : "Error: endpoint not found";
	struct sockaddr_in cliaddr_info = *(struct sockaddr_in *)&cliaddr;
	fprintf(stderr, "Sending message to port %d\n", ntohs(cliaddr_info.sin_port));
	if (sendto(proj->server.sockfd, msg, strlen(msg), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == -1) {
	    perror("sendto");
	}

    }
    if (close(proj->server.sockfd) != 0) {
	perror("Error closing socket:");
    }
    fprintf(stderr, "Exiting server threadfn\n");
    /* proj->server.active = false; */
    return NULL;

}


static void api_teardown_server(Project *proj);

/* Server */
int api_start_server(Project *proj, int port)
{
    if (proj->server.active) {
	fprintf(stderr, "Server already active on port %d. Tearing down.\n", proj->server.port);
	api_teardown_server(proj);
    }
    pthread_mutex_init(&proj->server.setup_lock, NULL);
    pthread_mutex_lock(&proj->server.setup_lock);
    static pthread_t servthread;
    proj->server.port = port;
    fprintf(stderr, "\n\nCalling pthread create....\n");
    pthread_create(&servthread, NULL, server_threadfn, (void *)proj);
    pthread_mutex_lock(&proj->server.setup_lock);
    fprintf(stderr, "... OK WE;re unlocked in main\n");
    if (!proj->server.active) {
	perror("Server setup failed: ");
	return 1;
    }

    fprintf(stderr, "Server active on port %d\n", port);
    proj->server.thread_id = servthread;

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
	api_hash_table[i] = NULL; /* Hash table will be reused if project swapped out */
    }
}

static void api_teardown_server(Project *proj)
{
    fprintf(stderr, "Tearing down server running on port %d. Sending quit message...\n", proj->server.port);
    proj->server.active = false;
    for (int i=0; i<5; i++) {
	char *msg = "quit";
	if (sendto(proj->server.sockfd, msg, strlen(msg), 0, (struct sockaddr *)&proj->server.servaddr, sizeof(proj->server.servaddr)) == -1) {
	    perror("sendto");
	    exit(1);
	}
    }
    int err = pthread_join(proj->server.thread_id, NULL);
    if (err != 0) {
	fprintf(stderr, "Error in pthread join: %s\n", err == EINVAL ? "Value specified by rehad is not joinable" : err == ESRCH ? "No thread found" : err == EDEADLK ? "Deadlock detected, or value of thread specifies this (calling) thread" : "Unknown error");
    }
    fprintf(stderr, "\t..done.\n");
    
}

void api_quit(Project *proj)
{
    if (proj->server.active) api_teardown_server(proj);
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
	} else {
	    while (n) {
		char dst[255];
		api_endpoint_get_route(n->ep, dst, 255);
		if (init) {
		    fprintf(stderr, "%d: %s\n", i, dst);
		} else {
		    fprintf(stderr, "\t%d: %s\n", i, dst);
		}
		n = n->next;
		init = false;
	    }
	}
    }
}


