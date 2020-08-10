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
extern int cacheTotalTiles;
typedef struct vmTile vmTile;
typedef struct vmReadRequest vmReadRequest;
typedef struct vmDrawRequest vmDrawRequest;
typedef struct vmSlideCache vmSlideCache;
class vmSlide;
class vmTileReader;
class vmDrawingArea;
class vmZoomArea;
class vmUpdateRegion;

extern QVector<vmTile*> cacheTiles;

class vmUpdateRegion
{
public:
  vmUpdateRegion(double xNew, double yNew, double widthNew, double heightNew) { x = (int) floor(xNew); y = (int) floor(yNew); width = (int) ceil(widthNew); height = (int) ceil(heightNew); nextRegion = NULL; }
  vmUpdateRegion(int xNew, int yNew, int widthNew, int heightNew) { x = xNew; y = yNew; width = widthNew; height = heightNew; nextRegion = NULL; }
  int x, y;
  int width, height;
  vmUpdateRegion * nextRegion;
};

struct vmReadRequest
{
  vmSlide *sl;
  vmDrawingArea *drawingWidget;
  vmZoomArea *zoomArea;
  vmTile* tile;
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
  int64_t tileX;
  int64_t tileY;
  bool diskLoad;
  bool isOnNetwork;
  bool isZoom;
  //bool filled;
  //bool error;
  //bool redraw;
  bool alreadyReq;
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
  vmUpdateRegion *nextRegion;
  bool filled;
  bool error;
  uint32_t *data;
};

void cacheInitialize(int maxTiles);
void cacheSetCacheMax(int maxTiles);

void cacheInitTile(vmTile *t, vmSlideCache *Slide, int32_t Level, int64_t x, int64_t y);
bool cacheInitTileEx(vmTile *t, vmSlideCache *cache, int32_t level, int64_t x, int64_t y, int32_t width, int32_t height, bool allocMem);

void cacheFreeTile(vmTile *t);
  
void cacheRelease(vmSlideCache*);
void cacheReleaseTile(vmTile*);
void cacheReleaseAll();

void cacheClose(vmSlideCache *cache);

double cacheGetLevelDownsample(vmSlideCache *cache, int32_t level);

void cacheGetLevelDimensions(vmSlideCache *cache, int32_t level, int64_t* width, int64_t* height);

int32_t cacheGetQuickLevel(vmSlideCache *cache);

int32_t cacheGetLevelCount(vmSlideCache *cache);

int32_t cacheGetBestLevelForDownsample(vmSlideCache *cache, double downSample);

QString cacheGetPropertyValue(vmSlideCache *cache, const char *name);

inline int64_t cacheCalculateTile(double xRead, double downSample, int tileWidth) { return (int64_t) floor(floor(xRead / downSample) / (double) tileWidth); }

vmSlideCache* cacheOpen(const char* filename, int32_t fileType, QString& errorMsg, uint8_t*);
//vmReadRequest* cacheReadRegion(vmSlide*, safeBmp *bmp, double xRead, double yRead, int32_t level, double width, double height, QWidget* drawingWidget, bool loadFromDisk, bool isZoom);
vmReadRequest* cacheReadRegion(vmReadRequest *readReq);
vmTile * cacheFillRequest(vmReadRequest *readReq);
void cacheFillL2Requests(vmReadRequest *readReq, safeBmp *bmp, double downSample, int64_t width, int64_t height);
//void cacheCopyRequests(vmDrawRequest *drawReq);
//void cacheCopyRequests(vmReadRequest *readReq, safeBmp *imgSrc);
void cacheCopyRequests(vmReadRequest *readReq, vmReadRequest *readReq2, safeBmp *imgSrc1, safeBmp *imgSrc2);
void cacheQueueRedraw(vmReadRequest *readReq);
void cacheQueueRedraw(vmReadRequest *readReq, vmUpdateRegion *updateRegion);

void slideRef(vmSlide*);
void slideUnref(vmSlide*);
void slideMarkClose(vmSlide*);

bool cacheCheckRegion(vmSlideCache *cache, int64_t xRead, int64_t yRead, int32_t level, int64_t width, int64_t height);

#endif

