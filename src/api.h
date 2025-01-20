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
void api_node_register(APINode *node, APINode *parent, char *obj_name);
void api_node_renamed(APINode *node);
/* static void api_endpoint_get_route(Endpoint *ep, char *dst, size_t dst_size); */

int api_start_server(int port);
    
void api_node_print_all_routes(APINode *node);
void api_table_print();
void api_quit();
/* void api_hash_table_destroy(); */

/* void api_node_renamed(APINode *api); */

#endif
