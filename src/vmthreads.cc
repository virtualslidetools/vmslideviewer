#include <cmath>
#include "vmslideviewer.h"

void vmReadThread::run()
{
  QVector<vmReadRequest*> readReqs;
  int totalReadReqs;

  slApp->readMutex.lock();
  totalReadReqs = slApp->totalReadReqs;
  slApp->readMutex.unlock();

  while (slApp->keepRunning || totalReadReqs > 0)
  {
    slApp->readMutex.lock();
    totalReadReqs = slApp->totalReadReqs;
    for (int i = 0; i < totalReadReqs; i++)
    {
      readReqs.append(slApp->readReqs[i]);
    }
    slApp->readMutex.unlock();
    qDebug() << "Filling requests... total " << totalReadReqs << "\n";
    for (int i = 0; i < totalReadReqs; i++)
    {
      vmReadRequest *readReq = readReqs[i];
      if (cacheFillRequests(readReq))
      {
        vmReadRequest *nextReq=readReq;
        while (nextReq)
        {
          readReq = nextReq;
          nextReq = readReq->tail;
    //      qDebug() << "Queueing redraw...\n";
          int x=readReq->xDest;// + (readReq->sl->xStart - readReq->xStart);
          int y=readReq->yDest;// + (readReq->sl->yStart - readReq->yStart);
          double downSampleLevel = readReq->sl->pyramidLevels[readReq->level].downSample;
          qDebug() << "in vmthreads.cc:run original width=" << readReq->width << " original height=" << readReq->height << " downsample: " << readReq->downSample << " level downsample: " << downSampleLevel;
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
      }
    }
    
    slApp->readMutex.lock();
    for (int a = 0; a < totalReadReqs; a++)
    {
      vmReadRequest *readReq = readReqs[a];
      for (int b = 0; b < slApp->totalReadReqs; b++)
      {
        if (slApp->readReqs[b]==readReq)
        {
          slApp->readReqs.remove(b);
          slApp->totalReadReqs--;
          vmReadRequest *nextReq=readReq;
          while (nextReq)
          {
            readReq = nextReq;
            nextReq = readReq->tail;
            readReq->sl->slideUnref();
            delete readReq;
          }
          break;
        }
      }  
    }
    totalReadReqs = slApp->totalReadReqs;
    slApp->readMutex.unlock();
    
    readReqs.clear();

    qDebug() << "Done cleaning up.\n";

    if (totalReadReqs==0 && slApp->keepRunning)
    {
      slApp->readCondMutex.lock();
      slApp->readCond.wait(&slApp->readCondMutex);
      slApp->readCondMutex.unlock();
    }
    slApp->readMutex.lock();
    totalReadReqs = slApp->totalReadReqs;
    slApp->readMutex.unlock();
  }
  //finished();
  //gArrayFree(slApp->sls, TRUE);
  //cacheReleaseAll();
}


