#ifndef __TYPES_H__
#define __TYPES_H__

#define BYTE unsigned char
#define WORD unsigned short
#define DWORD unsigned int
#define QWORD unsigned long
#define BOOL unsigned char

#define TRUE 1
#define FALSE 0
#define NULL 0

#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

#pragma pack(push, 1) // 1byte로 정렬 (추가 메모리 할당 방지)

typedef struct k_charctor_struct
{
    BYTE b_charactor;
    BYTE b_attribute;
} CHARACTER;

#pragma pack(pop)
#endif 