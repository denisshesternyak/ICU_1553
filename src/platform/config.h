#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int device_number;
    int module_number;
    int rt_addr;
} Device_t;

typedef struct {
    int id;
    long last_time;
} DataFrame_t;

typedef struct {
    int sub_address;
    int op_code;
    int rate;
    char text[65];
    short int handle;
    DataFrame_t frame;
} Message_t;

typedef struct {
    size_t count_tx;
    size_t count_rx;
    Message_t *messages_tx;
    Message_t *messages_rx;
} CommandList_t;

typedef struct {
    Device_t device;
    CommandList_t cmds;
} Config;

int command_handler(void* user, const char* section, const char* name, const char* value);

#endif //CONFIG_H