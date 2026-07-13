#include "EnvLibDrawingArea.hpp"
#include "../windows/EnvelopeLibraryWindow.hpp"
#include "../core/EnvelopeLibraryEntry.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QtGlobal>
#include <cmath>

#include "../utilities.hpp"

/**
 * @brief Constructor: set up state, enable mouse tracking, and build context menu
 * @param _envelopeLibraryWindow  parent window reference
 * @param parent  parent widget
 */
EnvLibDrawingArea::EnvLibDrawingArea(EnvelopeLibraryWindow* _envelopeLibraryWindow,
                                     QWidget* parent)
  : QWidget(parent),
    envelopeLibraryWindow(_envelopeLibraryWindow),
    activeSegment(nullptr),
    activeNode(nullptr),
    mouseLeftButtonPressedDown(false),
    upperY(1.0),
    lowerY(0.0),
    head(nullptr),
    tail(nullptr)
{
    // Enable mouse tracking so mouseMoveEvent fires without press
    setMouseTracking(true);

    // Build context-menu actions
    actionInsert = new QAction(tr("Insert Node"), this);
    connect(actionInsert, &QAction::triggered, this, &EnvLibDrawingArea::insertEnvelopeSegment);

    actionRemove = new QAction(tr("Remove Node"), this);
    connect(actionRemove, &QAction::triggered, this, &EnvLibDrawingArea::removeNode);

    actionSetFlexible = new QAction(tr("Set Flexible"), this);
    connect(actionSetFlexible, &QAction::triggered, this, &EnvLibDrawingArea::setFlexible);

    actionSetFixed = new QAction(tr("Set Fixed"), this);
    connect(actionSetFixed, &QAction::triggered, this, &EnvLibDrawingArea::setFixed);

    actionSetLinear = new QAction(tr("Set Linear"), this);
    connect(actionSetLinear, &QAction::triggered, this, &EnvLibDrawingArea::setLinear);

    actionSetSpline = new QAction(tr("Set Spline"), this);
    connect(actionSetSpline, &QAction::triggered, this, &EnvLibDrawingArea::setSpline);

    actionSetExponential = new QAction(tr("Set Exponential"), this);
    connect(actionSetExponential, &QAction::triggered, this, &EnvLibDrawingArea::setExponential);

    // Assemble QMenu
    m_pMenuPopup = new QMenu(this);
    m_pMenuPopup->addAction(actionInsert);
    m_pMenuPopup->addAction(actionRemove);
    m_pMenuPopup->addSeparator();
    m_pMenuPopup->addAction(actionSetFixed);
    m_pMenuPopup->addAction(actionSetFlexible);
    m_pMenuPopup->addSeparator();
    m_pMenuPopup->addAction(actionSetLinear);
    m_pMenuPopup->addAction(actionSetSpline);
    m_pMenuPopup->addAction(actionSetExponential);
}

/**
 * @brief Destructor: Qt parent-child cleanup handles everything
 */
EnvLibDrawingArea::~EnvLibDrawingArea()
{
    // Qt parent-child cleanup handles everything
}

/**
 * @brief Reset all internal fields to their defaults
 */
void EnvLibDrawingArea::resetFields()
{
    activeNode = nullptr;
    moveLeftBound = moveRightBound = 0.0;
    head = nullptr;
    tail = nullptr;
    upperY = 1.0;
    lowerY = 0.0;
}

/**
 * @brief Trigger a redraw of the given envelope
 * @param _envelope  envelope to display (unused parameter)
 */
void EnvLibDrawingArea::showGraph(EnvelopeLibraryEntry* /*_envelope*/)
{
    // schedule a repaint
    update();
}

/**
 * @brief Clear the drawing area (background will repaint white)
 */
void EnvLibDrawingArea::clearGraph()
{
    // repaint background white
    update();
}

/**
 * @brief Update the active node's coords from external input
 * @param _x  x-coordinate value
 * @param _y  y-coordinate value
 */
