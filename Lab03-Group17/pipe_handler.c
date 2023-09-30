#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "parser.h"

//Command to find location of pipe character in input string
//[Input] char* inputbuffer - input string to test
//[Input] size_t bufferlen - size of input buffer
//[Return] int - location in the string of the pipe character, or -1 pipe character not found
int findpipe(const char* inputbuffer, size_t bufferlen) {
    const char* pipePos = strchr(inputbuffer, '|');
    
    if (pipePos != NULL) {
        // Return the position of the pipe character
        return pipePos - inputbuffer;
    } else {
        // Pipe character not found
        return -1;
    }
}

//Command to execute two commands separated by a pipe
//[Input] char* first_command - first command to execute
//[Input] char* second_command - second command to execute
//[Input] size_t bufferlen - size of input buffer
void execute_pipe(char* first_command, char* second_command, size_t bufferlen) {
    char* first_args[bufferlen / 2];
    char* second_args[bufferlen / 2];

    // Tokenize the first command
    int first_arg_count;
    tokenize(first_command, first_args, &first_arg_count, bufferlen);

    // Tokenize the second command
    int second_arg_count;
    tokenize(second_command, second_args, &second_arg_count, bufferlen);

    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t first_child = fork();

    if (first_child == 0) {
        // In the first child process, close the read end of the pipe
        close(pipe_fd[0]);

        // Redirect standard output to the write end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]); // Close the write end of the pipe

        // Execute the first command
        char* first_full_path = find_command_path(first_args[0], bufferlen);

        if (first_full_path != NULL) {
            if (execve(first_full_path, first_args, NULL) == -1) {
                perror("execve");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Command not found: %s\n", first_args[0]);
            exit(EXIT_FAILURE);
        }
    } else if (first_child < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    pid_t second_child = fork();

    if (second_child == 0) {
        // In the second child process, close the write end of the pipe
        close(pipe_fd[1]);

        // Redirect standard input to the read end of the pipe
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]); // Close the read end of the pipe

        // Execute the second command
        char* second_full_path = find_command_path(second_args[0], bufferlen);

        if (second_full_path != NULL) {
            if (execve(second_full_path, second_args, NULL) == -1) {
                perror("execve");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Command not found: %s\n", second_args[0]);
            exit(EXIT_FAILURE);
        }
    } else if (second_child < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Close both ends of the pipe in the parent process
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    // Wait for both child processes to finish
    waitpid(first_child, NULL, 0);
    waitpid(second_child, NULL, 0);
}
