/** @file functions.c
 *  @brief Implementation of built-in shell and others functions.
 */

#include "../include/functions.h"
#include "functions.h"

/** @brief Global variable to store the initial working directory at shell startup. */
static char initial_cwd[BUFFER_MAX_SIZE] = {0};

/**
 * @brief Zero-initialize a Command structure.
 * @param cmd Command to reset.
 */
static void init_command(Command *cmd)
{
    memset(cmd, 0, sizeof(*cmd));
}

/**
 * @brief Free all allocated memory in a Command structure.
 * @param cmd Command to free.
 */
static void free_command(Command *cmd)
{
    if (!cmd)
        return;

    for (int i = 0; i < cmd->num_args && cmd->args[i]; i++)
    {
        free(cmd->args[i]);
        cmd->args[i] = NULL;
    }

    if (cmd->input_file)
    {
        free(cmd->input_file);
        cmd->input_file = NULL;
    }
    if (cmd->output_file)
    {
        free(cmd->output_file);
        cmd->output_file = NULL;
    }
    if (cmd->pipeline_cmd)
    {
        free(cmd->pipeline_cmd);
        cmd->pipeline_cmd = NULL;
    }
}

/**
 * @brief Apply input/output redirections for a command.
 * @param cmd Command holding redirection targets.
 * @return SUCCESS on success, FAILURE otherwise.
 */
