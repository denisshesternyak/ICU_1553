#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 200112L

#include <signal.h>
#include <unistd.h>

#include "mil_std_1553.h"
#include "config.h"

static void* bc_1553_thread(void* arg);
static int send_msg(int frameid);
static int read_msg(short int id);
static const char *format_time(uint64_t usec);
static uint64_t get_time_microseconds();
static void print_msg(const char *dir, const int opcode, const char *text, size_t len);
static int create_frames(Config *config);
static int create_frameid(int rt_addr, int dir, Message_t *msg);
static void add_text(const char *str, usint *msgdata, size_t len);
static int handle_error(int status, const char *msg);

static int handle = -1;
pthread_t handle_1553_thread;
volatile sig_atomic_t stop_flag = 0;

void handle_sigint(int sig) {
    stop_flag = 1;
}

int release_module_1553() {
    if(handle >= 0) {
        stop_flag = 1;
        Stop_Px(handle);
        Release_Module_Px(handle);
        printf("\nRelease MODULE_1553\n");
        handle = -1;
    }
    return 0;
}

int init_module_1553(Config *config) {
    usint device = (usint)config->device.device_number;
    usint module = (usint)config->device.module_number;
    int status;

    status = Init_Module_Px(device, module);
    if(status < 0) { return handle_error(status, "Init_Module_Px failure"); }

    handle = status;
    printf("Init MODULE_1553 success!\n");

    usint cardtype;
    status = Get_Card_Type_Px(handle, &cardtype);
    if(status < 0) { return handle_error(status, "Get_Card_Type_Px failure"); }

    if ((cardtype == MOD_1553_SF)) {
		printf("  The module is a Single-Function module (PxS),\n");
	}

    status = Get_Board_Status_Px(handle);
    printf("  Board status is ");
    if(status & SELF_TEST_OK)
        printf("SELF TEST OK  ");
    if(status & RAM_OK)
        printf("RAM OK  ");
    if(status & TIMERS_OK)
        printf("TIMERS OK  ");
    if(status & BOARD_READY)
        printf("BOARD READY\n");
    else
        printf("BOARD NOT READY\n");

    status = Set_Mode_Px(handle, BC_MODE);
    if(status < 0) { return handle_error(status, "Set_Mode failure"); }

    create_frames(config);

    if(pthread_create(&handle_1553_thread, NULL, bc_1553_thread, config) != 0) {
        perror("Failed to create receive 1553 thread");
        release_module_1553(handle);
        return -1;
    }

    return 0;
}

