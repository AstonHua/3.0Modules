#include "Hd_CameraModule_3DLMI3.h"
#include <QDebug>
#include <QQueue>
#include <QTextCodec>
#include <qfuture.h>
#include <QtConcurrent/qtconcurrent>

using namespace std;
#pragma execution_character_set("utf-8")


#pragma region LMI

QString byteArrayToUnicode(const QByteArray array)
{

    // state用于保存转换状态，它的成员invalidChars，可用来判断是否转换成功
    // 如果转换成功，则值为0，如果值大于0，则说明转换失败
    QTextCodec::ConverterState state;
    // 先尝试使用utf-8的方式把QByteArray转换成QString
    QString text = QTextCodec::codecForName("UTF-8")->toUnicode(array.constData(), array.size(), &state);
    // 如果转换时无效字符数量大于0，说明编码格式不对
    if (state.invalidChars > 0)
    {
        // 再尝试使用GBK的方式进行转换，一般就能转换正确(当然也可能是其它格式，但比较少见了)
        text = QTextCodec::codecForName("GBK")->toUnicode(array);
    }
    return text;
}

//注册回调 string对应自身的参数协议 （自定义）
void Hd_CameraModule_3DLMI3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* m_QObject, const QString& cameraIndex)
{
    myPBGLOBAL_callBack m_myPBGLOBAL_callBack;
    m_myPBGLOBAL_callBack.callBackFun = callBackFun;
    m_myPBGLOBAL_callBack.m_QObject = m_QObject;
    m_myPBGLOBAL_callBack.cameraIndex = cameraIndex;
    m_sdkFunc->QQueue_myPBGLOBAL_callBack.enqueue(m_myPBGLOBAL_callBack);
}

QMap< GoSystem, CameraFunSDKfactoryCls*> myModuleMap; //需要修改

