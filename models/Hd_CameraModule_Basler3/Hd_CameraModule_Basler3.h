#ifndef Hd_CameraModule_Basler_LineSweep_H
#define Hd_CameraModule_Basler_LineSweep_H
#include <QtCore/qglobal.h>
#include <Windows.h>
#include <QDebug>
#include <QQueue>
#include <QMutex>
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/gige/GigETransportLayer.h>
//#include <pylon/InstantCamera.h>
//#include <pylon/InstantInterface.h>
//#include <pylon/gige/BaslerGigECamera.h>
#include <opencv2/opencv.hpp>

#include "pbglobalobject.h"

using namespace cv;
using namespace std;
using namespace Pylon;
using namespace Basler_UniversalCameraParams;

#pragma execution_character_set("utf-8")

struct myPBGLOBAL_callBack
{
    void (*callBackFun)(QObject*, const std::vector<cv::Mat>&);
    QObject* m_QObject;
    QString cameraIndex;
};
class  Hd_CameraModule_Basler3 :public PbGlobalObject
    , public Pylon::CImageEventHandler             // Allows you to get notified about images received and grab errors.
    , public Pylon::CConfigurationEventHandler     // Allows you to get notified about device removal.
    , public Pylon::CCameraEventHandler
    //, public CInstantInterface
{
public:
    //1、创建：赋值给famliy
    explicit Hd_CameraModule_Basler3(int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
    ~Hd_CameraModule_Basler3();
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
    //注销回调 string对应自身的参数协议 （自定义）--->注销后还得取消连接状态
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);


    //从类中读数据到实例对象
    bool  readData(std::vector<cv::Mat>& mat, QByteArray& data);
    //实例对象把数据写入到类
    bool  writeData(std::vector<cv::Mat>& mat, QByteArray& data);
    bool  initParas(QByteArray&);
    bool  run();
    bool  checkStatus();

    int SearchDevice();

    //关闭设备
    void CloseDevice();
    //连接设备
    bool ConnctDevice();
    //重连设备
    bool reConnctDevice();
    //回调函数？
    virtual void OnImageGrabbed(Pylon::CInstantCamera& camera, const Pylon::CGrabResultPtr& grabResult);
    //断线回调
    void OnCameraDeviceRemoved(Pylon::CInstantCamera& camera);
    //错误
    void OnGrabError(Pylon::CInstantCamera& camera, const char* errorMessage);
    //设置曝光
    void SetExposureTime(float exposureValue);
    //设置增益
    void SetGain(float GainValue);
private:
    //MyThread *m_thread = nullptr;
    QMutex* m_mutex;

    Pylon::CBaslerUniversalInstantCamera m_camera;
    bool ifInit = false;

    bool ifFirst = true;
    //参数设置的数据
    QMap<QString, QString> m_ParasValueMap;
    QString moduleName = ""; 
    QQueue< myPBGLOBAL_callBack> QQueue_myPBGLOBAL_callBack; //回调队列

    //全局变量
    QByteArray mv_Status = "connected";//0106 新增相机断联报警
    //void* handle;
    string Username = "";
    bool m_flag = false;
    bool moduleStatus = true;
    int index = -1;
    //data == "rest_newProductIn"时要结束run；机台报警，取图超时，复位后重新拍的情况。
    bool ifBreak = false;
    //ifRunning减少ifBreak的判断；"rest_newProductIn"时，Logic_camera_thread框架会结束未处理完的相相机取图
    bool ifRunning = false;

    QQueue<cv::Mat> queuepic;
    Pylon::DeviceInfoList_t m_devices;

    int imgIndex = 0;
    const size_t c_maxCamerasToUse = 2;
    // Smart pointer to camera features
    Pylon::IIntegerEx& GetExposureTime();
    Pylon::IIntegerEx& GetGain();
    Pylon::CIntegerParameter m_exposureTime; //曝光
    Pylon::CIntegerParameter m_gain; //增益
    // The grab result retrieved from the camera.
    Pylon::CGrabResultPtr m_ptrGrabResult;
    bool m_invertImage = true;
    // The grab result as a windows DIB to be displayed on the screen.
    Pylon::CPylonBitmapImage m_bitmapImage;
    bool ifGetImage = false;

    bool ifmoduleRun = true;

    QMutex* n_mutex = nullptr;// new QMutex();
    //是否为标准通信流程：
    //标准通信流程，run函数设置曝光/增益；非标准通信流程（简单通信流程），回调函数设置曝光/增益
    bool ifStandard = false;
    //相机取图超时时间
    int getImageTimeOut = 1000;
    //总取图数
    int getImageNum = 0;
    //曝光值列表
    QQueue<float> expourseTimeList;
    //增益列表
    QQueue<float> gainList;
    //当前触发方式
    Basler_UniversalCameraParams::TriggerSourceEnums currentTriggerSource = Basler_UniversalCameraParams::TriggerSource_Software;
    //初始化时的触发方式
    Basler_UniversalCameraParams::TriggerSourceEnums initTriggerSource = Basler_UniversalCameraParams::TriggerSource_Software;
    bool ifChangeExpourseAndGain = false;
};

extern "C"
{   
    Q_DECL_EXPORT Hd_CameraModule_Basler3* create(int type = -1);
    Q_DECL_EXPORT void destory(Hd_CameraModule_Basler3* ptr);
}

#endif // Hd_CameraModule_Basler_LineSweep_H
//Hd_CameraModule_Basler3
