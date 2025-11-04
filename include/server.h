#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define PORT 502
#define BUF_SIZE 128

// function codes
#define READ_COILS 0x01
#define READ_DISCRETE_INPUTS 0x02
#define READ_HOLDING_REGISTERS 0x03
#define READ_INPUT_REGISTERS 0x04
#define WRITE_SINGLE_COIL 0x05
#define WRITE_SINGLE_HOLDING_REGISTER 0x06
#define WRITE_COILS 0x0F
#define WRITE_HOLDING_REGISTERS 0x10

// message bytes
#define TRAN_ID_MSB 0
#define TRAN_ID_LSB 1
#define PROT_ID_MSB 2
#define PROT_ID_LSB 3
#define LENGTH_MSB 4
#define LENGTH_LSB 5
#define UNIT_ID 6
#define F_CODE 7
// exception byte
#define EXCEPTION 8
// data length for reading registers byte
#define DATA_LENGTH 8
// address of registers to be written bytes
#define ADDRESS_MSB 8
#define ADDRESS_LSB 9
// quantity of registers to be written in sequence bytes
#define QUANTITY_MSB 10
#define QUANTITY_LSB 11
// data to be returned to client on reading operations, variable size
#define DATA(x) (x)

// exception codes
#define ILLEGAL_FC 0x01

// most significant and less significant byte macros
#define MSBYTE(x) ((x >> 8) & 0xFF)
#define LSBYTE(x) ((x) & 0xFF)

// round division up macro
#define CEIL(x, y) ((x + y - 1) / y)

// two char to short conversion macro
#define TO_SHORT(x, y) (((short)x) << 8) | y

struct ModbusFrame {
    // MBAP Header
    short transac_id;
    short prot_id;
    short length;
    unsigned char unit_id;
    // Application layer
    unsigned char func_code;
    short written_address;
    short written_quantity;
    unsigned char data_length;
    unsigned char exception;
    unsigned char *data;
};

struct Registers {
    short HR[2000];
    short IR[2000];
    bool CO[2000];
    bool DI[2000];
};

extern struct Registers registers;

//// declare registers as global variables
//extern short holding_registers[2000];
//extern short input_registers[2000];
//extern bool coils[2000];
//extern bool discrete_inputs[2000];

int server_setup(char *ip, int port);

void read_coils(struct ModbusFrame *packet, unsigned char *buff_recv);

void read_discrete_inputs(struct ModbusFrame *packet, unsigned char *buff_recv);

void read_holding_registers(struct ModbusFrame *packet, unsigned char *buff_recv);

void read_input_registers(struct ModbusFrame *packet, unsigned char *buff_recv);

void write_holding_registers(struct ModbusFrame *packet, unsigned char *buff_recv);

void write_coils(struct ModbusFrame *packet, unsigned char *buff_recv);

void exception(struct ModbusFrame *packet, unsigned char *buff_recv);

struct ModbusFrame modbus_frame(unsigned char *buff_recv);

unsigned char *read_response(struct ModbusFrame packet, int size);

unsigned char *write_response(struct ModbusFrame packet, int size);

unsigned char *exception_response(struct ModbusFrame packet, int size);

void start_server(char *ip, int port);