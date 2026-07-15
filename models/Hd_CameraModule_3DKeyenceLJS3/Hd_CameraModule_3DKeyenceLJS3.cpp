#include "Hd_CameraModule_3DKeyenceLJS3.h"

#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>
#include <exception>

namespace
{
struct OnePbLJS
{
    PbGlobalObject* base = nullptr;
    QWidget* baseWidget = nullptr;
    QString DeviceSn;
    QString ip;
    QString index;
};

QMap<QString, OnePbLJS> g_totalMapLJS;
QVector<QPair<QString, QString>> g_totalSnIpVecLJS;
LjsCameraFunSDKfactoryCls* g_sdkFuncLJS[MAX_LJS8A_DEVICENUM] = { nullptr };

QStringList getConnectedLjsDevicesFromARP()
{
    QStringList devices;
#ifdef Q_OS_WIN
    QProcess process;
    process.start("arp", QStringList() << "-a");
    process.waitForFinished();
    QString output = byteArrayToUnicode(process.readAllStandardOutput());
    QRegularExpression regex(QString(R"((\d+\.\d+\.\d+\.\d+)\s+([0-9a-fA-F-]+))").toLocal8Bit());
    QRegularExpressionMatchIterator matches = regex.globalMatch(output);
    while (matches.hasNext())
    {
        QRegularExpressionMatch match = matches.next();
        QString ip = match.captured(1);
        QStringList ipParts = ip.split('.');
        if (ipParts.size() == 4 && !ip.endsWith(".255") && !ip.endsWith(".0")
            && ipParts.at(0) == "192" && ipParts.at(1) == "168")
        {
            devices << ip;
        }
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    QProcess process;
    process.start("arp", QStringList() << "-n");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QRegularExpression regex(R"((\d+\.\d+\.\d+\.\d+)\s+\S+\s+\S+\s+([0-9a-fA-F:]+))");
    QRegularExpressionMatchIterator matches = regex.globalMatch(output);
    while (matches.hasNext())
    {
        QRegularExpressionMatch match = matches.next();
        devices << match.captured(1);
    }
#endif
    qDebug() << "LJS devices from ARP" << devices;
    return devices;
}

bool buildEthernetConfig(const QString& ip, int port, LJS8IF_ETHERNET_CONFIG* config)
{
    if (config == nullptr)
    {
        return false;
    }
    QStringList ipList = ip.split('.');
    if (ipList.size() != 4)
    {
        return false;
    }
    config->abyIpAddress[0] = static_cast<BYTE>(ipList.at(0).toInt());
    config->abyIpAddress[1] = static_cast<BYTE>(ipList.at(1).toInt());
    config->abyIpAddress[2] = static_cast<BYTE>(ipList.at(2).toInt());
    config->abyIpAddress[3] = static_cast<BYTE>(ipList.at(3).toInt());
    config->wPortNo = static_cast<WORD>(port);
    return true;
}

void ljsCallbackFunc(LJS8IF_PROFILE_HEADER* pProfileHeaderArray, WORD* pHeightProfileArray, BYTE* pLuminanceProfileArray,
    DWORD dwLuminanceEnable, DWORD dwProfileDataCount, DWORD dwCount, DWORD dwNotify, DWORD dwUser)
{
    try
    {
        if (dwUser >= MAX_LJS8A_DEVICENUM || dwCount == 0 || dwProfileDataCount == 0 /*|| dwNotify != 0*/)
        {
            return;
        }
        if (pHeightProfileArray == nullptr)
        {
            qCritical() << __FUNCTION__ << "line:" << __LINE__ << "height profile is null";
            return;
        }
        LjsCameraFunSDKfactoryCls* sdk = g_sdkFuncLJS[dwUser];
        if (sdk == nullptr)
        {
            qCritical() << __FUNCTION__ << "line:" << __LINE__ << "sdk is null" << dwUser;
            return;
        }

        if (pProfileHeaderArray != nullptr)
        {
            sdk->profileHeader = *pProfileHeaderArray;
            sdk->getParam.isProcTimeoutOccurred = pProfileHeaderArray->byProcTimeout;
        }
        sdk->getParam.luminance_enabled = static_cast<int>(dwLuminanceEnable);
        sdk->getParam.x_pointnum = static_cast<int>(dwProfileDataCount);
        sdk->getParam.y_linenum_acquired = static_cast<int>(dwCount);

        const int rows = static_cast<int>(dwCount);
        const int cols = static_cast<int>(dwProfileDataCount);
        cv::Mat heightMat(rows, cols, CV_16UC1, pHeightProfileArray);

        if (sdk->useExternalTrigger > 0)
        {
            QList<cv::Mat> getImageVector;
            int realIndex = sdk->Currentindex * sdk->OnceGetImageNum;
            if (dwLuminanceEnable == 1 && pLuminanceProfileArray != nullptr)
            {
                cv::Mat luminanceMat(rows, cols, CV_8UC1, pLuminanceProfileArray);
                getImageVector.push_back(luminanceMat.clone());
                if (sdk->CallbackFuncMap.keys().contains(realIndex))
                {
                    QObject* obj = sdk->CallbackFuncMap.value(realIndex).callbackparent;
                    if (obj != nullptr && sdk->CallbackFuncMap.value(realIndex).GetimagescallbackFunc != nullptr)
                    {
                        obj->setProperty("cameraIndex", QString::number(realIndex));
                        sdk->CallbackFuncMap.value(realIndex).GetimagescallbackFunc(obj, getImageVector);
                    }
                }
                ++realIndex;
            }

            getImageVector.clear();
            getImageVector.push_back(heightMat.clone());
            if (sdk->CallbackFuncMap.keys().contains(realIndex))
            {
                QObject* obj = sdk->CallbackFuncMap.value(realIndex).callbackparent;
                if (obj != nullptr && sdk->CallbackFuncMap.value(realIndex).GetimagescallbackFunc != nullptr)
                {
                    obj->setProperty("cameraIndex", QString::number(realIndex));
                    sdk->CallbackFuncMap.value(realIndex).GetimagescallbackFunc(obj, getImageVector);
                }
            }
        }
        else if (sdk->allowflag.load(std::memory_order::memory_order_acquire))
        {
            std::vector<cv::Mat> getImageVector;
            if (dwLuminanceEnable == 1 && pLuminanceProfileArray != nullptr)
            {
                cv::Mat luminanceMat(rows, cols, CV_8UC1, pLuminanceProfileArray);
                getImageVector.push_back(luminanceMat.clone());
            }
            getImageVector.push_back(heightMat.clone());
            sdk->ImageMats.push(getImageVector);
        }

        ++sdk->Currentindex;
        const int callbackCycle = sdk->OnceGetImageNum > 0 ? sdk->getImageMaxCoiunts / sdk->OnceGetImageNum : 1;
        if (callbackCycle > 0 && sdk->Currentindex >= callbackCycle)
        {
            sdk->Currentindex = 0;
        }
        qDebug() << __FUNCTION__ << "line:" << __LINE__ << "success to acquire LJS image" << dwUser << rows << cols;
    }
    catch (const std::exception& e)
    {
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << e.what();
    }
}
}

void Hd_CameraModule_3DKeyenceLJS3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
    CallbackFuncPackLJS tempPack;
    tempPack.callbackparent = parent;
    tempPack.cameraIndex = getString;
    tempPack.GetimagescallbackFunc = callBackFun;
    m_sdkFunc->CallbackFuncMap.insert(getString.toInt(), tempPack);
    qDebug() << "register LJS callback" << getString;
}

