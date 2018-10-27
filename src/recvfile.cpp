#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    int s;
    uint32_t namelen, client_address_size;
    unsigned short port;
    struct sockaddr_in client, server;
    char buf[32];

    /* argv[4] is internet address of server argv[5] is port of server.
    * Convert the port from ascii to integer and then from host byte
    * order to network byte order.
    */
    if (argc != 4)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <port> \n", argv[0]);
        exit(1);
    }
    port = htons(atoi(argv[4]));

    /*
    * Create a datagram socket in the internet domain and use the
    * default protocol (UDP).
    */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }

    /*
    * Bind my name to this socket so that clients on the network can
    * send me messages. (This allows the operating system to demultiplex
    * messages and get them to the correct server)
    *
    * Set up the server name. The internet address is specified as the
    * wildcard INADDR_ANY so that the server can get messages from any
    * of the physical internet connections on this host. (Otherwise we
    * would limit the server to messages from only one network
    * interface.)
    */
    server.sin_family = AF_INET;         /* Server is in Internet Domain */
    server.sin_port = port;              /* Use this port                */
    server.sin_addr.s_addr = INADDR_ANY; /* Server's Internet Address    */

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(2);
    }

    /*
    * Receive a message on socket s in buf  of maximum size 32
    * from a client. Because the last two paramters
    * are not null, the name of the client will be placed into the
    * client data structure and the size of the client address will
    * be placed into client_address_size.
    */
    client_address_size = sizeof(client);

    if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&client, &client_address_size) < 0)
    {
        perror("recvfrom()");
        exit(4);
    }
    /*
    * Print the message and the name of the client.
    * The domain should be the internet domain (AF_INET).
    * The port is received in network byte order, so we translate it to
    * host byte order before printing it.
    * The internet address is received as 32 bits in network byte order
    * so we use a utility that converts it to a string printed in
    * dotted decimal format for readability.
    */
    printf("Received message %s from domain %s port %d internet address %s\n",
           buf,
           (client.sin_family == AF_INET ? "AF_INET" : "UNKNOWN"),
           ntohs(client.sin_port),
           inet_ntoa(client.sin_addr));

    /*
    * Deallocate the socket.
    */
    close(s);
}