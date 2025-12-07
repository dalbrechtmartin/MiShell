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

/** @brief Parses a command line into a ParsedCommand structure.
 * @param line The command line input.
 * @param result Pointer to a ParsedCommand structure to store the parsed result.
 */
void parse_command(char *line, ParsedCommand *result);

/** @brief Execute the parsed commands.
 * @param command Pointer to the parsed command structure.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_command(ParsedCommand *command);

/** @brief Execute a single command with redirections.
 * @param cmd Pointer to the command to execute.
 * @param background 1 if background, 0 otherwise.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_single_command(Command *cmd, int background);
