#include "Hd_CameraModule_HIK3.h"

struct OnePb
{
    PbGlobalObject* base = nullptr;
    QWidget* baseWidget = nullptr;
    QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;
MV_CC_DEVICE_INFO_LIST m_stDevList;//相机设备
void __stdcall ReconnectDevice(unsigned int nMsgType, void* pUser0);

void __stdcall ImageCallBackEx(unsigned char* pData0, MV_FRAME_OUT_INFO_EX* pFrameInfo0, void* pUser0);

void CloseDevice(void* handle)
{
    MV_CC_StopGrabbing(handle);
    MV_CC_CloseDevice(handle);
    MV_CC_DestroyHandle(handle);
}

bool IsColor(MvGvspPixelType enType);

int SearchDevice()
{
    memset(&m_stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    // ch:枚举子网内所有设备 | en:Enumerate all devices within subnetgit
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE || MV_USB_DEVICE, &m_stDevList);
    if (MV_OK != nRet)
    {
        return 0;
    }
    if (m_stDevList.nDeviceNum == 0)
        return 0;
    return 1;
}

bool connctDevice(string GetSnName, void* handle, void* pUser)
{
    CameraFunSDKfactoryCls* CurrentCamera = (CameraFunSDKfactoryCls*)(pUser);
    if (!SearchDevice())
        return false;
    int index = 0;
    bool findflad = false;

    for (; index < m_stDevList.nDeviceNum; index++)
    {

        unsigned char* name = m_stDevList.pDeviceInfo[index]->SpecialInfo.stGigEInfo.chSerialNumber;
        //userID
        //unsigned char* name=m_stDevList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chUserDefinedName;
        string SnName = static_cast<string>((LPCSTR)name);
        if (SnName == GetSnName)
        {
            findflad = true;
            break;

        }
    }
    if (!findflad)
		return false;
    unsigned char* name = m_stDevList.pDeviceInfo[index]->SpecialInfo.stGigEInfo.chUserDefinedName;
    string userName = static_cast<string>((LPCSTR)name);
    qDebug() << "--UserName:" << QString::fromStdString(userName);
    if (handle != NULL)
    {
        MV_CC_StopGrabbing(handle);
        MV_CC_CloseDevice(handle);
        MV_CC_DestroyHandle(handle);
    }

    //打开相机
    MV_CC_CreateHandle(&handle, m_stDevList.pDeviceInfo[index]);
    int nRet = MV_CC_OpenDevice(handle);
    ////触发模式
    //MV_CC_SetEnumValue(handle, "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
    MV_CC_SetEnumValue(handle, "TriggerMode", MV_TRIGGER_MODE_ON);
    //   //加载用户集，相机设置应当UserSet1
    nRet = MV_CC_SetEnumValueByString(handle, "UserSetSelector", "UserSet1");
    if (MV_OK != nRet)
    {
        qDebug() << "Set UserSetSelector fail! nRet " << nRet;
    }
    // ch:注册抓图回调 | en:Register image callback
    MV_CC_SetIntValueEx(handle, "GevHeartbeatTimeout", 10000);//心跳日志

    nRet = MV_CC_RegisterImageCallBackEx(handle, ImageCallBackEx, CurrentCamera);//注册回调
    if (MV_OK != nRet)
        return false;
    nRet = MV_CC_RegisterExceptionCallBack(handle, ReconnectDevice, CurrentCamera);//断线重连
    if (MV_OK != nRet)
        return false;
    MV_CC_StartGrabbing(handle);
    CurrentCamera->handle = std::move(handle);
    return true;
}

void __stdcall ReconnectDevice(unsigned int nMsgType, void* pUser)
{
    CameraFunSDKfactoryCls* CurrentCamera = reinterpret_cast<CameraFunSDKfactoryCls*>(pUser);
    qDebug() << "[Error] " << " MV camera disconnects!";
    emit CurrentCamera->trigged(1);
    if (nMsgType == MV_EXCEPTION_DEV_DISCONNECT)
    {
        //断开连接
        MV_CC_CloseDevice(CurrentCamera->handle);
        int nRet = MV_CC_DestroyHandle(CurrentCamera->handle);
        CurrentCamera->handle = nullptr;
        BOOL bConnected = FALSE;
        while (1)
        {
            Sleep(100);
            if (connctDevice(CurrentCamera->SnCode, CurrentCamera->handle, CurrentCamera) == true)
            {
                qWarning() << "[Hd_CameraModule_HIK] " << "  Hd_CameraModule_HIK create success again! ";
                emit CurrentCamera->trigged(0);
                break;
            }

        }
    }
}

void __stdcall ImageCallBackEx(unsigned char* pData0, MV_FRAME_OUT_INFO_EX* pFrameInfo0, void* pUser0)
{
    double time_Start = (double)clock();
    int frameNum = 0;
    cv::Mat srcImage = cv::Mat();
    QList<cv::Mat> OutMats;
    CameraFunSDKfactoryCls* CurrentCamera = (CameraFunSDKfactoryCls*)(pUser0);
    qDebug() << CurrentCamera << pUser0;
    if (pFrameInfo0)
    {
        //获取的是单通道灰度图
        auto& stImageInfo = *pFrameInfo0;
        if (pFrameInfo0->enPixelType == PixelType_Gvsp_Mono8)//获取当前采集到的图像格式
        {
            srcImage = cv::Mat(pFrameInfo0->nHeight, pFrameInfo0->nWidth, CV_8UC1, pData0);
        }
        //获取的是RGB8图
        else if (pFrameInfo0->enPixelType == PixelType_Gvsp_BGR8_Packed)
        {
            srcImage = cv::Mat(pFrameInfo0->nHeight, pFrameInfo0->nWidth, CV_8UC3, pData0);
        }

        ////获取的是RGB8图
        //else if (pFrameInfo0->enPixelType == PixelType_Gvsp_RGB8_Packed)
        //{
        //    cv::Mat dst = cv::Mat(pFrameInfo0->nHeight, pFrameInfo0->nWidth, CV_8UC3, pData0);
        //    //cv::cvtColor(matimage, matimage, cv::COLOR_RGB2BGR);
        //    srcImage = cv::Mat::zeros(pFrameInfo0->nHeight, pFrameInfo0->nWidth, CV_8UC3);
        //    std::vector<cv::Mat> channels;
        //    cv::split(dst, channels);//分割matimage的通道
        //    std::vector<cv::Mat> dstchannels;
        //    for (int i = 2; i >= 0; i--)
        //    {
        //        dstchannels.push_back(channels[i]);
        //    }
        //    merge(dstchannels, srcImage);
        //}

        else if (IsColor(pFrameInfo0->enPixelType)) //其它格式彩色图
        {
            MvGvspPixelType enDstPixelType = PixelType_Gvsp_BGR8_Packed;
            srcImage = cv::Mat(stImageInfo.nHeight, stImageInfo.nWidth, CV_8UC3);

            unsigned int m_nBufSizeForSaveImage = stImageInfo.nWidth * stImageInfo.nHeight * 3;
            unsigned char* m_pBufForSaveImage = srcImage.data;

            //转换图像格式为BGR8
            MV_CC_PIXEL_CONVERT_PARAM stConvertParam = { 0 };
            memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));
            stConvertParam.nWidth = stImageInfo.nWidth;                 //ch:图像宽 | en:image width
            stConvertParam.nHeight = stImageInfo.nHeight;               //ch:图像高 | en:image height
            stConvertParam.pSrcData = pData0;                  //ch:输入数据缓存 | en:input data buffer
            stConvertParam.nSrcDataLen = stImageInfo.nFrameLen;         //ch:输入数据大小 | en:input data size

            stConvertParam.enSrcPixelType = stImageInfo.enPixelType;    //ch:输入像素格式 | en:input pixel format
            stConvertParam.enDstPixelType = PixelType_Gvsp_BGR8_Packed; //ch:输出像素格式 | en:output pixel format  适用于OPENCV的图像格式
            //stConvertParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed; //ch:输出像素格式 | en:output pixel format
            stConvertParam.pDstBuffer = m_pBufForSaveImage;                    //ch:输出数据缓存 | en:output data buffer
            stConvertParam.nDstBufferSize = m_nBufSizeForSaveImage;            //ch:输出缓存大小 | en:output buffer size
            //testflag.store(true, std::memory_order::memory_order_seq_cst);
            //for (int i = 0; i < 300; i++)
            // {
            //QThread::msleep(100);
            MV_CC_ConvertPixelType(CurrentCamera->handle, &stConvertParam);
            //}
            //testflag.store(false, std::memory_order::memory_order_seq_cst);
        }
        if (srcImage.empty())
        {
            srcImage = cv::Mat(5, 5, CV_8UC1).setTo(0);
        }

    }
    if (CurrentCamera->triggerMode.load(std::memory_order::memory_order_acquire) == 0)
    {
        CurrentCamera->triggerOffBack(srcImage);
        return;
    }
    //if (CurrentCamera->allowflag.load(std::memory_order::memory_order_acquire))
    {
        OutMats.push_back(srcImage.clone());
        if (CurrentCamera->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
        {
            if (CurrentCamera->allowflag.load(std::memory_order::memory_order_acquire))
                CurrentCamera->MatQueue.push(OutMats);
        }
        else
        {
            qDebug() << CurrentCamera->Currentindex<<"图像数量"<< OutMats.size();
            ///硬触发不受开关控制，没有缓存
            if (CurrentCamera->CallbackFuncMap.keys().contains(CurrentCamera->Currentindex))
            {
                QObject* obj = CurrentCamera->CallbackFuncMap.value(CurrentCamera->Currentindex).callbackparent;
                obj->setProperty("cameraIndex", QString::number(CurrentCamera->Currentindex));
                //QDateTime curT = QDateTime::currentDateTime();
				//QString imageName = curT.toString("hh_mm_ss_zzz") + "_" + QString::number(CurrentCamera->Currentindex) + "_" + QString::number(pFrameInfo0->nFrameNum);
				//cv::imwrite(QString(CurrentCamera->RootPath + imageName + ".jpg").toStdString(), srcImage);

                CurrentCamera->CallbackFuncMap.value(CurrentCamera->Currentindex).GetimagescallbackFunc(obj, OutMats);
            }
            else
            {
                qWarning() << "CallbackFuncMap.keys()" << CurrentCamera->CallbackFuncMap.keys() << CurrentCamera->Currentindex;
            }
        }
    }
    CurrentCamera->Currentindex++;
    if (CurrentCamera->exposureTimeMap.count(CurrentCamera->Currentindex) == 1)
    {
        MV_CC_SetFloatValue(CurrentCamera->handle, "ExposureTime", CurrentCamera->exposureTimeMap[CurrentCamera->Currentindex]);
		qDebug() << "ExposureTime" << CurrentCamera->exposureTimeMap[CurrentCamera->Currentindex];
    }
    if (CurrentCamera->gainMap.count(CurrentCamera->Currentindex) == 1)
    {
        MV_CC_SetFloatValue(CurrentCamera->handle, "Gain", CurrentCamera->gainMap[CurrentCamera->Currentindex]);
		qDebug() << "Gain" << CurrentCamera->gainMap[CurrentCamera->Currentindex];
    }
    if (CurrentCamera->gammaMap.count(CurrentCamera->Currentindex) == 1)
    {
        MV_CC_SetFloatValue(CurrentCamera->handle, "Gamma", CurrentCamera->gammaMap[CurrentCamera->Currentindex]);
		qDebug() << "Gamma" << CurrentCamera->gammaMap[CurrentCamera->Currentindex];
    }
    if (CurrentCamera->Currentindex >= CurrentCamera->getImageMaxCoiunts / CurrentCamera->OnceGetImageNum)	CurrentCamera->Currentindex = 0;

    pData0 = { 0 };
    QDateTime curT = QDateTime::currentDateTime();
    double time_End = (double)clock();

    qDebug() << " checkImg getImg,Time:" << (time_End - time_Start) << "ms"
             << "--CameraName:" << QString::fromLocal8Bit(CurrentCamera->SnCode.c_str()) << " timepoint " << curT.toString("hh:mm:ss.zzz")\
             << " imgIndex:" << CurrentCamera->Currentindex << "nFrameNum : " << pFrameInfo0->nFrameNum;
}

CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
{
    CloseDevice(handle);
}

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{

    if (!connctDevice(SnCode, getHandle(), this))
    {
        emit trigged(1);
        return false;
    }
    emit trigged(0);

    //qDebug() << getHandle();
    MVCC_ENUMVALUE stEnumValue = { 0 };
    MV_CC_GetEnumValue(handle, "TriggerSource", &stEnumValue);
    m_MV_CAM_TRIGGER_SOURCE = (MV_CAM_TRIGGER_SOURCE)stEnumValue.nCurValue;
    //qDebug() << stEnumValue.nCurValue;
    return true;

}

void CameraFunSDKfactoryCls::upDateParam()
{
    getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts").toInt();
    OnceGetImageNum = ParasValueMap.value("OnceImageCounts").toInt();
    timeOut = ParasValueMap.value("GetOnceImageTimes").toInt();
    qDebug() << getImageMaxCoiunts << OnceGetImageNum;
    return;
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
//类创建
Hd_CameraModule_HIK3::Hd_CameraModule_HIK3(QString sn, QString path, int settype, QObject* parent)
    : PbGlobalObject(settype, parent), Sncode(sn), RootPath(path)
{
    famliy = CAMERA2D;
    JsonFilePath = RootPath + Sncode + ".json";
    QString FirstCreateByte(R"({
	"SeralNum": ")" + sn + R"(",
	"GetOnceImageTimes": "1000",
	"LastUpdateTime": "",
	"OnceImageCounts":"1",
	"OnceSignalsGetImageCounts":"20",
	"ExposureTime":[],
	"Gain":[],
	"Gamma":[]})");

    if (!QFile(JsonFilePath).exists())
        createAndWritefile(JsonFilePath, FirstCreateByte.toUtf8());
    QJsonObject paramObj = load_JsonFile(JsonFilePath);
    for (auto objStr : paramObj.keys())
    {
        ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
    }
    m_sdkFunc = new  CameraFunSDKfactoryCls(Sncode, RootPath);
    connect(m_sdkFunc, &CameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });
}

