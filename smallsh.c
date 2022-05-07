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

bool inArray(int arr[], int arrLength, int value)
{
    for (int i = 0; i < arrLength; i++)
    {
        if (arr[i] == value)
        {
            return true;
        }
    }
    return false;
}

/*
 * Description: expands any instance of $$ into the current pid.
 * Recieves: a pointer to char array, a pid.
 * Returns: None -- overwrites the string in place.
 */
void expand(char *s, pid_t p)
{
    size_t length = strlen(s); // length of command

    // turn pid_t to a char array.
    char *pidToChar = (char *)malloc(10 * sizeof(char));
    sprintf(pidToChar, "%d", p);

    int howManyExpansions = 0; // holds the amount of "$$" found in the command (in s).

    // saves the indicies of where the "$$" are located.
    int *dollarArray = (int *)malloc(howManyExpansions * sizeof(int));
    int dollarIndex = 0;

    // finds instances of "$$" and updates the values needed.
    for (int i = 0; i < length - 1; i++)
    {
        if (s[i] == '$' && i != length - 1)
        {
            i++;
            if (s[i] == '$')
            {
                howManyExpansions++;
                dollarArray[dollarIndex] = i - 1;
                dollarIndex++;
            }
        }
    }

    // create a tmp char array to hold the new expanded command.
    int lengthOfTmp = ((((strlen(pidToChar) - 2) * howManyExpansions) + length + 1));
    char *tmp = (char *)malloc(lengthOfTmp * sizeof(char));

    int sOffset = 0; // holds the index of the s string.

    // fill the tmp char array  expanding the instances of "$$".
    for (int i = 0; i < lengthOfTmp; i++)
    {
        if (inArray(dollarArray, howManyExpansions, i))
        {
            strcat(tmp, pidToChar);
            i += strlen(pidToChar) - 1;
            sOffset += 2;
        }
        else
        {
            tmp[i] = s[sOffset];
            sOffset++;
        }
    }
    tmp[strlen(tmp)] = '\0'; // null terminate the string
    strcpy(s, tmp);          // overwrite s with the expanded array
    // printf("%s", s);
    fflush(stdout);
    // free memory used on the heap.
    free(dollarArray);
    free(tmp);
    free(pidToChar);
}

