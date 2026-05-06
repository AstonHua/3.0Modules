#ifndef Hd_Camera_Basler3_H
#define Hd_Camera_Basler3_H

#include <QtCore/qglobal.h>
#include <opencv2/opencv.hpp>
#include <QByteArray>
#include <iostream>
#include <Windows.h>
#include <time.h>

#include <QCheckBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QDebug>
#include <QDoubleValidator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <atomic>

// Basler Pylon SDK
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/ParameterIncludes.h>

// GenApi 事件相关
#include <GenApi/EventAdapter.h>
#include <GenApi/EventPort.h>
#include <GenApi/EventAdapterGEV.h>  // GigE Vision 事件
#include <GenApi/EventAdapterU3V.h>   // USB3 Vision 事件

// 项目公共头文件
#include "pbglobalobject.h"
#include <ThreadSafeQueue.h>
#include <struct.h>
#include <AlgParm.h>
#include "imageView.h"

// 使用命名空间
using namespace Pylon;
using namespace Basler_UniversalCameraParams;
using namespace GENAPI_NAMESPACE;
using namespace cv;
using namespace std;

#pragma execution_character_set("utf-8")

// 触发源定义（与海康保持一致）
#define BASLER_TRIGGER_SOURCE_ACTION 0
#define BASLER_TRIGGER_SOURCE_LINE1 1
#define BASLER_TRIGGER_SOURCE_LINE2 2
#define BASLER_TRIGGER_SOURCE_SOFTWARE 4

// 前置声明
struct CallbackFuncPack_Basler;
class CameraFunSDKfactoryCls;
class Hd_CameraModule_Basler3;
class mPrivateWidget_Basler;

// 回调函数包结构
struct CallbackFuncPack_Basler
{
    QObject* callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString cameraIndex;
};

class BaslerGrabHandler : public Pylon::CImageEventHandler
{
public:
    BaslerGrabHandler(CameraFunSDKfactoryCls* pParent) : m_pParent(pParent) {}

    // 重写 SDK 要求的纯虚函数
    virtual void OnImageGrabbed(Pylon::CInstantCamera& cam, const Pylon::CGrabResultPtr& ptrGrabResult) override;

private:
    CameraFunSDKfactoryCls* m_pParent;
};

// 相机功能工厂类
class CameraFunSDKfactoryCls : public QObject
{
    typedef std::function<void(cv::Mat&)> GetImageFun;
    Q_OBJECT

public:
    explicit CameraFunSDKfactoryCls(QString Sn, QString path, QObject* parent = nullptr);
    ~CameraFunSDKfactoryCls();

    bool initSdk(QMap<QString, QString>& insideValuesMaps);
    void upDateParam();
    bool setArrayByte(QString Key, QJsonArray array);

    // Basler相机特有方法
    bool connectCamera(string GetSnName);
    void disconnectCamera();
    bool setExposureTime(float value);
    bool setGain(int value);
    bool setGamma(float value);
    bool setTriggerMode(bool on);
    bool setTriggerSource(int source);
    bool setPixelFormat(const QString& format);
    bool setAcquisitionMode(const QString& mode);
    bool softwareTrigger();
    cv::Mat grabOneImage();
    void registerGetImageFun(GetImageFun fun) { triggerOffBack = fun; }
    void onImageGrabbed(CInstantCamera& cam, const CGrabResultPtr& ptrGrabResult);
    // 成员变量
    CBaslerUniversalInstantCamera* camera = nullptr;
    ThreadSafeQueue<std::vector<cv::Mat>> MatQueue;
    QMap<int, CallbackFuncPack_Basler> CallbackFuncMap;
    std::atomic_bool allowflag;
    int Currentindex = 0;
    string Username;
    string SnCode;
    std::map<int, float> exposureTimeMap;
    std::map<int, float> gainMap;
    std::map<int, float> gammaMap;
    QString RootPath;
    QMap<QString, QString> ParasValueMap;
    int getImageMaxCoiunts = 1;
    int OnceGetImageNum = 1;
    int timeOut = 1000;
    int m_triggerSource = BASLER_TRIGGER_SOURCE_SOFTWARE;
    std::atomic_int triggerMode{ 0 };

  
    // 相机参数范围
    double exposureMin = 0, exposureMax = 1000000;
    double gainMin = 0, gainMax = 24;
    double gammaMin = 0.1, gammaMax = 3.0;