//回调函数
// Data callback function
// This function is called from a separate thread spawned by the GoSDK library.
// Processing within this function should be minimal.
kStatus kCall onData(void* ctx, void* sys, void* dataset)
{
    GoSystem m_system1 = sys;
    if (!myModuleMap.keys().contains(m_system1))
    {
        qDebug() << "[INFO] " << __FUNCTION__ << " line:" << __LINE__ << "onData err!";
        return kOK;
    }
    CameraFunSDKfactoryCls* Data = myModuleMap.value(m_system1);// (Hd_CameraModule_3DLMI3*)sys;
    Data->m_mutex->lock();
    double time_Start = (double)clock();
    unsigned int i, j;
    //DataContext* context = (DataContext*)ctx;
    GoStamp stamp;
    std::vector<cv::Mat> picVec;
    Data->m_flag = true;
    for (i = 0; i < GoDataSet_Count(dataset); ++i)
    {
        GoDataMsg dataObj = GoDataSet_At(dataset, i);

        switch (GoDataMsg_Type(dataObj))
        {
        case GO_DATA_MESSAGE_TYPE_UNIFORM_SURFACE:
        {
            MyTiff temp;
            GoUniformSurfaceMsg surfaceMsg = dataObj;
            double XResolution = NM_TO_MM(GoSurfaceMsg_XResolution(surfaceMsg));
            double YResolution = NM_TO_MM(GoSurfaceMsg_YResolution(surfaceMsg));
            double ZResolution = NM_TO_MM(GoSurfaceMsg_ZResolution(surfaceMsg));
            double XOffset = UM_TO_MM(GoSurfaceMsg_XOffset(surfaceMsg));
            double YOffset = UM_TO_MM(GoSurfaceMsg_YOffset(surfaceMsg));
            double ZOffset = UM_TO_MM(GoSurfaceMsg_ZOffset(surfaceMsg));

            int Width = GoSurfaceMsg_Width(surfaceMsg);
            int Height = GoSurfaceMsg_Length(surfaceMsg);

            if (Width > 100)
            {
                temp.Tiff = (unsigned short*)malloc(Width * Height * sizeof(unsigned short));// new unsigned short[Width * Height * sizeof(unsigned short)];
                temp.w = Width;
                temp.h = Height;
                for (int rowIdx = 0; rowIdx < Height; rowIdx++)
                {
                    k16s* data = GoSurfaceMsg_RowAt(surfaceMsg, rowIdx);
                    for (int colIdx = 0; colIdx < Width; colIdx++)
                    {
                        unsigned short val = data[colIdx];
                        //unsigned short val = ZOffset + ZResolution * data[colIdx];
                        temp.Tiff[rowIdx * Width + colIdx] = val;
                    }
                }

                cv::Mat heightMat1 = cv::Mat(temp.h, temp.w, CV_16U, temp.Tiff);
                //heightMat1.copyTo(heightMat2);
                Data->heightMatS.push_back(heightMat1.clone());
                Data->m_lmiheightFlag = true;
                free(temp.Tiff);
                temp.Tiff = NULL;
                //delete[]temp.Tiff;
                //temp.Tiff = nullptr;
            }
            
            /*cv::Mat dst;
            SYSTEMTIME t;
            std::vector<int> params{ (int)cv::IMWRITE_TIFF_RESUNIT,2, (int)cv::IMWRITE_TIFF_COMPRESSION,1,(int)cv::IMWRITE_TIFF_XDPI,300,(int)cv::IMWRITE_TIFF_YDPI,300 };
            GetLocalTime(&t);
            std::string filename = std::to_string(t.wHour) + "-" + std::to_string(t.wMinute) + "-" + std::to_string(t.wSecond) + "-" + std::to_string(t.wMilliseconds) + ".tiff";
            cv::imwrite(filename, img, params);*/
        }
            break;
        case GO_DATA_MESSAGE_TYPE_SURFACE_INTENSITY:
        {
            MyBmp temp;
            GoSurfaceIntensityMsg surfaceIntMsg = dataObj;
            double XResolution = NM_TO_MM(GoSurfaceIntensityMsg_XResolution(surfaceIntMsg));
            double YResolution = NM_TO_MM(GoSurfaceIntensityMsg_YResolution(surfaceIntMsg));
            double XOffset = UM_TO_MM(GoSurfaceIntensityMsg_XOffset(surfaceIntMsg));
            double YOffset = UM_TO_MM(GoSurfaceIntensityMsg_YOffset(surfaceIntMsg));

            int Width = GoSurfaceIntensityMsg_Width(surfaceIntMsg);
            int Height = GoSurfaceIntensityMsg_Length(surfaceIntMsg);

            temp.Bmp = (unsigned char*)malloc(Width * Height * sizeof(unsigned char));;// new unsigned char[Width * Height * sizeof(unsigned char)];
            temp.w = Width;
            temp.h = Height;

            for (int rowIdx = 0; rowIdx < Height; rowIdx++)
            {
                k8u* data = GoSurfaceIntensityMsg_RowAt(surfaceIntMsg, rowIdx);
                for (int colIdx = 0; colIdx < Width; colIdx++)
                {
                    temp.Bmp[rowIdx * Width + colIdx] = data[colIdx];
                }
            }

            cv::Mat luminanceMat1 = cv::Mat(temp.h, temp.w, CV_8UC1, temp.Bmp);
            //luminanceMat1.copyTo(luminanceMat2);
            Data->luminanceMatS.push_back(luminanceMat1.clone());
            Data->m_lmiluminanceFlag = true;
            free(temp.Bmp);
            temp.Bmp = NULL;
            //delete[]temp.Bmp;
            //temp.Bmp = nullptr;
            //SYSTEMTIME t;
            //GetLocalTime(&t);
            //std::string filename = std::to_string(t.wHour) + "-" + std::to_string(t.wMinute) + "-" + std::to_string(t.wSecond) + "-" + std::to_string(t.wMilliseconds) + ".bmp";
            //cv::imwrite(filename, img);
        }
            break;
        }
        //DataSetCollection.push_back(dataObj);
    }
    if (Data->ifHardTrigger == true)//硬触发，回调
    {
        std::vector<cv::Mat> mat;
        mat.push_back(Data->heightMatS.dequeue()); //tiff
        mat.push_back(Data->luminanceMatS.dequeue()); //bmp
        //触发回调
        for (int i = 0; i < Data->QQueue_myPBGLOBAL_callBack.size(); i++)
            if (Data->QQueue_myPBGLOBAL_callBack[i].cameraIndex == QString::number(Data->imgIndex))
                (*Data->QQueue_myPBGLOBAL_callBack[i].callBackFun)(Data->QQueue_myPBGLOBAL_callBack[i].m_QObject, mat);
        Data->imgIndex++;
        if (Data->imgIndex >= Data->expourseTimeList.size())
            Data->imgIndex = 0;
    }


    GoDestroy(dataset);
    double time_End = (double)clock();
    qDebug() << "[INFO] " << __FUNCTION__ << " line:" << __LINE__  << " checkImg getImg Time:" << (time_End - time_Start) << "ms" 
        << " --CameraName:" << Data->k32u_id << " imgIndex:" << Data->imgIndex << "--FrameID : " << stamp.frameIndex<< " --callBackNum : " << Data->callBackNum++;

    Data->m_mutex->unlock();
    return kOK;
}

