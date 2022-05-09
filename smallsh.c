/*
 * Author: Jadon Procter
 * Program: smallsh
 * Purpose: Implent a small sh by utilizeing process commands and handling.
 */

// HEADER FILES: copied from OSU CS344 Spring 2022 Module 4 & 5
#define _POSIX_C_SOURCE 200809L // getline
#include <stdio.h>              // printf
#include <sys/types.h>          // pid_t, not used in this example
#include <unistd.h>             // getpid, getppid
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <signal.h>

// Global Bools for signals
bool sigamp = false;
bool siginteger = false;

// process struct for storing process information
struct process
{
    pid_t pid;
    int pid_stat;
};

/*
 * Description: toggles a bool to the opposite of its current value
 * Recieves: a pointer to bool
 * Returns: None -- overwrites the bool in place.
 */
void toggleSigBool(bool *b)
{
    *b = !*b;
}

/*
 * Description: handler function for SIGTSTP.
 * Recieves: an int.
 * Returns: None -- call to toggleSigBool.
 */
void sigtstp_handle(int sig)
{
    char fgOnlyOn[] = "\nParent Process running FOREGROUND ONLY mode!\n";
    char fgOnlyOff[] = "\nParent Process EXITED foreground only mode!\n";
    toggleSigBool(&sigamp);
    if (sigamp)
    {
        write(STDOUT_FILENO, fgOnlyOn, 47);
    }
    else
    {
        write(STDOUT_FILENO, fgOnlyOff, 46);
    }
}

/*
 * Description: handler function for SIGINT.
 * Recieves: an int.
 * Returns: None -- call to toggleSigBool.
 */
void sigint_handle(int sig)
{
    toggleSigBool(&siginteger);
    char terminate[] = "\nCurrent forground child process terminated by SIGINT\n";
    write(STDOUT_FILENO, terminate, 55);
}

/*
 * Description: checks is a given value is in a given array
 * Recieves: an int array (pointer), the int length of the array, the int value to be looking for.
 * Returns: true if value in the array, otherwise false.s
 */
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

