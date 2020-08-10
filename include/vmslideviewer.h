#ifndef VMSLIDEVIEWER_H
#define VMSLIDEVIEWER_H

#include <QApplication>
#include <QGraphicsView>
#include <QMainWindow>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QTabBar>
#include <QLabel>
#include <QtDebug>
#include <QGraphicsView>
#include <QGraphicsSceneDragDropEvent>
#include <QResizeEvent>
#include <QAction>
#include <QScrollArea>
#include <QScrollBar>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMenu>
#include <QRect>
#include <QPaintEvent>
#include <QPainter>
#include <QMdiArea>
#include <QGridLayout>
#include <QStyle>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QPointer>
#include <QScopedPointer>
#include <QScopedArrayPointer>
#include <QtMath>
#include "slidecache.h"
extern "C"
{
#include "openslide/openslide.h"
}

class vmSlide;
class vmSlideWindow;
class vmDrawingArea;
class vmZoomArea;
class vmRangeTool;
class vmReadThread;

typedef struct vmReadRequest vmReadRequest;
typedef struct vmSlideCache vmSlideCache;
typedef struct vmSlideLevel vmSlideLevel;

extern QMutex cacheMutex;


class vmReadThread : public QThread
{
  Q_OBJECT
public:
  void run() override;
};


class vmSlideApp : public QApplication
{
  Q_OBJECT
public:
  const int samplesPerPixel = 4;
  const int zoomWindowWidth=272;
  const int zoomWindowHeight=180;
  const int rangeToolWindowWidth=60;
  const int rangeToolWindowHeight=180;
  const int scaleWindowWidth=100;
  const int scaleWindowHeight=240;
  const int zoomGap=10;
  const int rangeToolGap=10;
  static uint8_t backgroundRgba[4];
  QVector<vmReadRequest*> readReqs;
  QMutex readMutex;
  QMutex readCondMutex;
  QWaitCondition readCond;
  vmReadThread readThread;
  int totalReadReqs;
  QVector<vmReadRequest*> netReqs;
  QMutex netMutex;
  int totalNetReqs;
  bool keepRunning;
protected:
  QVector<vmSlide*> sls;
  QVector<vmSlideWindow*> parentWindows;
  int currentWindowNumber;
  int currentFocusWindow;
  int totalWindows;
  int totalActiveWindows;
	int totalSlides;
public:
  vmSlideApp(int& argc, char* argv[]);
  void openCallback(int argc, char* argv[]);
  vmSlide* addSlide(QString& filename, bool newWindow, vmSlideWindow *window);
  int addSlide(vmSlide *sl);
  void removeSlide(int slideNumber);
  vmSlideWindow* createMainWindow();
  void removeWindow(vmSlideWindow *window);
  void setCurrentWindowNumber(int number) { currentWindowNumber = number; }
  void setCurrentFocusWindow(int number) { currentFocusWindow = number; }
  int getTotalActiveWindows() { return totalActiveWindows; }
  int getTotalSlides() { return totalSlides; }
  void closeEverything();
  vmSlide* findSlide(int windowNumber, int pageNumber);
  void closeZoomWindows(int windowNumber);
  void closeRangeTools(int windowNumber);
};

extern vmSlideApp* slApp;



class vmSlideWindow : public QMainWindow
{
  Q_OBJECT
protected:
  QPointer<QToolBar> mainToolBar;
  QPointer<vmDrawingArea> drawingArea;
  QPointer<vmZoomArea> zoomArea;
//  QString title;
  QPointer<QAction> openFileAct, openFileAct2, openFileButtonAct, exitAct, closeWindowAct, closeTabAct;
  QPointer<QAction> showZoomAct, showRangeToolAct;
  QPointer<QTabWidget> tabWidget;
  QPointer<QGraphicsScene> scene;
  QVector<int> tabNumbers;
  int titleBarHeight;
	int windowNumber;
  int currentTabNumber;
  int totalTabs;
  bool showZoom, showRange;
public:
  explicit vmSlideWindow();
  ~vmSlideWindow();
  void setupWindow();
  void closeEvent(QCloseEvent* event);
  void addSlidePage(vmSlide *sl);
  int getWindowNumber() { return windowNumber; }
  void setWindowNumber(int number) { windowNumber = number; }
  //int getCurrentSlideNumber() { return currentSlideNumber; }
  //void setCurrentSlideNumber(int number) { currentSlideNumber = number; }
  bool getShowZoom() { return showZoom; }
  bool getShowRange() { return showRange; }
  QString showOpenFileDlg();
  void removeAllZoomAreaWindows();
  void removeAllRangeTools();
  void createAllZoomAreaWindows();
  void createAllRangeTools();
  void enableCloseTabMenu() { closeTabAct->setEnabled(true); } 
//  void contextMenuEvent(QContextMenuEvent *event);
public slots:
  void openFileInNewWindow();
  void openFileInNewTab();
  void closeTab();
  void closeWindow();
  void quit();
  void tabChanged(int index);
  void tabClose(int index);
  void showZoomMenuActivated();
  void showRangeMenuActivated();
};


