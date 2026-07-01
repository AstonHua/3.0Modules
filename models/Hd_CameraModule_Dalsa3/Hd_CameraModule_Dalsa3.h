#ifndef HD_CAMERAMODULE_DALSA3_H
#define HD_CAMERAMODULE_DALSA3_H

#include <QtCore/qglobal.h>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLibrary>
#include <QMap>
#include <QMetaType>
#include <QMutex>
#include <QPushButton>
#include <QQueue>
#include <QTextCodec>
#include <QThread>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QDateTime>
#include <QGraphicsPixmapItem>

#include <Windows.h>
#include <time.h>

#include <opencv2/opencv.hpp>

#include "sapclassbasic.h"
#include "pbglobalobject.h"
#include "imageView.h"
#include "ThreadSafeQueue.h"
#include "struct.h"
#include "AlgParm.h"

#pragma execution_character_set("utf-8")

struct CallbackFuncPack
{
    QObject*            callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc;
    QString             cameraIndex;
};
class Hd_CameraModule_Dalsa3 : public PbGlobalObject
{
    Q_OBJECT
public:
    explicit Hd_CameraModule_Dalsa3(QString sn, QString path,
                                    int settype = -1, QObject* parent = nullptr);
    ~Hd_CameraModule_Dalsa3();

    /* ========== PbGlobalObject 虚函数实现 ========== */
    bool setParameter(const QMap<QString, QString>& paramMap);
    QMap<QString, QString> parameters();
    bool init();
    bool setData(const std::vector<cv::Mat>& mats, const QStringList& data);
    bool data(std::vector<cv::Mat>& imgs, QStringList& strList);
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent,
                             const QString& getString);
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent,
                           const QString& getString);

    /* ========== 属性访问 ========== */
    QString GetRootPath() const       { return RootPath; }
    QString GetSn() const             { return Sncode; }
    QString GetconfigFilename() const { return configFilename; }

    /* ========== 相机特征控制（Gain/Exposure/通用） ========== */

    /** @brief 设置 Gain 值（自动适配 INT32/DOUBLE 类型） */
    bool SetGain(double value);

    /** @brief 读取当前 Gain 值 */
    bool GetGain(double* pValue);

    /** @brief 获取 Gain 有效范围 */
    bool GetGainRange(double* pMin, double* pMax);

    /** @brief 设置曝光时间（微秒） */
    bool SetExposureTimeUs(double valueUs);

    /** @brief 读取当前曝光时间（微秒） */
    bool GetExposureTimeUs(double* pValueUs);

    /** @brief 获取曝光时间范围（微秒） */
    bool GetExposureTimeRange(double* pMinUs, double* pMaxUs);

    /** @brief 检查指定特征是否可用 */
    bool IsFeatureAvailable(const char* featureName);

    /** @brief 通用特征读（自动适配类型） */
    bool GetFeatureValue(const char* featureName, double* pValue);

    /** @brief 通用特征写（自动适配类型） */
    bool SetFeatureValue(const char* featureName, double value);

    /** @brief 获取整型特征值 */
    bool GetFeatureInt(const char* featureName, UINT32* pValue);

    /** @brief 设置整型特征值 */
    bool SetFeatureInt(const char* featureName, UINT32 value);

    // --- 成员变量 ---
    std::atomic_int  triggerMode{ 0 };
    QMap<int, CallbackFuncPack> CallbackFuncMap;
    int              Currentindex = 0;

    SapAcquisition   Acq;
    //SapAcqDevice     AcqDevice;
    SapBufferWithTrash Buffers;
    SapTransfer*     Xfer = nullptr;
    SapView          View;
    SapTransfer      AcqToBuf = SapAcqToBuf(&Acq, &Buffers);
    //SapTransfer      AcqDeviceToBuf = SapAcqDeviceToBuf(&AcqDevice, &Buffers);

    QString          Sncode;
    QString          RootPath;
    QString          JsonFilePath;
    QString          configFilename;
    QMap<QString, QString> ParasValueMap;
    int              timeOut = 1000;

    ThreadSafeQueue<QList<cv::Mat>> MatQueue;

    int              getImageMaxCoiunts = 1;
    int              OnceGetImageNum = 1;
    std::atomic_bool allowflag;

    bool             m_bExposureSequenceEnabled;
	bool             m_bGainSequenceEnabled;
    void             ApplyExposureByIndex();
    void             ApplyGainByIndex();
private:
    void __stdcall ReconnectDevice(unsigned int nMsgType, void* pUser0);
    bool CloseDevice();
    bool connctDevice(const std::string& getSnName, const std::string& cfgFilename);
    int     DeviceIndex;
    /* ---- 内部特征辅助 ---- */
    bool InitFeatureHelper();
    void CleanupFeatureHelper();

    /* ---- 内部特征缓存 ---- */
    SapAcqDevice*    m_pFeatureDevice;   /* 特征操控设备 */
    SapFeature*      m_pFeature;         /* 特征元数据 */
    bool             m_bGainAvailable;
    bool             m_bGainIsDouble;
    char             m_szGainName[64];
    double           m_dGainMin;
    double           m_dGainMax;
    bool             m_bExposureAvailable;
    double           m_dExposureMinUs;
    double           m_dExposureMaxUs;

    /* ---- 多曝光序列轮转 ---- */
 
    QMap<int, double> m_mapExposureSequence;
    QMap<int, double> m_mapGainSequence;
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
    Q_OBJECT;

public:
    explicit mPrivateWidget(void* handle);
    ~mPrivateWidget() {}
    void InitWidget();

    QPushButton*      SetDataBtn;
    QPushButton*      OpenGrapMat;
    QPushButton*      NotGrapMat;
    ImageViewer*      m_showimage;
    AlgParmWidget*    m_AlgParmWidget;
    Hd_CameraModule_Dalsa3* m_Camerahandle = nullptr;
};

#endif // HD_CAMERAMODULE_DALSA3_H
