/**************************************************************************
Initial author: Paul F. Richards (paulrichards321@gmail.com) 2016-2019

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************/
#include <QtGlobal>
#include <QApplication>
#include <QtDebug>
#include <QFileInfo>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include "vmslideviewer.h"
#include "vmtilereader.h"
#include "slidecache.h"
#include "safebmp.h"

#define GUINT32TOBE(x) (x)

int cacheMaxTiles;
int cacheTotalTiles;
QVector<vmTile*> cacheTiles;
QMutex cacheMutex;

void vmSlide::slideMarkClose()
{
  cacheMutex.lock();
  done=true;
  cacheRelease(cache);
  cacheMutex.unlock();
}

void vmSlide::slideUnref()
{
  if (cache==NULL || cache->pReader==NULL) return;
  cacheMutex.lock();
  ref--;
  if (done && ref <= 0)
  {
    cacheRelease(cache);
    vmTileReader *pReader=cache->pReader;
    if (pReader)
    {
      cache->pReader=NULL;
      pReader->close();
      delete pReader;
    }
    vmSlideCache *oldCache=cache;
    cache=NULL;
    delete oldCache;
    slApp->removeSlide(slideNumber);
  }
  cacheMutex.unlock();
}

void vmSlide::slideRef()
{
  if (cache==NULL || cache->pReader==NULL) return;
  cacheMutex.lock();
  ref++;
  cacheMutex.unlock();
}


double cacheGetLevelDownsample(vmSlideCache *cache, int32_t level) 
{ 
  double downSample=0.0; 
  if (cache->pReader) downSample=cache->pReader->getLevelDownsample(level);
  return downSample; 
}

void cacheGetLevelDimensions(vmSlideCache *cache, int32_t level, int64_t* width, int64_t* height) 
{ 
  if (cache->pReader) 
  {
    cache->pReader->getLevelDimensions(level, width, height);
  }
}

int32_t cacheGetQuickLevel(vmSlideCache *cache)
{
  int32_t level=0;
  if (cache->pReader) level=cache->pReader->getQuickLevel();
  return level;
}

int32_t cacheGetLevelCount(vmSlideCache *cache) 
{ 
  int32_t count=0; 
  if (cache->pReader) count=cache->pReader->getTotalLevels();
  return count; 
}

int32_t cacheGetBestLevelForDownsample(vmSlideCache *cache, double downSample)
{
  int32_t bestLevel=0;
  if (cache->pReader) bestLevel=cache->pReader->getBestLevelForDownSample(downSample);
  return bestLevel;
}

QString cacheGetPropertyValue(vmSlideCache *cache, const char *name)
{
  if (cache->pReader) return cache->pReader->getPropertyValue(name);
  return NULL;
}


void cacheInitTile(vmTile *t, vmSlideCache *cache, int32_t level, int64_t x, int64_t y)
{
  t->slide=cache;
  t->level=level;
  t->x=x;
  t->y=y;
  t->data = NULL;
  t->error = false;
  t->filled = false;
  t->nextRegion = NULL;
}


bool cacheInitTileEx(vmTile *t, vmSlideCache *cache, int32_t level, int64_t x, int64_t y, int32_t width, int32_t height, bool allocMem)
{
  t->slide=cache;
  t->level=level;
  t->x=x;
  t->y=y;
  t->error = false;
  t->filled = false;
  t->nextRegion = NULL;
  uint32_t* bmpPtr = NULL;
  if (allocMem)
  {
    int64_t size=width * height;
    bmpPtr = new uint32_t[size];
    if (bmpPtr == NULL) return false;
    uint32_t backgroundRgba = cache->backgroundRgba;
    for (int64_t i = 0; i < size; i++) bmpPtr[i] = backgroundRgba;
  }
  t->data = bmpPtr;
  return true;
}


void cacheFreeTile(vmTile *t)
{
  delete[] t->data;
  t->data=NULL;
}

unsigned int LitToBigEndian(unsigned int x)
{
	return (((x>>24) & 0x000000ff) | ((x>>8) & 0x0000ff00) | ((x<<8) & 0x00ff0000) | ((x<<24) & 0xff000000));
}

