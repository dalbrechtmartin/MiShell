/** @file libs.h
 *  @brief Includes libraries needed across the project.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
