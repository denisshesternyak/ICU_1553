#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

static void parse_message_field(const char* line, const char* prefix, Message_t* msg) {
    if (strncmp(line, prefix, strlen(prefix)) != 0)
        return;

    const char* field = line + strlen(prefix);

    if (strncmp(field, "OpCode=", 7) == 0) {
        sscanf(field + 7, "%x", &msg->opcode);
    } else if (strncmp(field, "Rate=", 5) == 0) {
        char rate_buf[16];
        sscanf(field + 5, "%15s", rate_buf);

        char* endptr;
        int rate_val = strtol(rate_buf, &endptr, 10);
        if (*endptr == '\0') {
            msg->rate_value = rate_val;
            msg->is_rate_numeric = 1;
        } else {
            strncpy(msg->rate_str, rate_buf, sizeof(msg->rate_str) - 1);
            msg->rate_str[sizeof(msg->rate_str) - 1] = '\0';
            msg->is_rate_numeric = 0;
        }
    } else if (strncmp(field, "SubAddress=", 11) == 0) {
        sscanf(field + 11, "%d", &msg->sub_address);
    }
}

void load_config(const char* filename, Config* config) {
    char line[256];
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Device_Number=", 14) == 0) {
            sscanf(line + 14, "%d", &config->device.device_number);
        } else if (strncmp(line, "Module_Number=", 14) == 0) {
            sscanf(line + 14, "%d", &config->device.module_number);
        } else if (strncmp(line, "RT_Addr=", 8) == 0) {
            sscanf(line + 8, "%d", &config->device.rt_addr);
        } else if (strncmp(line, "Sync_Word=", 10) == 0) {
            char rate_buf[16];
            sscanf(line + 10, "%15s", rate_buf);
            char* endptr;
            int rate_val = strtol(rate_buf, &endptr, 16);
            if (*endptr == '\0') {
                config->device.sync_word = rate_val;
            }
        }

        if (strncmp(line, "Source_IP=", 10) == 0) {
            sscanf(line + 10, "%15s", config->network.source_ip);
        } else if (strncmp(line, "Dest_IP=", 8) == 0) {
            sscanf(line + 8, "%15s", config->network.dest_ip);
        } else if (strncmp(line, "Source_Port=", 12) == 0) {
            sscanf(line + 12, "%d", &config->network.source_port);
        } else if (strncmp(line, "Dest_Port=", 10) == 0) {
            sscanf(line + 10, "%d", &config->network.dest_port);
        }

        parse_message_field(line, "IRST_01_commands_", &config->cmds.irst_01_commands);
        parse_message_field(line, "IRST_07_tod_cmd_", &config->cmds.irst_07_tod_cmd);
        parse_message_field(line, "IRST_09_navigation_data_", &config->cmds.irst_09_navigation_data);
        parse_message_field(line, "IRST_11_status_report_data_", &config->cmds.irst_11_status_report);
        parse_message_field(line, "IRST_15_bit_report_data_", &config->cmds.irst_15_bit_report);
        parse_message_field(line, "ICU_STATUS_data_", &config->cmds.icu_status);
        parse_message_field(line, "ICU_STATUS_ACK_data_", &config->cmds.icu_status_ack);
    }

    fclose(file);
}