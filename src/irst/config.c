#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "config.h"

static int parse_int_checked(const char *value, int min, int max, const char *field_name, int *out);

int command_handler(void* user, const char* section, const char* name, const char* value) {
    Config* config = (Config*)user;

    if(strcmp(name, "Port") == 0) {
        config->port = atoi(value);
        return 1;
    }

    if(strncmp(section, "command_", 8) == 0) {
        static char last_section[64] = "";
        static Message_t *current = NULL;

        if(strcmp(last_section, section) != 0) {
            Message_t *new_list = realloc(config->cmds.messages, sizeof(Message_t) * (config->cmds.count + 1));
            if(!new_list) {
                fprintf(stderr, "Memory allocation failed for messages\n");
                exit(EXIT_FAILURE);
            }
            config->cmds.messages = new_list;
            current = &config->cmds.messages[config->cmds.count];
            memset(current, 0, sizeof(Message_t));
            config->cmds.count++;
            strncpy(last_section, section, sizeof(last_section) - 1);
            last_section[sizeof(last_section) - 1] = '\0';
        }

        if(strcmp(name, "SubAddress") == 0) {
            if(!parse_int_checked(value, 0, 255, "SubAddress", &current->sub_address)) return 0;
        } else if(strcmp(name, "OpCode") == 0) {
            if(!parse_int_checked(value, 0, 255, "OpCode", &current->op_code)) return 0;
        } else if(strcmp(name, "Rate") == 0) {
            if(!parse_int_checked(value, 1, 1000, "Rate", &current->rate)) return 0;
        } else if(strcmp(name, "Message") == 0) {
            strncpy(current->text, value, sizeof(current->text) - 1);
        } else {
            fprintf(stderr, "Warning: unknown field '%s' in section [%s]\n", name, section);
        }

        return 1;
    }

    return 1;
}

static int parse_int_checked(const char *value, int min, int max, const char *field_name, int *out)
{
    char *endptr = NULL;
    errno = 0;

    long result = strtol(value, &endptr, 0);
    if(errno != 0 || *endptr != '\0') {
        fprintf(stderr, "Error: invalid integer for %s: '%s'\n", field_name, value);
        return 0;
    }

    if(result < min || result > max) {
        fprintf(stderr, "Error: %s out of range (%d..%d): %ld\n", field_name, min, max, result);
        return 0;
    }

    *out = (int)result;
    return 1;
}