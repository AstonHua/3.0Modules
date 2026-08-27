#include "Hd_CameraModule_Dalsa3.h"
#include <stdio.h>

#pragma execution_character_set("utf-8")
#define STRING_LENGTH 256
#define CAMERA_LINK_SERVER_NAME_PREFIX "CameraLink_"
/* ---- 全局变量 ---- */
struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString         DeviceSn;
};

QMap<QString, QString> DeviceListsMap;
static QMap<QString, OnePb> TotalMap;
static QMutex                mapMutex;
static int                   choic = 2;    /* 搜索相机类型系数 */
static bool                  g_bSaperaOpened = false;  /* Sapera 是否已初始化 */

/* ---- 前向声明 ---- */
static void ServerCallback(SapManCallbackInfo* pInfo);
static void XferCallback(SapXferCallbackInfo* pInfo);
static QStringList SearchDevice();

static void ServerCallback(SapManCallbackInfo* pInfo)
{
	int serverIndex = pInfo->GetServerIndex();
	char serverName[64] = { 0 };
	SapManager::GetServerName(serverIndex, serverName, sizeof(serverName));

	switch (pInfo->GetEventType())
	{
	case SapManager::EventServerNew:
		qDebug() << "==> Camera" << serverName << "connected for the first time";
		break;

	case SapManager::EventServerDisconnected:
		qDebug() << "==> Camera" << serverName << "disconnected";
		break;

	case SapManager::EventServerConnected:
		qDebug() << "==> Camera" << serverName << "reconnected";
		break;

	default:
		break;
	}
}
bool Hd_CameraModule_Dalsa3::CloseDevice()
{
	Acq.UnregisterCallback();

	/* 先清理特征辅助对象 */
	CleanupFeatureHelper();

	/* 停止并销毁传输对象 */
	if (Xfer != nullptr)
	{
		Xfer->Freeze();
		Xfer->Destroy();
		Xfer = nullptr;
	}

	/* 销毁缓冲区 */
	if (!Buffers.Destroy())
	{
		qWarning() << __FUNCTION__ << "Buffers.Destroy() failed";
	}

	/* 销毁采集对象 */
	if (!Acq.Destroy())
	{
		qWarning() << __FUNCTION__ << "Acq.Destroy() failed";
	}

	return true;
}

QStringList SearchDevice()
{
	QStringList CameraSeverName;

	/* Sapera 只初始化一次 */
	if (!g_bSaperaOpened)
	{
		SapManager::Open();
		SapManager::RegisterServerCallback(
			SapManager::EventServerNew
			| SapManager::EventServerDisconnected
			| SapManager::EventServerConnected
			| SapManager::EventResourceInfoChanged,
			ServerCallback, NULL);
		SapManager::SetDisplayStatusMode(SapManager::StatusLog);
		g_bSaperaOpened = true;
	}

	int serverCount = SapManager::GetServerCount();
	if (serverCount <= 0)
	{
		qDebug() << "No server found!";
		return CameraSeverName;
	}

	char serverName[CORSERVER_MAX_STRLEN];
	char featureBuf[STRING_LENGTH];
	int  featureCount = 0;
	DeviceListsMap.clear();
	for (int serverIndex = 0; serverIndex < serverCount; serverIndex++)
	{
		if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcqDevice) == 0)
		{
			continue;
		}
			char serverName[CORSERVER_MAX_STRLEN];
			SapManager::GetServerName(serverIndex, serverName, sizeof(serverName));
			char deviceName[CORPRM_GETSIZE(CORACQ_PRM_LABEL)];
			SapManager::GetResourceName(serverName, SapManager::ResourceAcqDevice, 0, deviceName, sizeof(deviceName));
			QString serverNameStr = serverName;
			if (serverNameStr.startsWith(CAMERA_LINK_SERVER_NAME_PREFIX))
			{
				DeviceListsMap[deviceName] = serverNameStr;
			}
	}

	for (int serverIndex = 0; serverIndex < serverCount; serverIndex++)
	{
		/* 只处理有 AcqDevice 资源的服务器 */
		if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcq) == 0)
		{
			continue;
		}
		SapManager::GetServerName(serverIndex, serverName, sizeof(serverName));
		CameraSeverName << serverName;
		QStringList keys = DeviceListsMap.keys();
		auto func = [=]() {
			for (int i = 0; i < keys.size(); i++) {
				if (keys.at(i).startsWith(serverName))
					return DeviceListsMap.value(keys.at(i));
			}
			return  QString("empty");
			};
		QString linkName = func();
		if (linkName == "empty")
			continue;
		SapAcqDevice camera(linkName.toStdString().c_str());
		BOOL status = camera.Create();
		if (status)
		{
			status = camera.GetFeatureCount(&featureCount);
		}
		if (status && featureCount > 0)
		{
			switch (choic)
			{
			case 1:
			{
				/* 按用户自定义名称 */
				if (camera.GetFeatureValue("DeviceUserID", featureBuf, sizeof(featureBuf)))
				{
					qDebug() << "[DeviceUserID]" << featureBuf;
				}
				else
				{
					qDebug() << "[DeviceUserID] N/A";
				}
				break;
			}
			case 2:
			{
				/* 按序列号 */
				if (camera.GetFeatureValue("DeviceID", featureBuf, sizeof(featureBuf)))
				{
					qDebug() << "[DeviceID]" << featureBuf;
				}
				else
				{
					qDebug() << "[DeviceID] N/A";
				}
				break;
			}
			case 3:
			{
				/* 按服务器名称 */
				qDebug() << "[ServerName]" << serverName;
				break;
			}
			case 4:
			{
				/* 按型号名称 */
				if (camera.GetFeatureValue("DeviceModelName", featureBuf, sizeof(featureBuf)))
				{
					qDebug() << "[DeviceModelName]" << featureBuf;
				}
				else
				{
					qDebug() << "[DeviceModelName] N/A";
				}
				break;
			}
			default:
				break;
			}
		}

		camera.Destroy();
	}

	if (CameraSeverName.isEmpty())
	{
		qDebug() << "No camera found!";
	}

	return CameraSeverName;
}

