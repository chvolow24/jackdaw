/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    api.h

    * create and maintain data structures related to UDP API
    * setup and teardown UDP server
    * triage messages sent via UDP

 *****************************************************************************************************************/

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef JDAW_API_H
#define JDAW_API_H

#define MAX_API_NODE_CHILDREN 255
#define MAX_API_NODE_ENDPOINTS 32

typedef struct endpoint Endpoint;
typedef struct api_node APINode;
typedef struct session Session;

typedef struct api_node {
    APINode *parent;
    APINode *children[MAX_API_NODE_CHILDREN];
    uint8_t num_children;
    Endpoint *endpoints[MAX_API_NODE_ENDPOINTS];
    uint8_t num_endpoints;
    char *obj_name;
    const char *fixed_name;
} APINode;

struct api_server {
    int port;
    bool active;
    APINode api_root;
    int sockfd;
    struct sockaddr_in servaddr;
    pthread_t thread_id;
    pthread_mutex_t setup_lock;
};

typedef struct api_hash_node {
    Endpoint *ep;
    struct api_hash_node *next;
    struct api_hash_node *prev;
    unsigned long index;
    char *route;
    bool deregistered;
} APIHashNode;

void api_endpoint_register(Endpoint *ep, APINode *parent);
Endpoint *api_endpoint_get(const char *route);
int api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size);
int api_node_get_route(APINode *node, char *dst, size_t dst_size);
int api_endpoint_get_route_until(Endpoint *ep, char *dst, size_t dst_size, APINode *until);
int api_endpoint_get_display_route_until(Endpoint *ep, char *dst, size_t dst_size, APINode *until);
void api_node_register(APINode *node, APINode *parent, char *obj_name, const char *fixed_name);
/* void api_node_register(APINode *node, APINode *parent, char *obj_name); */
void api_node_deregister(APINode *node);
void api_node_reregister(APINode *node);
/* void api_node_deregister(APINode *node); */
void api_node_renamed(APINode *node);
/* static void api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size); */

int api_start_server(int port);
    
void api_node_print_all_routes(APINode *node);
void api_node_print_routes_with_values(APINode *node);
void api_node_serialize(FILE *f, APINode *node);
/* void api_node_deserialize(FILE *f); */

/* Returns 0 on success, else error */
int api_node_deserialize(FILE *f, APINode *root);

void api_table_print();
void api_clear_all();
void api_quit();

/* Before swapping project, make room in session for new api structure */
void api_stash_current();
/* If proj read fails, use stashed info to reset api */
void api_reset_from_stash_and_discard();
/* Else if proj read successful, discard old api info */
void api_discard_stash();
/* void api_hash_table_destroy(); */

/* void api_node_renamed(APINode *api); */

#endif
