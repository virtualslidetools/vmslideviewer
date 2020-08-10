#include "vmtilereader.h"
#include <sstream>
#include <fstream>
#include "json/json.h"

void vmGoogleMapsRequest::freeReadRequest()
{
  if (readReq)
  {
    delete readReq;
    readReq = NULL;
  }
}


void vmNetworkAccessManager::replyFinished(QNetworkReply *reply)
{
  QNetworkRequest netReq = reply->request();
  vmGoogleMapsRequest * mapReq = (vmGoogleMapsRequest*) netReq.originatingObject();
  vmReadRequest * readReq = mapReq->getReadRequest();
  mapReq->clearReadRequest();
  if (readReq == NULL) 
  {
    qDebug() << "READREQ IS NULL!!!!!!!!!!!!!!!!!!!!";
    reply->deleteLater();
    return;
  }
  
  bool isReadable = reply->isReadable();
  qint64 dataSize = reply->bytesAvailable();
  QUrl url2 = reply->url();
  vmSlideCache *cache = readReq->sl->cache;
  double xMarker = readReq->xMarker;
  double yMarker = readReq->yMarker;
  vmTile *tile = readReq->tile;
  //double downSample = cacheGetLevelDownsample(cache, readReq->level);
  //int64_t tileX = floor(floor(xMarker / downSample) / (double) cache->tileWidth);
  //int64_t tileY = floor(floor(yMarker / downSample) / (double) cache->tileHeight);
  qDebug() << "reply url=" << url2.toString();
  qDebug() << "reply finished! readReq->xMarker=" << xMarker << " readReq->yMarker=" << yMarker << " bytesAvailable=" << dataSize << " level=" << readReq->level << " tileX=" << tile->x << " tileY=" << tile->y;
  qint64 tileSize = cache->tileWidth * cache->tileHeight * 4;
  QByteArray data;
  if (isReadable)
  {
    data = reply->readAll();
    dataSize = data.size();
  }
  bool processOk = false;
  char * tileData = NULL;
  if (isReadable && dataSize > 0)
  {
    QImage image;
    image.loadFromData(data, "JPG");
    QImage image2 = image.convertToFormat(QImage::Format_RGB32);
    int dataSize2 = image2.width() * image2.height() * 4;
    if (dataSize2 > tileSize)
    {
      dataSize2 = tileSize;
    }
    if (dataSize2 > 0)
    {
      processOk = true;
      tileData = new char[tileSize];
      memset(tileData, 255, tileSize);
      memcpy(tileData, image2.constBits(), dataSize2);
    }
  }  
  vmUpdateRegion * updateRegion = NULL;
  bool found = false;
  //int32_t topLevel = cacheGetQuickLevel(cache);
  bool alreadyFilled = false;

  cacheMutex.lock();
  if (readReq->sl->done)
  {
    cacheMutex.unlock();
    reply->deleteLater();
    return;
  }
  if (tile)
  {
    alreadyFilled = tile->filled;
    updateRegion = tile->nextRegion;
    tile->nextRegion = NULL;
    if (processOk == false)
    {
      tile->error = true;
    }
    else if (alreadyFilled == false)
    {
      tile->data = (uint32_t*) tileData;
      tile->filled = true;
    }
    found = true;
  }
  cacheMutex.unlock();

  if (found)
  {
    vmUpdateRegion * previousRegion = NULL;
    while (updateRegion)
    {
      previousRegion = updateRegion;
      if (processOk)
      {
        cacheQueueRedraw(readReq, updateRegion);
      }
      updateRegion = updateRegion->nextRegion;
      previousRegion->nextRegion = NULL;
      delete previousRegion;
    }
  }
  if (alreadyFilled == true && processOk && tileData)
  {
    delete[] tileData;
  }
  slApp->netMutex.lock();
  slApp->totalNetReqs--;
  slApp->netMutex.unlock();
  readReq->sl->slideUnref();
  delete readReq;
  reply->deleteLater();
}


