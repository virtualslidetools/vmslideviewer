#ifndef SAFE_BMP
#define SAFE_BMP
#ifdef __cplusplus
#include <cstdint>
#else
#include <string.h>
#endif

typedef struct 
{
  int width;
  int height;
  int strideWidth;
  uint32_t *data;
  bool freeData;
  bool freePtr;
} safeBmp;

safeBmp * safeBmpAlloc(int width, int height);
uint32_t * safeBmpAlloc2(safeBmp*, int width, int height);
void safeBmpInit(safeBmp *bmp, uint32_t *data, int width, int height);
safeBmp* safeBmpSrc(uint32_t *data, int64_t width, int64_t height);
void safeBmpUint32Set(safeBmp *bmp, uint32_t value);
inline void safeBmpByteSet(safeBmp *bmp, int value) { memset(bmp->data, value, bmp->strideWidth * bmp->height); }
void safeBmpFree(safeBmp *bmp);
inline void safeBmpClear(safeBmp *bmp) { bmp->width = 0; bmp->height = 0; bmp->strideWidth = 0; bmp->data = 0; bmp->freeData = false; bmp->freePtr = false; }
void safeBmpCpy(safeBmp *bmpDest, int x_dest, int y_dest, safeBmp *bmp_src, int x_src, int y_src, int cols, int rows);

#endif

