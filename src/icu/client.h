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
    const char *text; 
} PrintMsg_t;

extern volatile sig_atomic_t stop_flag;
extern pthread_t handle_rxclient_thread;
extern pthread_t handle_txclient_thread;

void handle_sigint(int sig);
void close_socket();
int init_socket(Config *config);
void handle_received_data(uint32_t subaddr, const char *text, uint32_t len);

#endif //CLIENT_H