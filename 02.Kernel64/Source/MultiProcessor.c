#include "MultiProcessor.h"
#include "MPConfiguationTable.h"
#include "AssemblyUtility.h"
#include "LocalAPIC.h"
#include "PIT.h"

volatile int g_iWakeUpApplicationProcessorCount = 0;
volatile QWORD g_qwAPICIDAddress = 0;

BOOL kStartUpApplicationProcessor(void)
{
    if (kAnalysisMPConfigurationTable() == FALSE)
        return;

    kEnableGlobalLocalAPIC();
    kEnableSoftwareLocalAPIC();

    if (kWakeUpApplicationProcessor() == FALSE)
        return FALSE;

    return TRUE;
}

static BOOL kWakeUpApplicationProcessor(void)
{
    MPCONFIGURATIONMANAGER *pstMPManager;
    MPCONFIGURATIONTABLEHEADER *pstMPHeader;

    QWORD qwLocalAPIBaseAddress;
    BOOL bInterruptFlag;

    bInterruptFlag = kSetInterruptFlag(FALSE);

    pstMPManager = kGetMPConfigurationManager();
    pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
    qwLocalAPIBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

    g_qwAPICIDAddress = qwLocalAPIBaseAddress + APIC_REGISTER_APICID;

    *(DWORD *)(qwLocalAPIBaseAddress + APIC_REGISTER_ICR_LOWER) = APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE | APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;

    kWaitUsingDirectPIT(MSTOCOUNT(10));

    if (*(DWORD *)(qwLocalAPIBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING)
    {
        kInitializePIT(MSTOCOUNT(1), TRUE);

        kSetInterruptFlag(bInterruptFlag);

        return FALSE;
    }

    for (int i = 0; i < 2; i++)
    {
        *(DWORD *)(qwLocalAPIBaseAddress + APIC_REGISTER_ICR_LOWER) = APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE | APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_STARTUP | 0x10;

        kWaitUsingDirectPIT(USTOCOUNT(200));

        if (*(DWORD *)(qwLocalAPIBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING)
        {
            kInitializePIT(MSTOCOUNT(1), TRUE);

            kSetInterruptFlag(bInterruptFlag);

            return FALSE;
        }
    }

    kInitializePIT(MSTOCOUNT(1), TRUE);

    kSetInterruptFlag(bInterruptFlag);

    while(g_iWakeUpApplicationProcessorCount < (pstMPManager->iProcessorCount - 1))
        kSleep(50);
    
    return TRUE;
}

BYTE kGetAPICID(void)
{
    MPCONFIGURATIONTABLEHEADER* pstMPHeader;
    QWORD qwLocalAPICBaseAddress;

    if(g_qwAPICIDAddress == 0)
    {
        pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
        if(pstMPHeader == NULL)
            return 0;

        qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
        g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
    }

    return *((DWORD*) g_qwAPICIDAddress) >> 24;
}

// 1/10까지 써야하는 베스킨라빈스 쿠폰있는데, 혹시 쓰실분 있습니까?
// 아이스크림을 안좋아해서 ㅎㅎ;;