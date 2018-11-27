format ELF64 executable 3

include "struct.inc"
include "syscalls.inc"
include "utils.inc"

segment readable executable
entry start

start:
;-----------------------------------------------------------------------------
; parsing command line arguments
;-----------------------------------------------------------------------------
  pop   rcx                       ; arg count
  cmp   rcx, 3                    ; needs to be at least two for the self program arg0 and target arg1
  jne   usage                     ; exit 1 if not

  add   rsp, 8                    ; skips arg0
  pop   rsi                       ; gets arg1

  mov   rdi, sourcePath
  push  rsi                       ; save rsi
  push  rdi
  call  strToVar

  pop   rsi                       ; restore rsi
  pop   rdi
  mov   rdi, targetProcessName
  pop   rsi                       ; gets arg2
  push  rdi
  call  strToVar
;-----------------------------------------------------------------------------
; opening source file for reading
;-----------------------------------------------------------------------------
  mov   rdi, sourcePath           ; loads sourcePath to rdi
  xor   rsi, rsi                  ; cleans rsi so open syscall doesnt try to use it as argument
  mov   rdx, O_RDONLY             ; O_RDONLY
  mov   rax, SYS_OPEN             ; open
  syscall
  mov   rbx, rax                  ; load rax (target fd = 3) to rbx
;-----------------------------------------------------------------------------
; getting source file information to fstat struct
;-----------------------------------------------------------------------------
  mov   rdi, rbx                  ; load rbx (target fd = 3) to rdi
  lea   rsi, [fstat]              ; load fstat struct to rsi
  mov   rax, SYS_FSTAT            ; sys_fstat
  syscall
;-----------------------------------------------------------------------------
; reading source file to buffer
;-----------------------------------------------------------------------------
  lea   rsi, [buffer]
  mov   rdx, fstat.st_size        ; loading file size to rdx
  mov   rdi, rbx                  ; load rdi with fd from rbx (3)
  mov   rax, SYS_READ             ; read file to buffer
  syscall
;-----------------------------------------------------------------------------
; close source file
;-----------------------------------------------------------------------------
  mov   rax, SYS_CLOSE            ; close file
  syscall
;-----------------------------------------------------------------------------
; creating memory fd with empty name ("")
;-----------------------------------------------------------------------------
  lea   rdi, [bogusName]          ; empty string
  mov   rsi, MFD_CLOEXEC          ; memfd mode
  mov   rax, SYS_MEMFD_CREATE 
  syscall	                        ; memfd_create
  mov   rbx, rax                  ; memfd fd from rax to rbx
;-----------------------------------------------------------------------------
; writing buffer (source file) content to memory fd
;-----------------------------------------------------------------------------
  lea   rsi, [buffer]
  mov   rdi, rbx                  ; load memfd fd from rbx into rdi
  mov   rax, SYS_WRITE            ; write buf to memfd fd
  syscall
;-----------------------------------------------------------------------------
; executing memory fd with targetProcessName
;-----------------------------------------------------------------------------
  xor   rdx, rdx
  lea   rsi, [argv]
  lea   rdi, [fdPath]
  mov   rax, SYS_EXECVE           ; execve the memfd fd in memory
  syscall
;-----------------------------------------------------------------------------
; exit normally if everything works as expected
;-----------------------------------------------------------------------------
  jmp   normal_exit
;-----------------------------------------------------------------------------
; initialized data
;-----------------------------------------------------------------------------
segment readable writable
fstat             STAT
usageMsg          db "Usage: memrun <path_to_elf_file> <process_name>", 0xA, 0
sourcePath        db 256 dup 0
targetProcessName db 256 dup 0
bogusName         db "", 0
buffer            rb fstat.st_size
fdPath            db "/proc/self/fd/3", 0
argv              dd targetProcessName