//初始化流程
bool InitLmi(k32u ID, CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{

    kStatus status;
    if ((status = GoSdk_Construct(&m_CameraFunSDKfactoryCls->api)) != kOK)
    {
        //printf("Error: GoSdk_Construct:%d\n", status);
        qDebug() << "[Error] "  << "Error: GoSdk_Construct:%d\n", status;
        return false;
    }
    //system1 = this;
    if ((status = GoSystem_Construct(&m_CameraFunSDKfactoryCls->system1, kNULL)) != kOK)
    {
        //printf("Error: GoSystem_Construct:%d\n", status);
        qDebug() << "[Error] "  << "Error: GoSystem_Construct:%d\n", status;
        return false;
    }


    if ((status = GoSystem_FindSensorById(m_CameraFunSDKfactoryCls->system1, ID, &m_CameraFunSDKfactoryCls->sensor)) != kOK)
    {
        //printf("Error: GoSystem_FindSensor:%d\n", status);
        qDebug() << "[Error] "  << "Error: GoSystem_FindSensor:%d\n", status;
        return false;
    }
    k32u getId = GoSensor_BuddyId(m_CameraFunSDKfactoryCls->sensor);
    /*GoSystem_Disconnect(system1);*/
    // create connection to GoSystem object
    if ((status = GoSensor_Connect(m_CameraFunSDKfactoryCls->sensor)) != kOK)
    {
        qDebug() << "[Error] " << "Error: GoSensor_Connect:%d\n", status;
        return false;
    }

    // enable sensor data channel
    if ((status = GoSystem_EnableData(m_CameraFunSDKfactoryCls->system1, kTRUE)) != kOK)
    {
        //printf("Error: GoSensor_EnableData:%d\n", status);
        qDebug() << "[Error] "<< "Error: GoSensor_EnableData:%d\n", status;
        return false;
    }
    qDebug() << "system1:"<< m_CameraFunSDKfactoryCls->system1;
    myModuleMap.insert(m_CameraFunSDKfactoryCls->system1, m_CameraFunSDKfactoryCls);
    // set data handler to receive data asynchronously
    if ((status = GoSystem_SetDataHandler(m_CameraFunSDKfactoryCls->system1, onData, m_CameraFunSDKfactoryCls->contextPointer)) != kOK)
    {
        //printf("Error: GoSystem_SetDataHandler:%d\n", status);
        qDebug() << "[Error] "  << "Error: GoSystem_SetDataHandler:%d\n", status;
        return false;
    }
    
    if ((status = GoSystem_SetDataCapacity(m_CameraFunSDKfactoryCls->system1, kSIZE_MAX)) != kOK)
    {
        qDebug() << "[Error] " << "Error: GoSystem_SetDataCapacity:%d\n", status;
    }
    return true;
}

bool StartLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
    kStatus status;
    if ((status = GoSystem_Start(m_CameraFunSDKfactoryCls->system1)) != kOK)
    {
        //printf("Error: GoSystem_Stop:%d\n", status);
        qDebug() << "[Error] "  << "Failed to start LMI ";
        return false;
    }
    return true;
}

bool StopLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
    kStatus status;
    if ((status = GoSystem_Stop(m_CameraFunSDKfactoryCls->system1)) != kOK)
    {
        qDebug() << "[Error] "  << "Failed to stop LMI ";
        return false;
    }
    return true;
}

void CloseLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
    // destroy handles
    GoDestroy(m_CameraFunSDKfactoryCls->system1);
    GoDestroy(m_CameraFunSDKfactoryCls->api);
}

#pragma endregion


