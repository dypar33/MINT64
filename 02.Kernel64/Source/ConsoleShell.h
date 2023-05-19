#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"

#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT 300
#define CONSOLESHELL_PROMPTMESSAGE "MINT64>"
#define PACKAGESIGNATURE "MINT64OSPACKAGE "
#define MAXFILENAMELENGTH 24

typedef void (*CommandFunction)(const char *pcParameter);

#pragma pack(push, 1)

typedef struct kShellCommandEntryStruct
{
    // command
    char *pcCommand;

    // help message
    char *pcHelp;

    // f pointer
    CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

typedef struct kParameterListStruct
{
    const char *pcBuffer;
    int iLength;
    int iCurrentPosition;
} PARAMETERLIST;

typedef struct PackageItemStruct
{
    char vcFileName[MAXFILENAMELENGTH];

    DWORD dwFileLength;
} PACKAGEITEM;

typedef struct PackageHeaderStruct
{
    char vcSignature[16];

    DWORD dwHeaderSize;

    PACKAGEITEM vstItem[0];
} PACKAGEHEADER;

#pragma pack(pop)

void kStartConsoleShell(void);
void kExecuteCommand(const char *pcCommandBuffer);
void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter);
int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter);

// command handler
static void kHelp(const char *pcParameterBuffer);
static void kCls(const char *pcParameterBuffer);
static void kShowTotalRAMSize(const char *pcParameterBuffer);
static void kStringToDecimalHexTest(const char *pcParameterBuffer);
static void kShutdown(const char *pcParameterBuffer);
static void kSetTimer(const char *pcParameterBuffer);
static void kSleepUsingPIT(const char *pcParameterBuffer);
static void kReadTimeStampCounter(const char *pcParameterBuffer);
static void kMeasureCPUSpeed(const char *pcParameterBuffer);
static void kShowDateAndTime(const char *pcParameterBuffer);
static void kCreateTestTask(const char *pcParameterBuffer);
static void kChangeTaskPriority(const char *pcParameterBuffer);
static void kShowTaskList(const char *pcParameterBuffer);
static void kKillTask(const char *pcParameterBuffer);
static void kCPULoad(const char *pcParameterBuffer);

static void kCreateThreadTask(void);
static void kTestMutex(const char *pcParameterBuffer);
static void kTestThread(const char *pcParameterBuffer);
static void kShowMatrix(const char *pcParameterBuffer);

static void kTestPIE(const char *pcParameterBuffer);

static void kShowDynamicMemoryInformation(const char *pcParameterBuffer);
static void kTestSequentialAllocation(const char *pcParameterBuffer);
static void kTestRandomAllocation(const char *pcParameterBuffer);
static void kRandomAllocationTask(void);

static void kShowHDDInformation(const char *pcParameterBuffer);
static void kReadSector(const char *pcParameterBuffer);
static void kWriteSector(const char *pcParameterBuffer);

static void kMountHDD(const char *pcParameterBuffer);
static void kFormatHDD(const char *pcParameterBuffer);
static void kShowFileSystemInformation(const char *pcParameterBuffer);

static void kCreateFileInRootDirectory(const char *pcParameterBuffer);
static void kDeleteFileInRootDirectory(const char *pcParameterBuffer);
static void kShowRootDirectory(const char *pcParameterBuffer);

static void kWriteDataToFile(const char *pcParameterBuffer);
static void kReadDataFromFile(const char *pcParameterBuffer);
static void kTestFileIO(const char *pcParameterBuffer);

static void kFlushCache(const char *pcParameterBuffer);
static void kTestPerformance(const char *pcParameterBuffer);

static void kDownloadFile(const char *pcParameterBuffer);

static void kShowMPConfigurationTable(const char *pcParameterBuffer);

static void kStartApplicationProcessor(const char *pcParameterBuffer);

static void kStartSymmetricIOMode(const char *pcParameterBuffer);
static void kShowIRQINTINMappingTable(const char *pcParameterBuffer);

static void kShowInterruptProcessingCount(const char *pcParameterBuffer);
static void kStartInterruptLoadBalancing(const char *pcParameterBuffer);

static void kStartTaskLoadBalancing(const char *pcParameterBuffer);
static void kChangeTaskAffinity(const char *pcParameterBuffer);

static void kShowVBEModeInfo(const char *pcParameterBuffer);
static void kTestSystemCall(const char *pcParameterBuffer);

static void kExecuteApplicationProgram(const char *pcParameterBuffer);

static void kInstallPackage(const char *pcParameterBuffer);

#endif