// WHERE THE MAIN FUNCTIONALITY LIVES.
int main()
{

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

    // Signals: Partially cited from OSU modules for CS344 Spring 2022
    // SIGINT:
    struct sigaction SIGINT_action;
    SIGINT_action.sa_handler = sigint_handle;
    // Block all catchable signals while handle_SIGINT is running
    sigfillset(&SIGINT_action.sa_mask);

    SIGINT_action.sa_flags = SA_RESTART; // restart
    sigaction(SIGINT, &SIGINT_action, NULL);

    // SIGTSTP:
    struct sigaction SIGTSTP_action;
    SIGTSTP_action.sa_handler = sigtstp_handle;
    // Block all catchable signals while handle_SIGINT is running
    sigfillset(&SIGTSTP_action.sa_mask);

    SIGTSTP_action.sa_flags = SA_RESTART; // restart
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // signal(SIGINT, sigint_handle);
    // signal(SIGTSTP, sigtstp_handle);

    // 'Global values' to help with cli loop.
    pid_t smallshPID = getpid();
    int exit = 0;
    int status = 0;
    struct process procs[100];
    int procsSize = 0;

    // Initialize user input 'command'
    size_t commandSize = 2048;                                // max size of command
    char *command = malloc((commandSize + 1) * sizeof(char)); // char ptr to entire command line

    // Initialize array of arguements to pass to exec command.
    int argArrIndex = 0;
    int argArrSize = 1;
    char **args = malloc(1 * sizeof(char *)); // max args 512

    // Initialize file handling.
    char *inputRedirect; // inputFile
    bool in = false;
    char *outputRedirect; // outputFile
    bool out = false;

    // Initialize the boolean amp to false -- whether or not to run in background.
    bool amp = false;

    // THE MAIN CLI LOOP --------------------------------
    while (exit == 0)
    {

        // 1b. COMMANDS WILL BE IN FORM: command [args...] [< inputFile] [> outputFile] [&].
        // '&' means the command will be executed in the background.
        // Max length of chars: 2048, Max length of args: 512.
        // no error checking command syntax!!!

        // 1a. USE THE COLON AS A PROMPT FOR EACH LINE
        printf(":");
        fflush(stdout);

        // Get user input.
        getline(&command, &commandSize, stdin);
        fflush(stdin);

        // Remove new line.
        command[strlen(command) - 1] = '\0';
        // If command is just null termination:
        if (!strcmp(command, "\0"))
        {
            continue;
        }

        // 3. EXPANSION OF VARIABLE "$$" INTO THE PROCESS ID OF SMALLSH ITSELF.
        expand(command, smallshPID);

        // Handle exit (requirement 4).
        if (strcmp(command, "exit") == 0)
        {
            break;
        }

        // 2. COMMENTS AND BLANK LINES
        // Any line that begins with '#' is a comment.
        // A blank line will also do nothing.
        // Just re-prompt.

        char *token = strtok(command, " ");

        if (strcmp(token, "#"))
        {
            // Add first token to arg 0.
            args[0] = malloc(strlen(token) * sizeof(char));
            strcpy(args[0], token);
            args[0][strlen(args[0])] = '\0';
            argArrIndex++;
            argArrSize++;

            // While loop till end of command.
            while ((token = strtok(NULL, " ")))
            {

                // Get input redirection if any.
                if (strcmp(token, "<") == 0)
                {
                    token = strtok(NULL, " ");
                    inputRedirect = malloc(strlen(token) * sizeof(char));
                    strcpy(inputRedirect, token);
                    inputRedirect[strlen(inputRedirect)] = '\0'; // null terminate
                    in = true;
                }
                // Get output redirection if any.
                else if (strcmp(token, ">") == 0)
                {
                    token = strtok(NULL, " ");
                    outputRedirect = malloc(strlen(token) * sizeof(char));
                    strcpy(outputRedirect, token);
                    outputRedirect[strlen(outputRedirect)] = '\0'; // null terminate
                    out = true;
                }
                // Check if too run in the background.
                else if (!strcmp(token, "&"))
                {
                    amp = true;
                }
                // Add the token to the array of argumants.
                else
                {
                    args = realloc(args, argArrSize * sizeof(char *));        // Resize the heap mem of args.
                    args[argArrIndex] = malloc(strlen(token) * sizeof(char)); // Allocate memory
                    strcpy(args[argArrIndex], token);                         // copy into args array at index.
                    args[argArrIndex][strlen(args[argArrIndex])] = '\0';      // null terminate
                    argArrIndex++;
                    argArrSize++;
                }
            }
            args = realloc(args, argArrSize * sizeof(char *));
            args[argArrIndex] = NULL; // NULL terminate the array for exec.
        }
        else
        {
            continue;
        }

        // 6. INPUT AND OUTPUT REDIRECTION.
        // Input redirected on stdin only open for reading. Print error and status 1 smallsh if unable.
        // Output redirected on stdout only open for writing. Print error and status 1 smallsh if unable.
        // Both can be redirected at the same time.

        // Save In and Out to regain cli.
        int saveIn = dup(0);
        int saveOut = dup(1);
        int inFile;
        int outFile;

        if (in)
        {
            // Open in file
            inFile = open(inputRedirect, O_RDONLY | O_CLOEXEC);
            if (inFile == -1)
            {
                perror("open() inputRedirect failed.");
                status = -1;
            }

            // Make in file replace stdin.
            int inRedirectResult = dup2(inFile, 0);
            if (inRedirectResult == -1)
            {
                perror("dup2() inputRedirect failed.");
                status = -1;
            }
        }
        if (out)
        {
            // open output file.
            outFile = open(outputRedirect, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC);
            if (outFile == -1)
            {
                perror("open() outputRedirect failed.");
                status = -1;
            }

            // Make out file replace stdout.
            int outRedirectResult = dup2(outFile, 1);
            if (outRedirectResult == -1)
            {
                perror("dup2() outputRedirect failed.");
                status = -1;
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
            if (argArrIndex == 2)
            {
                // If not to go up or current dir
                if (strcmp(args[1], "..") || strcmp(args[1], "."))
                {
                    int cdStat = chdir(args[1]);

                    if (cdStat < 0)
                    {
                        printf("Error: %s is not a directory", args[0]);
                        fflush(stdout);
                    }
                }
                // to go up a dir
                else if (!strcmp(args[1], ".."))
                {
                    chdir("..");
                }
                // stay in current dir
                else if (!strcmp(args[1], "."))
                {
                    chdir(".");
                }
            }
            // use HOME env var to go HOME.
            else
            {
                chdir(getenv("HOME"));
            }
        }
        // STATUS. - prints the exit status or the last foreground process status. (ignore 3 built in commands).
        else if (strcmp(args[0], "status") == 0)
        {
            printf("exit value %d\n", status);
            fflush(stdout);
        }
        else
        {

            // 5. EXECUTING OTHER COMMANDS.
            // When non-built-in command fork() a child process. the child will then use exec().
            // Your shell should use PATH and allow shell scripts to be exectuted.
            // If no command found then exit status 1.
            // terminate child process

            // Fork child process.
            int childProcessStatus;
            pid_t childProcess = fork();

            // error in fork()
            if (childProcess == -1)
            {
                perror("fork() failed");
                status = -1;
                return -1;
            }
            // fork worked -- child process here.
            else if (childProcess == 0)
            {

                status = 0;
                // if in background.
                if (amp == true)
                {
                    // If no input redirect on a background command redirect will be /dev/null.
                    // If no output redirect on a background command redirect will be /dev/null.
                    if (!in)
                    {
                        int inFD = open("/dev/null", O_RDWR | O_CLOEXEC);
                        dup2(inFD, 0);
                        in = true;
                    }
                    if (!out)
                    {
                        int outFD = open("/dev/null", O_RDWR | O_CLOEXEC);
                        dup2(outFD, 1);
                        out = true;
                    }
                }

                if (siginteger == true)
                {
                    status = 1;
                    return 0;
                }

                execvp(args[0], args);

                status = -1;
                perror("execvp() failed");
                return -1;
            }
            // parent process
            else
            {
                // 7. FOREGROUND AND BACKGROUND COMMANDS.
                // Forground doesn't return foreground access until termination.
                // Background prints pid and on termination prints pid and status.

                // run in foreground
                if (amp == false || sigamp == true)
                {
                    waitpid(childProcess, &childProcessStatus, 0);

                    // check if any bg processes finished.
                    for (int i = 0; i < procsSize; i++)
                    {
                        if (procs[i].pid != 0)
                        {
                            waitpid(-1, &procs[i].pid_stat, WNOHANG);
                            if (WIFEXITED(procs[i].pid_stat))
                            {
                                printf("Ending background task %d with exit %d\n", procs[i].pid, WIFEXITED(procs[i].pid_stat));
                                fflush(stdout);
                            }
                            procs[i].pid = 0;
                        }
                    }
                }
                // run in the background.
                else
                {
                    printf("Starting background task %d\n", childProcess);
                    fflush(stdout);

                    // check if any bg processes finished.
                    for (int i = 0; i < procsSize; i++)
                    {
                        if (procs[i].pid != 0)
                        {
                            waitpid(-1, &procs[i].pid_stat, WNOHANG);
                            if (WIFEXITED(procs[i].pid_stat))
                            {
                                printf("Ending background task %d with exit %d\n", procs[i].pid, WIFEXITED(procs[i].pid_stat));
                                fflush(stdout);
                            }
                            procs[i].pid = 0;
                        }
                    }

                    // add bg process to the list of processes.
                    waitpid(-1, &childProcessStatus, WNOHANG);
                    procs[procsSize].pid = childProcess;
                    procs[procsSize].pid_stat = childProcessStatus;
                    procsSize++;
                }

                if (in)
                {
                    // clean up in file and reset stdin.
                    close(inFile);
                    dup2(saveIn, 0);
                    free(inputRedirect);
                    in = false;
                }
                if (out)
                {
                    // clean up out file and reset stdout.
                    close(outFile);
                    dup2(saveOut, 1);
                    free(outputRedirect);
                    out = false;
                }
                status = 0;
            }
        }

        // FREE ARGS
        for (int i = 0; i < argArrIndex; i++)
        {
            free(args[i]);
        }
        // RESET AND REALLOCATE ARGS.
        argArrIndex = 0;
        argArrSize = 1;
        args = realloc(args, argArrSize * sizeof(char *));

        // amp back to false for next loop.
        amp = false;

        // reset integer signal for loop.
        toggleSigBool(&siginteger);
    }

    // FREE MEMORY
    free(args);
    free(command);

    return 0;
}