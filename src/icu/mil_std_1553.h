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
 * - load_datablk(): Loads a data block with specified text and length.
 * - getIsThreadRun(): Checks if a thread is running.
 *
 * @section Dependencies
 * - Standard: stdio.h, stdlib.h, pthread.h, unistd.h, string.h
 * - Custom: galahad_px.h, config.h
 *
 * @section Usage
 * Include this header to access MIL-STD-1553 functionality. Ensure all dependencies are available.
 */
#ifndef MIL_STD_1553_H
#define MIL_STD_1553_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "galahad_px.h"
#include "config.h"

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
 * @param config Pointer to the configuration structure.
 * @return int Status code: 0 for success, non-zero for failure.
 */
int init_module_1553(Config *config);

/**
 * @brief Releases the 1553 communication module.
 * 
 * @return int Status code: 0 for success, non-zero for failure.
 */
int release_module_1553();

/**
 * @brief Loads a data block with specified text and length.
 * 
 * @param blknum Data block number.
 * @param text Pointer to the text to load.
 * @param len Length of the text in bytes.
 * @return int Status code: 0 for success, non-zero for failure.
 */
int load_datablk(int blknum, const char *text, size_t len);

/**
 * @brief Checks if a thread is running.
 * 
 * @return int Non-zero if the thread is running, 0 otherwise.
 */
int getIsThreadRun();

#endif // MIL_STD_1553_H