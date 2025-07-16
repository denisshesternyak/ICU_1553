#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "config.h"
#include "mil_std_1553.h"
#include "udp.h"
#include <signal.h>

#define CONFIG_FILE "config.ini"

static void print_config(Config *config) {
    printf("Device_Number: %d\n", config->device.device_number);
    printf("Module_Number: %d\n", config->device.module_number);
    printf("RT_Addr: %d\n", config->device.rt_addr);
    printf("Sync_Word: 0x%X\n", config->device.sync_word);
    printf("-----------------\n");

    printf("SourceIP: %s, SourcePort: %d\n", config->network.source_ip, config->network.source_port);
    printf("DestIP: %s, DestPort: %d\n", config->network.dest_ip, config->network.dest_port);
    printf("-----------------\n");

    printf("IRST_01_COMMANDS: OpCode=%d, Rate=%d, SubAddr=%d\n", config->cmds.irst_01_commands.opcode, config->cmds.irst_01_commands.rate_value, config->cmds.irst_01_commands.sub_address);
    printf("IRST_07_TOD_CMD: OpCode=%d, Rate=%s, SubAddr=%d\n", config->cmds.irst_07_tod_cmd.opcode, config->cmds.irst_07_tod_cmd.rate_str, config->cmds.irst_07_tod_cmd.sub_address);
    printf("IRST_09_NAVIGATION_DATA: OpCode=%d, Rate=%d, SubAddr=%d\n", config->cmds.irst_09_navigation_data.opcode, config->cmds.irst_09_navigation_data.rate_value, config->cmds.irst_09_navigation_data.sub_address);
    printf("IRST_11_STATUS_REPORT_DATA: OpCode=%d, Rate=%d, SubAddr=%d\n", config->cmds.irst_11_status_report.opcode, config->cmds.irst_11_status_report.rate_value, config->cmds.irst_11_status_report.sub_address);
    printf("IRST_15_BIT_REPORT_DATA: OpCode=%d, Rate=%d, SubAddr=%d\n", config->cmds.irst_15_bit_report.opcode, config->cmds.irst_15_bit_report.rate_value, config->cmds.irst_15_bit_report.sub_address);
    printf("ICU_STATUS_DATA: OpCode=%d, Rate=%d, SubAddr=%d\n", config->cmds.icu_status.opcode, config->cmds.icu_status.rate_value, config->cmds.icu_status.sub_address);
    printf("ICU_STATUS_ACK_DATA: OpCode=%d, Rate=%d, SubAddr=%d\n", config->cmds.icu_status_ack.opcode, config->cmds.icu_status_ack.rate_value, config->cmds.icu_status_ack.sub_address);
}

int main(int argc, char **argv) {
    int handle;
    Config config;

    memset(&config, 0, sizeof(config));
    load_config(CONFIG_FILE, &config);

    //print_config(&config);

    set_sync_word(config.device.sync_word);

    if(init_module_1553(config.device.device_number, config.device.module_number, &handle) != 0) {
        perror("Failed initialized init_module_1553\n");
        return 1;
    }
    printf("Init module 1553 success!\n");

    signal(SIGINT, handle_sigint);

    pthread_t recv_thread;
    struct rt_args args = {.handle = handle, .rt_addr = config.device.rt_addr};

    if (pthread_create(&recv_thread, NULL, receive_1553_thread, &args) != 0) {
        perror("Failed to create receive thread");
        release_module_1553(handle);
        return 1;
    }

    pthread_join(recv_thread, NULL);
    release_module_1553(handle);
    printf("Exit.\n");
}