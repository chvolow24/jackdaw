#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "::1" // Use IPv6 loopback address

int send_message_udp(char *message, int port) {
    int sockfd;
    struct sockaddr_in6 servaddr;
    /* const char *message = "hello"; // Message to send to Pure Data */

    // Create an IPv6 UDP socket
    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, SERVER_IP, &servaddr.sin6_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sockfd);
	return 1;
    }

    // Send message to Pd
    if (sendto(sockfd, message, strlen(message), 0, 
               (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("sendto failed");
        close(sockfd);
	return 1;
    }

    printf("Message sent: %s\n", message);
    close(sockfd);
    return 0;
}
