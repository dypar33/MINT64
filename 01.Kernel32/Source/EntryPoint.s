[ORG 0x00]
[BITS 16]

SECTION .text

START:
    ; 세그먼트 세팅
    mov ax, 0x1000
    
    mov ds, ax
    mov es, ax

    cli ; 인터럽트가 발생하지 않도록
    lgdt [GDTR] ; GDTR 테이블 로드

    ; 보호 모드로 전환하기 위한 cr0 레지스터 세팅
    mov eax, 0x4000003B  ; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
    mov cr0, eax 

    ; 보호 모드 코드의 offset을 구하고 base addr인 0x10000을 더함
    jmp dword 0x08: (PROTECTED_MODE - $$ + 0x10000)

[BITS 32]
PROTECTED_MODE:
    ; GDTR의 보호 모드 커널용 데이터 세그먼트 디스크립터 offset을 세그먼트 셀렉터들에 설정
    mov ax, 0x10 
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; stack 세그먼트 설정
    mov ss, ax
    mov esp, 0xFFFE
    mov ebp, 0xFFFE

    ; print
    push (SWICH_SUCCESS_M - $$ + 0x10000)
    push 2
    push 0
    call PRINT_M

    jmp $

PRINT_M:
    push ebp
    mov ebp, esp

    push esi
    push edi
    push eax
    push ecx
    push edx

    ; x
    mov esi, dword [ebp + 8]
    mov eax, 2
    mul esi
    mov edi, eax

    ; y
    mov esi, dword [ebp + 12]
    mov eax, 160
    mul esi

    add edi, eax

    ; string addr
    mov esi, dword [ebp + 16]

.PRINT_LOOP:
    mov cl, byte [esi]

    cmp cl, 0
    je .PRINT_LOOP_END

    mov byte [0xB8000 + edi], cl

    inc esi
    add edi, 2

    jmp .PRINT_LOOP

.PRINT_LOOP_END:
    pop edx
    pop ecx
    pop eax
    pop edi
    pop esi

    pop ebp
    ret 12

align 8, db 0

dw 0x0000 

GDTR:
    dw GDTEND - GDT -1
    dd (GDT - $$ + 0x10000)
    
GDT:
    NULLDESCRIPTOR:
        dw 0x0000
        dw 0x0000
        db 0x00 
        db 0x00
        db 0x00 
        db 0x00 

    CODEDESCRIPTOR:
        dw 0xFFFF ; Limit [15:0]
        dw 0x0000 ; Base [15:0]
        db 0x00  ; Base[23:16]
        db 0x9A  ; P = 1, DPL = 0, Code Segment R/X
        db 0xCF  ; G=1, D=1, L=0, Limit[19:16]
        db 0x00 ; base [31:24]

    DATADESCRIPTOR:
        dw 0xFFFF ; Limit [15:0]
        dw 0x0000 ; Base [15:0]
        db 0x00 ; Base [23:16]
        db 0x92 ; P=1, DPL=0, Data Segment R/W
        db 0xCF ; G=1, D=1, L=0, Limit[19:16]
        db 0x00 ; Base [31:24]
GDTEND:


SWICH_SUCCESS_M: db "Swich To Protected Mode Success!", 0

times 512 - ( $ - $$ ) db 0x00