#include <cstdint>
#include <cstring>
#include <stdio.h>

const uint32_t MAX_DATA_LENGTH = 3;
class Packet {
    public:
        Packet(char rawMessage[]) {
            message = rawMessage;
        }

        Packet(char data[], uint32_t length) {
            message = new char[10 + length];
            
            setSOH();
            setSequenceNumber();
            setDataLength(length);
            setData(data);
            setChecksum(calculateChecksum());
        }

        char getSOH() {
            return message[0];
        }
        
        uint32_t getSequenceNumber() {
            uint32_t sequenceNumber;
            memcpy(&sequenceNumber, message + 1, sizeof(sequenceNumber));
            return sequenceNumber;
        }

        uint32_t getDataLength() {
            uint32_t dataLength;
            memcpy(&dataLength, message + 5, sizeof(dataLength));
            return dataLength;
        }

        char* getData() {
            char* data = new char[getDataLength()];
            memcpy(&data, message + 9, getDataLength());
            return data;
        }

        char getChecksum() {
            uint32_t checksum;
            memcpy(&checksum, message + 9 + getDataLength(), sizeof(checksum));
            return checksum;
        }

        void setSOH() {
            memcpy(message + 0, &SOH, sizeof(SOH));
        }

        void setSequenceNumber() {
            memcpy(message + 1, &nextSequenceNumber, sizeof(nextSequenceNumber));
            nextSequenceNumber++;
        }

        void setDataLength(const uint32_t& dataLength) {
            memcpy(message + 5, &dataLength, sizeof(dataLength));
        } 

        void setData(char data[]) {
            memcpy(message + 9, data, getDataLength());
        }

        void setChecksum(char checksum) {
            memcpy(message + 9 + getDataLength(), &checksum, sizeof(checksum));
        }

        char calculateChecksum() {
            char sum = 0;
            for (int i = 0; i < 9 + getDataLength(); i++) {
                sum += message[i];
            }
            return sum;
        }

        bool checkChecksum() {
            return calculateChecksum() == getChecksum();
        }

        void printMessage() {
            for(int i = 0; i < 10 + getDataLength(); i++) {
                printf("Byte %d: %x\n", i, message[i]);
            }
        }

        const char SOH = 0x1;
        char* message;
    private:
        static uint32_t nextSequenceNumber;
        
};
