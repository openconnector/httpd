
#ifndef LOGGER_H_
#define LOGGER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void init_logger(char *path);

void info(char *buf,size_t count);


#endif
