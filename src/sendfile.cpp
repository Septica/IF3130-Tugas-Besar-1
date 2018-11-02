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
time_t timeout = 3;

int s;                     //Socket connection number
struct sockaddr_in server; //Server Address type

typedef char packet[MAX_PACKET_SIZE]; //Packet to be sent
packet *buf;                          //Buffer memory

FILE *f; //File to be sent

uint32_t left, right;
bool *window_ack_mask, *window_sent_mask; //Window boolean true if sent/received, false if not yet sent/received

bool lastPacket, noResponse;

pthread_mutex_t lock;

// Membuat socket dan setting timeoutnya
void createSocket()
/* AF_INET as domain, SOCK_DGRAM as socket type for UDP connection */
{
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;

    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}

// Bind socket ke client
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

// Menyiapkan variabel server
void setupServer(in_addr_t addr, unsigned short port)
{
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = addr;
    server.sin_port = port;
}

// Menutup socket
void dealocateSocket()
{
    close(s);
}

// Mengirimkan packet dan print informasi packet
void sendPacket(Packet &packet)
{
    printf("Sending...\n");
    printf("Sequence Number: %d\n", packet.getSequenceNumber());
    printf("Data: \n");
    if (packet.getDataLength() > 8)
    {
        printf("Too big to be displayed");
    }
    else
    {
        for (int i = 0; i < packet.getDataLength(); i++)
        {
            printf("%c", packet.getData()[i]);
        }
    }
    printf("\n");
    printf("Checksum: %x\n", packet.getChecksum());
    printf("\n");

    if (sendto(s, packet.message, packet.getDataLength() + 10, 0, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("sendto()");
        exit(3);
    }
}

// Mengisi buffer sebanyak maksimal n paket
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
        memcpy(buf + i, packet.message, packet.getDataLength() + 10);

        if (length < MAX_DATA_LENGTH) {
            lastPacket = true;
        } 
    }
    return n;
}

// Loop untuk menerima ACK dan menandai packet yang di-ACK
void receiveACK()
{
    while (!(noResponse && lastPacket))
    {
        char tmp[6];
        uint32_t server_address_size = sizeof(server);
        if (recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr *)&server, &server_address_size) < 0) {
            noResponse = true;
            continue;
        }

        ACK ack(tmp);

        if (ack.checkChecksum())
        {
            uint32_t seq = ack.getSequenceNumber();
            printf("Received ACK: %d ", seq);
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
            printf("ACK Error\n");
        }
    }
}

// Membuka file
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

    int buffer_size = atoi(argv[3]);
    buf = new packet[buffer_size];

    int window_size = atoi(argv[2]);
    window_ack_mask = new bool[window_size]();
    window_sent_mask = new bool[window_size]();
    timespec *window_sent_time = new timespec[window_size];

    left = 0;
    right = window_size;
    noResponse = false;
    lastPacket = false;

    pthread_mutex_init(&lock, NULL);
    std::thread recv_ack(receiveACK);

    do {
        int n = fillBuffer(buffer_size);

        printf("%d packet(s) in buffer\n\n", n);

        // Loop selama semua packet dalam buffer belum terkirim
        while (left < Packet::nextSequenceNumber)
        {
            pthread_mutex_lock(&lock);

            if (window_ack_mask[0])
            {
                // Slide window
                int shift;
                for (shift = 1; window_ack_mask[shift] && shift < window_size; shift++)
                    ;
                for (int i = 0; i < window_size; i++)
                {

                    if (i < window_size - shift)
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
                right = left + window_size;
                printf("SHIFTED Left: %d Right %d\n", left, right);

                // Semua paket sudah terkirim
                if (left % buffer_size == 0)
                {
                    pthread_mutex_unlock(&lock);
                    break;
                }
            }

            pthread_mutex_unlock(&lock);

            // Kirim packet dalam window jika kondisi terpenuhi
            for (int i = 0; i < window_size; i++)
            {
                if (left % buffer_size + i >= n)
                    break;

                timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);

                pthread_mutex_lock(&lock);

                if (!window_sent_mask[i] || !window_ack_mask[i] && now.tv_sec - window_sent_time[i].tv_sec > 2)
                {
                    Packet tmp(buf[left % buffer_size + i]);
                    sendPacket(tmp);

                    window_sent_mask[i] = true;
                    clock_gettime(CLOCK_MONOTONIC, &window_sent_time[i]);
                }

                pthread_mutex_unlock(&lock);
            }
            
            if (noResponse && lastPacket) break;
        }
        printf("\n");
    } while (!lastPacket);

    printf("End\n");

    recv_ack.join();
    pthread_mutex_destroy(&lock);

    delete[] buf;
    delete[] window_ack_mask;
    delete[] window_sent_time;
    delete[] window_sent_mask;

    fclose(f);

    dealocateSocket();
}