class vmSlide
{
public:
  QString filename;
  int slideNumber;
  int windowNumber;
  int tabNumber;
	vmSlideWindow *parentWindow;
  vmSlideCache *cache;
  vmDrawingArea *drawingArea;
  vmZoomArea *zoomArea;
  vmRangeTool *rangeTool;
  bool done;
  bool isZoom;
  bool isOnNetwork;
  int ref;
  int zLevel, direction;
  int32_t level, zoomLevel, totalLevels;
  QVector <vmSlideLevel> pyramidLevels;
  int topWidth, topHeight;
  double xScale, yScale;
  double xZoomScale, yZoomScale;
  double xMouseScale, yMouseScale;
  int xMouse, yMouse;
  int xMoveStart, yMoveStart;
  int64_t xStart, yStart;
  bool mouseDown;
  bool doScale, doZoomScale;
  int64_t baseWidth, baseHeight;
  int windowWidth, windowHeight;
  double downSampleZ;
  double downSample, maxDownSample, minDownSample;
  int widgetWidth, widgetHeight;
  double maxPower, minPower, currentPower;
  double maxLeverPower, minLeverPower, leverPower, leverBuildPower;
  bool showZoom;
  QString maxPowerTxt;
  QString vendor;
  QString comment;
  QString baseMppX;
  QString baseMppY;
public:
  vmSlide();
  void calcScale(double power);
  void calcScroll(int direction);
  void findMagnification();
  void setMagnification(bool best);
  void scaleChanged(double power2);
  void markDone() { done=true; }
  bool getDone() { return done; }
  int getRefCount() { return ref; }
  void decrementRef() { ref--; }
  void incrementRef() { ref++; }
  vmSlideCache * getCache() { return cache; }
  void slideMarkClose();
  void slideUnref();
  void slideRef(); 
  bool getShowZoom() { return showZoom; }
  void setDrawingArea(vmDrawingArea *drawingAreaNew) { drawingArea = drawingAreaNew; }
  void setZoomArea(vmZoomArea *zoomAreaNew) { zoomArea = zoomAreaNew; }
  void setParentWindow(vmSlideWindow *window) { parentWindow = window; }
  vmSlideWindow* getParentWindow() { return parentWindow; }
  void setSlideNumber(int number) { slideNumber = number; }
  int getSlideNumber() { return slideNumber; }
  void setTabNumber(int number) { tabNumber = number; }
  void setWindowNumber(int number) { windowNumber = number; }
  bool openslide(QString& filename);
  inline bool getIsOnNetwork() { return isOnNetwork; }
};


class vmDrawingArea : public QGraphicsView
{
  Q_OBJECT
public:
  explicit vmDrawingArea(vmSlide *slNew, QWidget *parent, QGraphicsScene* scene);
  ~vmDrawingArea();
  //void paintEvent(QPaintEvent *event);
  //void render(QPainter *painter, const QRectF &destRect, const QRect &sourceRect, Qt::AspectRatioMode aspectRatioMode);
  //void drawBackground(QPainter*, const QRectF&);
  void drawForeground(QPainter*, const QRectF&);
  //void draw(QPainter *painter, const QRectF &destRect, const QRectF &sourceRect);
  void resizeEvent(QResizeEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void keyPressEvent(QKeyEvent *k);
  void setMagnification();
  void rangeToolClick(double leverPower);
  void initialize();
  void updateDrawingRect(int x, int y, int width, int height) { emit requestUpdate(x, y, width, height); }
protected:
  vmSlide *sl;
  vmSlideWindow *mainWindow;
  vmZoomArea *zoomArea;
  vmRangeTool *rangeTool;
  int xMoveStart, yMoveStart;
public: signals:
  void requestUpdate(int x, int y, int width, int height);
public slots:
  void updateDrawingRectSlot(int x, int y, int width, int height);
  void horizScrollChanged(int value);
  void vertScrollChanged(int value);
  void createZoomAreaWindow();
  void createRangeTool();
  void destroyZoomAreaWindow();
  void destroyRangeTool();
};


class vmZoomArea : public QWidget
{
  Q_OBJECT
public:
  explicit vmZoomArea(vmSlide *slNew);
  void paintEvent(QPaintEvent *event);
  void updateZoomRect(int x, int y, int width, int height) { emit requestZoomUpdate(x, y, width, height); }
protected:
  vmSlide *sl;
public: signals:
  void requestZoomUpdate(int x, int y, int width, int height);
public slots:
  void updateZoomRectSlot(int x, int y, int width, int height);
};


class vmRangeTool : public QWidget
{
  Q_OBJECT
public:
  explicit vmRangeTool(vmSlide *slNew);
  void setLevel(double leverPower);
  void paintEvent(QPaintEvent* evt);
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void assignSlide(vmSlide *slNew);
protected:
  vmSlide *sl;
  QVector<QLabel*> labels; 
  QPixmap *activePixmap;
  int rangeLevels;
  int lastLevel;
  bool mouseDown;
  bool mouseInWidget;
  int lastHover;
  int winWidth, winHeight;
};


struct vmSlideLevel
{
  int64_t width;
  int64_t height;
  double downSample;
  double power;
};

double vmToLeverPower(double power, double minPower, double maxLeverPower, bool halfMinPower);
double vmFromLeverPower(double scaleLever, double minPower);

#endif // VMSLIDEVIEWER_H