void cacheSwapAlpha(uint32_t *buff, int64_t size)
{
  /*
  for (int64_t i = 0; i < size; i++) {
    uint32_t pixel = buff[i];
    uint8_t a = pixel >> 24;
    if (a == 255) {
        // Common case.  Compiles to a shift and a BSWAP.
        buff[i] = GUINT32TOBE(pixel << 8);
    } else if (a == 0) {
        // Less common case.  Hardcode white pixel; could also
        // use value from openslide.background-color property
        // if it exists
        buff[i] = GUINT32TOBE(0xffffff00u);
    } else {
        // Unusual case.
        uint8_t r = 255 * ((pixel >> 16) & 0xff) / a;
        uint8_t g = 255 * ((pixel >> 8) & 0xff) / a;
        uint8_t b = 255 * (pixel & 0xff) / a;
        buff[i] = GUINT32TOBE(r << 24 | g << 16 | b << 8);
    }
  }
  */
  // GDK uses ABGR and openslide returns ARGB
  for (int64_t i = 0; i < size; i++) {
    uint32_t pixel = buff[i];
    uint8_t a = pixel >> 24;
    if (a == 255) {
        // Common case.  Just reorder to gdk pixbuf ABGR.
        uint8_t r = ((pixel >> 16) & 0xff);
        uint8_t g = ((pixel >> 8) & 0xff);
        uint8_t b = (pixel & 0xff);
        buff[i] = GUINT32TOBE(0xff | b << 8 | g << 16 | r << 24);
    } else if (a == 0) {
        // Less common case.  Hardcode white pixel; could also
        // use value from openslide.background-color property
        // if it exists
        buff[i] = GUINT32TOBE(0xffffffffu);
    } else {
        // Unusual case.
        uint8_t r = 255 * ((pixel >> 16) & 0xff) / a;
        uint8_t g = 255 * ((pixel >> 8) & 0xff) / a;
        uint8_t b = 255 * (pixel & 0xff) / a;
        buff[i] = GUINT32TOBE(0xff | b << 8 | g << 16 | r << 24);
    }
  }
}


vmSlideCache* cacheOpen(const char *filename, int32_t fileType, QString &errorMsg, uint8_t *backgroundRgba)
{
  //uint8_t backgroundRgba2[4] = { 0, 0, 0, 0 };
  vmSlideCache *newSlide = NULL;
  
  if (filename == NULL) return NULL;

  newSlide = new vmSlideCache;
  memset((void*)newSlide, 0, sizeof(newSlide));

  if (fileType == VM_OPENSLIDE_TYPE)
  {
    newSlide->pReader = new vmOpenSlideReader(filename);
    if (newSlide->pReader->getLastStatus() == VM_READ_ERROR)
    {
      errorMsg = newSlide->pReader->getErrorMsg();
      delete newSlide->pReader;
      delete newSlide;
      return NULL;
    }
    newSlide->tileWidth = newSlide->pReader->getTileWidth();
    newSlide->tileHeight = newSlide->pReader->getTileHeight();
    if (backgroundRgba)
    {
      memcpy(&newSlide->backgroundRgba, backgroundRgba, 4);
    }
    else
    {
      memcpy(&newSlide->backgroundRgba, newSlide->pReader->getDefaultBkgd(), 4);
    }
  }
  else if (fileType == VM_GOOGLEMAPS_TYPE)
  {
    newSlide->pReader = new vmGoogleMaps(filename);
    if (newSlide->pReader->getLastStatus() == VM_READ_ERROR)
    {
      errorMsg = newSlide->pReader->getErrorMsg();
      delete newSlide->pReader;
      delete newSlide;
      return NULL;
    }
    newSlide->tileWidth = newSlide->pReader->getTileWidth();
    newSlide->tileHeight = newSlide->pReader->getTileHeight();
    memcpy(&newSlide->backgroundRgba, newSlide->pReader->getDefaultBkgd(), 4);
  }
  else
  {
    return NULL;
  }
  return newSlide;
}


