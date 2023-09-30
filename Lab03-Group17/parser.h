#ifndef __PARSER_H
#define __PARSER_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdbool.h>

bool runinbackground(const char* inputbuffer, size_t bufferlen);
size_t trimstring(char* outputbuffer, const char* inputbuffer, size_t bufferlen);
bool isvalidascii(const char* inputbuffer, size_t bufferlen);
size_t firstword(char* outputbuffer, const char* inputbuffer, size_t bufferlen);
void tokenize(char* input, char** args, int* arg_count, size_t bufferlen);
void execute_background_command(char* command, char** args);
char* find_command_path(const char* command, size_t bufferlen);
void replace_home_directory(char* path);
void process_input(char* input, size_t bufferlen);
void execute_command(char** args);

#endif