void Hd_CameraModule_3DKeyenceLJS3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
    Q_UNUSED(parent);
    int index = getString.toInt();
    if (m_sdkFunc->CallbackFuncMap.keys().contains(index))
    {
        if (callBackFun == m_sdkFunc->CallbackFuncMap.value(index).GetimagescallbackFunc)
        {
            m_sdkFunc->CallbackFuncMap.remove(index);
        }
        else
        {
            qCritical() << "key of Values != Input Callbackfun" << getString;
        }
        qDebug() << "cancel LJS callback" << getString;
    }
}

LjsCameraFunSDKfactoryCls::LjsCameraFunSDKfactoryCls(int id, QString rootPath, QObject* praent)
    : deviceId(id), RootPath(rootPath), parent(praent)
{
}

LjsCameraFunSDKfactoryCls::~LjsCameraFunSDKfactoryCls()
{
    Sleep(10);
    if (isopen)
    {
        isopen = false;
        errCode = LJS8IF_StopHighSpeedDataCommunication(deviceId);
        qDebug() << __FUNCTION__ << "line:" << __LINE__ << "Stop HighSpeed errCode:" << errCode;
        LJS8A_ACQ_CloseDevice(deviceId);
    }
    if (deviceId >= 0 && deviceId < MAX_LJS8A_DEVICENUM)
    {
        g_sdkFuncLJS[deviceId] = nullptr;
    }
    LJS8IF_Finalize();
}

