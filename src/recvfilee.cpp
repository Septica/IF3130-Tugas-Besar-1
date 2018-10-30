#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <time.h>

#include "ack.cpp"
#include "packet.cpp"

int s; //This is the socket container
struct sockaddr_in client, server; //This is client and server address
FILE *f; //this is the file to be written
char* buf;
int bufferSize;

uint32_t left, right;
bool *window_message_mask;

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

void sendACK(){
    ACK ack(left, true);
    uint32_t serverAddressSize = sizeof(server);
    sendto(s, &ack, sizeof(ack), 0, (struct sockaddr*)&server, sizeof(server));
}

void receiveMessage()
{
    char tmp[MAX_PACKET_SIZE];
    uint32_t serverAddressSize = sizeof(server);
    recvfrom(s, tmp, sizeof(tmp), 0, (struct sockaddr*)&server, &serverAddressSize);
    Packet packet(tmp);
    if (packet.checkChecksum()){
        printf("Received good packet: %d", packet.getSequenceNumber());
        if (packet.getSequenceNumber() <= right){
            if (packet.getSequenceNumber()>=left){
                printf("Accepted\n");
                memcpy(buf+(packet.getSequenceNumber()%bufferSize),packet.getData(),packet.getDataLength());
                if(packet.getSOH()){
                    window_message_mask[packet.getSequenceNumber() - left] = true;
                }
                else{
                    window_message_mask[packet.getSequenceNumber() - left] = false;
                }
            }
        }
    }
    else {
        printf("Error Checksum \n");
    }
}

void writeToFile()
{
    printf("Writing to file...\n");
    fprintf(f, "%s", buf);
}

int main(int argc, char ** argv)
{
    //Check if args count not equals to 5
    if (argc != 5)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <port> \n", argv[0]);
        exit(1);
    }

    //Socket Setup
    createSocket();
    setupServer(htons(atoi(argv[4])));
    bindServer();

    //Preparing output file
    prepareFile(argv[1]);
    bufferSize = atoi(argv[3]);
    buf = new char[bufferSize * MAX_PACKET_SIZE];

    uint32_t windowSize = atoi(argv[2]);


    //Create window
    window_message_mask = new bool[windowSize];
    left = 0;
    right = windowSize-1;
    bool done = false;
    while (!done){
        receiveMessage();
        printf("Buf right now is: %s\n", buf);
        writeToFile();
        sendACK();
        left++;
        right++;
        if (buf == NULL){
            done = true;
        }
    }
    close(s);
    //Send ACK(N) if RequestNumber(N) got in
}