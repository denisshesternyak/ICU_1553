#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "server.h"
#include "config.h"

#define ICU_STATUS_OPCODE 0x10
#define ICU_STATUS_ACK_OPCODE 0x11
#define ICU_STATUS_ACK_MSG "ICU STATUS ACK" 

#define BUFFER_SIZE 1024

pthread_t trmt_client_thread;
pthread_t recv_client_thread;
struct sockaddr_in client_addr;
socklen_t addr_len;
char client_ip[INET_ADDRSTRLEN];

static void parse_buffer(const char *buffer);
static void *receive_data(void *arg);
static void *transmit_data(void *arg);
static int send_data(const char *msg, size_t len);
static void data_packing(MsgHeader1553_t *hdr, int opcode, const char *msg, size_t len);
static void create_header(uint32_t subaddr, const char *text, uint32_t len, MsgHeader1553_t *hdr);
static void print_msg(uint32_t opcode, const char *dir, const char *text, uint32_t len);
static uint32_t crc32(const void *data, size_t length);
// static void print_header(const MsgHeader1553_t *hdr);

static int sockfd = -1;

volatile sig_atomic_t stop_flag;

void handle_sigint(int sig) {
    stop_flag = 1;
    close_socket();
}

void close_socket() {
    if(sockfd > 0) {
        shutdown(sockfd, SHUT_RD);
        close(sockfd);
        sockfd = -1;
    }
    printf("\nClose SOCKET\n");
}

static void *receive_data(void *arg) {
    //Config *config = (Config*)arg;

    char buffer[BUFFER_SIZE];

    printf("  Running the receive SOCKET thread...\n");

    while (!stop_flag) {
        int received = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0, 
                               (struct sockaddr *)&client_addr, &addr_len);
        if (received < 0) {
            if (errno == EBADF) {
                printf("Socket was closed, exiting receive thread.\n");
                break;
            }
            perror("Receive failed");
            continue;
        }
        
        buffer[received] = '\0';
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        parse_buffer(buffer);
    }

    return NULL;
}


static void* transmit_data(void* arg) {
    Config *config = (Config*)arg;
    int frame_executed = 0;
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("  Running the transmit SOCKET thread...\n");
    printf("\n%-14s %-4s %-8s %-6s %-s\n", "Time", "Dir", "OpCode", "Len", "Message");

    while (!stop_flag) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000;

        CommandList_t *cmd = &config->cmds;
        for(int i = 0; i < cmd->count; i++) {
            Message_t *msg = &cmd->messages[i];
            int rate = 1000 / msg->rate;
            long last_time = msg->last_time;
            
            if(elapsed_ms - last_time >= rate) {
                MsgHeader1553_t header;
                data_packing(&header, msg->op_code, msg->text, strlen(msg->text));

                msg->last_time = elapsed_ms;
                frame_executed = 1;
            }
        }

        if(!frame_executed) {
            usleep(1000);
            continue;
        }
        frame_executed = 0;
    }

    pthread_exit(NULL);
}

int init_socket(Config *config) {
    struct sockaddr_in server_addr;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config->port);

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    addr_len = sizeof(client_addr);

    printf("Init SOCKET success!\n");

    if(pthread_create(&recv_client_thread, NULL, receive_data, config) != 0) {
        perror("Thread creation failed");
        close(sockfd);
        return -1;
    }

    if(pthread_create(&trmt_client_thread, NULL, transmit_data, config) != 0) {
        perror("Thread creation failed");
        close(sockfd);
        return -1;
    }

    return 0;
}

static void parse_buffer(const char *buffer) {
    MsgHeader1553_t header;
    memcpy(&header, buffer, sizeof(MsgHeader1553_t));

    const char *msg = buffer+HEADER_SIZE;
    size_t len = header.msg_length-HEADER_SIZE;

    if(crc32(msg, len) == header.payload_crc32) {
        print_msg(header.msg_opcode, "R", msg, len);

        if(header.msg_opcode == ICU_STATUS_OPCODE) {
            data_packing(&header, ICU_STATUS_ACK_OPCODE, ICU_STATUS_ACK_MSG, strlen(ICU_STATUS_ACK_MSG));
        }
    } 
}

static void data_packing(MsgHeader1553_t *hdr, int opcode, const char *msg, size_t len) {
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);

    create_header(opcode, msg, len, hdr);
    memcpy(message, hdr, HEADER_SIZE);
    memcpy(message+HEADER_SIZE, msg, len);

    if(send_data(message, hdr->msg_length) >= 0) {
        print_msg(hdr->msg_opcode, "T", msg, len);
    }
}

static int send_data(const char *msg, size_t len) {
    return sendto(sockfd, msg, len, 0, 
                    (struct sockaddr *)&client_addr, addr_len);
}

static uint64_t get_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static const char *format_time(uint64_t usec) {
    static char buf[20];
    time_t seconds = usec / 1000000;
    struct tm *tm_info = localtime(&seconds);
    int millisec = (usec % 1000000) / 1000;

    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             millisec);
    return buf;
}

static void create_header(uint32_t subaddr, const char *text, uint32_t len, MsgHeader1553_t *hdr) {
    static uint32_t msg_seq_counter = 1;
    hdr->msg_opcode = subaddr;
    hdr->msg_length = len + HEADER_SIZE;
    hdr->msg_seq_number = msg_seq_counter++;
    hdr->payload_crc32 = crc32(text, len);
    hdr->send_time = 0;
    hdr->receive_time = get_time_microseconds();
    hdr->send_time = get_time_microseconds();
}


static void print_msg(uint32_t opcode, const char *dir, const char *text, uint32_t len) {
    printf("%-14s %-4s 0x%-6.2X %-6u %-s\n",
        format_time(get_time_microseconds()),
        dir,
        opcode,
        len,
        text);
}

static uint32_t crc32(const void *data, size_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++) {
            if(crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return ~crc;
}

// static void print_header(const MsgHeader1553_t *hdr) {
//     printf("HEADER STRUCTURE:\n");
//     printf("  Sync Word      : 0x%08X\n", hdr->sync_word);
//     printf("  Msg OpCode     : 0x%08X\n", hdr->msg_opcode);
//     printf("  Msg Length     : %u bytes\n", hdr->msg_length);
//     printf("  Msg Sequence # : %u\n", hdr->msg_seq_number);
//     printf("  CRC-32         : 0x%08X\n", hdr->payload_crc32);
//     printf("  Send Time      : %lu us\n", (uint64_t)hdr->send_time);
//     printf("  Receive Time   : %lu us\n", (uint64_t)hdr->receive_time);
//     printf("  Spare          : %lu\n", (uint64_t)hdr->spare);
// }