bool LjsCameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{
    ParasValueMap = insideValuesMaps;
    upDateParam();

    errCode = LJS8IF_Initialize();
    qDebug() << __FUNCTION__ << "line:" << __LINE__ << "Initialize errCode:" << errCode;

    errCode = LJS8A_ACQ_OpenDevice(deviceId, &EthernetConfig, HighSpeedPortNo);
    if (errCode != LJS8IF_RC_OK)
    {
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << "Failed to open LJS device" << errCode;
        return false;
    }
    isopen = true;

    if (!InitHighSpeed())
    {
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << "InitHighSpeed error";
        return false;
    }
    return true;
}

int LjsCameraFunSDKfactoryCls::LJS8A_ACQ_OpenDevice(int lDeviceId, LJS8IF_ETHERNET_CONFIG* ethernetConfig, int highSpeedPortNo)
{
    if (ethernetConfig == nullptr)
    {
        return LJS8IF_RC_ERR_PARAMETER;
    }
    int openErrCode = LJS8IF_EthernetOpen(lDeviceId, ethernetConfig);
    EthernetConfig = *ethernetConfig;
    HighSpeedPortNo = highSpeedPortNo;
    qDebug() << __FUNCTION__ << "line:" << __LINE__ << "Open LJS device errCode:" << openErrCode;
    return openErrCode;
}

void LjsCameraFunSDKfactoryCls::LJS8A_ACQ_CloseDevice(int lDeviceId)
{
    LJS8IF_FinalizeHighSpeedDataCommunication(lDeviceId);
    LJS8IF_CommunicationClose(lDeviceId);
    qDebug() << __FUNCTION__ << "line:" << __LINE__ << "Close LJS device";
}

bool LjsCameraFunSDKfactoryCls::InitHighSpeed()
{
    try
    {
        startReq.bySendPosition = 2;
        errCode = LJS8IF_InitializeHighSpeedDataCommunicationSimpleArray(deviceId, &EthernetConfig,
            static_cast<WORD>(HighSpeedPortNo), &ljsCallbackFunc, static_cast<DWORD>(deviceId));
        if (errCode != LJS8IF_RC_OK && errCode != LJS8IF_RC_ERR_HISPEED_OPEN_YET)
        {
            qCritical() << __FUNCTION__ << "line:" << __LINE__ << "Initialize HighSpeed error" << errCode;
            return false;
        }

        errCode = LJS8IF_PreStartHighSpeedDataCommunication(deviceId, &startReq,
            static_cast<BYTE>(usePcImageFilter), &heightImageInfo);
        if (errCode != LJS8IF_RC_OK)
        {
            qCritical() << __FUNCTION__ << "line:" << __LINE__ << "PreStart HighSpeed error" << errCode;
            return false;
        }

        xImageSize = static_cast<int>(heightImageInfo.wXPointNum);
        yImageSize = static_cast<int>(heightImageInfo.wYLineNum);
        getParam.luminance_enabled = static_cast<int>(heightImageInfo.byLuminanceOutput);
        getParam.x_pointnum = xImageSize;
        getParam.y_linenum_acquired = yImageSize;
        getParam.x_pitch_um = heightImageInfo.dwPitchX / 100.0f;
        getParam.y_pitch_um = heightImageInfo.dwPitchY / 100.0f;
        getParam.z_pitch_um = heightImageInfo.dwPitchZ / 100.0f;

        errCode = LJS8IF_StartHighSpeedDataCommunication(deviceId);
        if (errCode != LJS8IF_RC_OK)
        {
            qCritical() << __FUNCTION__ << "line:" << __LINE__ << "Start HighSpeed error" << errCode;
            return false;
        }
        qDebug() << __FUNCTION__ << "line:" << __LINE__ << "Start HighSpeed success" << xImageSize << yImageSize;
    }
    catch (const std::exception& e)
    {
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << e.what();
        return false;
    }
    return true;
}

