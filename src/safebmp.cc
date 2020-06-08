#include <cstdlib>
#include <iostream>
#include <new>
#include <cstring>
#include "safebmp.h"

safeBmp *safeBmpAlloc(int width, int height)
{
  //int stideWidth = cairoFormatStrideForWidth(CAIROFORMATRGB24, width);
  safeBmp *bmp = NULL;
  int strideWidth = width;
  try
  {
    bmp = new safeBmp;
    if (bmp == NULL)
    {
      std::cerr << "Failed to allocate image of memory size=" << ((strideWidth * height) / 1024) << " kb." << std::endl;
      exit(1);
    }
    bmp->freePtr = true;
    bmp->height = height;
    bmp->width = width;
    bmp->strideWidth = width;
    if (strideWidth > 0 && height > 0)
    {
      bmp->data = new uint32_t[strideWidth * height];
      if (bmp->data == NULL) 
      {
        std::cerr << "Failed to allocate image of memory size=" << ((strideWidth * height) / 1024) << " kb." << std::endl;
        exit(1);
      }
      bmp->freeData = true;
    }
    else
    {
      bmp->data = NULL;
      bmp->freeData = false;
    }
  }
  catch (std::bad_alloc &xa)
  {
    std::cerr << "Failed to allocate image of memory size=" << ((strideWidth * height) / 1024) << " kb." << std::endl;
    exit(1);
  }
  return bmp;
}

uint32_t *safeBmpAlloc2(safeBmp *bmp, int width, int height)
{
  //int stideWidth = cairoFormatStrideForWidth(CAIROFORMATRGB24, width);
  try
  {
    if (width > 0 && height > 0)
    {
      bmp->data = new uint32_t[width * height];
    }
    else
    {
      bmp->data = NULL;
    }
  }
  catch (std::bad_alloc &xa)
  {
    bmp->data = NULL;
    std::cerr << "Failed to allocate image of memory size=" << ((width * height) / 1024) << " kb." << std::endl;
    exit(1);
  }
  if (bmp->data)
  {
    bmp->freeData = true;
    bmp->freePtr = false;
    bmp->strideWidth = width;
    bmp->height = height;
    bmp->width = width;
  }
  else
  {
    bmp->freeData = false;
    bmp->freePtr = false;
    bmp->strideWidth = 0;
    bmp->height = 0;
    bmp->width = 0;
  }
  return bmp->data;
}


safeBmp* safeBmpSrc(uint32_t *data, int64_t width, int64_t height)
{
  safeBmp *bmp=NULL;
  try 
  {
    bmp=new safeBmp;
  }
  catch (std::bad_alloc &xa)
  {
    std::cerr << "Failed to allocate bitmap struture!" << std::endl; 
    exit(1);
  }
  if (!bmp) return NULL;

  bmp->width = width;
  bmp->strideWidth = width;
  bmp->height = height;
  bmp->data = data;
  bmp->freeData = false;
  bmp->freePtr = true;

  return bmp;
}


void safeBmpInit(safeBmp *bmp, uint32_t *bmpPtr, int width, int height)
{
  bmp->width = width;
  bmp->strideWidth = width;
  //  bmp.strideWidth = cairoFormatStrideForWidth(CAIROFORMATRGB24, width);
  bmp->height = height;
  bmp->data = bmpPtr;
  bmp->freeData = false;
  bmp->freePtr = false;
}


void safeBmpFree(safeBmp *bmp)
{
  if (!bmp) return;
  if (bmp->freeData && bmp->data)
  {
    delete[] bmp->data;
    bmp->data = NULL;
  }
  if (bmp->freePtr)
  {
    delete bmp;
  }
}


void safeBmpUint32Set(safeBmp *bmp, uint32_t value)
{
  uint64_t size=bmp->strideWidth * bmp->height;
  uint32_t *data = bmp->data;
  for (uint64_t i=0; i < size; i++) data[i] = value;
}


void safeBmpCpy(safeBmp *bmpDest, int xDest, int yDest, safeBmp *bmpSrc, int xSrc, int ySrc, int cols, int rows)
{
  int xEnd = xSrc + cols;
  if (xSrc < 0)
  {
    cols += xSrc;
    xSrc = 0;
  }
  if (xEnd < 0 || xSrc > bmpSrc->width || cols <= 0 ||
      xDest > bmpDest->width) 
  {
    return;
  }
  if (xEnd > bmpSrc->width)
  {
    cols = bmpSrc->width - xSrc;
    if (cols < 0) return;
  }
  int yEnd = ySrc + rows;
  if (ySrc < 0)
  {
    rows += ySrc;
    ySrc = 0;
  }
  if (yEnd < 0 || ySrc > bmpSrc->height || rows <= 0 ||
      yDest > bmpDest->height) 
  {
    return;
  }
  if (yEnd > bmpSrc->height)
  {
    rows = bmpSrc->height - ySrc;
  }
  if (xDest < 0)
  {
    cols += xDest;
    xDest = 0;
  }
  if (xDest + cols > bmpDest->width)
  {
    cols = bmpDest->width - xDest;
  }
  if (yDest < 0)
  {
    rows += yDest;
    yDest = 0;
  }
  if (yDest + rows > bmpDest->height)
  {
    rows = bmpDest->height - yDest;
  }
  if (rows < 0 || cols <= 0) return;
  int srcRowWidth = bmpSrc->strideWidth * 4;
  int destRowWidth = bmpDest->strideWidth * 4;
  int xSrcOffset = xSrc * 4;
  int xDestOffset = xDest * 4;
  int colCopySize=cols*4;
  uint8_t *destData = (uint8_t*) bmpDest->data;
  uint8_t *srcData = (uint8_t*) bmpSrc->data;
  for (int y = 0; y < rows; y++)
  {
    int64_t src = ((y+ySrc) * srcRowWidth) + xSrcOffset;
    int64_t dest = ((y+yDest) * destRowWidth) + xDestOffset;
    memcpy(&destData[dest], &srcData[src], colCopySize);
  }
}

