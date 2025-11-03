#ifndef HD_3DCAMERALMI_MODULE_H
#define HD_3DCAMERALMI_MODULE_H
#include <QtCore/qglobal.h>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <GoSdk.h>
#include <QByteArray>
#include <windows.h>
#include "pbglobalobject.h"
#include "QQueue.h"
#include "QMap.h"
#include "QDir.h"
#include "qjsonobject.h"
#include "qjsonarray.h"
#include "qjsondocument.h"
#include "QLibrary.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib,"GoSdk.lib")
#pragma comment(lib,"kApi.lib")

using namespace cv;

#pragma region LMI

enum LmiStatus
{
    UnInit = 0,
    OK = 1,
    Grab = 2,
};

#define NM_TO_MM(VALUE) (((k64f)(VALUE))/1000000.0)
#define UM_TO_MM(VALUE) (((k64f)(VALUE))/1000.0)

#define INVALID_RANGE_16BIT     ((signed short)0x8000)          // gocator transmits range data as 16-bit signed integers. 0x8000 signifies invalid range data.
#define DOUBLE_MAX              ((k64f)1.7976931348623157e+308) // 64-bit double - largest positive value.
#define INVALID_RANGE_DOUBLE    ((k64f)-DOUBLE_MAX)             // floating point value to represent invalid range data.


struct MyTiff
{
    unsigned short* Tiff;
    int w;
    int h;
};

struct MyBmp
{
    unsigned char* Bmp;
    int w;
    int h;
};

struct DataContext
{
    k32u count;
};

struct myPBGLOBAL_callBack
{
    void (*callBackFun)(QObject*, const std::vector<cv::Mat>&);
    QObject* m_QObject;
    QString cameraIndex;
};

#pragma endregion


class CameraFunSDKfactoryCls : public QObject
{
    Q_OBJECT
public:
    explicit CameraFunSDKfactoryCls(QObject* parent = nullptr) : QObject(parent) {}
    void run(int);

    bool ifFirst = true;
    //参数设置的数据
    QMap<QString, QString> ParasValueMap;

    QByteArray mv_Status = "connected";//0106 新增相机断联报警
    long long imgIndex = 0;
    std::vector<GoDataMsg>DataSetCollection;
    std::mutex DataSetMutex;

    k32u k32u_id = 117599; //3D 序列号
    kAssembly api = kNULL;
    GoSystem system1 = kNULL;
    GoSensor sensor = kNULL;
    GoDataSet dataset = kNULL;
    DataContext* contextPointer = new DataContext();
    LmiStatus m_status = UnInit;
    bool m_lmiheightFlag = false;
    bool m_lmiluminanceFlag = false;
    bool moduleStatus = true;

    bool ifHardTrigger = true;//false软触发，true硬触发

    //data == "rest_newProductIn"时要结束run；机台报警，取图超时，复位后重新拍的情况。
    bool ifBreak = false;
    //ifRunning减少ifBreak的判断；"rest_newProductIn"时，Logic_camera_thread框架会结束未处理完的相相机取图
    bool ifRunning = false;

    //QQueue<std::vector<cv::Mat>> queuePic;
    QMutex* m_mutex;
    //static Mat heightMat1, heightMat2;//高度图
    //static Mat luminanceMat1, luminanceMat2;//灰度图

    QQueue< cv::Mat> heightMatS;//高度图
    QQueue<cv::Mat> luminanceMatS;//灰度图

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
    bool m_flag = false;
    int callBackNum = 0;
    QQueue< myPBGLOBAL_callBack> QQueue_myPBGLOBAL_callBack;
    //模块名称
    QString moduleName;
};

class Hd_CameraModule_3DLMI3 :public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_3DLMI3(int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
    ~Hd_CameraModule_3DLMI3();
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
    virtual void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);

    //从类中读数据到实例对象
    bool Hd_CameraModule_3DLMI3::readData(std::vector<cv::Mat>& vecPic, QByteArray& data);
    //实例对象把数据写入到类
    bool  writeData(std::vector<cv::Mat>&,QByteArray& qbyte);
    bool  initParas(QByteArray&);
    bool  run();
    bool  checkStatus();
    QJsonObject load_camera_Example();
    void threadCheckState(); //线程检查相机状态
    CameraFunSDKfactoryCls* m_sdkFunc;

};
extern "C"
{
   //Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path);
   //Q_DECL_EXPORT void destroy(const QString& name);
   //Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
   //Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
   //Q_DECL_EXPORT QStringList getCameraSnList();
    //Q_DECL_EXPORT Hd_25DCameraVJ_module * create(int type = -1);
    //Q_DECL_EXPORT void destory(Hd_25DCameraVJ_module * ptr);
}
extern "C"
{
    Q_DECL_EXPORT Hd_CameraModule_3DLMI3* create(int type = -1);
    Q_DECL_EXPORT void destory(Hd_CameraModule_3DLMI3* ptr);
}


#endif // HD_3DCAMERALMI_MODULE_H
