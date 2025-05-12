/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    api.h

    * typedefs supporting jackdaw UDP API
 *****************************************************************************************************************/

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef JDAW_API_H
#define JDAW_API_H

#define MAX_API_NODE_CHILDREN 255
#define MAX_API_NODE_ENDPOINTS 32

typedef struct endpoint Endpoint;
typedef struct api_node APINode;
typedef struct project Project;

typedef struct api_node {
    APINode *parent;
    APINode *children[MAX_API_NODE_CHILDREN];
    uint8_t num_children;
    Endpoint *endpoints[MAX_API_NODE_ENDPOINTS];
    uint8_t num_endpoints;
    char *obj_name;
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
} APIHashNode;

void api_endpoint_register(Endpoint *ep, APINode *parent);
Endpoint *api_endpoint_get(const char *route);
int api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size);
void api_node_register(APINode *node, APINode *parent, char *obj_name);
void api_node_renamed(APINode *node);
/* static void api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size); */

int api_start_server(Project *proj, int port);
    
void api_node_print_all_routes(APINode *node);
void api_node_print_routes_with_values(APINode *node);
void api_table_print();
void api_quit(Project *proj);
/* void api_hash_table_destroy(); */

/* void api_node_renamed(APINode *api); */

#endif
