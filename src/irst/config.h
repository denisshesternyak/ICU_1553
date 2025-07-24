#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int sub_address;
    int op_code;
    int rate;
    char text[65];
    long last_time;
} Message_t;

typedef struct {
    size_t count;
    Message_t *messages;
} CommandList_t;

typedef struct {
    int port;
    CommandList_t cmds;
} Config;

int command_handler(void* user, const char* section, const char* name, const char* value);

#endif //CONFIG_H