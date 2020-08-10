#include "vmslideviewer.h"

void vmRangeTool::paintEvent(QPaintEvent* evt)
{
  QRect destRect = evt->rect();
  QPainter painter(this);
  int y1 = destRect.y();
  int x1 = destRect.x();
  int height = destRect.height();
  int width = destRect.width();
  QPen whitePen(Qt::white);
  QPen blackPen(QColor(48, 48, 48));
  QPen shadowPen(QColor(80, 80, 80));
  QPen bluePen(QColor(0, 127, 250));
  QBrush whiteBrush(Qt::white);
  QBrush blackBrush(Qt::black);

  painter.setPen(whitePen);
  painter.setBrush(whiteBrush);
  painter.drawRect(1, 1, winWidth-2, winHeight-2);
  painter.setBrush(blackBrush);

  painter.setPen(blackPen);
  int y2=y1+height;
  if (y2 > winHeight-2)
  {
    y2 = winHeight-2;
  }
  int x2=x1+width;
  if (x2 > winWidth-2)
  {
    x2 = winWidth-2;
  }
  if (x1 <= 0)
  {
    painter.drawLine(0, 0, 0, y2);
  }
  if (y1 <= 0)
  {
    painter.drawLine(0, 0, x2, 0);
  }
  if (x2 >= winWidth-2)
  {
    painter.drawLine(x2, 0, x2, y2);
  }
  int innerWinHeight = winHeight-1;
  if (y2 >= innerWinHeight-1)
  {
    painter.drawLine(0, y2, x2, y2);
  }

  painter.setPen(shadowPen);
  if (x1+width >= winWidth-1)
  {
    painter.drawLine(winWidth-1, 1, winWidth-1, y1+height);
  }
  if (y1+height >= innerWinHeight)
  {
    painter.drawLine(1, innerWinHeight, x1+width, innerWinHeight);
  }

  if (x1 <= 10 && x1+width >= 10)
  {
    int y2=y1;
    if (y2 < 15) y2 = 15;
    if (y2+height > winHeight)
    {
      height = winHeight - y2;
    }
    if (height > 0)
    {
      painter.drawLine(12, y2, 12, height);
    }
  }
  if (x1 <= 11 && x1+width >= 11)
  {
    int yStart = qFloor(y1 / 10) * 10;
    int yStart2 = yStart;
    if (yStart2 < 10) yStart2 = 10;
    int yEnd = yStart2 + height;
    if (yEnd > winHeight) yEnd = winHeight;
    for (int y2=yStart2+5; y2 < yEnd; y2 += 10)
    {
      painter.drawLine(13, y2, 17, y2);
    }
  }
  QFont font = painter.font();
  font.setPointSize(10);
  painter.setFont(font);
  int yStart = qFloor(y1 / 20) * 20;
  int yEnd = y1+height;
  if (yEnd > winHeight) yEnd = winHeight;
  for (int y2=yStart; y2 <= yEnd; y2 += 20)
  {
    QString str;
    QTextStream ts(&str);
    ts.setRealNumberPrecision(3);
    if (y2 / 20 == lastHover)
    {
      painter.setPen(bluePen);
    }
    else
    {
      painter.setPen(blackPen);
    }
    double leverPower = (double) qFloor(y2 / 20) + sl->minLeverPower;

    double currentPower = vmFromLeverPower(leverPower, sl->leverBuildPower);
    ts << currentPower << "x";
    //qDebug() << "PWR: " << currentPower << "Y2: " << y2 << " YEND: " << yEnd << " MEASUREHEIGHT: " << winHeight;
    QString *str2 = ts.string();
    painter.drawText(19, y2+20, *str2); 
  } 
}


void vmRangeTool::mouseMoveEvent(QMouseEvent *event)
{
  if (mouseInWidget == false) return;
  int y = event->y();
  int currentHover = y / 20;
  if (currentHover == lastHover) return;
  lastHover = currentHover;
  update();
}


void vmRangeTool::mousePressEvent(QMouseEvent *event)
{
  Q_UNUSED(event);
  if (mouseInWidget == false) return;
  mouseDown = true;
}


