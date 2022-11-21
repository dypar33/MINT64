[BITS 64]

SECTION .text

extern Main

START:
    ; 세그먼트 세팅
    mov ax, 0x10
    mov ds, ax 
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 스택 세팅
    mov ss, ax
    mov rsp, 0x6FFFF8
    mov rbp, 0x6FFFF8

    call Main ; C 파일에 정의한 main 함수 호출

    jmp $