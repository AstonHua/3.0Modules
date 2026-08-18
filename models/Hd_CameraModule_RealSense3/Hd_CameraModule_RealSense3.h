#ifndef HD_CAMERAMODULE_REALSENSE3_H
#define HD_CAMERAMODULE_REALSENSE3_H

#include <QtCore/qglobal.h>
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <atomic>
#include <mutex>

#include "pbglobalobject.h"
#include <ThreadSafeQueue.h>
#include <struct.h>
#include <AlgParm.h>
#include <imageView.h>

using namespace cv;
using namespace std;
#pragma execution_character_set("utf-8")

struct CallbackFuncPack
{
    QObject*            callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString             cameraIndex;
};

class Hd_CameraModule_RealSense3 : public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_RealSense3(QString sn, QString path,
                                        int settype = -1, QObject* parent = nullptr);
    ~Hd_CameraModule_RealSense3();

    bool setParameter(const QMap<QString, QString>& paramMap);
    QMap<QString, QString> parameters();
    bool init();
    bool setData(const std::vector<cv::Mat>& mats, const QStringList& data);
    bool data(std::vector<cv::Mat>& imgs, QStringList& strList);
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent,
                             const QString& getString);
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent,
                           const QString& getString);

    QString GetRootPath() const { return JsonFilePath; }
    QString GetSn() const       { return SnCode; }
    bool    closeCamera();
    bool    checkStatus();

    void    upDateParam();
    bool    initSdk();
    void    onFrameCallback(const rs2::frame& frame);
    bool    applySensorPreset(const QString& presetName);

    QString         SnCode;
    QString         JsonFilePath;
    QMap<QString, QString> ParasValueMap;

    std::unique_ptr<rs2::pipeline> rsPipeline;
    std::unique_ptr<rs2::config>   rsConfig;

    ThreadSafeQueue<std::vector<cv::Mat>> MatQueue;
    QMap<int, CallbackFuncPack> CallbackFuncMap;

    int              triggedType = 1;
    int              getImageMaxCoiunts = 1;
    int              OnceGetImageNum = 2;
    int              timeOut = 3000;
    int              Currentindex = 0;
    std::atomic_bool allowflag{ true };
    std::atomic_bool isRunning{ false };
    std::atomic_bool m_pendingCapture{ false };
    QMutex           sdkMutex;

signals:
    void sendMats(cv::Mat);
};

extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name,
                              const QString& path);
    Q_DECL_EXPORT void destroy(const QString& name);
    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name);
    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name);
    Q_DECL_EXPORT QStringList getCameraSnList();
}

class mPrivateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit mPrivateWidget(void* handle);
    ~mPrivateWidget() {}
    void InitWidget();
    Hd_CameraModule_RealSense3* m_Camerahandle = nullptr;

private:
    QPushButton*   softTriggerBtn = nullptr;
    QPushButton*   allowGrabBtn = nullptr;
    QPushButton*   stopGrabBtn = nullptr;
    QComboBox*     presetCombo = nullptr;
    ImageViewer*   depthView = nullptr;
    ImageViewer*   colorView = nullptr;
    AlgParmWidget* m_AlgParmWidget = nullptr;
};

#endif
