#include "Hd_CameraModule_Basler3.h"

// 全局变量
QMap<QString, OnePb_Basler> TotalMap_Basler;
Pylon::DeviceInfoList_t g_deviceList;  // 修正类型

//=============================================================================
// 工具函数实现
//=============================================================================
bool SearchBaslerDevice()
{
    try
    {
        Pylon::PylonInitialize();
        Pylon::CTlFactory& tlFactory = Pylon::CTlFactory::GetInstance();
        g_deviceList.clear();
        tlFactory.EnumerateDevices(g_deviceList);  // 修正调用方式
        return (g_deviceList.size() > 0);
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "SearchBaslerDevice error: " << e.GetDescription();
        return false;
    }
}

bool IsColorBasler(Pylon::EPixelType enType)
{
    switch (enType)
    {
    case Pylon::PixelType_BayerBG8:
    case Pylon::PixelType_BayerGB8:
    case Pylon::PixelType_BayerRG8:
    case Pylon::PixelType_BayerGR8:
    case Pylon::PixelType_BGR8packed:
    case Pylon::PixelType_RGB8packed:
    case Pylon::PixelType_YUV422_YUYV_Packed:
        return true;
    default:
        return false;
    }
}

//=============================================================================
// CameraFunSDKfactoryCls 实现
//=============================================================================
CameraFunSDKfactoryCls::CameraFunSDKfactoryCls(QString Sn, QString path, QObject* parent)
    : QObject(parent), SnCode(Sn.toStdString()), RootPath(path)
{
    Pylon::PylonInitialize();
    allowflag = false;
    triggerMode = 1;
    camera = nullptr;
    m_isConnected = false;

    m_healthCheckTimer = new QTimer(this);
    connect(m_healthCheckTimer, &QTimer::timeout, this, &CameraFunSDKfactoryCls::checkCameraHealth);
}

CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
{
    m_healthCheckTimer->stop();

    if (camera)
    {
        disconnectCamera();
        delete camera;
        camera = nullptr;
    }
    MatQueue.clear();
    Pylon::PylonTerminate();
}

cv::Mat CameraFunSDKfactoryCls::convertToMat(const Pylon::CGrabResultPtr& ptrGrabResult)
{
    cv::Mat img;

    try
    {
        int width = ptrGrabResult->GetWidth();
        int height = ptrGrabResult->GetHeight();
        Pylon::EPixelType pixelType = ptrGrabResult->GetPixelType();
        uint8_t* pBuffer = (uint8_t*)ptrGrabResult->GetBuffer();

        switch (pixelType)
        {
        case Pylon::PixelType_Mono8:
            img = cv::Mat(height, width, CV_8UC1, pBuffer).clone();
            break;

        case Pylon::PixelType_BayerBG8:
        case Pylon::PixelType_BayerGB8:
        case Pylon::PixelType_BayerRG8:
        case Pylon::PixelType_BayerGR8:
        {
            cv::Mat rawImg(height, width, CV_8UC1, pBuffer);
            cv::cvtColor(rawImg, img, cv::COLOR_BayerBG2BGR);
        }
        break;

        case Pylon::PixelType_BGR8packed:
            img = cv::Mat(height, width, CV_8UC3, pBuffer).clone();
            break;

        case Pylon::PixelType_RGB8packed:
        {
            cv::Mat rgbImg(height, width, CV_8UC3, pBuffer);
            cv::cvtColor(rgbImg, img, cv::COLOR_RGB2BGR);
        }
        break;

        default:
            if (IsColorBasler(pixelType))
            {
                img = cv::Mat::zeros(height, width, CV_8UC3);
            }
            else
            {
                img = cv::Mat::zeros(height, width, CV_8UC1);
            }
            break;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "convertToMat error: " << e.GetDescription();
    }

    return img;
}

bool CameraFunSDKfactoryCls::connectCamera(std::string GetSnName)
{
    try
    {
        if (!SearchBaslerDevice())
            return false;

        int index = -1;
        for (size_t i = 0; i < g_deviceList.size(); i++)
        {
            std::string sn = g_deviceList[i].GetSerialNumber();
            if (sn == GetSnName)
            {
                index = i;
                break;
            }
        }

        if (index == -1)
            return false;

        // 如果相机已存在，先关闭
        if (camera)
        {
            if (camera->IsOpen())
            {
                if (camera->IsGrabbing())
                    camera->StopGrabbing();
                camera->Close();
            }
            delete camera;
            camera = nullptr;
        }

        // 创建并打开相机 - 修正构造方式
        Pylon::CTlFactory& tlFactory = Pylon::CTlFactory::GetInstance();
        Pylon::CDeviceInfo deviceInfo = g_deviceList[index];
        camera = new Pylon::CBaslerUniversalInstantCamera(tlFactory.CreateDevice(deviceInfo));

        if (!camera)
            return false;

        camera->Open();

        m_pGrabHandler = new BaslerGrabHandler(this);
        camera->RegisterImageEventHandler(
            m_pGrabHandler,                              // 参数1：SDK 兼容的回调对象
            Pylon::RegistrationMode_ReplaceAll,          // 参数2：注册模式
            Pylon::Cleanup_Delete                          // 参数3：清理策略
        );

        // 获取参数范围
        if (camera->ExposureTimeAbs.IsReadable())
        {
            exposureMin = camera->ExposureTimeAbs.GetMin();
            exposureMax = camera->ExposureTimeAbs.GetMax();
        }

        if (camera->GainRaw.IsReadable())
        {
            gainMin = camera->GainRaw.GetMin();
            gainMax = camera->GainRaw.GetMax();
        }

        if (camera->Gamma.IsReadable())
        {
            gammaMin = camera->Gamma.GetMin();
            gammaMax = camera->Gamma.GetMax();
        }
        camera->TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_On);
        setTriggerSource(BASLER_TRIGGER_SOURCE_SOFTWARE);

        // 设置默认触发模式\开始采集

        setTriggerMode(true);

        m_isConnected = true;
        m_healthCheckTimer->start(5000);

        qDebug() << "Basler camera connected:" << QString::fromStdString(GetSnName);
        return true;
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "connectCamera error: " << e.GetDescription();
        return false;
    }
}
// 图像回调函数（全程只注册一次，通过原子变量控制是否处理）
void CameraFunSDKfactoryCls::onImageGrabbed(CInstantCamera& cam, const CGrabResultPtr& ptrGrabResult)
{
    if (!ptrGrabResult->GrabSucceeded())
        return;
   QList<cv::Mat> Outmats;
    CameraFunSDKfactoryCls* currentUser = this;
    // 3. 转换为Mat（已有逻辑）
    cv::Mat srcImage = convertToMat(ptrGrabResult);
    if (srcImage.empty())
        return;
    
    if (this->triggerMode.load(std::memory_order::memory_order_acquire) == 0)
    {
        currentUser->triggerOffBack(srcImage);
        return;
    }
    Outmats.push_back(srcImage.clone());
    //if (currentUser->allowflag.load(std::memory_order::memory_order_acquire))
    {
        if (currentUser->m_triggerSource == BASLER_TRIGGER_SOURCE_SOFTWARE)
        {
            if (currentUser->allowflag.load(std::memory_order::memory_order_acquire))
                currentUser->MatQueue.push(Outmats);
        }
        else
        {
            qDebug() << currentUser->Currentindex;
            ///硬触发不受开关控制，没有缓存
            if (currentUser->CallbackFuncMap.keys().contains(currentUser->Currentindex))
            {
                QObject* obj = currentUser->CallbackFuncMap.value(currentUser->Currentindex).callbackparent;
                obj->setProperty("cameraIndex", QString::number(currentUser->Currentindex));
                currentUser->CallbackFuncMap.value(currentUser->Currentindex).GetimagescallbackFunc(obj, Outmats);
            }
            else
            {
                qWarning() << "CallbackFuncMap.keys()" << currentUser->CallbackFuncMap.keys() << currentUser->Currentindex;
            }
        }

    }
}
void CameraFunSDKfactoryCls::disconnectCamera()
{
    m_healthCheckTimer->stop();

    if (camera && camera->IsOpen())
    {
        if (camera->IsGrabbing())
        {
            camera->StopGrabbing();
            QThread::msleep(100); // 等待采集线程完全退出
        }

        if (m_pGrabHandler)
        {
            camera->DeregisterImageEventHandler(m_pGrabHandler);
            m_pGrabHandler = nullptr; // 避免野指针
        }
        Pylon::CGrabResultPtr ptrGrabResult;
        while (camera->RetrieveResult(10, ptrGrabResult, Pylon::TimeoutHandling_Return))
        {
            ptrGrabResult.Release();
        }
        camera->Close();
    }
    m_isConnected = false;
}