Hd_CameraModule_3DLMI3* create(int settype)
{
    qDebug() << __FUNCTION__ << "  line:" << __LINE__ << "Hd_CameraModule_3DLMI3 create success!";
    return new Hd_CameraModule_3DLMI3(settype);
}

void destory(Hd_CameraModule_3DLMI3* ptr)
{
    if (ptr)
    {
        delete  ptr;
        ptr = nullptr;
    }
    qDebug() << "[INFO] " << "Hd_CameraModule_3DLMI3  destory success!";
}

//int myIDS[10000] = { 0 };

//类创建
Hd_CameraModule_3DLMI3::Hd_CameraModule_3DLMI3(int settype, QObject* parent) : PbGlobalObject(settype, parent)
{
    m_sdkFunc = new CameraFunSDKfactoryCls();
    m_sdkFunc->m_mutex = new QMutex();
    m_sdkFunc->ifRunning = true;
    //m_mutex = new QMutex();
    //ifRunning = true;
    QFuture<void> myF= QtConcurrent::run(this, &Hd_CameraModule_3DLMI3::threadCheckState);
    if (type1 < 15)
        famliy = PGOFAMLIY::CAMERA2D;
    else if (type1 == 15)
        famliy = PGOFAMLIY::CAMERA2_5D;
    else if (type1 < 100)
        famliy = PGOFAMLIY::CAMERA3D;
    //id = myIDS[type1]++;
    qDebug() << "famliy::" << famliy;

    connect(this, &Hd_CameraModule_3DLMI3::trigged, [=](int val) {
        if (val == 1000) //清除计数
        {
            m_sdkFunc->m_mutex->lock();
            m_sdkFunc->callBackNum = 0;
            m_sdkFunc->imgIndex = 0;
            m_sdkFunc->m_mutex->unlock();
        }
        });
}


Hd_CameraModule_3DLMI3::~Hd_CameraModule_3DLMI3()
{
    this->disconnect();
    //myIDS[type1]--;
    m_sdkFunc->ifFirst = true;
    m_sdkFunc->ifRunning = false;
    m_sdkFunc->m_flag = false;
    Sleep(50);
    CloseLmi(m_sdkFunc);
    if (m_sdkFunc->m_mutex)
    {
        delete m_sdkFunc->m_mutex;
        m_sdkFunc->m_mutex = nullptr;
    }
    /*if(m_thread)
    {
        m_thread->exit();
        m_thread->wait(1);
        delete m_thread;
        m_thread = nullptr;
    }*/

    qDebug() << __FUNCTION__ << "line:" << __LINE__ << " delete success!";
}

//setParameter之后再调用，返回当前参数
    //相机：获取默认参数；
    //通信：获取初始化示例参数
QMap<QString, QString> Hd_CameraModule_3DLMI3::parameters()
{
    if (m_sdkFunc->ifFirst == true)
    {
        m_sdkFunc->ifFirst = false;

        m_sdkFunc->moduleName = "Hd_CameraModule_3DLMI3#" + QString::number(id);

        QJsonObject ExampleObj = load_camera_Example();
        QJsonObject cameraObj;
        QJsonArray  ExampleArray = ExampleObj.value("相机模块列表").toArray();
        for (int i = 0; i < ExampleArray.size(); i++)
        {
            QString name = ExampleArray[i].toObject().value("模块名称").toString();
            if (m_sdkFunc->moduleName.split("#")[0] == name)
                cameraObj = ExampleArray[i].toObject();
        } //参数设置的数据
        QMap<QString, QString> myParasValueMap;
        myParasValueMap.insert("相机key", "输入SN");
        //默认参数显示到界面
        QJsonArray ParameterArray = cameraObj.value("相机参数").toArray();
        for (int T = 0; T < ParameterArray.size(); T++)
        {
            QJsonObject ParameterObj = ParameterArray[T].toObject();
            QString ParameterKey = ParameterObj.value("参数名").toString();
            QString ParameterValue;
            QJsonArray ParameterValueArray = ParameterObj.value("相机参数值").toArray();
            for (int P = 0; P < ParameterValueArray.size(); P++)
            {
                if (P > 0)ParameterValue.append(",");
                ParameterValue.append(ParameterValueArray[P].toString());
            }
            myParasValueMap.insert(ParameterKey, ParameterValue);
        }
        return myParasValueMap;
    }
    else return m_sdkFunc->ParasValueMap;
}


