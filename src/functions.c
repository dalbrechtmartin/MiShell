/** @file functions.c
 *  @brief Implementation of built-in shell and others functions.
 */

#include "../include/functions.h"

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

/** @brief Exits the shell.
 * @return EXIT_SUCCESS.
 */
int exit_cmd()
{
    printf("Exiting MiShell...\n");
    exit(EXIT_SUCCESS);
}

/** @brief Parses a command line into a ParsedCommand structure.
 * @param line The command line input.
 * @param result Pointer to a ParsedCommand structure to store the parsed result.
 */
void parse_command(char *line, ParsedCommand *result)
{
    result->num_cmds = 0;
    result->is_background = 0;

    for (int i = 0; i < MAX_CMDS; i++)
    {
        result->ops[i] = NULL;
    }

    char *copy = strdup(line);
    if (!copy)
    {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }

    copy[strcspn(copy, "\n")] = 0;

    char *background = strstr(copy, " &");
    if (background)
    {
        *background = 0;
        result->is_background = 1;
    }

    char *token = strtok(copy, "&&||");
    while (token && result->num_cmds < MAX_CMDS)
    {
        while (*token == ' ')
            token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ')
            *end-- = 0;

        char *arg = strtok(token, " ");
        int arg_count = 0;
        while (arg && arg_count < BUFFER_MAX_SIZE - 1)
        {
            result->cmds[result->num_cmds].args[arg_count++] = strdup(arg);
            arg = strtok(NULL, " ");
        }
        result->cmds[result->num_cmds].args[arg_count] = NULL;
        result->cmds[result->num_cmds].num_args = arg_count;
        result->num_cmds++;

        token = strtok(NULL, "&&||");
        if (token)
        {
            if (strstr(line, "&&"))
            {
                result->ops[result->num_cmds - 1] = "&&";
            }
            else if (strstr(line, "||"))
            {
                result->ops[result->num_cmds - 1] = "||";
            }
            else if (strstr(line, "|"))
            {
                result->ops[result->num_cmds - 1] = "|";
            }
        }
    }

    free(copy);
}

/** @brief Execute the parsed commands.
 * @param command Pointer to the parsed command structure.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_command(ParsedCommand *command)
{
    if (command->num_cmds == 0)
    {
        return SUCCESS; // Empty command
    }

    // For now, handle only the first command (single command)
    char **args = command->cmds[0].args;

    if (args[0] == NULL)
    {
        return SUCCESS; // Empty line
    }

    char *cmd_name = args[0];

    // Check built-in commands
    if (strcmp(cmd_name, "cd") == 0)
    {
        return cd_cmd(args);
    }
    else if (strcmp(cmd_name, "pwd") == 0)
    {
        return pwd_cmd();
    }
    else if (strcmp(cmd_name, "echo") == 0)
    {
        return echo_cmd(args);
    }
    else if (strcmp(cmd_name, "exit") == 0)
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
            if (execvp(cmd_name, args) == -1)
            {
                fprintf(stderr, "Command not found: %s\n", cmd_name);
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
            if (!command->is_background)
            {
                wait(NULL);
            }
        }
        return SUCCESS;
    }
}
