#include "parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Function to test whether the input string ends with "&" and
// thus represents a command that should be run in the background
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] bool - true if string ends with "&"
bool runinbackground(const char* inputbuffer, size_t bufferlen) {
    size_t length = strlen(inputbuffer);
    if (length > 0 && inputbuffer[length - 1] == '&') {
        return true;
    }
    return false;
}

//Command to trim whitespace and ASCII control characters from buffer
//[Input] char* inputbuffer - input string to trim
//[Input] size_t bufferlen - size of input and output string buffers
//[Output] char* outputbuffer - output string after trimming 
//[Return] size_t - size of output string after trimming
size_t trimstring(char* outputbuffer, const char* inputbuffer, size_t bufferlen){   
    memcpy(outputbuffer, inputbuffer, bufferlen*sizeof(char));

    for(size_t ii = strlen(outputbuffer)-1; ii >=0; ii--){
        if(outputbuffer[ii] < '!') //In ASCII '!' is the first printable (non-control) character
        {
            outputbuffer[ii] = 0;
        }else{
            break;
        }    
    }

    return strlen(outputbuffer);
}

//Command to test that string only contains valid ascii characters (non-control and not extended)
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] bool - true if no invalid ASCII characters present
bool isvalidascii(const char* inputbuffer, size_t bufferlen){
    size_t testlen = bufferlen;
    size_t stringlength = strlen(inputbuffer);
    if (stringlength < bufferlen) {
        testlen = stringlength;
    }

    bool isValid = true;
    for (size_t ii = 0; ii < testlen; ii++) {
        char currentChar = inputbuffer[ii];
        // Allow printable ASCII characters, spaces, and newline characters
        isValid &= (currentChar >= ' ' && currentChar <= '~') || currentChar == ' ';
    }

    return isValid;
}

//Command to trim the input command to just be the first word
//[Input] char* inputbuffer - input string to trim
//[Input] size_t bufferlen - size of input and output string buffers
//[Output] char* outputbuffer - output string after trimming 
//[Return] size_t - size of output string after trimming
size_t firstword(char* outputbuffer, const char* inputbuffer, size_t bufferlen)
{
    // Initialize output buffer
    memset(outputbuffer, 0, bufferlen);

    // Skip leading whitespaces
    while (*inputbuffer && isspace(*inputbuffer)) {
        inputbuffer++;
    }

    // Copy the first word to the output buffer
    size_t i = 0;
    while (*inputbuffer && !isspace(*inputbuffer) && i < bufferlen - 1) {
        outputbuffer[i] = *inputbuffer;
        inputbuffer++;
        i++;
    }

    return strlen(outputbuffer);
}

// Function to split a string into tokens
//[Input] char* input - input string to tokenize
//[Input] char** args - array of strings to store the tokens
//[Input] int* arg_count - pointer to an integer to store the number of tokens
//[Input] size_t bufferlen - size of input and output string buffers
void tokenize(char* input, char** args, int* arg_count, size_t bufferlen) {
    *arg_count = 0;
    char* token = strtok(input, " ");
    
    while (token != NULL) {
        // Trim the token to remove whitespace and control characters
        char trimmedToken[bufferlen];
        size_t tokenLength = trimstring(trimmedToken, token, bufferlen);
        
        // Check if the token contains valid ASCII characters
        if (isvalidascii(trimmedToken, tokenLength)) {
            args[(*arg_count)++] = strdup(trimmedToken);
        } else {
            fprintf(stderr, "Invalid characters in token: %s\n", token);
        }
        
        token = strtok(NULL, " ");
    }
    args[*arg_count] = NULL;  // Mark the end of arguments with NULL
}

// Function to execute a background command
//[Input] char* command - command to execute
//[Input] char** args - array of strings containing the arguments to the command
void execute_background_command(char* command, char** args) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
        perror("fork");
        return;
    }

    if (child_pid == 0) {
        // In the child process, execute the command using execve
        if (execvp(command, args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // In the parent process, print a message about the background command
        printf("Background command %d started\n", child_pid);
    }
}

// Function to find the full path of a command based on PATH variable
//[Input] char* command - command to find
//[Input] size_t bufferlen - size of input and output string buffers
//[Return] char* - full path of the command
char* find_command_path(const char* command, size_t bufferlen) {
    // Get the PATH environment variable
    char* path = getenv("PATH");
    if (path == NULL) {
        fprintf(stderr, "PATH environment variable is not set.\n");
        return NULL;
    }

    char* path_copy = strdup(path);
    char* token = strtok(path_copy, ":");
    
    while (token != NULL) {
        // Check if the command exists in the current PATH directory
        char full_path[bufferlen];
        memset(full_path, 0, bufferlen);

        size_t tokenLength = firstword(full_path, token, bufferlen);
        
        if (tokenLength > 0) {
            char trimmedCommand[bufferlen];
            size_t commandLength = firstword(trimmedCommand, command, bufferlen);
            
            if (commandLength > 0 && isvalidascii(trimmedCommand, commandLength)) {
                snprintf(full_path, sizeof(full_path), "%s/%s", token, trimmedCommand);
                
                // Check if the file at full_path is executable
                if (access(full_path, X_OK) == 0) {
                    free(path_copy); // Free the copied PATH string
                    return strdup(full_path);
                }
            }
        }

        token = strtok(NULL, ":");
    }

    free(path_copy); // Free the copied PATH string
    return NULL; // Command not found in any PATH directory
}

// Function to replace the home directory with "~" in a path
//[Input] char* path - path to modify
void replace_home_directory(char* path) {
    char* home = getenv("HOME");
    if (home != NULL && strncmp(path, home, strlen(home)) == 0) {
        memmove(path, "~", 1);
        memmove(path + 1, path + strlen(home), strlen(path) - strlen(home) + 1);
    }
}

// Function to execute a background command
//[Input] char* command - command to execute
//[Input] char** args - array of strings containing the arguments to the command
void process_input(char* input, size_t bufferlen) {
    // Tokenize the input string
    int arg_count;
    char* args[bufferlen / 2];
    tokenize(input, args, &arg_count, bufferlen);

    // Check if the last argument is "&" indicating a background command
    if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
        // Remove the "&" from the arguments
        args[arg_count - 1] = NULL;
        arg_count--;

        // Execute the background command
        execute_background_command(args[0], args);
    } else {
        // Execute the command as a foreground command
        execute_command(args);
    }
}

// Function to execute a command
//[Input] char** args - array of strings containing the arguments to the command
void execute_command(char** args) {
    // Fork a child process to execute the command
    pid_t forkV = fork();
    if (forkV == 0) {
        // In the child process, execute the command using execve
        if (execve(args[0], args, NULL) == -1) {
            fprintf(stderr, "Error running command in execve\n");
            exit(EXIT_FAILURE); // Exit the child process with an error status
        }
    } else if (forkV < 0) {
        fprintf(stderr, "Error forking process\n");
        exit(EXIT_FAILURE); // Exit the child process with an error status
    } else {
        // In the parent process, wait for the child to finish
        wait(NULL);
    }
}
