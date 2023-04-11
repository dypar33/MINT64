#include "Page.h"

void k_initialize_page_tables(void){
    PML4TENTRY* pstPML4TEntry;
    PDPTENTRY* pstPDPTEntry;
    PDENTRY* pstPDEntry;
    DWORD dwMappingAddress;
    int i;

    pstPML4TEntry = (PML4TENTRY*) 0x100000;
    k_set_page_entry_data(&(pstPML4TEntry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0x0);
    for(i = 1; i < PAGE_MAXENTRYCOUNT ; i++){
        k_set_page_entry_data(&(pstPML4TEntry[i]), 0x0, 0x0, 0x0, 0x0);
    }

    pstPDPTEntry = (PDPTENTRY*) 0x101000;
    for(i = 0; i < 64; i++){
        k_set_page_entry_data(&(pstPDPTEntry[i]), 0, 0x102000 + (i * PAGE_TABLESIZE),
            PAGE_FLAGS_DEFAULT, 0x0);
    }
    for(i = 64; i < PAGE_MAXENTRYCOUNT; i++){
        k_set_page_entry_data(&(pstPDPTEntry[i]), 0x0, 0x0, 0x0, 0x0);
    }

    pstPDEntry = (PDENTRY*) 0x102000;
    dwMappingAddress = 0x0;
    for(i = 0; i < PAGE_MAXENTRYCOUNT * 64 ; i++){
        k_set_page_entry_data(&(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12,
            dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0x0);
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

void k_set_page_entry_data(PTENTRY* pstEntry, DWORD dwUpperBaseAddress,
        DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags){
    pstEntry->dw_attribute_and_lower_baseaddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry->dw_upper_base_Address_and_exb = (dwUpperBaseAddress & 0xff) | dwUpperFlags;
}