    Pylon::CImageEventHandler* m_pGrabHandler = nullptr;
signals:
    void trigged(int);
    void imageGrabbed(cv::Mat img);

private slots:
    void checkCameraHealth();

private:
    GetImageFun triggerOffBack;
    QTimer* m_healthCheckTimer = nullptr;
    bool m_isConnected = false;
    // 图像转换
    cv::Mat convertToMat(const CGrabResultPtr& ptrGrabResult);

};

inline void BaslerGrabHandler::OnImageGrabbed(Pylon::CInstantCamera& cam, const Pylon::CGrabResultPtr& ptrGrabResult)
{
    m_pParent->onImageGrabbed(cam, ptrGrabResult);
}

// 主相机模块类
class Hd_CameraModule_Basler3 : public PbGlobalObject
{
    Q_OBJECT

public:
    explicit Hd_CameraModule_Basler3(QString sn, QString path, int settype = -1, QObject* parent = nullptr);
    ~Hd_CameraModule_Basler3();

    // 通用函数实现
    bool setParameter(const QMap<QString, QString>&) override;
    QMap<QString, QString> parameters() override;
    bool init() override;
    bool setData(const std::vector<cv::Mat>&, const QStringList&) override;
    bool data(std::vector<cv::Mat>&, QStringList&) override;
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&) override;
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&) override;

    QString GetRootPath() const { return RootPath; }
    QString GetSn() const { return Sncode; }

    QString Sncode;
    QString RootPath;
    QString JsonFilePath;
    CameraFunSDKfactoryCls* m_sdkFunc = nullptr;
    QMap<QString, QString> ParasValueMap;

signals:
    void sendMats(cv::Mat);
};

// 导出函数声明
extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path);
    Q_DECL_EXPORT void destroy(const QString& name);
    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
    Q_DECL_EXPORT QStringList getCameraSnList();
}

// 全局变量声明
struct OnePb_Basler
{
    PbGlobalObject* base = nullptr;
    QWidget* baseWidget = nullptr;
    QString DeviceSn;
};

// 工具函数声明
bool SearchBaslerDevice();
bool IsColorBasler(Pylon::EPixelType enType);

// 界面类声明
class mPrivateWidget : public QWidget
{
    Q_OBJECT;

public:
    mPrivateWidget(void* handle);
    ~mPrivateWidget() {};
    void InitWidget();
    Hd_CameraModule_Basler3* m_Camerahandle = nullptr;
    void getRes(QByteArray byte);

private slots:
    void showImage(cv::Mat& image);

private:
    void createConnect();
    void loadCurrentParams();
    void updateShowTable();

    QGridLayout* layout = nullptr;
    QJsonObject BytePtr;

    QPushButton* SetDataBtn = nullptr;
    QPushButton* ContinuesBtn = nullptr;
    QPushButton* Details = nullptr;

    QComboBox* first = nullptr;
    QComboBox* Second = nullptr;
    QComboBox* BalanceWhiteAuto = nullptr;
    QSpinBox* BalanceRatioR = nullptr;
    QSpinBox* BalanceRatioG = nullptr;
    QSpinBox* BalanceRatioB = nullptr;
    QComboBox* GamaDisable = nullptr;
    QSpinBox* Count = nullptr;
    QSpinBox* timeout = nullptr;
    QLineEdit* gain = nullptr;
    QLineEdit* Gama = nullptr;
    QLineEdit* Exposure = nullptr;
    viewWidget* m_showimage = nullptr;
    QPushButton* saveBtn = nullptr;

    // Basler特有控件
    QComboBox* pixelFormat = nullptr;
    QComboBox* acquisitionMode = nullptr;
    QLineEdit* frameRate = nullptr;
    QDoubleValidator* frameRateValidator = nullptr;

    MyTableWidget* gainTable = nullptr;
    MyTableWidget* GamaTable = nullptr;
    MyTableWidget* ExposureTimeTable = nullptr;

    QPushButton* Add = nullptr;
    QPushButton* Delete = nullptr;
    QPushButton* takeEffect = nullptr;
    QTableWidget* showTable = nullptr;
    AlgParmWidget* m_AlgParmWidget = nullptr;

    QDoubleValidator* doubleValidator1 = nullptr;
    QDoubleValidator* doubleValidator2 = nullptr;
    QDoubleValidator* doubleValidator3 = nullptr;

    double gainMin, gainMax;
    double ExposureMin, ExposureMax;
    double GamaMin, GamaMax;

    QMap<QPair<int, int>, QString> cellOriginalValues;

signals:
    void sendImage(QImage);
};

#endif // HD_CAMERAMODULE_BASLER3_H