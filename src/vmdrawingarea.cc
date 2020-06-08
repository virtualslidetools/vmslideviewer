#include <QtMath>
#include <cmath>
#include "vmslideviewer.h"

void vmDrawingArea::drawForeground(QPainter *painter, const QRectF &destRect)
{
  vmReadRequest readReq;
  vmReadRequest* readReq2 = NULL;
  safeBmp bmpDest;

  safeBmpClear(&bmpDest);
  int x= qRound(destRect.x());
  int y= qRound(destRect.y());
  int width= qCeil(destRect.width());
  int height= qCeil(destRect.height());
  int xDest = qRound(destRect.x());
  int yDest = qRound(destRect.y());

  if (sl->cache != NULL && sl->totalLevels > sl->level)
  {
    safeBmp bmp, bmp2;
    uint32_t *bmp_ptr = NULL;
    uint32_t *bmp_ptr2 = NULL;
    double xRead = floor((double) x * sl->downSample); // - 1
    double yRead = floor((double) y * sl->downSample); // - 1
    double grabWidth = ceil((double) width * sl->xScale); // + 2
    double grabHeight = ceil((double) height * sl->yScale); // + 2
    int32_t topLevel=cacheGetLevelCount(sl->cache) - 1;
    double downSample2 = sl->pyramidLevels[topLevel].downSample;
    double xScale2 = sl->downSample / downSample2;
    double yScale2 = xScale2;
    double grabWidth2 = ceil((double) width * xScale2);// + 2;  // previous ceil + 1
    double grabHeight2 = ceil((double) height * yScale2);// + 2;
    safeBmpAlloc2(&bmp, grabWidth, grabHeight);
    safeBmpAlloc2(&bmp2, grabWidth2, grabHeight2);
    bmp_ptr = bmp.data;
    bmp_ptr2 = bmp2.data;
    readReq.sl=sl;
    readReq.imgDest = &bmp2;
    readReq.drawingWidget = this;
    readReq.zoomArea = NULL;
    readReq.xRead = xRead;
    readReq.yRead = yRead;
    readReq.width = grabWidth2;
    readReq.height = grabHeight2;
    readReq.level = topLevel;
    readReq.xStart = x * sl->downSample;
    readReq.yStart = y * sl->downSample;
    readReq.downSample = downSample2;
    readReq.diskLoad = true;
    readReq.isZoom = false;
    readReq.xDest = x;
    readReq.yDest = y;
    qDebug() << "in vmdrawingarea.cc:paintevent: L2 Region repainting: x=" << x << " y=" << y << " width=" << round(width) << " height=" << round(height) << " dest x=" << xDest << " dest y=" << yDest << "\n";
    if (bmp.data && bmp2.data)
    {
      cacheReadRegion(&readReq);
      readReq.imgDest = &bmp;
      readReq.width = grabWidth;
      readReq.height = grabHeight;
      readReq.level = sl->level;
      readReq.downSample = sl->downSample;
      readReq.diskLoad = false;
      qDebug() << "in vmdrawingarea.cc:paintevent: Normal Region repainting: x=" << x << " y=" << y << " width=" << width << " height=" << height << " xRead=" <<  xRead << " yRead=" << yRead << " grabWidth=" << grabWidth << " grabHeight=" << grabHeight << " sl->downSample=" << sl->downSample << " sl->xScale=" << sl->xScale << " sl->yScale=" << sl->yScale << " xDest=" << xDest << " yDest=" << yDest << "\n";
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
        scaledImg2 = img2.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        uint32_t* data = (uint32_t*) scaledImg.bits();
        uint32_t* data2 = (uint32_t*) scaledImg2.bits();
        safeBmpInit(&bmp, data, width, height);
        safeBmpInit(&bmp2, data2, width, height);
        safeBmpAlloc2(&bmpDest, width, height);
        safeBmpUint32Set(&bmpDest, sl->cache->backgroundRgba);
        readReq.imgDest = &bmpDest;
        cacheCopyRequests(&readReq, readReq2, &bmp, &bmp2);
        img3 = QImage((uchar*)bmpDest.data, width, height, width*4, QImage::Format_RGB32);
        targetImg = &img3;
      }
      //painter->drawImage(xDest, yDest, *targetImg, 0, 0, -1, -1, Qt::DiffuseDither);
      painter->drawImage(destRect, *targetImg);
      //painter->drawImage(xDest, yDest, *targetImg);

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
    if (readReq2)
    {
      slApp->readMutex.lock();
      slApp->readReqs.append(readReq2);
      slApp->totalReadReqs++;
      slApp->readMutex.unlock();
      slApp->readCondMutex.lock();
      slApp->readCond.wakeOne();
      slApp->readCondMutex.unlock();
    } 
  }
}

/*
void vmDrawingArea::drawBackground(QPainter *painter, const QRectF& destRect)
{
  Q_UNUSED(painter);
  Q_UNUSED(destRect);
  qDebug() << "DRAW BACKGROUND CALLED!";
  //if (sl == NULL)
  {
    //qDebug() << " NO SLIDE DATA! ERASING WITH DEFAULT BRUSH! ";
    QGraphicsView::drawBackground(painter, destRect);
  }
  //setBackgroundBrush(Qt::NoBrush);
  //draw(painter, destRect, destRect);
  //drawForeground(painter, rect);
}
*/


