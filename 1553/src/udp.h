#ifndef __UDP_H
#define __UDP_H

#define HEADER_SIZE 44

typedef struct {
    unsigned int sync_word;
    unsigned int msg_opcode;
    unsigned int msg_length;
    unsigned int msg_seq_number;
    unsigned int payload_crc32;
    unsigned long long send_time;
    unsigned long long receive_time;
    unsigned long long spare;
} MsgHeader1553_t;

void set_sync_word(unsigned int val);
unsigned int crc32(const void *data, size_t length);
void handle_received_data(unsigned int subaddr, const char *text, unsigned int len);

#endif //__UDP_H