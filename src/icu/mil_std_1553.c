#include "client.h"
#include "mil_std_1553.h"

/**
 * @brief Thread function for handling RT 1553 communication.
 * 
 * @param arg Pointer to thread arguments (context or configuration data).
 * @return void* Thread exit status or result.
 */
static void* rt_1553_thread(void* arg);

/**
 * @brief Adds text to a message data buffer.
 * 
 * @param msgdata Pointer to the message data buffer.
 * @param data Pointer to the text to add.
 * @param len Length of the text in bytes.
 */
static void add_text(usint *msgdata, uint8_t *data, size_t len);

/**
 * @brief Handles errors by logging or reporting them.
 * 
 * @param status Error status code.
 * @param msg Pointer to the error message string.
 * @return int Status code indicating error handling result.
 */
static int handle_error(int status, const char *msg);

static int handle = -1;
pthread_t handle_1553_thread;
static int isThreadRun;

int getIsThreadRun() {
    return isThreadRun;
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
    if(status < 0) return handle_error(status, "Init_Module_Px failure");

    handle = status;
    printf("Init MODULE_1553 success!\n");

    usint cardtype;
    status = Get_Card_Type_Px(handle, &cardtype);
	if (status < 0)	printf ("Get_Card_Type_Px returned error: %s\n", Print_Error_Px(status));

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

    if(pthread_create(&handle_1553_thread, NULL, rt_1553_thread, config) != 0) {
        perror("Failed to create receive 1553 thread");
        release_module_1553(handle);
        return -1;
    }

    return 0;
}

int load_datablk(int blknum, uint8_t *data, size_t len) {
    int status;
    usint msgdata[32];
    memset(msgdata, 0, sizeof(msgdata));

    add_text(msgdata, data, len);

    status = Load_Datablk_Px(handle, blknum, msgdata);
    if(status < 0) { return handle_error(status, "Load_Datablk_Px failure");}

    return 0;
}

static void* rt_1553_thread(void* arg) {
    Config *config = (Config*)arg;

    int rt_addr = config->device.rt_addr;
    char errstr[255];
    int status, blknum;
    usint rt, subaddr, direction, wordCount, rt_id;
    usint msgdata[32] = {0};
    
    status = Set_Mode_Px(handle, RT_MODE);
    if(status < 0) { handle_error(status, "Set_Mode failure"); }

    status = Set_RT_Active_Px(handle, rt_addr, 0);
    if(status < 0) { handle_error(status, "Set_RT_Active failure");}

    int rtid;
    CommandList_t *cmd = &config->cmds;
    for (size_t i = 0; i < cmd->count_rx; ++i) {
        Message_t *msg = &cmd->messages_rx[i];
        for (size_t i = 0; i < 32; ++i) {
            if(i == msg->op_code) {
                RT_Id_Px(rt_addr, TRANSMIT, msg->op_code, &rtid);
            } else {
                RT_Id_Px(rt_addr, RECEIVE, i, &rtid);
            }
            Assign_RT_Data_Px(handle, rtid, i);
        }
    }

    status = Run_RT_Px(handle);
    if(status < 0) { handle_error(status, "Run_RT failure"); }    

    isThreadRun = 1;
    printf("  Running the receive MODULE_1553 thread...\n");

    struct CMDENTRYRT rtcmd;
    while (!stop_flag) {
        if(Read_RT_Status_Px(handle) > 0) {
            status = Get_Next_RT_Message_Px(handle, &rtcmd);
            if(status < 0) continue;
            
            Parse_CommandWord_Px(rtcmd.command, &rt, &subaddr, &direction, &wordCount, &rt_id);
            if(direction == TRANSMIT) continue;
            if(wordCount == 0) wordCount = 32; // 0 indicates a word count of 32.

            blknum = Get_Blknum_Px(handle, rt_id);

            status = Read_Datablk_Px(handle, blknum, msgdata);
            if(status < 0) {
                Get_Error_String_Px(status, errstr);
                printf("Failed to read data block %d: %s\n", blknum, errstr);
                continue;
            }

            uint8_t received_text[64] = {0};
            uint32_t len = 0;

            for (int j = 0; j < wordCount && len < 64; j++) {
                usint word = msgdata[j];
                uint8_t byte;
                if((byte = (word >> 8) & 0xFF) == 0) break;
                received_text[len++] = byte;
                if((byte = word & 0xFF) == 0) break;
                received_text[len++] = byte;
            }

            if(len > 0) {
                handle_received_data(subaddr, received_text, len);
            }
        }
    }

    pthread_exit(NULL);
}

static void add_text(usint *msgdata, uint8_t *data, size_t len) {
    if(len > 64) { printf("add_text failure: length of the message > 64\n"); return; }

    for (int i = 0; i < len; i += 2) {
        unsigned char c1 = (i < len) ? data[i] : 0;
        unsigned char c2 = (i + 1 < len) ? data[i + 1] : 0;
        msgdata[i / 2] = (c1 << 8) | c2;
    }
}

static int handle_error(int status, const char *msg) {
    char errstr[255];
    Get_Error_String_Px(status, errstr);
    printf("%s: %s\n", msg, errstr);
    Release_Module_Px(handle);
    return status;
}
