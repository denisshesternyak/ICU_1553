#ifndef MIL_STD_1553_H
#define MIL_STD_1553_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "galahad_px.h"
#include "config.h"

struct rt_args {
    int handle;
    Config config;
};

void handle_sigint(int sig);

// Initialize the 1553 module
int init_module_1553(int devnum, int modnum, int *handle);

// Release the 1553 module
int release_module_1553(int handle);

// Transmit data as Bus Controller (BC)
int transmit_1553(int handle, const char *text, int rt_addr);

// Receive data as Remote Terminal (RT)
void* receive_1553_thread(void* arg);

// Print message status
void print_status_1553(usint statusword);

#endif // MIL_STD_1553_H