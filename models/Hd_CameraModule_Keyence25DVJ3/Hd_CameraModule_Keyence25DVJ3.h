#ifndef Hd_25DCameraVJ_module_H
#define Hd_25DCameraVJ_module_H

#define CameraTypeArea 1
#define CameraTypeLine 2
#define CameraTypeXT 3
#define CameraTypeRB 4
#include <QtCore/qglobal.h>
#include <Windows.h>
#include <time.h>

#include "KglDevice.h"
#include "KglStream.h"
#include "KglSystem.h"
#include "KglBuffer.h"
#include "KglDefine.h"
#include "KglResult.h"
#include "KglDeviceEventSink.h"
#include <pbglobalobject.h>
#include <ThreadSafeQueue.h>
#include <struct.h>
using namespace Keyence;

using namespace cv;
using namespace  std;
struct CallbackFuncPack
{
    QObject* callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString cameraIndex;
};
class cameraFunSDKfactoryCls;
class  Hd_25DCameraVJ_module :public PbGlobalObject
{
    Q_OBJECT
public:
    Hd_25DCameraVJ_module(QString snName , QString rootPath,int settype = -1, QObject* parent = nullptr);
    ~Hd_25DCameraVJ_module();
    bool setParameter(const QMap<QString, QString>&);
    QMap<QString, QString> parameters();
    //初始化(加载模块待内存)
    bool init();
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    //获取数据
    bool data(std::vector<cv::Mat>&, QStringList&);
    //注册回调 string对应自身的参数协议 （自定义）
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);


private://inside
    QString SnCode;
    QString RootPath;
    QString JsonFilePath;
private:
    QMap<QString, QString> ParasValueMap;
 
    cameraFunSDKfactoryCls* m_sdkFunc = nullptr;

private:

};
class LocalKglDeviceEventSink : public Keyence::KglDeviceEventSink
{
public:

    LocalKglDeviceEventSink();
    ~LocalKglDeviceEventSink();

    virtual void onLinkDisconnected(const KglDevice* pDevice);
    virtual void onEventGenICam(const KglDevice* pDevice,
        const uint16_t  eventID,
        const uint16_t  channel,
        const uint64_t  blockID,
        const uint64_t  timestamp,
        const uint32_t  nodenum,
        const KglGenParameter** pNode
    );

};

class cameraFunSDKfactoryCls :public QObject
{
    Q_OBJECT;
public:
    KglDevice* kglDevice = nullptr;
    KglStream* kglStream = nullptr;
    KglGenParameterArray* kgllFeatureNodes = nullptr;
    KglGenParameterArray* kgllStreamFeatureNodes = nullptr;
    LocalKglDeviceEventSink* ab = nullptr;
    KglBuffer* kglBuffer = nullptr;
public:
    cameraFunSDKfactoryCls(QString sn,QString RootPath);
    ~cameraFunSDKfactoryCls();
    void upDateParam();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);
    bool setParamMap(const QMap<QString, QString>& ParasValueMap);
    bool Connect(string name);
    void Disconnect();
    void getPictureThread();
    bool AcquisitionStartEx_SingleFrame(bool bMultiCaptureUpdateImage, std::string sMultiCaptureImageType, cv::Mat& result);
    void setFeatureNodes(const std::string sFeatureName, const int64_t value);
    void setIntegerValue(const std::string sFeatureName, const int64_t value);
    void setFloatValue(const std::string sFeatureName, const double value);
    void setBooleanValue(const std::string sFeatureName, const bool value);
    void setEnumValue(const std::string sFeatureName, std::string value);
    void setStringValue(const std::string sFeatureName, const std::string);
    void executeCommand(const std::string sFeatureName);
    void getIntegerValue(const std::string sFeatureName, int64_t& value);
    void getFloatValue(const std::string sFeatureName, double& value);
    void getBooleanValue(const std::string sFeatureName, bool& value);
    void getEnumValue(const std::string sFeatureName, std::string& value);
    void getStringValue(const std::string sFeatureName, std::string& value);
    void ImportDeviceParameters(std::string sFilePath);
    std::vector<std::string> GetEnableImageType(std::string sImageType);
    void ConvertRGBA8toBGRA8(void* buffer, int pixelSize);
    bool AcquisitionStart();
    void AcquisitionStop();
    bool QueueBuffer();
    cv::Mat RetrieveBuffer(std::string imagetype);
    void SaveBMP(cv::Mat bmp);
    bool TriggerSoftware();
    QVector<CallbackFuncPack> CallbackFuncVec;
public:
    int MaxTimeOut = 1000;
    QString RootPath;
    std::string CurrentsTriggerSource;
    int Currentindex = 0;
    int GetImageNums = 1;
    QMap<QString, QString> ParasValueMap;
    std::vector<std::string> sImageType = {};
    int iCameraType;
    std::string sModelName;
    std::string snName;
    std::shared_ptr<ThreadSafeQueue<std::vector<Mat>>> MatVecQueue;
    std::string xmlPath = "./CA-HL08MX_9E710040_Features.xml";
    std::thread getpicturethread;
	bool ThreadRunningflag = false;
    bool bBGEnable = false;
    std::atomic_bool allowflag;
    std::atomic_bool stopbit = false;
    bool isBufferCapturestart = false;
signals:
    void trigged(int);
};


//事件要在VJ软件端开启功能
extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path);
    Q_DECL_EXPORT void destroy(const QString& name);
    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
    Q_DECL_EXPORT QStringList getCameraSnList();
}
void GetCallbackMat(QObject*, const std::vector<cv::Mat>&);
class mPrivateWidget :public QWidget
{
    Q_OBJECT;
public:
    mPrivateWidget(void*);
    ~mPrivateWidget() {};
    void InitWidget();
    QPushButton* SetDataBtn;
    QPushButton* SetStopBtn;
    //ImageViewer* m_showimage;
    Hd_25DCameraVJ_module* m_Camerahandle = nullptr;
    QLabel* m_label;


};
#endif // Hd_25DCameraVJ_module_H
//Hd_25DCameraVJ_module
