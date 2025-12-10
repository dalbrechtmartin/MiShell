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
    ParsedCommand parsed_command;

    init_history_directory();

    // Batch mode: ./MiShell -c "command"
    if (argc >= 3 && strcmp(argv[1], "-c") == 0)
    {
        strncpy(line, argv[2], BUFFER_MAX_SIZE - 1);
        line[BUFFER_MAX_SIZE - 1] = '\0';

        parse_command(line, &parsed_command);
        execute_command(&parsed_command);
        return EXIT_SUCCESS;
    }

    // Normal interactive mode
    FILE *history_file = fopen_history_file("r");

    if (history_file)
    {
        load_history(history_file);
        fclose(history_file);
        // delete_history(); // Uncomment to clear history on each start
    }

    while (1)
    {
        printf("MiShell> $ ");
        if (fgets(line, BUFFER_MAX_SIZE, stdin) == NULL)
        {
            break;
        }
        else
        {
            save_in_history(line);
            parse_command(line, &parsed_command);
            execute_command(&parsed_command);
        }
    }
    exit_cmd();
}