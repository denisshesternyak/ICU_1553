/**
 * @file config.h
 * @brief Configuration structures and handler for MIL-STD-1553 module.
 *
 * Defines core data structures for device settings, command messages, and communication parameters 
 * used by the MIL-STD-1553 communication module. Provides a configuration handler for parsing 
 * key-value pairs from external configuration files or sources.
 *
 * @section Structures
 * - Device_t: Holds identifiers for the target device and module, including RT address.
 * - DataFrame_t: Tracks message ID and timing information.
 * - Message_t: Represents a 1553 message with parameters such as subaddress, opcode, rate, and text.
 * - CommandList_t: Organizes transmit and receive message lists.
 * - Config: Root configuration structure combining device info and command lists.
 *
 * @section Functions
 * - command_handler(): Callback for parsing and validating configuration entries.
 *
 * Use this header to define and manage runtime configuration for the 1553 communication module.
 */
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int device_number;
    int module_number;
    int rt_addr;
} Device_t;

typedef struct {
    int id;
    long last_time;
} DataFrame_t;

typedef struct {
    int sub_address;
    int op_code;
    int rate;
    char text[65];
    short int handle;
    DataFrame_t frame;
} Message_t;

typedef struct {
    size_t count_tx;
    size_t count_rx;
    Message_t *messages_tx;
    Message_t *messages_rx;
} CommandList_t;

typedef struct {
    Device_t device;
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