void EnvLibDrawingArea::setActiveNodeCoordinate(const QString& _x, const QString& _y)
{
    // Update the selected node's x/y from user text fields
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env || !activeNode) return;

    double newX = _x.toDouble();
    double newY = _y.toDouble();
    
    // Apply boundary constraints
    if (activeNode->leftSeg && activeNode->rightSeg) {
        // Middle nodes: constrain X between adjacent nodes
        double leftBound = activeNode->leftSeg->leftNode->x + 0.001;
        double rightBound = activeNode->rightSeg->rightNode->x - 0.001;
        newX = qBound(leftBound, newX, rightBound);
    } else {
        // Head/tail nodes: only constrain Y
        newX = activeNode->x; // Keep X unchanged for head/tail
    }
    
    // Constrain Y to [0, 10]
    newY = qBound(0.0, newY, 10.0);
    
    activeNode->x = newX;
    activeNode->y = newY;
    
    MUtilities::modified();
    adjustBoundary(env);
    showGraph(env);
}

/**
 * @brief Recompute upperY/lowerY based on envelope data
 * @param _envelope  envelope to analyze
 */
void EnvLibDrawingArea::adjustBoundary(EnvelopeLibraryEntry* _envelope)
{
    if (!_envelope) return;
    EnvLibEntrySeg* segment = _envelope->head->rightSeg;
    double maxVal = 0.0, minVal = 0.0;

    // Find actual max and min y values (not just ceil/floor)
    while (segment) {
        EnvLibEntryNode* nd = segment->rightNode;
        if (nd->y > maxVal) maxVal = nd->y;
        if (nd->y < minVal) minVal = nd->y;
        segment = nd->rightSeg;
    }

    // Fixed 0-10 range (Y is capped at 10)
    upperY = qMax(1.0, maxVal);
    lowerY = 0.0;

    // Update boundary display
    QString txt = QString("%1\n\n\n\n\n\n\n\n\n\n\n\n%2")
                  .arg(QString::number(upperY, 'f', 3))
                  .arg(QString::number(lowerY, 'f', 3));
}

/**
 * @brief Paint the envelope graph
 * @param event  paint event (unused)
 */
