#ifndef PBGLOBALOBJECT_H
#define PBGLOBALOBJECT_H

#include <QObject>
#include<opencv2/opencv.hpp>
#include <opencv.hpp>

//type对应的相机、触发方式
#define HIK_Hard 0 //海康
#define HIK_Soft 1
#define Basler_Hard 2
#define Basler_Soft 3
#define DaHua_Hard 4
#define DaHua_Soft 5
#define DaHeng_Hard 6
#define DaHeng_Soft 7
#define Dalsa 8
#define DalsaDAQ_Hard 9
#define DalsaDAQ_Soft 10
#define ITEK_Hard 11
#define ITEK_Soft 12
#define OPT_Hard 13
#define OPT_Soft 14
#define Keyence_25D 15
#define Keyence_3D 16
#define LMI_3D 17
#define Sszn_3D 18
#define ReadImage 1000

typedef void (*PBGLOBAL_CALLBACK_FUN)(QObject*, const std::vector<cv::Mat>&);
class PbGlobalObject : public QObject
{
    Q_OBJECT
public:
    explicit PbGlobalObject(int type, QObject* parent = nullptr) : QObject(parent), type1(type) { } //对应哪个品牌相机(触发方式)/通信
    virtual ~PbGlobalObject() { }

    //#######################通用函数#######################
    //初始化参数；通信/相机的初始化参数
    virtual bool setParameter(const QMap<QString, QString>&) = 0;
    //setParameter之后再调用，返回当前参数
    //相机：获取默认参数；
    //通信：获取初始化示例参数
    virtual QMap<QString, QString> parameters() = 0;
    //初始化(加载模块待内存)
    virtual bool init() = 0;

    //设置数据
    //相机：第一张图的参数，QStringList：00 曝光值 增益值；
    //相机：第N张图的参数，QStringList：非00 曝光值 增益值；
    //完成后发送信号trigged(bool);
    virtual bool setData(const std::vector<cv::Mat>&, const QStringList&) = 0;
    //获取数据
    virtual bool data(std::vector<cv::Mat>&, QStringList&) = 0;

    //注册回调 string对应自身的参数协议 （自定义）
    virtual void registerCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&) {}

    //注销回调 string对应自身的参数协议 （自定义）--->注销后还得取消连接状态
    virtual void cancelCallBackFun(PBGLOBAL_CALLBACK_FUN, QObject*, const QString&) {}

    //#######################通用函数######################
public:
    //大类
    enum PGOFAMLIY { GLOBALVARIABLE = 0, CAMERA2D, CAMERA2_5D, CAMERA3D, PLC, COM }famliy;
    //小类
    int type1 = -1;  //对应哪个品牌相机(触发方式)/通信
    int type2 = -1;
    int type3 = -1;

    //名称
    QString name = "";
    int id;

signals:
    void trigged(int);      //切换连接状态也需要发送trgged信号  //0，相机重连成功；1，相机掉线；2:软触发; 3：硬触发;   1000，重置计数
};
#endif // PBGLOBALOBJECT_H
