# potato
Basic debugger using ptrace &amp; libdwarf

# Building
There are a few packages required to build the debugger: `make` and `libdwarf`.
On Debian you can install these by using the following commands:
```shell
> sudo apt install build-essential
> sudo apt install libdwarf-dev
```
Type `make all` in the project root directory.

# Features
- Software breakpoints on specific adresses (usage: `function_bp`)
- Software breakpoints on specific functions (usage: `address_bp`)
- Support for programs compiled as PIE (using the load address from `/proc/*processID*/maps`)
- Single step program execution (usage: `step`)
- Resume program execution (usage: `continue`)
- Register value lookup (usage: `reg *registerName*`)
