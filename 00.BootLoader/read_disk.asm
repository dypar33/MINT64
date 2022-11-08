[ORG 0x00]
[BITS 16]

SECTION .text

.START:
    mov ah, 0x1000 ; 이미지를 복사할 주소 (0x10000)
    mov es, ah

.READ_DISK:
    mov bh, SECTOR_NUM ; sector
    dec bh
    mov ch, TRACK_NUM ; track
    mov dh, HEAD_NUM ; head
    mov dl, DISK_NUM ; disk num

    mov bx, 0x00 ; write addr
.LOOP
    mov ah, 2 ; read_disk

    inc bh
    mov al, bh

    int ah ; interrupt
    test ah, ah
    jne .READ_DISK_ERROR

    add bx, 0x200 ; 1 섹터는 0x200byte이므로 읽은만큼 주소 증가


    cmp al, 18 ; sector 18
    jne .LOOP
    
    cmp dh, 1
    mov dh, 1
    jne .LOOP

    cmp ch, 79
    mov al, 0
    mov dh, 0
    jne .LOOP

.END
    mov ch, 0

jmp $

.READ_DISK_ERROR:



SECTOR_NUM:
    db 0x02
HEAD_NUM:
    db 0x00
TRACK_NUM:
    db 0x00
DISK_NUM:
    db 0x00

