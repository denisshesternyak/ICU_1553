#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>

#include "client.h"
#include "mil_std_1553.h"

#define HEADER_SIZE 44
#define BUFFER_SIZE 1024

#define ICU_STATUS_ACK_OPCODE 0x11

static int sockfd = -1;
static struct sockaddr_in server_addr;
static MsgHeader1553_t header;

pthread_t recv_client_thread;
pthread_t trmt_client_thread;

volatile sig_atomic_t stop_flag = 0;

static void parse_buffer(const char *buffer);
static void print_msg(uint32_t opcode, const char *from, const char *to, const char *text, uint32_t len);
static void *receive_data(void *arg);
static void* transmit_data(void* arg);
void create_header(uint32_t subaddr, const char *text, uint32_t len, MsgHeader1553_t *hdr);

void handle_sigint(int sig) {
    stop_flag = 1;
    close_socket();
}

uint32_t crc32(const void *data, size_t length) {
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

void close_socket() {
    if(sockfd > 0) {
        shutdown(sockfd, SHUT_RD);
        close(sockfd);
        sockfd = -1;
    }
    printf("Close SOCKET\n");
}

int init_socket(Config *config) {
    header.sync_word = config->device.sync_word;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(config->network.source.port);
    if(bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->network.destination.port);
    if(inet_pton(AF_INET, config->network.destination.ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sockfd);
        return -1;
    }

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

static ssize_t send_data(const char *message, size_t len) {
    return sendto(sockfd, message, len, 0, 
                         (struct sockaddr *)&server_addr, sizeof(server_addr));
}

static void *receive_data(void *arg) {
    //Config *config = (Config*)arg;

    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);
    
    printf("  Running the receive SOCKET thread...\n");

    while (!stop_flag) {
        int received = recvfrom(sockfd, buffer, BUFFER_SIZE-1, 0, 
                               (struct sockaddr *)&from_addr, &addr_len);
        if(received == 0) break;
        if(received < 0) {
            if(errno == EBADF) {
                fprintf(stderr, "Socket was closed (EBADF), exiting thread.\n");
            } else if(errno == EINTR) {
                fprintf(stderr, "Interrupted by signal (EINTR), exiting thread.\n");
            } else {
                perror("Receive failed");
            }
            break;
        }

        buffer[received] = '\0';
        char from_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, INET_ADDRSTRLEN);
        //printf("Received from %s:%d: %s\n", from_ip, ntohs(from_addr.sin_port), buffer);

        parse_buffer(buffer);
    }

    return NULL;
}

static void* transmit_data(void* arg) {
    Config *config = (Config*)arg;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int opcode = 0;
    const char *msg;
    size_t len;
    int frame_executed = 0;

    printf("  Running the transmit SOCKET thread...\n");

    while (!stop_flag) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000;

        for(int i = 0; i < config->cmds.count; i++) {
            int rate = 1000 / config->cmds.messages[i].rate;
            long last_time = config->cmds.messages[i].frame.last_time;
            
            if(elapsed_ms - last_time >= rate) {
                config->cmds.messages[i].frame.status = 1;
                config->cmds.messages[i].frame.last_time = elapsed_ms;
            }
        }

        for(int i = 0; i < config->cmds.count; i++) {
            if(config->cmds.messages[i].frame.status) {
                opcode = config->cmds.messages[i].op_code;
                msg = config->cmds.messages[i].text;
                len = strlen(msg);

                config->cmds.messages[i].frame.status = 0;
                frame_executed = 1;
                break;
            }
        }

        if(!frame_executed) {
            usleep(1000);
            continue;
        }
        frame_executed = 0;

        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE);

        create_header(opcode, msg, len, &header);
        memcpy(message, &header, HEADER_SIZE);
        memcpy(message+HEADER_SIZE, msg, len);

        if(send_data(message, header.msg_length) >= 0) {
            print_msg(opcode, "ICU", "IRST", msg, len);
        }
    }

    pthread_exit(NULL);
}

static void parse_buffer(const char *buffer) {
    MsgHeader1553_t header;
    memcpy(&header, buffer, sizeof(MsgHeader1553_t));
    const char *msg = buffer+HEADER_SIZE;
    size_t len = header.msg_length-HEADER_SIZE;

    if(crc32(msg, len) == header.payload_crc32) {
        if(header.msg_opcode == ICU_STATUS_ACK_OPCODE) {
            print_msg(header.msg_opcode, "IRST", "ICU", msg, len);
        } else {
            //SEND TO PLATFORM
            print_msg(header.msg_opcode, "IRST", "PLATFORM", msg, len);
        }
    }    
}

static void print_msg(uint32_t opcode, const char *from, const char *to, const char *text, uint32_t len) {
    printf("%-14s %-10s %-10s 0x%-6.2X %-6u %-s\n",
        format_time(get_time_microseconds()),
        from,
        to,
        opcode,
        len,
        text);
}

void handle_received_data(uint32_t subaddr, const char *text, uint32_t len) {
    static uint32_t msg_seq_counter = 1;

    header.msg_opcode = subaddr;
    header.msg_length = len + HEADER_SIZE;
    header.msg_seq_number = msg_seq_counter++;
    header.payload_crc32 = crc32(text, len);
    header.send_time = 0;
    header.receive_time = get_time_microseconds();

    print_msg(subaddr, "PLATFORM", "IRST", text, len);

    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);

    header.send_time = get_time_microseconds();
    memcpy(message, &header, HEADER_SIZE);
    memcpy(message+HEADER_SIZE, text, len);

    if(send_data(message, header.msg_length) < 0) {
        perror("Failed to send the message");
    }
}

void create_header(uint32_t subaddr, const char *text, uint32_t len, MsgHeader1553_t *hdr) {
    static uint32_t msg_seq_counter = 1;
    hdr->msg_opcode = subaddr;
    hdr->msg_length = len + HEADER_SIZE;
    hdr->msg_seq_number = msg_seq_counter++;
    hdr->payload_crc32 = crc32(text, len);
    hdr->send_time = 0;
    hdr->receive_time = get_time_microseconds();
    hdr->send_time = get_time_microseconds();
}

