#include <cstdint>
#include <cstring>
#include <stdio.h>

class Packet {
    public:
        Packet(char data[], uint32_t length) {
            message = new char[10 + length];
            memcpy(message + 0, &SOH, sizeof(SOH));
            memcpy(message + 1, &nextSequenceNumber, sizeof(nextSequenceNumber));
            memcpy(message + 5, &length, sizeof(length));
            memcpy(message + 9, data, length);
            char checksum = calculateChecksum();
            memcpy(message + 9 + length, &checksum, sizeof(checksum));
        }
        
        uint32_t getSequenceNumber() {
            uint32_t sequenceNumber;
            memcpy(message + 1, &sequenceNumber, sizeof(sequenceNumber));
            return sequenceNumber;
        }

        uint32_t getDataLength() {
            uint32_t dataLength;
            memcpy(message + 5, &dataLength, sizeof(dataLength));
            return dataLength;
        }

        char* getData() {
            char* data = new char[getDataLength()];
            memcpy(message + 9, &data, getDataLength());
            return data;
        }

        char getChecksum() {
            uint32_t checksum;
            memcpy(message + 9 + getDataLength(), &checksum, sizeof(checksum));
            return checksum;
        }

        void printMessage() {
            for( int i = 0; i < 10 + getDataLength(); i++) {
                printf("%x", message[i]);
            }
            printf("\n");
        }

        const char SOH = 0x1;
        

    private:
        static uint32_t nextSequenceNumber;
        char* message;

        char calculateChecksum() {
            char sum = 0;
            for (int i = 0; i < 9; i++) {
                sum += message[i];
            }
            return sum;
        }
};