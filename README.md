# xv6-shell
Custom Unix-style shell implementation in xv6 supporting command execution, pipes, and process management using C

This project implements a custom Unix-style shell in the xv6 operating system as part of an Operating Systems coursework project.

The shell parses and executes user commands, supporting features such as command execution, pipelines, and process management. It interacts directly with the xv6 kernel using system calls like `fork()`, `exec()`, and `wait()` to create and manage processes.

## Features

- Command parsing and execution
- Support for command pipelines (`|`)
- Process creation using `fork()`
- Program execution using `exec()`
- Parent-child process synchronization using `wait()`

## Technologies

- C
- xv6 Operating System
- Unix system calls
- Linux development environment

## Example Usage

$ ls | grep test
test1.txt
test2.txt

## Learning Outcomes

This project demonstrates practical understanding of:

- Unix process management
- Inter-process communication using pipes
- Shell command parsing
- Low-level systems programming in C

## Note

You will need a RISC-V "newlib" tool chain from
https://github.com/riscv/riscv-gnu-toolchain, and qemu compiled for
riscv64-softmmu.  Once they are installed, and in your shell
search path, you can run "make qemu".