void EnvLibDrawingArea::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;

    adjustBoundary(env);

    int w = width(), h = height();

    // Draw grid lines
    painter.save();
    QPen gridPen(QColor(220,220,220));
    gridPen.setStyle(Qt::DashLine);
    painter.setPen(gridPen);
    // Y-axis: lines every 0.25 (4 intervals)
    for (int i = 1; i < 4; ++i) {
        int y = h - i * h / 4;
        painter.drawLine(0, y, w, y);
    }

    // X-axis: lines every 0.2 (5 intervals)
    for (int i = 1; i < 5; ++i) {
        int x = i * w / 5;
        painter.drawLine(x, 0, x, h);
    }
    painter.restore();

    // Draw axis labels
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    painter.setPen(Qt::black);
    // Y-axis labels along left edge at each grid line (skip 0, shown by X-axis)
    for (int i = 1; i <= 4; ++i) {
        int y = h - i * h / 4;
        double value = lowerY + (upperY - lowerY) * i / 4.0;
        QString label = QString::number(value, 'f', 2);
        painter.drawText(3, y - 3, label);
    }
    // X-axis labels along bottom at each grid line
    for (int i = 0; i <= 5; ++i) {
        int x = i * w / 5;
        QString label = QString::number(i * 0.2, 'f', 1);
        painter.drawText(x + 2, h - 3, label);
    }

    //adjustBoundary(env);

    // Draw each segment
    EnvLibEntrySeg* seg = env->head ? env->head->rightSeg : nullptr;
    while (seg) {
        EnvLibEntryNode* L = seg->leftNode;
        EnvLibEntryNode* R = seg->rightNode;
        double x1 = L->x * w*w / double(w+1);
        double y1 = h - getAdjustedY(L->y)*h*h / double(h+1);
        double x2 = R->x * w*w / double(w+1);
        double y2 = h - getAdjustedY(R->y)*h*h / double(h+1);

        QPen pen;
        // color by type (use dark/contrasting colors)
        if (seg->segmentType == envSegmentTypeLinear)
            pen.setColor(Qt::blue); // Blue for linear
        else if (seg->segmentType == envSegmentTypeExponential)
            pen.setColor(Qt::red); // Red for exponential
        else
            pen.setColor(Qt::darkGreen); // Dark green for spline

        pen.setWidth(seg->segmentProperty == envSegmentPropertyFixed ? 1 : 3);
        painter.setPen(pen);

        // Draw exponential curve for exponential segments
        if (seg->segmentType == envSegmentTypeExponential) {
            // Ensure x1 < x2 for the exponential calculation
            double startx = x1, starty = y1, endx = x2, endy = y2;
            if (startx > endx) {
                std::swap(startx, endx);
                std::swap(starty, endy);
            }
            
            // Draw exponential curve using small line segments
            const double step = 2.0; // Step size in pixels
            double tempx = startx;
            
            // Use the exact LASS exponential formula:
            // value = y1 + (y2 - y1) * ((1 - pow(base, (I * alpha))) / (1 - pow(base, alpha)))
            const double base = 2.718282; // e
            double alpha = 3.0;
            if (starty > endy) {
                alpha = -alpha; // Reverse curve direction
            }
            
            while (tempx < endx - step) {
                double I = (tempx - startx) / (endx - startx); // normalized time [0,1]
                double exp_term = (1.0 - pow(base, I * alpha)) / (1.0 - pow(base, alpha));
                double tempy = starty + (endy - starty) * exp_term;
                
                double nextx = tempx + step;
                double nextI = (nextx - startx) / (endx - startx);
                double next_exp_term = (1.0 - pow(base, nextI * alpha)) / (1.0 - pow(base, alpha));
                double nexty = starty + (endy - starty) * next_exp_term;
                
                painter.drawLine(QPointF(tempx, tempy), QPointF(nextx, nexty));
                tempx = nextx;
            }
            
            // Draw final line to endpoint
            double finalI = (tempx - startx) / (endx - startx);
            double final_exp_term = (1.0 - pow(base, finalI * alpha)) / (1.0 - pow(base, alpha));
            painter.drawLine(QPointF(tempx, starty + (endy - starty) * final_exp_term), 
                           QPointF(endx, endy));
        } else {
            // Draw straight line for linear and spline segments
            painter.drawLine(QPointF(x1,y1), QPointF(x2,y2));
        }

        // draw endpoints
        painter.setBrush(Qt::black); // Black for node points
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x1,y1), 5,5);
        painter.drawEllipse(QPointF(x2,y2), 5,5);

        seg = R->rightSeg;
    }

    // highlight active node
    if (activeNode) {
        double ax = activeNode->x * w*w / double(w+1);
        double ay = h - getAdjustedY(activeNode->y)*h*h / double(h+1);
        painter.setBrush(Qt::magenta); // Magenta for active node
        painter.drawEllipse(QPointF(ax,ay), 8,8);
    }
}

/**
 * @brief Track mouse movement (update coords display & drag)
 * @param event  mouse move event
 */