static int apply_command_redirections(Command *cmd)
{
    if (cmd->input_file)
    {
        int input_fd = open(cmd->input_file, O_RDONLY);
        if (input_fd == -1)
        {
            perror("open input");
            return FAILURE;
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (cmd->output_file)
    {
        int flags = O_WRONLY | O_CREAT | (cmd->output_append ? O_APPEND : O_TRUNC);
        int output_fd = open(cmd->output_file, flags, 0644);
        if (output_fd == -1)
        {
            perror("open output");
            return FAILURE;
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
    return SUCCESS;
}

/**
 * @brief Wait for a child unless running in background.
 * @param child_pid PID returned by fork.
 * @param background 1 to skip waiting, 0 to wait.
 * @return SUCCESS if child exited with 0, otherwise FAILURE.
 */
static int run_child_and_wait(pid_t child_pid, int background)
{
    if (child_pid < 0)
    {
        perror("fork");
        return FAILURE;
    }
    if (background)
    {
        return SUCCESS;
    }
    int child_status;
    wait(&child_status);
    if (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0)
    {
        return SUCCESS;
    }
    return FAILURE;
}

/**
 * @brief Execute a pipeline (pipeline_cmd) via sh -c.
 * @param cmd Command containing pipeline_cmd and optional redirections.
 * @param background 1 to run without waiting, 0 to wait.
 * @return SUCCESS on success, FAILURE otherwise.
 */
static int run_pipeline_command(Command *cmd, int background)
{
    pid_t child_pid = fork();
    if (child_pid == 0)
    {
        if (apply_command_redirections(cmd) != SUCCESS)
        {
            exit(EXIT_FAILURE);
        }
        execlp("sh", "sh", "-c", cmd->pipeline_cmd, (char *)NULL);
        perror("exec sh");
        exit(EXIT_FAILURE);
    }
    return run_child_and_wait(child_pid, background);
}

/**
 * @brief Execute a non-builtin command with execvp.
 * @param cmd Command containing args and optional redirections.
 * @param background 1 to run without waiting, 0 to wait.
 * @return SUCCESS on success, FAILURE otherwise.
 */
static int run_external_command(Command *cmd, int background)
{
    pid_t child_pid = fork();
    if (child_pid == 0)
    {
        if (apply_command_redirections(cmd) != SUCCESS)
        {
            exit(EXIT_FAILURE);
        }
        execvp(cmd->args[0], cmd->args);
        fprintf(stderr, "Command not found: %s\n", cmd->args[0]);
        exit(EXIT_FAILURE);
    }
    return run_child_and_wait(child_pid, background);
}

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

/**
 * @brief Parse one command string into a Command structure.
 * Handles tokenization, input/output redirection detection.
 * @param cmd_str Command string to parse.
 * @param cmd Command structure to fill.
 */
static void parse_single_command(char *cmd_str, Command *cmd)
{
    init_command(cmd);

    // Whitespace
    while (*cmd_str && isspace((unsigned char)*cmd_str))
        cmd_str++;

    // Trailing whitespace
    char *end = cmd_str + strlen(cmd_str) - 1;
    while (end >= cmd_str && isspace((unsigned char)*end))
        *end-- = '\0';

    if (*cmd_str == '\0')
        return;

    // Check if command contains pipes
    if (strchr(cmd_str, '|'))
    {
        cmd->pipeline_cmd = strdup(cmd_str);
        return;
    }

    // Tokenize the command string
    char *copy = strdup(cmd_str);
    int arg_idx = 0;
    char *token = strtok(copy, " ");

    while (token && arg_idx < BUFFER_MAX_SIZE - 1)
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " ");
            if (token)
                cmd->input_file = strdup(token);
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(NULL, " ");
            if (token)
            {
                cmd->output_file = strdup(token);
                cmd->output_append = 0;
            }
        }
        else if (strcmp(token, ">>") == 0)
        {
            token = strtok(NULL, " ");
            if (token)
            {
                cmd->output_file = strdup(token);
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

    free(copy);
}

/** @brief Parses a command line into a ParsedCommand structure.
 * Splits only on "&&"; pipes are kept inside commands.
 * @param line The command line input.
 * @param result Pointer to a ParsedCommand structure to store the parsed result.
 */
void parse_command(char *line, ParsedCommand *result)
{
    memset(result, 0, sizeof(*result));
    for (int i = 0; i < MAX_CMDS - 1; i++)
        result->ops[i] = OP_NONE;

    char *line_copy = strdup(line);
    if (!line_copy)
    {
        perror("Memory error");
        exit(EXIT_FAILURE);
    }

    // Remove newline
    line_copy[strcspn(line_copy, "\n")] = '\0';

    // &
    char *bg_ptr = strrchr(line_copy, '&');
    if (bg_ptr && (bg_ptr == line_copy || *(bg_ptr - 1) != '&'))
    {
        *bg_ptr = '\0';
        result->is_background = 1;
    }

    // Split on &&
    char *cmd_str = line_copy;
    while (*cmd_str && result->num_cmds < MAX_CMDS)
    {
        char *and_pos = strstr(cmd_str, "&&");

        if (and_pos)
            *and_pos = '\0';

        parse_single_command(cmd_str, &result->cmds[result->num_cmds]);

        if (result->cmds[result->num_cmds].args[0] != NULL ||
            result->cmds[result->num_cmds].pipeline_cmd != NULL)
        {
            result->num_cmds++;
            if (and_pos && result->num_cmds < MAX_CMDS)
                result->ops[result->num_cmds - 1] = OP_AND;
        }

        if (!and_pos)
            break;
        cmd_str = and_pos + 2;
    }

    free(line_copy);
}

/** @brief Execute the parsed commands.
 * @param command Pointer to the parsed command structure.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_command(ParsedCommand *command)
{
    if (command->num_cmds == 0)
        return SUCCESS;

    int status = SUCCESS;
    for (int i = 0; i < command->num_cmds; i++)
    {
        int result = execute_single_command(&command->cmds[i], command->is_background);
        if (result != SUCCESS)
            status = FAILURE;
    }

    for (int i = 0; i < command->num_cmds; i++)
    {
        free_command(&command->cmds[i]);
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
        return SUCCESS;

    if (cmd->pipeline_cmd)
        return run_pipeline_command(cmd, background);

    // Only if no redirections
    if (cmd->input_file == NULL && cmd->output_file == NULL)
    {
        if (strcmp(cmd->args[0], "cd") == 0)
            return cd_cmd(cmd->args);
        if (strcmp(cmd->args[0], "pwd") == 0)
            return pwd_cmd();
        if (strcmp(cmd->args[0], "echo") == 0)
            return echo_cmd(cmd->args);
        if (strcmp(cmd->args[0], "exit") == 0)
            return exit_cmd();
    }

    return run_external_command(cmd, background);
}

/** @brief Gets the full path to the history file in the startup directory.
 * @param path Buffer where the path will be stored.
 * @return SUCCESS on success, FAILURE on failure.
 */
static int get_history_file_path(char *path)
{
    if (initial_cwd[0] == '\0')
    {
        snprintf(path, BUFFER_MAX_SIZE, "%s", HISTORY_FILE_NAME);
        return SUCCESS;
    }

    int written = snprintf(path, BUFFER_MAX_SIZE, "%s/%s", initial_cwd, HISTORY_FILE_NAME);
    if (written >= BUFFER_MAX_SIZE || written < 0)
    {
        perror("History file path too long");
        return FAILURE;
    }

    return SUCCESS;
}

/** @brief Opens the history file with the specified mode.
 * @param mode File open mode ("r", "a", etc.)
 * @return FILE pointer on success, NULL on failure.
 */
FILE *fopen_history_file(const char *mode)
{
    char history_path[BUFFER_MAX_SIZE];
    if (get_history_file_path(history_path) != SUCCESS)
        return NULL;

    return fopen(history_path, mode);
}

/** @brief Initialize the initial working directory (to be called at startup).
 * @return SUCCESS on success, FAILURE on failure.
 */
int init_history_directory()
{
    if (getcwd(initial_cwd, sizeof(initial_cwd)) == NULL)
    {
        perror("getcwd");
        return FAILURE;
    }
    return SUCCESS;
}

/** @brief Saves a command line to the history file.
 * @param command_line The command line to save.
 */
void save_in_history(const char *command_line)
{
    FILE *history_file = fopen_history_file("a");
    if (history_file)
    {
        fprintf(history_file, "%s", command_line);
        fclose(history_file);
    }
}

/** @brief Loads command history from the history file.
 * @param history_file File pointer to the opened history file.
 */
void load_history(FILE *history_file)
{
    if (history_file)
    {
        char line[BUFFER_MAX_SIZE];
        while (fgets(line, sizeof(line), history_file))
        {
            printf("%s", line);
        }
    }
}

/** @brief Deletes the history file.
 */
void delete_history()
{
    char history_path[BUFFER_MAX_SIZE];
    if (get_history_file_path(history_path) != SUCCESS)
        return;

    if (remove(history_path) != 0)
    {
        perror("Error deleting history file");
    }
}