Hd_CameraModule_HIK3::~Hd_CameraModule_HIK3()
{
    if (m_sdkFunc)
    {
        delete m_sdkFunc;
    }
}
//setParameter之后再调用，返回当前参数
//相机：获取默认参数；
//通信：获取初始化示例参数
QMap<QString, QString> Hd_CameraModule_HIK3::parameters()
{
    return ParasValueMap;
}
//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_HIK3::setParameter(const QMap<QString, QString>& ParameterMap)
{
    ParasValueMap = ParameterMap;
    m_sdkFunc->ParasValueMap = ParasValueMap;

    m_sdkFunc->upDateParam();
    return true;
}
//初始化(加载模块待内存)
bool Hd_CameraModule_HIK3::init()
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
        else if (Code == 11)
        {
            m_sdkFunc->triggerMode.store(1, std::memory_order::memory_order_release);

        }
        else if (Code == 10)
        {
            m_sdkFunc->triggerMode.store(0, std::memory_order::memory_order_release);
        }
        qDebug() << "捕获到信号" << Code;
    });
    setParameter(ParasValueMap);
    bool flag = m_sdkFunc->initSdk(ParasValueMap);
    if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
        type1 = 1;
    else if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE < 4)
        type1 = 0;
    type2 = 0;//不需要需要触发器，出图完成后给plc信号即可

    qDebug() << "SDKFUNC" << m_sdkFunc;
    return flag;
}

bool Hd_CameraModule_HIK3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
    Q_UNUSED(mats);
    if (mats.empty() && data.isEmpty())
    {
        MV_CC_SetCommandValue(m_sdkFunc->handle, "TriggerSoftware");
        //emit trigged(501);
        return true;
    }
    return true;
}
//获取数据
bool Hd_CameraModule_HIK3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{

	QList<cv::Mat> takenMats;
    m_sdkFunc->MatQueue.wait_for_pop(m_sdkFunc->timeOut, takenMats);

	
    if (takenMats.empty())
    {
        ImgS.push_back(cv::Mat::zeros(100, 100, 0));
        qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
        return false;
    }
    else
    {
        ImgS.push_back(takenMats.last());
    }
    return true;
}

void Hd_CameraModule_HIK3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent, const QString& getString)
{
    CallbackFuncPack TempPack;
    TempPack.callbackparent = parent;
    TempPack.cameraIndex = getString;
    TempPack.GetimagescallbackFunc = func;
    m_sdkFunc->CallbackFuncMap.insert(getString.toInt(), TempPack);
    qDebug() << m_sdkFunc << "registerCallBackFun" << getString;
}

void Hd_CameraModule_HIK3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
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
        qDebug() << "cancelCallBackFun" << getString;
    }
    return;
}

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
    if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
        return false;
    if (TotalMap.keys().contains(name.split(':').first())) return true;
    OnePb temp;
    temp.base = new Hd_CameraModule_HIK3(DeviceSn, path + "/Hd_CameraModule_HIK3/");
    if (!temp.base->init())
        return false;
    temp.baseWidget = new mPrivateWidget(temp.base);
    temp.DeviceSn = DeviceSn;
    TotalMap.insert(name.split(':').first(), temp);
    return  true;
}

void destroy(const QString& name)
{
    auto temp = TotalMap.take(name);
    if (temp.base)
    {
        delete temp.base;
    }
    if (temp.baseWidget)
    {
        delete temp.baseWidget;
    }
}

QWidget* getCameraWidgetPtr(const QString& name)
{
    if (TotalMap.value(name).baseWidget)
        return TotalMap.value(name).baseWidget;
    return nullptr;
}

PbGlobalObject* getCameraPtr(const QString& name)
{
    if (TotalMap.value(name).base)
        return TotalMap.value(name).base;
    return nullptr;
}

