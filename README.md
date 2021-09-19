# fork mission statement

Implement memrun in C, mainly for allowing to run gcc compiled code without temporary filesystem files. Implementation should work for any ELF binaries, verified for x86_64 as well as armv7l. "[C script](C#c-script)" [run_from_memory_stdin.c](C/run_from_memory_stdin.c) makes use of [memrun.c](C/memrun.c), provides mixed bash and C code execution. Use of memrun is the only "run from memory" solution for C++ ("tcc -run" cannot help, because tcc is a C compiler only), see [C++ script](C#c-script-1):  

# memrun
Small tool written in Golang to run ELF (x86_64) binaries from memory with a given process name. Works on Linux where kernel version is >= 3.17 (relies on the `memfd_create` syscall).

# Usage

Build it with `$ go build memrun.go` and execute it. The first argument is the process name (string) you want to see in `ps auxww` output for example. Second argument is the path for the ELF binary you want to run from memory. 

```
Usage: memrun process_name elf_binary
```
