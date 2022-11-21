[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07c0:.MAIN

.PRINT_SCREEN:
    push bp
    mov bp, sp

    push es
    push ax
    push cx
    push bx
    push dx

    mov dx, word [bp + 0x4] ; 16bit는 주소가 2byte. X좌표
    mov bx, word [bp + 0x6] ; Y좌표
    mov cx, word [bp + 0x8] ; 주소

    mov ax, 0xB800
    mov es, ax

    mul bx, 160 ; 한 라인의 바이트 수 80*2
    mul dx, 2

    add ax, bx
    add ax,  dx

.LOOP:
    mov bl, byte [cx]
    test bl, bl
    je .LOOP_END

    mov [es:ax], bl
    add ax, 0x2

    inc cx

    jmp .LOOP



.LOOP_END:    
    pop dx
    pop bx
    pop cx
    pop ax
    pop es

    mov sp, bp
    pop ebp
    ret


.MAIN:
    