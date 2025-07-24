#ifndef MIL_STD_1553_H
#define MIL_STD_1553_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "galahad_px.h"
#include "config.h"

extern pthread_t handle_1553_thread;

void handle_sigint(int sig);

// Initialize the 1553 module
int init_module_1553(Config *config);

// Release the 1553 module
int release_module_1553();

int load_datablk(int blknum, const char *text, size_t len);

int getIsThreadRun();

#endif // MIL_STD_1553_H