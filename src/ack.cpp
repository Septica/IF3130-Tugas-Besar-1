#include <cstdint>
#include <cstring>

class ACK {
    public:
        ACK(char rawData[]) {
            memcpy(message, rawData, 6);
        }

        ACK(uint32_t sequenceNumber, bool isAcknowledged) {
            setACK(isAcknowledged);
            setSequenceNumber(sequenceNumber);
            setChecksum(calculateChecksum());
        }

        char getACK() {
            return message[0];
        }

        uint32_t getSequenceNumber() {
            uint32_t sequenceNumber;
            memcpy(&sequenceNumber, message + 1, sizeof(sequenceNumber));
            return sequenceNumber;
        }

        char getChecksum() {
            return message[5];
        }

        void setACK(bool isAcknowledged) {
            message[0] = isAcknowledged;
        }

        void setSequenceNumber(uint32_t sequenceNumber) {
            memcpy(message + 1, &sequenceNumber, sizeof(sequenceNumber));
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