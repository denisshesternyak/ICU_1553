#ifndef CONFIG_H
#define CONFIG_H

#include <signal.h>

typedef struct {
    int device_number;
    int module_number;
    int rt_addr;
} Device_t;

typedef struct {
    int id;
    long last_time;
    int status;
} DataFrame_t;

typedef struct {
    int sub_address;
    int op_code;
    int rate;
    char text[65];
    DataFrame_t frame;
} Message_t;

typedef struct {
    size_t count;
    Message_t *messages;
} CommandList_t;

typedef struct {
    Device_t device;
    CommandList_t cmds;
} Config;

extern volatile sig_atomic_t stop_flag;

int command_handler(void* user, const char* section, const char* name, const char* value);

#endif //CONFIG_H