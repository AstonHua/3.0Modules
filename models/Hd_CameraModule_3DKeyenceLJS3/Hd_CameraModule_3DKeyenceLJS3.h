#ifndef Hd_CameraModule_3DKeyenceLJS_H
#define Hd_CameraModule_3DKeyenceLJS_H
#pragma execution_character_set("utf-8")

#include <QtCore/qglobal.h>
#include <QByteArray>
#include <QDebug>
#include <QMap>
#include <QWidget>
#include <Windows.h>
#include <atomic>
#include <vector>

#include "pbglobalobject.h"
#include "LJS8_IF.h"
#include "LJS8_ErrorCode.h"
#include "ThreadSafeQueue.h"
#include <struct.h>
#include <AlgParm.h>

const int MAX_LJS8A_DEVICENUM = 6;
const int MAX_LJS8A_XDATANUM = 3200;

typedef struct {
    int timeout_ms;
    int useExternalTrigger;
    int usePcImageFilter;
} LJS8A_ACQ_SETPARAM;

typedef struct {
    int luminance_enabled;
    int x_pointnum;
    int y_linenum_acquired;
    float x_pitch_um;
    float y_pitch_um;
    float z_pitch_um;
    int isProcTimeoutOccurred;
} LJS8A_ACQ_GETPARAM;

struct CallbackFuncPackLJS
{
    QObject* callbackparent = nullptr;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc = nullptr;
    QString cameraIndex;
};

class LjsCameraFunSDKfactoryCls : public QObject
{
    Q_OBJECT
public:
    LjsCameraFunSDKfactoryCls(int DevicedID, QString rootPath, QObject* parent);
    ~LjsCameraFunSDKfactoryCls();

    bool initSdk(QMap<QString, QString>& insideValuesMaps);
    int LJS8A_ACQ_OpenDevice(int lDeviceId, LJS8IF_ETHERNET_CONFIG* EthernetConfig, int HighSpeedPortNo);
    void LJS8A_ACQ_CloseDevice(int lDeviceId);
    bool InitHighSpeed();
    void upDateParam();
    bool run();
    int getTrigger() const { return useExternalTrigger; }

public:
    QString RootPath;
    QObject* parent = nullptr;
    std::atomic_bool allowflag{ false };
    ThreadSafeQueue<std::vector<cv::Mat>> ImageMats;
    QMap<int, CallbackFuncPackLJS> CallbackFuncMap;

    int Currentindex = 0;
    int getImageMaxCoiunts = 1;
    int deviceId = 0;
    int xImageSize = 0;
    int yImageSize = 0;
    int timeout_ms = 0;
    int useExternalTrigger = 1;
    int usePcImageFilter = 1;
    int HighSpeedPortNo = 24692;
    bool isopen = false;
    int errCode = LJS8IF_RC_OK;

    LJS8A_ACQ_SETPARAM setParam{};
    LJS8A_ACQ_GETPARAM getParam{};
    LJS8IF_ETHERNET_CONFIG EthernetConfig{};
    LJS8IF_HIGH_SPEED_PRE_START_REQ startReq{};
    LJS8IF_HEIGHT_IMAGE_INFO heightImageInfo{};
    LJS8IF_PROFILE_HEADER profileHeader{};
    QMap<QString, QString> ParasValueMap;
    int OnceGetImageNum = 2;
signals:
    void trigged(int);
};

class Hd_CameraModule_3DKeyenceLJS3 : public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_3DKeyenceLJS3(int index, QString ip, QString rootPath, int settype = -1, QObject* parent = nullptr);
    ~Hd_CameraModule_3DKeyenceLJS3();

    bool setParameter(const QMap<QString, QString>&);
    QMap<QString, QString> parameters();
    bool init();
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    bool data(std::vector<cv::Mat>&, QStringList&);
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    QString GetRootPath() const { return RootPath; }
    QString GetSn() const { return SnName; }

private:
    QString ip;
    QString JsonFile;
    QString RootPath;
    int deviceId = 0;
    QString SnName;
    LjsCameraFunSDKfactoryCls* m_sdkFunc = nullptr;
    QMap<QString, QString> ParasValueMap;
};

class mPrivateWidget : public QWidget
{
    Q_OBJECT
public:
    mPrivateWidget(void*);
    ~mPrivateWidget() {};
    void InitWidget();
    QPushButton* SetDataBtn = nullptr;
    QPushButton* OpenGrapMat = nullptr;
    QPushButton* NotGrapMat = nullptr;
    ImageViewer* m_showimage = nullptr;
    AlgParmWidget* m_AlgParmWidget = nullptr;
    Hd_CameraModule_3DKeyenceLJS3* m_Camerahandle = nullptr;
};

extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path);
    Q_DECL_EXPORT void destroy(const QString& name);
    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
    Q_DECL_EXPORT QStringList getCameraSnList();
}

#endif // Hd_CameraModule_3DKeyenceLJS_H
