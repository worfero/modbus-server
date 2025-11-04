#include "modbus.h"
#include "server.h"

struct Registers registers = {
    .HR = {0},
    .IR = {0},
    .CO = {0},
    .DI = {0}
};

void read_coils(struct ModbusFrame *packet, unsigned char *buff_recv) {
    // number of coils to be read is 11th byte of client request
    unsigned char number_of_coils = buff_recv[11];
    // data length is the division of the number of coils to be read by 8 (char size) rounded up
    packet->data_length = CEIL(number_of_coils, 8);
    // allocate memory for data section of the packet
    packet->data = (unsigned char *)calloc((packet->data_length), sizeof(unsigned char));
    // packet length is data section length plus the 3 previous bytes
    packet->length = packet->data_length + 3;
    // data fetched from desired coils
    for(int i=0; i < packet->data_length; i++) {
        for (int j = 0; j < 8; ++j) {
            if (registers.CO[buff_recv[9] + (j+(i*8))] && (number_of_coils > 0)) {
                // If the coil is true, set the bit on the data section array
                packet->data[i] |= (1 << j); 
            }
            if(number_of_coils > 0) {
                number_of_coils--;
            }
        }
    }
}

void read_discrete_inputs(struct ModbusFrame *packet, unsigned char *buff_recv) {
    // number of coils to be read is 11th byte of client request
    unsigned char number_of_discretes = buff_recv[11];
    // data length is the division of the number of coils to be read by 8 (char size) rounded up
    packet->data_length = CEIL(number_of_discretes, 8);
    // allocate memory for data section of the packet
    packet->data = (unsigned char *)calloc((packet->data_length), sizeof(unsigned char));
    // packet length is data section length plus the 3 previous bytes
    packet->length = packet->data_length + 3;
    // data fetched from desired coils
    for(int i=0; i < packet->data_length; i++) {
        for (int j = 0; j < 8; ++j) {
            if (registers.DI[buff_recv[9] + (j+(i*8))] && (number_of_discretes > 0)) {
                // If the coil is true, set the bit on the data section array
                packet->data[i] |= (1 << j); 
            }
            if(number_of_discretes > 0) {
                number_of_discretes--;
            }
        }
    }
}

void read_holding_registers(struct ModbusFrame *packet, unsigned char *buff_recv) {
    // data length is 2 times the number of registers (11th byte of client request)
    packet->data_length = buff_recv[11]*2;
    // allocate memory for data section of the packet
    packet->data = (unsigned char *)calloc((packet->data_length), sizeof(unsigned char));
    // packet length is data section length plus the 3 previous bytes
    packet->length = packet->data_length + 3;
    // data fetched from desired registers
    for(int i=0; i < packet->data_length/2; i++) {
        packet->data[i*2] = (unsigned char)(MSBYTE(registers.HR[buff_recv[9]+i]));
        packet->data[(i*2)+1] = (unsigned char)(LSBYTE(registers.HR[buff_recv[9]+i]));
    }
}

void read_input_registers(struct ModbusFrame *packet, unsigned char *buff_recv) {
    // data length is 2 times the number of registers (11th byte of client request)
    packet->data_length = buff_recv[11]*2;
    // allocate memory for data section of the packet
    packet->data = (unsigned char *)calloc((packet->data_length), sizeof(unsigned char));
    // packet length is data section length plus the 3 previous bytes
    packet->length = packet->data_length + 3;
    // data fetched from desired registers
    for(int i=0; i < packet->data_length/2; i++) {
        packet->data[i*2] = (unsigned char)(MSBYTE(registers.IR[buff_recv[9]+i]));
        packet->data[(i*2)+1] = (unsigned char)(LSBYTE(registers.IR[buff_recv[9]+i]));
    }
}

void write_holding_registers(struct ModbusFrame *packet, unsigned char *buff_recv) {
    // data length is always 4 bytes for write multiple holding registers. 2 for the starting address and 2 for the quantity
    packet->data_length = 4;
    // packet length is data section length plus the 3 previous bytes
    packet->length = packet->data_length + 3;
    // get address to be written from bytes 8 and 9 of client request
    packet->written_address = TO_SHORT(buff_recv[8], buff_recv[9]);
    // get quantity of written addresses in sequence from bytes 10 and 11 of client request
    packet->written_quantity = TO_SHORT(buff_recv[10], buff_recv[11]);
    // data to be written to desired registers
    for(int i=0; i < packet->written_quantity; i++) {
        registers.HR[(packet->written_address + i)] = TO_SHORT(buff_recv[13+(2*i)], buff_recv[14+(2*i)]);
    }
}

