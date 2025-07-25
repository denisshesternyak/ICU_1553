/**
 * @file server.h
 * @brief Server-side socket interface for MIL-STD-1553 communication.
 *
 * Declares server-side components for handling network communication over sockets,
 * used in conjunction with MIL-STD-1553 operations. Includes a packed message header
 * structure, printable message metadata, and thread management for sending and receiving data.
 *
 * @section Structures
 * - MsgHeader1553_t: Packed message header containing metadata such as opcode, payload size, CRC, and timestamps.
 * - PrintMsg_t: Describes a message in a printable/loggable format, including direction and payload text.
 *
 * @section Globals
 * - handle_txclient_thread: Thread for sending messages over the network.
 * - handle_rxclient_thread: Thread for receiving messages from the network.
 *
 * @section Functions
 * - handle_sigint(): Signal handler to safely terminate communication on SIGINT (Ctrl+C).
 * - close_socket(): Shuts down the active communication socket.
 * - init_socket(): Initializes the server socket based on the provided Config structure.
 *
 * This header is used by components responsible for accepting and managing client connections
 * and transmitting MIL-STD-1553-related data over the network.
 */
#ifndef SERVER_H
#define SERVER_H

#include <pthread.h>
#include <stdint.h>

#include "config.h"

#define HEADER_SIZE 44

#pragma pack(push, 1)
typedef struct {
    uint32_t sync_word;
    uint32_t msg_opcode;
    uint32_t msg_length;
    uint32_t msg_seq_number;
    uint32_t payload_crc32;
    uint64_t send_time;
    uint64_t receive_time;
    uint64_t spare;
} MsgHeader1553_t;
#pragma pack(pop)

typedef struct {
    uint32_t opcode;
    uint32_t len;
    const char *dir;
    const char *text; 
} PrintMsg_t;

extern pthread_t handle_txclient_thread;
extern pthread_t handle_rxclient_thread;

/**
 * @brief Signal handler for SIGINT (Ctrl+C).
 * 
 * @param sig Signal number (e.g., SIGINT).
 */
void handle_sigint(int sig);

/**
 * @brief Closes the communication socket.
 */
void close_socket();

/**
 * @brief Initializes the communication socket.
 * 
 * @param config Pointer to the configuration structure.
 * @return int Status code: 0 for success, non-zero for failure.
 */
int init_socket(Config *config);

#endif //SERVER_H