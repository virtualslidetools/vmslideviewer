#include "vmslideviewer.h"

vmSlideWindow::vmSlideWindow()
{
  setupWindow();
}


vmSlideWindow::~vmSlideWindow()
{
}


void vmSlideWindow::tabChanged(int tab)
{
  qDebug() << " Tab Changed: " << tab;
  //drawingArea->hide();
  //drawingArea = drawingAreas[tab];
  //drawingArea->show();
  currentTabNumber = tab;
  QString title = "vmslideviewer";
  vmSlide* sl=slApp->findSlide(windowNumber, tab);
  if (sl)
  {
    title.append(" ");
    title.append(sl->filename);
    setWindowTitle(title);
  }
  else
  {
    setWindowTitle(title);
  }
  if (zoomArea)
  {
    zoomArea->update();
  }
}


void vmSlideWindow::closeEvent(QCloseEvent* event)
{
  close();
  slApp->removeWindow(this);
  event->accept();
}


void vmSlideWindow::closeTab()
{
  tabClose(currentTabNumber);
}


void vmSlideWindow::closeWindow()
{
  slApp->removeWindow(this);
}


void vmSlideWindow::quit()
{
  slApp->closeEverything();
  slApp->quit();
}


void vmSlideWindow::addSlidePage(vmSlide *sl)
{
  drawingArea = new vmDrawingArea(sl, tabWidget, scene);
  sl->setWindowNumber(windowNumber);
  int tabNumber = tabWidget->addTab(drawingArea, sl->filename);
  sl->setTabNumber(tabNumber);
  tabNumbers.append(tabNumber);
  totalTabs++;
  zoomArea = sl->zoomArea;
  if (zoomArea)
  {
    zoomArea->update();
  }
}


QString vmSlideWindow::showOpenFileDlg()
{
  QString selFilter = tr("SVS Files (*.svs)");
  QString dir= tr("");
  QString fileName
      = QFileDialog::getOpenFileName(this, tr("Open Slide"), dir, tr("All files (*.*)"), &selFilter);
  return fileName;
}


void vmSlideWindow::openFileInNewWindow()
{
  QString filename=showOpenFileDlg();
  if (filename.isEmpty()) return;
  slApp->addSlide(filename, true, this);
}


void vmSlideWindow::openFileInNewTab()
{
  QString filename=showOpenFileDlg();
  if (filename.isEmpty()) return;
  slApp->addSlide(filename, false, this);
}


void vmSlideWindow::tabClose(int tab)
{
  tabWidget->removeTab(tab);
  vmSlide* sl=slApp->findSlide(windowNumber, tab);
  sl->slideMarkClose();
  if (tabWidget->count() <= 0)
  {
    closeTabAct->setEnabled(false);
  }
}


void vmSlideWindow::showZoomMenuActivated()
{
  showZoom = (showZoom == true ? false : true);
  if (showZoom)
  {
    createAllZoomAreaWindows();
    showZoomAct->setText(tr("&Hide Zoom Window"));
  }
  else
  {
    removeAllZoomAreaWindows();
    showZoomAct->setText(tr("&Show Zoom Window"));
  }
//  showZoomAct->setChecked(showZoom);
}


void vmSlideWindow::showRangeMenuActivated()
{
  showRange = (showRange == true ? false : true);
  if (showRange)
  {
    createAllRangeTools();
    showRangeToolAct->setText(tr("Hide &Range Tool"));
  }
  else
  {
    removeAllRangeTools();
    showRangeToolAct->setText(tr("Show &Range Tool"));
  }
//  showRangeToolAct->setChecked(showRange);
}


void vmSlideWindow::removeAllZoomAreaWindows()
{
  int total = tabNumbers.size();
  for (int i = 0; i < total; i++)
  {
    int tab=tabNumbers[i];
    vmSlide* sl=slApp->findSlide(windowNumber, tab);
    sl->zoomArea = NULL;
    vmDrawingArea* drawingArea = sl->drawingArea;
    if (drawingArea)
    {
      drawingArea->destroyZoomAreaWindow();
    }
  }
}


void vmSlideWindow::removeAllRangeTools()
{
  int total = tabNumbers.size();
  for (int i = 0; i < total; i++)
  {
    int tab=tabNumbers[i];
    vmSlide* sl=slApp->findSlide(windowNumber, tab);
    sl->rangeTool = NULL;
    vmDrawingArea* drawingArea = sl->drawingArea;
    if (drawingArea)
    {
      drawingArea->destroyRangeTool();
    }
  }
}


