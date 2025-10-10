#include "server.h"

short holding_registers[2000] = {0};
short input_registers[2000] = {0};
bool coils[2000] = {0};
bool discrete_inputs[2000] = {0};

int server_setup() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket fd
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Set socket options failed");
        exit(EXIT_FAILURE);
    }

    // Setup address (IPv4)
    address.sin_family = AF_INET;

    address.sin_addr.s_addr = INADDR_ANY;

    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 3) < 0) {
        perror("listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    return server_fd;
}

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
            if (coils[buff_recv[9] + (j+(i*8))] && (number_of_coils > 0)) {
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
            if (discrete_inputs[buff_recv[9] + (j+(i*8))] && (number_of_discretes > 0)) {
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
        packet->data[i*2] = (unsigned char)(MSBYTE(holding_registers[buff_recv[9]+i]));
        packet->data[(i*2)+1] = (unsigned char)(LSBYTE(holding_registers[buff_recv[9]+i]));
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
        packet->data[i*2] = (unsigned char)(MSBYTE(input_registers[buff_recv[9]+i]));
        packet->data[(i*2)+1] = (unsigned char)(LSBYTE(input_registers[buff_recv[9]+i]));
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
        holding_registers[(packet->written_address + i)] = TO_SHORT(buff_recv[13+(2*i)], buff_recv[14+(2*i)]);
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
                coils[(packet->written_address + (j+(i*8)))] = (buff_recv[13+i] >> j) & 0x01; // write j(th) bit of the byte to be written in client request
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