bool Hd_CameraModule_Dalsa3::connctDevice(const std::string& getSnName,
	const std::string& cfgFilename)
{
	QStringList CameraSeverName = SearchDevice();
	if (CameraSeverName.isEmpty())
	{
		qWarning() << __FUNCTION__ << "No camera found";
		return false;
	}

	/* 在结果中匹配目标服务器名 */
	int index = -1;
	QString targetName = QString::fromStdString(getSnName);
	for (int i = 0; i < CameraSeverName.length(); i++)
	{
		if (CameraSeverName[i] == targetName)
		{
			index = i;
			DeviceIndex = index;
			break;
		}
	}

	if (index < 0)
	{
		qWarning() << __FUNCTION__ << "Camera" << targetName
			<< "not found in search results:" << CameraSeverName;
		return false;
	}

	QString name = CameraSeverName[index];
	qDebug() << "[INFO]" << __FUNCTION__ << "Connecting to:" << name;
	if (SapManager::GetResourceCount(getSnName.c_str(), SapManager::ResourceAcq) > 0)
	{
		SapLocation loc(getSnName.c_str(), 0);
		Acq = SapAcquisition(loc, cfgFilename.c_str());
		Buffers = SapBufferWithTrash(2, &Acq);
		AcqToBuf = SapAcqToBuf(&Acq, &Buffers, XferCallback, this);
		Xfer = &AcqToBuf;

		if (!Acq.Create())
		{
			qWarning() << __FUNCTION__ << "Acq.Create() failed";
			return false;
		}
	}
	/*else if (SapManager::GetResourceCount(getSnName.c_str(), SapManager::ResourceAcqDevice) > 0)
	{
		SapLocation loc(getSnName.c_str(), 0);

		AcqDevice = SapAcqDevice(loc, FALSE);
		Buffers = SapBufferWithTrash(2, &AcqDevice);
		AcqDeviceToBuf = SapAcqDeviceToBuf(&AcqDevice, &Buffers,
			XferCallback, this);
		Xfer = &AcqDeviceToBuf;

		if (!AcqDevice.Create())
		{
			qWarning() << __FUNCTION__ << "AcqDevice.Create() failed";
			return false;
		}
		if (cfgFilename != "NoFile")
		{
			if (!AcqDevice.LoadFeatures(cfgFilename.c_str()))
			{
				qWarning() << __FUNCTION__ << "AcqDevice.LoadConfig() failed";
				return false;
			}
		}
	}*/
	else
	{
		qWarning() << __FUNCTION__ << "No Acq or AcqDevice resource";
		return false;
	}

	/* 创建缓冲区 */
	if (!Buffers.Create())
	{
		qWarning() << __FUNCTION__ << "Buffers.Create() failed";
		return false;
	}

	/* 创建传输对象 */
	if (Xfer == nullptr)
	{
		qWarning() << __FUNCTION__ << "Xfer is NULL";
		return false;
	}

	if (!Xfer->Create())
	{
		qWarning() << __FUNCTION__ << "Xfer->Create() failed";
		return false;
	}

	/* 初始化特征辅助（Gain/Exposure 控制） */
	InitFeatureHelper();

	return true;
}