QJsonObject Hd_CameraModule_3DLMI3::load_camera_Example()
{
    //QDir dir = QApplication::applicationDirPath();
    QString currPath;// = dir.path();
    currPath = "../runtime/Bin_DEVICE";
    QString json_cfg_file_path = currPath + "/camera_Example.json";

    QJsonObject json_object;
    try
    {
        QJsonParseError jsonError;
        if (json_cfg_file_path.isEmpty())
        {
            qCritical() << __FUNCTION__ << " line:" << __LINE__ << " JsonPath is null!";
            return json_object;
        }

        QFile JsonFile;
        JsonFile.setFileName(json_cfg_file_path);
        //if (!JsonFile.isReadable())
        //{
        //    qCritical() << __FUNCTION__ << " line:" << __LINE__ << " camera_Example.json not isReadable!";
        //    //return json_object;
        //}
        JsonFile.open(QIODevice::ReadOnly);

        QByteArray m_Byte = JsonFile.readAll();
        if (m_Byte.isEmpty())
        {
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << json_cfg_file_path + " Content is empty";
            JsonFile.close();
            return json_object;
        }

        QJsonDocument jsonDocument(QJsonDocument::fromJson(m_Byte, &jsonError));

        if (!jsonDocument.isNull() && jsonError.error == QJsonParseError::NoError)
        {
            if (jsonDocument.isObject())
            {
                json_object = jsonDocument.object();
                JsonFile.close();
                return json_object;
            }
        }
        else
        {
            qCritical() << __FUNCTION__ << " line:" << __LINE__ << json_cfg_file_path + " is error!";
        }
        JsonFile.close();

    }
    catch (QString ev)
    {
        qCritical() << __FUNCTION__ << " line:" << __LINE__ << " ev:" << ev;
    }
    return json_object;
}


//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_3DLMI3::setParameter(const QMap<QString, QString>& ParameterMap)
{
    m_sdkFunc->ParasValueMap = ParameterMap;
    //if (m_module.moduleName.isEmpty())
    //m_module.moduleName = getmoduleName();// = ParasValueMap.value()
    //if(id=)
    m_sdkFunc->moduleName = "Hd_CameraModule_3DLMI3#" + QString::number(id);
    return true;
}

//初始化(加载模块待内存)
bool Hd_CameraModule_3DLMI3::init()
{
    ////传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
    QString initByte = m_sdkFunc->ParasValueMap.value("相机key");
    foreach(QString key, m_sdkFunc->ParasValueMap.keys())
    {
        if (key == "相机key")
            continue;
        initByte += "+" + key + "#";
        QString ParameterValue = m_sdkFunc->ParasValueMap.value(key);
        initByte += ParameterValue.replace(",", "#");
    }

    if (initParas(initByte.toLocal8Bit()))
    {
        qDebug() << __FUNCTION__ << " line: " << __LINE__ << " key: " << m_sdkFunc->ParasValueMap.value("相机key") << " initParas   success! ";
        /*Base* myPtr = m_module.module_ptr;
        connect(myPtr, &Base::trigged, [=](bool ifConnect) {
            qDebug() << __FUNCTION__ << " line: " << __LINE__ << " key: " << ParasValueMap.value("相机key") << " m_module.module_ptr, &Base::trigged! ifConnect" << ifConnect;
            emit trigged(ifConnect);
            });*/
    }
    else
    {
        qCritical() << __FUNCTION__ << "  line: " << __LINE__ << " moduleName: " << m_sdkFunc->ParasValueMap.value("相机key") << " initParas failed! ";
        return false;
    }
    //}
    //else
    //{
    //    delete m_lib;
    //    m_lib = nullptr;
    //    return false;
    //}
    return true;
}

//设置数据
    //相机：第一张图的参数，QStringList：00 曝光值 增益值；
    //相机：第N张图的参数，QStringList：非00 曝光值 增益值；
    //完成后发送信号trigged(bool);