void vmSlideWindow::createAllZoomAreaWindows()
{
  int total = tabNumbers.size();
  zoomArea = NULL;
  for (int i = 0; i < total; i++)
  {
    int tab=tabNumbers[i];
    vmSlide* sl=slApp->findSlide(windowNumber, tab);
    vmDrawingArea* drawingArea = sl->drawingArea;
    if (drawingArea)
    {
      drawingArea->createZoomAreaWindow();
    }
  }
}


void vmSlideWindow::createAllRangeTools()
{
  int total = tabNumbers.size();
  for (int i = 0; i < total; i++)
  {
    int tab=tabNumbers[i];
    vmSlide* sl=slApp->findSlide(windowNumber, tab);
    vmDrawingArea* drawingArea = sl->drawingArea;
    if (drawingArea)
    {
      drawingArea->createRangeTool();
    }
  }
}


void vmSlideWindow::setupWindow()
{
  windowNumber = 0;
  currentTabNumber = 0;
  showZoom = true;
  showRange = true;
  drawingArea = NULL;
  zoomArea = NULL;
  //----------------------------------------------------------------------
  // Create Menus
  //---------------------------------------------------------------------- 
  openFileAct = new QAction(tr("&Open File in New Tab..."), this);
  openFileAct->setShortcuts(QKeySequence::Open);
  connect(openFileAct, SIGNAL(triggered()), this, SLOT(openFileInNewTab()));

  openFileAct2 = new QAction(tr("Open File in New &Window..."), this);
  connect(openFileAct2, SIGNAL(triggered()), this, SLOT(openFileInNewWindow()));

  closeTabAct = new QAction(tr("&Close Tab"), this);
  closeTabAct->setEnabled(false);
  connect(closeTabAct, SIGNAL(triggered()), this, SLOT(closeTab()));

  closeWindowAct = new QAction(tr("Close Window"), this);
  closeWindowAct->setShortcuts(QKeySequence::Quit);
  connect(closeWindowAct, SIGNAL(triggered()), this, SLOT(closeWindow()));

  exitAct = new QAction(tr("Close All"), this);
  connect(exitAct, SIGNAL(triggered()), this, SLOT(quit()));

  QMenu *menu = menuBar()->addMenu(tr("&File"));

  menu->addAction(openFileAct);
  menu->addAction(openFileAct2);
  menu->addAction(closeTabAct);
  menu->addAction(closeWindowAct);
  menu->addAction(exitAct);

  showZoomAct = new QAction(tr("&Hide Zoom Window"), this);
  if (showZoom == false)
  {
    showZoomAct->setText(tr("&Show Zoom Window"));
  }
//  showZoomAct->setCheckable(true);
//  showZoomAct->setChecked(showZoom);
  connect(showZoomAct, SIGNAL(triggered()), this, SLOT(showZoomMenuActivated()));

  showRangeToolAct = new QAction(tr("Hide &Range Tool"), this);
  if (showRange == false)
  {
    showRangeToolAct->setText(tr("Show &Range Tool"));
  }
  //showRangeToolAct->setCheckable(true);
  //showRangeToolAct->setChecked(showRange);

  connect(showRangeToolAct, SIGNAL(triggered()), this, SLOT(showRangeMenuActivated()));
  QMenu *menu2 = menuBar()->addMenu(tr("&Show"));

  menu2->addAction(showZoomAct);
  menu2->addAction(showRangeToolAct);
#ifdef Q_OS_OSX
  setUnifiedTitleAndToolBarOnMac(true);
#endif

  //----------------------------------------------------------------------
  // Create Toolbar
  //---------------------------------------------------------------------- 
  //mainToolBar = addToolBar("Main Toolbar");
  //QPixmap openPixmap(":/images/openfileicon.png");
  //openFileButtonAct = new QAction(QIcon(openPixmap), "&Open File...", this);
  //connect(openFileButtonAct, SIGNAL(triggered()), this, SLOT(showOpenFileDlg()));
  //mainToolBar->addAction(openFileButtonAct);
  int xPos=x();
  int yPos=y();
  QSize size1, size2;
  size1=frameSize();
  size2=size();
  titleBarHeight=style()->pixelMetric(QStyle::PM_TitleBarHeight);
  qDebug() << " titleBarHeight=" << titleBarHeight;
  if (xPos<0) xPos=2;
  if (yPos<titleBarHeight) yPos=titleBarHeight+2;
  setGeometry(xPos, yPos, 800, 600);
  QLayout *currentLayout = layout();
  tabWidget = new QTabWidget(); 
  tabWidget->setTabsClosable(true);
  connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
  connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabClose(int)));
  currentLayout->addWidget(tabWidget);
  setCentralWidget(tabWidget);
  tabWidget->show();
  scene=new QGraphicsScene(tabWidget);
}