void __stdcall Hd_CameraModule_Dalsa3::ReconnectDevice(unsigned int /*nMsgType*/,
	void* /*pUser*/)
{
	qWarning() << "ReconnectDevice: camera disconnected!";
}

static void XferCallback(SapXferCallbackInfo* pInfo)
{
	double time_Start = (double)clock();
	QList<cv::Mat> OutMats;

	Hd_CameraModule_Dalsa3* pCam =
		reinterpret_cast<Hd_CameraModule_Dalsa3*>(pInfo->GetContext());
	if (pCam == nullptr)
	{
		qWarning() << "XferCallback: pCam is NULL";
		return;
	}

	if (pInfo->IsTrash())
	{
		qDebug() << "Frames acquired in trash buffer:"
			<< pInfo->GetEventCount();
	}
	else
	{
		void* pAddr = nullptr;
		if (!pCam->Buffers.GetAddress(&pAddr) || pAddr == nullptr)
		{
			qWarning() << "XferCallback: GetAddress failed or NULL";
			return;
		}

		int w = pCam->Buffers.GetWidth();
		int h = pCam->Buffers.GetHeight();
		int depth = pCam->Buffers.GetPixelDepth();

		cv::Mat img;
		if (depth == 8)
		{
			img = cv::Mat(h, w, CV_8U, pAddr);
		}
		else if (depth == 16)
		{
			img = cv::Mat(h, w, CV_16U, pAddr);
		}
		else
		{
			qWarning() << "XferCallback: unsupported pixel depth" << depth;
			return;
		}

		OutMats.push_back(img.clone());

		if (pCam->triggerMode.load(std::memory_order_acquire) == 0)
		{
			/* 软触发模式：推入队列 */
			pCam->MatQueue.push(OutMats);
		}
		else
		{
			/* 硬触发模式：直接回调 */
			if (pCam->CallbackFuncMap.contains(pCam->Currentindex))
			{
				QObject* obj =
					pCam->CallbackFuncMap.value(pCam->Currentindex).callbackparent;
				obj->setProperty("cameraIndex",
					QString::number(pCam->Currentindex));
				pCam->CallbackFuncMap.value(pCam->Currentindex)
					.GetimagescallbackFunc(obj, OutMats);
			}
			else
			{
				qWarning() << "CallbackFuncMap missing index:"
					<< pCam->Currentindex;
			}
		}
	}

	pCam->Currentindex++;
	// 防呆：除数为 0 时按 1 处理，避免抓图回调内除零崩溃；getImageMaxCoiunts 为 0 时禁止轮转
	const int divisor = qMax(pCam->OnceGetImageNum, 1);
	if (pCam->getImageMaxCoiunts > 0
		&& pCam->Currentindex >= pCam->getImageMaxCoiunts / divisor)
	{
		pCam->Currentindex = 0;
	}

	if (pCam->m_bExposureSequenceEnabled)
	{
		pCam->ApplyExposureByIndex();
	}
	if (pCam->m_bGainSequenceEnabled)
	{
		pCam->ApplyGainByIndex();
	}
	double time_End = (double)clock();
	qDebug() << "XferCallback time:" << (time_End - time_Start) << "ms"
		<< "Camera:" << pCam->Sncode
		<< "index:" << pCam->Currentindex;
}


/* ========================================================================
 *  类构造与析构
 * ======================================================================== */

Hd_CameraModule_Dalsa3::Hd_CameraModule_Dalsa3(QString sn, QString path,
	int settype, QObject* parent)
	: PbGlobalObject(settype, parent)
	, Sncode(sn)
	, RootPath(path)
	, m_pFeatureDevice(nullptr)
	, m_pFeature(nullptr)
	, m_bGainAvailable(false)
	, m_bGainIsDouble(false)
	, m_dGainMin(0.0)
	, m_dGainMax(0.0)
	, m_bExposureAvailable(false)
	, m_dExposureMinUs(0.0)
	, m_dExposureMaxUs(0.0)
	, m_bExposureSequenceEnabled(false)
	, m_bGainSequenceEnabled(false)
{
	famliy = CAMERA2D;
	m_szGainName[0] = '\0';
	JsonFilePath = RootPath + Sncode + ".json";

	/* 默认 JSON 配置 */
	QString FirstCreateByte(
		R"({
    "acqDeviceNumber": ")" + sn + R"(",
    "GetOnceImageTimes": "100000",
    "LastUpdateTime": "",
    "OnceImageCounts": "1",
    "OnceSignalsGetImageCounts": "20",
    "configFilename": "D:\\Camera\\260529\\STW-1-1.ccf",
    "TriggerMode": "1",
    "ExposureSequence": "",
    "GainSequence": ""
})");

	if (!QFile(JsonFilePath).exists())
	{
		createAndWritefile(JsonFilePath, FirstCreateByte.toUtf8());
	}

	QJsonObject paramObj = load_JsonFile(JsonFilePath);
	for (const auto& key : paramObj.keys())
	{
		ParasValueMap.insert(key, paramObj.value(key).toString());
	}
}