int main()
{
    pid_t smallshPID = getpid();
    int exit = 0;
    int status = 0; // status save
    while (exit == 0)
    {
        // 1b. COMMANDS WILL BE IN FORM: command [args...] [< inputFile] [> outputFile] [&].
        // '&' means the command will be executed in the background.
        // Max length of chars: 2048, Max length of args: 512.
        // no error checking command syntax!!!

        size_t commandSize = 2049;                                  // max size of command
        char *command = (char *)malloc(commandSize * sizeof(char)); // char ptr to entire command line

        char **args = (char **)malloc(1 * sizeof(char)); // max args 512
        int argArrIndex = 0;                             // number of arguments

        char *inputRedirect; // inputFile
        bool in = false;
        char *outputRedirect; // outputFile
        bool out = false;

        bool amp = false;     // run in background
        char *save = command; // save ptr for tokenization

        // 1a. USE THE COLON AS A PROMPT FOR EACH LINE

        printf(":");
        fflush(stdout); // flush to sanatize output

        size_t bytesRead = getline(&command, &commandSize, stdin); // get entire line.
        fflush(stdin);                                             // flush to sanatize inputs

        // remove new line
        command[bytesRead - 1] = '\0';
        // if command is just null termination:
        if (!strcmp(command, "\0"))
        {

            free(command);
            free(args);
            continue;
        }

        // 3. EXPANSION OF VARIABLE "$$" INTO THE PROCESS ID OF SMALLSH ITSELF.
        expand(command, smallshPID);

        // Handle exit (requirement 4).
        if (strcmp(command, "exit") == 0)
        {
            for (int i = 0; i < argArrIndex; i++)
            {
                free(args[i]);
            }
            if (in)
            {
                free(inputRedirect);
            }
            if (out)
            {
                free(outputRedirect);
            }
            free(args);
            free(command);
            break;
        }

        // 2. COMMENTS AND BLANK LINES
        // Any line that begins with '#' is a comment.
        // A blank line will also do nothing.
        // Just re-prompt.

        char *token = strtok_r(save, " ", &save);

        if (strcmp(token, "#"))
        {
            args[0] = (char *)malloc((strlen(token) + 1) * sizeof(char));
            strcpy(args[0], token);
            args[0][strlen(token)] = '\0';
            argArrIndex++;

            while ((token = strtok_r(save, " ", &save)))
            {
                int argLength = strlen(token);                                    // length of argument.
                args = realloc(args, (argArrIndex + 2) * sizeof(char));           // realloc args  array size.
                args[argArrIndex] = (char *)malloc(argLength * sizeof(char) + 1); // malloc the index arg array.
                token[strlen(token)] = '\0';                                      // null terminate string
                strcpy(args[argArrIndex], token);                                 // coopy into args array at index.
                argArrIndex++;

                if (strcmp(token, "<") == 0)
                {
                    token = strtok_r(save, " ", &save);
                    inputRedirect = (char *)malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(inputRedirect, token);
                    inputRedirect[strlen(inputRedirect)] = '\0'; // null terminate
                    in = true;
                }
                else if (strcmp(token, ">") == 0)
                {
                    token = strtok_r(save, " ", &save);
                    outputRedirect = (char *)malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(outputRedirect, token);
                    outputRedirect[strlen(outputRedirect)] = '\0'; // null terminate
                    out = true;
                }
            }
            args = realloc(args, (argArrIndex + 2) * sizeof(char));
            args[argArrIndex] = NULL; // null terminate args array for future use in exec().
        }
        else
        {
            for (int i = 0; i < argArrIndex; i++)
            {
                free(args[i]);
            }
            if (in)
            {
                free(inputRedirect);
            }
            if (out)
            {
                free(outputRedirect);
            }
            free(args);
            free(command);
            continue;
        }

        // 6. INPUT AND OUTPUT REDIRECTION.
        // Input redirected on stdin only open for reading. Print error and status 1 smallsh if unable.
        // Output redirected on stdout only open for writing. Print error and status 1 smallsh if unable.
        // Both can be redirected at the same time.

        if (in)
        {
            int inFile = open(inputRedirect, O_RDONLY);
            if (inFile == -1)
            {
                perror("open() inputRedirect failed.");
                status = -1;
                return -1;
            }
            fcntl(inFile, F_SETFD, FD_CLOEXEC); // close on exec call

            int inRedirectResult = dup2(inFile, 0);
            if (inRedirectResult == -1)
            {
                perror("dup2() inputRedirect failed.");
                status = -1;
                return -1;
            }
        }
        if (out)
        {
            int outFile = open(outputRedirect, O_WRONLY);
            if (outFile == -1)
            {
                perror("open() outputRedirect failed.");
                status = -1;
                return -1;
            }
            fcntl(outFile, F_SETFD, FD_CLOEXEC); // close on exec call

            int outRedirectResult = dup2(outFile, 1);
            if (outRedirectResult == -1)
            {
                perror("dup2() outputRedirect failed.");
                status = -1;
                return -1;
            }
        }

        // 4. BUILT IN COMMANDS: EXIT, CD, AND STATUS.
        // Don't need input and output redirection for these commands.
        // No set exit status.
        // These commands with '&' will be ignored.
        // EXIT. - no args, leaves shell. -- handled above
        // CD. - with no args, goes home. otherwise goes to specified path.
        if (strcmp(args[0], "cd") == 0)
        {
            char dir[100];
            getcwd(dir, 100);
            printf("%s\n", dir);
            if (argArrIndex == 2)
            {
                if (strcmp(args[1], "..") || strcmp(args[1], "."))
                {
                    int cdStat = chdir(args[1]);

                    char dir1[100];
                    getcwd(dir1, 100);
                    printf("%s\n", dir1);

                    if (cdStat < 0)
                    {
                        printf("Error: %s is not a directory", args[0]);
                        fflush(stdout);
                    }
                }
                else if (!strcmp(args[1], ".."))
                {
                    chdir("..");
                    char dir2[100];
                    getcwd(dir2, 100);
                    printf("%s\n", dir2);
                }
                else if (!strcmp(args[1], "."))
                {

                    chdir(".");
                    char dir3[100];
                    getcwd(dir3, 100);
                    printf("%s\n", dir3);
                }
            }
            else
            {
                chdir(getenv("HOME"));
            }
        }
        // STATUS. - prints the exit status or the last foreground process status. (ignore 3 built in commands).
        else if (strcmp(args[0], "status") == 0)
        {
            printf("STATUS is: %d\n", status);
            fflush(stdout);
        }
        else
        {

            // 5. EXECUTING OTHER COMMANDS.
            // When non-built-in command fork() a child process. the child will then use exec().
            // Your shell should use PATH and allow shell scripts to be exectuted.
            // If no command found then exit status 1.
            // terminate child process

            int childProcessStatus;
            pid_t childProcess = fork();

            if (childProcess == -1)
            {
                perror("fork() failed");
                status = -1;
                return -1;
            }
            else if (childProcess == 0)
            {
                printf("starting child process %d\n", getpid());
                fflush(stdout);
                status = 0;
                execvp(args[0], args);

                status = -1;
                perror("execvp() failed");
                return 1;
            }
            else
            {
                printf("Parent Process %d\n", getpid());
                fflush(stdout);
                // waitpid(childProcess, &childProcessStatus, 0);
                wait(&childProcessStatus);
                printf("Finished Child Process %d\n", childProcess);
                fflush(stdout);
                status = 1;
            }
        }

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

        // FREE MEMORY
        for (int i = 0; i < argArrIndex - 1; i++)
        {
            free(args[i]);
        }
        if (in)
        {
            free(inputRedirect);
        }
        if (out)
        {
            free(outputRedirect);
        }
        free(args);
        free(command);
    }

    return 0;
}