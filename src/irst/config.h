/**
 * @file config.h
 * @brief Configuration structures for command processing and network port setup.
 *
 * Defines data structures for storing MIL-STD-1553-related command configurations and
 * a network port parameter. Includes a configuration handler function used to parse and
 * populate configuration data from an external source (e.g., INI file).
 *
 * @section Structures
 * - Message_t: Describes a single command message with subaddress, opcode, rate, payload, and timestamp.
 * - CommandList_t: Holds a dynamic list of Message_t entries and their count.
 * - Config: Root configuration structure containing a network port and associated commands.
 *
 * @section Functions
 * - command_handler(): Parses configuration fields and updates the Config structure.
 *
 * Use this header in components that require access to command definitions and network configuration.
 */
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

/**
 * @brief Handles configuration commands by parsing sections and fields from a configuration source.
 * 
 * @param user Pointer to the configuration structure (cast to Config* internally).
 * @param section The configuration section name (e.g., "device", "send_command_N", "get_command_N").
 * @param name The field name within the section (e.g., "Device_Number", "SubAddress").
 * @param value The value associated with the field name.
 * @return int Returns 1 on successful processing or 0 if validation fails (e.g., invalid integer values).
 */
int command_handler(void* user, const char* section, const char* name, const char* value);

#endif //CONFIG_H