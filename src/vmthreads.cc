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
    
    for (int i = 0; i < totalReadReqs; i++)
    {
      vmReadRequest *readReq = readReqs[i];
      vmTile * tile = cacheFillRequest(readReq);
      if (tile)
      {
        vmUpdateRegion * updateRegion = tile->nextRegion;
        while (updateRegion)
        {
          cacheQueueRedraw(readReq, updateRegion);
          updateRegion = updateRegion->nextRegion;
        }
      }
    }
    
    slApp->readMutex.lock();
    for (int a = 0; a < totalReadReqs; a++)
    {
      vmReadRequest *readReq = readReqs[a];
      for (int b = 0; b < slApp->totalReadReqs; b++)
      {
        if (readReq == slApp->readReqs[b])
        {
          slApp->readReqs.remove(b);
          slApp->totalReadReqs--;
          readReq->sl->slideUnref();
          delete readReq;
          break;
        }
      }
    }
    totalReadReqs = slApp->totalReadReqs;
    slApp->readMutex.unlock();
    /*
    for (int i = 0; i < slApp->totalReadReqs; i++)
    {
      vmReadRequest *readReq = slApp->readReqs[i];
      if (readReq->filled || readReq->error)
      {
        slApp->readReqs.remove(i);
        slApp->totalReadReqs--;
        readReq->sl->slideUnref();
        delete readReq;
        break;
      }  
    }*/
    
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
}