bool Hd_CameraModule_3DLMI3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
    //foreach(QString kidData,data)
        //if (kidData.contains("SetExposureTime"))
    {
        std::vector<cv::Mat> ImgS;
        QByteArray bData;
        for (int i = 0; i < data.size(); i++)
            //if(i>0)
            //    bData +=","+ data[i].toLocal8Bit();
            //else
            bData += data[i].toLocal8Bit();
        qDebug() << "Hd_CameraModule_3DLMI3::setData::" << bData;
        writeData(ImgS, bData);
        //if (m_module.module_ptr)
         //   m_module.module_ptr->writeData(ImgS, bData);
        //return true;
    }
    foreach(QString kidData, data)
        if (kidData.contains("SetExposureTime") || kidData.contains("rest_newProductIn"))
            return true;
    bool runResult = run(); //m_module.module_ptr->run();
    //emit trigged(runResult);
    return runResult;
}
//获取数据
bool Hd_CameraModule_3DLMI3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
    //std::vector<cv::Mat> ImgS;
    QByteArray data;
    //m_module.module_ptr->readData(ImgS, data);
    readData(ImgS, data);
    if (!data.isEmpty())QStringListdata.append(byteArrayToUnicode(data));
    return true;
}


//从类中读数据到实例对象
bool Hd_CameraModule_3DLMI3::readData(std::vector<cv::Mat>& vecPic, QByteArray &data )
{
    
    /*if (data == "connect")
    {
        data = mv_Status;
        return true;
    }*/
  

    m_sdkFunc->m_mutex->lock();
    if (m_sdkFunc->luminanceMatS.empty() || m_sdkFunc->heightMatS.empty())
    {
        qDebug() << " [Error] " << " srcImage is null";
        m_sdkFunc->m_mutex->unlock();
        return false;
    } 
    cv::Mat mheightMat2 = cv::Mat();
    m_sdkFunc->heightMatS.dequeue().copyTo(mheightMat2);
    cv::Mat mluminanceMat2 = cv::Mat();
    m_sdkFunc->luminanceMatS.dequeue().copyTo(mluminanceMat2);
    vecPic.push_back(mheightMat2.clone());
    vecPic.push_back(mluminanceMat2.clone());

    m_sdkFunc->m_mutex->unlock();
    qCritical() << " [info] " << __FUNCTION__ << " line:" << __LINE__ << "  checkImg readData id:" << m_sdkFunc->k32u_id <<" imgIndex" << m_sdkFunc->imgIndex;
    /*vector<cv::Mat> dstvec;
    dstvec.push_back(heightMat2.clone());
    dstvec.push_back(luminanceMat2.clone());
    m_mutex->unlock();
    vecPic.assign(dstvec.begin(), dstvec.end());*/
    return true;
    
}
//实例对象把数据写入到类
bool Hd_CameraModule_3DLMI3::writeData(std::vector<cv::Mat> &, QByteArray &qbyte)
{
    /*string reciveid = qbyte.toStdString();

    id = atoi(reciveid.c_str());*/
    //2024.02.18更新，相机对应的读SN信号时，表示新产品进入，设置初始值
    if (qbyte == "rest_newProductIn")
    {
        //根据实际项目，设置初始值：曝光、增益，ROI区域等
        /*{
            if (expourseTimeList.size() > 0)
                SetExposureTime(expourseTimeList[0]);
            if (gainList.size() > 0)
                SetGain(gainList[0]);
        }*/
        if (m_sdkFunc->luminanceMatS.size() > 0 || m_sdkFunc->heightMatS.size() > 0)
        {
            m_sdkFunc->m_mutex->lock();
            m_sdkFunc->luminanceMatS.clear();
            m_sdkFunc->heightMatS.clear();
            m_sdkFunc->m_mutex->unlock();
            std::cout << "rest_newProductIn , queuePic.size() > 0 ,signal maybe too fast " << m_sdkFunc->k32u_id << std::endl;
            qWarning() << __FUNCTION__ << "  line: " << __LINE__ << "rest_newProductIn , queuePic.size() > 0 ,signal maybe too fast " << m_sdkFunc->k32u_id;
        }
        if (m_sdkFunc->ifRunning == true)
        {
            m_sdkFunc->ifBreak = true;
            Sleep(10);
        }
        m_sdkFunc->imgIndex = 0;
        m_sdkFunc->getImageNum = 0;
        return true;
    }
    //else
    //{
    //    m_flag = false;
    //    expourseTimeList.clear();
    //    gainList.clear();
    //}
    return true;
}