void CameraFunSDKfactoryCls::checkCameraHealth()
{
    if (!camera || !camera->IsOpen())
    {
        if (m_isConnected)
        {
            qDebug() << "[Error] Basler camera disconnected!";
            m_isConnected = false;
            emit trigged(1);

            // 尝试重连
            QTimer::singleShot(1000, [this]() {
                if (connectCamera(SnCode))
                {
                    qWarning() << "[Hd_CameraModule_Basler] Reconnect success!";
                    emit trigged(1);
                }
                });
        }
        return;
    }

    try
    {
        // 尝试获取设备信息来验证连接
        Pylon::String_t sn = camera->GetDeviceInfo().GetSerialNumber();
        if (!m_isConnected)
        {
            m_isConnected = true;
            emit trigged(1);
        }
    }
    catch (const Pylon::GenericException& e)
    {
        if (m_isConnected)
        {
            qDebug() << "[Error] Basler camera connection lost!";
            m_isConnected = false;
            emit trigged(1);

            QTimer::singleShot(1000, [this]() {
                if (connectCamera(SnCode))
                {
                    qWarning() << "[Hd_CameraModule_Basler] Reconnect success!";
                    emit trigged(1);
                }
                });
        }
    }
}


cv::Mat CameraFunSDKfactoryCls::grabOneImage()
{
    cv::Mat img;
   QList<cv::Mat> out;
    try
    {
        if (!camera || !camera->IsGrabbing())
            return img;

        if (m_triggerSource == BASLER_TRIGGER_SOURCE_SOFTWARE)
        {
            // 软触发模式
            
            camera->ExecuteSoftwareTrigger();

            Pylon::CGrabResultPtr ptrGrabResult;
            if (camera->RetrieveResult(1000, ptrGrabResult, Pylon::TimeoutHandling_Return))
            {
                // if (ptrGrabResult->GrabSucceeded())
                // {
                //     // img = convertToMat(ptrGrabResult);
                //     // out.push_back(img);
                //     // MatQueue.push(out);
                // }
                
            }
            ptrGrabResult.Release();
        }
        else
        {
            // 硬触发模式
            Pylon::CGrabResultPtr ptrGrabResult;
            if (camera->RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException))
            {
                if (ptrGrabResult->GrabSucceeded())
                {
                    img = convertToMat(ptrGrabResult);
                    out.push_back(img);
                    MatQueue.push(out);
                }
            }
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "grabOneImage error: " << e.GetDescription();
    }

    return img;
}

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{
    if (!connectCamera(SnCode))
    {
        emit trigged(1);
        return false;
    }

    emit trigged(1);

    // 获取当前触发源 - 修正版本
    try
    {
        if (camera && camera->IsOpen())
        {
            // 使用 ToString() 获取字符串形式
            GenICam::gcstring source = camera->TriggerSource.ToString();

            if (source == "Software")
                m_triggerSource = BASLER_TRIGGER_SOURCE_SOFTWARE;
            else if (source == "Line1")
                m_triggerSource = BASLER_TRIGGER_SOURCE_LINE1;
            else if (source == "Line2")
                m_triggerSource = BASLER_TRIGGER_SOURCE_LINE2;
            else  // "Line0" 或其他
                m_triggerSource = BASLER_TRIGGER_SOURCE_ACTION;

            qDebug() << "Current trigger source:" << source.c_str();
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Get trigger source error: " << e.GetDescription();
        m_triggerSource = BASLER_TRIGGER_SOURCE_SOFTWARE;  // 默认值
    }

    return true;
}

void CameraFunSDKfactoryCls::upDateParam()
{
    getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts").toInt();
    OnceGetImageNum = ParasValueMap.value("OnceImageCounts").toInt();
    timeOut = ParasValueMap.value("GetOnceImageTimes").toInt();
    qDebug() << "Params updated:" << getImageMaxCoiunts << OnceGetImageNum << timeOut;
}

bool CameraFunSDKfactoryCls::setArrayByte(QString Key, QJsonArray array)
{
    if (Key == "ExposureTime")
    {
        exposureTimeMap.clear();
        for (auto str : array)
        {
            QStringList pair = str.toString().split(",", QString::SkipEmptyParts);
            if (pair.size() == 2)
            {
                int index = pair[0].toInt();
                float value = pair[1].toFloat();
                exposureTimeMap[index] = value;
            }
        }
    }
    else if (Key == "Gain")
    {
        gainMap.clear();
        for (auto str : array)
        {
            QStringList pair = str.toString().split(",", QString::SkipEmptyParts);
            if (pair.size() == 2)
            {
                int index = pair[0].toInt();
                float value = pair[1].toFloat();
                gainMap[index] = value;
            }
        }
    }
    else if (Key == "Gamma")
    {
        gammaMap.clear();
        for (auto str : array)
        {
            QStringList pair = str.toString().split(",", QString::SkipEmptyParts);
            if (pair.size() == 2)
            {
                int index = pair[0].toInt();
                float value = pair[1].toFloat();
                gammaMap[index] = value;
            }
        }
    }
    return true;
}

bool CameraFunSDKfactoryCls::setExposureTime(float value)
{
    try
    {
        if (camera && camera->IsOpen() && camera->ExposureTimeAbs.IsWritable())
        {
            camera->ExposureTimeAbs.SetValue(value);
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set exposure time error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::setGain(int value)
{
    try
    {
        if (camera && camera->IsOpen() && camera->GainRaw.IsWritable())
        {
            camera->GainRaw.SetValue(value);
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set gain error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::setGamma(float value)
{
    try
    {
        if (camera && camera->IsOpen() && camera->Gamma.IsWritable())
        {
            camera->Gamma.SetValue(value);
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set gamma error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::setTriggerMode(bool on)
{
    try
    {
        if (camera && camera->IsOpen())
        {
            if (camera->IsGrabbing())
            {
                camera->StopGrabbing();
            }
            if (on)
            {
                camera->AcquisitionMode.SetValue(Basler_UniversalCameraParams::AcquisitionMode_Continuous);
                camera->TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_On);
                camera->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Software); // 强制设为软触发源
                camera->StartGrabbing(
                    Pylon::GrabStrategy_LatestImageOnly
                );
                
            }
            else
            {
                camera->AcquisitionMode.SetValue(Basler_UniversalCameraParams::AcquisitionMode_Continuous);
                camera->TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_Off);
                camera->StartGrabbing(
                    Pylon::GrabStrategy_LatestImageOnly,
                    Pylon::GrabLoop_ProvidedByInstantCamera  
                );
                
            }

            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set trigger mode error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::setTriggerSource(int source)
{
    try
    {
        if (camera && camera->IsOpen())
        {
            switch (source)
            {
            case BASLER_TRIGGER_SOURCE_ACTION:
                camera->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Action1);  // 使用字符串
                break;
            case BASLER_TRIGGER_SOURCE_LINE1:
                camera->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Line1);
                break;
            case BASLER_TRIGGER_SOURCE_LINE2:
                camera->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Line2);
                break;
            case BASLER_TRIGGER_SOURCE_SOFTWARE:
                camera->TriggerSource.SetValue(TriggerSourceEnums::TriggerSource_Software);
                break;
            }
            m_triggerSource = source;
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set trigger source error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::setPixelFormat(const QString& format)
{
    try
    {
        if (camera && camera->IsOpen() && camera->PixelFormat.IsWritable())
        {
            if (format == "Mono8")
                camera->PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormat_Mono8);
            else if (format == "BayerBG8")
                camera->PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormat_BayerBG8);
            else if (format == "BayerGB8")
                camera->PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormat_BayerGB8);
            else if (format == "BayerRG8")
                camera->PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormat_BayerRG8);
            else if (format == "BayerGR8")
                camera->PixelFormat.SetValue(Basler_UniversalCameraParams::PixelFormat_BayerGR8);
            else if (format == "BGR8")
                camera->PixelFormat.SetValue("BGR8");
            else if (format == "RGB8")
                camera->PixelFormat.SetValue("RGB8");
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set pixel format error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::setAcquisitionMode(const QString& mode)
{
    try
    {
        if (camera && camera->IsOpen() && camera->AcquisitionMode.IsWritable())
        {
            if (mode == "Continuous")
                camera->AcquisitionMode.SetValue(Basler_UniversalCameraParams::AcquisitionMode_Continuous);
            else if (mode == "SingleFrame")
                camera->AcquisitionMode.SetValue(Basler_UniversalCameraParams::AcquisitionMode_SingleFrame);
            else if (mode == "MultiFrame")
                camera->AcquisitionMode.SetValue(Basler_UniversalCameraParams::AcquisitionMode_MultiFrame);
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Set acquisition mode error: " << e.GetDescription();
    }
    return false;
}

bool CameraFunSDKfactoryCls::softwareTrigger()
{
    try
    {
        if (camera && camera->IsOpen())
        {
            //camera->ExecuteSoftwareTrigger();
            grabOneImage();
            return true;
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "Software trigger error: " << e.GetDescription();
    }
    return false;
}

//=============================================================================
// Hd_CameraModule_Basler3 实现
//=============================================================================
Hd_CameraModule_Basler3::Hd_CameraModule_Basler3(QString sn, QString path, int settype, QObject* parent)
    : PbGlobalObject(settype, parent), Sncode(sn), RootPath(path)
{
    famliy = CAMERA2D;
    JsonFilePath = RootPath + Sncode + ".json";

    QString FirstCreateByte(R"({
        "SeralNum": ")" + sn + R"(",
        "GetOnceImageTimes": "1000",
        "LastUpdateTime": "",
        "OnceImageCounts": "1",
        "OnceSignalsGetImageCounts": "20",
        "ExposureTime": [],
        "Gain": [],
        "Gamma": []
    })");

    if (!QFile(JsonFilePath).exists())
        createAndWritefile(JsonFilePath, FirstCreateByte.toUtf8());

    QJsonObject paramObj = load_JsonFile(JsonFilePath);
    for (auto objStr : paramObj.keys())
    {
        if (paramObj.value(objStr).isString())
            ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
    }

    m_sdkFunc = new CameraFunSDKfactoryCls(sn, path, this);
    connect(m_sdkFunc, &CameraFunSDKfactoryCls::trigged, this,
        [=](int value) { emit trigged(value); });
    connect(m_sdkFunc, &CameraFunSDKfactoryCls::imageGrabbed, this,
        [=](cv::Mat img) { emit sendMats(img); });
}

Hd_CameraModule_Basler3::~Hd_CameraModule_Basler3()
{
    if (m_sdkFunc)
    {
        delete m_sdkFunc;
    }
}

QMap<QString, QString> Hd_CameraModule_Basler3::parameters()
{
    return ParasValueMap;
}

bool Hd_CameraModule_Basler3::setParameter(const QMap<QString, QString>& ParameterMap)
{
    ParasValueMap = ParameterMap;
    //m_sdkFunc->ParasValueMap = ParasValueMap;
    m_sdkFunc->upDateParam();
    return true;
}

bool Hd_CameraModule_Basler3::init()
{
    connect(this, &PbGlobalObject::trigged, [=](int Code) {
        if (Code == 1000)
        {
            m_sdkFunc->Currentindex = 0;
            m_sdkFunc->MatQueue.clear();
            m_sdkFunc->allowflag.store(true, std::memory_order::memory_order_release);
            emit trigged(501);
        }
        else if (Code == 1001)
        {
            m_sdkFunc->allowflag.store(false, std::memory_order::memory_order_release);
        }
        else if (Code == 1)
        {
            m_sdkFunc->triggerMode.store(1, std::memory_order::memory_order_release);
            //m_sdkFunc->setTriggerMode(true);
        }
        else if (Code == 0)
        {
            m_sdkFunc->triggerMode.store(0, std::memory_order::memory_order_release);
            //m_sdkFunc->setTriggerMode(false);
        }
        });

    //setParameter(ParasValueMap);
    bool flag = m_sdkFunc->initSdk(ParasValueMap);

    //if (m_sdkFunc->m_triggerSource == BASLER_TRIGGER_SOURCE_SOFTWARE)
       // type1 = 1;
   // else
       // type1 = 0;

    //type2 = 0;

    return flag;
}

bool Hd_CameraModule_Basler3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
    Q_UNUSED(mats);
    Q_UNUSED(data);

    if (m_sdkFunc->m_triggerSource == BASLER_TRIGGER_SOURCE_SOFTWARE)
    {
        trigged(1000);
        return m_sdkFunc->softwareTrigger();
        //
    }
    return true;
}

bool Hd_CameraModule_Basler3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
    Q_UNUSED(QStringListdata);

    // 从队列中获取图像
    QList<cv::Mat> out;
    m_sdkFunc->MatQueue.wait_for_pop(m_sdkFunc->timeOut, out);
    if (out.isEmpty())
    {
        ImgS.push_back(cv::Mat::zeros(100, 100, 0));
        return false;
    }
    ImgS = out.toVector().toStdVector();
    return true; 

    // 超时或失败，尝试直接抓取一张
    //cv::Mat img = m_sdkFunc->grabOneImage();
    //if (!img.empty())
    //{
        //ImgS.push_back(img);
        //return true;
    //}
}

void Hd_CameraModule_Basler3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent, const QString& getString)
{
    CallbackFuncPack_Basler TempPack;
    TempPack.callbackparent = parent;
    TempPack.cameraIndex = getString;
    TempPack.GetimagescallbackFunc = func;
    m_sdkFunc->CallbackFuncMap.insert(getString.toInt(), TempPack);
}

void Hd_CameraModule_Basler3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
    int index = getString.toInt();
    if (m_sdkFunc->CallbackFuncMap.keys().contains(index))
    {
        if (callBackFun == m_sdkFunc->CallbackFuncMap.value(index).GetimagescallbackFunc)
            m_sdkFunc->CallbackFuncMap.remove(index);
        else
        {
            qCritical() << "key of Values != Input Callbackfun" << getString;
        }
    }
}

//=============================================================================
// 导出函数实现
//=============================================================================
extern "C"
{
    Q_DECL_EXPORT bool create(const QString& DeviceSn, const QString& name, const QString& path)
    {
        if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
            return false;

        QString key = name.split(':').first();
        if (TotalMap_Basler.keys().contains(key))
            return true;

        OnePb_Basler temp;
        temp.base = new Hd_CameraModule_Basler3(DeviceSn, path + "/Hd_CameraModule_Basler3/");

        if (!temp.base->init())
        {
            delete temp.base;
            return false;
        }

        temp.baseWidget = new mPrivateWidget(temp.base);
        temp.DeviceSn = DeviceSn;
        TotalMap_Basler.insert(key, temp);
        return true;
    }

    Q_DECL_EXPORT void destroy(const QString& name)
    {
        auto temp = TotalMap_Basler.take(name);
        if (temp.base)
        {
            delete temp.base;
        }
        if (temp.baseWidget)
        {
            delete temp.baseWidget;
        }
    }

    Q_DECL_EXPORT QWidget* getCameraWidgetPtr(const QString& name)
    {
        if (TotalMap_Basler[name].baseWidget)
            return TotalMap_Basler[name].baseWidget;
        return nullptr;
    }

    Q_DECL_EXPORT PbGlobalObject* getCameraPtr(const QString& name)
    {
        if (TotalMap_Basler[name].base)
            return TotalMap_Basler[name].base;
        return nullptr;
    }

    Q_DECL_EXPORT QStringList getCameraSnList()
    {
        QStringList temp;

        if (!SearchBaslerDevice())
            return temp;

        for (size_t i = 0; i < g_deviceList.size(); i++)
        {
            std::string sn = g_deviceList[i].GetSerialNumber();
            if (!sn.empty())
                temp << QString::fromStdString(sn);
        }

        // 移除已使用的相机
        foreach(const auto & tmp, TotalMap_Basler)
        {
            if (temp.contains(tmp.DeviceSn))
            {
                temp.removeOne(tmp.DeviceSn);
            }
        }

        return temp;
    }
}

//=============================================================================
// mPrivateWidget 实现 - UI界面
//=============================================================================
mPrivateWidget::mPrivateWidget(void* handle)
{
    m_Camerahandle = reinterpret_cast<Hd_CameraModule_Basler3*>(handle);
    InitWidget();

    // 连接图像显示信号
    connect(this, &mPrivateWidget::sendImage, this,[=](QImage img) { m_showimage->reciveImage("", img); },Qt::QueuedConnection);

}

void mPrivateWidget::showImage(cv::Mat& image)
{
    emit sendImage(cvMatToQImage(image));
}

void mPrivateWidget::loadCurrentParams()
{
    try
    {
        if (!m_Camerahandle->m_sdkFunc->camera 
            || !m_Camerahandle->m_sdkFunc->camera->IsOpen())
            return;

        auto& camera = *m_Camerahandle->m_sdkFunc->camera;

        if (camera.ExposureTimeAbs.IsReadable())
        {
            ExposureMin = camera.ExposureTimeAbs.GetMin();
            ExposureMax = camera.ExposureTimeAbs.GetMax();
            double curExposure = camera.ExposureTimeAbs.GetValue();
            Exposure->setText(QString::number(curExposure, 'f', 2));
        }

        // 获取增益
        if (camera.GainRaw.IsReadable())
        {
            gainMin = camera.GainRaw.GetMin();
            gainMax = camera.GainRaw.GetMax();
            double curGain = camera.GainRaw.GetValue();
            gain->setText(QString::number(curGain));
        }

        // 获取伽马值
        if (camera.Gamma.IsReadable())
        {
            GamaMin = camera.Gamma.GetMin();
            GamaMax = camera.Gamma.GetMax();
            double curGamma = camera.Gamma.GetValue();
            Gama->setText(QString::number(curGamma, 'f', 2));
        }

        // 获取触发模式
        if (camera.TriggerMode.IsReadable())
        {
            int64_t mode = camera.TriggerMode.GetValue();
            first->setCurrentIndex(mode == 1 ? 0 : 1);
        }

        // 获取触发源
        if (camera.TriggerSource.IsReadable())
        {
            
            TriggerSourceEnums sourceEnum = camera.TriggerSource.GetValue();
            // 匹配枚举值（注意：Basler枚举中无TriggerSource_Line0，需确认相机是否真的支持Line0）
            if (sourceEnum == TriggerSourceEnums::TriggerSource_Action1)  // 若相机无Line0，可注释/删除此分支
                Second->setCurrentIndex(0);
            else if (sourceEnum == TriggerSourceEnums::TriggerSource_Line1)
                Second->setCurrentIndex(1);
            else if (sourceEnum == TriggerSourceEnums::TriggerSource_Line2)
                Second->setCurrentIndex(2);
            else if (sourceEnum == TriggerSourceEnums::TriggerSource_Software)
                Second->setCurrentIndex(3);
        }

        // 获取像素格式
        if (camera.PixelFormat.IsReadable())
        {
            PixelFormatEnums format = camera.PixelFormat.GetValue();
           
            if (format == PixelFormat_Mono8) pixelFormat->setCurrentIndex(0); 
            else if (format == PixelFormat_BayerBG8) pixelFormat->setCurrentIndex(1);
            else if (format == PixelFormat_BayerGB8) pixelFormat->setCurrentIndex(2);
            else if (format == PixelFormat_BayerRG8) pixelFormat->setCurrentIndex(3);
            else if (format == PixelFormat_BayerGR8) pixelFormat->setCurrentIndex(4);
            else if (format == PixelFormat_BGR8) pixelFormat->setCurrentIndex(5);
            else if (format == PixelFormat_RGB8) pixelFormat->setCurrentIndex(6);
        }

        // 获取帧率
        if (camera.AcquisitionFrameRateAbs.IsReadable())
        {
            double fps = camera.AcquisitionFrameRateAbs.GetValue();
            frameRate->setText(QString::number(fps, 'f', 2));
        }

        // 获取采集模式
        if (camera.AcquisitionMode.IsReadable())
        {
            GenICam::gcstring mode = camera.AcquisitionMode.ToString();
            if (mode == "Continuous") acquisitionMode->setCurrentIndex(0);
            else if (mode == "SingleFrame") acquisitionMode->setCurrentIndex(1);
            else if (mode == "MultiFrame") acquisitionMode->setCurrentIndex(2);
        }
    }
    catch (const Pylon::GenericException& e)
    {
        qDebug() << "loadCurrentParams error: " << e.GetDescription();
    }
}

void mPrivateWidget::getRes(QByteArray byte)
{
    MyTableWidget* button = qobject_cast<MyTableWidget*>(sender());
    QString type = button->property("Name").toString();

    if (type == "Gain")
    {
        BytePtr.insert("Gain", QJsonDocument::fromJson(byte).array());
    }
    else if (type == "ExposureTime")
    {
        BytePtr.insert("ExposureTime", QJsonDocument::fromJson(byte).array());
    }
    else if (type == "Gamma")
    {
        BytePtr.insert("Gamma", QJsonDocument::fromJson(byte).array());
    }

    m_AlgParmWidget->reLoadByte(QJsonDocument(BytePtr).toJson());
}

void mPrivateWidget::InitWidget()
{
    // 创建固定高度的标签
    auto createLabel = [this](const QString& text, int height = 30) -> QLabel* {
        QLabel* lbl = new QLabel(text, this);
        lbl->setFixedHeight(height);
        return lbl;
        };

    // 创建固定高度的下拉框
    auto createComboBox = [this](const QStringList& items, int height = 30) -> QComboBox* {
        QComboBox* cb = new QComboBox(this);
        cb->addItems(items);
        cb->setFixedHeight(height);
        return cb;
        };

    // 创建固定高度的输入框
    auto createLineEdit = [this](int height = 30) -> QLineEdit* {
        QLineEdit* le = new QLineEdit(this);
        le->setFixedHeight(height);
        return le;
        };

    // 创建固定高度的SpinBox
    auto createSpinBox = [this](int maxVal = 2000000, int height = 30) -> QSpinBox* {
        QSpinBox* sb = new QSpinBox(this);
        sb->setMaximum(maxVal);
        sb->setFixedHeight(height);
        return sb;
        };

    // 创建固定大小的pushbutton
    auto createPushButton = [this](const QString& text, int width = 50, int height = 30) -> QPushButton* {
        QPushButton* pBtn = new QPushButton(this);
        pBtn->setText(text);
        pBtn->setFixedHeight(height);
        pBtn->setFixedWidth(width);
        return pBtn;
        };

    // 创建验证器
    auto createDoubleValidator = [this](double minVal = 0.0, double maxVal = 10000000.0, int decimals = 4) -> QDoubleValidator* {
        QDoubleValidator* validator = new QDoubleValidator(minVal, maxVal, decimals, this);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setLocale(QLocale::C);
        return validator;
        };

    // 创建滚动区域
    auto createScrollArea = [this]()->QScrollArea* {
        QScrollArea* area = new QScrollArea(this);
        area->setObjectName("content");
        area->setContentsMargins(0, 0, 0, 0);
        area->setWidgetResizable(true);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        area->setFrameShape(QFrame::NoFrame);
        return area;
        };

    // 读取配置文件
    QFile file(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray byte = file.readAll();
        file.close();
        BytePtr = QJsonDocument::fromJson(byte).object();
    }

    this->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* mainHboxLayout = new QHBoxLayout(this);
    QVBoxLayout* MainLayout = new QVBoxLayout;
    QGridLayout* girlayout = new QGridLayout();
    QSplitter* Splitter = new QSplitter(Qt::Horizontal, this);
    QVBoxLayout* AlgParmLayout = new QVBoxLayout();

    // 相机参数控件创建
    {
        QLabel* cameraTitle = createLabel(tr("Basler相机参数设置"));
        cameraTitle->setObjectName("titleLabel1");

        QLabel* triggerModel = createLabel(tr("触发模式:"));
        QLabel* triggerSoure = createLabel(tr("触发源:"));
        QLabel* GammaDisableL = createLabel(tr("伽马使能"));
        QLabel* GainL = createLabel(tr("增益(db)"));
        QLabel* GamaL = createLabel(tr("伽马校正"));
        QLabel* ExposureL = createLabel(tr("曝光时间(us)"));
        QLabel* BalanceWhiteAutoL = createLabel(tr("自动白平衡"));
        QLabel* BalanceRatioL = createLabel(tr("白平衡比"));
        QLabel* BalanceRatioRL = createLabel(tr("R:"));
        QLabel* BalanceRatioGL = createLabel(tr("G:"));
        QLabel* BalanceRatioBL = createLabel(tr("B:"));

        // Basler特有参数标签
        QLabel* PixelFormatL = createLabel(tr("像素格式:"));
        QLabel* FrameRateL = createLabel(tr("帧率(fps):"));
        QLabel* AcquisitionModeL = createLabel(tr("采集模式:"));

        BalanceRatioRL->setLayoutDirection(Qt::RightToLeft);

        // 输入控件
        gain = createLineEdit();
        Gama = createLineEdit();
        Exposure = createLineEdit();
        frameRate = createLineEdit();

        // 下拉框
        first = createComboBox(QStringList() << tr("打开") << tr("关闭"));
        Second = createComboBox(QStringList() << "Action" << "Line1" << "Line2"  << tr("软触发"));
        GamaDisable = createComboBox(QStringList() << tr("打开") << tr("关闭"));
        BalanceWhiteAuto = createComboBox(QStringList() << tr("关闭") << tr("一次") << tr("连续"));

        // Basler特有下拉框
        pixelFormat = createComboBox(QStringList()
            << "Mono8" << "BayerBG8" << "BayerGB8" << "BayerRG8" << "BayerGR8"
            << "BGR8" << "RGB8");
        acquisitionMode = createComboBox(QStringList()
            << "Continuous" << "SingleFrame" << "MultiFrame");

        BalanceRatioR = createSpinBox(1000);
        BalanceRatioG = createSpinBox(1000);
        BalanceRatioB = createSpinBox(1000);

        // 隐藏触发模式
        triggerModel->hide();
        first->hide();

        BalanceRatioL->hide();
        BalanceRatioRL->hide();
        BalanceRatioR->hide();
        BalanceRatioGL->hide();
        BalanceRatioG->hide();
        BalanceRatioBL->hide();
        BalanceRatioB->hide();
        BalanceWhiteAutoL->hide();
        BalanceWhiteAuto->hide();
        // 创建验证器
        doubleValidator1 = createDoubleValidator(ExposureMin, ExposureMax, 2);
        doubleValidator2 = createDoubleValidator(gainMin, gainMax, 2);
        doubleValidator3 = createDoubleValidator(GamaMin, GamaMax, 3);
        frameRateValidator = createDoubleValidator(0, 1000, 2);

        Exposure->setValidator(doubleValidator1);
        // gain->setValidator(doubleValidator2);
        // Gama->setValidator(doubleValidator3);
        frameRate->setValidator(frameRateValidator);

        // 白平衡布局
        QHBoxLayout* hbox = new QHBoxLayout();
        hbox->addWidget(BalanceRatioRL);
        hbox->addWidget(BalanceRatioR);
        hbox->addWidget(BalanceRatioGL);
        hbox->addWidget(BalanceRatioG);
        hbox->addWidget(BalanceRatioBL);
        hbox->addWidget(BalanceRatioB);
        hbox->setContentsMargins(0, 0, 0, 0);

        // 相机参数布局
        int row = 0;
        girlayout->addWidget(triggerModel, row, 0);
        girlayout->addWidget(first, row++, 1);

        girlayout->addWidget(triggerSoure, row, 0);
        girlayout->addWidget(Second, row++, 1);

        girlayout->addWidget(GainL, row, 0);
        girlayout->addWidget(gain, row++, 1);

        girlayout->addWidget(GammaDisableL, row, 0);
        girlayout->addWidget(GamaDisable, row++, 1);

        girlayout->addWidget(GamaL, row, 0);
        girlayout->addWidget(Gama, row++, 1);

        girlayout->addWidget(ExposureL, row, 0);
        girlayout->addWidget(Exposure, row++, 1);

        girlayout->addWidget(PixelFormatL, row, 0);
        girlayout->addWidget(pixelFormat, row++, 1);

        girlayout->addWidget(FrameRateL, row, 0);
        girlayout->addWidget(frameRate, row++, 1);

        girlayout->addWidget(AcquisitionModeL, row, 0);
        girlayout->addWidget(acquisitionMode, row++, 1);

        girlayout->addWidget(BalanceWhiteAutoL, row, 0);
        girlayout->addWidget(BalanceWhiteAuto, row++, 1);

        girlayout->addWidget(BalanceRatioL, row, 0);
        girlayout->addLayout(hbox, row++, 1);


        girlayout->setColumnStretch(0, 1);
        girlayout->setColumnStretch(1, 2);
        girlayout->setContentsMargins(16, 0, 8, 0);
        girlayout->setSpacing(3);
        girlayout->setAlignment(Qt::AlignTop);

        QScrollArea* widgetCameraParamScroll = createScrollArea();
        QWidget* widgetCameraParam = new QWidget(this);
        QVBoxLayout* paramLayout = new QVBoxLayout();
        widgetCameraParam->setObjectName("content");

        QHBoxLayout* cameraTitleLayout = new QHBoxLayout();
        cameraTitleLayout->addWidget(cameraTitle);
        cameraTitleLayout->setContentsMargins(0, 0, 0, 0);

        paramLayout->addLayout(cameraTitleLayout, 0);
        paramLayout->addLayout(girlayout, 1);
        paramLayout->setContentsMargins(0, 0, 0, 0);

        widgetCameraParam->setLayout(paramLayout);
        widgetCameraParam->setContentsMargins(0, 0, 0, 0);
        widgetCameraParamScroll->setWidget(widgetCameraParam);

        AlgParmLayout->addWidget(widgetCameraParamScroll);
        AlgParmLayout->setContentsMargins(0, 0, 0, 0);
    }

    // 流程参数控件创建
    {
        QLabel* CountL = createLabel(tr("一次信号取图次数"));
        QLabel* TimeOutL = createLabel(tr("单张图超时时间"));
        QLabel* liucTitle = createLabel(tr("流程参数设置"));
        liucTitle->setObjectName("titleLabel1");

        Count = createSpinBox();
        timeout = createSpinBox(10000);
        QStackedWidget* changeWidget = new QStackedWidget(this);

        saveBtn = new QPushButton(tr("保存"), this);
        saveBtn->setMinimumHeight(30);
        saveBtn->setObjectName("borderbutton");

        // 创建表格并设置数据
        gainTable = new MyTableWidget(this, BytePtr.value("Gain").toArray());
        GamaTable = new MyTableWidget(this, BytePtr.value("Gamma").toArray());
        ExposureTimeTable = new MyTableWidget(this, BytePtr.value("ExposureTime").toArray());

        gainTable->setProperty("Name", "Gain");
        GamaTable->setProperty("Name", "Gamma");
        ExposureTimeTable->setProperty("Name", "ExposureTime");

        showTable = new QTableWidget(this);
        Add = createPushButton(tr("添加"));
        Delete = createPushButton(tr("删除"));
        takeEffect = createPushButton(tr("设定生效"), 110);

        QLabel* tips = createLabel(tr("注意:设定生效按钮点击时,当前界面设定生效,请在未生产时操作"));
        tips->setStyleSheet("color: rgb(255, 0, 0);");

        QStringList headers;
        headers << tr("id") << tr("Exposure") << tr("Gain") << tr("Gamma");
        showTable->setColumnCount(headers.size());
        showTable->setHorizontalHeaderLabels(headers);
        showTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        showTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        showTable->verticalHeader()->setVisible(false);

        // 设置默认值
        Count->setValue(m_Camerahandle->m_sdkFunc->getImageMaxCoiunts);
        timeout->setValue(m_Camerahandle->m_sdkFunc->timeOut);

        // 流程参数布局
        QGridLayout* girlayout_param = new QGridLayout();
        girlayout_param->addWidget(TimeOutL, 0, 0);
        girlayout_param->addWidget(timeout, 0, 1);
        girlayout_param->addWidget(CountL, 1, 0);
        girlayout_param->addWidget(Count, 1, 1);
        girlayout_param->setColumnStretch(0, 1);
        girlayout_param->setColumnStretch(1, 2);
        girlayout_param->setContentsMargins(16, 8, 0, 0);
        girlayout_param->setSpacing(3);

        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        btnLayout->addWidget(takeEffect);
        btnLayout->addWidget(Add);
        btnLayout->addWidget(Delete);
        btnLayout->setContentsMargins(16, 20, 0, 0);

        QScrollArea* widgetLiucScroll = createScrollArea();
        QWidget* widgetLiuc = new QWidget(this);
        widgetLiuc->setObjectName("content");
        widgetLiuc->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout* vLayout = new QVBoxLayout();
        QHBoxLayout* liucTitleLayout = new QHBoxLayout();
        liucTitleLayout->addWidget(liucTitle);

        vLayout->addLayout(liucTitleLayout, 0);
        vLayout->addLayout(girlayout_param, 1);
        vLayout->addLayout(btnLayout);
        vLayout->addWidget(showTable);
        vLayout->addWidget(changeWidget);
        vLayout->addWidget(tips);
        vLayout->addWidget(saveBtn);
        vLayout->setContentsMargins(0, 8, 8, 8);
        vLayout->setSpacing(3);

        widgetLiuc->setLayout(vLayout);
        widgetLiucScroll->setWidget(widgetLiuc);
        AlgParmLayout->addWidget(widgetLiucScroll);
        AlgParmLayout->setSpacing(6);

        changeWidget->addWidget(gainTable);
        changeWidget->addWidget(GamaTable);
        changeWidget->addWidget(ExposureTimeTable);
        changeWidget->setContentsMargins(8, 0, 0, 0);
        changeWidget->hide();
    }

    // 参数json显示界面
    {
        m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
        m_AlgParmWidget->hide();
    }

    // 图片显示界面
    {
        SetDataBtn = createPushButton(tr("软触发"), 100);
        ContinuesBtn = createPushButton(tr("连续取图"), 100);
        Details = createPushButton(tr("详情"), 100);
        Details->hide();

        m_showimage = new viewWidget();
        m_showimage->setMinimumSize(500, 500);
        QLabel* title = createLabel("Basler相机");

        QHBoxLayout* MainBtnLayout = new QHBoxLayout;
        MainBtnLayout->addWidget(title);
        MainBtnLayout->addStretch();
        MainBtnLayout->addWidget(ContinuesBtn);
        MainBtnLayout->addWidget(SetDataBtn);
        MainBtnLayout->addWidget(Details);

        MainLayout->addLayout(MainBtnLayout);
        MainLayout->addWidget(m_showimage);
    }

    // 连接相机图像回调
    auto func = std::bind(&mPrivateWidget::showImage, this, std::placeholders::_1);
    m_Camerahandle->m_sdkFunc->registerGetImageFun(func);
    // 设置分割器
    QWidget* mainLayoutWidget = new QWidget(Splitter);
    mainLayoutWidget->setLayout(MainLayout);

    AlgParmLayout->setStretch(0, 2);
    AlgParmLayout->setStretch(1, 5);

    QWidget* algParmLayoutWidget = new QWidget(Splitter);
    algParmLayoutWidget->setLayout(AlgParmLayout);
    algParmLayoutWidget->setContentsMargins(0, 0, 0, 0);

    Splitter->addWidget(mainLayoutWidget);
    Splitter->addWidget(algParmLayoutWidget);
    Splitter->addWidget(m_AlgParmWidget);

    QList<int> ratios;
    ratios << 4 << 3 << 0;
    int totalRatio = 7;

    QTimer::singleShot(10, this, [=]() {
        int totalWidth = Splitter->width();
        QList<int> splitterSizes;
        for (int ratio : ratios) {
            int size = (totalWidth * ratio) / totalRatio;
            splitterSizes << size;
        }
        Splitter->setSizes(splitterSizes);
        });

    Splitter->setContentsMargins(0, 0, 0, 0);
    mainHboxLayout->addWidget(Splitter);
    mainHboxLayout->setContentsMargins(0, 0, 0, 0);

    // 加载当前参数
    loadCurrentParams();

    // 初始化表格
    gainTable->initData();
    GamaTable->initData();
    ExposureTimeTable->initData();

    // 创建信号连接
    createConnect();
}

void mPrivateWidget::createConnect()
{
    // 触发模式
    connect(first, &QComboBox::currentTextChanged, this, [=](QString text) {
        bool on = (text == tr("打开"));
        m_Camerahandle->m_sdkFunc->setTriggerMode(on);
        m_Camerahandle->trigged(on ? 1 : 0);

        //m_Camerahandle->m_sdkFunc->grabOneImage();
        });

    // 触发源
    connect(Second, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        int source = BASLER_TRIGGER_SOURCE_ACTION;
        if (text == "Action") source = BASLER_TRIGGER_SOURCE_ACTION;
        else if (text == "Line1") source = BASLER_TRIGGER_SOURCE_LINE1;
        else if (text == "Line2") source = BASLER_TRIGGER_SOURCE_LINE2;
        else if (text == tr("软触发")) source = BASLER_TRIGGER_SOURCE_SOFTWARE;

        m_Camerahandle->m_sdkFunc->setTriggerSource(source);
        //m_Camerahandle->type1 = (source == BASLER_TRIGGER_SOURCE_SOFTWARE) ? 1 : 0;
        });

    // 连续取图按钮
    connect(ContinuesBtn, &QPushButton::clicked, this, [=]() {
        if (ContinuesBtn->text() == tr("连续取图"))
        {
            first->setCurrentIndex(1);
            ContinuesBtn->setText(tr("停止取图"));
            SetDataBtn->setEnabled(false);

        }
        else
        {
            first->setCurrentIndex(0);
            ContinuesBtn->setText(tr("连续取图"));
            SetDataBtn->setEnabled(true);
        }
        });

    // 增益设置
    connect(gain, &QLineEdit::editingFinished, this, [=]() {
        m_Camerahandle->m_sdkFunc->setGain(gain->text().toInt());
        });

    // 伽马设置
    connect(Gama, &QLineEdit::editingFinished, this, [=]() {
        m_Camerahandle->m_sdkFunc->setGamma(float(Gama->text().toFloat()));
        });

    // 曝光时间设置
    connect(Exposure, &QLineEdit::editingFinished, this, [=]() {
        m_Camerahandle->m_sdkFunc->setExposureTime(float(Exposure->text().toFloat()));
        });

    // 像素格式设置
    connect(pixelFormat, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        m_Camerahandle->m_sdkFunc->setPixelFormat(text);
        });

    // 帧率设置
    connect(frameRate, &QLineEdit::editingFinished, this, [=]() {
        try
        {
            auto& camera = *m_Camerahandle->m_sdkFunc->camera;
            if (camera.IsOpen() && camera.AcquisitionFrameRateAbs.IsWritable())
            {
                camera.AcquisitionFrameRateAbs.SetValue(frameRate->text().toDouble());
            }
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << "Set frame rate error: " << e.GetDescription();
        }
        });

    // 采集模式设置
    connect(acquisitionMode, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        m_Camerahandle->m_sdkFunc->setAcquisitionMode(text);
        });

    // 伽马使能
    connect(GamaDisable, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        try
        {
            auto& camera = *m_Camerahandle->m_sdkFunc->camera;
            if (!camera.IsOpen()) return;

            bool enable = (text == tr("打开"));
            if (camera.GammaEnable.IsWritable())
            {
                camera.GammaEnable.SetValue(enable);
            }
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << "Set gamma enable error: " << e.GetDescription();
        }
        });

    // 白平衡设置
    connect(BalanceWhiteAuto, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        try
        {
            auto& camera = *m_Camerahandle->m_sdkFunc->camera;
            if (!camera.IsOpen() || !camera.BalanceWhiteAuto.IsWritable())
                return;

            if (text == tr("关闭"))
            {
                camera.BalanceWhiteAuto.SetValue(Basler_UniversalCameraParams::BalanceWhiteAuto_Off);
                BalanceRatioR->setEnabled(true);
                BalanceRatioG->setEnabled(true);
                BalanceRatioB->setEnabled(true);

                if (camera.BalanceRatioSelector.IsWritable())
                {
                    camera.BalanceRatioSelector.SetValue(Basler_UniversalCameraParams::BalanceRatioSelector_Red);
                    BalanceRatioR->setValue((int)camera.BalanceRatio.GetValue());

                    camera.BalanceRatioSelector.SetValue(Basler_UniversalCameraParams::BalanceRatioSelector_Green);
                    BalanceRatioG->setValue((int)camera.BalanceRatio.GetValue());

                    camera.BalanceRatioSelector.SetValue(Basler_UniversalCameraParams::BalanceRatioSelector_Blue);
                    BalanceRatioB->setValue((int)camera.BalanceRatio.GetValue());
                }
            }
            else if (text == tr("一次"))
            {
                camera.BalanceWhiteAuto.SetValue(Basler_UniversalCameraParams::BalanceWhiteAuto_Once);
                BalanceRatioR->setEnabled(false);
                BalanceRatioG->setEnabled(false);
                BalanceRatioB->setEnabled(false);
            }
            else if (text == tr("连续"))
            {
                camera.BalanceWhiteAuto.SetValue(Basler_UniversalCameraParams::BalanceWhiteAuto_Continuous);
                BalanceRatioR->setEnabled(false);
                BalanceRatioG->setEnabled(false);
                BalanceRatioB->setEnabled(false);
            }
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << "Set white balance error: " << e.GetDescription();
        }
        });

    // 白平衡比值设置
    connect(BalanceRatioR, &QSpinBox::editingFinished, this, [=]() {
        try
        {
            auto& camera = *m_Camerahandle->m_sdkFunc->camera;
            if (camera.IsOpen() && camera.BalanceRatioSelector.IsWritable())
            {
                camera.BalanceRatioSelector.SetValue(Basler_UniversalCameraParams::BalanceRatioSelector_Red);
                camera.BalanceRatio.SetValue(BalanceRatioR->value());
            }
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << "Set red balance error: " << e.GetDescription();
        }
        });

    connect(BalanceRatioG, &QSpinBox::editingFinished, this, [=]() {
        try
        {
            auto& camera = *m_Camerahandle->m_sdkFunc->camera;
            if (camera.IsOpen() && camera.BalanceRatioSelector.IsWritable())
            {
                camera.BalanceRatioSelector.SetValue(Basler_UniversalCameraParams::BalanceRatioSelector_Green);
                camera.BalanceRatio.SetValue(BalanceRatioG->value());
            }
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << "Set green balance error: " << e.GetDescription();
        }
        });

    connect(BalanceRatioB, &QSpinBox::editingFinished, this, [=]() {
        try
        {
            auto& camera = *m_Camerahandle->m_sdkFunc->camera;
            if (camera.IsOpen() && camera.BalanceRatioSelector.IsWritable())
            {
                camera.BalanceRatioSelector.SetValue(Basler_UniversalCameraParams::BalanceRatioSelector_Blue);
                camera.BalanceRatio.SetValue(BalanceRatioB->value());
            }
        }
        catch (const Pylon::GenericException& e)
        {
            qDebug() << "Set blue balance error: " << e.GetDescription();
        }
        });

    // 软触发按钮
    connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
        std::vector<cv::Mat> mats;
        QStringList list;
        //emit m_Camerahandle->trigged(1000);
        //m_Camerahandle->m_sdkFunc->grabOneImage();
        m_Camerahandle->setData(mats, list);
        m_Camerahandle->data(mats, list);
        if (!mats.empty())
        {
            QImage showImg = cvMatToQImage(mats.at(0));
            m_showimage->reciveImage("", showImg);
        }
        

        });

    // JSON参数修改
    connect(m_AlgParmWidget, &AlgParmWidget::SengCurrentByte, this, [=](QByteArray byte) {
        QJsonObject paramObj = QJsonDocument::fromJson(byte).object();
        QMap<QString, QString> ParameterMap;
        for (auto objStr : paramObj.keys())
        {
            if (paramObj.value(objStr).isString())
                ParameterMap.insert(objStr, paramObj.value(objStr).toString());
            else if (paramObj.value(objStr).isArray())
            {
                m_Camerahandle->m_sdkFunc->setArrayByte(objStr, paramObj.value(objStr).toArray());
            }
        }
        m_Camerahandle->setParameter(ParameterMap);
        });

    // 流程参数修改
    connect(Count, &QSpinBox::editingFinished, this, [=]() {
        int currentValue = Count->value();
        m_Camerahandle->m_sdkFunc->getImageMaxCoiunts = currentValue;
        m_Camerahandle->m_sdkFunc->ParasValueMap["OnceSignalsGetImageCounts"] = QString::number(currentValue);
        BytePtr["OnceSignalsGetImageCounts"] = QString::number(currentValue);
        m_AlgParmWidget->reLoadByte(QJsonDocument(BytePtr).toJson());
        });

    connect(timeout, &QSpinBox::editingFinished, this, [=]() {
        int currentValue = timeout->value();
        m_Camerahandle->m_sdkFunc->timeOut = currentValue;
        m_Camerahandle->m_sdkFunc->ParasValueMap["GetOnceImageTimes"] = QString::number(currentValue);
        BytePtr["GetOnceImageTimes"] = QString::number(currentValue);
        m_AlgParmWidget->reLoadByte(QJsonDocument(BytePtr).toJson());
        });

    // 表格数据变化连接
    connect(gainTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
    connect(GamaTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
    connect(ExposureTimeTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);

    // 保存按钮
    connect(saveBtn, &QPushButton::clicked, this, [=]() {
        if (m_AlgParmWidget)
            emit m_AlgParmWidget->save();
        });

    // 设定生效按钮
    connect(takeEffect, &QPushButton::clicked, this, [=]() {
        auto clickMyTableTakeEffectButton = [](MyTableWidget* tableWidget) {
            if (!tableWidget) return;
            QToolButton* takeEffectBtn = tableWidget->getTakeEffectButton();
            if (takeEffectBtn) {
                QMouseEvent pressEvent(QEvent::MouseButtonPress, takeEffectBtn->rect().center(),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(takeEffectBtn, &pressEvent);
                QMouseEvent releaseEvent(QEvent::MouseButtonRelease, takeEffectBtn->rect().center(),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(takeEffectBtn, &releaseEvent);
            }
            };
        clickMyTableTakeEffectButton(ExposureTimeTable);
        clickMyTableTakeEffectButton(gainTable);
        clickMyTableTakeEffectButton(GamaTable);
        });

    // 添加行
    connect(Add, &QPushButton::clicked, this, [=]() {
        auto createCenteredItem = [](const QString& text) -> QTableWidgetItem* {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            return item;
            };

        int row = showTable->rowCount();
        showTable->insertRow(row);
        showTable->setItem(row, 0, createCenteredItem(""));
        showTable->setItem(row, 1, createCenteredItem(""));
        showTable->setItem(row, 2, createCenteredItem(""));
        showTable->setItem(row, 3, createCenteredItem(""));

        auto clickMyTableAddButton = [](MyTableWidget* tableWidget) {
            if (!tableWidget) return;
            QToolButton* addBtn = tableWidget->getAddRowButton();
            if (addBtn) {
                QMouseEvent pressEvent(QEvent::MouseButtonPress, addBtn->rect().center(),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(addBtn, &pressEvent);
                QMouseEvent releaseEvent(QEvent::MouseButtonRelease, addBtn->rect().center(),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(addBtn, &releaseEvent);
            }
            };

        clickMyTableAddButton(ExposureTimeTable);
        clickMyTableAddButton(gainTable);
        clickMyTableAddButton(GamaTable);
        });

    // 删除行
    connect(Delete, &QPushButton::clicked, this, [=]() {
        QItemSelectionModel* selectionModel = showTable->selectionModel();
        QSet<int> selectedRows;
        QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
        foreach(QModelIndex index, selectedIndexes) {
            if (index.isValid()) {
                selectedRows.insert(index.row());
            }
        }

        if (selectedRows.isEmpty()) return;

        QList<int> rows = selectedRows.values();
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        for (int row : rows) {
            ExposureTimeTable->removeTableRow(row);
            gainTable->removeTableRow(row);
            GamaTable->removeTableRow(row);
            showTable->removeRow(row);
        }
        });

    // 表格单元格编辑
    connect(showTable, &QTableWidget::cellDoubleClicked, this, [=](int row, int col) {
        if (col != 1 && col != 2 && col != 3) return;
        QTableWidgetItem* item = showTable->item(row, col);
        if (item) {
            cellOriginalValues[QPair<int, int>(row, col)] = item->text().trimmed();
        }
        });

    connect(showTable, &QTableWidget::cellChanged, this, [=](int row, int col) {
        auto validateTableCell = [](const QString& text, QDoubleValidator* validator) -> bool {
            if (!validator) return true;
            if (text.isEmpty()) return true;
            int pos = 0;
            return validator->validate(const_cast<QString&>(text), pos) == QValidator::Acceptable;
            };

        QTableWidgetItem* item = showTable->item(row, col);
        if (!item) return;

        QString text = item->text().trimmed();

        if (col != 0)
        {
            bool isValid = true;
            switch (col) {
            case 1:
                isValid = validateTableCell(text, doubleValidator1);
                break;
            case 2:
                isValid = validateTableCell(text, doubleValidator2);
                break;
            case 3:
                isValid = validateTableCell(text, doubleValidator3);
                break;
            default:
                return;
            }

            if (!isValid) {
                QString originalValue = cellOriginalValues.value(QPair<int, int>(row, col), "");
                QMessageBox::warning(this, tr("输入非法"),
                    tr("合法范围：%1 ~ %2")
                    .arg(col == 1 ? ExposureMin : (col == 2 ? gainMin : GamaMin))
                    .arg(col == 1 ? ExposureMax : (col == 2 ? gainMax : GamaMax)));
                item->setText(originalValue);
                item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                return;
            }

            cellOriginalValues[QPair<int, int>(row, col)] = text;

            if (col == 1)
                ExposureTimeTable->setItemData(row, 1, text);
            else if (col == 2)
                gainTable->setItemData(row, 1, text);
            else if (col == 3)
                GamaTable->setItemData(row, 1, text);
        }
        else
        {
            bool isInt = false;
            text.toInt(&isInt);
            if (!isInt && !text.isEmpty())
            {
                QString originalValue = cellOriginalValues.value(QPair<int, int>(row, col), "");
                QMessageBox::warning(this, tr("输入非法"), tr("请输入整数"));
                item->setText(originalValue);
                return;
            }

            ExposureTimeTable->setItemData(row, 0, text);
            gainTable->setItemData(row, 0, text);
            GamaTable->setItemData(row, 0, text);
        }
        });

    // 表格行添加信号
    connect(ExposureTimeTable, &MyTableWidget::addNewLine, this, [=](int row, int col, QString value) {
        showTable->blockSignals(true);
        int rowCount = showTable->rowCount();
        if (rowCount < row + 1) showTable->setRowCount(row + 1);
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        showTable->setItem(row, col, item);
        showTable->blockSignals(false);
        });

    connect(gainTable, &MyTableWidget::addNewLine, this, [=](int row, int col, QString value) {
        showTable->blockSignals(true);
        int rowCount = showTable->rowCount();
        if (rowCount < row + 1) showTable->setRowCount(row + 1);
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        if (col != 0)
            showTable->setItem(row, col + 1, item);
        showTable->blockSignals(false);
        });

    connect(GamaTable, &MyTableWidget::addNewLine, this, [=](int row, int col, QString value) {
        showTable->blockSignals(true);
        int rowCount = showTable->rowCount();
        if (rowCount < row + 1) showTable->setRowCount(row + 1);
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        if (col != 0)
            showTable->setItem(row, col + 2, item);
        showTable->blockSignals(false);
        });
}
