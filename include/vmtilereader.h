#ifndef VM_TILE_READER_H

#include <QtGlobal>
#include <QString>
#include <stdint.h>
#include "openslide/openslide.h"
#define VM_TILE_READER_H 1
#define VM_READ_ERROR -1

#define VM_OPENSLIDE_TYPE 1
#define VM_OLYMPUS_TYPE 2

class vmTileReader
{
public:
  virtual void read(uint32_t* data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height) = 0;
  inline int getLastStatus() { return mLastStatus; }
  inline int getTileWidth() { return mTileWidth; }
  inline int getTileHeight() { return mTileHeight; }
  inline unsigned const char * getDefaultBkgd() { return mDefaultBkgd; }
  QString getErrorMsg() { return mErrorMsg; }
  //---------------------------------------------------------------------------
  // Below methods must be implemented in the subclasses
  //---------------------------------------------------------------------------
  virtual void close() = 0;
  virtual double getLevelDownsample(int32_t level) = 0;
  virtual void getLevelDimensions(int32_t level, int64_t *width, int64_t *height) = 0;
  virtual int32_t getTotalLevels() = 0;
  virtual int32_t getBestLevelForDownSample(double downSample) = 0;
  virtual QString getPropertyValue(const char * name) = 0;
  virtual ~vmTileReader() {}
protected:
  int mLastStatus;
  int mTileWidth, mTileHeight;
  unsigned char mDefaultBkgd[4];
  QString mErrorMsg;
};


class vmOpenSlideReader : public vmTileReader
{
public:
  vmOpenSlideReader(const char* filename);
  void read(uint32_t* data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height);
  virtual void close();
  virtual double getLevelDownsample(int32_t level);
  virtual void getLevelDimensions(int32_t level, int64_t *width, int64_t *height);
  virtual int32_t getTotalLevels();
  virtual int32_t getBestLevelForDownSample(double downSample);
  virtual QString getPropertyValue(const char * name);
  virtual ~vmOpenSlideReader(); 
protected:
  openslide_t* mOpenslide;
};

/*
class vmOlympusReader : public vmTileReader
{
  vmOlympusReader(const char* filename);
  int read(int64_t x, int64_t y, int level, int tileWidth, int tileHeight);
}
*/
#endif
