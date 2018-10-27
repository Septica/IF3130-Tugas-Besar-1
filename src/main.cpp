#include "packet.cpp"
#include "stdio.h"


uint32_t Packet::nextSequenceNumber = 0;

int main() {
    Packet test("b", 1);
    test.printMessage();
}