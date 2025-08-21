/**
 * @file client.h
 * @brief Network communication client for MIL-STD-1553 interface.
 *
 * Declares functions, types, and globals used for handling socket-based network communication
 * in the MIL-STD-1553 system. Includes message header structure for transmission, utilities
 * for socket initialization and shutdown, and multithreaded data handling.
 *
 * @section Structures
 * - MsgHeader1553_t: Packed message header containing metadata for each transmitted frame (e.g., sync word, opcode, length, CRC, timestamps).
 * - PrintMsg_t: Human-readable representation of a message for logging or debug output.
 *
 * @section Globals
 * - stop_flag: Flag set by the signal handler to request thread termination.
 * - handle_rxclient_thread: Thread handle for receiving data from the network.
 * - handle_txclient_thread: Thread handle for sending data over the network.
 *
 * @section Functions
 * - handle_sigint(): Signal handler to cleanly shut down on SIGINT (Ctrl+C).
 * - close_socket(): Closes active network socket(s).
 * - init_socket(): Initializes socket connection using values from the Config structure.
 * - handle_received_data(): Processes and dispatches incoming data to the appropriate handler.
 *
 * This header is used by components responsible for external communication over sockets in
 * conjunction with MIL-STD-1553 operations.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>
#include <stdint.h>
#include <signal.h>

#include "config.h"

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
    const char *from;
    const char *to;
    uint8_t *data; 
} PrintMsg_t;

extern volatile sig_atomic_t stop_flag;
extern pthread_t handle_rxclient_thread;
extern pthread_t handle_txclient_thread;

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

/**
 * @brief Processes received data from the communication interface.
 * 
 * @param subaddr Subaddress of the received message.
 * @param data Pointer to the received message content.
 * @param len Length of the message content in bytes.
 */
void handle_received_data(uint32_t subaddr, uint8_t *data, uint32_t len);

#endif //CLIENT_H