Hd_CameraModule_Dalsa3::~Hd_CameraModule_Dalsa3()
{
	CloseDevice();
}

QMap<QString, QString> Hd_CameraModule_Dalsa3::parameters()
{
	return ParasValueMap;
}
bool Hd_CameraModule_Dalsa3::setParameter(const QMap<QString, QString>& ParameterMap)
{
	// 防呆：取图次数/出图数量下限为 1、超时下限 10ms，避免回调内除零（SIGFPE）与立即超时
	timeOut = qMax(ParameterMap.value("GetOnceImageTimes").toInt(), 10);
	OnceGetImageNum = qMax(ParameterMap.value("OnceImageCounts").toInt(), 1);
	getImageMaxCoiunts = qMax(ParameterMap.value("OnceSignalsGetImageCounts").toInt(), 1);
	configFilename = ParameterMap.value("configFilename");
	type1 = ParameterMap.value("TriggerMode").toInt();

	if (type1 == 0)
	{
		triggerMode.store(1, std::memory_order_release);
		if (Xfer != nullptr)
		{
			Xfer->Grab();
		}
	}

	/* 解析曝光序列: "index1:value1,index2:value2" */
	QString seqStr = ParameterMap.value("ExposureSequence");
	m_mapExposureSequence.clear();
	m_bExposureSequenceEnabled = false;

	if (!seqStr.isEmpty())
	{
		QStringList parts = seqStr.split(',');
		for (const QString& part : parts)
		{
			QStringList kv = part.split(':');
			if (kv.size() != 2)
			{
				continue;
			}

			bool idxOk = false, valOk = false;
			int    idx = kv[0].trimmed().toInt(&idxOk);
			double value = kv[1].trimmed().toDouble(&valOk);

			if (idxOk && valOk && idx >= 0 && value > 0.0)
			{
				m_mapExposureSequence.insert(idx, value);
			}
		}
		m_bExposureSequenceEnabled = !m_mapExposureSequence.isEmpty();
	}
	QString GainStr = ParameterMap.value("GainSequence");
	m_mapGainSequence.clear();
	m_bGainSequenceEnabled = false;
	if (!GainStr.isEmpty())
	{
		QStringList parts = GainStr.split(',');
		for (const QString& part : parts)
		{
			QStringList kv = part.split(':');
			if (kv.size() != 2)
			{
				continue;
			}
			bool idxOk = false, valOk = false;
			int    idx = kv[0].trimmed().toInt(&idxOk);
			double value = kv[1].trimmed().toDouble(&valOk);
			if (idxOk && valOk && idx >= 0 && value > 0.0)
			{
				m_mapGainSequence.insert(idx, value);
			}
		}
		m_bGainSequenceEnabled = !m_mapGainSequence.isEmpty();
	}

	ParasValueMap = ParameterMap;
	return true;
}
//初始化(加载模块待内存)
bool Hd_CameraModule_Dalsa3::init()
{
	connect(this, &PbGlobalObject::trigged, [=](int Code) {
		switch (Code)
		{
		case 1000:
			Currentindex = 0;
			MatQueue.clear();
			allowflag.store(true, std::memory_order_release);
			if (m_bExposureSequenceEnabled)
			{
				ApplyExposureByIndex();
			}
			emit trigged(501);
			break;
		case 1001:
			allowflag.store(false, std::memory_order_release);
			break;
		default:
			allowflag.store(true, std::memory_order_release);
			break;
		}
		});

	setParameter(ParasValueMap);

	if (!connctDevice(GetSn().toStdString(), GetconfigFilename().toStdString()))
	{
		qWarning() << __FUNCTION__ << "Camera init FAILED";
		return false;
	}

	if (Xfer == nullptr)
	{
		qWarning() << __FUNCTION__ << "Xfer is NULL after connctDevice";
		return false;
	}

	if (type1 == 0)
	{
		triggerMode.store(1, std::memory_order_release);
		Xfer->Grab();
	}
	else
	{
		triggerMode.store(0, std::memory_order_release);
	}

	qDebug() << __FUNCTION__ << "Camera Init Success:" << GetSn();
	return true;
}

bool Hd_CameraModule_Dalsa3::setData(const std::vector<cv::Mat>& mats,
	const QStringList& data)
{
	Q_UNUSED(mats);
	Q_UNUSED(data);

	if (mats.empty() && data.isEmpty())
	{
		/* 软触发：Xfer 必须在初始化时已创建 */
		if (Xfer == nullptr)
		{
			qWarning() << __FUNCTION__ << "Xfer is NULL, cannot snap";
			return false;
		}

		Xfer->Init(true);
		Xfer->Snap(1);
		return true;
	}

	return true;
}
/* ========================================================================
 *  data() / register / cancel
 * ======================================================================== */

