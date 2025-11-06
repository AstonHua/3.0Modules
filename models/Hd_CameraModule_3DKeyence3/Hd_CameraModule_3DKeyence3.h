#ifndef Hd_CameraModule_3DKeyence_H
#define Hd_CameraModule_3DKeyence_H
#pragma once
#include <QtCore/qglobal.h>
#include <QByteArray>
#include <QDebug>
#include <iostream>
#include <Windows.h>
#include "pbglobalobject.h"
#include "QQueue.h"
#include "QMap.h"
#include "QDir.h"
#include "qapplication.h"
#include "qjsonobject.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "QLibrary.h"
#include <qmutex.h>
#include "LJX8_IF.h"
#include "LJX8_ErrorCode.h"
#include "ThreadSafeQueue.h"
#include <struct.h>
#include <imageView.h>
const int MAX_LJXA_DEVICENUM = 6;
//#pragma comment(lib, "winmm.lib")
#pragma execution_character_set("utf-8")
using namespace std;
using namespace cv;
typedef struct {
    int 	y_linenum;
    float	y_pitch_um;
    int		timeout_ms;
    int		use_external_batchStart;
} LJXA_ACQ_SETPARAM;

typedef struct {
    int		luminance_enabled;
    int		x_pointnum;
    int		y_linenum_acquired;
    float	x_pitch_um;
    float	y_pitch_um;
    float	z_pitch_um;
} LJXA_ACQ_GETPARAM;



struct CallbackFuncPack
{
    QObject* callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString cameraIndex;
};
class cameraFunSDKfactoryCls :public QObject
{
    Q_OBJECT
public:
    cameraFunSDKfactoryCls(int DevicedID, QString rootPath,QObject* parent);
    ~cameraFunSDKfactoryCls();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);
    int  LJXA_ACQ_OpenDevice(int lDeviceId, LJX8IF_ETHERNET_CONFIG* EthernetConfig, int HighSpeedPortNo);
    void LJXA_ACQ_CloseDevice(int lDeviceId);
    bool InitHighSpeed();
    void upDateParam();
    bool run();
    int getTrigger() { return use_external_batchStart; }
public:
    QString RootPath;//家目录
    QObject* parent = nullptr;
    std::atomic_bool allowflag;

    ThreadSafeQueue<vector<cv::Mat>> ImageMats;//图像缓存队列

    QVector<CallbackFuncPack> CallbackFuncVec;
    LJX8IF_HIGH_SPEED_PRE_START_REQ* startReq_ptr = nullptr;
    LJX8IF_PROFILE_INFO* profileInfo_ptr = nullptr;

    // Static variable
    int Currentindex = 0;

    int deviceId = 0;			 // Set "0" if you use only 1 head.
    int xImageSize = 0;			 // Number of X points.
    int yImageSize = 0;			 // Number of Y lines.
    float y_pitch_um = 0;		 // Data pitch of Y data. (e.g. your encoder setting)
    int	timeout_ms = 0;		 // Timeout value for the acquiring image (in milisecond).
    int use_external_batchStart = 0; // Set "1" if you controll the batch start timing externally. (e.g. terminal input) 0内部触发，1外部触发
    unsigned short zUnit = 0;

    unsigned short* heightImage = nullptr;		    // Height image
    unsigned short* luminanceImage = nullptr;		// Luminance image

    LJXA_ACQ_SETPARAM* setParam_Ptr = nullptr;
    LJXA_ACQ_GETPARAM* getParam_Ptr = nullptr;
    int HighSpeedPortNo = 24692;		            // Port number for high-speed communication
    bool isopen = false;
    int errCode;

    LJX8IF_ETHERNET_CONFIG EthernetConfig;//当前id设备对应ip信息

    //QString camera_name;    // 相机名称
    //int	recontimeout_ms = 3000;

    QMap<QString, QString> ParasValueMap;
signals:
    void trigged(int);
};
class  Hd_CameraModule_3DKeyence3 :public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_3DKeyence3(int index,QString ip, QString rootPath,int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
    ~Hd_CameraModule_3DKeyence3();
    //#######################通用函数#######################
    //初始化参数；通信/相机的初始化参数
    bool setParameter(const QMap<QString, QString>&);

    //setParameter之后再调用，返回当前参数
    //相机：获取默认参数；
    //通信：获取初始化示例参数
    QMap<QString, QString> parameters();
    //初始化(加载模块到内存)
    bool init();

    //设置数据
    //相机：第一张图的参数，QStringList：00 曝光值 增益值；
    //相机：第N张图的参数，QStringList：非00 曝光值 增益值；
    //完成后发送信号trigged(bool);
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    //获取数据
    bool data(std::vector<cv::Mat>&, QStringList&);
    //注册回调 string对应自身的参数协议 （自定义）
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);

private:
    QString ip;
    QString JsonFile;
    QString RootPath;
    QString HeanSn;
    int deviceId = 0;
    QString SnName;
    cameraFunSDKfactoryCls* m_sdkFunc = nullptr;
    //参数设置的数据
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
    Hd_CameraModule_3DKeyence3* m_Camerahandle = nullptr;

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

#endif // HD_MVCAMERA_MODULE_H
