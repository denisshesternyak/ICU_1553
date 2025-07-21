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

    if(strcmp(section, "device") == 0) {
        if(strcmp(name, "Device_Number") == 0) {
            config->device.device_number = atoi(value);
        } else if(strcmp(name, "Module_Number") == 0) {
            config->device.module_number = atoi(value);
        } else if(strcmp(name, "RT_Addr") == 0) {
            config->device.rt_addr = atoi(value);
        } else if(strcmp(name, "Sync_Word") == 0) {
            config->device.sync_word = strtoul(value, NULL, 0);
        }
        return 1;
    }

    if(strcmp(section, "source") == 0) {
        if(strcmp(name, "IP") == 0) {
            strncpy(config->network.source.ip, value, sizeof(config->network.source.ip) - 1);
        } else if(strcmp(name, "Port") == 0) {
            config->network.source.port = atoi(value);
        }
        return 1;
    }

    if(strcmp(section, "destination") == 0) {
        if(strcmp(name, "IP") == 0) {
            strncpy(config->network.destination.ip, value, sizeof(config->network.destination.ip) - 1);
        } else if(strcmp(name, "Port") == 0) {
            config->network.destination.port = atoi(value);
        }
        return 1;
    }

    return 1;
}