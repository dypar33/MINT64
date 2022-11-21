#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void k_print_string(int ix, int iy, const char* p_string);
BOOL k_initializing_k64_area(void);
BOOL k_has_enough_memory(void);
void k_copy_kernel64_image_to_2mbyte();

void Main(void)
{
    DWORD dw_eax, dw_ebx, dw_ecx, dw_edx;
    char vc_vendor_str[13] = {0,};

    k_print_string(0, 3, "C Language Kernel Started!");

    k_print_string(0, 4, "Checking Memory Size........................[    ]");
    if(k_has_enough_memory() == FALSE)
    {
        k_print_string(45, 4, "Fail");
        k_print_string(0, 5, "Not Enough Memory..");
        while(1);
    }

    k_print_string(45, 4, "Pass");

    k_print_string(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
    

    // initializing kernel64 area in memory
    if(k_initializing_k64_area() == FALSE)
    {
        k_print_string(45, 5, "fail");
        k_print_string(0, 6, "IA-32e Kernel Area Initialization Fail..");
    }
    k_print_string(45, 5, "Pass");

    k_print_string(0, 6, "IA-32e Page Tables Initialize...............[    ]");
    k_initialize_page_tables();
    k_print_string(45,6,"Pass");

    kReadCPUID(0x00, &dw_eax, &dw_ebx, &dw_ecx, &dw_edx);
    *(DWORD*) vc_vendor_str = dw_ebx;
    *((DWORD*) vc_vendor_str+1) = dw_edx;
    *((DWORD*) vc_vendor_str+2) = dw_ecx;
    k_print_string(0, 7, "Processor Vendor String.....................[            ]");
    k_print_string(45, 7, vc_vendor_str);

    kReadCPUID(0x80000001, &dw_eax, &dw_ebx, &dw_ecx, &dw_edx);
    k_print_string( 0, 8, "64bit Mode Support Check....................[    ]" );

    if(dw_edx & (1 << 29))
        k_print_string(45 ,8, "Pass");
    else
    {
        k_print_string(45, 8, "Fail");
        k_print_string(0, 9, "This Processor does not support 64bit mode");
        while(1);
    }

    k_print_string(0, 9, "Copy IA-32e Kernel To 2M Address............[    ]");
    k_copy_kernel64_image_to_2mbyte();
    k_print_string(45, 9, "Pass");


    k_print_string(0, 10, "Switch to IA-32E Mode");

    kSwitchAndExecute64bitKernel();
    
    while(1);
}

void k_print_string(int ix, int iy, const char* p_string)
{
    CHARACTER* pst_screen = (CHARACTER*) 0xB8000;

    pst_screen += (iy*80) + ix;

    for (int i = 0; p_string[i] != 0; i++)
    {
        pst_screen[i].bCharactor = p_string[i];
    }
    
}

BOOL k_initializing_k64_area(void)
{
    DWORD* start_addr = (DWORD*)0x100000;
    
    while((DWORD) start_addr < 0x600000)
    {
        *start_addr = 0x00;

        if(*start_addr != 0)
            return FALSE;

        start_addr += 1;
    }

    return TRUE;
}

BOOL k_has_enough_memory(void)
{
    DWORD* current_address;

    current_address = (DWORD*) 0x100000;

    while ((DWORD) current_address < 0x4000000)
    {
        *current_address = 0x12345678;

        if(*current_address != 0x12345678)
            return FALSE;

        current_address += (0x100000/4);
    }
    return TRUE;
}

void k_copy_kernel64_image_to_2mbyte(void)
{
    WORD w_kernel32_sector_count, w_total_kernel_sector_count;
    DWORD* pdw_source_address, *pdw_destination_address;

    w_total_kernel_sector_count = *((WORD*)0x7C05);
    w_kernel32_sector_count = *((WORD*) 0x7C07);

    pdw_source_address = (DWORD*)(0x10000 + (w_kernel32_sector_count * 512));
    pdw_destination_address = (DWORD*) 0x200000;

    for (int i = 0; i < 512 * (w_total_kernel_sector_count - w_kernel32_sector_count) / 4; i++)
    {
        *pdw_destination_address = *pdw_source_address;
        pdw_destination_address++;
        pdw_source_address++;
    }
}