void write_coils(struct ModbusFrame *packet, unsigned char *buff_recv) {
    unsigned int number_of_bytes;

    // data length is always 4 bytes for write multiple holding registers. 2 for the starting address and 2 for the quantity
    packet->data_length = 4;
    // packet length is data section length plus the 3 previous bytes
    packet->length = packet->data_length + 3;
    // get address to be written from bytes 8 and 9 of client request
    packet->written_address = TO_SHORT(buff_recv[8], buff_recv[9]);
    // get quantity of written addresses in sequence from bytes 10 and 11 of client request
    packet->written_quantity = TO_SHORT(buff_recv[10], buff_recv[11]);
    // number of registers to be written is 11th byte of client request
    number_of_bytes = CEIL(packet->written_quantity, 8);
    // write coils logic
    for(int i=0; i < number_of_bytes; i++) { // byte count
        for(int j=0; j < 8; j++) { // bit count
            if((j+(i*8)) < packet->written_quantity) { // if there are still coils to be written, proceed
                registers.CO[(packet->written_address + (j+(i*8)))] = (buff_recv[13+i] >> j) & 0x01; // write j(th) bit of the byte to be written in client request
            }
            else { // if the number of coils to be written was reached, stop there and break
                break;
            }
        }
    }
}

void exception(struct ModbusFrame *packet, unsigned char *buff_recv) {
    // generate exception function code (FC + 128 according to modbus definition)
    packet->func_code = packet->func_code + 0x80;
    // exception code 01 - function code not found
    packet->exception = ILLEGAL_FC;
    // for exceptions, length is always 6
    packet->length = 6;

    printf("Illegal function code, request was not successful\n");
}

struct ModbusFrame modbus_frame(unsigned char *buff_recv) {
    struct ModbusFrame packet;
    // transaction ID = first two bytes of client request
    packet.transac_id = TO_SHORT(buff_recv[0], buff_recv[1]);
    // protocol ID is always zero for modbus
    packet.prot_id = 0;
    // server unit ID
    packet.unit_id = 1;
    // function code provided by the client's request 7th byte
    packet.func_code = buff_recv[7];

    switch(packet.func_code){
        case READ_COILS:
            read_coils(&packet, buff_recv);
            break;
        case READ_DISCRETE_INPUTS:
            read_discrete_inputs(&packet, buff_recv);
            break;
        case READ_HOLDING_REGISTERS:
            read_holding_registers(&packet, buff_recv);
            break;
        case READ_INPUT_REGISTERS:
            read_input_registers(&packet, buff_recv);
            break;
        case WRITE_COILS:
            write_coils(&packet, buff_recv);
            break;
        case WRITE_HOLDING_REGISTERS:
            write_holding_registers(&packet, buff_recv);
            break;
        default:
            exception(&packet, buff_recv);
            break;
    }

    return packet;
}

unsigned char *read_response(struct ModbusFrame packet, int size) {
    unsigned char *buffer = (unsigned char *)malloc(size * sizeof(unsigned char));
    // creating response message
    buffer[TRAN_ID_MSB] = (unsigned char)(LSBYTE(packet.transac_id));
    buffer[TRAN_ID_LSB] = (unsigned char)(MSBYTE(packet.transac_id));
    buffer[PROT_ID_MSB] = (unsigned char)(LSBYTE(packet.prot_id));
    buffer[PROT_ID_LSB] = (unsigned char)(MSBYTE(packet.prot_id));
    buffer[LENGTH_MSB] = (unsigned char)(LSBYTE(packet.length));
    buffer[LENGTH_LSB] = (unsigned char)(MSBYTE(packet.length));
    buffer[UNIT_ID] = packet.unit_id;
    buffer[F_CODE] = packet.func_code;
    buffer[DATA_LENGTH] = packet.data_length;
    for(int i=0; i < packet.data_length; i++) {
        buffer[DATA(i+9)] = packet.data[i];
    }
    free(packet.data);
    
    return buffer;
}

