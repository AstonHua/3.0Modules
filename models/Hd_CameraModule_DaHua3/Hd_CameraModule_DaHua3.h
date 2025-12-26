#ifndef Hd_CameraModule_DaHua3_H
#define Hd_CameraModule_DaHua3_H

#include <QtCore/qglobal.h>

#include <QByteArray>
#include <iostream>
#include <Windows.h>
#include <QDateTime>
#include <ThreadSafeQueue.h>
#include <pbglobalobject.h>
//#pragma comment(lib, "MVSDKmd.lib")


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "IMVApi.h"
#include <time.h>
#include <struct.h>
using namespace cv;
using namespace  std;

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
    explicit CameraFunSDKfactoryCls(QString Sn, QString path, QObject* parent = nullptr);
    ~CameraFunSDKfactoryCls();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);
    void* getHandle() const { return devHandle; }
    void upDateParam() {};
    IMV_HANDLE devHandle = NULL;//相机句柄
    ThreadSafeQueue<cv::Mat> MatQueue;
    QVector<CallbackFuncPack> CallbackFuncVec;
    std::atomic_bool allowflag;
    int Currentindex = 0;
    string Username;
    string SnCode;
    QString RootPath;
    QMap<QString, QString> ParasValueMap;
    _IMV_String triggerType;//0,SoftWare;2,Line1;3,Line2;9,Line1andLine2
    //MV_CAM_TRIGGER_SOURCE m_MV_CAM_TRIGGER_SOURCE;//触发方式
signals:
    void trigged(int);

};
class  Hd_CameraModule_DaHua3 :public PbGlobalObject
{
public:
    Hd_CameraModule_DaHua3(QString sn, QString path, int settype = -1, QObject* parent = nullptr);
     ~Hd_CameraModule_DaHua3();
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

private:
    QString Sncode;
    QString RootPath;
    QString JsonFilePath;
    shared_ptr<CameraFunSDKfactoryCls> m_sdkFunc;
    QMap<QString, QString> ParasValueMap;
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
    Hd_CameraModule_DaHua3* m_Camerahandle = nullptr;

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
#endif // Hd_CameraModule_DaHua3_H
