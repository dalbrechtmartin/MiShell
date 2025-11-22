/** @file typedef.h
 *  @brief Contains type definitions and constants used across the project.
 */

#define BUFFER_MAX_SIZE 1024

#define SUCCESS 0
#define FAILURE 1

#define NB_COMMANDS 4
#define NB_OPERATORS 8

typedef struct commands
{
    const char *names[NB_COMMANDS];
} commands;

static const commands valid_commands =
    {
        .names = {
            "cd",
            "echo",
            "pwd",
            "exit"}};

typedef struct operators
{
    const char *operators[NB_OPERATORS];
} operators;

static const operators valid_operators =
    {
        .operators = {
            "&&",
            "&",
            "||",
            "|",
            ">>",
            ">",
            "<",
            "<<"}};