void vmDrawingArea::updateDrawingRectSlot(int x, int y, int width, int height)
{
  /*
  Q_UNUSED(x);
  Q_UNUSED(y);
  Q_UNUSED(width);
  Q_UNUSED(height);
  */
  qDebug() << " background update: x=" << x << " y=" << y << " width=" << width << " height=" << height;
  QRectF rect(x, y, width, height);
  //invalidateScene(rect, QGraphicsScene::ForegroundLayer);
  //update(x, y, x+width, y+height);
  QList<QRectF> all;
  all.append(rect);
  updateScene(all);
}


void vmDrawingArea::resizeEvent(QResizeEvent *event)
{
  QWidget::resizeEvent(event);
  QSize size = event->size();
  int cx = size.width();
  int cy = size.height();
  qDebug() << " window width=" << cx << " window height=" << cy;
  if (cx < 40)
  {
    cx = 40;
  }
  if (cy < 40) 
  {
    cy = 40;
  }
  if (sl)
  {
    sl->windowWidth = cx;
    sl->windowHeight = cy;
    qreal xMax = round(sl->baseWidth / sl->downSample) + 2 * (width() - 40);
    qreal yMax = round(sl->baseHeight / sl->downSample) + 2 * (height() - 40);
    QRectF rect(-width()+40, -height()+40, xMax, yMax);
    qreal xMin2 = round(sl->xStart / sl->downSample);
    qreal yMin2 = round(sl->yStart / sl->downSample);
    QRectF rect2(xMin2, yMin2, width(), height());
    setSceneRect(rect);
    ensureVisible(rect2, 0, 0);
    invalidateScene(rect);
  }
  else
  {
    setSceneRect(0, 0, cx, cy);
  }
  qDebug() << " updateZoomPos: width=" << cx << " height=" << cy;
  if (zoomArea)
  {
    cx -= (slApp->zoomWindowWidth + slApp->zoomGap);
    if (cx < 0) cx=100;
    cy -= (slApp->zoomWindowHeight + slApp->zoomGap);
    if (cy < 0) cy=100;
    zoomArea->move(cx, cy);
  }
}


void vmDrawingArea::horizScrollChanged(int value)
{
  //QScrollBar* horizScroll = horizontalScrollBar();
  sl->xStart = round((double) value * sl->downSample);
}


void vmDrawingArea::vertScrollChanged(int value)
{
  //QScrollBar* vertScroll = verticalScrollBar();
  sl->yStart = round((double) value * sl->downSample);
}

void vmDrawingArea::createZoomAreaWindow()
{
  if (zoomArea == NULL)
  {
    zoomArea = new vmZoomArea(sl);
  }
}

void vmDrawingArea::createRangeTool()
{
  if (rangeTool == NULL)
  {
    rangeTool = new vmRangeTool(sl);
    rangeTool->setLevel(sl->leverPower);
  }
}


void vmDrawingArea::destroyZoomAreaWindow()
{
  if (zoomArea)
  {
    delete zoomArea;
    zoomArea = NULL;
  }
}

void vmDrawingArea::destroyRangeTool()
{
  if (rangeTool)
  {
    delete rangeTool;
    rangeTool = NULL;
  }
}

vmDrawingArea::vmDrawingArea(vmSlide *slNew, QWidget *parent, QGraphicsScene* scene) : QGraphicsView(scene, parent)
{
  sl = slNew;
  sl->drawingArea = this;
  mainWindow = sl->parentWindow;
  sl->showZoom = mainWindow->getShowZoom();
  setupViewport(parent);
  setScene(scene);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setInteractive(true);
  setDragMode(QGraphicsView::NoDrag);
  //setDragMode(QGraphicsView::ScrollHandDrag);
  //setBackgroundBrush(Qt::NoBrush);
  //setAutoFillBackground(false);
  //setAttribute(Qt::WA_OpaquePaintEvent);
  //setAttribute(Qt::WA_NoSystemBackground);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setCacheMode(QGraphicsView::CacheNone);
  setFocusPolicy(Qt::StrongFocus);
  QScrollBar* horizScroll = horizontalScrollBar();
  QScrollBar* vertScroll = verticalScrollBar();
  connect(horizScroll, SIGNAL(valueChanged(int)), this, SLOT(horizScrollChanged(int)));
  connect(vertScroll, SIGNAL(valueChanged(int)), this, SLOT(vertScrollChanged(int)));
  connect(this, SIGNAL(requestUpdate(int,int,int,int)), this, SLOT(updateDrawingRectSlot(int,int,int,int)));
  zoomArea = NULL;
  rangeTool = NULL;
  setMouseTracking(true);
  show();
  setFocus();
  if (mainWindow->getShowZoom())
  {
    createZoomAreaWindow();
  }
  createRangeTool();
  if (mainWindow->getShowRange())
  {
    createRangeTool();
  }
}


