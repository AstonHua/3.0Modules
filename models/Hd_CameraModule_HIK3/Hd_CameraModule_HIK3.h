#ifndef Hd_CameraModule_HIK3_H
#define Hd_CameraModule_HIK3_H

#include <QtCore/qglobal.h>
#include <MvCameraControl.h>
#include <opencv.hpp>
#include <QByteArray>
#include <iostream>
#include <Windows.h>
#include <time.h>
#include <QPushButton>
#include "pbglobalobject.h"
#include <ThreadSafeQueue.h>
#include <struct.h>
#include <AlgParm.h>
using namespace cv;
using namespace std;
#pragma execution_character_set("utf-8")
struct CallbackFuncPack
{
    QObject* callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString cameraIndex;
};
class CameraFunSDKfactoryCls : public QObject
{
    Q_OBJECT
public:
    explicit CameraFunSDKfactoryCls(QString Sn, QString path ,QObject* parent = nullptr)
		: QObject(parent), SnCode(Sn.toStdString()),RootPath(path) {}
    ~CameraFunSDKfactoryCls();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);
	void* getHandle() { return handle; }
	void upDateParam();
    void* handle = nullptr;//相机句柄
    ThreadSafeQueue<std::vector<cv::Mat>> MatQueue;
    QMap<int,CallbackFuncPack> CallbackFuncMap;
    std::atomic_bool allowflag;
    int Currentindex = 0;
    string Username;
    string SnCode;
	std::map<int, float> exposureTimeMap;
	std::map<int, float> gainMap;
	std::map<int, float> gammaMap;
	QString RootPath;
	QMap<QString, QString> ParasValueMap;
    int getImageMaxCoiunts = 1;//一次信号取图次数
    int OnceGetImageNum = 1;//一次取图出图数量
    int timeOut = 1000;
    MV_CAM_TRIGGER_SOURCE m_MV_CAM_TRIGGER_SOURCE;//触发方式
signals:
	void trigged(int);
private:
	void InitExposure_Gain_GamaMap();

};
class  Hd_CameraModule_HIK3 :public PbGlobalObject
{
    Q_OBJECT
public:
    //1、创建：赋值给famliy
    explicit Hd_CameraModule_HIK3(QString sn, QString path ,int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
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

    QString GetRootPath() const { return RootPath; }
    QString GetSn() const { return Sncode; }
    QString Sncode;
	QString RootPath;
	QString JsonFilePath;
    CameraFunSDKfactoryCls* m_sdkFunc =nullptr;
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

class mPrivateWidget :public QWidget
{
	Q_OBJECT;
public:
    mPrivateWidget(void*);
    ~mPrivateWidget() {};
    void InitWidget();
    QPushButton* SetDataBtn;
    QPushButton* OpenGrapMat;
    QPushButton* NotGrapMat;
    ImageViewer* m_showimage;
    //MyTableWidget* m_paramsTable;
    AlgParmWidget* m_AlgParmWidget;
	Hd_CameraModule_HIK3* m_Camerahandle = nullptr;

};
#endif // Hd_CameraModule_HIK3_H
