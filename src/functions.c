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
    memset(result, 0, sizeof(*result));
    for (int i = 0; i < MAX_CMDS - 1; i++)
    {
        result->ops[i] = OP_NONE;
    }

    char *copy = strdup(line);
    if (!copy)
    {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }
    copy[strcspn(copy, "\n")] = 0;

    // Background detection: ignore trailing '&' while keeping logical operators intact
    char *bg = strrchr(copy, '&');
    if (bg && (bg == copy || *(bg - 1) != '&'))
    {
        *bg = '\0';
        result->is_background = 1;
    }

    char *cursor = copy;
    while (*cursor != '\0' && result->num_cmds < MAX_CMDS)
    {
        while (isspace((unsigned char)*cursor))
        {
            cursor++;
        }
        if (*cursor == '\0')
        {
            break;
        }

        // Split only on "&&"; everything else (including pipes) stays inside the command
        char *op_pos = strstr(cursor, "&&");
        char *segment = cursor;
        char *next = op_pos ? op_pos + 2 : cursor + strlen(cursor);
        if (op_pos)
        {
            *op_pos = '\0';
        }

        // Trim right-side spaces from the segment
        char *end = segment + strlen(segment);
        while (end > segment && isspace((unsigned char)*(end - 1)))
        {
            *(--end) = '\0';
        }

        if (*segment == '\0')
        {
            cursor = next;
            continue;
        }

        Command *cmd = &result->cmds[result->num_cmds];
        cmd->input_file = NULL;
        cmd->output_file = NULL;
        cmd->output_append = 0;
        cmd->pipeline_cmd = NULL;

        // If the segment contains a pipe, keep it raw and delegate to the shell
        if (strstr(segment, "|"))
        {
            cmd->pipeline_cmd = strdup(segment);
            cmd->args[0] = NULL;
            cmd->num_args = 0;
        }
        else
        {
            int arg_idx = 0;
            char *token = strtok(segment, " ");
            while (token && arg_idx < BUFFER_MAX_SIZE - 1)
            {
                if (strcmp(token, "<") == 0)
                {
                    token = strtok(NULL, " ");
                    if (token)
                    {
                        cmd->input_file = token;
                    }
                }
                else if (strcmp(token, ">") == 0)
                {
                    token = strtok(NULL, " ");
                    if (token)
                    {
                        cmd->output_file = token;
                        cmd->output_append = 0;
                    }
                }
                else if (strcmp(token, ">>") == 0)
                {
                    token = strtok(NULL, " ");
                    if (token)
                    {
                        cmd->output_file = token;
                        cmd->output_append = 1;
                    }
                }
                else
                {
                    cmd->args[arg_idx++] = strdup(token);
                }
                token = strtok(NULL, " ");
            }
            cmd->args[arg_idx] = NULL;
            cmd->num_args = arg_idx;
        }

        result->num_cmds++;
        if (op_pos && result->num_cmds < MAX_CMDS)
        {
            result->ops[result->num_cmds - 1] = OP_AND;
        }
        cursor = next;
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

    if (command->num_cmds == 1)
    {
        return execute_single_command(&command->cmds[0], command->is_background);
    }

    int status = SUCCESS;
    for (int i = 0; i < command->num_cmds; i++)
    {
        int result = execute_single_command(&command->cmds[i], command->is_background);
        if (result != SUCCESS)
        {
            status = FAILURE;
        }
    }
    return status;
}

/** @brief Execute a single command with redirections.
 * @param cmd Pointer to the command to execute.
 * @param background 1 if background, 0 otherwise.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_single_command(Command *cmd, int background)
{
    if (cmd->pipeline_cmd == NULL && cmd->args[0] == NULL)
    {
        return SUCCESS;
    }

    // If this is a raw pipeline string, delegate directly to sh -c
    if (cmd->pipeline_cmd)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child
            if (cmd->input_file)
            {
                int fd = open(cmd->input_file, O_RDONLY);
                if (fd == -1)
                {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmd->output_file)
            {
                int flags = O_WRONLY | O_CREAT | (cmd->output_append ? O_APPEND : O_TRUNC);
                int fd = open(cmd->output_file, flags, 0644);
                if (fd == -1)
                {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execlp("sh", "sh", "-c", cmd->pipeline_cmd, (char *)NULL);
            perror("exec sh");
            exit(EXIT_FAILURE);
        }
        else if (pid < 0)
        {
            perror("fork");
            return FAILURE;
        }
        else
        {
            if (!background)
            {
                int status;
                wait(&status);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    return SUCCESS;
                }
                else
                {
                    return FAILURE;
                }
            }
            return SUCCESS;
        }
    }

    if (cmd->args[0] == NULL)
    {
        return SUCCESS;
    }

    // Check built-in commands (don't support redirections for simplicity)
    if (strcmp(cmd->args[0], "cd") == 0)
    {
        return cd_cmd(cmd->args);
    }
    else if (strcmp(cmd->args[0], "pwd") == 0)
    {
        return pwd_cmd();
    }
    else if (strcmp(cmd->args[0], "echo") == 0)
    {
        return echo_cmd(cmd->args);
    }
    else if (strcmp(cmd->args[0], "exit") == 0)
    {
        return exit_cmd();
    }
    else
    {
        // External command
        pid_t pid = fork();
        if (pid == 0)
        {
            // Child
            if (cmd->input_file)
            {
                int fd = open(cmd->input_file, O_RDONLY);
                if (fd == -1)
                {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmd->output_file)
            {
                int flags = O_WRONLY | O_CREAT | (cmd->output_append ? O_APPEND : O_TRUNC);
                int fd = open(cmd->output_file, flags, 0644);
                if (fd == -1)
                {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if (execvp(cmd->args[0], cmd->args) == -1)
            {
                fprintf(stderr, "Command not found: %s\n", cmd->args[0]);
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
            if (!background)
            {
                int status;
                wait(&status);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    return SUCCESS;
                }
                else
                {
                    return FAILURE;
                }
            }
            return SUCCESS;
        }
    }
    return FAILURE; // To satisfy compiler
}
