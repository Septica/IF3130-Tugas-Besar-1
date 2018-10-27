#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int s;
    unsigned short port;
    struct sockaddr_in server;
    char buf[32];

    /* argv[4] is internet address of server argv[5] is port of server.
    * Convert the port from ascii to integer and then from host byte
    * order to network byte order.
    */
    if (argc != 6)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <destination_ip> <destination_port> \n", argv[0]);
        exit(1);
    }
    port = htons(atoi(argv[5]));

    /* Create a datagram socket in the internet domain and use the
    * default protocol (UDP).
    */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }

    /* Set up the server name */
    server.sin_family = AF_INET;                 /* Internet Domain    */
    server.sin_port = port;                      /* Server Port        */
    server.sin_addr.s_addr = inet_addr(argv[4]); /* Server's Address   */

    strcpy(buf, "Hello");

    /* Send the message in buf to the server */
    if (sendto(s, buf, (strlen(buf) + 1), 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto()");
        exit(2);
    }

    /* Deallocate the socket */
    close(s);
}