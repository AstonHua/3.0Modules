#ifndef Hd_CameraModule_HIK3_H
#define Hd_CameraModule_HIK3_H

#include <QtCore/qglobal.h>
#include <MvCameraControl.h>
#include <opencv.hpp>
#include <QByteArray>
#include <iostream>
#include <Windows.h>
#include <time.h>
#include <QPushButton>
#include <QPointer>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
#include "pbglobalobject.h"
#include <ThreadSafeQueue.h>
#include <struct.h>
#include <AlgParm.h>
#include <dmessagebox.h>
using namespace cv;
using namespace std;
#pragma execution_character_set("utf-8")
struct CallbackFuncPack
{
    QPointer<QObject> callbackparent;
    PBGLOBAL_CALLBACK_FUN GetimagescallbackFunc = nullptr;
    QString cameraIndex;
};
class CameraFunSDKfactoryCls : public QObject
{
    typedef std::function<void(cv::Mat&)> GetImageFun;
    Q_OBJECT
public:
    explicit CameraFunSDKfactoryCls(QString Sn, QString path ,QObject* parent = nullptr)
		: QObject(parent), SnCode(Sn.toStdString()),RootPath(path) {}
    ~CameraFunSDKfactoryCls();
    bool initSdk(QMap<QString, QString>& insideValuesMaps);
	void* getHandle() { return handle; }
	void upDateParam();
    bool setArrayByte(QString Key, QJsonArray);
    void* handle = nullptr;//相机句柄
    ThreadSafeQueue<QList<cv::Mat>> MatQueue;
    QMap<int,CallbackFuncPack> CallbackFuncMap;
    struct AsyncCallbackTask
    {
        int cameraIndex = 0;
        quint64 generation = 0;
        void* handleSnapshot = nullptr;
        MV_FRAME_OUT_INFO_EX frameInfo = {};
        QByteArray rawFrameData;
    };
    std::mutex CallbackFuncMapMutex;
    std::mutex AsyncCallbackMutex;
    std::condition_variable AsyncCallbackCv;
    std::deque<AsyncCallbackTask> AsyncCallbackQueue;
    std::thread AsyncCallbackWorker;
    std::atomic_bool AsyncCallbackRunning{false};
    std::atomic<quint64> AsyncCallbackGeneration{0};
    void startAsyncCallbackWorker();
    void stopAsyncCallbackWorker();
    void enqueueAsyncCallback(int cameraIndex, quint64 generation, void* handleSnapshot, const MV_FRAME_OUT_INFO_EX& frameInfo, const QByteArray& rawFrameData);
    void asyncCallbackLoop();
    std::atomic_bool allowflag{true};
     int IntNumEvent = 0;
     int IntNumCallback = 0;
    int Currentindex = 0;
    string Username;
    string SnCode;
	std::map<int, float> exposureTimeMap;
	std::map<int, float> gainMap;
	std::map<int, float> gammaMap;
	QString RootPath;
	QMap<QString, QString> ParasValueMap;
    int getImageMaxCoiunts = 1;//一次信号取图次数
    int OnceGetImageNum = 1;//一次取图出图数量
    int timeOut = 1000;
    MV_CAM_TRIGGER_SOURCE m_MV_CAM_TRIGGER_SOURCE;//触发方式
    // std::atomic_int triggerMode = 0;//触发模式 0关闭 1打开
    std::atomic_int triggerMode{1};//触发模式 0关闭 1打开
    void registerGetImageFun(GetImageFun fun) { triggerOffBack = fun; }
    GetImageFun triggerOffBack;
signals:
	void trigged(int);

};
class  Hd_CameraModule_HIK3 :public PbGlobalObject
{

    Q_OBJECT
public:
    //1、创建：赋值给famliy
    explicit Hd_CameraModule_HIK3(QString sn, QString path ,int settype = -1, QObject* parent = nullptr);//对应哪个品牌相机(触发方式)/通信
    ~Hd_CameraModule_HIK3();
    //#######################通用函数#######################
    bool setParameter(const QMap<QString, QString>&);
    QMap<QString, QString> parameters();
    //初始化(加载模块到内存)
    bool init();
    bool setData(const std::vector<cv::Mat>&, const QStringList&);
    //获取数据
    bool data(std::vector<cv::Mat>&, QStringList&);
    //注册回调 string对应自身的参数协议 （自定义）
    void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    //注销回调 string对应自身的参数协议 （自定义）--->注销后还得取消连接状态
    void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&);
    QString GetRootPath() const { return RootPath; }
    QString GetSn() const { return Sncode; }
    QString Sncode;
	QString RootPath;
	QString JsonFilePath;
    CameraFunSDKfactoryCls* m_sdkFunc =nullptr;
    QMap<QString, QString> ParasValueMap;

signals:
    void sendMats(cv::Mat);

private slots:
		void Gettrigged(int);
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

class mPrivateWidget :public QWidget
{
	Q_OBJECT;
public:
    mPrivateWidget(void*);
    ~mPrivateWidget() {};
    void InitWidget();
	Hd_CameraModule_HIK3* m_Camerahandle = nullptr;
    void getRes(QByteArray);

private:
    void createConnect();
	void showImage(cv::Mat& image);
    QGridLayout* layout = nullptr;
    QJsonObject BytePtr;

    QPushButton* SetDataBtn     = nullptr;
    QPushButton* ContinuesBtn   = nullptr;
    QPushButton* Details        = nullptr;

    QComboBox* first            = nullptr; //触发模式
    QComboBox* Second           = nullptr;//触发源
    QComboBox* BalanceWhiteAuto = nullptr;//自动白平衡开关
    QSpinBox * BalanceRatioR     = nullptr;
    QSpinBox * BalanceRatioG     = nullptr;
    QSpinBox * BalanceRatioB     = nullptr;
    QComboBox* GamaDisable      = nullptr;
    QSpinBox * Count            = nullptr;//一次信号取图次数
    QSpinBox * timeout          = nullptr;//单张图超时时间
    viewWidget* m_showimage     = nullptr;
    QLineEdit* gain             = nullptr;
    QLineEdit* Gama             = nullptr;
    QLineEdit* Exposure         = nullptr;
    QPushButton*saveBtn         = nullptr;

    MyTableWidget* gainTable = nullptr;
    MyTableWidget* GamaTable = nullptr;
    MyTableWidget* ExposureTimeTable = nullptr;

    QPushButton *Add            = nullptr;
    QPushButton *Delete         = nullptr;
    QPushButton *takeEffect     = nullptr;
    QTableWidget * showTable    = nullptr;
    AlgParmWidget* m_AlgParmWidget= nullptr;

    QDoubleValidator* doubleValidator1 = nullptr;
    QDoubleValidator* doubleValidator2 = nullptr;
    QDoubleValidator* doubleValidator3 = nullptr;

    float gainMin,gainMax;
    float ExposureMin,ExposureMax;
    float GamaMin,GamaMax;

    QMap<QPair<int, int>, QString> cellOriginalValues;
    QList<int> BalanceRatioLst = {100,200,300};


signals:
    void sendImage(QImage);
};
#endif // Hd_CameraModule_HIK3_H