void LjsCameraFunSDKfactoryCls::upDateParam()
{
    timeout_ms = ParasValueMap.value("GetOnceImageTimes", "5000").toInt();
    if (!buildEthernetConfig(ParasValueMap.value("Ip"), ParasValueMap.value("Port", "24691").toInt(), &EthernetConfig))
    {
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << "invalid ip" << ParasValueMap.value("Ip");
        return;
    }
    xImageSize = ParasValueMap.value("xImageSize", "3200").toInt();
    yImageSize = ParasValueMap.value("yImageSize", "6400").toInt();
    deviceId = ParasValueMap.value("DeviceId", "0").toInt();
    useExternalTrigger = ParasValueMap.value("useExternalTrigger", ParasValueMap.value("use_external_batchStart", "1")).toInt();
    usePcImageFilter = ParasValueMap.value("usePcImageFilter", "1").toInt();
    HighSpeedPortNo = ParasValueMap.value("HighSpeedPortNo", "24692").toInt();
    getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts", "6").toInt();
    OnceGetImageNum = ParasValueMap.value("OnceImageCounts", "2").toInt();

    setParam.timeout_ms = timeout_ms;
    setParam.useExternalTrigger = useExternalTrigger;
    setParam.usePcImageFilter = usePcImageFilter;
}

bool LjsCameraFunSDKfactoryCls::run()
{
    if (!isopen)
    {
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << "device is not opened";
        return false;
    }
    return true;
}

Hd_CameraModule_3DKeyenceLJS3::Hd_CameraModule_3DKeyenceLJS3(int DevicedID, QString ip, QString RootPath, int settype, QObject* parent)
    : PbGlobalObject(settype, parent), ip(ip), RootPath(RootPath), deviceId(DevicedID)
{
    famliy = PGOFAMLIY::CAMERA3D;
    SnName = QString("LJS_%1").arg(DevicedID);
    for (const auto& pair : g_totalSnIpVecLJS)
    {
        if (pair.second == ip)
        {
            SnName = pair.first;
            break;
        }
    }

    JsonFile = RootPath + SnName + ".json";
    QString firstCreateByte(R"({"DeviceId": ")" + QString::number(DevicedID) + R"(",
    "GetOnceImageTimes": "5000",
    "Ip": ")" + ip + R"(",
    "Port": "24691",
    "HighSpeedPortNo": "24692",
    "xImageSize": "3200",
    "yImageSize": "6400",
    "OnceImageCounts":"2",
    "useExternalTrigger":"1",
    "use_external_batchStart":"1",
    "usePcImageFilter":"1",
    "OnceSignalsGetImageCounts":"6"})");
    if (!QFile(JsonFile).exists())
    {
        QDir().mkpath(RootPath);
        createAndWritefile(JsonFile, firstCreateByte.toUtf8());
    }
    QJsonObject paramObj = load_JsonFile(JsonFile);
    for (auto objStr : paramObj.keys())
    {
        ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
    }

    m_sdkFunc = new LjsCameraFunSDKfactoryCls(DevicedID, RootPath, this);
    if (deviceId >= 0 && deviceId < MAX_LJS8A_DEVICENUM)
    {
        g_sdkFuncLJS[deviceId] = m_sdkFunc;
    }
    connect(m_sdkFunc, &LjsCameraFunSDKfactoryCls::trigged, this, [=](int value) { emit trigged(value); });
}

Hd_CameraModule_3DKeyenceLJS3::~Hd_CameraModule_3DKeyenceLJS3()
{
    if (m_sdkFunc)
    {
        delete m_sdkFunc;
        m_sdkFunc = nullptr;
    }
    qDebug() << __FUNCTION__ << "line:" << __LINE__ << "delete success";
}

QMap<QString, QString> Hd_CameraModule_3DKeyenceLJS3::parameters()
{
    return m_sdkFunc->ParasValueMap;
}

bool Hd_CameraModule_3DKeyenceLJS3::setParameter(const QMap<QString, QString>& ParameterMap)
{
    ParasValueMap = ParameterMap;
    m_sdkFunc->ParasValueMap = ParasValueMap;
    m_sdkFunc->upDateParam();
    return true;
}

bool Hd_CameraModule_3DKeyenceLJS3::init()
{
    connect(this, &PbGlobalObject::trigged, [=](int Code) {
        if (Code == 1000)
        {
            m_sdkFunc->Currentindex = 0;
            m_sdkFunc->ImageMats.clear();
            m_sdkFunc->allowflag.store(true, std::memory_order::memory_order_release);
            emit trigged(501);
        }
        else if (Code == 1001)
        {
            m_sdkFunc->allowflag.store(false, std::memory_order::memory_order_release);
        }
    });

    setParameter(ParasValueMap);
    bool flag = m_sdkFunc->initSdk(ParasValueMap);
    emit trigged(flag ? 0 : 1);

    if (m_sdkFunc->getTrigger())
    {
        type1 = 0;
        type2 = 0;
    }
    else
    {
        type1 = 1;
        type2 = 1;
    }
    qDebug() << deviceId << SnName << "LJS init" << flag;
    m_sdkFunc->allowflag.store(flag, std::memory_order::memory_order_release);
    return flag;
}

