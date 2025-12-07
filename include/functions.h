/** @file functions.h
 *  @brief Contains the declarations of the built-in shell functions.
 */

#include "libs.h"
#include "typedef.h"

/** @brief Changes the current directory.
 * @param args Array of arguments where args[1] is the target directory.
 * @return SUCCESS on success, FAILURE on failure.
 */
int cd_cmd(char **args);

/** @brief Prints the current working directory.
 * @return SUCCESS on success, FAILURE on failure.
 */
int pwd_cmd();

/** @brief Prints the given arguments to the standard output.
 * @param args Array of arguments to be printed.
 * @return SUCCESS on success, FAILURE on failure.
 */
int echo_cmd(char **args);

/** @brief Exits the shell.
 * @return EXIT_SUCCESS.
 */
int exit_cmd();

/** @brief Parses an input line into up to MAX_CMDS commands.
 * Splits only on the logical operator "&&"; leaves pipes intact to be handled
 * downstream. Background execution is detected via a trailing '&'.
 * @param line Raw input line (will not be modified).
 * @param result Destination structure for parsed commands and operators.
 */
void parse_command(char *line, ParsedCommand *result);

/** @brief Execute the ParsedCommand produced by parse_command.
 * Runs single commands directly or multiple commands sequentially.
 * @param command Parsed command bundle to execute.
 * @return SUCCESS on success, FAILURE if any command fails.
 */
int execute_command(ParsedCommand *command);

/** @brief Execute one command (builtin, pipeline, or external) with redirections.
 * Pipelines are delegated to `sh -c` when present in `pipeline_cmd`.
 * @param cmd Command to execute.
 * @param background 1 to avoid waiting, 0 to wait for completion.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_single_command(Command *cmd, int background);
