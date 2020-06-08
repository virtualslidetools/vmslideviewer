#include <QtGlobal>
#include <QApplication>
#include <QMessageBox>
#include <QScrollBar>
#include <QVector>
#include <QStyle>
#include <QMutex>
#include <QtDebug>
#include <iostream>
#include <fstream>
#include <cmath>
#include "vmslideviewer.h"

vmSlideApp *slApp;
uint8_t vmSlideApp::backgroundRgba[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

double vmToLeverPower(double power, double minPower, double maxLeverPower) 
{
  double levelFloor = minPower / 2;
  double levelCeil = minPower;
  int count=-1;
  while (levelCeil < power)
  {
    levelFloor = levelCeil;
    levelCeil *= 2;
    count++;
  }
  double perc = (power - levelFloor) / (levelCeil - levelFloor);
  double value = (double) count + perc;
  if (value < -1.0) value = -1.0;
  if (value > maxLeverPower) value = maxLeverPower;
  return value;
}


double vmFromLeverPower(double scaleLever, double minPower)
{
  double levelFloor = floor(scaleLever);
  double powerFloor = minPower * pow(2.0, levelFloor);
  double powerCeil = powerFloor * 2;
  
  double offset = (scaleLever - levelFloor) *  (powerCeil - powerFloor);
  //gPrintf("scaleLever=%f minPower=%f converted=%f\n", scaleLever, minPower, powerFloor+offset);
  return powerFloor + offset;
  //return ((double) minPower * pow(2.0, (double) scalePower))
}


int vmSlideApp::addSlide(vmSlide *sl)
{
  sls.append(sl);
  return ++totalSlides;
}


void vmSlideApp::removeSlide(int slideNumber)
{
  if (slideNumber > 0)
  {
    sls.remove(slideNumber-1);
    totalSlides--;
  }
}


vmSlideWindow* vmSlideApp::createMainWindow()
{
  vmSlideWindow* window = new vmSlideWindow();
  parentWindows.append(window);
  window->setWindowNumber(totalWindows++);
  totalActiveWindows++;
  return window;
}


vmSlide* vmSlideApp::addSlide(QString& filename, bool newWindow, vmSlideWindow *window)
{
  vmSlide *sl = new vmSlide();
  bool openSuccess=false;
  if (sl)
  {
    sl->setParentWindow(window);
    openSuccess = sl->openslide(filename);
  }
  if (openSuccess)
  {
    sl->setSlideNumber(slApp->addSlide(sl));
    if (!window || newWindow || totalActiveWindows==0)
		{
      window = createMainWindow();
      sl->setParentWindow(window);
    }
		window->addSlidePage(sl);
    //setDefaultCursor();
    QString title = "vmslideviewer ";
    title.append(filename);
    window->setWindowTitle(title);
    window->show();
    //createZoomWindow(sl);
    //createZoomRange(sl);
    //gtkWidgetSetSensitive(sl->parentWindow->infoMenuItem, TRUE);
  }
  else if (sl)
  {
    delete sl;
    sl=NULL;
  }
  return sl;
}


void vmSlide::findMagnification()
{
  //int downSample = round(sl->downSample * 10);
  bool set=false;
  /*
  for (int32T level=0; level < sl->totalLevels; level++)
  {
    double currentSample=gArrayIndex(sl->pyramidLevels, slideLevelT, level).downSample;
    if (downSample==round(currentSample*10))
    {
      sl->downSample = currentSample;
      sl->downSampleZ = currentSample / 4;
      sl->level = level;
      set = true;
      sl->doScale = false;
      break;
    }
  }*/
  if (set==false)
  {
    level = cacheGetBestLevelForDownsample(cache, downSample);
    doScale = true;
  }
}


void vmSlide::setMagnification(bool getBest)
{
  doZoomScale = true;
  if (getBest)
  {
    findMagnification();
  }
  zoomLevel = cacheGetBestLevelForDownsample(cache, downSampleZ);
  if (doScale==true)
  {
    xScale = downSample / pyramidLevels[level].downSample;
    yScale = xScale;
  }
  xZoomScale = downSampleZ / pyramidLevels[zoomLevel].downSample;
  yZoomScale = xZoomScale;
  xMouseScale = downSample / downSampleZ;
  yMouseScale = downSample / downSampleZ;
  if (drawingArea) 
  {
    qDebug() << "ENTIRE WIDGET REDRAW SET!";
    qreal xMax = round(baseWidth / downSample) + 2 * (drawingArea->width() - 40);
    qreal yMax = round(baseHeight / downSample) + 2 * (drawingArea->height() - 40);
    QRectF rect(-drawingArea->width()+40, -drawingArea->height()+40, xMax, yMax);

    qreal xMin = round(xStart / downSample);
    qreal yMin = round(yStart / downSample);
    QRectF rect2(xMin, yMin, drawingArea->width(), drawingArea->height());
    drawingArea->setSceneRect(rect);
    drawingArea->ensureVisible(rect2, 0, 0);
    drawingArea->invalidateScene(rect2);
  }
}


bool vmSlide::openslide(QString& _filename)
{
  QString slideError; 
  const char * c_filename;
  const char * c_maxPowerTxt;
  QByteArray ba;
  filename = _filename;
  if (filename.isNull()) return false;
 
  ba = filename.toUtf8();
  c_filename = ba.data();
  qDebug() << "Opening file: " << c_filename << "\n";
  QFileInfo fileinfo(filename);
  QString ext = fileinfo.suffix();
  QString extUpper = ext.toUpper();

  int32_t fileType = VM_OPENSLIDE_TYPE;
  vmSlideCache * newCache = NULL;
  if (fileinfo.isDir() == false && fileinfo.isReadable() == false)
  {
    slideError = "Do not have read permission.";
  }
  else
  {
    if (fileinfo.isDir() || extUpper == "INI")
    {
      fileType = VM_OLYMPUS_TYPE;
    }
    qDebug() << "Opening file: " << c_filename << "\n";
    newCache = cacheOpen(c_filename, fileType, slideError, vmSlideApp::backgroundRgba);
  }
  if (newCache && slideError.isEmpty())
  {
    cache = newCache;
    xStart = 0;
    yStart = 0;
    totalLevels = cacheGetLevelCount(cache);
    
    maxPowerTxt=cacheGetPropertyValue(newCache, OPENSLIDE_PROPERTY_NAME_OBJECTIVE_POWER);
    maxPower = 0.0;
    if (!maxPowerTxt.isEmpty())
    {
      QByteArray ba2=maxPowerTxt.toUtf8();
      c_maxPowerTxt = ba2.data(); 
      maxPower = atof(c_maxPowerTxt);
    }
    if (maxPower == 0.0) 
    {
      maxPowerTxt = "";
      maxPower = 40.0;
    }
 
    pyramidLevels.clear();
    double downSample2 = 1.0;
    double power = 1.0;
    minDownSample = 1.0;
    maxDownSample = 1.0;
    minPower = 1.0;
    for (int32_t level = 0; level < totalLevels; level++)
    {
      vmSlideLevel levelData;
      downSample2 = cacheGetLevelDownsample(newCache, level);
      levelData.downSample=downSample2;
      power = maxPower / downSample2;
      levelData.power = power;
      if (level==0)
      {
        minDownSample = downSample;
        maxDownSample = downSample;
        maxPower = power;
        minPower = power;
      }
      if (downSample2 < minDownSample) minDownSample = downSample2;
      if (downSample2 > maxDownSample) maxDownSample = downSample2;
      if (power < minPower)  minPower = power;
      if (power > maxPower)  maxPower = power;
      cacheGetLevelDimensions(newCache, level, &levelData.width, &levelData.height);
      qDebug() << "Level " << level << " Down Sample: " << downSample2 << " Power: " << power << "\n";
      qDebug() << "Width: " << (int) levelData.width << " Height: " << (int) levelData.height << "\n";
      if (level==0)
      {
        baseWidth = levelData.width;
        baseHeight = levelData.height;
      }
      pyramidLevels.append(levelData);
    }
    currentPower = minPower;
    leverPower = 0.0;
    power = maxPower;
    double totalPartitions = 0.0;
    for (; power >= 1.0; power /= 2) 
    {
      totalPartitions++;
    }
    leverBuildPower = power;
    leverPower = vmToLeverPower(currentPower, leverBuildPower, totalPartitions);
    maxLeverPower = totalPartitions;
    minLeverPower = -2.0;

    downSample = maxDownSample;
    downSampleZ = downSample / 4;
    setMagnification(true);
    
    vendor=cacheGetPropertyValue(newCache, OPENSLIDE_PROPERTY_NAME_VENDOR);
    if (vendor.isNull()) vendor="";
    comment=cacheGetPropertyValue(newCache, OPENSLIDE_PROPERTY_NAME_COMMENT); 
    if (comment.isNull()) comment="";
    baseMppX=cacheGetPropertyValue(newCache, OPENSLIDE_PROPERTY_NAME_MPP_X);
    if (baseMppX.isNull()) baseMppX="";
    baseMppY=cacheGetPropertyValue(newCache, OPENSLIDE_PROPERTY_NAME_MPP_Y);
    if (baseMppY.isNull()) baseMppY="";
  }
  else
  {
    QString error;
    if (slideError.isEmpty())
    {
      error = "";
    }
    else
    {
      error = ": ";
      error.append(slideError);
    }

    QMessageBox msgBox;
    QString msg;
    msg = "Cannot open slide file '";
    msg += filename;

    if (error.length()>0)
    {
      msg += error;
    }
    else
    {
      msg += "'";
    }
    msgBox.setText(msg);
    msgBox.exec();
    return false;
  }
  return true;
} 


void vmSlide::calcScale(double power)
{
  double downSample2 = maxPower / power;
  double downSampleZ2 = downSample2 / 4;
  if (downSample2 < 1.0 || downSampleZ2 < 0.25) 
  {
    downSample2=1.0;
    downSampleZ2 = 0.25;
  }
  double factor;
  double diff;
  double width=windowWidth;
  double height=windowHeight;
  if (downSample > downSample2)
  {
    factor=downSample / downSample2;
    diff=(((width * factor) - width) / 2) * downSample2;
    //gPrintf("zoom %f add x=%f", factor, diff);
    xStart += round(diff);
    diff=(((height * factor) - height) / 2) * downSample2;
    //gPrintf(" add y=%f\n", diff);
    yStart += round(diff);
  }
  else
  {
    factor=downSample2 / downSample;
    diff=(((width * factor) - width) / 2) * downSample;
    //gPrintf("zoom %f subtract x=%f", factor, diff);
    xStart -= round(diff);
    diff=(((height * factor) - height) / 2) * downSample;
    //gPrintf(" subtract y=%f\n", diff);
    yStart -= round(diff);
  } 
  downSample=downSample2;
  downSampleZ = downSampleZ2;
  currentPower = power;
  //gPrintf("vmDoScale: sl->currentPower=%f\n", sl->currentPower);
  setMagnification(true);
}


void vmSlide::scaleChanged(double leverPower2)
{
  if (leverPower2 < minLeverPower)
  {
    leverPower2 = minLeverPower;
  }
  else if (leverPower2 > maxLeverPower)
  {
    leverPower2 = maxLeverPower;
  }
  double power2 = vmFromLeverPower(leverPower2, leverBuildPower);
  leverPower = leverPower2;
  if (rangeTool)
  {
    rangeTool->setLevel(leverPower);
  }
  if (power2 > maxPower)
  {
    power2 = maxPower;
  }
  else if (power2 < minPower / 8)
  {
    power2 = minPower / 8;
  }
  qDebug() << " from scale current power " << power2 << " and scale power" <<  leverPower2;
  calcScale(power2);
} 


void vmSlide::calcScroll(int direction)
{
  double leverPower2 = leverPower;
  double oldPower = leverPower;
  if (direction > 0)
  {
    leverPower2 += 0.5;
  }
  else if (direction < 0)
  {
    leverPower2 -= 0.5;
  }
  else
  {
    return;
  }
  if (leverPower2 < -1.0)
  {
    leverPower2 = -1.0;
  }
  else if (leverPower2 > maxLeverPower)
  {
    leverPower2 = maxLeverPower;
  }
  double currentPower2 = vmFromLeverPower(leverPower2, leverBuildPower);
  leverPower = leverPower2;    
  scaleChanged(leverPower);
  if (/*scaleAdjShow && */ leverPower != oldPower)
  {
    //adjustmentSetValue(leverPower);
    //
    calcScale(currentPower2);
    qDebug() << "NEW DOWNSAMPLE: leverPower=" << leverPower2 << " downSample" << downSample << " CURRENT POWER: " << currentPower;
  }
  /*
  if (!scaleAdjShow) 
  {
    calcScale(currentPower);
  }*/
}


void vmSlideApp::closeEverything()
{
  for (int i = 0; i < totalWindows; i++)
  {
    vmSlideWindow * window = parentWindows[i];
    if (window)
    {
      removeWindow(window);
    }
  }
}


vmSlide* vmSlideApp::findSlide(int windowNumber, int tabNumber)
{
  for (int i = 0; i < totalSlides; i++)
  {
    if (sls[i]->windowNumber == windowNumber && sls[i]->tabNumber == tabNumber)
    {
      return sls[i];
    }
  }
  return NULL;
}


void vmSlideApp::removeWindow(vmSlideWindow *window)
{
  totalActiveWindows--;
  qDebug() << " totalActiveWindows=" << totalActiveWindows;
  parentWindows[window->getWindowNumber()] = 0;
  for (int i = 0; i < slApp->totalSlides; i++)
  {
    vmSlide* sl=sls[i];
    vmSlideWindow* parent=sl->getParentWindow();
    if (parent && window && parent->getWindowNumber()==window->getWindowNumber())
    {
      slApp->removeSlide(sl->getSlideNumber());
      sl->slideMarkClose(); 
    }
  }
  if (totalActiveWindows<=0)
  {
    qDebug() << " set keepRunning=false";
    slApp->keepRunning = false;
    quit();
  }
}


vmSlide::vmSlide()
{
  slideNumber = 0;
  tabNumber = 0;
  windowNumber = 0;
  parentWindow = NULL;
  cache = NULL;
  drawingArea = NULL;
  zoomArea = NULL;
  rangeTool = NULL;
  done = false;
  ref = 0;
  zLevel = 0;
  direction = 0;
  level = 0;
  zoomLevel = 0;
  totalLevels = 0;
  topWidth = 0;
  topHeight = 0;
  xScale = 1.0;
  yScale = 1.0;
  xZoomScale = 1.0;
  yZoomScale = 1.0;
  xMouseScale = 1.0;
  yMouseScale = 1.0;
  xMouse = 0;
  yMouse = 0;
  xMoveStart = 0;
  yMoveStart = 0;
  xStart = 0;
  yStart = 0;
  mouseDown = false;
  doScale = true;
  doZoomScale = false;
  baseWidth = 0;
  baseHeight = 0;
  windowWidth = 0;
  windowHeight = 0;
  downSampleZ = 1.0;
  downSample = 1.0;
  maxDownSample = 1.0;
  minDownSample = 1.0;
  widgetWidth = 0;
  widgetHeight = 0;
  maxPower = 0.0;
  minPower = 0.0;
  currentPower = 0.0;
  maxLeverPower = 0.0;
  minLeverPower = 0.0;
  leverPower = 0.0;
  leverBuildPower = 0.0;
  showZoom = false; //true;
  if (slApp->getTotalSlides()==0)
  {
    slApp->setCurrentWindowNumber(0);
    slApp->setCurrentFocusWindow(0);
  }
}


vmSlideApp::vmSlideApp(int& argc, char* argv[]) : QApplication(argc, argv)
{
  totalReadReqs = 0;
  keepRunning = true;
  currentWindowNumber = 0;
  currentFocusWindow = 0;
  totalWindows = 0;
  totalActiveWindows = 0;
  totalSlides = 0;
  slApp=this;
  cacheInitialize(4000);
  readThread.start();
  qDebug() << "Start vmSlideApp()! \n";
  vmSlideWindow * window = NULL;

  window = createMainWindow();
  QString title = "vmslideviewer";
  window->setWindowTitle(title);
  window->show();

  for (int i=1; i < argc; i++)
  {
    QString filename = argv[i];
    vmSlideWindow *window2 = NULL;
    if (filename.isEmpty()) continue;
    if (i==1) window2 = window;
    addSlide(filename, false, window2);
  }
}


int main(int argc, char *argv[])
{
  QCoreApplication::addLibraryPath(".");
  vmSlideApp app(argc, argv);
  int status = app.exec();
  app.readCondMutex.lock();
  app.readCond.wakeOne();
  app.readCondMutex.unlock();
  app.readThread.wait(60000);
  return status;
}