vmDrawingArea::~vmDrawingArea()
{
  if (sl)
  {
    qDebug() << "CLOSING SLIDE DATA!!!!";
    sl->slideMarkClose();
  }
}


void vmDrawingArea::wheelEvent(QWheelEvent *event)
{
  int numDegrees = event->delta() / 8;
  int numSteps = numDegrees / 15;

  if (event->orientation() == Qt::Vertical)
  {
    qDebug() << " scrolled " << numSteps << " steps.";
    sl->calcScroll(numSteps);
  }
  event->accept();
}


void vmDrawingArea::rangeToolClick(double leverPower)
{
  double currentPower = vmFromLeverPower(leverPower, sl->leverBuildPower);
  if (sl->leverPower != leverPower)
  {
    sl->leverPower = leverPower;
    sl->scaleChanged(sl->leverPower);
    sl->calcScale(currentPower);
  }
}


void vmDrawingArea::mousePressEvent(QMouseEvent *event)
{
  sl->mouseDown = true; 
  xMoveStart = (event->x() * sl->downSample); 
  yMoveStart = (event->y() * sl->downSample);
  setCursor(Qt::ClosedHandCursor);
  qDebug() << " mouse pressed x=" << xMoveStart << " y=" << yMoveStart;
  event->accept();
}


void vmDrawingArea::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event);

  sl->mouseDown = false;
  setCursor(Qt::OpenHandCursor);
  event->accept();
}


void vmDrawingArea::mouseMoveEvent(QMouseEvent *event)
{
  QWidget * view = viewport();
  if (sl==NULL || sl->cache==NULL) return;
  //slApp->currentWindowNumber = sl->parentWindow->windowNumber;
  double x = round(event->x() * sl->downSample);
  double y = round(event->y() * sl->downSample);
  QScrollBar* horizScroll = horizontalScrollBar();
  QScrollBar* vertScroll = verticalScrollBar();
 
  qDebug() << " horizScroll->value()=" << horizScroll->value() << " vertScroll->value()=" << vertScroll->value();
  double xStart = (double) horizScroll->value() * sl->downSample;
  double yStart = (double) vertScroll->value() * sl->downSample;

  qDebug() << " view width=" << view->width() << " this width=" << width();

  if (sl->mouseDown)
  {
    setCursor(Qt::ClosedHandCursor);
    qDebug() << " mouse diff x=" << round((sl->xMoveStart - x) / sl->downSample) << " diff y=" << round((sl->yMoveStart -y) / sl->downSample);
    double xDiff = xMoveStart - x;
    double yDiff = yMoveStart - y;
    double xStart2 = xStart + xDiff;
    double yStart2 = yStart + yDiff;
    double xStart3 = round(xStart2 / sl->downSample);
    double yStart3 = round(yStart2 / sl->downSample);

    double xMinPos = horizScroll->minimum();
    double xMaxPos = horizScroll->maximum();
    if (xStart3 > xMaxPos)
    {
      //xStart2 = round(xMaxPos * sl->downSample);
    }
    else if (xStart3 < xMinPos)
    {
      //xStart2 = round(xMinPos * sl->downSample);
    }

    double yMinPos = vertScroll->minimum();
    double yMaxPos = vertScroll->maximum();
    if (yStart3 > yMaxPos)
    {
      //yStart2 = round(yMaxPos * sl->downSample);
    }
    else if (yStart3 < yMinPos)
    {
      //yStart2 = round(yMinPos * sl->downSample);
    }
    
    qDebug() << " preMouse yStart=" << yStart2 << " xStart=" << xStart2;
    qreal xPixelStart = (qreal) round(xStart2 / sl->downSample);
    qreal yPixelStart = (qreal) round(yStart2 / sl->downSample);
    QRectF rect(xPixelStart, yPixelStart, view->width(), view->height());
    
    ensureVisible(rect, 0, 0);

    xMoveStart = x; 
    yMoveStart = y;
  }
  else if (sl->zoomArea)
  {
    sl->zoomArea->update();
  }
  sl->xMouse = sl->xStart + x;
  sl->yMouse = sl->yStart + y;

  qDebug() << " mouse move xStart=" << sl->xStart << " yStart=" << sl->yStart << " raw x=" << event->x() << " raw y=" << event->y() << " downsample=" << sl->downSample;
  qDebug() << " mouse move openslide x=" << sl->xMouse << " y=" << sl->yMouse;
  event->accept();
}


void vmDrawingArea::keyPressEvent(QKeyEvent *k)
{
  switch (k->key())
  {
    case Qt::Key_Up:
      qDebug() << "Key_Up pressed.";
      sl->calcScroll(-1);
      break;
    case Qt::Key_Down:
      qDebug() << "Key_Down pressed.";
      sl->calcScroll(1);
      break;
    case Qt::Key_C:
      qDebug() << "Releasing cache...";
      cacheMutex.lock();
      cacheReleaseAll();
      cacheMutex.unlock();
      qDebug() << "Done releasing cache.";
      break;
  }
}

