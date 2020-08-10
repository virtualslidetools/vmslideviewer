#ifndef VM_TILE_READER_H
#define VM_TILE_READER_H 1
#include <QtGlobal>
#include <QString>
#include <QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <stdint.h>
#include "openslide/openslide.h"
#include "slidecache.h"
#define VM_TILE_READER_H 1
#define VM_READ_ERROR -1

#define VM_OPENSLIDE_TYPE 1
#define VM_OLYMPUS_TYPE 2
#define VM_GOOGLEMAPS_TYPE 3

class vmTileReader
{
public:
  virtual void read(uint32_t* data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height) = 0;
  virtual void networkRead(struct vmReadRequest *readReq) = 0;
  inline int getLastStatus() { return mLastStatus; }
  inline int getTileWidth() { return mTileWidth; }
  inline int getTileHeight() { return mTileHeight; }
  inline unsigned const char * getDefaultBkgd() { return mDefaultBkgd; }
  inline double getSlideDepth() { return mSlideDepth; } 
  inline QString getErrorMsg() { return mErrorMsg; }
  inline bool getIsOnNetwork() { return mIsOnNetwork; }
  //---------------------------------------------------------------------------
  // Below methods must be implemented in the subclasses
  //---------------------------------------------------------------------------
  virtual void close() = 0;
  virtual double getLevelDownsample(int32_t level) = 0;
  virtual void getLevelDimensions(int32_t level, int64_t *width, int64_t *height) = 0;
  virtual int32_t getTotalLevels() = 0;
  virtual int32_t getQuickLevel() = 0;
  virtual int32_t getBestLevelForDownSample(double downSample) = 0;
  virtual QString getPropertyValue(const char * name) = 0;
  virtual ~vmTileReader() {}
protected:
  int mLastStatus;
  int mTileWidth, mTileHeight;
  unsigned char mDefaultBkgd[4];
  QString mErrorMsg;
  double mSlideDepth;
  bool mIsOnNetwork;
};


class vmOpenSlideReader : public vmTileReader
{
public:
  vmOpenSlideReader(const char* filename);
  void read(uint32_t* data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height);
  void networkRead(struct vmReadRequest* readReq);
  virtual void close();
  virtual double getLevelDownsample(int32_t level);
  virtual void getLevelDimensions(int32_t level, int64_t *width, int64_t *height);
  virtual int32_t getQuickLevel();
  virtual int32_t getTotalLevels();
  virtual int32_t getBestLevelForDownSample(double downSample);
  virtual QString getPropertyValue(const char * name);
  virtual ~vmOpenSlideReader(); 
protected:
  openslide_t* mOpenslide;
};


class vmNetworkAccessManager : public QNetworkAccessManager
{
  Q_OBJECT
public slots:
  virtual void replyFinished(QNetworkReply *reply);
};


class vmGoogleMaps : public vmTileReader
{
public:
  vmGoogleMaps(const char* filename);
  void read(uint32_t* data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height);
  void networkRead(struct vmReadRequest * readReq);
  virtual void close();
  virtual double getLevelDownsample(int32_t level);
  virtual void getLevelDimensions(int32_t level, int64_t *width, int64_t *height);
  virtual int32_t getTotalLevels();
  virtual int32_t getQuickLevel();
  virtual int32_t getBestLevelForDownSample(double downSample);
  virtual QString getPropertyValue(const char * name);
  virtual ~vmGoogleMaps(); 
protected:
  vmNetworkAccessManager* mManager;
  QString mSlideUrlFormat;
  int64_t mWidth, mHeight;
  int32_t mLevels;
  double mPreviewWinWidth, mPreviewWinHeight;
  QString mDescription;
  QString mCopyright;
};


class vmGoogleMapsRequest : public QObject
{
protected:
  struct vmReadRequest* readReq;
public:
  vmGoogleMapsRequest(struct vmReadRequest *newReadReq) { readReq = newReadReq; }
  inline vmReadRequest* getReadRequest() { return readReq; }
  inline void clearReadRequest() { readReq = NULL; }
  void freeReadRequest();
};

#endif