bool Hd_CameraModule_Dalsa3::data(std::vector<cv::Mat>& ImgS,
	QStringList& QStringListdata)
{
	Q_UNUSED(QStringListdata);
	QList<cv::Mat> img;

	if (!MatQueue.wait_for_pop(timeOut, img))
	{
		/* 超时返回空图 */
		ImgS.push_back(cv::Mat::zeros(100, 100, CV_8UC1));
		qWarning() << __FUNCTION__ << "wait_for_pop timeout";
		return false;
	}

	for (int i = 0; i < img.size(); i++)
	{
		ImgS.push_back(img[i].clone());
	}

	if (ImgS.empty())
	{
		ImgS.push_back(cv::Mat::zeros(100, 100, CV_8UC1));
		qWarning() << __FUNCTION__ << "result is empty";
		return false;
	}

	return true;
}

void Hd_CameraModule_Dalsa3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN func,
	QObject* parent,
	const QString& getString)
{
	CallbackFuncPack pack;
	pack.callbackparent = parent;
	pack.cameraIndex = getString;
	pack.GetimagescallbackFunc = func;
	CallbackFuncMap.insert(getString.toInt(), pack);
}

void Hd_CameraModule_Dalsa3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun,
	QObject* parent,
	const QString& getString)
{
	int index = getString.toInt();
	if (CallbackFuncMap.contains(index))
	{
		if (callBackFun == CallbackFuncMap.value(index).GetimagescallbackFunc)
		{
			CallbackFuncMap.remove(index);
		}
		else
		{
			qWarning() << "cancelCallBackFun: func mismatch for key" << getString;
		}
	}
}

/* ========================================================================
 *  相机特征控制（Gain / Exposure / 通用）
 * ======================================================================== */

bool Hd_CameraModule_Dalsa3::InitFeatureHelper()
{
	/* 释放旧对象 */
	CleanupFeatureHelper();

	/* 创建独立的特征操控设备 */
	QStringList keys = DeviceListsMap.keys();
	auto func = [=]() {
		for (int i = 0; i < keys.size(); i++) {
			if (keys.at(i).startsWith(GetSn()))
				return DeviceListsMap.value(keys.at(i));
		}
		return QString("empty");
		};
	QString linkName = func();
	if (linkName != "empty")
	{
		m_pFeatureDevice = new SapAcqDevice(linkName.toStdString().c_str());
	}
	else
	{
		return false;
	}
	if (m_pFeatureDevice == nullptr)
	{
		qWarning() << "InitFeatureHelper: new SapAcqDevice failed";
		return false;
	}
	else
	{
		if (!m_pFeatureDevice->Create())
		{
			qWarning() << "InitFeatureHelper: Create() failed";
			delete m_pFeatureDevice;
			m_pFeatureDevice = nullptr;
			return false;
		}

	}
	m_pFeature = new SapFeature(m_pFeatureDevice->GetLocation());
	if (m_pFeature == nullptr)
	{
		qWarning() << "InitFeatureHelper: new SapFeature failed";
		delete m_pFeature;
		m_pFeature = nullptr;
		return false;
	}

	if (!m_pFeature->Create())
	{
		qWarning() << "m_pFeature: Create() failed";
		delete m_pFeature;
		m_pFeature = nullptr;
		return false;
	}
	/* 探测 Gain 特征 */
	m_bGainAvailable = false;
	int isAvail = false;
	m_szGainName[0] = '\0';

	if (m_pFeatureDevice->IsFeatureAvailable("Gain", &isAvail) && isAvail)
	{
		snprintf(m_szGainName, sizeof(m_szGainName) - 1, "Gain");
	}
	else if (m_pFeatureDevice->IsFeatureAvailable("GainRaw", &isAvail) && isAvail)
	{
		snprintf(m_szGainName, sizeof(m_szGainName) - 1, "GainRaw");
	}

	if (m_szGainName[0] != '\0')
	{
		m_szGainName[sizeof(m_szGainName) - 1] = '\0';
		if (m_pFeatureDevice->GetFeatureInfo(m_szGainName, m_pFeature))
		{
			SapFeature::Type type = SapFeature::TypeUndefined;
			m_pFeature->GetType(&type);

			if (type == SapFeature::TypeDouble || type == SapFeature::TypeFloat)
			{
				m_bGainIsDouble = true;
				double dMin = 0.0, dMax = 0.0;
				m_pFeature->GetMin(&dMin);
				m_pFeature->GetMax(&dMax);
				m_dGainMin = dMin;
				m_dGainMax = dMax;
			}
			else
			{
				m_bGainIsDouble = false;
				UINT32 uMin = 0, uMax = 0;
				m_pFeature->GetMin(&uMin);
				m_pFeature->GetMax(&uMax);
				m_dGainMin = static_cast<double>(uMin);
				m_dGainMax = static_cast<double>(uMax);
			}

			m_bGainAvailable = true;
			qDebug() << "[Gain]" << m_szGainName
				<< "range:" << m_dGainMin << "-" << m_dGainMax
				<< "isDouble:" << m_bGainIsDouble;
		}
	}

	/* 探测 ExposureTime 特征 */
	m_bExposureAvailable = false;
	if (m_pFeatureDevice->IsFeatureAvailable("ExposureTime", &isAvail) && isAvail)
	{
		if (m_pFeatureDevice->GetFeatureInfo("ExposureTime", m_pFeature))
		{
			double dMin = 0.0, dMax = 0.0;
			if (m_pFeature->GetMin(&dMin) && m_pFeature->GetMax(&dMax))
			{
				m_dExposureMinUs = dMin;
				m_dExposureMaxUs = dMax;
			}
			else
			{
				UINT32 uMin = 0, uMax = 0;
				if (m_pFeature->GetMin(&uMin) && m_pFeature->GetMax(&uMax))
				{
					m_dExposureMinUs = static_cast<double>(uMin);
					m_dExposureMaxUs = static_cast<double>(uMax);
				}
				else
				{
					m_dExposureMinUs = 0.0;
					m_dExposureMaxUs = 0.0;
				}
			}

			m_bExposureAvailable = true;
			qDebug() << "[ExposureTime] range:"
				<< m_dExposureMinUs << "-" << m_dExposureMaxUs << "us";
			double currentExposure = 0.0;
			bool flag = GetExposureTimeUs(&currentExposure);
			qDebug() << "[ExposureTime] get:"
				<< currentExposure << "us"<< flag;
			bool flag1 = SetExposureTimeUs(currentExposure);
			qDebug() << "[ExposureTime] get:"
				<< currentExposure << "us" << flag1;
		}
	}

	return (m_bGainAvailable || m_bExposureAvailable);
}

