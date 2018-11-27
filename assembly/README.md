# memrun
Small tool written in Assembly (FASM) to run ELF (x86_64) binaries from memory with a given process name. Works on Linux where kernel version is >= 3.17 (relies on the `memfd_create` syscall).

# Usage

Build it with `$ fasm MEMRUN.ASM` and execute it. The first argument is the path for the ELF binary you want to run from memory and the second argument is the process name (string) you want to see in `ps auxww` output for example.

```
Usage: memrun <path_to_elf_file> <process_name>
```
