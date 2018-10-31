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

bool end;

pthread_mutex_t lock;

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
    if (packet.getDataLength() > 0)
    {
        printf("Sequence Number: %d\n", packet.getSequenceNumber());
        printf("Data: \n");
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
        if (feof(f))
            return i;
        char data[MAX_DATA_LENGTH];
        int length = fread(data, 1, MAX_DATA_LENGTH, f);
        printf("Length: %d\n", length);
        Packet packet(data, length);
        memcpy(buf + i * MAX_PACKET_SIZE, packet.message, packet.getDataLength() + 10);
    }
    return n;
}

void receiveACK()
{
    char tmp[6];
    while (!end)
    {
        uint32_t server_address_size = sizeof(server);
        recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&server, &server_address_size);
        ACK ack(tmp);

        if (ack.checkChecksum())
        {
            uint32_t seq = ack.getSequenceNumber();
            printf("Received good ACK: %d ", seq);
            if (seq >= left && seq < right)
            {
                printf("ACCEPTED\n");
                pthread_mutex_lock(&lock);
                if (ack.getACK())
                {
                    window_ack_mask[seq - left] = true;
                }
                else
                {
                    window_sent_mask[seq - left] = false;
                }
                pthread_mutex_unlock(&lock);
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

void prepareFile(char *filename)
{
    printf("Opening '%s'...\n", filename);
    f = fopen(filename, "rb");
    if (f == NULL)
    {
        printf("Failed. Exiting.\n");
        exit(0);
    }
    printf("Success\n");
}

void sendEOF()
{
    Packet endPacket(NULL, 0);
    sendPacket(endPacket);
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
    prepareFile(argv[1]);

    printf("\n");

    int bufferSize = atoi(argv[3]);
    buf = new char[bufferSize * MAX_PACKET_SIZE];

    uint32_t windowSize = atoi(argv[2]);

    window_ack_mask = new bool[windowSize];
    window_sent_mask = new bool[windowSize];
    timespec *window_sent_time = new timespec[windowSize];

    end = false;
    pthread_mutex_init(&lock, NULL);
    std::thread recv_ack(receiveACK);

    left = 0;
    right = left + windowSize;
    while (int n = fillBuffer(bufferSize))
    {
        printf("%d packet(s) in buffer\n\n", n);

        pthread_mutex_lock(&lock);
        for (int i = 0; i < windowSize; i++)
        {
            window_ack_mask[i] = false;
            window_sent_mask[i] = false;
        }
        pthread_mutex_unlock(&lock);

        while (left < Packet::nextSequenceNumber)
        {
            if (window_ack_mask[0])
            {
                pthread_mutex_lock(&lock);
                int shift;
                for (shift = 1; window_ack_mask[shift] && shift < windowSize; shift++);
                for (int i = 0; i < windowSize; i++)
                {
                    
                    if (i < windowSize - shift)
                    {
                        window_ack_mask[i] = window_ack_mask[i + shift];
                        window_sent_time[i] = window_sent_time[i + shift];
                        window_sent_mask[i] = window_sent_mask[i + shift];
                    }
                    else
                    {
                        window_sent_mask[i] = false;
                        window_ack_mask[i] = false;
                    }
                    
                }
                left += shift;
                right = left + windowSize;
                printf("SHIFTED Left: %d Right %d\n", left, right);
                pthread_mutex_unlock(&lock);
                
                if (left % bufferSize == 0)
                    break;
            }
            

            for (int i = 0; i < windowSize; i++)
            {
                if (i + left % bufferSize >= n)
                    break;

                timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                pthread_mutex_lock(&lock);
                if (!window_sent_mask[i] ||
                    !window_ack_mask[i] && now.tv_sec - window_sent_time[i].tv_sec > 2)
                {
                    Packet tmp(buf + (i + left % bufferSize) * MAX_PACKET_SIZE);
                    sendPacket(tmp);

                    window_sent_mask[i] = true;
                    clock_gettime(CLOCK_MONOTONIC, &window_sent_time[i]);
                }
                pthread_mutex_unlock(&lock);
            }
        }
        printf("\n");
    }

    delete[] window_ack_mask;
    delete[] window_sent_time;
    delete[] window_sent_mask;

    sendEOF();

    end = true;
    recv_ack.join();
    pthread_mutex_destroy(&lock);
    delete[] buf;
}