#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>
#include <QLabel>
#include <QToolTip>
#include <QJsonDocument>
//#include "grapshapeview.h"
#include <QDebug>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>

class InteractiveGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit InteractiveGraphicsView(QWidget* parent = nullptr);

    // 控制缩放因子
    void setZoomFactor(double factor);
    double zoomFactor() const { return m_zoom; }

protected:
    // 鼠标事件处理
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    // 滚轮缩放
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    // 缩放时保持鼠标下的点固定
    void scaleView(qreal scaleFactor);

private:
    void moveViewByKeyboard(int dx, int dy);
    void zoomByKeyboard(bool zoomIn);
    void centerOnCurrentItem();  // 居中当前项
    bool m_dragging = false;
    QPoint m_lastMousePos;
    const double zoomStep = 0.1; // 每次缩放增减 10%
    double m_zoom = 1.0;
    const double ZOOM_MIN = 0.1;  // 最小缩放
    const double ZOOM_MAX = 10.0; // 最大缩放

signals:
    void sendDoublickSignals();
};

class viewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit viewWidget(QWidget* parent = nullptr);
    explicit viewWidget(viewWidget* parent);
    ~viewWidget();
    void init();
    void setName(QString name);
    void setKey(QString key) { m_key = key; }
    void reciveImage(QString key, QImage&); // 传递图片
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == this && event->type() == QEvent::HoverEnter) {

            showNGInfo();
        }
        return QWidget::eventFilter(watched, event);
    }
    void clearData();
    QString getName() { return m_Name; }
    QString getKey() { return m_key; }
    QPixmap getImage();
    QLabel* getLabel() {
        return labptr.get();
    }
    void reciveResult(QString, QByteArray);
    //void  mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;   //鼠标点击事件关联
protected:
    void resizeEvent(QResizeEvent* event)override;
private:
    QGraphicsScene scene;
    void showNGInfo();
    QMap<QString, int> defcetMap;
    int NGCounts = 0;
    QString    m_key;
    QString    m_Name;
    QGraphicsPixmapItem* mainitem;
    QScopedPointer<QLabel>       labptr;
    QScopedPointer<QVBoxLayout>  layout_ptr;
    QScopedPointer<InteractiveGraphicsView>  view_ptr;
signals:
    void sendDoublickSignals(QString);
};

#endif // VIEWWIDGET_H
