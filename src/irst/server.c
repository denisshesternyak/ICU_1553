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

pthread_t trmt_client_thread;
pthread_t recv_client_thread;

static void print_msg(uint32_t opcode, const char *dir, const char *client_ip, const char *text, uint32_t len);
static uint32_t crc32(const void *data, size_t length);
static void print_header(const MsgHeader1553_t *hdr);

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

void *receive_data(void *arg) {
    //Config *config = (Config*)arg;

    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];

    printf("  Running the receive SOCKET thread...\n");
    printf("\n%-14s %-10s %-10s %-16s %-8s %-6s %-s\n", "Time", "From", "To", "ClientIP", "OpCode", "Len", "Message");

    while (!stop_flag) {
        socklen_t addr_len = sizeof(client_addr);
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
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        MsgHeader1553_t header;
        memcpy(&header, buffer, sizeof(MsgHeader1553_t));
        //print_header(&header);

        const char *msg = buffer+HEADER_SIZE;
        size_t len = header.msg_length-HEADER_SIZE;

        // printf("Received from %s:%d: %s\n", client_ip, ntohs(client_addr.sin_port), msg);
        // printf("CRC32: 0x%-6.2X 0x%-6.2X\n", crc32(msg, len), header.payload_crc32);

        if(crc32(msg, len) == header.payload_crc32) {
            print_msg(header.msg_opcode, "R", client_ip, msg, len);
        }

        // const char *response = "Hello from server!";
        // int sent = sendto(sockfd, response, strlen(response), 0, 
        //                 (struct sockaddr *)&client_addr, addr_len);
        // if(sent < 0) {
        //     perror("Sendto failed");
        // } else {
        //     printf("Sent to %s:%d: %s\n", client_ip, ntohs(client_addr.sin_port), response);
        // }
    }

    return NULL;
}

int init_socket(Config *config) {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

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

    printf("Init SOCKET success!\n");

    if(pthread_create(&recv_client_thread, NULL, receive_data, config) != 0) {
        perror("Thread creation failed");
        close(sockfd);
        return -1;
    }

    return 0;
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

static void print_msg(uint32_t opcode, const char *dir, const char *client_ip, const char *text, uint32_t len) {
    printf("%-14s %-4s %-16s 0x%-6.2X %-6u %-s\n",
        format_time(get_time_microseconds()),
        dir,
        client_ip,
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

static void print_header(const MsgHeader1553_t *hdr) {
    printf("HEADER STRUCTURE:\n");
    printf("  Sync Word      : 0x%08X\n", hdr->sync_word);
    printf("  Msg OpCode     : 0x%08X\n", hdr->msg_opcode);
    printf("  Msg Length     : %u bytes\n", hdr->msg_length);
    printf("  Msg Sequence # : %u\n", hdr->msg_seq_number);
    printf("  CRC-32         : 0x%08X\n", hdr->payload_crc32);
    printf("  Send Time      : %lu us\n", (uint64_t)hdr->send_time);
    printf("  Receive Time   : %lu us\n", (uint64_t)hdr->receive_time);
    printf("  Spare          : %lu\n", (uint64_t)hdr->spare);
}