void cacheRelease(vmSlideCache *cache)
{
  for (int i=0; i < cacheTotalTiles; i++)
  {
    vmTile *t=cacheTiles[i];
    if (t->slide == cache)
    {
      cacheTiles.remove(i);
      cacheTotalTiles--;
      delete[] t->data;
      delete t;
    }
  }
}


void cacheReleaseAll()
{
  for (int i=0; i < cacheTotalTiles; i++)
  {
    vmTile *t=cacheTiles[i];
    cacheTotalTiles--;
    delete[] t->data;
    delete t;
  }
  cacheTiles.clear();
  cacheTotalTiles = 0;
}


vmReadRequest* cacheReadRegion(vmReadRequest *readReq)
{
  safeBmp imgSrc;
  vmSlide* sl=readReq->sl;
  if (sl==NULL || sl->done==true) return NULL;
  vmSlideCache *cache = sl->cache;
  if (cache==NULL || cache->pReader==NULL || cache->tileWidth == 0 || cache->tileHeight == 0) return NULL;
  qDebug() << "xRead=" << (int) readReq->xRead << " yRead=" << (int) readReq->yRead << " width=" << (int) readReq->width << " height=" << readReq->height << "\n";
  safeBmpUint32Set(readReq->imgDest, cache->backgroundRgba);
  int32_t topLevel=cacheGetQuickLevel(cache);
  double downSample = cacheGetLevelDownsample(cache, readReq->level);
  double downSample2 = cacheGetLevelDownsample(cache, topLevel);
  //double resample = downSample2 / downSample;
  vmReadRequest *headReadReq = NULL;
  vmReadRequest *prevReadReq = NULL;

  int64_t levelWidth, levelHeight, levelWidth2, levelHeight2;
  cacheGetLevelDimensions(cache, readReq->level, &levelWidth, &levelHeight); 
  cacheGetLevelDimensions(cache, topLevel, &levelWidth2, &levelHeight2);
  int64_t totalWidth = round((double) levelWidth * downSample);
  int64_t totalHeight = round((double) levelHeight * downSample);
  int64_t totalWidth2 = round((double) levelWidth2 * downSample2);
  int64_t totalHeight2 = round((double) levelHeight2 * downSample2);
  double width = readReq->width * downSample;
  double height = readReq->height * downSample;
  if (totalWidth2 > totalWidth) totalWidth = totalWidth2;
  if (totalHeight2 > totalHeight) totalHeight = totalHeight2;
  if (readReq->xRead > totalWidth || readReq->yRead > totalHeight) return NULL;
  if (readReq->xRead + width < 0.0 || readReq->yRead + height < 0.0) return NULL;
  if (round(readReq->xRead + width) > totalWidth)
  {
    width = totalWidth - readReq->xRead;
    //readReq->width = (totalWidth - readReq->xRead) / readReq->downSample;
  }
  if (round(readReq->yRead + height) > totalHeight)
  {
    height = totalHeight - readReq->yRead;
    //readReq->height = (totalHeight - readReq->yRead) / readReq->downSample;
  }
  //int64_t tileX = readReq->tileX;
  //int64_t tileY = readReq->tileY;
  double startX = floor(floor(readReq->xRead / downSample) / (double) cache->tileWidth);
  double startY = floor(floor(readReq->yRead / downSample) / (double) cache->tileHeight);
  double maxX = (double) readReq->xRead + width; 
  double maxY = (double) readReq->yRead + height;
  int64_t endStepX = ceil(ceil(maxX / downSample) / (double) cache->tileWidth);  
  int64_t endStepY = ceil(ceil(maxY / downSample) / (double) cache->tileHeight);
  double endBlockX = (double) endStepX * downSample * (double) cache->tileWidth;
  double endBlockY = (double) endStepY * downSample * (double) cache->tileHeight;
  double addX = (double) cache->tileWidth * (double) downSample;
  double addY = (double) cache->tileHeight * (double) downSample;
  if (startX < 0)
  {
    if (endBlockX > 0) 
      startX = 0;
    else
      return NULL;
  }
  if (startY < 0)
  {
    if (endBlockY > 0) 
      startY = 0;
    else
      return NULL;
  }
  bool quit=false;
  for (int64_t yStep=startY; yStep < endStepY && quit==false; yStep++)
  {
    double yMarker = (double) yStep * (double) addY;
    for (int64_t xStep=startX; xStep < endStepX; xStep++)
    {
      double xMarker = (double) xStep * (double) addX;
      double xBlockMax=xMarker + addX;
      double xStart = (xMarker < readReq->xRead ? readReq->xRead : xMarker);
      double xSrc = xStart - xMarker;
      double xDest = xStart - readReq->xRead;
      if (xBlockMax > maxX) xBlockMax = maxX;
      double totalXCopy=xBlockMax - xStart;
      double yBlockMax=yMarker + addY;
      double yStart = (yMarker < readReq->yRead ? readReq->yRead : yMarker);
      double ySrc = yStart - yMarker;
      double yDest = yStart - readReq->yRead;
      if (yBlockMax > maxY) yBlockMax = maxY;
      double totalYCopy=yBlockMax - yStart;

      if (xSrc < 0 && xSrc+totalXCopy >= 0)
      {
        double xDiff=-xSrc;
        xSrc=0;
        xDest += xDiff;
        totalXCopy -= xDiff;
      }
      if (xDest < 0 && xDest+totalXCopy >= 0)
      {
        double xDiff=-xDest;
        xDest=0;
        xSrc += xDiff;
        totalXCopy -= xDiff;
      }
      xSrc = round(xSrc / downSample)-1;
      xDest = round(xDest / downSample);
      totalXCopy = ceil(totalXCopy / downSample);
 
      ySrc = round(ySrc / downSample)-1;
      yDest = round(yDest / downSample);
      totalYCopy = ceil(totalYCopy / downSample);

      if (totalXCopy <= 0 || totalYCopy <= 0) continue;
      totalXCopy+=2;
      totalYCopy+=2;

      bool found=false;
      bool filled=false;
      bool error=false;
      vmTile *tile=NULL;
      int index=0;
      cacheMutex.lock();
      if (sl->done == true)
      {
        cacheMutex.unlock();
        headReadReq=NULL;
        quit=true;
        break;
      }
      while (index < cacheTotalTiles)
      {
        tile=cacheTiles[index];
        if (tile->slide==cache && 
            tile->level==readReq->level && 
            tile->x==xStep && 
            tile->y==yStep)
        {
          found=true;
          filled=tile->filled;
          error=tile->error;
          if (filled==false && error==false)
          {
            vmUpdateRegion * updateRegion = tile->nextRegion;
            vmUpdateRegion * previousRegion = NULL;
            vmUpdateRegion * newRegion = new vmUpdateRegion(readReq->xDest+xSrc, readReq->yDest+ySrc, totalXCopy, totalYCopy);
            do 
            {
              previousRegion = updateRegion;
              updateRegion = updateRegion->nextRegion;
            } 
            while (updateRegion != NULL);
            previousRegion->nextRegion = newRegion;
          }
          cacheTiles.remove(index);
          cacheTiles.append(tile);
          break;
        }
        index++;
      }
      cacheMutex.unlock();
      uint32_t *imgPtr=NULL;
      //uint32_t *tempImgPtr=NULL;
      //bool diskLoad = false, alreadyReq = false, redraw = false;
      if (found) 
      {
        if (filled)
        {
          imgPtr = tile->data;
        }
      }
      else     
      {
        tile=new vmTile;
        bool initTileMem = readReq->isOnNetwork ? false : true;
        if (tile && cacheInitTileEx(tile, cache, readReq->level, floor(floor(xMarker / downSample) / (double) cache->tileWidth), floor(floor(yMarker / downSample) / (double) cache->tileHeight), cache->tileWidth, cache->tileHeight, initTileMem))
        {
          tile->nextRegion = new vmUpdateRegion(readReq->xDest+xSrc, readReq->yDest+ySrc, totalXCopy, totalYCopy);
          qDebug() << "cache miss at openslide x=" << (int) floor(xMarker) << " y=" << (int) floor(yMarker) << "\n";
          cacheMutex.lock();
          if (sl->done)
          {
            cacheMutex.unlock();
            headReadReq=NULL;
            quit=true;
            break;
          }
          if (cacheTotalTiles >= cacheMaxTiles)
          {
            qDebug() << "Evicting tile!\n";
            int64_t i;
            for (i=0; i<cacheTotalTiles; i++)
            {
              tile=cacheTiles[i];
              if (tile->slide==cache && tile->level != topLevel && tile->filled == true) break;
            }
            if (i >= cacheTotalTiles) 
            {
              i=0;
              tile=cacheTiles[i];
            }
            cacheTiles.remove(i);
            cacheTotalTiles--;
            delete[] tile->data;
            tile->data = NULL;
            delete tile;
            tile=NULL;
          }
          cacheTiles.append(tile);
          cacheTotalTiles++;
          cacheMutex.unlock();
   
          if (readReq->isOnNetwork == false && readReq->diskLoad)
          {
            cache->pReader->read(tile->data, round(xMarker), round(yMarker), readReq->level, cache->tileWidth, cache->tileHeight);
            //cacheSwapAlpha(tile->data, cache->tileWidth * cache->tileHeight);  
            imgPtr = tile->data;
            cacheMutex.lock();
            tile->filled = true;
            if (sl->done)
            {
              cacheMutex.unlock();
              headReadReq = NULL;
              quit = true;
              break;
            }
            cacheMutex.unlock();
            filled = true;
          }
        }
      }
      if (filled==false)
      {
        vmReadRequest *readReq2 = new vmReadRequest;
        memset(readReq2, 0, sizeof(vmReadRequest));
        sl->slideRef();
        readReq2->drawingWidget = readReq->drawingWidget;
        readReq2->zoomArea = readReq->zoomArea;
        readReq2->sl = sl;
        readReq2->tile = tile;
        readReq2->downSample = readReq->downSample;
        readReq2->xMarker = xMarker;
        readReq2->yMarker = yMarker;
        readReq2->xRead = readReq->xRead;
        readReq2->yRead = readReq->yRead;
        readReq2->width = xBlockMax - xStart;
        readReq2->height = yBlockMax - yStart;
        readReq2->level = readReq->level;
        readReq2->isZoom = readReq->isZoom;
        readReq2->xStart = readReq->xStart;
        readReq2->yStart = readReq->yStart;
        readReq2->xDest = readReq->xDest + xDest;
        readReq2->yDest = readReq->yDest + yDest;
        /*
        if (readReq2->isZoom)
        {
          //readReq2->xDest = readReq->xDest + round((xMarker - readReq->xRead) / readReq2->downSample);
          //readReq2->yDest = readReq->yDest + round((yMarker - readReq->yRead) / readReq2->downSample);
        }
        else
        {
          //readReq2->xDest = readReq->xDest + round((xMarker - readReq->xRead) / readReq->downSample);
          //readReq2->yDest = readReq->yDest + round((yMarker - readReq->yRead) / readReq->downSample);
        }*/
        readReq2->diskLoad = true;
        readReq2->isOnNetwork = readReq->isOnNetwork;
        readReq2->alreadyReq = found;
        if (prevReadReq)
        {
          prevReadReq->tail = readReq2;
        }
        else
        {
          headReadReq = readReq2;
        }
        prevReadReq = readReq2;
      }
      else
      {
        safeBmpInit(&imgSrc, imgPtr, cache->tileWidth, cache->tileHeight);
        safeBmpCpy(readReq->imgDest, xDest, yDest, &imgSrc, xSrc, ySrc, totalXCopy, totalYCopy);
      }
    }
  }

  if (headReadReq)
  {
    if (headReadReq->isOnNetwork==true)
    {
      vmReadRequest * nextReq = headReadReq;
      slApp->netMutex.lock();
      do
      {
        if (nextReq->alreadyReq == false)
        {
          //slApp->netReqs.append(nextReq);
          slApp->totalNetReqs++;
        }
        nextReq = nextReq->tail;
      }
      while (nextReq);
      slApp->netMutex.unlock();

      nextReq = headReadReq;
      do
      {
        if (nextReq->alreadyReq == false)
        {
          qDebug() << "Requesting nextReq @ " << nextReq;
          cache->pReader->networkRead(nextReq);
        }
        nextReq = nextReq->tail;
      }
      while (nextReq);
    }
    else
    {
      bool startReadThread = false;
      vmReadRequest * nextReq = headReadReq;
      slApp->readMutex.lock();
      do
      {
        if (nextReq->alreadyReq == false)
        {
          slApp->readReqs.append(nextReq);
          slApp->totalReadReqs++;
          startReadThread = true;
        }
        nextReq = nextReq->tail;
      }
      while (nextReq);
      slApp->readMutex.unlock();
      
      if (startReadThread)
      {
        slApp->readCondMutex.lock();
        slApp->readCond.wakeOne();
        slApp->readCondMutex.unlock();
      }
    } 
  }
  return headReadReq;
}


