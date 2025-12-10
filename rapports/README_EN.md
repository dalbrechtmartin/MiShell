# MiShell – Development Report

## Table of Contents

- [Overview](#overview)
- [Important: Feature Clarification](#important-feature-clarification)
- [Development Approach](#development-approach)
  - [Phase 1: Simple Commands Without Arguments](#phase-1-simple-commands-without-arguments)
  - [Phase 2: Execution of External Commands](#phase-2-execution-of-external-commands)
  - [Phase 3: Arguments and Redirections](#phase-3-arguments-and-redirections)
  - [Phase 4: Operators](#phase-4-operators--my-biggest-challenge)
  - [Phase 5: Redirections with Builtins](#phase-5-redirections-with-builtins)
  - [Phase 6: Command History](#phase-6-command-history--well-thought-out-persistence)
- [Final Architecture](#final-architecture)
- [Challenges Encountered and Solutions](#challenges-encountered-and-solutions)
- [Supported Commands](#supported-commands)
- [Usage Examples](#usage-examples)
- [Build](#build)
- [Dependencies](#dependencies)
- [License](#license)

---

## Overview

MiShell is a mini-shell implemented in C, developed as part of a school project. This project allowed me to explore the basic mechanisms of a command interpreter: parsing, execution, child process management, and logical operators.

## Important: Feature Clarification

Redirections (`<`, `>`, `>>`), logical operators (`&&`, `||`), and pipes (`|`) **were mandatory** according to the project specification (FM02). However, from my understanding, the specification did **not** explicitly require implementing them as **built-ins** (features coded directly into the shell).

This meant I had a choice:

1. **Implement them myself** (redirections via `dup2()`, parsing operators, etc.) – which I did initially but abandoned because it was too complex
2. **Delegate them to the system shell** via `sh -c` – which is what I did subsequently for pipes and `||`

The only commands that **had to be built-in** were: `cd`, `pwd`, `echo`, `exit` (FM03).

[⬆ Back to top](#table-of-contents)

## Development Approach

### Phase 1: Simple Commands Without Arguments

At first, I started with the essentials: **executing simple commands**. The goal was to be able to enter a command (e.g., `pwd`, `ls`) and display it correctly.

- Creation of built-in commands: `cd`, `pwd`, `echo`, `exit`
- Basic structure without functions initially
- A simple loop that reads input and executes

### Phase 2: Execution of External Commands

Next, to execute **any command**, not just my built-ins, I added the ability to launch external commands via `execvp()` by creating a child process (`fork()`).

**Key decision**: If a command was not a built-in, instead of raising an error, I **delegate its execution to the child process** which will search for it in the system `PATH`. This proved **very useful later** because this approach allowed me to easily handle complex operators.

### Phase 3: Arguments and Redirections

I then added **argument handling** (`ls -al`) and **redirections** (`>`, `>>`, `<`) – two mandatory features of the specification. Parsing became more complex:

- Tokenize the command line
- Detect redirections
- Apply redirections in the child process with `dup2()`

[⬆ Back to top](#table-of-contents)

### Phase 4: Operators – My Biggest Challenge

The specification required supporting the `&&` (logical AND), `||` (logical OR) operators and **pipes** (`|`). This is where I **really struggled** because initially I mistakenly thought I had to manage them entirely as "built-ins".

**My initial attempts** were too complex:

- Attempt 1: Parse all operators at the same time → very nested and hard to follow code
- Attempt 2: Store commands in complex structures → too many cases to handle

**The final solution**: I opted for a **minimalist approach**:

1. **Split only on `&&`** → this gives me at most 3 commands (in accordance with `MAX_CMDS`)
2. **Keep pipes AND `||` inside commands** → rather than parsing them myself, I pass them to the system shell
3. **Create a `parse_single_command()` function** that handles **one command** at a time

Thus:

- `ps && ls` → two commands executed sequentially (handled by MiShell)
- `ls | grep ".txt"` → **a single command** passed to `sh -c` which handles the pipe
- `false || echo "ok"` → **a single command** passed to `sh -c` which handles the `||`
- `ps && ls | grep ".txt"` → two commands: `ps` on one side, `ls | grep ".txt"` on the other

**Advantage**: The code becomes **very readable**. Pipes and `||` are not my concern – I let the shell handle them via `sh -c`. I just need to manage the logical split on `&&`.

[⬆ Back to top](#table-of-contents)

### Phase 5: Redirections with Builtins

One final challenge: builtins (`echo`, `pwd`) did not support redirections. I added a check: if a builtin has a redirection, it is executed via the child process (like an external command).

[⬆ Back to top](#table-of-contents)

### Phase 6: Command History – Well-Thought-Out Persistence

At first, I had a simple idea: save each command in a `mishell_history.txt` file. But quickly, I ran into a common problem with relative paths.

**The problem**:

When I used a relative path (`mishell_history.txt`), the file was created in the current directory at shell startup. But as soon as a user did `cd`, the current directory changed! The next saved command created a **new file** in the new directory instead of continuing to write in the original one.

**My attempts**:

1. **First idea**: Use the `$HOME` variable to store history in the user's home directory. But on Windows/WSL, this variable is not always defined, and it wasn't really what I wanted.

2. **Second idea**: Keep a relative path but add a global `history_path` variable and pass it everywhere. It worked, but it was redundant and inelegant.

**The final solution**:

I implemented a **static global variable `initial_cwd`** that saves the current directory at shell startup via `getcwd()`. Then, no matter where we navigate with `cd`, the history is always written to the same place: `<startup_directory>/mishell_history.txt`.

```c
// At startup
init_history_directory();  // Captures cwd in initial_cwd

// For each command
save_in_history(line);     // Always uses initial_cwd
```

[⬆ Back to top](#table-of-contents)

## Final Architecture

```
parse_command() [MiShell]
  └─ Split on && only
  └─ parse_single_command() (for each segment) [MiShell]
     └─ Detects pipes or || → stores everything in pipeline_cmd [Delegation]
     └─ Otherwise, tokenizes and detects redirections [MiShell]

execute_command() [MiShell]
  └─ execute_single_command() (for each command) [MiShell]
     └─ If pipeline_cmd → run_pipeline_command() [sh -c]
     └─ If builtin without redirection → executes directly [MiShell]
     └─ Otherwise → run_external_command() [fork + execvp]
```

### Note on Batch Mode (FM05)

**Batch mode** allows executing a command directly without going through interactive mode.

**Usage**:

```sh
$ ./MiShell -c "ls -al"
$ ./MiShell -c "pwd && echo done"
$ ./MiShell -c "export MY_VAR=test && echo $MY_VAR"
```

**Implementation**:

In `main()`, we first check if the `-c` argument is present. If the argument is not provided, we switch to normal interactive mode. I simply reused the existing parsing and execution functions.

### Note on Environment Variables (FM06)

Environment variables are managed via the built-in `export` command. This command uses `putenv()` to modify the environment of the current process and all child processes created afterwards.

**Usage**:

```c
export VAR=value
```

**Example**:

```sh
MiShell> $ export MY_VAR=hello
MiShell> $ echo $MY_VAR
hello

MiShell> $ export PATH=/custom/path:$PATH
MiShell> $ which my_command
/custom/path/my_command
```

**Implementation**:

The `export_cmd()` function:

1. Verifies that the argument contains an `=`
2. Duplicates the string (because `putenv()` does not copy it)
3. Calls `putenv()` to modify the environment
4. Variables remain available to all child processes launched afterwards

### Note on Aliases (FM07)

For **system shell aliases** (like `ll` for `ls -al`), no special implementation is needed in MiShell. Since an alias is not a recognized built-in command, it is automatically treated as an **external command** and executed via `fork() + execvp()`. The system shell handles resolving the alias. This works naturally without additional code.

### Responsibility Distribution

| Feature                         | Managed by MiShell   | Delegated to sh -c |
| ------------------------------- | -------------------- | ------------------ |
| Parsing &&                      | ✅                   | ❌                 |
| Parsing \|\|                    | ❌                   | ✅                 |
| Parsing pipes                   | ❌                   | ✅                 |
| Sequential execution with &&    | ✅                   | ❌                 |
| Conditional execution with \|\| | ❌                   | ✅                 |
| Pipe handling                   | ❌                   | ✅                 |
| Redirection <, >, >>            | ✅                   | ❌                 |
| Built-in commands               | ✅                   | ❌                 |
| External commands               | ✅ (via fork/execvp) | ❌                 |
| Background & detection          | ✅                   | ❌                 |
| Persistent history              | ✅                   | ❌                 |
| System shell aliases            | ❌                   | ✅                 |
| Environment variables           | ✅ (via export)      | ❌                 |

[⬆ Back to top](#table-of-contents)

## Challenges Encountered and Solutions

| Challenge                      | Cause                                           | Solution                                                      |
| ------------------------------ | ----------------------------------------------- | ------------------------------------------------------------- |
| Complex parsing                | Attempting to handle all operators at once      | Split only on `&&`, delegate pipes to the shell               |
| Non-functional pipes           | Attempt to create a complex structure for pipes | Store in `pipeline_cmd` and pass to `sh -c`                   |
| Redirections on builtins       | No fork handling for builtins                   | Add fork if redirection detected                              |
| Invalid memory                 | `strtok()` modifies the string, then we free it | Add `strdup()` on filenames                                   |
| Hard to maintain code          | Everything in one function, no structure        | Complete refactoring: function extraction, explicit names     |
| History created in wrong place | Relative path + `cd` = duplicated files         | Capture cwd at startup in a global variable                   |
| Double free in memory          | Freeing `ParsedCommand` in two places           | Create `free_command()` and call it only once                 |
| snprintf truncation warning    | `snprintf()` could truncate the history path    | Check return value of `snprintf()` instead of pre-calculating |

[⬆ Back to top](#table-of-contents)

## Supported Commands

- **Execution modes**:
  - Interactive mode (prompt `MiShell> $`)
  - Batch mode (`-c "command"`)
- **Builtins**: `cd`, `pwd`, `echo`, `export`, `exit`
- **Environment variables**: Created with `export VAR=value`
- **External**: Any command available in the `PATH`
- **Aliases**: System shell aliases are supported (e.g., `ll` if it exists)
- **Operators**:
  - `&&` (logical AND) – handled directly by MiShell
  - `||` (logical OR) – delegated to the system shell via `sh -c`
  - `&` (background)
- **Pipes**: `|` (delegated to the system shell via `sh -c`)
- **Redirections**: `<` (input), `>` (output), `>>` (append)
- **History**: All commands are automatically logged in `mishell_history.txt`

[⬆ Back to top](#table-of-contents)

## Usage Examples

```sh
# Redirection + AND (handled by MiShell)
MiShell> $ echo Hello > test.txt && cat test.txt
Hello

# Pipe (delegated to sh -c)
MiShell> $ ls -al | grep ".c"
-rw-r--r-- ... functions.c
-rw-r--r-- ... mishell.c

# OR logic (delegated to sh -c)
MiShell> $ false || echo "Fallback executed"
Fallback executed

# AND handled by MiShell
MiShell> $ cd /tmp && pwd
/tmp

# Combination: AND (MiShell) + Pipe (sh -c)
MiShell> $ echo "test" && ls | head -n 2
test
file1
file2

MiShell> $ exit
Exiting MiShell...
```

[⬆ Back to top](#table-of-contents)

## Build

```sh
make all       # Compile everything
make clean     # Clean object files
make doc       # Generate Doxygen documentation
```

[⬆ Back to top](#table-of-contents)

## Dependencies

- C compiler (gcc)
- Standard C libraries (stdio, stdlib, etc.)
- POSIX functions (fork, wait, execvp) – **requires Unix/Linux/WSL**

[⬆ Back to top](#table-of-contents)

## License

See [LICENSE](LICENSE).
