#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "parser.h"
#include "pipe_handler.h"

#define BUFLEN 1024

int main() {
    char buffer[BUFLEN];
    char* args[BUFLEN / 2]; // Max number of arguments is half the buffer size

    printf("Welcome to the Group17 shell! Enter commands, enter 'quit' to exit\n");
    while (1) {
        // Get the current username
        char username[BUFLEN];
        if (getlogin_r(username, sizeof(username)) != 0) {
            perror("getlogin_r");
            return -1;
        }

        // Get the current working directory
        char cwd[BUFLEN];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd");
            return -1;
        }

        // Replace the home directory with "~" in the path
        replace_home_directory(cwd);

        // Print the terminal prompt with the current directory
        printf("@%s: %s$ ", username, cwd);

        char* input = fgets(buffer, sizeof(buffer), stdin);
        if (!input) {
            fprintf(stderr, "Error reading input\n");
            return -1;
        }

        // Remove trailing newline character
        input[strcspn(input, "\n")] = '\0';

        // Handle the "quit" command to exit the shell
        if (strcmp(input, "quit") == 0) {
            printf("Bye!!\n");
            return 0;
        }

        // Check if the input contains a pipe character
        int pipe_position = findpipe(input, BUFLEN);
        if (pipe_position >= 0) {
            // Split the input into two commands based on the pipe character
            char* command1 = input;
            char* command2 = input + pipe_position + 1;

            // Execute the commands on both sides of the pipe
            execute_pipe(command1, command2, BUFLEN);
        } else {
            // Check if the input ends with "&" indicating a background command
            if (runinbackground(input, BUFLEN)) {
                // Remove the "&" from the input
                input[strlen(input) - 1] = '\0';

                // Execute the command as a background command
                process_input(input, BUFLEN);
            } else {
                execute_command(args);
            }
        }
    }

    return 0;
}
