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
struct sockaddr_in client, server;

typedef char frame_data[MAX_DATA_LENGTH];
int buffer_size;
frame_data *buf;

FILE *f;

uint32_t left, right;
bool *window_packet_mask;

uint32_t end_frame_seq_num;
uint32_t end_frame_data_len;
bool is_end_frame_received;

pthread_mutex_t lock;

void createSocket()
{
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
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
    uint32_t namelen = sizeof(server);
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
}

void sendACK(uint32_t sequence_number, bool is_acknowledged)
{
    ACK ack(sequence_number, is_acknowledged);
    sendto(s, ack.message, sizeof(ack.message), 0, (struct sockaddr *)&client, sizeof(client));
    printf("Send %s: %d\n", is_acknowledged ? "ACK" : "NAK", sequence_number);
}

int receivePacket()
{
    while (!is_end_frame_received)
    {
        char tmp[MAX_PACKET_SIZE];
        uint32_t client_address_size = sizeof(client);
        if (recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&client, &client_address_size) < 0)
        {
            if (left > end_frame_seq_num)
            {
                is_end_frame_received = true;
            }
            continue;
        }

        Packet packet(tmp);

        uint32_t seq = packet.getSequenceNumber();
        if (packet.checkChecksum())
        {
            printf("Received Packet: %d\n", seq);
            if (seq < right)
            {
                if (seq >= left)
                {
                    uint32_t length = packet.getDataLength();
                    memcpy(buf + seq % buffer_size, packet.getData(), length);

                    if (length < MAX_DATA_LENGTH) {
                        end_frame_seq_num = seq;
                        end_frame_data_len = length;
                        is_end_frame_received = true;
                    } else if (seq > end_frame_seq_num) {
                        end_frame_seq_num = seq;
                        end_frame_data_len = MAX_DATA_LENGTH;
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
            printf("Received bad frame\n");
            sendACK(seq, false);
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

    printf("\n");

    buffer_size = atoi(argv[3]);
    buf = new frame_data[buffer_size];

    int window_size = atoi(argv[2]);
    window_packet_mask = new bool[window_size]();

    left = 0;
    right = window_size;
    end_frame_seq_num = 0;
    is_end_frame_received = false;

    pthread_mutex_init(&lock, NULL);
    std::thread recv_packet(receivePacket);

    while (!(is_end_frame_received && left > end_frame_seq_num))
    {
        pthread_mutex_lock(&lock);

        if (window_packet_mask[0])
        {
            int shift;
            for (shift = 1; shift < window_size && window_packet_mask[shift]; shift++)
                ;
            for (int i = 0; i < window_size - shift; i++)
            {
                window_packet_mask[i] = window_packet_mask[i + shift];
            }
            for (int i = window_size - shift; i < window_size; i++)
            {
                window_packet_mask[i] = false;
            }

            for (int i = 0; i < shift; i++)
            {
                fwrite(buf + left % buffer_size, 1, is_end_frame_received && left == end_frame_seq_num ? end_frame_data_len : MAX_DATA_LENGTH, f);

                left = left + 1;
                right = left + window_size;
            }
            printf("SHIFTED Left: %d Right: %d\n", left, right);
        }

        pthread_mutex_unlock(&lock);
    }

    recv_packet.join();
    pthread_mutex_destroy(&lock);

    delete[] buf;
    delete[] window_packet_mask;

    fclose(f);

    dealocateSocket();
}