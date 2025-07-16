#ifndef __CONFIG_H
#define __CONFIG_H

typedef struct {
    int device_number;
    int module_number;
    int rt_addr;
    int sync_word;
} Device_t;

typedef struct {
    char ip[16];
    int port;
} NetData_t;

typedef struct {
    NetData_t source;
    NetData_t destination;
} Network_t;

typedef struct {
    int sub_address;
    int op_code;
    char rate[8];
} Message_t;

typedef struct {
    size_t count;
    Message_t *messages;
} CommandList_t;

typedef struct {
    Device_t device;
    Network_t network;
    CommandList_t cmds;
} Config;

int command_handler(void* user, const char* section, const char* name, const char* value);

#endif //__CONFIG_H