vmGoogleMaps::vmGoogleMaps(const char* filename)
{
  std::ifstream infile;
  char* buffer = NULL;
  int length = 0;

  mWidth = 0;
  mHeight = 0;
  mLevels = 0;
  mSlideDepth = 0;
  mPreviewWinWidth = 0;
  mPreviewWinHeight = 0;
  mTileWidth = 256;
  mTileHeight = 256;
  mLastStatus = 0;

  memset(mDefaultBkgd, 255, 4);
  mManager = new vmNetworkAccessManager();   
  mManager->connect(mManager, SIGNAL(finished(QNetworkReply*)), mManager, SLOT(replyFinished(QNetworkReply*)));
  try 
  {
    infile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    infile.open(filename, std::ifstream::in);
    infile.seekg(0, infile.end);
    length = infile.tellg();
    infile.seekg(0, infile.beg);

    if (length > 4096) length = 4096;
    buffer = new char[length+1];
  
    infile.read(buffer, length);
    int gcount = infile.gcount();
    if (gcount < 0) gcount = 0;
    if (gcount > 4096) gcount = 4096;
    buffer[gcount]=0;
    infile.close();
  }
  catch (std::ifstream::failure & ex)
  {
    mErrorMsg = QString::fromStdString(ex.what());     
    mLastStatus = VM_READ_ERROR;   
    return;
  }
  if (buffer == NULL)
  {
    mErrorMsg = "Failed to allocate memory!";
    mLastStatus = VM_READ_ERROR;
    return;
  }
  Json::CharReaderBuilder builder;
  Json::Value root;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  std::string jsonParseErrs;
  
  if (!reader->parse(buffer, buffer+length, &root, &jsonParseErrs)) 
  {
    mLastStatus = VM_READ_ERROR;
    mErrorMsg = QString::fromStdString(jsonParseErrs);
  }
  bool convOk = true;
  if (root.isMember("slideUrlFormat"))
  {
    mSlideUrlFormat = QString::fromStdString(root["slideUrlFormat"].asString());
    QString httpTxt = "http://";
    QString httpsTxt = "https://";
    QString networkSlashTxt = "//";
    if (mSlideUrlFormat.isEmpty())
    {
      mErrorMsg.append("Field 'slideUrlFormat' is empty. \n");
    }
    if (mSlideUrlFormat.contains(httpTxt)==false && mSlideUrlFormat.contains(httpsTxt)==false)
    {
      if (mSlideUrlFormat.contains(networkSlashTxt))
      {
        mSlideUrlFormat = mSlideUrlFormat.prepend("http:");
      }
      else
      {
        mSlideUrlFormat = mSlideUrlFormat.prepend("http://");
      }
    }
  }
  else
  {
    mErrorMsg.append("Missing 'slideUrlFormat' member in Json file. \n");
  }
  if (root.isMember("width"))
  {
    QString widthTxt = QString::fromStdString(root["width"].asString());
    mWidth = widthTxt.toLongLong(&convOk);
    if (convOk == false)
    {
      mErrorMsg.append("Member 'width' not in integer format. width=");
      mErrorMsg.append(widthTxt);
      mErrorMsg.append(" \n");
    }
  }
  else
  {
    mErrorMsg.append("Missing 'width' member in Json file. \n");
  }
  if (root.isMember("height"))
  {
    QString heightTxt = QString::fromStdString(root["height"].asString());
    mHeight = heightTxt.toLongLong(&convOk);
    if (convOk == false)
    {
      mErrorMsg.append("Member 'height' not in integer format. height=");
      mErrorMsg.append(heightTxt);
      mErrorMsg.append(" \n");
    }
  }
  else
  {
    mErrorMsg.append("Missing 'height' member in Json file. \n");
  }
  if (root.isMember("levels"))
  {
    QString levelsTxt = QString::fromStdString(root["levels"].asString());
    mLevels = levelsTxt.toInt(&convOk)+1;
    if (convOk == false)
    {
      mErrorMsg.append("Member 'levels' not in integer format. levels=");
      mErrorMsg.append(levelsTxt);
      mErrorMsg.append(" \n");
    }
  } 
  else
  {
    mErrorMsg.append("Missing 'levels' member in Json file. \n");
  }
  if (root.isMember("slideDepth"))
  {
    QString slideDepthTxt = QString::fromStdString(root["slideDepth"].asString());
    mSlideDepth = slideDepthTxt.toDouble(&convOk);
    if (convOk == false)
    {
      mErrorMsg.append("Member 'slideDepth' not in integer or real format. slideDepth=");
      mErrorMsg.append(slideDepthTxt);
      mErrorMsg.append(" \n");
    }
  } 
  else
  {
    mErrorMsg.append("Missing 'slideDepth' member in Json file. \n");
  }
  if (root.isMember("previewWinWidth"))
  {
    QString previewWinWidthTxt = QString::fromStdString(root["previewWinWidth"].asString());
    mPreviewWinWidth = previewWinWidthTxt.toInt(&convOk);
  } 
  if (root.isMember("previewWinHeight"))
  {
    QString previewWinHeightTxt = QString::fromStdString(root["previewWinHeight"].asString());
    mPreviewWinHeight = previewWinHeightTxt.toInt(&convOk);
  } 
  if (root.isMember("copyright"))
  {
    mCopyright = QString::fromStdString(root["copyright"].asString());
  }
  if (mErrorMsg.size() > 0)
  {
    mLastStatus = VM_READ_ERROR;
  }
  mIsOnNetwork = true;
}


void vmGoogleMaps::close()
{
  if (mManager)
  {
    delete mManager;
    mManager = NULL;
  }
}


