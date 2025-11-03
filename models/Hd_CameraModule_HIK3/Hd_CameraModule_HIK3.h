#ifndef Hd_CameraModule_HIK3_H
#define Hd_CameraModule_HIK3_H

#include <QtCore/qglobal.h>

#include<opencv2/opencv.hpp>
#include <MvCameraControl.h>
#include <opencv.hpp>
#include <QByteArray>
#include <QDebug>
#include<iostream>
#include <Windows.h>
#include <time.h>
#include <QThread>
#include <QtConcurrent/qtconcurrent>
#include <qfuture.h>
#include "qjsonobject.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "QLibrary.h"
#include "QMetaType.h"
#include "QDateTime.h"
#include "QMutex.h"
#include "QQueue.h"
#include "QMap.h"
#include "QDir.h"
#include <QJsonParseError>
#include <QWidget>
#include <QTextCodec>
#include <QPushButton>
#include <imageView.h>
#include <QGraphicsPixmapItem>

#include "pbglobalobject.h"
#include <ThreadSafeQueue.h>
//#include <mPrivateWidget.h>
using namespace cv;
using namespace std;
#pragma execution_character_set("utf-8")
struct CallbackFuncPack
{
    QObject* callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString cameraIndex;
};
class cameraFunSDKfactoryCls : public QObject
{
    Q_OBJECT
public:
    explicit cameraFunSDKfactoryCls(QString Sn, QObject* parent = nullptr) : QObject(parent), SnCode(Sn.toStdString()) {}
    ~cameraFunSDKfactoryCls();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);
	void* getHandle() { return handle; }
    void* handle = nullptr;//相机句柄
    ThreadSafeQueue<cv::Mat> MatQueue;
    QVector<CallbackFuncPack> CallbackFuncVec;
    std::atomic_bool allowflag;
    int Currentindex = 0;
    string Username;
    string SnCode;
    MV_CAM_TRIGGER_SOURCE m_MV_CAM_TRIGGER_SOURCE;//触发方式
signals:
	void trigged(int);

};
class  Hd_CameraModule_HIK3 :public PbGlobalObject
{
    Q_OBJECT
public:
    //1、创建：赋值给famliy
    explicit Hd_CameraModule_HIK3(QString sn,int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
    ~Hd_CameraModule_HIK3();
    //#######################通用函数#######################
    bool setParameter(const QMap<QString, QString>&);
    QMap<QString, QString> parameters();
    //初始化(加载模块到内存)
    bool init();
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    //获取数据
    bool data(std::vector<cv::Mat>&, QStringList&);
    //注册回调 string对应自身的参数协议 （自定义）
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    //注销回调 string对应自身的参数协议 （自定义）--->注销后还得取消连接状态
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    int SearchDevice();

    QJsonObject load_camera_Example();
    QString getmoduleName();
    QString Sncode;
    cameraFunSDKfactoryCls* m_sdkFunc =nullptr;
    QMap<QString, QString> ParasValueMap;
signals:
    void sendMats(cv::Mat);

};


extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path);
    Q_DECL_EXPORT void destroy(const QString& name);
    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
    Q_DECL_EXPORT QStringList getCameraSnList();
    //Q_DECL_EXPORT Hd_25DCameraVJ_module * create(int type = -1);
    //Q_DECL_EXPORT void destory(Hd_25DCameraVJ_module * ptr);
}
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

class mPrivateWidget :public QWidget
{
	Q_OBJECT;
public:
	mPrivateWidget(void*);
	~mPrivateWidget() {};
	void InitWidget();
	QPushButton* SetDataBtn;
	ImageViewer* m_showimage;
	Hd_CameraModule_HIK3* m_Camerahandle = nullptr;

};
#endif // Hd_CameraModule_HIK3_H
