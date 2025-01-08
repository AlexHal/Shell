# Simple Unix Shell

A custom Unix shell implemented in C, showcasing core functionalities of a command-line interface and process management.

## Features

- **Command Execution**: Executes user commands using `fork()` and `execvp()` with support for foreground and background processes.
- **Built-In Commands**: Includes:
  - `cd`: Change directories.
  - `pwd`: Print current directory.
  - `echo`: Display messages.
  - `exit`: Terminate the shell.
  - `jobs`: List background processes.
  - `fg`: Bring background jobs to the foreground.
- **Redirection**: Redirects command output to files using `dup()`.
- **Piping**: Implements command chaining with pipes via `pipe()`.

## Technologies

- **Language**: C
- **Platform**: Linux/Unix
- **Key System Calls**: `fork()`, `execvp()`, `pipe()`, `dup()`, `waitpid()`

## How to Run

1. Clone this repository.
2. Compile the shell using `gcc`:
   ```bash
   gcc -o my_shell shell.c
