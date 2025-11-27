#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define LOG_FILENAME_MAX_LEN 256 
#define DEFAULT_LOG_FILENAME "default.log"
#define TIME_BUFFER_SIZE 64

int init_logger(const char* filename);

void add_log(const char* message);

void close_logger();

#endif //LOGGER_H