static void* bc_1553_thread(void* arg) {
    Config *config = (Config*)arg;

    int frame_executed = 0;
    CommandList_t *cmd;
    Message_t *msg;

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("  Running the transmit MODULE_1553 thread...\n");
    printf("\n%-14s %-4s %-8s %-6s %-s\n", "Time", "Dir", "OpCode", "Len", "Message");

    while (!stop_flag) {
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000;

        cmd = &config->cmds;

        for(int i = 0; i < cmd->count_tx; i++) {
            msg = &cmd->messages_tx[i];
            int rate = 1000 / msg->rate;
            long last_time = msg->frame.last_time;
            
            if(elapsed_ms - last_time >= rate) {
                send_msg(msg->frame.id);
                print_msg("T", msg->op_code, msg->text, strlen(msg->text));

                frame_executed = 1;
                msg->frame.last_time = elapsed_ms;
            }
        }

        for(int i = 0; i < cmd->count_rx; i++) {
            msg = &cmd->messages_rx[i];
            int rate = 1000 / msg->rate;
            long last_time = msg->frame.last_time;
            
            if(elapsed_ms - last_time >= rate) {                
                send_msg(msg->frame.id);
                read_msg(msg->handle);
                
                frame_executed = 1;
                msg->frame.last_time = elapsed_ms;
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

static int send_msg(int frameid) {
    int status;
    usint statusword;

    status = Start_Frame_Px(handle, frameid, FULLFRAME);
    if(status < 0) { return handle_error(status, "Start_Frame failure"); }

    status = Run_BC_Px(handle, 1);
    if(status < 0) { return handle_error(status, "Run_BC failure"); }

    do {
        status = Get_Msgentry_Status_Px(handle, frameid, 0, &statusword);
    } while ((statusword & END_OF_MSG) != END_OF_MSG);

    return 0;
}

static int read_msg(short int id) {
    int status;
    size_t len = 0; 
    char data[65];
    memset(data, 0, 65);
    usint data_buffer[34];
    memset(data_buffer, 0, sizeof(data_buffer));

    status = Read_Message_Px(handle, id, data_buffer);
    if(status < 0) { return handle_error(status, "Read_Message failure"); }

    for (int i = 2; i < 34; i++) {
        usint word = data_buffer[i];
        uint8_t byte;
        if((byte = (word >> 8) & 0xFF) == 0) break;
        data[len++] = byte;
        if((byte = word & 0xFF) == 0) break;
        data[len++] = byte;
    }
    data[len] = '\0';

    int op_code = (data_buffer[0] & 0x03E0) >> 5;
    print_msg("R", op_code, data, len);
    
    return 0;
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

static uint64_t get_time_microseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void print_msg(const char *dir, const int opcode, const char *text, size_t len) {
    printf("%-14s %-4s 0x%-6.2X %-6lu %-s\n",
        format_time(get_time_microseconds()),
        dir,
        opcode,
        len,
        text);
}

static int create_frames(Config *config) {
    int rt_addr = config->device.rt_addr;
    CommandList_t *cmd = &config->cmds;
    Message_t *msg;

    for(int i = 0; i < cmd->count_tx; i++) {
        msg = &cmd->messages_tx[i];
        if(create_frameid(rt_addr, RECEIVE, msg) != 0) { goto end; }
    }

    for(int i = 0; i < cmd->count_rx; i++) {
        msg = &cmd->messages_rx[i];
        if(create_frameid(rt_addr, TRANSMIT, msg) != 0) { goto end; }
    }
    
    return 0;

end:
    perror("Failed create a message\n");
    return -1;
}

static int create_frameid(int rt_addr, int dir, Message_t *msg) {
    int status, wordcount;
    struct FRAME framestruct[2];
    usint cmdtype, msgdata[34];
    memset(msgdata, 0, sizeof(msgdata));
    
    if(dir == RECEIVE) {
        size_t len = strlen(msg->text);
        if(len >= 64) len = 64;

        wordcount = (len + 1) / 2;
        cmdtype = BC2RT;

        add_text(msg->text, msgdata, len);
    } else {
        wordcount = 32;
        cmdtype = RT2BC;
    }

    status = Command_Word_Px(rt_addr, dir, msg->sub_address, wordcount, &msgdata[0]);
    if(status < 0) return handle_error(status, "Command_Word_Px failure");

    status = Create_1553_Message_Px(handle, cmdtype, msgdata, &msg->handle);
    if(status < 0) return handle_error(status, "Create_1553_Message failure");

    framestruct[0].id = msg->handle;
    framestruct[0].gaptime = 5000;
    framestruct[1].id = 0;

    int frame_id = Create_Frame_Px(handle, &framestruct[0]);
    if(frame_id < 0) return handle_error(frame_id, "Create_Frame failure");

    msg->frame.id = frame_id;

    return 0;
}

static void add_text(const char *str, usint *msgdata, size_t len) {
    if(len > 64) { printf("add_text failure: length of the message > 64\n"); return; }

    for (int i = 0; i < len; i += 2) {
        unsigned char c1 = (i < len) ? str[i] : 0;
        unsigned char c2 = (i + 1 < len) ? str[i + 1] : 0;
        msgdata[i / 2 + 1] = (c1 << 8) | c2;
    }
}

static int handle_error(int status, const char *msg) {
    char errstr[255];
    Get_Error_String_Px(status, errstr);
    printf("%s: %s\n", msg, errstr);
    release_module_1553();
    return status;
}
