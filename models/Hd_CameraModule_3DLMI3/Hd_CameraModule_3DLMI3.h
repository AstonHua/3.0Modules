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
#include <QWidget>
#include <ThreadSafeQueue.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib,"GoSdk.lib")
#pragma comment(lib,"kApi.lib")

using namespace cv;
using namespace std;
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

using DataContext = k32u ;

struct CallbackFuncPack
{
    QObject* callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString cameraIndex;
};
#pragma endregion
class GoSystemOnceExplem
{
public:
    static GoSystemOnceExplem* getInstance();
    static void destroyInstance();
    kAssembly getkAssembly() { return api; }
    GoSystem getsystem1() { return system1; }
    ~GoSystemOnceExplem() {
        if (api)delete api;
        if (system1)delete system1;
    };
private:
    explicit GoSystemOnceExplem(QObject* parent = nullptr) {
        kStatus status;
        if ((status = GoSdk_Construct(&api)) != kOK)
        {
            //printf("Error: GoSdk_Construct:%d\n", status);
            qDebug() << "[Error] " << "Error: GoSdk_Construct:%d\n", status;
            return ;
        }
        //system1 = this;
        if ((status = GoSystem_Construct(&system1, kNULL)) != kOK)
        {
            //printf("Error: GoSystem_Construct:%d\n", status);
            qDebug() << "[Error] " << "Error: GoSystem_Construct:%d\n", status;
            return ;
        }
    };
   

    // 禁止拷贝和赋值
    GoSystemOnceExplem(const GoSystemOnceExplem&) = delete;
    GoSystemOnceExplem& operator=(const GoSystemOnceExplem&) = delete;
   
    static QMutex mutex;
    static QScopedPointer<GoSystemOnceExplem> instance;
private:
    kAssembly api = kNULL;
    GoSystem system1 = kNULL;
};

class CameraFunSDKfactoryCls : public QObject
{
    Q_OBJECT
public:
    explicit CameraFunSDKfactoryCls(QString sn, QString path, QObject* parent = nullptr);
    void upDateParam();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);

    std::atomic_bool allowflag;//允许接收图像
    QString RootPath;//家目录
    QObject* parent = nullptr;
    k32u k32u_id = 117599; //3D 序列号
    QMap<QString, QString> ParasValueMap;
    ushort Currentindex = 0;
    GoSystem system1 = kNULL;
    GoSensor sensor = kNULL;
    GoDataSet dataset = kNULL;
    DataContext* contextPointer = new DataContext();
    LmiStatus m_status = UnInit;
    QVector<CallbackFuncPack> CallbackFuncVec;
    ThreadSafeQueue<vector<cv::Mat>> ImageMats;//图像缓存队列

    int  triggedType = 0;
signals:
    void trigged(int);
};

class Hd_CameraModule_3DLMI3 :public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_3DLMI3(QString Sncode ,QString Path,int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
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
    //完成后发送信号trigged(bool);
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    //获取数据
    bool data(std::vector<cv::Mat>&, QStringList&);
    //注册回调 string对应自身的参数协议 （自定义）
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);

    bool  checkStatus();
    void threadCheckState(); //线程检查相机状态
    CameraFunSDKfactoryCls* m_sdkFunc;
    QMap<QString, QString> ParasValueMap;
private:
    QString JsonFile;
    QString RootPath;
    QString SnName;
};
extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path);
    Q_DECL_EXPORT void destroy(const QString& name);
    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
    Q_DECL_EXPORT QStringList getCameraSnList();
}

#endif // HD_3DCAMERALMI_MODULE_H