QStringList getCameraSnList()
{
    QStringList temp;
    if (!SearchDevice())
        return temp;

    for (int i = 0; i < m_stDevList.nDeviceNum; i++)
    {
        //SN
        unsigned char* name = m_stDevList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chSerialNumber;
        //userID
        //unsigned char* name=m_stDevList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chUserDefinedName;
        string userName = static_cast<string>((LPCSTR)name);
        temp << QString::fromStdString(userName);
    }
    //查询已经使用的
    foreach(const auto& tmp, TotalMap)
    {
        if (temp.contains(tmp.DeviceSn))
        {
            temp.removeOne(tmp.DeviceSn);
        }
    }
    return temp;
}

bool IsColor(MvGvspPixelType enType)
{
    switch (enType)
    {
    case PixelType_Gvsp_RGB8_Packed:
    case PixelType_Gvsp_YUV422_Packed:
    case PixelType_Gvsp_YUV422_YUYV_Packed:
    case PixelType_Gvsp_BayerGR8:
    case PixelType_Gvsp_BayerRG8:
    case PixelType_Gvsp_BayerGB8:
    case PixelType_Gvsp_BayerBG8:
    case PixelType_Gvsp_BayerGB10:
    case PixelType_Gvsp_BayerGB10_Packed:
    case PixelType_Gvsp_BayerBG10:
    case PixelType_Gvsp_BayerBG10_Packed:
    case PixelType_Gvsp_BayerRG10:
    case PixelType_Gvsp_BayerRG10_Packed:
    case PixelType_Gvsp_BayerGR10:
    case PixelType_Gvsp_BayerGR10_Packed:
    case PixelType_Gvsp_BayerGB12:
    case PixelType_Gvsp_BayerGB12_Packed:
    case PixelType_Gvsp_BayerBG12:
    case PixelType_Gvsp_BayerBG12_Packed:
    case PixelType_Gvsp_BayerRG12:
    case PixelType_Gvsp_BayerRG12_Packed:
    case PixelType_Gvsp_BayerGR12:
    case PixelType_Gvsp_BayerGR12_Packed:
        return true;
    default:
        return false;
    }
}

