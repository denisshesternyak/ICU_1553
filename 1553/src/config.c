#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

volatile sig_atomic_t stop_flag = 0;

void handle_sigint(int sig) {
    stop_flag = 1;
}

int command_handler(void* user, const char* section, const char* name, const char* value) {
    Config* config = (Config*)user;
    Message_t* current = NULL;

    if (strcmp(section, "device") == 0) {
        if (strcmp(name, "Device_Number") == 0) {
            config->device.device_number = atoi(value);
        } else if (strcmp(name, "Module_Number") == 0) {
            config->device.module_number = atoi(value);
        } else if (strcmp(name, "RT_Addr") == 0) {
            config->device.rt_addr = atoi(value);
        } else if (strcmp(name, "Sync_Word") == 0) {
            config->device.sync_word = strtoul(value, NULL, 0);
        }
        return 1;
    }

    if (strcmp(section, "source") == 0) {
        if (strcmp(name, "IP") == 0) {
            strncpy(config->network.source.ip, value, sizeof(config->network.source.ip) - 1);
        } else if (strcmp(name, "Port") == 0) {
            config->network.source.port = atoi(value);
        }
        return 1;
    }

    if (strcmp(section, "destination") == 0) {
        if (strcmp(name, "IP") == 0) {
            strncpy(config->network.destination.ip, value, sizeof(config->network.destination.ip) - 1);
        } else if (strcmp(name, "Port") == 0) {
            config->network.destination.port = atoi(value);
        }
        return 1;
    }

    if (strncmp(section, "command_", 8) != 0)
        return 1;

    int subaddr = atoi(section + 8);

    if (config->cmds.count > 0 && config->cmds.messages != NULL) {
        for (size_t i = 0; i < config->cmds.count; ++i) {
            if (config->cmds.messages[i].sub_address == subaddr) {
                current = &config->cmds.messages[i];
                break;
            }
        }
    }

    if (!current) {
        size_t new_size = sizeof(Message_t) * (config->cmds.count + 1);
        Message_t *new_list = realloc(config->cmds.messages, new_size);
        if (!new_list) {
            fprintf(stderr, "Memory allocation failed for messages\n");
            exit(EXIT_FAILURE);
        }
        config->cmds.messages = new_list;
        current = &config->cmds.messages[config->cmds.count];
        memset(current, 0, sizeof(Message_t));
        current->sub_address = subaddr;
        config->cmds.count++;
    }

    if (strcmp(name, "SubAddress") == 0) {
        current->sub_address = atoi(value);
    } else if (strcmp(name, "OpCode") == 0) {
        current->op_code = strtol(value, NULL, 0);
    } else if (strcmp(name, "Rate") == 0) {
        strncpy(current->rate, value, sizeof(current->rate) - 1);
    }

    return 1;
}