#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "galahad_px.h"
#include "config.h"
#include "ini.h"
#include "mil_std_1553.h"

#define CONFIG_FILE "config.ini"

// static void print_config(Config *config) {
//     printf("Device_Number: %d\n", config->device.device_number);
//     printf("Module_Number: %d\n", config->device.module_number);
//     printf("RT_Addr: %d\n", config->device.rt_addr);
//     printf("-----------------\n");

//     for (size_t i = 0; i < config->cmds.count; ++i) {
//         Message_t *msg = &config->cmds.messages[i];
//         printf("  SubAddr: %02u, OpCode: 0x%02X, Rate: %d, Text: %s\n",
//                msg->sub_address, msg->op_code, msg->rate, msg->text);
//         DataFrame_t df = msg->frame;
//         printf("  Id: %d, Last time: %lu, Status: %d\n",
//                df.id, df.last_time, df.status);
//     }
// }

int main(int argc, char **argv) {
    Config config;
    memset(&config, 0, sizeof(config));

    if(ini_parse(CONFIG_FILE, command_handler, &config) < 0) {
        printf("Can't load %s\n", CONFIG_FILE);
        return 1;
    }

    // print_config(&config);

    if(init_module_1553(&config) != 0) {
        perror("Failed initialization module 1553 \n");
        return 1;
    }

    signal(SIGINT, handle_sigint);

    pthread_join(trmt_1553_thread, NULL);
    release_module_1553();

    free(config.cmds.messages_tx);
    free(config.cmds.messages_rx);
    printf("Exit.\n");
    return 0;
}

