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
	return 1;
}

bool connctDevice(string GetSnName, void* handle, void* pUser)
{
	CameraFunSDKfactoryCls* CurrentCamera = (CameraFunSDKfactoryCls*)(pUser);
	if (!SearchDevice())
		return false;
	int index = 0;
	for (; index < m_stDevList.nDeviceNum; index++)
	{

		unsigned char* name = m_stDevList.pDeviceInfo[index]->SpecialInfo.stGigEInfo.chSerialNumber;
		//userID
		//unsigned char* name=m_stDevList.pDeviceInfo[i]->SpecialInfo.stGigEInfo.chUserDefinedName;
		string SnName = static_cast<string>((LPCSTR)name);
		if (SnName == GetSnName)
			break;

	}
	unsigned char* name = m_stDevList.pDeviceInfo[index]->SpecialInfo.stGigEInfo.chUserDefinedName;
	string userName = static_cast<string>((LPCSTR)name);
	qDebug() << "[INFO] " << " start" << "--UserName:" << QString::fromStdString(userName);
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
	qDebug() << "pUser0" << CurrentCamera;
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
	std::vector<cv::Mat> OutMats;
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
			qDebug() << CurrentCamera->Currentindex;
			///硬触发不受开关控制，没有缓存
			if (CurrentCamera->CallbackFuncMap.keys().contains(CurrentCamera->Currentindex))
			{
				QObject* obj = CurrentCamera->CallbackFuncMap.value(CurrentCamera->Currentindex).callbackparent;
				obj->setProperty("cameraIndex", QString::number(CurrentCamera->Currentindex));
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
	}
	if (CurrentCamera->gainMap.count(CurrentCamera->Currentindex) == 1)
	{
		MV_CC_SetFloatValue(CurrentCamera->handle, "Gain", CurrentCamera->gainMap[CurrentCamera->Currentindex]);
	}
	if (CurrentCamera->gammaMap.count(CurrentCamera->Currentindex) == 1)
	{
		MV_CC_SetFloatValue(CurrentCamera->handle, "Gamma", CurrentCamera->gammaMap[CurrentCamera->Currentindex]);
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
		for (auto &str : array)
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
		for (auto& str : array)
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
		for (auto& str : array)
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
		else if (Code == 1)
		{
			m_sdkFunc->triggerMode.store(1, std::memory_order::memory_order_release);

		}
		else if (Code == 0)
		{
			m_sdkFunc->triggerMode.store(0, std::memory_order::memory_order_release);
		}
		});
	setParameter(ParasValueMap);
	bool flag = m_sdkFunc->initSdk(ParasValueMap);
	if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
		type1 = 1;
	else if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE < 4)
		type1 = 0;
	type2 = 0;//不需要需要触发器，出图完成后给plc信号即可
	//qDebug() << m_sdkFunc->handle;
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

	m_sdkFunc->MatQueue.wait_for_pop(m_sdkFunc->timeOut, ImgS);
	if (ImgS.empty())
	{
		ImgS.push_back(cv::Mat::zeros(100, 100, 0));
		qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
		return false;
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
	QHBoxLayout* mainHboxLayout = new QHBoxLayout(this);
	QVBoxLayout* MainLayout = new QVBoxLayout;
	QGridLayout* girlayout = new QGridLayout();
	{
		QLabel* triggerModel = new QLabel(this);
		triggerModel->setText(tr("触发模式:"));
		QLabel* triggerSoure = new QLabel(this);
		triggerSoure->setText(tr("触发源:"));

		QComboBox* first = new QComboBox(this);
		first->addItems(QStringList() << "打开" << "关闭");
		connect(first, &QComboBox::currentTextChanged, this, [=](QString text) {
			if (text == "打开")
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerMode", MV_TRIGGER_MODE_ON);
				emit m_Camerahandle->trigged(1);
			}
			else
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerMode", MV_TRIGGER_MODE_OFF);
				emit m_Camerahandle->trigged(0);
			}
			});
		QComboBox* Second = new QComboBox(this);
		Second->addItems(QStringList() << "Line0" << "Line1" << "Line2" << "Line3" << "软触发");
		connect(Second, &QComboBox::currentTextChanged, this, [=](QString text) {
			if (text == "Line0")
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
			}
			else if (text == "Line1")
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", MV_TRIGGER_SOURCE_LINE1);
			}
			else if (text == "Line2")
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", MV_TRIGGER_SOURCE_LINE2);
			}
			else if (text == "Line3")
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", MV_TRIGGER_SOURCE_LINE3);
			}
			else if (text == "软触发")
			{
				MV_CC_SetEnumValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
			}
			});

		//ui->lineEdit->setValidator(doubleValidator);
		QLineEdit* gain = new QLineEdit();
		QLineEdit* Gama = new QLineEdit();
		QLineEdit* Exposure = new QLineEdit();

		QDoubleValidator* doubleValidator1 = new QDoubleValidator(0.00, 10000000.0, 4, this);
		doubleValidator1->setNotation(QDoubleValidator::StandardNotation);
		MVCC_FLOATVALUE stFloatValue = { 0 };
		MV_CC_GetFloatValue(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", &stFloatValue);
		Exposure->setText(QString::number(stFloatValue.fCurValue, 'f', 4));
		doubleValidator1->setBottom(stFloatValue.fMin);
		doubleValidator1->setTop(stFloatValue.fMax);
		Exposure->setValidator(doubleValidator1);

		QDoubleValidator* doubleValidator2 = new QDoubleValidator(0.00, 10000000.0, 4, this);
		doubleValidator2->setNotation(QDoubleValidator::StandardNotation);
		MV_CC_GetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gain", &stFloatValue);
		gain->setText(QString::number(stFloatValue.fCurValue, 'f', 4));
		doubleValidator2->setBottom(stFloatValue.fMin);
		doubleValidator2->setTop(stFloatValue.fMax);
		gain->setValidator(doubleValidator2);

		QDoubleValidator* doubleValidator3 = new QDoubleValidator(0.00, 10000000.0, 4, this);
		doubleValidator3->setNotation(QDoubleValidator::StandardNotation);
		MV_CC_GetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gamma", &stFloatValue);
		Gama->setText(QString::number(stFloatValue.fCurValue, 'f', 4));
		doubleValidator3->setBottom(stFloatValue.fMin);
		doubleValidator3->setTop(stFloatValue.fMax);
		Gama->setValidator(doubleValidator3);
		connect(gain, &QLineEdit::editingFinished, this, [=]() {
			MV_CC_SetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gain", float(gain->text().toFloat()));
			});
		connect(Gama, &QLineEdit::editingFinished, this, [=]() {
			MV_CC_SetFloatValue(m_Camerahandle->m_sdkFunc->handle, "Gamma", float(Gama->text().toFloat()));
			});
		connect(Exposure, &QLineEdit::editingFinished, this, [=]() {
			MV_CC_SetFloatValue(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", float(Exposure->text().toFloat()));

			});
		QLabel* GainL = new QLabel();
		QLabel* GamaL = new QLabel();
		QLabel* ExposureL = new QLabel();
		GainL->setText(tr("增益(db)"));
		GamaL->setText(tr("伽马校正"));
		ExposureL->setText(tr("曝光时间(us)"));
		girlayout->addWidget(triggerModel, 0, 0);
		girlayout->addWidget(first, 0, 1);
		girlayout->addWidget(triggerSoure, 1, 0);
		girlayout->addWidget(Second, 1, 1);
		girlayout->addWidget(GainL, 2, 0);
		girlayout->addWidget(gain, 2, 1);
		girlayout->addWidget(GamaL, 3, 0);
		girlayout->addWidget(Gama, 3, 1);
		girlayout->addWidget(ExposureL, 4, 0);
		girlayout->addWidget(Exposure, 4, 1);
	}
	{



	}
	girlayout->setAlignment(Qt::AlignTop);
	QGroupBox* groupBox = new QGroupBox(this);
	groupBox->setTitle(tr("相机参数设置"));
	groupBox->setLayout(girlayout);
	QVBoxLayout* AlgParmLayout = new QVBoxLayout();
	AlgParmLayout->addWidget(groupBox);
	{
		QFile file(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
		file.open(QIODevice::ReadOnly);
		QByteArray byte = file.readAll();
		file.close();
		QStackedWidget* changeWidget = new QStackedWidget(this);
		QComboBox* comboBox = new QComboBox(this);
		comboBox->addItems(QStringList() << tr("增益(db)") << tr("伽马校正") << tr("曝光时间(us)"));
		QGroupBox* groupBoxliuc = new QGroupBox(this);
		groupBoxliuc->setTitle(tr("流程参数设置"));

		QVBoxLayout* vLayout = new QVBoxLayout();
		vLayout->addWidget(comboBox);
		vLayout->addWidget(changeWidget);
		groupBoxliuc->setLayout(vLayout);
		AlgParmLayout->addWidget(groupBoxliuc);

		BytePtr  = QJsonDocument::fromJson(byte).object();

		MyTableWidget* gainTable = new MyTableWidget(this, BytePtr.value("Gain").toArray());
		gainTable->setProperty("Name", "Gain");
		MyTableWidget* GamaTable = new MyTableWidget(this, BytePtr.value("Gamma").toArray());
		GamaTable->setProperty("Name", "Gama");
		MyTableWidget* ExposureTimeTable = new MyTableWidget(this, BytePtr.value("ExposureTime").toArray());
		ExposureTimeTable->setProperty("Name", "ExposureTime");
		changeWidget->addWidget(gainTable);
		changeWidget->addWidget(GamaTable);
		changeWidget->addWidget(ExposureTimeTable);
		connect(gainTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
		connect(GamaTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
		connect(ExposureTimeTable, &MyTableWidget::SendCurrentResult, this, &mPrivateWidget::getRes);
		QObject::connect(comboBox, &QComboBox::currentTextChanged, this, [=](QString index) {
			changeWidget->setCurrentIndex(comboBox->currentIndex());

			});
	}
	SetDataBtn = new QPushButton(this);
	SetDataBtn->setText(tr("软触发"));
	OpenGrapMat = new QPushButton(this);
	OpenGrapMat->setText(tr("允许取图"));
	NotGrapMat = new QPushButton(this);
	NotGrapMat->setText(tr("禁止取图"));
	m_showimage = new viewWidget();
	m_showimage->setMinimumSize(500, 500);
	m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
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
	MainLayout->addWidget(m_showimage);
	MainLayout->addWidget(SetDataBtn);
	MainLayout->addWidget(OpenGrapMat);
	MainLayout->addWidget(NotGrapMat);
	auto func = std::bind(&mPrivateWidget::showImage, this, std::placeholders::_1);

	m_Camerahandle->m_sdkFunc->registerGetImageFun(func);
	connect(OpenGrapMat, &QPushButton::clicked, this, [=]() {emit m_Camerahandle->trigged(1000); });
	connect(NotGrapMat, &QPushButton::clicked, this, [=]() {emit m_Camerahandle->trigged(1001); });
	connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;  QStringList list;
		emit m_Camerahandle->trigged(1000);
		m_Camerahandle->setData(mats, list);
		m_Camerahandle->data(mats, list);
		cv::Mat tempMat = mats.at(0);
		m_showimage->reciveImage("", cvMatToQImage(tempMat));
		});
	mainHboxLayout->addLayout(MainLayout);
	mainHboxLayout->addLayout(AlgParmLayout);
	mainHboxLayout->addWidget(m_AlgParmWidget);

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