void Hd_CameraModule_Dalsa3::CleanupFeatureHelper()
{
	if (m_pFeature != nullptr)
	{
		m_pFeature->Destroy();
		delete m_pFeature;
		m_pFeature = nullptr;
	}

	if (m_pFeatureDevice != nullptr)
	{
		m_pFeatureDevice->Destroy();
		delete m_pFeatureDevice;
		m_pFeatureDevice = nullptr;
	}

	m_bGainAvailable = false;
	m_bExposureAvailable = false;
}

bool Hd_CameraModule_Dalsa3::SetGain(double value)
{
	if (m_pFeatureDevice == nullptr || m_pFeature == nullptr)
	{
		qWarning() << "SetGain: feature helper not initialized";
		return false;
	}

	if (!m_bGainAvailable)
	{
		qWarning() << "SetGain: Gain feature not available";
		return false;
	}

	/* 范围校验 */
	if (value < m_dGainMin || value > m_dGainMax)
	{
		qWarning() << "SetGain: value" << value << "out of range ["
			<< m_dGainMin << "," << m_dGainMax << "]";
		return false;
	}

	BOOL ok = FALSE;
	if (m_bGainIsDouble)
	{
		ok = m_pFeatureDevice->SetFeatureValue(m_szGainName, value);
	}
	else
	{
		if (value < 0.0 || value > static_cast<double>(UINT32_MAX))
		{
			qWarning() << "SetGain: value" << value << "out of UINT32 range";
			return false;
		}
		UINT32 intVal = static_cast<UINT32>(value + 0.5);
		ok = m_pFeatureDevice->SetFeatureValue(m_szGainName, intVal);
	}

	if (!ok)
	{
		qWarning() << "SetGain: SetFeatureValue failed";
	}
	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::GetGain(double* pValue)
{
	if (pValue == nullptr || m_pFeatureDevice == nullptr)
	{
		return false;
	}

	*pValue = 0.0;

	if (!m_bGainAvailable)
	{
		return false;
	}

	BOOL ok = FALSE;
	if (m_bGainIsDouble)
	{
		double val = 0.0;
		ok = m_pFeatureDevice->GetFeatureValue(m_szGainName, &val);
		if (ok)
		{
			*pValue = val;
		}
	}
	else
	{
		UINT32 val = 0;
		ok = m_pFeatureDevice->GetFeatureValue(m_szGainName, &val);
		if (ok)
		{
			*pValue = static_cast<double>(val);
		}
	}

	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::GetGainRange(double* pMin, double* pMax)
{
	if (pMin == nullptr || pMax == nullptr)
	{
		return false;
	}

	*pMin = 0.0;
	*pMax = 0.0;

	if (!m_bGainAvailable)
	{
		return false;
	}

	*pMin = m_dGainMin;
	*pMax = m_dGainMax;
	return true;
}

bool Hd_CameraModule_Dalsa3::SetExposureTimeUs(double valueUs)
{
	if (m_pFeatureDevice == nullptr)
	{
		qWarning() << "SetExposureTimeUs: feature helper not initialized";
		return false;
	}

	if (!m_bExposureAvailable)
	{
		qWarning() << "SetExposureTimeUs: ExposureTime not available";
		return false;
	}

	/* 范围校验 */
	if (valueUs < m_dExposureMinUs || valueUs > m_dExposureMaxUs)
	{
		qWarning() << "SetExposureTimeUs: value" << valueUs << "us out of range ["
			<< m_dExposureMinUs << "," << m_dExposureMaxUs << "]";
		return false;
	}

	BOOL ok = m_pFeatureDevice->SetFeatureValue("ExposureTime", valueUs);
	if (!ok)
	{
		qWarning() << "SetExposureTimeUs: SetFeatureValue failed";
	}
	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::GetExposureTimeUs(double* pValueUs)
{
	if (pValueUs == nullptr || m_pFeatureDevice == nullptr)
	{
		return false;
	}

	*pValueUs = 0.0;

	if (!m_bExposureAvailable)
	{
		return false;
	}

	double val = 0.0;
	BOOL ok = m_pFeatureDevice->GetFeatureValue("ExposureTime", &val);
	if (ok)
	{
		*pValueUs = val;
	}
	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::GetExposureTimeRange(double* pMinUs, double* pMaxUs)
{
	if (pMinUs == nullptr || pMaxUs == nullptr)
	{
		return false;
	}

	*pMinUs = 0.0;
	*pMaxUs = 0.0;

	if (!m_bExposureAvailable)
	{
		return false;
	}

	*pMinUs = m_dExposureMinUs;
	*pMaxUs = m_dExposureMaxUs;
	return true;
}

void Hd_CameraModule_Dalsa3::ApplyExposureByIndex()
{
	if (m_mapExposureSequence.isEmpty())
	{
		return;
	}

	if (!m_mapExposureSequence.contains(Currentindex))
	{
		return;  /* 当前帧无需切换曝光 */
	}

	double exposureUs = m_mapExposureSequence.value(Currentindex);
	SetExposureTimeUs(exposureUs);
}


void Hd_CameraModule_Dalsa3::ApplyGainByIndex()
{
	if (m_mapGainSequence.isEmpty())
	{
		return;
	}

	if (!m_mapGainSequence.contains(Currentindex))
	{
		return;  /* 当前帧无需切换增益 */
	}

	double gain = m_mapGainSequence.value(Currentindex);
	SetGain(gain);
}
bool Hd_CameraModule_Dalsa3::IsFeatureAvailable(const char* featureName)
{
	if (m_pFeatureDevice == nullptr || featureName == nullptr)
	{
		return false;
	}

	BOOL isAvail = FALSE;
	if (!m_pFeatureDevice->IsFeatureAvailable(featureName, &isAvail))
	{
		return false;
	}
	return (isAvail == TRUE);
}

bool Hd_CameraModule_Dalsa3::GetFeatureValue(const char* featureName,
	double* pValue)
{
	if (m_pFeatureDevice == nullptr || featureName == nullptr || pValue == nullptr)
	{
		return false;
	}

	*pValue = 0.0;

	double val = 0.0;
	BOOL ok = m_pFeatureDevice->GetFeatureValue(featureName, &val);
	if (ok)
	{
		*pValue = val;
	}
	else
	{
		/* 尝试整型读取 */
		UINT32 intVal = 0;
		ok = m_pFeatureDevice->GetFeatureValue(featureName, &intVal);
		if (ok)
		{
			*pValue = static_cast<double>(intVal);
		}
	}

	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::SetFeatureValue(const char* featureName,
	double value)
{
	if (m_pFeatureDevice == nullptr || featureName == nullptr)
	{
		return false;
	}

	BOOL ok = m_pFeatureDevice->SetFeatureValue(featureName, value);
	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::GetFeatureInt(const char* featureName,
	UINT32* pValue)
{
	if (m_pFeatureDevice == nullptr || featureName == nullptr || pValue == nullptr)
	{
		return false;
	}

	*pValue = 0;

	UINT32 val = 0;
	BOOL ok = m_pFeatureDevice->GetFeatureValue(featureName, &val);
	if (ok)
	{
		*pValue = val;
	}
	return (ok == TRUE);
}

bool Hd_CameraModule_Dalsa3::SetFeatureInt(const char* featureName,
	UINT32 value)
{
	if (m_pFeatureDevice == nullptr || featureName == nullptr)
	{
		return false;
	}

	BOOL ok = m_pFeatureDevice->SetFeatureValue(featureName, value);
	return (ok == TRUE);
}

/* ========================================================================
 *  extern "C" 导出函数
 * ======================================================================== */

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
	if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
	{
		return false;
	}

	if (TotalMap.contains(name.split(':').first()))
	{
		return true;  /* 已存在 */
	}

	OnePb temp;
	temp.base = new Hd_CameraModule_Dalsa3(DeviceSn,
		path + "/Hd_CameraModule_Dalsa3/");
	if (!temp.base->init())
	{
		delete temp.base;
		qWarning() << "create: init() failed for" << DeviceSn;
		return false;
	}

	temp.baseWidget = new mPrivateWidget(temp.base);
	temp.DeviceSn = DeviceSn;

	TotalMap.insert(name.split(':').first(), temp);
	return true;
}

void destroy(const QString& name)
{
	OnePb temp = TotalMap.take(name);
	if (temp.base != nullptr)
	{
		delete temp.base;
		temp.base = nullptr;
	}
	if (temp.baseWidget != nullptr)
	{
		delete temp.baseWidget;
		temp.baseWidget = nullptr;
	}
}

QWidget* getCameraWidgetPtr(const QString& name)
{
	if (TotalMap.contains(name) && TotalMap.value(name).baseWidget != nullptr)
	{
		return TotalMap.value(name).baseWidget;
	}
	return nullptr;
}

PbGlobalObject* getCameraPtr(const QString& name)
{
	if (TotalMap.contains(name) && TotalMap.value(name).base != nullptr)
	{
		return TotalMap.value(name).base;
	}
	return nullptr;
}

QStringList getCameraSnList()
{
	QStringList result;
	QStringList CameraSeverName = SearchDevice();

	if (CameraSeverName.isEmpty())
	{
		return result;
	}

	result = CameraSeverName;

	/* 排除已使用的设备 */
	QMapIterator<QString, OnePb> it(TotalMap);
	while (it.hasNext())
	{
		it.next();
		if (result.contains(it.value().DeviceSn))
		{
			result.removeOne(it.value().DeviceSn);
		}
	}

	return result;
}

/* ========================================================================
 *  mPrivateWidget UI
 * ======================================================================== */

mPrivateWidget::mPrivateWidget(void* handle)
{
	m_Camerahandle = reinterpret_cast<Hd_CameraModule_Dalsa3*>(handle);
	InitWidget();
}

void mPrivateWidget::InitWidget()
{
	QHBoxLayout* mainHboxLayout = new QHBoxLayout(this);
	QVBoxLayout* mainVLayout = new QVBoxLayout;

	SetDataBtn = new QPushButton(tr("软触发"), this);
	OpenGrapMat = new QPushButton(tr("允许取图"), this);
	NotGrapMat = new QPushButton(tr("禁止取图"), this);
	m_showimage = new ImageViewer(this);
	m_AlgParmWidget = new AlgParmWidget(
		m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");

	/* 算法参数修改 → 重新设置参数 */
	connect(m_AlgParmWidget, &AlgParmWidget::SengCurrentByte, this,
		[=](QByteArray byte) {
			QJsonObject paramObj = QJsonDocument::fromJson(byte).object();
			QMap<QString, QString> paramMap;
			for (const auto& key : paramObj.keys())
			{
				paramMap.insert(key, paramObj.value(key).toString());
			}
			m_Camerahandle->setParameter(paramMap);
		});

	mainVLayout->addWidget(m_showimage);
	mainVLayout->addWidget(SetDataBtn);
	mainVLayout->addWidget(OpenGrapMat);
	mainVLayout->addWidget(NotGrapMat);

	connect(OpenGrapMat, &QPushButton::clicked, this, [=]() {
		emit m_Camerahandle->trigged(1000);
		});
	connect(NotGrapMat, &QPushButton::clicked, this, [=]() {
		emit m_Camerahandle->trigged(1001);
		});
	connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;
		QStringList list;
		emit m_Camerahandle->trigged(1000);
		m_Camerahandle->setData(mats, list);
		m_Camerahandle->data(mats, list);
		if (!mats.empty())
		{
			cv::Mat tempMat = mats.at(0);
			m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
		}
		});

	mainHboxLayout->addLayout(mainVLayout, 4);
	mainHboxLayout->addWidget(m_AlgParmWidget, 3);
}