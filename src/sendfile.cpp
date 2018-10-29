#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <thread>

#include "packet.cpp"
#include "ack.cpp"

uint32_t Packet::nextSequenceNumber = 0;

int s;
struct sockaddr_in server;
char *buf;
FILE *f;

time_t timeout = 3;

uint32_t left, right;

bool *window_ack_mask, *window_sent_mask;
timespec *window_sent_time;

void createSocket()
{
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }
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

void receiveACK()
{
    char tmp[6];
    while (true)
    {
        uint32_t server_address_size = sizeof(server);
        recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&server, &server_address_size);
        ACK ack(tmp);

        if (ack.checkChecksum())
        {
            printf("Received good ACK: %d " , ack.getSequenceNumber());
            if (ack.getSequenceNumber() >= left && ack.getSequenceNumber() <= right)
            {
                printf("ACCEPTED\n");
                if (ack.getACK())
                {
                    window_ack_mask[ack.getSequenceNumber() - left] = true;
                }
                else
                {
                    window_sent_mask[ack.getSequenceNumber() - left] = false;
                }
            }
            else
            {
                printf("IGNORED\n");
            }
        }
        else
        {
            printf("ERROR CHECKSUM\n");
        }
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

    uint32_t windowSize = atoi(argv[2]);
    uint32_t left = 0, right = left + windowSize;

    while (int n = fillBuffer(bufferSize))
    {
        printf("%d packet(s) in buffer\n\n", n);

        window_ack_mask = new bool[windowSize];
        window_sent_time = new timespec[windowSize];
        window_sent_mask = new bool[windowSize];

        for (int i = 0; i < windowSize; i++)
        {
            window_ack_mask[i] = false;
            window_sent_mask[i] = false;
        }

        std::thread recv_ack(receiveACK);

        while (true)
        {
            if (window_ack_mask[0])
            {
                int shift = 0;
                for (int i = 1; i < windowSize; i++)
                {
                    shift++;
                    if (!window_ack_mask[i])
                        break;
                }
                for (int i = 0; i < windowSize - shift; i++)
                {
                    window_ack_mask[i] = window_ack_mask[i + shift];
                    window_sent_time[i] = window_sent_time[i + shift];
                    window_sent_mask[i] = window_sent_mask[i + shift];
                }
                for (int i = windowSize - shift; i < windowSize; i++)
                {
                    window_sent_mask[i] = false;
                    window_ack_mask[i] = false;
                }
            }

            for (int i = 0; i < windowSize; i++)
            {
                if (i + left >= n)
                    break;

                timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                if (!window_sent_mask[i] ||
                    !window_ack_mask[i] && now.tv_sec - window_sent_time[i].tv_sec > 2)
                {
                    Packet tmp(buf + (i + left) * MAX_PACKET_SIZE);
                    sendPacket(tmp);

                    window_sent_mask[i] = true;
                    clock_gettime(CLOCK_MONOTONIC, &window_sent_time[i]);
                }
            }
        }

        delete[] window_ack_mask;
        delete[] window_sent_time;
        delete[] window_sent_mask;
        recv_ack.join();
        Packet lastPacket(buf + (n - 1) * MAX_PACKET_SIZE);
        if (lastPacket.getDataLength() < MAX_DATA_LENGTH || n < bufferSize)
            break;

        printf("\n");
    }

    delete[] buf;

    char end[10] = {};
    Packet endPacket(end);
    sendPacket(endPacket);
}