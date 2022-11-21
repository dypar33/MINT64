#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"

#define PAGE_FLAGS_P 0x00000001     // Present
#define PAGE_FLAGS_RW 0x00000002    // R/W
#define PAGE_FLAGS_US 0x00000004    // User/Supervisor
#define PAGE_FLAGS_PWT 0x00000008   // Page Level Write-through
#define PAGE_FLAGS_PCD 0x00000010   // Page Level Cache Disable
#define PAGE_FLAGS_A 0x00000020     // Accessed
#define PAGE_FLAGS_D 0x00000040     // Dirty
#define PAGE_FLAGS_PS 0x00000080    // Page Size
#define PAGE_FLAGS_G 0x00000100     // Global
#define PAGE_FLAGS_PAT 0x00001000   // Page Attribute Table Index
#define PAGE_FLAGS_EXB 0x80000000   // Execute Disable 비트 
#define PAGE_FLAGS_DEFAULT ( PAGE_FLAGS_P | PAGE_FLAGS_RW )
#define PAGE_TABLESIZE 0x1000
#define PAGE_MAXENTRYCOUNT 512
#define PAGE_DEFAULTSIZE 0x200000

#pragma pack (push, 1)

typedef struct k_page_table_entry
{
    DWORD dw_attribute_and_lower_baseaddress;
    DWORD dw_upper_base_Address_and_exb;
} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#pragma pack (pop)

void k_initialize_page_tables(void);
void k_set_page_entry_data(PTENTRY* pst_entry, DWORD dw_upper_baseaddress,
                DWORD dw_lower_baseaddress, DWORD dw_lower_flags, DWORD dw_upper_flags);


#endif