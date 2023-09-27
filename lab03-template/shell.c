#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFLEN 1024

int main() {
    char buffer[BUFLEN];
    char* args[2];

    printf("Welcome to the Group17 shell! Enter commands, enter 'quit' to exit\n");
    while (1) {
        // Print the terminal prompt and get input
        printf("$ ");
        char *input = fgets(buffer, sizeof(buffer), stdin);
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

        // Fork a child process to execute the command
        pid_t forkV = fork();
        if (forkV == 0) {
            // In the child process, execute the command
            args[0] = input; // The input is the full path to the executable
            args[1] = NULL;
            if (execve(input, args, NULL) == -1) {
                fprintf(stderr, "Error running command in execve\n");
                return -1;
            }
        } else if (forkV < 0) {
            fprintf(stderr, "Error forking process\n");
            return -1;
        } else {
            // In the parent process, wait for the child to finish
            wait(NULL);
        }
    }

    return 0;
}
