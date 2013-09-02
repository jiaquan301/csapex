#ifndef QINTERACTIVE_RECT_H
#define QINTERACTIVE_RECT_H
/// SYSTEM
#include <QRectF>
#include <QPen>
#include <QGraphicsRectItem>
/// DECLARATIONS
class QGraphicsSceneMouseEvent;
class QStyleOptionGraphicsItem;

class QInteractiveItem : public QGraphicsRectItem {
public:
    QInteractiveItem  (const QRectF &rect);
    void setPen       (const QPen &pen);
    void setSelectPen (const QPen &pen);
    void setClass     (const int classID_);
    int  getClass();
    void setLimitedPos(const QPointF& next_pos);

protected:
    enum Interaction{INIT, NONE, MOVING, STOPPED};

    void mousePressEvent  (QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent   (QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    int                 classID_;
    QPen                pen_;
    QPen                pen_selected_;
    Interaction         interaction_;

    QPointF             drag_start_rect_pos_;
    QPointF             drag_start_mouse_pos_;
};

#endif // QINTERACTIVE_RECT_H
