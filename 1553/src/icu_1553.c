#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "config.h"
#include "ini.h"
#include "mil_std_1553.h"
#include "udp.h"

#define CONFIG_FILE "config.ini"

// static void print_config(Config *config) {
//     printf("Device_Number: %d\n", config->device.device_number);
//     printf("Module_Number: %d\n", config->device.module_number);
//     printf("RT_Addr: %d\n", config->device.rt_addr);
//     printf("Sync_Word: 0x%X\n", config->device.sync_word);
//     printf("-----------------\n");

//     printf("SourceIP: %s, SourcePort: %d\n", config->network.source.ip, config->network.source.port);
//     printf("DestIP: %s, DestPort: %d\n", config->network.destination.ip, config->network.destination.port);
//     printf("-----------------\n");

//     for (size_t i = 0; i < config->cmds.count; ++i) {
//         Message_t *msg = &config->cmds.messages[i];
//         printf("  SubAddr: %02u, OpCode: 0x%02X, Rate: %s\n",
//                msg->sub_address, msg->op_code, msg->rate);
//     }
// }

int main(int argc, char **argv) {
    int handle;
    Config config = {0};

    if (ini_parse(CONFIG_FILE, command_handler, &config) < 0) {
        printf("Can't load %s\n", CONFIG_FILE);
        return 1;
    }

    // print_config(&config);

    set_sync_word(config.device.sync_word);

    if(init_module_1553(config.device.device_number, config.device.module_number, &handle) != 0) {
        perror("Failed initialized init_module_1553\n");
        return 1;
    }
    printf("Init module 1553 success!\n");

    signal(SIGINT, handle_sigint);

    pthread_t recv_thread;
    struct rt_args args = {.handle = handle, .config = config};

    if (pthread_create(&recv_thread, NULL, receive_1553_thread, &args) != 0) {
        perror("Failed to create receive thread");
        release_module_1553(handle);
        return 1;
    }

    pthread_join(recv_thread, NULL);
    release_module_1553(handle);
    free(config.cmds.messages);
    printf("Exit.\n");
}