bool Hd_CameraModule_3DKeyenceLJS3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
    if (mats.empty() && data.isEmpty())
    {
        if (m_sdkFunc->getTrigger() > 0)
        {
            qDebug() << "LJS external trigger mode, skip software trigger";
            return true;
        }
        int triggerErrCode = LJS8IF_Trigger(deviceId);
        if (triggerErrCode == LJS8IF_RC_OK)
        {
            emit trigged(501);
            return true;
        }
        qWarning() << "LJS8IF_Trigger error" << triggerErrCode;
        return false;
    }
    return true;
}

bool Hd_CameraModule_3DKeyenceLJS3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
    Q_UNUSED(QStringListdata);
    m_sdkFunc->ImageMats.wait_for_pop(m_sdkFunc->timeout_ms, ImgS);
    if (ImgS.empty())
    {
        ImgS.push_back(cv::Mat::zeros(100, 100, CV_8UC1));
        qCritical() << __FUNCTION__ << "line:" << __LINE__ << "srcImage is null";
        return false;
    }
    return true;
}

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
    if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
    {
        return false;
    }
    const QString mapKey = name.split(':').first();
    if (g_totalMapLJS.keys().contains(mapKey))
    {
        return true;
    }
    int indexSn = -1;
    if (name.endsWith("old"))
    {
        getCameraSnList();
    }
    for (int i = 0; i < g_totalSnIpVecLJS.size(); ++i)
    {
        if (g_totalSnIpVecLJS.at(i).first == DeviceSn)
        {
            indexSn = i;
            break;
        }
    }
    if (indexSn == -1)
    {
        qCritical() << "NOT Found LJS Device ID" << DeviceSn;
        return false;
    }

    OnePbLJS temp;
    temp.base = new Hd_CameraModule_3DKeyenceLJS3(indexSn, g_totalSnIpVecLJS.at(indexSn).second, path + "/Hd_CameraModule_3DKeyenceLJS3/");
    if (!temp.base->init())
    {
        delete temp.base;
        temp.base = nullptr;
        return false;
    }
    temp.baseWidget = new mPrivateWidget(temp.base);
    temp.DeviceSn = DeviceSn;
    temp.ip = g_totalSnIpVecLJS.at(indexSn).second;
    temp.index = QString::number(indexSn);
    g_totalMapLJS.insert(mapKey, temp);
    return true;
}

void destroy(const QString& name)
{
    const QString mapKey = name.split(':').first();
    auto temp = g_totalMapLJS.take(mapKey);
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
    const QString mapKey = name.split(':').first();
    if (g_totalMapLJS.value(mapKey).baseWidget)
    {
        return g_totalMapLJS.value(mapKey).baseWidget;
    }
    return nullptr;
}

PbGlobalObject* getCameraPtr(const QString& name)
{
    const QString mapKey = name.split(':').first();
    if (g_totalMapLJS.value(mapKey).base)
    {
        return g_totalMapLJS.value(mapKey).base;
    }
    return nullptr;
}

