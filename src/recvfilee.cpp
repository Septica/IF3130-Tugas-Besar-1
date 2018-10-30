#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "ack.cpp"
#include "packet.cpp"

int s; //This is the socket container
struct sockaddr_in client, server; //This is client and server address
FILE *f; //this is the file to be written
char* buf

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

void receiveMessage()
{
    serverAddressSize = sizeof(server);
    recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&server);
    Packet packet(buf);
    if (packet.checkChecksum()){
        
    }
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

    while(1){
        //Create window

        //Send ACK(N) if RequestNumber(N) got in
    }
}