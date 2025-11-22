#include <stdio.h>
#include <stdlib.h>

/**
 * Displays a message including the first argument passed.
 */
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("Message: %s\n", argv[1]);
    return EXIT_SUCCESS;
}