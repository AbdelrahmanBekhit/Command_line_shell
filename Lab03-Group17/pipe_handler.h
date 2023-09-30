#ifndef __PIPE_HANDLER_H
#define __PIPE_HANDLER_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>

int findpipe(const char* inputbuffer, size_t bufferlen);
void execute_pipe(char* first_command, char* second_command, size_t bufferlen);

#endif