unsigned char *write_response(struct ModbusFrame packet, int size) {
    unsigned char *buffer = (unsigned char *)malloc(size * sizeof(unsigned char));
    // creating response message
    buffer[TRAN_ID_MSB] = (unsigned char)(LSBYTE(packet.transac_id));
    buffer[TRAN_ID_LSB] = (unsigned char)(MSBYTE(packet.transac_id));
    buffer[PROT_ID_MSB] = (unsigned char)(LSBYTE(packet.prot_id));
    buffer[PROT_ID_LSB] = (unsigned char)(MSBYTE(packet.prot_id));
    buffer[LENGTH_MSB] = (unsigned char)(LSBYTE(packet.length));
    buffer[LENGTH_LSB] = (unsigned char)(MSBYTE(packet.length));
    buffer[UNIT_ID] = packet.unit_id;
    buffer[F_CODE] = packet.func_code;
    buffer[ADDRESS_MSB] = (unsigned char)(LSBYTE(packet.written_address));
    buffer[ADDRESS_LSB] = (unsigned char)(MSBYTE(packet.written_address));
    buffer[QUANTITY_MSB] = (unsigned char)(LSBYTE(packet.written_quantity));
    buffer[QUANTITY_LSB] = (unsigned char)(MSBYTE(packet.written_quantity));

    return buffer;
}

unsigned char *exception_response(struct ModbusFrame packet, int size) {
    unsigned char *buffer = (unsigned char *)malloc(size * sizeof(unsigned char));
    // creating response message
    buffer[TRAN_ID_MSB] = (unsigned char)(LSBYTE(packet.transac_id));
    buffer[TRAN_ID_LSB] = (unsigned char)(MSBYTE(packet.transac_id));
    buffer[PROT_ID_MSB] = (unsigned char)(LSBYTE(packet.prot_id));
    buffer[PROT_ID_LSB] = (unsigned char)(MSBYTE(packet.prot_id));
    buffer[LENGTH_MSB] = (unsigned char)(LSBYTE(packet.length));
    buffer[LENGTH_LSB] = (unsigned char)(MSBYTE(packet.length));
    buffer[UNIT_ID] = packet.unit_id;
    buffer[F_CODE] = packet.func_code;
    buffer[EXCEPTION] = packet.exception;

    return buffer;
}

THREAD_FUNC client_connection(void *arg){
    socket_type client_socket = *(socket_type *)arg;
    free(arg);

    while(1){
        struct ModbusFrame packet;
        // Allocate memory for client message buffer, since the data package length varies
        unsigned char *buff_recv = (unsigned char *)malloc(BUF_SIZE * sizeof(unsigned char));
        // Declaring pointer to server response buffer, which memory will be allocated later
        unsigned char *buff_sent;
        ssize_t bytes_recv;

        if((bytes_recv = read_sck(client_socket, buff_recv, BUF_SIZE)) > 0) {
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
            
            ssize_t res_size = packet.length + 6;

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
            send(client_socket, (char *)buff_sent, res_size, 0);
            free(buff_sent);
        }
        else{
            free(buff_recv);
            printf("Connection lost...\n");
            CLOSESOCKET(client_socket);
            break;
        }
        free(buff_recv);
    }
    #ifndef _WIN32
        return NULL;
    #else
        return 0;
    #endif
}

void ModbusTCPServer(char *ip, int port) {
    int server_fd = server_setup(ip, port);
    while(1){
        struct sockaddr_in address;
        #ifdef _WIN32
            int addrlen = sizeof(address);
        #else
            socklen_t addrlen = sizeof(address);
        #endif

        socket_type *new_socket = malloc(sizeof(socket_type));

        // Accept connections
        if((*new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            printf("Connection failed\n");
            free(new_socket);
            continue;
        }
        else {
            printf("Connection accepted\n");
            THREAD_TYPE thread_id;
            #ifdef _WIN32
                thread_id = CreateThread(NULL, 0, client_connection, new_socket, 0, NULL);
                if (thread_id == NULL) {
                    printf("Failed to create thread\n");
                    CLOSESOCKET(*new_socket);
                    free(new_socket);
                } else {
                    CloseHandle(thread_id);
                }
            #else
                if (pthread_create(&thread_id, NULL, client_connection, new_socket) != 0) {
                    perror("pthread_create");
                    CLOSESOCKET(*new_socket);
                    free(new_socket);
                }
                pthread_detach(thread_id);
            #endif
        }
    }
    CLOSESOCKET(server_fd);
    #ifdef _WIN32
        WSACleanup();
    #endif
}