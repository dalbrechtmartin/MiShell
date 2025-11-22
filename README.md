# MiShell

A mini-shell for a CNAM Student Project.

## Overview

MiShell is a simple command-line shell implemented in C. It supports basic built-in commands and can execute external programs.

## Features

- Built-in commands: `cd`, `pwd`, `echo`, `exit`
- Execution of external commands
- Error handling for invalid commands

## Building

Ensure you have `gcc` and `make` installed. On Windows, use MinGW or switch to WSL/Cygwin for full POSIX support.

```sh
make all
```

This compiles the source files and links them into `./bin/MiShell`.

## Usage

Run the shell:

```sh
./bin/MiShell
```

Enter commands at the prompt (e.g., `pwd`, `ls`, `cd /path`, `exit`).

## Project Structure

- `src/`: Source code files
- `include/`: Header files
- `bin/`: Compiled executable
- `doc/`: Generated documentation (run `make doc` to update)

## Dependencies

- Standard C libraries (stdio, stdlib, etc.)
- POSIX functions (fork, wait) – requires Unix-like environment

## License

See [LICENSE](LICENSE).
