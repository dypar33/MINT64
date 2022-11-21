[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kLoadTR, kLoadIDTR, kLoadGDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS

kInPortByte:
    push rdx

    mov rdx, rdi
    mov rax, 0
    in al, dx

    pop rdx
    ret

kOutPortByte:
    push rdx
    push rax

    mov rdx, rdi
    mov rax, rsi
    out dx, al

    pop rax
    pop rdx
    ret

; GDTR 레지스터에 GDT 테이블 설정
; GDT 테이블의 정보를 저장한 자료구조의 주소가 인자로
kLoadGDTR:
    lgdt [rdi]
    ret

; TR 레지스터에 TSS 세그먼트 디스크립터 설정
; TSS 세그먼트 디스크립터 오프셋이 인자로
kLoadTR:
    ltr di
    ret

; IDTR 레지스터에 IDT 테이블 설정
; IDT 테이블의 정보를 저장하는 자료구조의 주소가 인자로
kLoadIDTR:
    lidt [rdi]
    ret

kEnableInterrupt:
    sti
    ret

kDisableInterrupt:
    cli
    ret 

kReadRFLAGS:
    pushfq
    pop rax

    ret