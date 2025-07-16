#ifndef __CONFIG_H
#define __CONFIG_H

typedef struct {
    int device_number;
    int module_number;
    int rt_addr;
    int sync_word;
} Device_t;

typedef struct {
    char source_ip[16];
    char dest_ip[16];
    int source_port;
    int dest_port;
} Network_t;

typedef struct {
    int opcode;
    char rate_str[8];
    int rate_value;
    int is_rate_numeric;
    int sub_address;
} Message_t;

typedef struct {
    Message_t irst_01_commands;
    Message_t irst_07_tod_cmd;
    Message_t irst_09_navigation_data;
    Message_t irst_11_status_report;
    Message_t irst_15_bit_report;
    Message_t icu_status;
    Message_t icu_status_ack;
} CMD_IRST_t;

typedef struct {
    Device_t device;
    Network_t network;
    CMD_IRST_t cmds;
} Config;

void load_config(const char* filename, Config* config);

#endif //__CONFIG_H