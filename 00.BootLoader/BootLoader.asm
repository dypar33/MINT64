[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START

TOTALSECTORCOUNT: dw 0x2
KERNEL32_SECTOR_COUNT: dw 0x2

START:
    ; 세그먼트 세팅
    mov ax, 0x07C0
    mov ds, ax
    mov ax, 0xB800
    mov es, ax

    ; 스택 설정
    mov ax, 0x0000
    mov ss, ax
    mov sp, 0xFFFE
    mov bp, 0xFFFE

    mov si, 0

; 화면 지우기
.SCREEN_CLS_LOOP:
    mov byte [es:si], 0
    mov byte [es:si+1], 0x0A
    add si, 2

    cmp si, 80 * 25 * 2
    jl .SCREEN_CLS_LOOP


    push M1
    push 0
    push 0
    call PRINT_M
    
    push IMAGE_LODING_M
    push 1
    push 0
    call PRINT_M

; disk 리셋
RESET_DISK:
    mov ax, 0 ; bios disk reset call nu,
    mov dl, 0 ; disk type
    int 0x13 ; interrupt

    jc ERROR_HANDLER ; if error occurred

    ; os를 복사할 0x1000 주소로 세그먼트 세팅
    mov si, 0x1000
    mov es, si
    mov bx, 0x0000

    mov di, word [TOTALSECTORCOUNT]

READ_DISK_DATA:
    ; di가 0이되면 다 읽은 것이다.
    cmp di, 0 
    je READ_DISK_DATA_END

    sub di, 0x1

    ; argu 세팅
    mov ah, 0x02 ; interrupt number (bios read disk)
    mov al, 0x1 ; sector count
    mov ch, byte [TRACK_NUMBER] ; track 
    mov cl, byte [SECTOR_NUMBER] ; sector
    mov dh, byte [HEAD_NUMBER] ; head
    mov dl, 0x00  ; drive number
    int 0x13 ; interrupt

    jc ERROR_HANDLER ; if error occurred

    ; 읽을만큼 세그먼트 레지스터에 +
    add si, 0x0020 
    mov es, si

    ; sector 값 += 1
    mov al, byte [SECTOR_NUMBER]
    add al , 1

    ; 19가 됐다면 head에 +1 & sector 1로 초기화
    mov byte [SECTOR_NUMBER], al
    cmp al, 37
    jl READ_DISK_DATA

    xor byte [HEAD_NUMBER], 0x01
    mov byte [SECTOR_NUMBER], 0x01

    ; xor 된 head가 0이라면 track += 1
    cmp byte [HEAD_NUMBER], 0x00
    jne READ_DISK_DATA

    add byte [TRACK_NUMBER], 0x01
    jmp READ_DISK_DATA




READ_DISK_DATA_END:
    push LOADING_COMPLETE_M
    push 1
    push 40
    call PRINT_M

    jmp 0x1000:0x0000 ; 로딩한 os 이미지 실행



; error 발생 시 오류 출력
ERROR_HANDLER:
    push DISK_ERR_M
    push 1
    push 20
    
    call PRINT_M
    add sp, 6

    jmp $
    

; 문자열 출력
PRINT_M:
    push bp
    mov bp, sp

    push es
    push si
    push di
    push ax
    push cx
    push dx

    mov ax, 0xB800 ; video memory
    mov es, ax ; segment setting

    ; x
    mov di, word [bp + 4] ; 2nd argu


    ; y
    mov si, word [bp + 6] ; 1st argu
    mov ax, 160           ; (80 * 2) = (라인 글자 수 * 2)
    mul si
    add di, ax

    mov si, word [bp + 8] ; 3rd argu

.PM_LOOP:
    mov cl, byte [si]

    cmp cl, 0
    je .PM_END

    mov byte [es:di], cl

    add di, 2
    add si, 1

    jmp .PM_LOOP


.PM_END:
    pop dx
    pop cx
    pop ax
    pop di
    pop si
    pop es

    pop bp
    ret 6



; data
M1: db 'MINT64 OS Boot Loader Start', 0 

DISK_ERR_M: db 'Disk Error..', 0
IMAGE_LODING_M: db 'OS Img Loading...', 0
LOADING_COMPLETE_M: db 'Done!', 0

SECTOR_NUMBER: db 0x02
HEAD_NUMBER: db 0x00
TRACK_NUMBER: db 0x00 

; filling 0x00
times 510 - ( $ - $$ ) db 0x00

; magic value
db 0x55, 0xAA