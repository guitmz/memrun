format ELF64 executable 3

include "struct.inc"
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
  syscall                         ; rax contains source fd (3)
  push  rax                       ; saving rax with source fd
;-----------------------------------------------------------------------------
; getting source file information to fstat struct
;-----------------------------------------------------------------------------
  mov   rdi, rax                  ; load rax (source fd = 3) to rdi
  lea   rsi, [fstat]              ; load fstat struct to rsi
  mov   rax, SYS_FSTAT            ; sys_fstat
  syscall                         ; fstat struct conntains file information
  mov   r12, qword[rsi + 48]      ; r12 contains file size in bytes (fstat.st_size)
;-----------------------------------------------------------------------------
; creating memory map for source file
;-----------------------------------------------------------------------------
  pop   rax                        ; restore rax containing source fd
  mov   r8, rax                    ; load r8 with source fd from rax
  mov   rax, SYS_MMAP              ; mmap number
  mov   rdi, 0                     ; operating system will choose mapping destination
  mov   rsi, r12                   ; load rsi with page size from fstat.st_size in r12
  mov   rdx, 0x1                   ; new memory region will be marked read only
  mov   r10, 0x2                   ; pages will not be shared
  mov   r9, 0                      ; offset inside test.txt
  syscall                          ; now rax will point to mapped location
  push  rax                        ; saving rax with mmap address
;-----------------------------------------------------------------------------
; close source file
;-----------------------------------------------------------------------------
  mov   rdi, r8                   ; load rdi with source fd from r8
  mov   rax, SYS_CLOSE            ; close source fd
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
; writing memory map (source file) content to memory fd
;-----------------------------------------------------------------------------
  pop   rax                       ; restoring rax with mmap address
  mov   rdx, r12                  ; rdx contains fstat.st_size from r12
  mov   rsi, rax                  ; load rsi with mmap address
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
fdPath            db "/proc/self/fd/3", 0
argv              dd targetProcessName
