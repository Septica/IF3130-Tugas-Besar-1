#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "packet.cpp"
uint32_t Packet::nextSequenceNumber = 0;

int s;
struct sockaddr_in server;
char *buf;

FILE *f;

const int MAX_PACKET_SIZE = MAX_DATA_LENGTH + 10;

void createSocket()
{
    /* Create a datagram socket in the internet domain and use the
    * default protocol (UDP).
    */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }
}

void setupServer(in_addr_t addr, unsigned short port)
{
    /* Set up the server name */
    server.sin_family = AF_INET;   /* Internet Domain    */
    server.sin_addr.s_addr = addr; /* Server's Address   */
    server.sin_port = port;        /* Server Port        */
}

void dealocateSocket()
{
    /* Deallocate the socket */
    close(s);
}

void sendPacket(Packet *packet)
{
    packet->printMessage();
    /* Send the message in buf to the server */
    if (sendto(s, packet->message, (packet->getDataLength() + 10), 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto()");
        exit(2);
    }

    
}

int fillBuffer(int n)
{
    printf("Filling buffer...\n");
    for (int i = 0; i < n;) {
        printf("Iteration %d\n", i);

        bool last = false;
        char data[MAX_DATA_LENGTH];
        int length;

        for (length = 0; length < MAX_DATA_LENGTH; length++) {
            int c = fgetc(f);
            if (c == EOF) {
                last = true;
                break;
            }

            data[length] = c;
            printf("%c", data[length]);
        }
        printf("\n");

        Packet packet(data, length);
        printf("Copying...\n");
        packet.printMessage();
        memcpy(buf + i * MAX_PACKET_SIZE, &packet, packet.getDataLength());

        printf("try printing...\n");
        Packet *tmp = (Packet *)buf + i * MAX_PACKET_SIZE;
        tmp->printMessage();

        i++;

        if (last) {
            printf("EOF, %d packets\n", i);
            return length;
        }
    }
}

int main(int argc, char **argv)
{

    /* argv[4] is internet address of server argv[5] is port of server.
    * Convert the port from ascii to integer and then from host byte
    * order to network byte order.
    */
    if (argc != 6)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <destination_ip> <destination_port> \n", argv[0]);
        exit(1);
    }

    createSocket();
    setupServer(inet_addr(argv[4]), htons(atoi(argv[5])));

    int maxPacketsInBuffer = atoi(argv[3]) / MAX_PACKET_SIZE;

    buf = new char[maxPacketsInBuffer * MAX_PACKET_SIZE];

    FILE *fo;

    printf("Opening '%s'...\n", argv[1]);
    f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("Failed. Exiting.\n");
        exit(0);
    }
    printf("Success\n");
    
    printf("\n");

    while (int n = fillBuffer(maxPacketsInBuffer)) {
        for (int i = 0; i < n; i++) {
            printf("Sending packet %d...\n", i);
            Packet *tmp = (Packet *)buf + i * MAX_PACKET_SIZE;
            tmp->printMessage();
            sendPacket((Packet *)buf + i * MAX_PACKET_SIZE);
            printf("Success");
        }
    }
}