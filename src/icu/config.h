/**
 * @file config.h
 * @brief Configuration structures for MIL-STD-1553 communication and network interface.
 *
 * Defines configuration data types used to initialize and manage the MIL-STD-1553 communication
 * module and associated network settings. Includes structures for device parameters, command messages,
 * and socket endpoints. Also provides a handler function for parsing key-value configuration entries.
 *
 * @section Structures
 * - Device_t: Contains hardware-specific identifiers such as device number, module number, RT address, and sync word.
 * - NetData_t: Describes a network endpoint (IP address and port).
 * - Network_t: Groups source and destination network settings.
 * - Message_t: Represents a 1553 command or telemetry message with metadata (subaddress, opcode, rate, etc.).
 * - CommandList_t: Holds lists of transmit and receive messages.
 * - Config: Root configuration structure combining all subsystems: device, network, and command lists.
 *
 * @section Functions
 * - command_handler(): Parses configuration fields and populates the Config structure.
 *
 * This header is intended to be included wherever access to system configuration is required.
 */
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int device_number;
    int module_number;
    int rt_addr;
    int sync_word;
} Device_t;

typedef struct {
    char ip[16];
    int port;
} NetData_t;

typedef struct {
    NetData_t source;
    NetData_t destination;
} Network_t;

typedef struct {
    int sub_address;
    int op_code;
    int rate;
    char text[65];
    long last_time;
} Message_t;

typedef struct {
    size_t count_tx;
    size_t count_rx;
    Message_t *messages_tx;
    Message_t *messages_rx;
} CommandList_t;

typedef struct {
    Device_t device;
    Network_t network;
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