#include "Types.h"

void k_print_string(int ix, int iy, const char* p_string);

void Main(void)
{
    k_print_string(0, 3, "C Language Kernel Started!");

    while(1);
}

void k_print_string(int ix, int iy, const char* p_string)
{
    CHARACTER* pst_screen = (CHARACTER*) 0xB8000;

    pst_screen += (iy*80) + ix;

    for (int i = 0; p_string[i] != 0; i++)
    {
        pst_screen[i].b_charactor = p_string[i];
    }
    
}