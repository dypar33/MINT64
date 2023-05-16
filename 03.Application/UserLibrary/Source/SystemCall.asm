[BITS 64]

SECTION .text

global ExecuteSystemCall

extern Main, exit

_START:
    call Main

    mov rdi, rax

    call exit

    jmp $
    ret

ExecuteSystemCall:
    push rcx
    push r11

    syscall

    pop r11
    pop rcx
    ret