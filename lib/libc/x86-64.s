BITS 64
section .text

; x86-64 POSIX user-level calling convention:
;   Arguments in: RDI, RSI, RDX, RCX, R8, and R9
;   Return value in: RAX
; x86-64 POSIX kernel calling convention:
;   Syscall number in: RAX
;   Arguments in: RDI, RSI, RDX, R10, R8, and R9
;   Return value in: RAX
%macro def_long_syscall 2   ; works for any syscall
global %1
%1:
    mov r10, rcx  ; calling convention adapter
    mov rax, %2   ; set syscall number
    syscall
    ret
%endmacro

%macro def_syscall 2    ; only works for syscalls with <= 3 arguments
global %1
%1:
    mov rax, %2   ; set syscall number
    syscall
    ret
%endmacro

; SYS_* constants are inserted by the C preprocessor during build
def_syscall read, SYS_read
def_syscall write, SYS_write
def_syscall open, SYS_open
def_syscall close, SYS_close
def_syscall fsync, SYS_fsync
def_syscall munmap, SYS_munmap
def_syscall _exit, SYS_exit
def_syscall getpid, SYS_getpid
def_syscall kill, SYS_kill
def_long_syscall mmap, SYS_mmap

global memset
memset:                 ; rdi = dest, esi = c, rdx = len
    movzx rax, sil      ; set rax to the zero extended value of c cast to a byte
    mov rsi, 0x0101010101010101
    imul rax, rsi       ; rax = 8 consecutive bytes of value c
    mov rsi, rdi        ; save dest in rsi because rdi will be used
    mov rcx, rdx
    shr rcx, 3          ; set rcx to number of quadwords
    rep stosq           ; set rdi to rax, inc rdi, dec rcx, loop until rcx=0
    mov rcx, rdx
    and rcx, 7          ; set rcx to number of remaining bytes
    rep stosb           ; set remaining bytes to rax
    mov rax, rsi        ; set return value to dest
    ret

global memcpy
memcpy:                 ; rdi = dest, esi = src, rdx = len
    mov rax, rdi        ; set return value to dest
    mov rcx, rdx
    shr rcx, 3          ; set rcx to number of quadwords
    rep movsq           ; copy quadwords
    mov rcx, rdx
    and rcx, 7          ; set rcx to number of remaining bytes
    rep movsb           ; copy remaining bytes
    ret

; *_FILENO constants are inserted by the C preprocessor during build
close_fds:
    mov rdi, STDIN_FILENO
    call close
    mov rdi, STDOUT_FILENO
    call close
    mov rdi, STDERR_FILENO
    call close
    ret

extern main
global _start
_start:
    pop rdi         ; rdi = argc
    mov rsi, rsp    ; rsi = argv
    call main
    push rax        ; save return value of main
    call close_fds
    pop rdi         ; pass return value of main to _exit
    call _exit
    hlt
