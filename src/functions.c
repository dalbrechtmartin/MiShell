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
int cd_cmd(char **args)
{
    return SUCCESS;
}

/** @subsection PWD_COMMAND PWD Command
 *  Implementation of the pwd command.
 */
int pwd_cmd()
{
    return SUCCESS;
}

/** @subsection ECHO_COMMAND ECHO Command
 *  Implementation of the echo command.
 */
int echo_cmd(char **args)
{
    return SUCCESS;
}

/** @subsection EXIT_COMMAND EXIT Command
 *  Implementation of the exit command.
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

int execute_command(char *line)
{
    return SUCCESS;
}

/** @endsection */