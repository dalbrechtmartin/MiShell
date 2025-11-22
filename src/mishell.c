/** @file mishell.c
 *  @brief Main function of the shell.
 */

#include "../include/functions.h"

/** @brief Main entry point for the MiShell program.
 *  @return EXIT_SUCCESS on normal termination.
 */
int main(int argc, char **argv)
{
    char line[BUFFER_MAX_SIZE];

    while (1)
    {
        printf("MiShell> $ ");
        if (fgets(line, BUFFER_MAX_SIZE, stdin) == NULL)
        {
            break;
        }
        else
        {
            execute_command(line);
        }
    }
    exit_cmd();
}