#include "2DGraphics.h"
#include "Types.h"

typedef struct
{
    int elem;
    unsigned short code[256];
    unsigned char size[256];
    unsigned char value[256];
} HUFF;

typedef struct
{

    int width;
    int height;

    int mcu_width;
    int mcu_height;

    int max_h, max_v;
    int compo_count;
    int compo_id[3];
    int compo_sample[3];
    int compo_h[3];
    int compo_v[3];
    int compo_qt[3];

    int scan_count;
    int scan_id[3];
    int scan_ac[3];
    int scan_dc[3];
    int scan_h[3];
    int scan_v[3];
    int scan_qt[3];

    int interval;

    int mcu_buf[32 * 32 * 4];
    int *mcu_yuv[4];
    int mcu_preDC[3];

    int dqt[3][64];
    int n_dqt;

    HUFF huff[2][3];

    unsigned char *data;
    int data_index;
    int data_size;

    unsigned long bit_buff;
    int bit_remain;

} JPEG;

BOOL kJPEGInit(JPEG *jpeg, BYTE *pbFileBuffer, DWORD dwFileSize);
BOOL kJPEGDecode(JPEG *jpeg, COLOR *rgb);