void cacheCopyRequests(vmReadRequest *readReq, vmReadRequest *readReq2, safeBmp *imgSrc1, safeBmp *imgSrc2)
{
  if (readReq->sl==NULL || readReq->sl->getDone()==true) return;
  vmSlideCache *cache = readReq->sl->getCache();
  int32_t topLevel=cacheGetLevelCount(cache)-1;
  double downSample1 = cacheGetLevelDownsample(cache, readReq->level);
  double downSample2 = cacheGetLevelDownsample(cache, topLevel);

  int64_t levelWidth, levelHeight, levelWidth2, levelHeight2;
  cacheGetLevelDimensions(cache, readReq->level, &levelWidth, &levelHeight); 
  cacheGetLevelDimensions(cache, topLevel, &levelWidth2, &levelHeight2);
  int64_t totalWidth = round((double) levelWidth * downSample1);
  int64_t totalHeight = round((double) levelHeight * downSample1);
  int64_t totalWidth2 = round((double) levelWidth2 * downSample2);
  int64_t totalHeight2 = round((double) levelHeight2 * downSample2);
  double width = (double) readReq->width * downSample1;
  double height = (double) readReq->height * downSample1;
  if (totalWidth2 > totalWidth) totalWidth = totalWidth2;
  if (totalHeight2 > totalHeight) totalHeight = totalHeight2;
  if (readReq->xRead > totalWidth || readReq->yRead > totalHeight) return;
  if (readReq->xRead + width < 0.0 || readReq->yRead + height < 0.0) return;
  if (round(readReq->xRead + width) > totalWidth)
  {
    width = totalWidth - readReq->xRead;
  }
  if (round(readReq->yRead + height) > totalHeight)
  {
    height = totalHeight - readReq->yRead;
  }
  double maxX = (double) readReq->xRead + width;
  double maxY = (double) readReq->yRead + height;
  safeBmpCpy(readReq->imgDest, 0, 0, imgSrc1, 0, 0, imgSrc1->width, imgSrc1->height);

  vmReadRequest* nextReq = readReq2; 
  while (nextReq)
  {
    double xMarker = nextReq->xMarker;
    double yMarker = nextReq->yMarker;

    double xStart = (xMarker < nextReq->xRead ? nextReq->xRead : xMarker);
    double xBlockMax=xStart + nextReq->width;
    double xSrc = xStart - nextReq->xRead;
    if (xBlockMax > maxX) xBlockMax = maxX;
    double totalXCopy=xBlockMax - xStart;
    
    double yStart = (yMarker < nextReq->yRead ? nextReq->yRead : yMarker);
    double yBlockMax=yStart + nextReq->height;
    double ySrc = yStart - nextReq->yRead;
    if (yBlockMax > maxY) yBlockMax = maxY;
    double totalYCopy=yBlockMax - yStart;

    totalXCopy = ceil(totalXCopy / nextReq->downSample) + 2;
    xSrc = round(xSrc / nextReq->downSample) - 1;
    totalYCopy = ceil(totalYCopy / nextReq->downSample) + 2;
    ySrc = round(ySrc / nextReq->downSample) - 1;
    safeBmpCpy(readReq->imgDest, xSrc, ySrc, imgSrc2, xSrc, ySrc, totalXCopy, totalYCopy);
    
    vmReadRequest * previousReq = nextReq;
    nextReq = nextReq->tail;
    previousReq->tail = NULL;
    if (previousReq->alreadyReq)
    {
      delete previousReq;
    }
  }
}


