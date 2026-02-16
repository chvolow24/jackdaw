/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include "api.h"
#include "endpoint.h"
#include "session.h"
#include "string.h"
#include "type_serialize.h"
#include "value.h"


#define API_HASH_TABLE_SIZE 1024
#define MAX_ROUTE_DEPTH 16
#define MAX_ROUTE_LEN 256

extern volatile bool CANCEL_THREADS;

static struct api_hash_node *api_hash_table[API_HASH_TABLE_SIZE] = {0};
static struct api_hash_node **stashed_api_hash_table = NULL;
static APINode stashed_api_root = {0};


static unsigned long api_hash_route(const char *route);
int api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size);
static APIHashNode **api_endpoint_get_hash_node(const char *route);
static void api_hash_node_destroy(APIHashNode *ahn);

static void api_endpoint_insert_into_table(Endpoint *ep)
{
    char route[255];
    api_endpoint_get_route(ep, route, 255);
    /* fprintf(stderr, "INSERT route %s\n", route); */
    unsigned long hash_i = api_hash_route(route);
    APIHashNode *new = calloc(1, sizeof(struct api_hash_node));
    new->route = strdup(route);
    new->ep = ep;
    new->index = hash_i;
    /* fprintf(stderr, "\t->hash: %lu\n", hash_i); */
    ep->hash_node = new;
    APIHashNode *ahn = NULL;
    if ((ahn = api_hash_table[hash_i])) {
	/* fprintf(stderr, "\t->entry exists!\n"); */
	while (ahn->next) {
	    ahn = ahn->next;
	}
	ahn->next = new;
	new->prev = ahn;
    } else {
	/* fprintf(stderr, "\t->inserting...\n"); */
	api_hash_table[hash_i] = new;
    }
}

/* Insert an endpoint into the API tree, and into the route hash table */
void api_endpoint_register(Endpoint *ep, APINode *parent)
{
    /* fprintf(stderr, "REGISTER EP %s to %s\n", ep->local_id, parent->obj_name); */
    if (parent->num_endpoints == MAX_API_NODE_ENDPOINTS) {
	fprintf(stderr, "API setup error: node \"%s\" max num endpoints\n", parent->obj_name);
	return;
    }
    parent->endpoints[parent->num_endpoints] = ep;
    parent->num_endpoints++;
    ep->parent = parent;

    api_endpoint_insert_into_table(ep);
}


void api_node_register(APINode *node, APINode *parent, char *obj_name, const char *fixed_name)
{
    if (parent->num_children == MAX_API_NODE_CHILDREN) {
	fprintf(stderr, "API setup error: node \"%s\" max num children\n", parent->obj_name);
	return;
    }
    parent->children[parent->num_children] = node;
    parent->num_children++;
    node->obj_name = obj_name;
    node->fixed_name = fixed_name;
    node->parent = parent;
}


