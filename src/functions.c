/** @file functions.c
 *  @brief Implementation of built-in shell and others functions.
 */

#include "../include/functions.h"

/** @section BUILTIN_COMMANDS Built-in Commands
 *  This section contains implementations of built-in shell commands.
 */

/** @subsection CD_COMMAND CD Command
 *  Implementation of the cd command.
 */

/** @brief Changes the current directory.
 * @param args Array of arguments where args[1] is the target directory.
 * @return SUCCESS on success, FAILURE on failure.
 */
int cd_cmd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "cd: missing argument\n");
        return FAILURE;
    }
    if (chdir(args[1]) != 0)
    {
        fprintf(stderr, "cd: %s: No such file or directory\n", args[1]);
        return FAILURE;
    }
    return SUCCESS;
}

/** @subsection PWD_COMMAND PWD Command
 *  Implementation of the pwd command.
 */

/** @brief Prints the current working directory.
 * @return SUCCESS on success, FAILURE on failure.
 */
int pwd_cmd()
{
    char cwd[BUFFER_MAX_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(stderr, "pwd: %s\n", strerror(errno));
        return FAILURE;
    }
    printf("%s\n", cwd);
    return SUCCESS;
}

/** @subsection ECHO_COMMAND ECHO Command
 *  Implementation of the echo command.
 */

/** @brief Prints the given arguments to the standard output.
 * @param args Array of arguments to be printed.
 * @return SUCCESS on success, FAILURE on failure.
 */
int echo_cmd(char **args)
{
    int i = 1;
    while (args[i] != NULL)
    {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");
    return SUCCESS;
}

/** @subsection EXIT_COMMAND EXIT Command
 *  Implementation of the exit command.
 */

/** @brief Exits the shell.
 * @return EXIT_SUCCESS.
 */
int exit_cmd()
{
    printf("Exiting MiShell...\n");
    exit(EXIT_SUCCESS);
}
/** @endsection */

/** @section BASIC_FUNCTIONS Basic Functions
 *  This section contains implementations of basic functions used by the shell.
 */

/** @brief Parse the user input to verify if is a valid command and execute it.
 * @param line User input.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_command(char *line)
{
    // Parse the line into args
    char *args[BUFFER_MAX_SIZE];
    int arg_count = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && arg_count < 9)
    {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;

    if (args[0] == NULL)
    {
        return SUCCESS; // Empty line
    }

    char *command = args[0];

    // Check built-in commands
    if (strcmp(command, "cd") == 0)
    {
        return cd_cmd(args);
    }
    else if (strcmp(command, "pwd") == 0)
    {
        return pwd_cmd();
    }
    else if (strcmp(command, "echo") == 0)
    {
        return echo_cmd(args);
    }
    else if (strcmp(command, "exit") == 0)
    {
        return exit_cmd();
    }
    else
    {
        // External command: fork and exec
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process
            if (execvp(command, args) == -1)
            {
                fprintf(stderr, "Command not found: %s\n", command);
                exit(EXIT_FAILURE);
            }
        }
        else if (pid < 0)
        {
            perror("fork");
            return FAILURE;
        }
        else
        {
            // Parent process
            wait(NULL);
        }
        return SUCCESS;
    }
}

/** @endsection */
