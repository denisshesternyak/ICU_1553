/**
 * @file mil_std_1553.h
 * @brief Interface for MIL-STD-1553 bus communication.
 *
 * Provides a C interface for working with the MIL-STD-1553 serial data bus, commonly used in avionics.
 * This module includes initialization and cleanup routines, signal handling, and a structure for message logging.
 * Communication is managed in a separate thread and configured via external parameters.
 *
 * @section Structures
 * - PrintMsg_t: Stores message metadata (opcode, length, direction, and description).
 *
 * @section Globals
 * - handle_1553_thread: Thread handle for background communication.
 *
 * @section Functions
 * - handle_sigint(): Cleans up resources on SIGINT.
 * - init_module_1553(): Initializes the 1553 interface using configuration.
 * - release_module_1553(): Shuts down and releases resources.
 *
 * @section Dependencies
 * - Standard: stdio.h, stdlib.h, string.h, pthread.h, stdint.h
 * - Custom: galahad_px.h, config.h
 *
 * @section Usage
 * Include this header to access MIL-STD-1553 functionality. Ensure all dependencies are available.
 */
#ifndef MIL_STD_1553_H
#define MIL_STD_1553_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#include "galahad_px.h"
#include "config.h"

typedef struct {
    uint32_t opcode;
    uint32_t len;
    const char *dir;
    const char *text; 
} PrintMsg_t;

extern pthread_t handle_1553_thread;

/**
 * @brief Signal handler for SIGINT (Ctrl+C).
 * 
 * @param sig Signal number (e.g., SIGINT).
 */
void handle_sigint(int sig);

/**
 * @brief Initializes the 1553 communication module.
 * 
 * @param config Pointer to the configuration structure for the 1553 module.
 * @return int Status code indicating success (0) or failure (non-zero).
 */
int init_module_1553(Config *config);

/**
 * @brief Releases the 1553 communication module.
 * 
 * @return int Status code indicating success (0) or failure (non-zero).
 */
int release_module_1553();

#endif // MIL_STD_1553_H