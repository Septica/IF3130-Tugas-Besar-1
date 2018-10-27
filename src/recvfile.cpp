#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "packet.cpp"
#include "ack.cpp"
#define H_ACK 1
#define H_NAK 0
#define H_PACK 1
#define H_EOF 0
#define MAX_BUFFER_SIZE 11000

const int MAX_PACKET_SIZE = MAX_DATA_LENGTH + 10;

uint32_t nextSequenceNumber = 0;

bool done = false;
int s;
uint32_t namelen, client_address_size;
unsigned short port;
struct sockaddr_in client, server;
char *buf;
int windowsize, buffersize, maxPacketsInBuffer,idxbuf=0;

FILE *f;

void createSocket(){
    
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket()");
        exit(1);
    }
}

void setupServer(unsigned short port){
    
    server.sin_family = AF_INET;         /* Server is in Internet Domain */
    server.sin_port = port;              /* Use this port                */
    server.sin_addr.s_addr = INADDR_ANY; /* Server's Internet Address    */
}

void bindServer(){
    
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind()");
        exit(2);
    }
    
}

void dealocateSocket(){
    close(s);
}


void sendACK(ACK& ack)
{
    if (sendto(s, ack.message, sizeof(ack), 0, (struct sockaddr *)&client, sizeof(client)) < 0)
    {
        perror("sendto()");
        exit(2);
    }
}

void sendWrongACK(){
    ACK desiredACK(nextSequenceNumber,true);
    sendACK(desiredACK);
}

void processMessage(){
    printf("masuk processmessage\n");

    client_address_size = sizeof(client);
    buf = new char[1034];
    

    if (recvfrom(s,buf, 1034, 0, (struct sockaddr *)&client, &client_address_size) < 0)
    {
        perror("recvfrom()");
        exit(4);
    }

    Packet currPacket(buf);
    
    if(currPacket.getSequenceNumber() != nextSequenceNumber){
        sendWrongACK();
        return;
    }
    printf("getdata :\n");
    currPacket.printMessage();
    

    printf("tengah\n");
    char* test = currPacket.getData();
    printf("%d\n", currPacket.getDataLength());
    for (int i = 0; i < currPacket.getDataLength(); i++) {
        printf("%c\n", test[i]);
    }
    // printf("sebelum\n");
    // memcpy(buffer,currPacket.getData(),currPacket.getDataLength());
    // printf("tengah\n");
    // buffer[currPacket.getDataLength()]='\0';
    // printf("buffer : %s\n",buffer);
    //fprintf(f,buffer);
    printf("asdfasdfasdfasdfasdfasdfasdfasd\n");
    if(currPacket.getSOH()==H_EOF){
        done = true;
    }
    
}

int main(int argc, char *argv[])
{

    if (argc != 5)
    {
        printf("Usage: %s <filename> <windowsize> <buffersize> <port> \n", argv[0]);
        exit(1);
    }
    windowsize = atoi(argv[2]);
    buffersize = atoi(argv[3]);
    maxPacketsInBuffer = buffersize/MAX_PACKET_SIZE;
    buf = new char[maxPacketsInBuffer*MAX_PACKET_SIZE];

    createSocket();
    setupServer(htons(atoi(argv[4])));
    bindServer();

    f = fopen(argv[1],"w");
    if (f==NULL){
        printf("Failed to write file. Exit\n");
        exit(0);
    }

    while(!done){
        
        processMessage();
    }
    
    
}