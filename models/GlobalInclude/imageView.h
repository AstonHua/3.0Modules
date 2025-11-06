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
#include <QPushButton>
#include <opencv.hpp>
#include <QGraphicsPixmapItem>

class ImageViewer : public QGraphicsView {
public:
	ImageViewer(QWidget* parent = nullptr) : QGraphicsView(parent), scaleFactor(1.0) {
		setDragMode(QGraphicsView::ScrollHandDrag);
		setRenderHint(QPainter::Antialiasing, true);
		setRenderHint(QPainter::SmoothPixmapTransform, true);
	}

	void loadImage(const QString& imagePath) {
		QPixmap pixmap(imagePath);
		if (!pixmap.isNull()) {
			scene.clear();
			QGraphicsPixmapItem* temp = scene.addPixmap(pixmap);
			setScene(&scene);
			setSceneRect(pixmap.rect());
			resetTransform();
			this->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);
			scaleFactor = 1.0;
		}
	} void loadImage(const QPixmap& pixmap) {
		// QPixmap pixmap(imagePath);
		if (!pixmap.isNull()) {
			scene.clear();
			QGraphicsPixmapItem* temp = scene.addPixmap(pixmap);
			setScene(&scene);
			setSceneRect(pixmap.rect());
			resetTransform();
			this->fitInView(scene.itemsBoundingRect(), Qt::KeepAspectRatio);
			scaleFactor = 1.0;
		}
	}
	void Clear() { scene.clear(); }
	void GetImage(QImage& image)
	{
		for (QGraphicsItem* item : scene.items()) {
			if (auto pixItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(item)) {
				image = pixItem->pixmap().toImage();
				break;
			}
		}
	}
	void GetPoingToScale(QPointF, float);
protected:
	void wheelEvent(QWheelEvent* event) override {
		const double scaleFactorIncrement = 1.15;
		if (event->angleDelta().y() > 0) {
			scale(scaleFactorIncrement, scaleFactorIncrement);
			scaleFactor *= scaleFactorIncrement;
		}
		else {
			scale(1.0 / scaleFactorIncrement, 1.0 / scaleFactorIncrement);
			scaleFactor /= scaleFactorIncrement;
		}
	}

private:
	QGraphicsScene scene;
	double scaleFactor;
};
#endif // VIEWWIDGET_H
