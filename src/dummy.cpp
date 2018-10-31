#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#include "ack.cpp"
#include "packet.cpp"

int s;
uint32_t namelen, client_address_size;
struct sockaddr_in client, server;
char *buf;
FILE *f;

int bufferSize;
uint32_t left, right;

bool *window_packet_mask;

uint32_t eofSeq;
uint32_t eofLength;
bool end = false;
pthread_mutex_t lock;

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
    f = fopen(fileName, "wb");
    if (f == NULL)
    {
        printf("Failed. Exiting.\n");
        exit(0);
    }
    printf("Success\n");
    printf("\n");
}

void sendACK(int sequenceNumber, bool isAcknowledged)
{
    ACK ack(sequenceNumber, true);
    sendto(s, ack.message, sizeof(ack.message), 0, (struct sockaddr *)&client, sizeof(client));
    printf("Send ACK: %d\n", sequenceNumber);
}

int receivePacket()
{
    while (!end)
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
            uint32_t seq = packet.getSequenceNumber();
            if (packet.getDataLength() > 0)
            {
                printf("Received Packet: %d\n", seq);
                if (seq < right)
                {
                    if (seq >= left)
                    {
                        uint32_t length = packet.getDataLength();
                        memcpy(buf + seq % bufferSize, packet.getData(), length);
                        if (length < MAX_DATA_LENGTH)
                        {
                            eofSeq = seq;
                            eofLength = length;
                            end = true;
                        }

                        pthread_mutex_lock(&lock);
                        window_packet_mask[seq - left] = true;
                        pthread_mutex_unlock(&lock);
                    }
                    sendACK(seq, true);
                }
            }
            else
            {
                printf("EOF\n");
                eofSeq = seq - 1;
                eofLength = MAX_DATA_LENGTH;
                end = true;
            }
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
    buf = new char[bufferSize * MAX_DATA_LENGTH];

    uint32_t windowSize = atoi(argv[2]);
    window_packet_mask = new bool[windowSize];
    for (int i = 0; i < windowSize; i++)
    {
        window_packet_mask[i] = false;
    }
    left = 0;
    right = windowSize;

    pthread_mutex_init(&lock, NULL);
    std::thread recv_packet(receivePacket);

    while (!end || left <= eofSeq)
    {
        pthread_mutex_lock(&lock);
        if (window_packet_mask[0])
        {
            int shift;
            for (shift = 1; window_packet_mask[shift] && shift < windowSize; shift++);
            for (int i = 0; i < windowSize; i++)
            {
                if (i < windowSize - shift)
                {
                    window_packet_mask[i] = window_packet_mask[i + shift];
                }
                else
                {
                    window_packet_mask[i] = false;
                }
                
            }

            for (int i = 0; i < shift; i++)
            {
                int length = end && eofSeq == left ? eofLength : MAX_DATA_LENGTH;
                fwrite(buf + left % bufferSize + i, 1, length, f);
                // for (int i = 0; i < length; i++) {
                //     fputc(buf[left % bufferSize + i], f);
                // }
                right = ++left + windowSize;
            }
            printf("SHIFTED Left: %d Right: %d\n", left, right);
        }
        pthread_mutex_unlock(&lock);
    }

    delete[] buf;
    delete[] window_packet_mask;

    recv_packet.join();

    pthread_mutex_destroy(&lock);

    fclose(f);

    dealocateSocket();
}