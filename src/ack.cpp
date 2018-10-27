#include <cstdint>
#include <cstring>

class ACK {
    public:
        ACK(uint32_t nextSequenceNumber) {
            setACK();
            setNextSequenceNumber(nextSequenceNumber);
            setChecksum(calculateChecksum());
        }

        char getACK() {
            return message[0];
        }

        uint32_t getNextSequenceNumber() {
            uint32_t nextSequenceNumber;
            memcpy(&nextSequenceNumber, message + 1, sizeof(nextSequenceNumber));
            return nextSequenceNumber;
        }

        char getChecksum() {
            return message[5];
        }

        void setACK() {
            message[0] = -1;
        }

        void setNextSequenceNumber(uint32_t& nextSequenceNumber) {
            memcpy(message + 1, &nextSequenceNumber, sizeof(nextSequenceNumber));
        }

        void setChecksum(char checksum) {
            memcpy(message + 5, &checksum, sizeof(checksum));
        }

        char calculateChecksum() {
            char sum = 0;
            for (int i = 0; i < 5; i++) {
                sum += message[i];
            }
            return sum;
        }

        bool checkChecksum() {
            return calculateChecksum() == getChecksum();
        }
        
        char message[6];
};