#include <QtMath>
#include <cmath>
#include "vmslideviewer.h"

void vmZoomArea::paintEvent(QPaintEvent* event)
{
  vmReadRequest readReq;
  vmReadRequest* readReq2 = NULL;
  
  if (sl == NULL) return;
  vmSlideCache * cache = sl->cache;
  safeBmp bmpDest;

  safeBmpClear(&bmpDest);
  QPainter painter(this);

  QRect destRect = event->rect();

  int x=(int) destRect.x();
  int y=(int) destRect.y();
  int width=(int) destRect.width();
  int height=(int) destRect.height();
  
  if (cache != NULL && sl->totalLevels > sl->level)
  {
    safeBmp bmp, bmp2;
    uint32_t *bmp_ptr = NULL;
    uint32_t *bmp_ptr2 = NULL;
    double grabWidth = ceil((double) width * sl->xZoomScale);
    double grabHeight = ceil((double) height * sl->yZoomScale);
    double xRead = round(sl->xMouse + (((double) x - (double) slApp->zoomWindowWidth / 16.0) * sl->downSample)); 
    double yRead = round(sl->yMouse + (double) y * sl->downSample);
    int32_t topLevel = cacheGetQuickLevel(cache);
    double downSample2 = cacheGetLevelDownsample(cache, topLevel);
    double xScale2 = sl->downSampleZ / downSample2; 
    double yScale2 = xScale2;
    double grabWidth2 = ceil((double) width * xScale2);
    double grabHeight2 = ceil((double) height * yScale2);
    safeBmpAlloc2(&bmp, (int) grabWidth, (int) grabHeight);
    safeBmpAlloc2(&bmp2, (int) grabWidth2, (int) grabHeight2);
    bmp_ptr = bmp.data;
    bmp_ptr2 = bmp2.data;
    memset(&readReq, 0, sizeof(readReq));
    readReq.sl=sl;
    readReq.imgDest = &bmp2;
    readReq.drawingWidget = NULL;
    readReq.zoomArea = this;
    readReq.xRead = xRead;
    readReq.yRead = yRead;
    readReq.width = grabWidth2;
    readReq.height = grabHeight2;
    readReq.level = topLevel;
    readReq.xStart = x * sl->downSample; //sl->xStart; 
    readReq.yStart = y * sl->downSample; //sl->yStart;
    readReq.downSample = downSample2;
    readReq.diskLoad = true;
    readReq.isZoom = true;
    readReq.xDest = x;
    readReq.yDest = y;
    readReq.isOnNetwork = sl->getIsOnNetwork();
    if (bmp.data && bmp2.data)
    {
      cacheReadRegion(&readReq);
      readReq.level = sl->zoomLevel;
      readReq.imgDest = &bmp;
      readReq.width = grabWidth;
      readReq.height = grabHeight;
      readReq.downSample = sl->downSampleZ;
      readReq.diskLoad = false;
      qDebug() << "in vmzoomarea.cc: Normal zoom region repainting: x=" << x << " y=" << y << " width=" << (int) width << " height=" << (int) height << " xRead=" << xRead << " yRead=" << yRead << " grabWidth=" << grabWidth << " grabHeight=" << grabHeight << " sl->downSampleZ=" << sl->downSampleZ << " sl->xZoomScale=" << sl->xZoomScale << " sl->yZoomScale=" << sl->yZoomScale << " sl->downSample=" << sl->downSample << " xDest=" << x << " yDest=" << y << "\n";
      readReq2 = cacheReadRegion(&readReq);
      QImage img = QImage((uchar*)bmp.data, (int) grabWidth, (int) grabHeight, ((int) grabWidth)*4, QImage::Format_RGB32);
      QImage scaledImg;
      QImage img2 = QImage((uchar*)bmp2.data, (int) grabWidth2, (int) grabHeight2, ((int) grabWidth2)*4, QImage::Format_RGB32);
      QImage scaledImg2;
      QImage *targetImg = &img2;
      QImage img3;
      
      if (img.constBits() && img2.constBits())
      {
        scaledImg = img.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        // targetImg = &scaledImg;
        scaledImg2 = img2.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        // targetImg = &scaledImg2;
        uint32_t* data = (uint32_t*) scaledImg.bits();
        uint32_t* data2 = (uint32_t*) scaledImg2.bits();
        safeBmpInit(&bmp, data, width, height);
        safeBmpInit(&bmp2, data2, width, height);
        safeBmpAlloc2(&bmpDest, width, height);
        safeBmpUint32Set(&bmpDest, sl->cache->backgroundRgba);
        readReq.imgDest = &bmpDest;
        cacheCopyRequests(&readReq, readReq2, &bmp, &bmp2);
        img3 = QImage((uchar*)bmpDest.data, width, height, width*4, QImage::Format_ARGB32);
        targetImg = &img3;
      }
      painter.drawImage(destRect, *targetImg);
      //painter.drawImage(x, y, *targetImg, 0, 0, -1, -1, Qt::DiffuseDither);
      if (bmp_ptr)
      {
        delete[] bmp_ptr;
        bmp_ptr = NULL;
      }
      if (bmp_ptr2)
      {
        delete[] bmp_ptr2;
        bmp_ptr2 = NULL;
      }
      safeBmpFree(&bmpDest);
    }
  }
  x=(int) destRect.x();
  y=(int) destRect.y();
  width=(int) destRect.width();
  height=(int) destRect.height();
 
  QPen blackPen(Qt::black);

  painter.setPen(blackPen);
  if (x <= 0)
  {
    painter.drawLine(0, 0, 0, y+height);
  }
  if (y <= 0)
  {
    painter.drawLine(0, 0, x+width, 0);
  }
  if (x+width >= slApp->zoomWindowWidth-1)
  {
    painter.drawLine(slApp->zoomWindowWidth-1, 0, slApp->zoomWindowWidth-1, y+height);
  }
  if (y+height >= slApp->zoomWindowHeight-1)
  {
    painter.drawLine(0, slApp->zoomWindowHeight-1, x+width, slApp->zoomWindowHeight-1);
  }
}


void vmZoomArea::updateZoomRectSlot(int x, int y, int width, int height)
{
  Q_UNUSED(x);
  Q_UNUSED(y);
  Q_UNUSED(width);
  Q_UNUSED(height);
  
  update();
}


vmZoomArea::vmZoomArea(vmSlide *slNew) : QWidget(slNew->drawingArea) 
{ 
  sl = slNew;
  sl->zoomArea = this;
  setUpdatesEnabled(true);
  //setAttribute(Qt::WA_OpaquePaintEvent);
  //setAttribute(Qt::WA_NoSystemBackground);
  connect(this, SIGNAL(requestZoomUpdate(int,int,int,int)), this, SLOT(updateZoomRectSlot(int,int,int,int)));
  int cx = sl->drawingArea->width();
  int cy = sl->drawingArea->height();
  qDebug() << " parent width=" << cx << " parent height=" << cy;
  cx -= (slApp->zoomWindowWidth + slApp->zoomGap);
  if (cx < 0) cx = 100;
  cy -= (slApp->zoomWindowHeight + slApp->zoomGap);
  if (cy < 0) cy = 100;
  setGeometry(cx, cy, slApp->zoomWindowWidth, slApp->zoomWindowHeight);
  show();
}