static void api_node_deregister_internal(APINode *node, bool remove_from_parent)
{
    if (node->parent && remove_from_parent && node->parent->num_children > 0) {
	bool displace = false;
	for (int i=0; i<node->parent->num_children; i++) {
	    APINode *sibling = node->parent->children[i];
	    if (sibling == node) displace = true;
	    else if (displace) {
		node->parent->children[i - 1] = node->parent->children[i];
	    }
	}
	node->parent->num_children--;
	/* fprintf(stderr, "Node %s children:\n", node->parent->obj_name); */
	/* for (int i=0; i<node->parent->num_children; i++) { */
	/*     fprintf(stderr, "\t->%s\n", node->parent->children[i]->obj_name); */
	/* } */
    }
    for (int i=0; i<node->num_endpoints; i++) {
	Endpoint *ep = node->endpoints[i];
	if (ep->automation && !ep->automation->track->deleted) {
	    automation_remove(ep->automation);
	}
	char route[255];
	api_endpoint_get_route(ep, route, 255);
	APIHashNode **ahn = api_endpoint_get_hash_node(route);
	if (ahn) {
	    (*ahn)->deregistered = true;
	    /* if ((*ahn)->prev) { */
	    /* 	(*ahn)->prev->next = (*ahn)->next; */
	    /* 	if ((*ahn)->next) { */
	    /* 	    (*ahn)->next->prev = (*ahn)->prev; */
	    /* 	} */
	    /* 	/\* api_hash_node_destroy(*ahn); *\/ */
	    /* 	/\* *ahn = NULL; *\/ */
	    /* } else if ((*ahn)->next) { */
	    /* 	(*ahn)->next->prev = NULL; */
	    /* 	APIHashNode *to_destroy = *ahn; */
	    /* 	*ahn = (*ahn)->next; */
	    /* 	api_hash_node_destroy(to_destroy); */
	    /* } else { */
	    /* 	api_hash_node_destroy(*ahn); */
	    /* 	*ahn = NULL; */
	    /* } */
	    /* /\* (*ahn)->deregistered = true; *\/ */
	}
    }
    for (int i=0; i<node->num_children; i++) {
	api_node_deregister_internal(node->children[i], false);
    }

}
void api_node_deregister(APINode *node)
{
    api_node_deregister_internal(node, true);
}

static APIHashNode **api_endpoint_get_hash_node_incl_deregistered(const char *route);

static void api_node_reregister_internal(APINode *node, bool reinsert_into_parent)
{
    if (reinsert_into_parent && node->parent) {
	node->parent->children[node->parent->num_children] = node;
	node->parent->num_children++;
    }
    for (int i=0; i<node->num_endpoints; i++) {
	Endpoint *ep = node->endpoints[i];
	if (ep->automation && !ep->automation->deleted) {
	    automation_reinsert(ep->automation);
	}
	char route[255] = {0};
	api_endpoint_get_route(ep, route, 255);
	APIHashNode **ahn = api_endpoint_get_hash_node_incl_deregistered(route);
	if (ahn) {
	    (*ahn)->deregistered = false;
	}
    }
    for (int i=0; i<node->num_children; i++) {
	api_node_reregister_internal(node->children[i], false);
    }
}

void api_node_reregister(APINode *node)
{
    api_node_reregister_internal(node, true);
}

void api_node_set_defaults(APINode *node)
{
    for (int i=0; i<node->num_endpoints; i++) {
	Endpoint *ep = node->endpoints[i];
	if (ep->has_default_val) {
	    endpoint_write(ep, ep->default_val, true, true, true, false);
	}
    }
    for (int i=0; i<node->num_children; i++) {
	api_node_set_defaults(node->children[i]);
    }
}

/* void api_node_destroy(APINode *node) */
/* { */
/*     for (int i=0; i<node->num_endpoints; i++) { */
/* 	Endpoint *ep = node->endpoints[i]; */
/* 	char route[255]; */
/* 	api_endpoint_get_route(ep, route, 255); */
/* 	APIHashNode **ahn = api_endpoint_get_hash_node(route); */
/* 	if (ahn) { */
/* 	    (*ahn)->deregistered = true; */
/* 	} */
/*     } */
/*     for (int i=0; i<node->num_children; i++) { */
/* 	api_node_deregister(node->children[i]); */
/*     } */
/* } */

static char make_idchar(char c)
{
    if (c >= 'a' && c <= 'z') return c;
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    if (c >= '0' && c <= '9') return c;
    return '_';
}


