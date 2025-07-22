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

extern pthread_t trmt_client_thread;
extern pthread_t recv_client_thread;

void handle_sigint(int sig);
void close_socket();
int init_socket(Config *config);
void *receive_data(void *arg);

#endif //SERVER_H