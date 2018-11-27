O_RDONLY    = 0
MFD_CLOEXEC = 1

struct STAT
  st_dev     dw  ?     ; ID of device containing file
  pad1       dw  ?
  st_ino     dd  ?     ; inode number
  st_mode    dw  ?     ; protection
  st_nlink   dw  ?     ; number of hard links
  st_uid     dw  ?     ; user ID of owner
  st_gid     dw  ?     ; group ID of owner
  st_rdev    dw  ?     ; device ID (if special file)
  pad2       dw  ?
  st_size    dd  ?     ; total size, in bytes
  st_blksize dd  ?     ; block size
  st_blocks  dd  ?

  st_atime   dd  ?     ; time of last access
  unused1    dd  ?

  st_mtime   dd  ?     ; time of last modification
  unused2    dd  ?

  st_ctime   dd  ?     ; time of last status change
  unused3    dd  ?
  unused4    dd  ?
  unused5    dd  ?
ends


normal_exit:
    xor     rdi, rdi		; exit code 0
	mov     rax, SYS_EXIT   ; sys_exit 
	syscall

bad_exit:
    mov     rdi, 1
    mov     rax, SYS_EXIT
    syscall

usage:
    lea     rdi, [usageMsg]
    call    print
    jmp     bad_exit


strlen:						; rdi is the default search for scasb
    push rdi
    push rcx
    mov rcx, -1
    xor eax, eax
    repne scasb 
    not rcx
    mov rax, rcx 
    pop rcx
    pop rdi
    ret


strlen_old:
	push    rdi				; push to stack
	push    rcx				; push to stack
	xor	    rcx, rcx		; set rcx to 0
	mov 	rcx, -1  		; move -1 to rcx
	xor  	al, al 			; set al to 0
	cld						; clear the direction flags
	repne 	scasb 			; repeat if not equal to al
	neg 	rcx				; negate rcx
	sub 	rcx, 1			; subtract 1 from rcx
	mov 	rax, rcx		; move value from rcx to rax
	pop		rcx				; restore original rcx value
	pop		rdi				; restore original rdi value
	ret

print:
	push rsi
	push rdx
	push rdi
    call      strlen
    mov       rdx, rax
    mov       rsi, rdi
    mov       rdi, 1
    mov       rax, SYS_WRITE
    syscall
	pop rsi
	pop rdx
	pop rdi
    ret

strToVar:
    mov  al, [rsi]          
    mov  [rdi], al           
    inc  rsi              
    inc  rdi
    cmp byte [rsi], 0   ; Check for null terminator
    jne strToVar                ; loop if not null
    ret