mPrivateWidget::mPrivateWidget(void* handle)
{
    m_Camerahandle = reinterpret_cast<Hd_CameraModule_HIK3*>(handle);
    InitWidget();
    connect(this, &mPrivateWidget::sendImage,this, [=](QImage img) {
            m_showimage->reciveImage("", img); }
        , Qt::QueuedConnection);
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
    auto createPushButton = [this](const QString& text,int width = 50, int height = 30) -> QPushButton* {
        QPushButton* pBtn = new QPushButton(this);
        pBtn->setText(text);
        pBtn->setFixedHeight(height);
        pBtn->setFixedWidth(width);
        return pBtn;
    };
    // 返回：QDoubleValidator*（自动设置为标准小数表示法）
    auto createDoubleValidator = [this](float minVal = 0.0, float maxVal = 10000000.0, int decimals = 4) -> QDoubleValidator* {
        QDoubleValidator* validator = new QDoubleValidator(minVal, maxVal, decimals, this);
        validator->setNotation(QDoubleValidator::StandardNotation);
        validator->setLocale(QLocale::C);
        validator->setBottom(minVal);
        validator->setTop(maxVal);
        return validator;
    };
    // 创建QScrollArea
    auto createScrollArea= [this]()->QScrollArea*{
        QScrollArea *area= new QScrollArea(this);
        area->setObjectName("content"); // 保留原ObjectName
        area->setContentsMargins(0, 0, 0, 0); // 保留原边距
        area->setWidgetResizable(true);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // 隐藏水平滚动条
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // 垂直滚动条按需显示
        area->setFrameShape(QFrame::NoFrame); // 去掉滚动区域边框，保持原有样式
        return area;
    };

    QFile file(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
    file.open(QIODevice::ReadOnly);
    QByteArray byte = file.readAll();
    file.close();
    BytePtr  = QJsonDocument::fromJson(byte).object();

    this->setContentsMargins(0,0,0,0);
    QHBoxLayout* mainHboxLayout = new QHBoxLayout(this);
    QVBoxLayout* MainLayout = new QVBoxLayout;
    QGridLayout* girlayout = new QGridLayout();
    QSplitter *Splitter = new QSplitter(Qt::Horizontal, this);
    QVBoxLayout* AlgParmLayout = new QVBoxLayout();
    //相机参数控件创建
    {
        QLabel *cameraTitle = createLabel(tr("相机参数设置"));
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
        BalanceRatioRL->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
        gain = createLineEdit();
        Gama = createLineEdit();
        Exposure = createLineEdit();

        first = createComboBox(QStringList() << tr("打开") << tr("关闭"));
        Second = createComboBox(QStringList() << "Line0" << "Line1" << "Line2" << "Line3" << "软触发");
        if(m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
			Second->setCurrentIndex(4);
		else if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_LINE0)
			Second->setCurrentIndex(0);
		else if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_LINE1)
			Second->setCurrentIndex(1);
		else if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_LINE2)
			Second->setCurrentIndex(2);
		else if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_LINE3)
			Second->setCurrentIndex(3);
        GamaDisable = createComboBox(QStringList() << tr("打开") << tr("关闭"));
        BalanceWhiteAuto = createComboBox(QStringList() << tr("关闭") << tr("一次")<<tr("连续"));
        BalanceRatioR = createSpinBox();
        BalanceRatioG = createSpinBox();
        BalanceRatioB = createSpinBox();

        triggerModel->hide();
        first->hide();
        //相机参数控件初值设置
        MVCC_FLOATVALUE stFloatValue = { 0 };
        MV_CC_GetFloatValue(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", &stFloatValue);
        ExposureMin = stFloatValue.fMin;
        ExposureMax = stFloatValue.fMax;
        float ExposureCur = stFloatValue.fCurValue;
        MV_CC_GetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gain", &stFloatValue);
        gainMin = stFloatValue.fMin;
        gainMax = stFloatValue.fMax;
        float gainCur = stFloatValue.fCurValue;
        MV_CC_GetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gamma", &stFloatValue);
        GamaMin = stFloatValue.fMin;
        GamaMax = stFloatValue.fMax;
        float GamaCur = stFloatValue.fCurValue;

        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_OFF);
        MVCC_INTVALUE stIntValue = { 0 };
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 0);
        MV_CC_GetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue);
        BalanceRatioR->setMaximum(stIntValue.nMax);
        BalanceRatioR->setMinimum(stIntValue.nMin);
        BalanceRatioR->setValue(stIntValue.nCurValue);
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 1);
        MV_CC_GetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue);
        BalanceRatioG->setMaximum(stIntValue.nMax);
        BalanceRatioG->setMinimum(stIntValue.nMin);
        BalanceRatioG->setValue(stIntValue.nCurValue);

        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 2);
        MV_CC_GetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue);
        BalanceRatioB->setMaximum(stIntValue.nMax);
        BalanceRatioB->setMinimum(stIntValue.nMin);
        BalanceRatioB->setValue(stIntValue.nCurValue);

        doubleValidator1 = createDoubleValidator(ExposureMin,ExposureMax);
        doubleValidator2 = createDoubleValidator(gainMin,gainMax);
        doubleValidator3 = createDoubleValidator(GamaMin,GamaMax);

        Exposure->setText(QString::number(ExposureCur, 'f', 4));
        Exposure->setValidator(doubleValidator1);
        gain->setText(QString::number(gainCur, 'f', 4));
        gain->setValidator(doubleValidator2);
        Gama->setText(QString::number(GamaCur, 'f', 4));
        Gama->setValidator(doubleValidator3);

        QHBoxLayout *hbox = new QHBoxLayout();
        hbox->addWidget(BalanceRatioRL);
        hbox->addWidget(BalanceRatioR);
        hbox->addWidget(BalanceRatioGL);
        hbox->addWidget(BalanceRatioG);
        hbox->addWidget(BalanceRatioBL);
        hbox->addWidget(BalanceRatioB);
        hbox->setContentsMargins(0, 0, 0, 0);
        //相机参数布局
        girlayout->addWidget(triggerModel, 0, 0);
        girlayout->addWidget(first, 0, 1);
        girlayout->addWidget(triggerSoure, 1, 0);
        girlayout->addWidget(Second, 1, 1);
        girlayout->addWidget(GainL, 2, 0);
        girlayout->addWidget(gain, 2, 1);
        girlayout->addWidget(GammaDisableL,3,0);
        girlayout->addWidget(GamaDisable,3,1);
        girlayout->addWidget(GamaL, 4, 0);
        girlayout->addWidget(Gama, 4, 1);
        girlayout->addWidget(ExposureL, 5, 0);
        girlayout->addWidget(Exposure, 5, 1);
        girlayout->addWidget(BalanceWhiteAutoL,6,0);
        girlayout->addWidget(BalanceWhiteAuto,6,1);
        girlayout->addWidget(BalanceRatioL,7,0);
        girlayout->addLayout(hbox,7,1);
        // girlayout->addWidget(BalanceRatioR,7,1);
        // girlayout->addWidget(BalanceRatioGL,8,0);
        // girlayout->addWidget(BalanceRatioG,8,1);
        // girlayout->addWidget(BalanceRatioBL,9,0);
        // girlayout->addWidget(BalanceRatioB,9,1);
        girlayout->setColumnStretch(0, 1);
        girlayout->setColumnStretch(1, 2);
        girlayout->setContentsMargins(16, 0, 8, 0);
        girlayout->setSpacing(3);
        girlayout->setAlignment(Qt::AlignTop);

        QScrollArea* widgetCameraParamScroll = createScrollArea();
        QWidget *widgetCameraParam = new QWidget(this);
        QVBoxLayout *paramLayout = new QVBoxLayout();
        widgetCameraParam->setObjectName("content");
        QHBoxLayout* cameraTitleLayout = new QHBoxLayout();

        cameraTitleLayout->addWidget(cameraTitle);
        cameraTitleLayout->setContentsMargins(0,0,0,0);
        paramLayout->addLayout(cameraTitleLayout,0);
        paramLayout->addLayout(girlayout,1);
        paramLayout->setContentsMargins(0,0,0,0);
        widgetCameraParam->setLayout(paramLayout);
        widgetCameraParam->setContentsMargins(0,0,0,0);
        widgetCameraParamScroll->setWidget(widgetCameraParam);

        AlgParmLayout->addWidget(widgetCameraParamScroll);
        AlgParmLayout->setContentsMargins(0,0,0,0);
    }

    //流程控件创建
    {
        QLabel* CountL = createLabel(tr("一次信号取图次数"));
        QLabel* TimeOutL = createLabel(tr("单张图超时时间"));
        QLabel *liucTitle = createLabel(tr("流程参数设置"));
        liucTitle->setObjectName("titleLabel1");
        Count = createSpinBox();
        timeout = createSpinBox();
        QStackedWidget* changeWidget = new QStackedWidget(this);

        saveBtn = new QPushButton(tr("保存"),this);
        saveBtn->setMinimumHeight(30);
        saveBtn->setObjectName("borderbutton");

        gainTable = new MyTableWidget(this, BytePtr.value("Gain").toArray());
        GamaTable = new MyTableWidget(this, BytePtr.value("Gamma").toArray());
        ExposureTimeTable = new MyTableWidget(this, BytePtr.value("ExposureTime").toArray());
        gainTable->setProperty("Name", "Gain");
        GamaTable->setProperty("Name", "Gamma");
        ExposureTimeTable->setProperty("Name", "ExposureTime");
        showTable = new QTableWidget(this);
        Add = createPushButton(tr("添加"));
        Delete = createPushButton(tr("删除"));
        takeEffect= createPushButton(tr("设定生效"),110);

        QLabel *tips = createLabel(tr("注意:设定生效按钮点击时,当前界面设定生效,请在未生产时操作"));
        tips->setStyleSheet("color: rgb(255, 0, 0);");

        QStringList headers;
        headers << tr("id")<< tr("Exposure")<<tr("Gain") << tr("Gamma")  ;
        showTable->setColumnCount(headers.size());
        showTable->setHorizontalHeaderLabels(headers);
        showTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        showTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        showTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        showTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
        showTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        showTable->verticalHeader()->setVisible(false);

        //流程参数设置
        Count->setValue(m_Camerahandle->m_sdkFunc->getImageMaxCoiunts);
        timeout->setValue(m_Camerahandle->m_sdkFunc->timeOut);
        //流程参数布局
        QGridLayout* girlayout_param = new QGridLayout();
        girlayout_param->addWidget(TimeOutL,0,0);
        girlayout_param->addWidget(timeout,0,1);
        girlayout_param->addWidget(CountL,1,0);
        girlayout_param->addWidget(Count,1,1);
        girlayout_param->setColumnStretch(0, 1);  // 第一列拉伸因子为1
        girlayout_param->setColumnStretch(1, 2);  // 第二列拉伸因子为2
        girlayout_param->setContentsMargins(16, 8, 0, 0);
        girlayout_param->setSpacing(3);

        QHBoxLayout * btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        btnLayout->addWidget(takeEffect);
        btnLayout->addWidget(Add);
        btnLayout->addWidget(Delete);
        btnLayout->setContentsMargins(16, 20, 0, 0);

        QScrollArea* widgetLiucScroll = createScrollArea();
        QWidget *widgetLiuc = new QWidget(this);
        widgetLiuc->setObjectName("content");
        widgetLiuc->setContentsMargins(0,0,0,0);
        QVBoxLayout* vLayout = new QVBoxLayout();
        QHBoxLayout* liucTitleLayout = new QHBoxLayout();
        liucTitleLayout->addWidget(liucTitle);
        vLayout->addLayout(liucTitleLayout,0);
        vLayout->addLayout(girlayout_param,1);
        vLayout->addLayout(btnLayout);
        vLayout->addWidget(showTable);
        vLayout->addWidget(changeWidget);
        vLayout->addWidget(tips);
        vLayout->addWidget(saveBtn);
        vLayout->setContentsMargins(0,8,8,8);
        vLayout->setSpacing(3);
        widgetLiuc->setLayout(vLayout);
        widgetLiucScroll->setWidget(widgetLiuc);
        AlgParmLayout->addWidget(widgetLiucScroll);
        AlgParmLayout->setSpacing(6);

        changeWidget->addWidget(gainTable);
        changeWidget->addWidget(GamaTable);
        changeWidget->addWidget(ExposureTimeTable);
        changeWidget->setContentsMargins(8,0,0,0);
        changeWidget->hide();
    }

    //参数json显示界面（隐藏）
    {
        m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
        m_AlgParmWidget->hide();
    }

    //图片显示界面控件创建
    {
        SetDataBtn = createPushButton(tr("软触发"),100);
        ContinuesBtn = createPushButton(tr("连续取图"),100);
        Details = createPushButton(tr("详情"),100);
        Details->hide();
        m_showimage = new viewWidget();
        m_showimage->setMinimumSize(500, 500);
        QLabel *title = createLabel("海康相机");

        //图片显示界面布局
        QHBoxLayout* MainBtnLayout = new QHBoxLayout;
        MainBtnLayout->addWidget(title);
        MainBtnLayout->addStretch();
        MainBtnLayout->addWidget(ContinuesBtn);
        MainBtnLayout->addWidget(SetDataBtn);
        MainBtnLayout->addWidget(Details);
        MainLayout->addLayout(MainBtnLayout);
        MainLayout->addWidget(m_showimage);
    }

    auto func = std::bind(&mPrivateWidget::showImage, this, std::placeholders::_1);
    m_Camerahandle->m_sdkFunc->registerGetImageFun(func);

    QWidget* mainLayoutWidget = new QWidget(Splitter);
    mainLayoutWidget->setLayout(MainLayout); // 将MainLayout设置到容器Widget

    AlgParmLayout->setStretch(0, 2);
    AlgParmLayout->setStretch(1, 5);
    QWidget* algParmLayoutWidget = new QWidget(Splitter);
    algParmLayoutWidget->setLayout(AlgParmLayout); // 将AlgParmLayout设置到容器Widget
    algParmLayoutWidget->setContentsMargins(0,0,0,0);

    Splitter->addWidget(mainLayoutWidget);
    Splitter->addWidget(algParmLayoutWidget);
    Splitter->addWidget(m_AlgParmWidget);
    QList<int> ratios;
    ratios << 4<< 3 << 0; // 三个子项的比例
    int totalRatio = 7;
    // 延迟设置尺寸（必须等Splitter完成布局后，才能获取正确的总宽度）
    // 使用QTimer::singleShot确保布局完成后再计算
    QTimer::singleShot(10, this, [=]() {
        int totalWidth = Splitter->width();
        QList<int> splitterSizes;
        for (int ratio : ratios) {
            int size = (totalWidth * ratio) / totalRatio;
            splitterSizes << size;
        }
        Splitter->setSizes(splitterSizes);
    });

    Splitter->setContentsMargins(0,0,0,0);
    mainHboxLayout->addWidget(Splitter);
    mainHboxLayout->setContentsMargins(0,0,0,0);
    createConnect();
    gainTable->initData();
    GamaTable->initData();
    ExposureTimeTable->initData();
}

