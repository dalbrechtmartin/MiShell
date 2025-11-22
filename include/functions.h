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

/** @brief Execute the valids commands.
 * @return SUCCESS on success, FAILURE on failure.
 */
int execute_command(char *line);
