#include "modbus.h"

int main() {
    for(int i = 0; i < 101; i++) {
        registers.HR[i] = i;
        registers.IR[i] = 2*i;
        registers.CO[i] = i%2;
        registers.DI[i+1] = i%2;
    }

    ModbusTCPServer("127.0.0.1", 502);

    return 0;
}