/*
 * Author: Jadon Procter
 * Program: smallsh
 * Purpose: Implent a small sh by utilizeing process commands and handling.
 */

// HEADER FILES: copied from OSU CS344 Spring 2022 Module 4 & 5
#include <stdio.h>     // printf
#include <sys/types.h> // pid_t, not used in this example
#include <unistd.h>    // getpid, getppid
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

void expand(char *s, pid_t p)
{
    size_t length = strlen(s);
    int howManyExpansions = 0;
    int index = 0;
    char *pidToChar = (char *)malloc(10 * sizeof(char));
    sprintf(pidToChar, "%d", p);
    for (int i = 0; i < length - 1; i++)
    {
        if (s[i] == '$' && i != length - 1)
        {
            i++;
            if (s[i] == '$')
            {
                howManyExpansions++;
            }
        }
    }
    for (int i = 0; i < howManyExpansions; i++)
    {
        while (index < length)
        {
            if (s[index] == '$' && index != length - 1)
            {
                if (s[index + 1] == '$')
                {
                    // change "$$" to PID
                    char *tmp = (char *)malloc((strlen(s) + strlen(pidToChar) + 1) * sizeof(char));

                    for (int j = 0; j < index; j++)
                    {
                        tmp[j] = s[j];
                    }
                    strcat(tmp, pidToChar);
                    index += strlen(pidToChar);
                    for (int j = index; j < length + strlen(pidToChar); j++)
                    {
                        tmp[j] = s[j];
                    }
                    strcpy(s, tmp);
                    free(tmp);
                    index = length;
                }
            }
            index++;
        }
    }

    free(pidToChar);
    s[length] = '\0';
}

int main()
{
    pid_t smallshPID = getpid();
    int exit = 0;

    while (exit == 0)
    {
        // 1b. COMMANDS WILL BE IN FORM: command [args...] [< inputFile] [> outputFile] [&].
        // '&' means the command will be executed in the background.
        // Max length of chars: 2048, Max length of args: 512.
        // no error checking command syntax!!!

        char *command;             // char ptr to entire command line
        size_t commandSize = 2049; // max size of command
        command = (char *)malloc(commandSize * sizeof(char));
        char cmd[11];             // chars in cmd
        char *args[512];          // max args 512
        char inputRedirect[501];  // inputFile
        char outputRedirect[501]; // outputFile
        bool amp = false;         // run in background
        char *save = command;     // save ptr for tokenization

        // 1a. USE THE COLON AS A PROMPT FOR EACH LINE
        fflush(stdout); // flush to sanatize output
        printf(":");

        fflush(stdin);                                             // flush to sanatize input
        size_t bytesRead = getline(&command, &commandSize, stdin); // get entire line.

        // remove new line
        command[bytesRead - 1] = '\0';

        // 3. EXPANSION OF VARIABLE "$$" INTO THE PROCESS ID OF SMALLSH ITSELF.
        expand(command, smallshPID);

        // Handle exit (requirement 4).
        if (strcmp(command, "exit") == 0)
        {
            free(command);
            break;
        }

        char *token = strtok_r(save, " ", &save);

        // 2. COMMENTS AND BLANK LINES
        // Any line that begins with '#' is a comment.
        // A blank line will also do nothing.
        // Just re-prompt.
        if (strcmp(token, "#"))
        {
            strcpy(cmd, token);
            int argArrIndex = 0;

            while ((token = strtok_r(save, " ", &save)))
            {
                if (strcmp(token, "<") == 0)
                {
                    token = strtok_r(save, " ", &save);
                    strcpy(inputRedirect, token);
                }
                else if (strcmp(token, ">") == 0)
                {
                    token = strtok_r(save, " ", &save);
                    strcpy(outputRedirect, token);
                }
                else if (strcmp(token, "#") == 0)
                {
                    break;
                }
                else
                {
                    int argLength = strlen(token);
                    args[argArrIndex] = (char *)malloc(argLength * sizeof(char) + 1);
                    strcpy(args[argArrIndex], token);
                    argArrIndex++;
                }
            }

            // 4. BUILT IN COMMANDS: EXIT, CD, AND STATUS.
            // Don't need input and output redirection for these commands.
            // No set exit status.
            // These commands with '&' will be ignored.
            // EXIT. - no args, leaves shell. -- handled above
            // CD. - with no args, goes home. otherwise goes to specified path.
            if (strcmp(cmd, "cd") == 0)
            {
                if (argArrIndex == 1)
                {
                    char *currDir = getcwd(args[0], strlen(args[0]));
                    chdir(currDir);
                }
                else
                {
                    chdir(getenv("HOME"));
                }
            }
            // STATUS. - prints the exit status or the last foreground process status. (ignore 3 built in commands).

            printf("cmd: %s ", cmd);
            for (int i = 0; i < argArrIndex; i++)
            {
                printf(" arg: %s ", args[i]);
            }
            printf("\n");
            // printf("infile: %s outfile: %s\n", inputRedirect, outputRedirect);

            for (int i = 0; i < argArrIndex; i++)
            {
                free(args[i]);
            }
        }
    }

    // 4. BUILT IN COMMANDS: EXIT, CD, AND STATUS.
    // Don't need input and output redirection for these commands.
    // No set exit status.
    // These commands with '&' will be ignored.
    // EXIT. - no args, leaves shell.
    // CD. - with no args, goes home. otherwise goes to specified path.
    // STATUS. - prints the exit status or the last foreground process status. (ignore 3 built in commands).

    // 5. EXECUTING OTHER COMMANDS.
    // When non-built-in command fork() a child process. the child will then use exec().
    // Your shell should use PATH and allow shell scripts to be exectuted.
    // If no command found then exit status 1.
    // terminate child process

    // 6. INPUT AND OUTPUT REDIRECTION.
    // Input redirected on stdin only open for reading. Print error and status 1 smallsh if unable.
    // Output redirected on stdout only open for writing. Print error and status 1 smallsh if unable.
    // Both can be redirected at the same time.

    // 7. FOREGROUND AND BACKGROUND COMMANDS.
    // Forground doesn't return foreground access until termination.
    // Background prints pid and on termination prints pid and status.
    // If no input redirect on a background command redirect will be /dev/null.
    // If no output redirect on a background command redirect will be /dev/null.

    // 8. SIGNALS: SIGINT & SIGTSTP.
    // SIGINT:
    // Ctrl-C will send SIGINT to parent process and all children at the same time.
    // Children terminate self in FG and if in BG process ignore SIGINT.
    // Parent FG process will print out signal.
    // SIGTSTP:
    // FG child processes ignore.
    // BG child processes ignore.
    // Parent must display an informative message.
    // Shell will then no longer run background processes.
    // '&' will be ignored.
    // A second SIGTSTP will return the shell to normal.
    return 0;
}