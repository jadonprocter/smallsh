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
int howManyTokens(char *s)
{
    int count = 0;
    while (strtok(s, " "))
        count++;
    return count;
}

int main()
{
    int exit = 0;

    while (exit == 0)
    {
        // 1b. COMMANDS WILL BE IN FORM: command [args...] [< inputFile] [> outputFile] [&].
        // '&' means the command will be executed in the background.
        // Max length of chars: 2048, Max length of args: 512.
        // no error checking command syntax!!!
        char command[2049] = "cmd a1 a2 a3 a4 < input"; // max char 2048
        char cmd[21];                                   // chars in cmd
        char *args[513];                                // max args 512
        char inputRedirect[501];                        // inputFile
        char outputRedirect[501];                       // outputFile

        char delim[] = " ";

        fflush(stdout); // flush to sanatize output
        // 1a. USE THE COLON AS A PROMPT FOR EACH LINE
        printf(":");
        fflush(stdin); // flush to sanatize input
        // scanf("%s", command);
        if (strcmp(command, "exit") == 0)
        {
            break;
        }

        // TOKENIZE TO GET COMMANDS AND ARGS AND REDIRECTS...
        char *stok = strtok(command, delim);
        strcpy(cmd, stok);
        int argsIndex = 0;

        while (stok != NULL)
        {
            stok = strtok(NULL, delim);
            if (strcmp(stok, "<") == 0)
            {
                stok = strtok(NULL, delim);
                strcpy(inputRedirect, stok);
            }
            else if (strcmp(stok, ">") == 0)
            {

                stok = strtok(NULL, delim);
                strcpy(outputRedirect, stok);
            }
            else
            {
                if (argsIndex < 512)
                {
                    strcpy(args[argsIndex], stok);
                    argsIndex++;
                }
            }
        }
    }

    // 2. COMMENTS AND BLANK LINES
    // Any line that begins with '#' is a comment.
    // A blank line will also do nothing.
    // Just re-prompt.

    // 3. EXPANSION OF VARIABLE "$$" INTO THE PROCESS ID OF SMALLSH ITSELF.

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