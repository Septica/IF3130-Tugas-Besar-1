#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ack.cpp"
#include "packet.cpp"

int s;
uint32_t namelen, client_address_size;
struct sockaddr_in client, server;
char *buf;
FILE *f;

int bufferSize;
uint32_t lastSequenceNumber;

void createSocket()
{
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }
}

void setupServer(unsigned short port)
{
    server.sin_family = AF_INET;
    server.sin_port = port;
    server.sin_addr.s_addr = INADDR_ANY;
}

void bindServer()
{
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(2);
    }
}

void findOutPort()
{
    namelen = sizeof(server);
    if (getsockname(s, (struct sockaddr *)&server, &namelen) < 0)
    {
        perror("getsockname()");
        exit(3);
    }
    printf("Port assigned is %d\n", ntohs(server.sin_port));
}

void dealocateSocket()
{
    close(s);
}

void prepareFile(char *fileName)
{
    printf("Opening '%s'...\n", fileName);
    f = fopen(fileName, "w");
    if (f == NULL)
    {
        printf("Failed. Exiting.\n");
        exit(0);
    }
    printf("Success\n");
    printf("\n");
}

void writePacket(char data[], uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        printf("%c\n", data[i]);
        fputc(data[i], f);
    }
}

int receivePacket(int lastACK)
{
    char tmp[MAX_PACKET_SIZE];
    client_address_size = sizeof(client);
    if (recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&client, &client_address_size) < 0)
    {
        perror("recvfrom()");
        exit(4);
    }

    Packet packet(tmp);
    if (packet.checkChecksum())
    {
        if (packet.getSOH())
        {
            if (packet.getSequenceNumber() == lastACK)
            {
                writePacket(packet.getData(), packet.getDataLength());
                return lastACK + 1;
            }
            else
            {
                return lastACK;
            }
        }
        else
        {
            return -1;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <port> \n", argv[0]);
        exit(1);
    }

    createSocket();
    setupServer(htons(atoi(argv[4])));
    bindServer();
    findOutPort();

    prepareFile(argv[1]);

    bufferSize = atoi(argv[3]);
    buf = new char[bufferSize * MAX_PACKET_SIZE];

    int lastACK = 0;
    uint32_t windowSize = atoi(argv[2]);

    while ((lastACK = receivePacket(lastACK)) > 0)
    {
        printf("Need Packet : %d\n\n", lastACK);
        ACK ack(lastACK, true);
        sendto(s, ack.message, 6, 0, (struct sockaddr *)&client, sizeof(client));
    }

    fclose(f);

    dealocateSocket();
}