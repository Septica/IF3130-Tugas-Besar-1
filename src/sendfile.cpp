#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>

#include "packet.cpp"
#include "ack.cpp"

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

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 999999;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
}

void bindSocket()
{
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
    server.sin_port = 0;                 /* Use any available port       */
    server.sin_addr.s_addr = INADDR_ANY; /* Server's Internet Address    */

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(2);
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

void sendPacket(Packet &packet)
{
    printf("Sending...\n");
    if (packet.getSOH()) {
        printf("Sequence Number: %d\n", packet.getSequenceNumber());
        printf("Data: ");
            for (int i = 0; i < packet.getDataLength(); i++)
                printf("%c", packet.getData()[i]);
    } else {
        printf("EOF");
    }
   
    printf("\n\n");
    /* Send the message in buf to the server */
    if (sendto(s, packet.message, packet.getDataLength() + 10, 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto()");
        exit(2);
    }
}

int fillBuffer(int n)
{
    printf("Filling buffer...\n");
    for (int i = 0; i < n; i++)
    {
        char data[MAX_DATA_LENGTH];
        int length;

        for (length = 0; length < MAX_DATA_LENGTH; length++)
        {
            int c = fgetc(f);
            if (c == EOF)
                break;
            data[length] = c;
        }

        if (length == 0)
        {
            return i;
        }

        Packet packet(data, length);
        memcpy(buf + i * MAX_PACKET_SIZE, packet.message, packet.getDataLength() + 10);
    }
    printf("\n");
    return n;
}

int32_t receiveACK(int32_t lastACK)
{
    char tmp[6];
    uint32_t server_address_size = sizeof(server);
    int received = recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&server, &server_address_size);
    if (received < 0)
    {
        return lastACK;
    }
    ACK ack(tmp);
    printf("Received %s with sequence number %d\n",
           ack.getACK() ? "ACK" : "NAK",
           ack.getSequenceNumber());
    return ack.getACK() ? ack.getSequenceNumber() : lastACK;
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
    bindSocket();
    setupServer(inet_addr(argv[4]), htons(atoi(argv[5])));

    int maxPacketsInBuffer = atoi(argv[3]) / MAX_PACKET_SIZE;

    buf = new char[maxPacketsInBuffer * MAX_PACKET_SIZE];

    printf("Opening '%s'...\n", argv[1]);
    f = fopen(argv[1], "r");
    if (f == NULL)
    {
        printf("Failed. Exiting.\n");
        exit(0);
    }
    printf("Success\n");

    printf("\n");

    uint32_t lastACK = 0;
    uint32_t windowSize = atoi(argv[2]);

    while (int n = fillBuffer(maxPacketsInBuffer))
    {
        printf("%d packet(s) in buffer\n\n", n);
        while (lastACK < Packet::nextSequenceNumber)
        {
            int offset = lastACK % maxPacketsInBuffer;

            for (int i = offset; i < offset + windowSize; i++)
            {
                if (i >= n)
                    break;
                Packet tmp(buf + i * MAX_PACKET_SIZE);
                sendPacket(tmp);
                //tmp.printMessage();
            }

            lastACK = receiveACK(lastACK);
            printf("\n");
        }

        Packet lastPacket(buf + (n - 1) * MAX_PACKET_SIZE);
        if (lastPacket.getDataLength() < MAX_DATA_LENGTH || n < maxPacketsInBuffer)
            break;

        printf("\n");
    }

    char end[10];
    for (int i = 0; i < 10; i++)
    {
        end[i] = 0;
    }
    Packet endPacket(end);
    sendPacket(endPacket);
}