vmGoogleMaps::~vmGoogleMaps()
{
  close();
}


void vmGoogleMaps::read(uint32_t* data, int64_t x, int64_t y, int32_t level, int64_t width, int64_t height)
{
  int shift = (mLevels-1) - level;
  if (shift < 0) shift = 0;
  int startTileX = (x >> shift) / 256;
  int startTileY = (y >> shift) / 256;
  int endTileX = ceil((double)((x+width) >> shift) / 256);
  int endTileY = ceil((double)((y+height) >> shift) / 256);
  int endY = y + height;
  int endX = x + width;
  
  for (int tileY = startTileY; tileY <= endTileY; tileY++)
  {
    int yStart = 0;
    int y2 = tileY * 256;
    if (y2 < y) 
    {
      yStart = y - y2;
    }
    int yEnd = 256;
    if (tileY == endTileY)
    {
      yEnd = endY - y2;
    }
    for (int tileX = startTileX; tileX < endTileX; tileX++)
    {
      int xStart = 0;
      int x2 = tileX * 256;
      if (x2 < x) 
      {
        xStart = x - x2;
      }
      int xEnd = 256;
      if (tileX == endTileX)
      {
        xEnd = endX - x2;
      }
      QString localUrlTxt = mSlideUrlFormat;
      QString zStr = "{z}";
      QString yStr = "{y}";
      QString xStr = "{x}";
      QString zStrAfter = QString::number(level);
      QString yStrAfter = QString::number(tileY);
      QString xStrAfter = QString::number(tileX);
      localUrlTxt.replace(zStr, zStrAfter);
      localUrlTxt.replace(yStr, yStrAfter);
      localUrlTxt.replace(xStr, xStrAfter);
      safeBmp imgDest;

      safeBmpInit(&imgDest, data, width, height);
      
      QImage tile;
      if (tile.load(localUrlTxt)==false)
      {
        safeBmpUint32Set(&imgDest, 255);
        qDebug() << "File not found!" << localUrlTxt;
        return;
      }
      int tileHeight = tile.height();
      int tileWidth = tile.width();
      uint32_t* tileData = (uint32_t*) tile.bits();
      safeBmp imgSrc;
      safeBmpInit(&imgSrc, tileData, tileWidth, tileHeight);
      safeBmpCpy(&imgDest, xStart, yStart, &imgSrc, xStart, yStart, xEnd - xStart, yEnd - yStart);
    }
  }
}


void vmGoogleMaps::networkRead(vmReadRequest* readReq)
{
  int64_t x = readReq->xMarker;
  int64_t y = readReq->yMarker;
  int32_t level = (mLevels-1) - readReq->level;

  int shift = readReq->level;
  if (shift < 0) shift = 0;
  int tileX = (x >> shift) / 256;
  int tileY = (y >> shift) / 256;
  QString urlTxt = mSlideUrlFormat;
  QString zStr = "{z}";
  QString yStr = "{y}";
  QString xStr = "{x}";
  QString zStrAfter = QString::number(level);
  QString yStrAfter = QString::number(tileY);
  QString xStrAfter = QString::number(tileX);
  urlTxt.replace(zStr, zStrAfter);
  urlTxt.replace(yStr, yStrAfter);
  urlTxt.replace(xStr, xStrAfter);
  QUrl url(urlTxt);
  QNetworkRequest req(url);
  qDebug() << "fetching... " << urlTxt;
  vmGoogleMapsRequest * mapsReq = new vmGoogleMapsRequest(readReq);
  req.setOriginatingObject(mapsReq);
  qDebug() << "networkRead: readReq=" << readReq << " mapsReq=" << mapsReq;
  mManager->get(req);  
}


double vmGoogleMaps::getLevelDownsample(int32_t level)
{
  return (double) (1 << level); 
}


void vmGoogleMaps::getLevelDimensions(int32_t level, int64_t *newWidth, int64_t *newHeight)
{
  *newWidth = 256 << ((mLevels-1) - level);
  *newHeight = 256 << ((mLevels-1) - level);
}


int32_t vmGoogleMaps::getTotalLevels()
{
  return mLevels; 
}


int32_t vmGoogleMaps::getQuickLevel()
{
  return mLevels - 4;
}


int32_t vmGoogleMaps::getBestLevelForDownSample(double downSample)
{
  if (downSample >= (double) (1 << (mLevels-1))) return (mLevels-1);
  if (downSample <= 1.0) return 0;
  int level = mLevels;
  while (level > 0)
  {
    int upper = 1 << level;
    level--;
    int lower = 1 << level;
    if ((double) upper > (double) downSample && (double) lower <= (double) downSample)
    {
      return level;
    }
  }
  return mLevels - 1;
}


QString vmGoogleMaps::getPropertyValue(const char * name)
{
  Q_UNUSED(name);
  QString value="";
  return value;
}