void mPrivateWidget::createConnect()
{
    connect(first, &QComboBox::currentTextChanged, this, [=](QString text) {
        int mode = (text == tr("打开")) ? MV_TRIGGER_MODE_ON : MV_TRIGGER_MODE_OFF;
        int flag = (text == tr("打开")) ? 11 : 10;
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerMode", mode);
        m_Camerahandle->trigged(flag);
    });
    connect(Second, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        int source = MV_TRIGGER_SOURCE_LINE0;
        if (text == "Line1") source = MV_TRIGGER_SOURCE_LINE1;
        else if (text == "Line2") source = MV_TRIGGER_SOURCE_LINE2;
        else if (text == "Line3") source = MV_TRIGGER_SOURCE_LINE3;
        else if (text == tr("软触发")) source = MV_TRIGGER_SOURCE_SOFTWARE;
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", source);
        m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE = (MV_CAM_TRIGGER_SOURCE)source;
        if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
            m_Camerahandle->type1 = 1;
        else if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE < 4)
            m_Camerahandle->type1 = 0;
        qDebug() << "MV_CAM_TRIGGER_SOURCE" << m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE << "\t" << m_Camerahandle->type1;
    });

    connect(ContinuesBtn,&QPushButton::clicked,this,[=](){
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

    connect(gain, &QLineEdit::editingFinished, this, [=]() {
        MV_CC_SetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gain", float(gain->text().toFloat()));
    });
    connect(Gama, &QLineEdit::editingFinished, this, [=]() {
        MV_CC_SetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gamma", float(Gama->text().toFloat()));
    });
    connect(Exposure, &QLineEdit::editingFinished, this, [=]() {
        MV_CC_SetFloatValue(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", float(Exposure->text().toFloat()));
    });

    connect(BalanceWhiteAuto,&QComboBox::currentTextChanged,this,[=](const QString& text){
        int nRet = 0;
        if (text == tr("关闭"))
        {
            BalanceRatioR->setEnabled(true);
            BalanceRatioG->setEnabled(true);
            BalanceRatioB->setEnabled(true);
            BalanceRatioR->blockSignals(true);
            BalanceRatioG->blockSignals(true);
            BalanceRatioB->blockSignals(true);
            nRet = MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_OFF);
            if (nRet == 0)
            {
                qDebug() << "设置BalanceWhiteAuto";
                MVCC_INTVALUE stIntValue = { 0 };
                MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 0);
                MV_CC_GetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue);
                qDebug()<<stIntValue.nCurValue<<stIntValue.nMax<<stIntValue.nMin;
                BalanceRatioR->setMaximum(stIntValue.nMax);
                BalanceRatioR->setMinimum(stIntValue.nMin);
                BalanceRatioR->setValue(stIntValue.nCurValue);

                MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 1);
                MV_CC_GetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue);
                BalanceRatioG->setMaximum(stIntValue.nMax);
                BalanceRatioG->setMinimum(stIntValue.nMin);
                BalanceRatioG->setValue(stIntValue.nCurValue);

                MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 2);
                MV_CC_GetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue);
                BalanceRatioB->setMaximum(stIntValue.nMax);
                BalanceRatioB->setMinimum(stIntValue.nMin);
                BalanceRatioB->setValue(stIntValue.nCurValue);
            }
            BalanceRatioR->blockSignals(false);
            BalanceRatioG->blockSignals(false);
            BalanceRatioB->blockSignals(false);
        }
        else if (text == tr("一次"))
        {
            BalanceRatioR->setEnabled(false);
            BalanceRatioG->setEnabled(false);
            BalanceRatioB->setEnabled(false);
            nRet = MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_ONCE);
        }
        else
        {
            BalanceRatioR->setEnabled(false);
            BalanceRatioG->setEnabled(false);
            BalanceRatioB->setEnabled(false);
            nRet = MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_CONTINUOUS);
        }
    });
    connect(BalanceRatioR,&QSpinBox::editingFinished,this,[=](){
        int value = BalanceRatioR->value();
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 0);
        MV_CC_SetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio",value);
    });
    connect(BalanceRatioG,&QSpinBox::editingFinished,this,[=](){
        int value = BalanceRatioG->value();
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 1);
        MV_CC_SetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio",value);
    });
    connect(BalanceRatioB,&QSpinBox::editingFinished,this,[=](){
        int value = BalanceRatioB->value();
        MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 2);
        MV_CC_SetIntValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio",value);
    });

    connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
        std::vector<cv::Mat> mats;
        QStringList list;
       // emit m_Camerahandle->trigged(1000);
        m_Camerahandle->setData(mats, list);
        m_Camerahandle->data(mats, list);
        cv::Mat tempMat = mats.at(0);
        QImage showImg = cvMatToQImage(tempMat);
        m_showimage->reciveImage("", showImg);
    });
    connect(Details,&QPushButton::clicked,this, [=](){
        //

        //
    });

    connect(m_AlgParmWidget, &AlgParmWidget::SengCurrentByte, this, [=](QByteArray byte) {
        QJsonObject paramObj = QJsonDocument::fromJson(byte).object();
        QMap<QString, QString> ParameterMap;
        for (auto objStr : paramObj.keys())
        {
            if(paramObj.value(objStr).isString())
                ParameterMap.insert(objStr, paramObj.value(objStr).toString());
            else if(paramObj.value(objStr).isArray())
            {
                m_Camerahandle->m_sdkFunc->setArrayByte(objStr, paramObj.value(objStr).toArray());
            }
            else if (paramObj.value(objStr).isObject())
            {

            }
        }
        m_Camerahandle->setParameter(ParameterMap);
    });
	emit m_AlgParmWidget->SengCurrentByte(QJsonDocument(BytePtr).toJson());
    connect(GamaDisable, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        int nRet = 0;
        if (text == tr("打开")) {
            nRet = MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "GammaSelector", MV_GAMMA_SELECTOR_USER);
            if (nRet != 0) {
                qDebug() << "设置伽马选择器失败，错误码：" << nRet;
                return;
            }
            nRet = MV_CC_SetBoolValue(m_Camerahandle->m_sdkFunc->handle, "GammaEnable", true);
        } else {
            nRet = MV_CC_SetBoolValue(m_Camerahandle->m_sdkFunc->handle, "GammaEnable", false);
        }
        qDebug() << (nRet == 0 ? (text == tr("打开") ? tr("伽马功能已打开") : tr("伽马功能已关闭"))
                               : tr("操作失败，错误码：") + QString::number(nRet));
    });

    connect(Count,&QSpinBox::editingFinished,this,[=](){
        int currentValue = Count->value();
        m_Camerahandle->m_sdkFunc->getImageMaxCoiunts = currentValue;
        m_Camerahandle->m_sdkFunc->ParasValueMap["OnceSignalsGetImageCounts"] = QString::number(currentValue);
        //设置回去
        BytePtr["OnceSignalsGetImageCounts"] = QString::number(currentValue);
        m_AlgParmWidget->reLoadByte(QJsonDocument(BytePtr).toJson());
    });

    connect(timeout,&QSpinBox::editingFinished,this,[=](){
        int currentValue = timeout->value();
        m_Camerahandle->m_sdkFunc->timeOut = currentValue;
        m_Camerahandle->m_sdkFunc->ParasValueMap["GetOnceImageTimes"] = QString::number(currentValue);
        //设置回去
        BytePtr["GetOnceImageTimes"] = QString::number(currentValue);
        m_AlgParmWidget->reLoadByte(QJsonDocument(BytePtr).toJson());
    });
    connect(gainTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
    connect(GamaTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
    connect(ExposureTimeTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
    connect(saveBtn,&QPushButton::clicked,this,[=](){
        if (m_AlgParmWidget)
            emit m_AlgParmWidget->save();
    });

    connect(takeEffect,&QPushButton::clicked,this,[=](){
        auto clickMyTableTakeEffectButton = [](MyTableWidget* tableWidget) {
            if (!tableWidget) return;
            QToolButton* takeEffectBtn = tableWidget->getTakeEffectButton();
            if (takeEffectBtn) {
                // 模拟鼠标左键点击按钮
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
    connect(Add,&QPushButton::clicked,this,[=](){
        auto createCenteredItem = [](const QString& text) -> QTableWidgetItem* {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // 水平+垂直居中
            return item;
        };
        int row = showTable->rowCount();
        showTable->setRowCount(row+1);
        //后续判重
        showTable->setItem(row, 0, createCenteredItem("")); // id
        showTable->setItem(row, 1, createCenteredItem("")); // 曝光时间
        showTable->setItem(row, 2, createCenteredItem("")); // Gain
        showTable->setItem(row, 3, createCenteredItem("")); // 伽马校正
        auto clickMyTableAddButton = [](MyTableWidget* tableWidget) {
            if (!tableWidget) return;
            QToolButton* addBtn = tableWidget->getAddRowButton();
            if (addBtn) {
                // 模拟鼠标左键点击按钮
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

    connect(Delete,&QPushButton::clicked,this,[=](){
        QItemSelectionModel* selectionModel = showTable->selectionModel();
        QSet<int> selectedRows;
        QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
        foreach (QModelIndex index, selectedIndexes) {
            if (index.isValid()) { // 校验索引有效性
                selectedRows.insert(index.row());
            }
        }
        if (selectedRows.isEmpty()) return;
        QList<int> rows = selectedRows.toList();
        qSort(rows.begin(), rows.end(), qGreater<int>()); // 降序排序
        for (int row : rows) {
            ExposureTimeTable->removeTableRow(row);
            gainTable->removeTableRow(row);
            GamaTable->removeTableRow(row);

            showTable->removeRow(row);

        }
    });
    // 先绑定单元格开始编辑信号，记录原始值
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
            if (text.isEmpty())return true;
            int pos = 0;
            return validator->validate(const_cast<QString&>(text), pos) == QValidator::Acceptable;
        };

        QTableWidgetItem* item = showTable->item(row, col);
        if (!item) return;
        QString text = item->text().trimmed();
        if (col!=0)
        {
            bool isValid = true;
            switch (col) {
            case 1: // 曝光时间
                isValid = validateTableCell(text, doubleValidator1);
                break;
            case 2: // 增益
                isValid = validateTableCell(text, doubleValidator2);
                break;
            case 3: // 伽马校正
                isValid = validateTableCell(text, doubleValidator3);
                break;
            default: //无需校验
                return;
            }
            if (!isValid) {
                QString originalValue = cellOriginalValues.value(QPair<int, int>(row, col), "");
                DMessageBox::warning(this, tr("输入非法"),
                                     tr("合法范围：%1 ~ %2")
                                         .arg(col==1 ? ExposureMin : (col==2 ? gainMin : GamaMin))
                                         .arg(col==1 ? ExposureMax : (col==2 ? gainMax : GamaMax)));
                item->setText(originalValue);
                item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                return;
            }
            cellOriginalValues[QPair<int, int>(row, col)] = text;
            if (col == 1)//曝光时间表格修改
            {
                ExposureTimeTable->setItemData(row,1,text);
            }
            else if (col == 2)//增益修改
            {
                gainTable->setItemData(row,1,text);
            }
            else if (col == 3) //伽马校正
            {
                GamaTable->setItemData(row,1,text);
            }
        }
        else//图片列表只需要判断是不是int，并且同步更改三个列表
        {
            bool isInt = false;
            text.toInt(&isInt);
            if (!isInt && !text.isEmpty())
            {
                QString originalValue = cellOriginalValues.value(QPair<int, int>(row, col), "");
                DMessageBox::warning(this, tr("输入非法"),tr("请输入整数"));
                item->setText(originalValue);
                return;
            }
            //修改三个表格的第一行
            ExposureTimeTable->setItemData(row,0,text);
            gainTable->setItemData(row,0,text);
            GamaTable->setItemData(row,0,text);
        }
    });

    connect(ExposureTimeTable,&MyTableWidget::addNewLine,this,[=](int row ,int col,QString value){
        showTable->blockSignals(true);
        qDebug()<<"ExposureTimeTable:"<<row<<col<<value;
        int rowCount = showTable->rowCount();
        if (rowCount < row+1) showTable->setRowCount(row+1);
        QTableWidgetItem *item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        showTable->setItem(row,col,item);
        showTable->blockSignals(false);
    });
    connect(gainTable,&MyTableWidget::addNewLine,this,[=](int row ,int col,QString value){
        showTable->blockSignals(true);
        qDebug()<<"gainTable:"<<row<<col<<value;
        int rowCount = showTable->rowCount();
        if (rowCount < row+1) showTable->setRowCount(row+1);
        QTableWidgetItem *item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        if (col!=0)
            showTable->setItem(row,col+1,item);
        showTable->blockSignals(false);
    });
    connect(GamaTable,&MyTableWidget::addNewLine,this,[=](int row ,int col,QString value){
        showTable->blockSignals(true);
        qDebug()<<"GamaTable:"<<row<<col<<value;
        int rowCount = showTable->rowCount();
        if (rowCount < row+1) showTable->setRowCount(row+1);
        QTableWidgetItem *item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        if (col!=0)
            showTable->setItem(row,col+2,item);
        showTable->blockSignals(false);
    });
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

void mPrivateWidget::showImage(cv::Mat& image)
{
    emit sendImage(cvMatToQImage(image));
    //m_showimage->reciveImage("", cvMatToQImage(image.clone()));
}