//传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
bool Hd_CameraModule_3DLMI3::initParas(QByteArray& initData)
{
    QByteArrayList valueList = initData.split('+');

    if (valueList.size() > 1)
    for (int i = 1; i < valueList.size(); i++)
    {
        QString valuei = byteArrayToUnicode(valueList[i]);
        if (valuei.contains("是否为标准通信流程"))
        {
            if (valuei.contains("true"))
                m_sdkFunc->ifStandard = true;
        }
        if (valuei.contains("取图超时"))
        {
            QStringList gainL = valuei.split('#');
            if (gainL.size() > 1)
                m_sdkFunc->getImageTimeOut = gainL[1].toInt();
            if (m_sdkFunc->getImageTimeOut < 50)
                m_sdkFunc->getImageTimeOut = 1000;
        }
    }

    string reciveid = valueList[0].toStdString();

    m_sdkFunc->k32u_id = atoi(reciveid.c_str()); //id 相机序列号

    bool flag = InitLmi(m_sdkFunc->k32u_id, m_sdkFunc); //3D初始化 
    m_sdkFunc->moduleStatus = flag ? OK : UnInit;
    if (!flag)
    {
        qDebug() << "[Error] "  << "Failed to Init LMI,id:"<< m_sdkFunc->k32u_id;
        return false;
    }

    flag = StartLmi(m_sdkFunc);
    m_sdkFunc->moduleStatus == flag ? Grab : OK;

    if (!flag)
    {
        qDebug() << "[Error] "  << " Failed to start LMI";
        return false;
    }

    qDebug() << "[INFO] " << " success to start LMI,id:" << m_sdkFunc->k32u_id;

    return true;
}

bool Hd_CameraModule_3DLMI3::run()
{
    m_sdkFunc->ifRunning = true;
    m_sdkFunc->ifBreak = false;
    bool result = false;
    //qDebug() << "[info]" << __FUNCTION__ << " line:" <<  __LINE__ << " run start" << k32u_id;
    try
    {
        m_sdkFunc->m_flag = false;
        int mytime = 0;
        while (true)
        {
            if (m_sdkFunc->moduleStatus == false || mytime++ > m_sdkFunc->getImageTimeOut || m_sdkFunc->ifBreak == true)
            {
                result = false;
                break;
            }
            m_sdkFunc->m_mutex->lock();
            if (m_sdkFunc->luminanceMatS.size() > 0 && m_sdkFunc->heightMatS.size() > 0)
            {
                result = true;
                m_sdkFunc->m_mutex->unlock();
                break;
            }
            m_sdkFunc->m_mutex->unlock();
            Sleep(1);
        }
    }
    catch (QString ev)
    {
        qDebug() << "[Error] " << " ev:" << ev ;
    }
    //qDebug() << "[info]"<< __FUNCTION__ << " line:" <<  __LINE__ << " run end" ;
    return result;
}

//相机连接状态：正常连接或断开
bool Hd_CameraModule_3DLMI3::checkStatus()
{
    return m_sdkFunc->moduleStatus;
}
void Hd_CameraModule_3DLMI3::threadCheckState() //线程检查相机状态
{
    int checkNum = 0;
    while (m_sdkFunc->ifRunning == true)
    {
        if (checkNum++ == 50)
        {
            if (m_sdkFunc->sensor != kNULL)
            {
                bool currentState = false;
                int mstate = GoSensor_State(m_sdkFunc->sensor);
                if (mstate == 6)
                    currentState = false;
                else
                    currentState = true;
                if (currentState != m_sdkFunc->moduleStatus)
                {
                    qDebug() << "[INFO] " << "moduleStatus changed emit trigged:" << m_sdkFunc->moduleStatus;
                    if (currentState == true) //重连成功
                    {
                        emit trigged(0);
                    }
                    else //掉线
                    {
                        emit trigged(1);
                    }
                    m_sdkFunc->moduleStatus = currentState;
                }
                if (currentState == false) //掉线，重新连接
                {
                    if (init())
                    {
                        qDebug() << "[INFO] " << "moduleStatus changed emit trigged:" << m_sdkFunc->moduleStatus;
                        emit trigged(0);
                    }
                }
            }
            checkNum = 0;
        }
        Sleep(10);
    }
}