QStringList getCameraSnList()
{
    QStringList temp;
    QVector<QString> ipVec;
    bool isfileReaded = false;
    const QString ipDir = "./3DKeyenceLJS/";
    const QString ipFilePath = ipDir + "Ip.txt";

    if (QFile(ipFilePath).exists())
    {
        QFile file(ipFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream in(&file);
            while (!in.atEnd())
            {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty())
                {
                    ipVec.push_back(line);
                }
            }
            file.close();
            isfileReaded = !ipVec.isEmpty();
        }
        else
        {
            qCritical() << "Failed to open LJS Ip.txt for reading.";
        }
    }

    if (!isfileReaded)
    {
        QDir().mkpath(ipDir);
        QStringList list = getConnectedLjsDevicesFromARP();
        for (const auto& ipStr : list)
        {
            if (ipVec.size() >= MAX_LJS8A_DEVICENUM)
            {
                break;
            }
            LJS8IF_ETHERNET_CONFIG ethernetConfig{};
            if (!buildEthernetConfig(ipStr, 24691, &ethernetConfig))
            {
                continue;
            }
            const int deviceIndex = ipVec.size();
            int initErrCode = LJS8IF_Initialize();
            Q_UNUSED(initErrCode);
            int errCode = LJS8IF_EthernetOpen(deviceIndex, &ethernetConfig);
            if (errCode == LJS8IF_RC_OK)
            {
                ipVec.push_back(ipStr);
                LJS8IF_CommunicationClose(deviceIndex);
            }
            else
            {
                qDebug() << __FUNCTION__ << "line:" << __LINE__ << "Open LJS device errCode:" << errCode << ipStr;
            }
            LJS8IF_Finalize();
        }

        if (!ipVec.empty())
        {
            QFile file(ipFilePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            {
                QTextStream out(&file);
                for (const auto& ipStr : ipVec)
                {
                    out << ipStr << "\n";
                }
                file.close();
            }
            else
            {
                qCritical() << "Failed to open LJS Ip.txt for writing.";
            }
        }
    }

    g_totalSnIpVecLJS.clear();
    for (int i = 0; i < ipVec.size() && i < MAX_LJS8A_DEVICENUM; ++i)
    {
        LJS8IF_ETHERNET_CONFIG ethernetConfig{};
        if (!buildEthernetConfig(ipVec.at(i), 24691, &ethernetConfig))
        {
            continue;
        }
        int initErrCode = LJS8IF_Initialize();
        Q_UNUSED(initErrCode);
        int errCode = LJS8IF_EthernetOpen(i, &ethernetConfig);
        if (errCode == LJS8IF_RC_OK)
        {
            char serialNo[64] = { 0 };
            int snErrCode = LJS8IF_GetSerialNumber(i, serialNo);
            if (snErrCode == LJS8IF_RC_OK)
            {
                QString snName = QString::fromLocal8Bit(serialNo);
                QPair<QString, QString> tempPair;
                tempPair.first = snName;
                tempPair.second = ipVec.at(i);
                g_totalSnIpVecLJS.push_back(tempPair);
                temp << snName;
                qDebug() << "Get LJS Device" << i << serialNo << ipVec.at(i);
            }
            else
            {
                qCritical() << "LJS8IF_GetSerialNumber error" << snErrCode << ipVec.at(i);
            }
            LJS8IF_CommunicationClose(i);
        }
        LJS8IF_Finalize();
    }

    foreach (const auto& tmp, g_totalMapLJS)
    {
        if (temp.contains(tmp.DeviceSn))
        {
            temp.removeOne(tmp.DeviceSn);
        }
    }
    return temp;
}

mPrivateWidget::mPrivateWidget(void* handle)
{
    m_Camerahandle = reinterpret_cast<Hd_CameraModule_3DKeyenceLJS3*>(handle);
    InitWidget();
}

void mPrivateWidget::InitWidget()
{
    QHBoxLayout* mainHboxLayout = new QHBoxLayout(this);
    QVBoxLayout* MainLayout = new QVBoxLayout;

    SetDataBtn = new QPushButton(this);
    SetDataBtn->setText(tr("软触发"));
    OpenGrapMat = new QPushButton(this);
    OpenGrapMat->setText(tr("允许取图"));
    NotGrapMat = new QPushButton(this);
    NotGrapMat->setText(tr("禁止取图"));
    m_showimage = new ImageViewer(this);
    m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");

    connect(m_AlgParmWidget, &AlgParmWidget::SengCurrentByte, this, [=](QByteArray byte) {
        QJsonObject paramObj = QJsonDocument::fromJson(byte).object();
        QMap<QString, QString> ParameterMap;
        for (auto objStr : paramObj.keys())
        {
            ParameterMap.insert(objStr, paramObj.value(objStr).toString());
        }
        m_Camerahandle->setParameter(ParameterMap);
    });

    MainLayout->addWidget(m_showimage);
    MainLayout->addWidget(SetDataBtn);
    MainLayout->addWidget(OpenGrapMat);
    MainLayout->addWidget(NotGrapMat);

    connect(OpenGrapMat, &QPushButton::clicked, this, [=]() { emit m_Camerahandle->trigged(1000); });
    connect(NotGrapMat, &QPushButton::clicked, this, [=]() { emit m_Camerahandle->trigged(1001); });
    connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
        std::vector<cv::Mat> mats;
        QStringList list;
        emit m_Camerahandle->trigged(1000);
        if (m_Camerahandle->setData(mats, list) && m_Camerahandle->data(mats, list) && !mats.empty())
        {
            cv::Mat tempMat = mats.at(0);
            m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
        }
    });

    mainHboxLayout->addLayout(MainLayout, 4);
    mainHboxLayout->addWidget(m_AlgParmWidget, 3);
}