void vmRangeTool::mouseReleaseEvent(QMouseEvent *event)
{
  if (mouseDown == false) return;
  mouseDown = false;
  int y = event->y() / 20;
  if (y >= 0 && y <= rangeLevels)
  {
    sl->drawingArea->rangeToolClick((double) y - 1.0);
  }
}


void vmRangeTool::enterEvent(QEvent *event)
{
  Q_UNUSED(event);
  mouseInWidget = true;
}


void vmRangeTool::leaveEvent(QEvent *event)
{
  Q_UNUSED(event);
  mouseInWidget = false;
  lastHover = -1;
  update();
}


vmRangeTool::vmRangeTool(vmSlide *slNew) : QWidget(slNew->drawingArea) 
{ 
  sl = slNew;
  sl->rangeTool = this;
  //setAutoFillBackground(true);
  int x = slApp->rangeToolGap;
  int y = slApp->rangeToolGap;
  raise();

  activePixmap = new QPixmap(10, 10);
  activePixmap->fill(Qt::transparent);
  QPainter *painter = new QPainter(activePixmap);
  QColor midBlack(64, 64, 64);
  QColor dark(64, 64, 64);
  QPen darkPen(midBlack);
  QBrush darkBrush(dark);
  painter->setPen(darkPen);
  painter->setBrush(darkBrush);
  painter->setRenderHint(QPainter::Antialiasing, true);

  QRectF rect = QRectF(0, 0, 10, 10);

  QPainterPath path;
  QPointF points[4] = {
    QPointF(2.0, 2.0),
    QPointF(rect.width()-2.0, (rect.height())/2),
    QPointF(2.0, rect.height()-2.0),
    QPointF(2.0, 2.0)
  };
  painter->drawPolygon(points, 4, Qt::OddEvenFill);
  
  //path.moveTo(rect.left() + (rect.width() / 2), rect.top());
  //path.lineTo(rect.bottomLeft());
  //path.lineTo(rect.bottomRight());
  //path.lineTo(rect.left() + (rect.width() / 2), rect.top());
  //painter->fillPath(path, QBrush(QColor(0,0,0)));
    
  QFont font;
  font.setPointSize(10);
  QFontMetrics fm(font);
  
  int step = 0;
  winWidth = 0;
  for (double leverPower = sl->minLeverPower; leverPower <= sl->maxLeverPower; leverPower += 0.5)
  {
    QLabel *labelPixmap = new QLabel(this);
    labels.push_back(labelPixmap);
    labelPixmap->setGeometry(2, (step*10)+10, 10, 10);
    labelPixmap->show();
    labelPixmap->raise();
    step++;
    
    QString str;
    QTextStream ts(&str);
    ts.setRealNumberPrecision(3);
    double currentPower = vmFromLeverPower(leverPower, sl->leverBuildPower);
    ts << currentPower << "x";
    int lineWidth;
    #if QT_VERSION < 0x050B00
      lineWidth = fm.width(str);
    #else
      lineWidth = fm.horizontalAdvance(str);
    #endif
    if (lineWidth > winWidth) winWidth = lineWidth;
  }
  rangeLevels = step+2;
  winWidth += 27;
  winHeight = rangeLevels * 10;
  setGeometry(x, y, winWidth, winHeight);
  show();
  lastLevel = -1;
  lastHover = -1;
  mouseDown = false;
  setCursor(Qt::PointingHandCursor);
  mouseInWidget = false;
  setMouseTracking(true);
  setLevel(sl->leverPower);
}


void vmRangeTool::setLevel(double leverPower)
{
  qDebug() << "SET LEVEL=" << leverPower << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  if (sl && sl->cache)
  {
    if (lastLevel >= 0)
    {
      labels[lastLevel]->clear();
    }
    int level = (int) qRound((leverPower * 2) - (2 * sl->minLeverPower));
    //int level = (int) qRound((leverPower * 2) - 2.0);

    if (level >= 0 && level <= rangeLevels)
    {
      labels[level]->setPixmap(*activePixmap);
      lastLevel = level;
    }
  }
}

