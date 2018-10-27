#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ack.cpp"

int main(int argc, char *argv[])
{
    int s;
    uint32_t namelen, client_address_size;
    unsigned short port;
    struct sockaddr_in client, server;
    char buf[32];

    if (false)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <port> \n", argv[0]);
        exit(1);
    }
    port = htons(atoi(argv[4]));

    
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }

    server.sin_family = AF_INET;         /* Server is in Internet Domain */
    server.sin_port = 40000;              /* Use this port                */
    server.sin_addr.s_addr = INADDR_ANY; /* Server's Internet Address    */

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(2);
    }

    /* Find out what port was really assigned and print it */
   namelen = sizeof(server);
   if (getsockname(s, (struct sockaddr *) &server, &namelen) < 0)
   {
       perror("getsockname()");
       exit(3);
   }

   printf("Port assigned is %d\n", ntohs(server.sin_port));

    
    client_address_size = sizeof(client);

    while (true) {
        if (recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&client, &client_address_size) < 0)
        {
            perror("recvfrom()");
            exit(4);
        }
        int32_t seq;
        memcpy(&seq, buf + 1, 4);
        ACK ack(seq, true);

        sendto(s, ack.message, 6, 0, (struct sockaddr *)&client, sizeof(client));
        printf("SEQ: %d\n", seq);

        printf("Received message %s from domain %s port %d internet address %s\n",
            buf,
            (client.sin_family == AF_INET ? "AF_INET" : "UNKNOWN"),
            ntohs(client.sin_port),
            inet_ntoa(client.sin_addr));

    }
    
    /*
    * Deallocate the socket.
    */
    close(s);
}