void EnvLibDrawingArea::mouseMoveEvent(QMouseEvent* event)
{
    int w = width(), h = height();
    double x = event->x()*(w+1)/double(w*w);
    double y = 1.0 - event->y()*(h+1)/double(h*h);
    y = mouseAdjustY(y);

    // round to 3 decimals
    x = qRound(x*1000)/1000.0;
    y = qRound(y*1000)/1000.0;
    x = qBound(0.0, x, 1.0);
    // Constrain Y to [0, 4]
    y = qBound(0.0, y, 10.0);

    if (mouseLeftButtonPressedDown) {
        mouseX = x;
        mouseY = y;
        moveNode();
    }
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief Handle presses (select node, start drag, show popup)
 * @param event  mouse press event
 */
void EnvLibDrawingArea::mousePressEvent(QMouseEvent* event)
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) { QWidget::mousePressEvent(event); return; }

    activeSegment = nullptr;
    mouseX = event->x();
    mouseY = height() - event->y();

    // pick a node within ±5px
    EnvLibEntryNode* cand = env->head;
    int w = width(), h = height();
    bool found = false;
    while (cand) {
        double nx = cand->x*w*w/double(w+1);
        double ny = getAdjustedY(cand->y)*h*h/double(h+1);
        if (mouseX >= nx-5 && mouseX <= nx+5 &&
            mouseY >= ny-5 && mouseY <= ny+5) {
            activeNode = cand;
            found = true;
            break;
        }
        if (!cand->rightSeg) break;
        cand = cand->rightSeg->rightNode;
    }
    if (!found) {
        activeNode = nullptr;
        envelopeLibraryWindow->setEntries("",""); 
    } else {
        envelopeLibraryWindow->setEntries(
          QString::number(activeNode->x,'f',4),
          QString::number(activeNode->y,'f',4));
    }
    adjustBoundary(env);
    showGraph(env);

    // right-click → context menu
    if (event->button() == Qt::RightButton) {
        actionRemove->setEnabled(activeNode && activeNode->leftSeg && activeNode->rightSeg);
        m_pMenuPopup->exec(event->globalPos());
    }
    // left-click → start drag
    else if (event->button() == Qt::LeftButton) {
        mouseLeftButtonPressedDown = true;
    }
    QWidget::mousePressEvent(event);
}

/**
 * @brief Handle releases (end drag)
 * @param event  mouse release event
 */
void EnvLibDrawingArea::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        mouseLeftButtonPressedDown = false;
        activeSegment = nullptr;
    }
    QWidget::mouseReleaseEvent(event);
}

/**
 * @brief Fallback for context menu key events
 * @param event  context menu event
 */
void EnvLibDrawingArea::contextMenuEvent(QContextMenuEvent* event)
{
    // ensure correct sensitivity
    actionRemove->setEnabled(activeNode && activeNode->leftSeg && activeNode->rightSeg);
    m_pMenuPopup->exec(event->globalPos());
}

/**
 * @brief Insert a new node at the mouse position
 */
void EnvLibDrawingArea::insertEnvelopeSegment()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;
    MUtilities::modified();

    int w = width(), h = height();
    double ix = mouseX*(w+1)/double(w*w);
    double iy = 1.0 - mouseY*(h+1)/double(h*h);
    iy = mouseAdjustY(iy); // Convert to actual Y value
    ix = qRound(ix*1000)/1000.0;
    iy = qRound(iy*1000)/1000.0;

    EnvLibEntryNode* L = env->head;
    EnvLibEntryNode* R = L->rightSeg->rightNode;
    while (L->x < ix && R->x < ix) {
        L = R;
        R = R->rightSeg->rightNode;
    }

    // splice in new node & segment
    EnvLibEntryNode* newN = new EnvLibEntryNode(ix, iy);
    EnvLibEntrySeg*  newS = new EnvLibEntrySeg();
    L->rightSeg->rightNode = newN;
    newN->leftSeg = L->rightSeg;
    newN->rightSeg = newS;
    newS->leftNode = newN;
    newS->rightNode = R;
    R->leftSeg     = newS;
    newS->segmentType     = L->rightSeg->segmentType;
    newS->segmentProperty = L->rightSeg->segmentProperty;

    adjustBoundary(env);
    showGraph(env);
}

/**
 * @brief Set the segment at mouse position to fixed length
 */
void EnvLibDrawingArea::setFixed()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;
    MUtilities::modified();

    double ix = mouseX*(width()+1)/double(width()*width());
    EnvLibEntryNode* L = env->head;
    EnvLibEntryNode* R = L->rightSeg->rightNode;
    while (L->x < ix && R->x < ix) {
        L = R;
        R = R->rightSeg->rightNode;
    }
    L->rightSeg->segmentProperty = envSegmentPropertyFixed;
    adjustBoundary(env);
    showGraph(env);
}

/**
 * @brief Set the segment at mouse position to flexible length
 */
