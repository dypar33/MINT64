#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define FILESYSTEM_SIGNATURE 0x7E38CF10
#define FILESYSTEM_SECTORSPERCLUSTER 8
#define FILESYSTEM_LASTCLUSTER 0xFFFFFFFF
#define FILESYSTEM_FREECLUSTER 0x00
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT ((FILESYSTEM_SECTORSPERCLUSTER * 512) / sizeof(DIRECTORYENTRY))
#define FILESYSTEM_CLUSTERSIZE (FILESYSTEM_SECTORSPERCLUSTER * 512)

#define FILESYSTEM_HANDLE_MAXCOUNT (TASK_MAXCOUNT * 3)

#define FILESYSTEM_MAXFILENAMELENGTH 24

#define FILESYSTEM_TYPE_FREE 0
#define FILESYSTEM_TYPE_FILE 1
#define FILESYSTEM_TYPE_DIRECTORY 2

#define FILESYSTEM_SEEK_SET 0
#define FILESYSTEM_SEEK_CUR 1
#define FILESYSTEM_SEEK_END 2

typedef BOOL (*fReadHDDInformation) (BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int (*fReadHDDSector) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int (*fWriteHDDSector) (BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

#define fopen kOpenFile
#define fread kReadFile
#define fwrite kWriteFile
#define fseek kSeekFile
#define fclose kCloseFile
#define remove kRemoveFile
#define opendir kOpenDirectory
#define readdir kReadDirectory
#define rewinddir kRewindDirectory
#define closedir kCloseDirectory

#define SEEK_SET FILESYSTEM_SEEK_SET
#define SEEK_CUR FILESYSTEM_SEEK_CUR
#define SEEK_END FILESYSTEM_SEEK_END

#define size_t DWORD
#define dirent kDirectoryEntryStruct
#define d_name vcFileName

#pragma pack(push, 1)

typedef struct kPartitionStruct
{
    BYTE bBootableFlag;             // 부팅 가능 flag
    BYTE vbStartingCHSAddress[3];   // 파티션 CHS 시작 주소
    BYTE bPartitionType;            // 파티션 종류 (fat32 같은)
    BYTE vbEndingCHSAddress[3];     // 파티션 끝 주소

    DWORD dwStartingLBAAddress;     // 파티션 LBA 시작 주소
    DWORD dwSizeInSector;           // 총 섹터 수
} PARTITION;

typedef struct kMBRStruct
{
    BYTE vbBootCode[430];           // 부트 로더 코드

    DWORD dwSignature;              // 파일 시스템 시그니처
    DWORD dwReservedSectorCount ;    // 예약된 영역 섹터 수
    DWORD dwClusterLinkSectorCount; // 클러스터 링크 테이블 영역 섹터 수
    DWORD dwTotalClusterCount;      // 총 클러스터 개수 

    PARTITION vstPartition[4];      // 4개의 파티션 정보 테이블

    BYTE vbBootLoaderSignature[2];  // 부트로더 시그니처 (0x55, 0xAA)
} MBR;

typedef struct kDirectoryEntryStruct
{
    char vcFileName[FILESYSTEM_MAXFILENAMELENGTH]; // 파일 이름

    DWORD dwFileSize;                              // 파일 사이즈
    DWORD dwStartClusterIndex;                     // 시작 클러스터 인덱스
} DIRECTORYENTRY;

typedef struct kFileHandleStruct
{
    int iDirectoryEntryOffset;

    DWORD dwFileSize;

    DWORD dwStartClusterIndex;
    DWORD dwCurrentClusterIndex;
    DWORD dwPreviousClusterIndex;
    DWORD dwCurrentOffset;
} FILEHANDLE;

typedef struct kDirectoryHandleStruct
{
    DIRECTORYENTRY* pstDirectoryBuffer;
    int iCurrentOffset;
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct
{
    BYTE bType;

    union
    {
        FILEHANDLE stFileHandle;
        DIRECTORYHANDLE stDirectoryHandle;
    };
} FILE, DIR;

#pragma pack(pop)

typedef struct kFileSystemManagerStruct
{
    BOOL bMounted;                                  // mount 됐는지 (정상적으로 인식 됐는지)

    // 각 영역의 섹터 수 및 LBA 시작 주소
    DWORD dwReservedSectorCount;                  
    DWORD dwClusterLinkAreaStartAddress;
    DWORD dwClusterLinkAreaSize;
    DWORD dwDataAreaStartAddress;
    
    DWORD dwTotalClusterCount;                    // 데이터 영역의 클러스터 총 개수

    DWORD dwLastAllocatedClusterLinkSectorOffset; // 마지막으로 할당된 클러스터 링크 테이블으 섹터 offset

    MUTEX stMutex;                                // 동기화 객체

    FILE* pstHandlePool;
} FILESYSTEMMANAGER;


BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);

static DWORD kFindFreeCluster(void);
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* dwData);

static int kFindFreeDirectoryEntry(void);
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);

void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

FILE* kOpenFile(const char* pcFileName, const char* pcMode);
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin);
int kCloseFile(FILE* pstFile);
int kRemoveFile(const char* pcFileName);
DIR* kOpenDirectory(const char* pcDirectoryName);
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory);
void kRewindDirectory(DIR* pstDirectory);
int kCloseDirectory(DIR* pstDirectory);
BOOL kWriteZero(FILE* pstFile, DWORD dwCount);
BOOL kIsFileOpened(const DIRECTORYENTRY* pstEntry);

static void* kAllocateFileDirectoryHandle(void);
static void kFreeFileDirectoryHandle(FILE* pstFile);
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex);
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex);
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle);

#endif