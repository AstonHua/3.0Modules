#ifndef Hd_CameraModule_3DKeyence_H
#define Hd_CameraModule_3DKeyence_H

#include <QtCore/qglobal.h>
#include <QByteArray>
#include <QDebug>
#include<iostream>
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

#include "LJX8_IF.h"
#include "LJX8_ErrorCode.h"

#pragma comment(lib, "winmm.lib")
#pragma execution_character_set("utf-8")

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

const int MAX_LJXA_DEVICENUM = 6;

class  Hd_CameraModule_3DKeyence3 :public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_3DKeyence3(int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
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

    //加载示例参数
    QJsonObject load_camera_Example();

    //设置数据
    //相机：第一张图的参数，QStringList：00 曝光值 增益值；
    //相机：第N张图的参数，QStringList：非00 曝光值 增益值；
    //完成后发送信号trigged(bool);
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    //获取数据
    bool data(std::vector<cv::Mat>&, QStringList&);
    //注册回调 string对应自身的参数协议 （自定义）
    virtual void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);


    //从类中读数据到实例对象
    bool readData(std::vector<cv::Mat>& mats, QByteArray& data);
    //实例对象把数据写入到类
    bool writeData(std::vector<cv::Mat>& mat, QByteArray& data);
    bool  initParas(QByteArray&);
    bool  run();

    int  LJXA_ACQ_OpenDevice(int lDeviceId, LJX8IF_ETHERNET_CONFIG* EthernetConfig, int HighSpeedPortNo);
    int  LJXA_ACQ_StartAsync(int lDeviceId, LJXA_ACQ_SETPARAM* setParam);
    int  LJXA_ACQ_AcquireAsync(int lDeviceId, unsigned short* heightImage, unsigned short* luminanceImage, LJXA_ACQ_SETPARAM* setParam, LJXA_ACQ_GETPARAM* getParam);
    void LJXA_ACQ_CloseDevice(int lDeviceId);

    bool decodeInitData(QByteArray byte,QVector<QByteArray> &vector_data);
    bool InitHighSpeed();
private:
    int LJXA_ACQ_Acquire(int lDeviceId, unsigned short* heightImage, unsigned short* luminanceImage, LJXA_ACQ_SETPARAM* setParam, LJXA_ACQ_GETPARAM* getParam);
    LJX8IF_HIGH_SPEED_PRE_START_REQ  *startReq_ptr    = nullptr;
    LJX8IF_PROFILE_INFO              *profileInfo_ptr = nullptr;

    // Static variable
    LJX8IF_ETHERNET_CONFIG _ethernetConfig[MAX_LJXA_DEVICENUM];
    int _highSpeedPortNo[MAX_LJXA_DEVICENUM];

    LJXA_ACQ_GETPARAM _getParam[MAX_LJXA_DEVICENUM];

    int deviceId   = 0;			 // Set "0" if you use only 1 head.
    int xImageSize = 0;			 // Number of X points.
    int yImageSize = 0;			 // Number of Y lines.
    float y_pitch_um = 0;		 // Data pitch of Y data. (e.g. your encoder setting)
    int	timeout_ms   = 0;		 // Timeout value for the acquiring image (in milisecond).
    int use_external_batchStart = 1; // Set "1" if you controll the batch start timing externally. (e.g. terminal input) 0内部触发，1外部触发
    std::string outputFilePath1 = "sample_height.csv";
    std::string outputFilePath2 = "sample_height.tif";
    std::string outputFilePath3 = "sample_luminance.tif";

    //std::vector< cv::Mat>  vector_mat;
    unsigned short zUnit = 0;

    unsigned short* heightImage = nullptr;		    // Height image
    unsigned short* luminanceImage = nullptr;		// Luminance image

    LJXA_ACQ_SETPARAM *setParam_Ptr = nullptr;
    LJXA_ACQ_GETPARAM *getParam_Ptr = nullptr;
    int HighSpeedPortNo = 24692;		            // Port number for high-speed communication
    bool isopen = false;
    int errCode;

    LJX8IF_ETHERNET_CONFIG EthernetConfig;

    QString camera_name;    // 相机名称
    int	recontimeout_ms = 3000; 
private:
    bool ifFirst = true;
    //参数设置的数据
    QMap<QString, QString> ParasValueMap;
    QString getmoduleName();
};

extern "C"
{
   Q_DECL_EXPORT Hd_CameraModule_3DKeyence3 * create(int type = -1);
   Q_DECL_EXPORT void destory(Hd_CameraModule_3DKeyence3 * ptr);
}

#endif // HD_MVCAMERA_MODULE_H
