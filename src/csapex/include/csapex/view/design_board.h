#ifndef DESIGN_BOARD_H
#define DESIGN_BOARD_H

/// COMPONENT
#include <csapex/csapex_fwd.h>

/// SYSTEM
#include <QWidget>
#include <QScrollArea>

/// FORWARD DECLARATIONS
namespace Ui
{
class DesignBoard;
}

namespace csapex
{

class DesignBoard : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString class READ cssClass)

public:
    DesignBoard(CommandDispatcher *dispatcher, DragIO& dragio_, QWidget* parent = 0);
    virtual ~DesignBoard();

    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent* e);

    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* e);

    void wheelEvent(QWheelEvent * e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

    void focusInEvent(QFocusEvent* e);
    void focusOutEvent(QFocusEvent* e);

    virtual bool eventFilter(QObject* o, QEvent* e);
    void dragEnterEvent(QDragEnterEvent* e);
    void dragMoveEvent(QDragMoveEvent* e);
    void dropEvent(QDropEvent* e);
    void dragLeaveEvent(QDragLeaveEvent * e);

    void enterEvent(QEvent * e);

    QString cssClass() {
        return QString("DesignBoard");
    }
    void enableGrid(bool);

    void scrollBy(int dx, int dy);
    void setView(int sx, int sy);

public Q_SLOTS:
    void updateCursor();
    void showBoxDialog();
    void showContextMenuGlobal(const QPoint& pos);
    void showContextMenu(const QPoint& pos);
    void showContextMenuEditBox(Box* box, const QPoint& pos);
    void showContextMenuAddNode(const QPoint& global_pos);
    void findMinSize(Box* box);

    void addBoxEvent(Box* box);
    void removeBoxEvent(Box* box);
    void refresh();
    void reset();
    void setSpace(bool);

private:
    void findParentScroll();

private:
    Ui::DesignBoard* ui;

    CommandDispatcher* dispatcher_;
    Overlay* overlay;
    DragIO& drag_io_;

    bool space_;
    bool drag_;
    QPoint drag_start_pos_;

    QScrollArea* parent_scroll;

    int initial_pos_x_;
    int initial_pos_y_;
};

}
#endif // DESIGN_BOARD_H