void EnvLibDrawingArea::setFlexible()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;
    MUtilities::modified();

    double ix = mouseX*(width()+1)/double(width()*width());
    EnvLibEntryNode* L = env->head;
    EnvLibEntryNode* R = L->rightSeg->rightNode;
    while (L->x < ix && R->x < ix) {
        L = R;
        R = R->rightSeg->rightNode;
    }
    L->rightSeg->segmentProperty = envSegmentPropertyFlexible;
    showGraph(env);
}

/**
 * @brief Set the segment at mouse position to linear interpolation
 */
void EnvLibDrawingArea::setLinear()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;
    MUtilities::modified();

    double ix = mouseX*(width()+1)/double(width()*width());
    EnvLibEntryNode* L = env->head;
    EnvLibEntryNode* R = L->rightSeg->rightNode;
    while (L->x < ix && R->x < ix) {
        L = R;
        R = R->rightSeg->rightNode;
    }
    L->rightSeg->segmentType = envSegmentTypeLinear;
    showGraph(env);
}

/**
 * @brief Set the segment at mouse position to spline interpolation
 */
void EnvLibDrawingArea::setSpline()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;
    MUtilities::modified();

    double ix = mouseX*(width()+1)/double(width()*width());
    EnvLibEntryNode* L = env->head;
    EnvLibEntryNode* R = L->rightSeg->rightNode;
    while (L->x < ix && R->x < ix) {
        L = R;
        R = R->rightSeg->rightNode;
    }
    L->rightSeg->segmentType = envSegmentTypeSpline;
    showGraph(env);
}

/**
 * @brief Set the segment at mouse position to exponential interpolation
 */
void EnvLibDrawingArea::setExponential()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env) return;
    MUtilities::modified();

    double ix = mouseX*(width()+1)/double(width()*width());
    EnvLibEntryNode* L = env->head;
    EnvLibEntryNode* R = L->rightSeg->rightNode;
    while (L->x < ix && R->x < ix) {
        L = R;
        R = R->rightSeg->rightNode;
    }
    L->rightSeg->segmentType = envSegmentTypeExponential;
    showGraph(env);
}

/**
 * @brief Remove the currently active node
 */
void EnvLibDrawingArea::removeNode()
{
    if (!activeNode || !activeNode->leftSeg || !activeNode->rightSeg) return;
    activeNode->leftSeg->rightNode = activeNode->rightSeg->rightNode;
    activeNode->rightSeg->rightNode->leftSeg = activeNode->leftSeg;
    delete activeNode->rightSeg;
    delete activeNode;
    activeNode = nullptr;

    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    MUtilities::modified();
    adjustBoundary(env);
    showGraph(env);
}

/**
 * @brief Actually move the active node during drag
 */
void EnvLibDrawingArea::moveNode()
{
    EnvelopeLibraryEntry* env = envelopeLibraryWindow->getActiveEnvelope();
    if (!env || !activeNode) return;
    MUtilities::modified();

    double lb = activeNode->leftSeg  ? activeNode->leftSeg->leftNode->x + 0.001 : 0.0;
    double rb = activeNode->rightSeg ? activeNode->rightSeg->rightNode->x - 0.001 : 1.0;

    if (!activeNode->leftSeg || !activeNode->rightSeg) {
        activeNode->y = qBound(0.0, mouseY, 10.0);
    } else {
        activeNode->x = qBound(lb, mouseX, rb);
        activeNode->y = qBound(0.0, mouseY, 10.0);
    }

    envelopeLibraryWindow->setEntries(
      QString::number(activeNode->x,'f',4),
      QString::number(activeNode->y,'f',4));

    adjustBoundary(env);
    showGraph(env);
}

/**
 * @brief Map true y → [0,1] for drawing
 * @param y  true y-coordinate
 * @return adjusted y-coordinate for drawing
 */
double EnvLibDrawingArea::getAdjustedY(double y) const
{
    return y/(upperY-lowerY) + qAbs(lowerY)/(upperY-lowerY);
}

/**
 * @brief Map [0,1] mouse y back to true y
 * @param y  mouse y-coordinate [0,1]
 * @return true y-coordinate
 */
double EnvLibDrawingArea::mouseAdjustY(double y) const
{
    return y*(upperY-lowerY) + lowerY;
} 