vmTile* cacheFillRequest(vmReadRequest *readReq)
{
  if (readReq==NULL || readReq->sl==NULL || readReq->sl->done==true) return NULL;
  vmSlideCache *cache = readReq->sl->cache;
  if (cache==NULL || cache->pReader==NULL || cache->tileWidth == 0 || cache->tileHeight == 0) return NULL;
  //int32_t topLevel=cacheGetQuickLevel(cache);
  //double downSample = cacheGetLevelDownsample(cache, readReq->level);
  bool redraw=false;
  //bool found=false;
  vmTile* tile = readReq->tile;
  //int index=0;
  double xMarker = readReq->xMarker;
  double yMarker = readReq->yMarker;
  //int64_t tileX = readReq->tileX;
  //int64_t tileY = readReq->tileY;
  //int64_t tileX = floor(floor(readReq->xMarker / downSample) / (double) cache->tileWidth);
  //int64_t tileY = floor(floor(readReq->yMarker / downSample) / (double) cache->tileHeight);
  bool alreadyFilled = false;
  
  cache = readReq->sl->cache;
  
  cacheMutex.lock();
  alreadyFilled = tile->filled;
  cacheMutex.unlock();
  
  if (alreadyFilled == false)
  {
    qDebug() << "Filling read request " << readReq << "\n";
    int tileSize = cache->tileWidth * cache->tileHeight * 4;
    char * tileData = new char[tileSize];
    memset(tileData, 255, tileSize);
    qDebug() << "Reading region xMarker=" << (int) xMarker << " yMarker=" << (int) yMarker << "\n";
    cache->pReader->read((uint32_t*)tileData, round(xMarker), round(yMarker), readReq->level, cache->tileWidth, cache->tileHeight);
    //cacheSwapAlpha(tile->data, cache->tileWidth * cache->tileHeight);  
    bool done = false;
    cacheMutex.lock();
    tile->data = (uint32_t*) tileData;
    tile->filled = true;
    done = readReq->sl->done;
    cacheMutex.unlock();
    if (done == false)
    {
      redraw = true;
    }
  }
  if (redraw) return tile;
  return NULL;
}


