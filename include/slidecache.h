#ifndef CACHEDSLIDE
#define CACHEDSLIDE

#include <QString>
#include <QWidget>
#include <QVector>
#include <unistd.h>
#include "safebmp.h"
#include "vmslideviewer.h"
#include "vmtilereader.h"

extern int cacheMaxTiles; 
typedef struct vmTile vmTile;
typedef struct vmReadRequest vmReadRequest;
typedef struct vmDrawRequest vmDrawRequest;
typedef struct vmSlideCache vmSlideCache;
class vmSlide;
class vmTileReader;
class vmDrawingArea;
class vmZoomArea;

extern QVector<vmTile*> cacheTiles;

struct vmReadRequest
{
  vmSlide *sl;
  vmDrawingArea *drawingWidget;
  vmZoomArea *zoomArea;
  double xRead;
  double yRead;
  double width;
  double height;
  int xStart;
  int yStart;
  int32_t level;
  double downSample;
  double xMarker;
  double yMarker;
  bool diskLoad;
  bool isZoom;
  bool filled;
  vmReadRequest *tail;
  safeBmp* imgDest;
  int xDest;
  int yDest;
};


struct vmSlideCache
{
  vmTileReader *pReader; // openslide_t *slide;
  int32_t tileWidth;
  int32_t tileHeight;
  uint32_t backgroundRgba;
};

struct vmTile
{
  vmSlideCache *slide;
  int32_t level;
  int64_t x;
  int64_t y;
  uint32_t *data;
};

void cacheInitialize(int maxTiles);
void cacheSetCacheMax(int maxTiles);

uint32_t* cacheInitTile(vmTile *t, vmSlideCache *Slide, int32_t Level, int64_t x, int64_t y, int32_t width, int32_t height);
void cacheFreeTile(vmTile *t);
  
void cacheRelease(vmSlideCache*);
void cacheReleaseTile(vmTile*);
void cacheReleaseAll();

void cacheClose(vmSlideCache *cache);

double cacheGetLevelDownsample(vmSlideCache *cache, int32_t level);

void cacheGetLevelDimensions(vmSlideCache *cache, int32_t level, int64_t* width, int64_t* height);

int32_t cacheGetLevelCount(vmSlideCache *cache);

int32_t cacheGetBestLevelForDownsample(vmSlideCache *cache, double downSample);

QString cacheGetPropertyValue(vmSlideCache *cache, const char *name);

vmSlideCache* cacheOpen(const char* filename, int32_t fileType, QString& errorMsg, uint8_t*);
//vmReadRequest* cacheReadRegion(vmSlide*, safeBmp *bmp, double xRead, double yRead, int32_t level, double width, double height, QWidget* drawingWidget, bool loadFromDisk, bool isZoom);
vmReadRequest* cacheReadRegion(vmReadRequest *readReq);
bool cacheFillRequests(vmReadRequest *readReq);
void cacheFillL2Requests(vmReadRequest *readReq, safeBmp *bmp, double downSample, int64_t width, int64_t height);
//void cacheCopyRequests(vmDrawRequest *drawReq);
//void cacheCopyRequests(vmReadRequest *readReq, safeBmp *imgSrc);
void cacheCopyRequests(vmReadRequest *readReq, vmReadRequest *readReq2, safeBmp *imgSrc1, safeBmp *imgSrc2);

void slideRef(vmSlide*);
void slideUnref(vmSlide*);
void slideMarkClose(vmSlide*);

bool cacheCheckRegion(vmSlideCache *cache, int64_t xRead, int64_t yRead, int32_t level, int64_t width, int64_t height);

#endif

