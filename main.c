#include "server.h"

int main() {
    int server_fd = server_setup();
    int new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    for(int i = 0; i < 101; i++) {
        holding_registers[i] = i;
        input_registers[i] = 2*i;
        coils[i] = i%2;
        discrete_inputs[i+1] = i%2;
    }

    struct ModbusFrame packet;

    while(1){
        // Accept connections
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            printf("Connection failed\n");
        }
        else {
            printf("Connection accepted\n");
            while(1){
                // Allocate memory for client message buffer, since the data package length varies
                unsigned char *buff_recv = (unsigned char *)malloc(BUF_SIZE * sizeof(unsigned char));
                // Declaring pointer to server response buffer, which memory will be allocated later
                unsigned char *buff_sent;
                _ssize_t bytes_recv;

                if((bytes_recv = read(new_socket, buff_recv, BUF_SIZE)) > 0) {
                    // fills some of the response bytes according to client's request
                    packet = modbus_frame(buff_recv);

                    // total response message length
                    int size = packet.length + 6;

                    switch(packet.func_code) {
                        // Read operation buffer
                        case READ_COILS:
                        case READ_DISCRETE_INPUTS:
                        case READ_HOLDING_REGISTERS:
                        case READ_INPUT_REGISTERS:
                            buff_sent = read_response(packet, size);
                            break;
                        // Write operation buffer
                        case WRITE_COILS:
                        case WRITE_HOLDING_REGISTERS:
                            buff_sent = write_response(packet, size);
                            break;
                        // Illegal function code exception
                        default:
                            buff_sent = exception_response(packet, size);
                            break;
                    }
                    
                    _ssize_t res_size = packet.length + 6;

                    printf("Client message: 0x");
                    for(int i = 0; i < bytes_recv; i++){
                        printf("%02X ", (unsigned char)buff_recv[i]);
                    }
                    printf("\n");
                    printf("Server response: 0x");
                    for(int i = 0; i < res_size; i++){
                        printf("%02X ", (unsigned char)buff_sent[i]);
                    }
                    printf("\n");
                    send(new_socket, buff_sent, res_size, 0);
                    free(buff_sent);
                }
                else{
                    free(buff_recv);
                    printf("Connection lost...\n");
                    break;
                }
                free(buff_recv);
            }
        }
    }

    return 0;
}