void cacheSetCacheMax(int maxTiles)
{
  cacheMaxTiles = maxTiles;
}

void cacheInitialize(int maxTiles)
{
  cacheTiles.clear();
  cacheMaxTiles = maxTiles;
}


void cacheQueueRedraw(vmReadRequest *readReq)
{
  int x=readReq->xDest;
  int y=readReq->yDest;
  double downSampleLevel = readReq->sl->pyramidLevels[readReq->level].downSample;
  qDebug() << "cacheQueueRedraw: in slidecache.cc:run original width=" << readReq->width << " original height=" << readReq->height << " downsample: " << readReq->downSample << " level downsample: " << downSampleLevel;
  int width=round((double) readReq->width / readReq->downSample);
  int height=round((double) readReq->height / readReq->downSample);
  qDebug() << "in vmthreads.cc:run Marked updated region: x=" << x << " y=" << y << " width=" << width << " height=" << height << "\n";
  if (readReq->isZoom)
  {
    readReq->zoomArea->updateZoomRect(x, y, width, height);
  }
  else
  {
    readReq->drawingWidget->updateDrawingRect(x, y, width, height);
  }
}


void cacheQueueRedraw(vmReadRequest *readReq, vmUpdateRegion *updateRegion)
{
  qDebug() << "in vmthreads.cc:run Marked updated region: x=" << updateRegion->x << " y=" << updateRegion->y << " width=" << updateRegion->width << " height=" << updateRegion->height << "\n";
  if (readReq->isZoom)
  {
    readReq->zoomArea->updateZoomRect(updateRegion->x, updateRegion->y, updateRegion->width, updateRegion->height);
  }
  else
  {
    readReq->drawingWidget->updateDrawingRect(updateRegion->x, updateRegion->y, updateRegion->width, updateRegion->height);
  }
}

