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
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }

    struct timeval read_timeout;
    read_timeout.tv_sec = 5;
    read_timeout.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
}

void bindClient()
{
    struct sockaddr_in client;

    client.sin_family = AF_INET;
    client.sin_port = 0;
    client.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&client, sizeof(client)) < 0)
    {
        perror("bind()");
        exit(2);
    }
}

void setupServer(in_addr_t addr, unsigned short port)
{
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = addr;
    server.sin_port = port;
}

void dealocateSocket()
{
    close(s);
}

void sendPacket(Packet &packet)
{
    printf("Sending...\n");
    if (packet.getSOH())
    {
        printf("Sequence Number: %d\n", packet.getSequenceNumber());
        printf("Data: ");
        for (int i = 0; i < packet.getDataLength(); i++)
            printf("%c", packet.getData()[i]);
        printf("\n");
        printf("Checksum: %x\n", packet.getChecksum());
    }
    else
    {
        printf("EOF");
    }
    printf("\n");

    if (sendto(s, packet.message, packet.getDataLength() + 10, 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto()");
        exit(3);
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
            return i;

        Packet packet(data, length);
        memcpy(buf + i * MAX_PACKET_SIZE, packet.message, packet.getDataLength() + 10);
    }
    return n;
}

int32_t receiveACK(int32_t lastACK)
{
    char tmp[6];
    uint32_t server_address_size = sizeof(server);
    if (recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&server, &server_address_size) < 0)
    {
        return lastACK;
    }
    ACK ack(tmp);
    if (ack.checkChecksum())
    {
        printf("Received %s with sequence number %d\n",
               ack.getACK() ? "ACK" : "NAK",
               ack.getSequenceNumber());
        return ack.getACK() ? ack.getSequenceNumber() : lastACK;
    }
    else
    {
        return lastACK;
    }
}

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <destination_ip> <destination_port> \n", argv[0]);
        exit(1);
    }

    createSocket();
    bindClient();
    setupServer(inet_addr(argv[4]), htons(atoi(argv[5])));

    printf("Opening '%s'...\n", argv[1]);
    f = fopen(argv[1], "r");
    if (f == NULL)
    {
        printf("Failed. Exiting.\n");
        exit(0);
    }
    printf("Success\n");
    printf("\n");

    int bufferSize = atoi(argv[3]);
    buf = new char[bufferSize * MAX_PACKET_SIZE];

    uint32_t lastACK = 0;
    uint32_t windowSize = atoi(argv[2]);

    while (int n = fillBuffer(bufferSize))
    {
        printf("%d packet(s) in buffer\n\n", n);
        while (lastACK < Packet::nextSequenceNumber)
        {
            int offset = lastACK % bufferSize;

            for (int i = offset; i < offset + windowSize; i++)
            {
                if (i >= n)
                    break;
                Packet tmp(buf + i * MAX_PACKET_SIZE);
                sendPacket(tmp);
            }

            lastACK = receiveACK(lastACK);
            printf("\n");
        }

        Packet lastPacket(buf + (n - 1) * MAX_PACKET_SIZE);
        if (lastPacket.getDataLength() < MAX_DATA_LENGTH || n < bufferSize)
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