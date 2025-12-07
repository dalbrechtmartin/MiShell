/** @file typedef.h
 *  @brief Contains type definitions and constants used across the project.
 */

/** @brief Maximum size for input buffer. */
#define BUFFER_MAX_SIZE 1024

/** @brief General success code. */
#define SUCCESS 0
/** @brief General failure code. */
#define FAILURE 1

/** @brief Maximum number of commands in a parsed command line. */
#define MAX_CMDS 3

/** @brief Enum for operators between commands. */
typedef enum
{
    OP_NONE,
    OP_PIPE,
    OP_AND,
    OP_OR
} Operator;

/** @brief Structure to hold a single command and its arguments. */
typedef struct
{
    char *args[BUFFER_MAX_SIZE];
    int num_args;
    char *pipeline_cmd; // Full command string executed via shell when it contains pipes
    char *input_file;   // for < redirection
    char *output_file;  // for > or >> redirection
    int output_append;  // 1 if >>, 0 if >
} Command;

/** @brief Structure to hold parsed commands and operators. */
typedef struct
{
    Command cmds[MAX_CMDS];
    Operator ops[MAX_CMDS - 1];
    int num_cmds;
    int is_background; // 0 = false, 1 = true
} ParsedCommand;
