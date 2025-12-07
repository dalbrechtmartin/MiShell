/** @file typedef.h
 *  @brief Contains type definitions and constants used across the project.
 */

#define BUFFER_MAX_SIZE 1024

#define SUCCESS 0
#define FAILURE 1

#define MAX_CMDS 3

/** @brief Structure to hold a single command and its arguments. */
typedef struct
{
    char *args[BUFFER_MAX_SIZE];
    int num_args;
} Command;

/** @brief Structure to hold parsed commands and operators. */
typedef struct
{
    Command cmds[MAX_CMDS];
    char *ops[2];
    int num_cmds;
    int is_background; // 0 = false | 1 = true
} ParsedCommand;
