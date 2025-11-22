/** @file mishell.c
 *  @brief Main function of the shell.
 */

#include "../include/functions.h"

int main(int argc, char **argv)
{
    int status = SUCCESS;
    char line[BUFFER_MAX_SIZE];

    while (status == SUCCESS)
    {
        printf("MiShell> $ ");
        if (fgets(line, BUFFER_MAX_SIZE, stdin) == NULL)
        {
            status = FAILURE;
            break;
        }
        else
        {
            line[strcspn(line, "\n")] = 0; // Delete newline character
        }
    }
    exit_cmd();
}