int api_node_get_route(APINode *node, char *dst, size_t dst_size)
{
    char *components[MAX_ROUTE_DEPTH];
    int num_components = 0;

    while (node && node->parent) {
	components[num_components] = node->fixed_name ? (char *)node->fixed_name : node->obj_name;
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
    return offset;
}

int api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size)
{
    char *components[MAX_ROUTE_DEPTH];
    int num_components = 0;

    components[num_components] = (char *)ep->local_id;
    num_components++;
    APINode *node = ep->parent;
    while (node && node->parent) {
	components[num_components] = node->fixed_name ? (char *)node->fixed_name : node->obj_name;
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
    return offset;
}

/* "until" sets upper limit (exclusive) on tree navigation */
int api_endpoint_get_route_until(Endpoint *ep, char *dst, size_t dst_size, APINode *until)
{
    char *components[MAX_ROUTE_DEPTH];
    int num_components = 0;

    components[num_components] = (char *)ep->local_id;
    num_components++;
    APINode *node = ep->parent;
    while (node && node->parent && node != until) {
	components[num_components] = node->fixed_name ? (char *)node->fixed_name : node->obj_name;
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
    return offset;
}

int api_endpoint_get_display_route_until(Endpoint *ep, char *dst, size_t dst_size, APINode *until)
{
    char *components[MAX_ROUTE_DEPTH];
    int num_components = 0;

    components[num_components] = (char *)ep->display_name;
    num_components++;
    APINode *node = ep->parent;
    while (node && node->parent && node != until) {
	components[num_components] = node->fixed_name ? (char *)node->fixed_name : node->obj_name;
	num_components++;
	node = node->parent;
    }

    num_components--;
    int offset = 0;
    bool first = true;
    while (num_components >= 0) {
	char *str = components[num_components];
	/* char lowered[dst_size]; */
	/* for (int i=0; i<strlen(str); i++) { */
	/*     lowered[i] = make_idchar(str[i]); */
	/* } */
        /* str[strlen(str)] = '\0'; */
	if (first) {
	    offset += snprintf(dst + offset, dst_size - offset, "%s", str);
	    first = false;
	} else {
	    offset += snprintf(dst + offset, dst_size - offset, " / %s", str);
	}
	num_components--;
    }
    return offset;
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

static APIHashNode **api_endpoint_get_hash_node_incl_deregistered(const char *route)
{
    unsigned long hash = api_hash_route(route);
    APIHashNode **ahn = api_hash_table + hash;
    if (!*ahn) return NULL;
    if (!(*ahn)->next) return ahn;
    while (1) {
	if (strcmp((*ahn)->route, route) == 0) {
	    return ahn;
	} else if ((*ahn)->next) {
	    ahn = &(*ahn)->next;
	} else {
	    return NULL;
	}
    }
}

static APIHashNode **api_endpoint_get_hash_node(const char *route)
{
    unsigned long hash = api_hash_route(route);
    APIHashNode **ahn = api_hash_table + hash;
    if (!*ahn) return NULL;
    if (!(*ahn)->next && !(*ahn)->deregistered) return ahn;
    while (1) {
	if (!(*ahn)->deregistered && strcmp((*ahn)->route, route) == 0) {
	    return ahn;
	} else if ((*ahn)->next) {
	    ahn = &(*ahn)->next;
	} else {
	    return NULL;
	}
    }
    
}

Endpoint *api_endpoint_get(const char *route)
{
    APIHashNode **ahn = api_endpoint_get_hash_node(route);
    if (ahn) {
	return (*ahn)->ep;
    }
    return NULL;
    /* unsigned long hash = api_hash_route(route); */
    /* APIHashNode *ahn = api_hash_table[hash]; */
    /* if (!ahn) return NULL; */
    /* if (!ahn->next) return ahn->ep; */
    /* while (1) { */
    /* 	if (strcmp(ahn->route, route) == 0) { */
    /* 	    return ahn->ep; */
    /* 	} else if (ahn->next) { */
    /* 	    ahn = ahn->next; */
    /* 	} else { */
    /* 	    return NULL; */
    /* 	} */
    /* } */
}

static void api_hash_node_destroy(APIHashNode *ahn);

/* This function called in the text "name_completion" function.
   Any TextEntry with this completion will call it upon edit, assuming
   that the completion_target is the api node */
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
    Session *session = session_get();
    int port = session->server.port;
    if ((session->server.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("Socket creation failed");
	exit(1);
    }

    memset(&session->server.servaddr, 0, sizeof(session->server.servaddr));

    session->server.servaddr.sin_family = AF_INET;
    session->server.servaddr.sin_addr.s_addr = INADDR_ANY;
    session->server.servaddr.sin_port = htons(port);

    if (bind(session->server.sockfd, (const struct sockaddr *)&session->server.servaddr, sizeof(session->server.servaddr)) < 0) {
	/* perror("Bind failed"); */
	session->server.active = false;
        pthread_mutex_unlock(&session->server.setup_lock);
	return NULL;
    }
    char buffer[1024];
    session->server.active = true;
    pthread_mutex_unlock(&session->server.setup_lock);
    while (session->server.active) {
	struct sockaddr_in cliaddr;
	memset(&cliaddr, '\0', sizeof(cliaddr));
	socklen_t len = sizeof(cliaddr);
	memset(buffer, '\0', sizeof(buffer));
	if (recvfrom(session->server.sockfd, (char *)buffer, 1024, 0, (struct sockaddr *)&cliaddr, &len) < 0) {
	    perror("recvfrom");
	    exit(1);
	}


	/* struct sockaddr_in client_info = *(struct sockaddr_in *)&sa; */
	/* fprintf(stderr, "CLI: %d, %d\n", ntohs(client_info.sin_port), client_info.sin_addr.s_addr); */
	/* fprintf(stderr, "HOST: %d, %d\n", ntohs(session->server.servaddr.sin_port), session->server.servaddr.sin_addr.s_addr); */
						 
						 
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
	/* fprintf(stderr, "Val string: %s\n", buffer + val_offset); */

	Endpoint *ep = api_endpoint_get(buffer);
	if (ep) {
	    /* fprintf(stderr, "REC: %s\n", buffer); */
	    char dst[255];
	    api_endpoint_get_route(ep, dst, 255);
	    /* fprintf(stderr, "ROUTE: %s\n", dst); */
	    Value new_val = jdaw_val_from_str(buffer + val_offset, ep->val_type);
	    endpoint_write(ep, new_val, true, true, true, false);
	} else {
	    fprintf(stderr, "not found: %s\n", buffer);
	}


	/* Send response */
	char *msg = ep ? "200 OK;" : "Error: endpoint not found";
	if (sendto(session->server.sockfd, msg, strlen(msg), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) == -1) {
	    perror("sendto");
	}

    }
    if (close(session->server.sockfd) != 0) {
	perror("Error closing socket:");
    }
    fprintf(stderr, "Exiting server threadfn\n");
    /* session->server.active = false; */
    return NULL;

}


static void api_teardown_server();

/* Server */
int api_start_server(int port)
{
    Session *session = session_get();
    if (session->server.active) {
	fprintf(stderr, "Server already active on port %d. Tearing down.\n", session->server.port);
	api_teardown_server();
    }
    pthread_mutex_init(&session->server.setup_lock, NULL);
    pthread_mutex_lock(&session->server.setup_lock);
    static pthread_t servthread;
    session->server.port = port;
    pthread_create(&servthread, NULL, server_threadfn, NULL);
    pthread_mutex_lock(&session->server.setup_lock);
    if (!session->server.active) {
	perror("Server setup failed: ");
	return 1;
    }

    fprintf(stderr, "Server active on port %d\n", port);
    session->server.thread_id = servthread;

    return 0;
}




/* Cleanup */


static void api_hash_node_destroy(APIHashNode *ahn)
{
    if (ahn->route)
	free(ahn->route);
    free(ahn);
}

static void api_hash_table_destroy(APIHashNode **table)
{
    for (int i=0; i<API_HASH_TABLE_SIZE; i++) {
	APIHashNode *ahn = table[i];
	while (ahn) {
	    APIHashNode *next = ahn->next;
	    api_hash_node_destroy(ahn);
	    ahn = next;
	}
	table[i] = NULL; /* Hash table will be reused if project swapped out */
    }
}

static void api_teardown_server()
{
    Session *session = session_get();
    fprintf(stderr, "Tearing down server running on port %d. Sending quit message...\n", session->server.port);
    session->server.active = false;
    for (int i=0; i<5; i++) {
	char *msg = "quit";
	if (sendto(session->server.sockfd, msg, strlen(msg), 0, (struct sockaddr *)&session->server.servaddr, sizeof(session->server.servaddr)) == -1) {
	    perror("sendto");
	    exit(1);
	}
    }
    int err = pthread_join(session->server.thread_id, NULL);
    if (err != 0) {
	fprintf(stderr, "Error in pthread join: %s\n", err == EINVAL ? "Value specified by rehad is not joinable" : err == ESRCH ? "No thread found" : err == EDEADLK ? "Deadlock detected, or value of thread specifies this (calling) thread" : "Unknown error");
    }
    fprintf(stderr, "\t..done.\n");
    
}


void api_stash_current()
{
    stashed_api_hash_table = malloc(sizeof(api_hash_table));
    memcpy(stashed_api_hash_table, api_hash_table, sizeof(api_hash_table));
    memset(api_hash_table, 0, sizeof(api_hash_table));
    stashed_api_root = session_get()->server.api_root;
    memset(&session_get()->server.api_root, 0, sizeof(APINode));
}

void api_reset_from_stash_and_discard()
{
    memcpy(api_hash_table, stashed_api_hash_table, sizeof(api_hash_table));
    session_get()->server.api_root = stashed_api_root;
    free(stashed_api_hash_table);
}

void api_discard_stash()
{
    api_hash_table_destroy(stashed_api_hash_table);
    free(stashed_api_hash_table);
}
/* Use to remove all nodes and endpoints from the API */
/* void api_clear_all() */
/* { */
/*     memset(&session_get()->server.api_root, 0, sizeof(APINode)); */
/*     api_hash_table_destroy(); */

/* } */

void api_quit()
{
    Session *session = session_get();
    if (session->server.active) api_teardown_server();
    api_hash_table_destroy(api_hash_table);
}




/* Testing & logging */

void api_node_print_all_routes(APINode *node)
{
    int dstlen = 255;
    char dst[dstlen];
    for (int i=0; i<node->num_endpoints; i++) {
	api_endpoint_get_route(node->endpoints[i], dst, dstlen);
	fprintf(stderr, "%s\n", dst);
	/* fprintf(stderr, "\t->hash: %lu\n", api_hash_route(dst)); */
    }
    for (int i=0; i<node->num_children; i++) {
        api_node_print_all_routes(node->children[i]);
    }
}

void api_node_print_routes_with_values(APINode *node)
{
    int dstlen = 255;
    char dst[dstlen];
    char valdst[dstlen];
    for (int i=0; i<node->num_endpoints; i++) {
	api_endpoint_get_route(node->endpoints[i], dst, dstlen);
	ValType t;
	jdaw_val_to_str(valdst, dstlen, endpoint_safe_read(node->endpoints[i], &t), t, 5);
	fprintf(stderr, "%s %s\n", dst, valdst);
    }
    for (int i=0; i<node->num_children; i++) {
        api_node_print_routes_with_values(node->children[i]);
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

static void api_node_serialize_recursive(FILE *f, APINode *root, APINode *node)
{
    char dst[255];
    api_node_get_route(node, dst, 255);
    if (node->do_not_serialize) return;
    char buf[MAX_ROUTE_LEN];
    /* fprintf(stderr, "NODE %p, parent name? %s name %s, ep: %d, children: %d\n", node, node->parent->obj_name, node->obj_name, node->num_children, node->num_endpoints); */
    for (int i=0; i<node->num_endpoints; i++) {
	Endpoint *ep = node->endpoints[i];
	if (ep->do_not_serialize) continue;
	api_endpoint_get_route_until(ep, buf, MAX_ROUTE_LEN, root);
	/* fprintf(stderr, "SER ROUTE: %s\n", buf); */
	fprintf(f, "%s ", buf);
	/* fwrite(buf, 1, strlen(buf), f); */
	ValType t;
	Value val = endpoint_safe_read(ep, &t);
	jdaw_val_serialize(f, val, t);
	fputc('\n', f);
    }
    for (int i=0; i<node->num_children; i++) {
	api_node_serialize_recursive(f, root, node->children[i]);
    }


}
void api_node_serialize(FILE *f, APINode *root)
{
    /* Only serialize the root node name (only one name per serialized obj) */
    if (root->obj_name) {
	fprintf(f, "NAME");
	int16_t len = strlen(root->obj_name);
	int16_ser_le(f, &len);
	fprintf(f, "%s\n", root->obj_name);
    }

    api_node_serialize_recursive(f, root, root);
    fputc('\0', f);
}

/* Returns 0 on success, else error */
int api_node_deserialize(FILE *f, APINode *root)
{
    char keywd[4] = {0};
    fread(keywd, 1, 4, f);
    if (strncmp(keywd, "NAME", 4) == 0) {
	int16_t len = int16_deser_le(f);
	char name_buf[len + 1];
	fread(name_buf, 1, len, f);
	name_buf[len] = '\0';
	if (!root->obj_name) {
	    fprintf(stderr, "Error: deserializing node named \"%s\", but root has no name ptr\n", name_buf);
	    exit(1);
	}
	memcpy(root->obj_name, name_buf, len + 1);
	char c;
	fread(&c, 1, 1, f); /* Read the newline */
	if (c != '\n') {
	    fprintf(stderr, "Error: newline not present after API node name\n");
	    exit(1);
	}
	if (!root->fixed_name) {
	    api_node_renamed(root);
	}
    } else {
	fseek(f, -4, SEEK_CUR);
    }
    
    char route_buf[MAX_ROUTE_LEN];
    int partial_route_len = api_node_get_route(root, route_buf, MAX_ROUTE_LEN);
    int ret = 0;
    /* char val_buf[128]; */
    while (1) {
	int c;
	int i=partial_route_len;
	route_buf[i] = '\0';
	while ((c = fgetc(f)) != ' ' && c != '\0' && c != '\n') {
	    /* fprintf(stderr, "\tNEXT CHAR: %c\n", c); */
	    route_buf[i] = c;
	    i++;
	}
	route_buf[i] = '\0';
	ValType t;
	Value v = jdaw_val_deserialize(f, &t);

	/* DEBUG */
	/* char str_buf[255]; */
	/* jdaw_val_to_str(str_buf, 255, v, t, 4); */
	/* END DEBUG */
	
	Endpoint *ep = api_endpoint_get(route_buf);

	
	if (ep) {
	    /* DEBUG */
	    /* if (strcmp(ep->local_id, "sustain") == 0) { */
	    /* 	fprintf(stderr, "DESER %s == %s (ep %p)\n", route_buf, str_buf, ep); */
	    /* } */
	    /* END DEBUG */

	    endpoint_write(ep, v, true, true, true, false);
	} else {
	    ret++;
	    fprintf(stderr, "ERROR: API route node \"%s\" not found!\n", route_buf);
	    breakfn();
	}
	/* jdaw_val_to_str(route_buf, MAX_ROUTE_LEN, v, t, 3); */
	/* fprintf(stderr, "Write to %p, Val type %d == %s\n", ep, t, route_buf); */
	fgetc(f);
	c = fgetc(f);
	if (c == '\0') {
	    /* buf[0] = fgetc(f); */
	    /* buf[1] = fgetc(f); */
	    /* buf[2] = fgetc(f); */
	    /* buf[3] = fgetc(f); */
	    /* buf[4] = '\0'; */
	    /* fprintf(stderr, "done! next bytes: %s\n", buf); */
	    return ret;
	} else {
	    ungetc(c, f);
	}
    }
    return ret;
}
