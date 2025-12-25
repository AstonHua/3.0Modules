#include "Hd_CameraModule_DaHua3.h"
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QTextCodec>
struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;
IMV_DeviceList m_stDevList;
static void onDeviceLinkNotify(const IMV_SConnectArg* pConnectArg, void* pUser);

//设置曝光
void SetExposureTime(void* devHandle, float exposureValue)
{
	qDebug() << "[INFO] " << "exposureValue:" << exposureValue;
	IMV_SetEnumFeatureValue(devHandle, "ExposureAuto", 0);
	IMV_SetDoubleFeatureValue(devHandle, "ExposureTime", exposureValue);
}
//设置增益
void SetGain(void* devHandle, float GainValue)
{
	qDebug() << "[INFO] " << "GainValue:" << GainValue;
	IMV_SetEnumFeatureValue(devHandle, "GainAuto", 0);
	IMV_SetDoubleFeatureValue(devHandle, "GainRaw", GainValue);
}

bool IsColor(IMV_EPixelType enType)
{
	switch (enType)
	{
	case gvspPixelRGB8:
	case gvspPixelBayGR8:
	case gvspPixelBayGB8:
	case gvspPixelBayBG8:
	case gvspPixelBayRG8:
	case gvspPixelBayGR10:
	case gvspPixelBayRG10:
	case gvspPixelBayGB10:
	case gvspPixelBayBG10:
	case gvspPixelBayGR12:
	case gvspPixelBayRG12:
	case gvspPixelBayGB12:
	case gvspPixelBayBG12:
	case gvspPixelBayGR10Packed:
	case gvspPixelBayRG10Packed:
	case gvspPixelBayGB10Packed:
	case gvspPixelBayBG10Packed:
	case gvspPixelBayGR12Packed:
	case gvspPixelBayRG12Packed:
	case gvspPixelBayGB12Packed:
	case gvspPixelBayBG12Packed:
	case gvspPixelYUV422_8_UYVY:
	case gvspPixelYUV411_8_UYYVYY:
	case gvspPixelYUV422_8:
	case gvspPixelYUV8_UYV:
		return true;
	default:
		return false;
	}
}



// 数据帧回调函数
// Data frame callback function
static void onGetFrame(IMV_Frame* pFrame, void* pUser)
{
	double time_Start = (double)clock();
	if (pFrame == NULL)
	{
		printf("pFrame is NULL\n");
		return;
	}
	CameraFunSDKfactoryCls* currentUser = reinterpret_cast<CameraFunSDKfactoryCls*>(pUser);

	int inputIndex = 0;
	//IMV_EPixelType convertFormat = gvspPixelMono8;
	std::vector<cv::Mat> Outmats;
	cv::Mat srcImage = cv::Mat();
	if (pFrame->frameInfo.pixelFormat == gvspPixelMono8)
	{
		srcImage = cv::Mat(pFrame->frameInfo.height, pFrame->frameInfo.width, CV_8UC1, (uint8_t*)pFrame->pData);
	}
	else if (pFrame->frameInfo.pixelFormat == gvspPixelBGR8)
	{
		srcImage = cv::Mat(pFrame->frameInfo.height, pFrame->frameInfo.width, CV_8UC3, (uint8_t*)pFrame->pData);
	}
	else if (pFrame->frameInfo.pixelFormat == gvspPixelBayRG8)
	{
		srcImage = cv::Mat(pFrame->frameInfo.height, pFrame->frameInfo.width, CV_8UC1, (uint8_t*)pFrame->pData);
		cv::cvtColor(srcImage, srcImage, cv::COLOR_BayerRG2BGR);
	}
	else /*if(IsColor(pFrame->frameInfo.pixelFormat))*/ //(pFrame->frameInfo.pixelFormat == gvspPixelRGB8 || pFrame->frameInfo.pixelFormat == gvspPixelYUV422_8_UYVY)
	{
		IMV_PixelConvertParam stPixelConvertParam;
		unsigned char* pDstBuf = NULL;
		unsigned int			nDstBufSize = 0;
		int						ret = IMV_OK;
		const char* pConvertFormatStr = NULL;
		IMV_EPixelType temp = pFrame->frameInfo.pixelFormat;
		if (IsColor(temp))
		{
			//cv::Mat src;
			switch (temp)
			{
			case gvspPixelRGB8:
				nDstBufSize = sizeof(unsigned char) * pFrame->frameInfo.width * pFrame->frameInfo.height * 3;
				pConvertFormatStr = (const char*)"RGB8";
				break;

			case gvspPixelBGR8:
				nDstBufSize = sizeof(unsigned char) * pFrame->frameInfo.width * pFrame->frameInfo.height * 3;

				pConvertFormatStr = (const char*)"BGR8";
				break;
			case gvspPixelBGRA8:
				nDstBufSize = sizeof(unsigned char) * pFrame->frameInfo.width * pFrame->frameInfo.height * 4;

				pConvertFormatStr = (const char*)"BGRA8";
				break;
			case gvspPixelMono8:
				nDstBufSize = sizeof(unsigned char) * pFrame->frameInfo.width * pFrame->frameInfo.height;
				pConvertFormatStr = (const char*)"Mono8";
				break;			
			}

			pDstBuf = (unsigned char*)malloc(nDstBufSize);
			if (NULL == pDstBuf)
			{
				printf("malloc pDstBuf failed!\n");
				return;
			}
		}


		// 图像转换成BGR8
		// convert image to BGR8
		memset(&stPixelConvertParam, 0, sizeof(stPixelConvertParam));
		stPixelConvertParam.nWidth = pFrame->frameInfo.width;
		stPixelConvertParam.nHeight = pFrame->frameInfo.height;
		stPixelConvertParam.ePixelFormat = pFrame->frameInfo.pixelFormat;
		stPixelConvertParam.pSrcData = pFrame->pData;
		stPixelConvertParam.nSrcDataLen = pFrame->frameInfo.size;
		stPixelConvertParam.nPaddingX = pFrame->frameInfo.paddingX;
		stPixelConvertParam.nPaddingY = pFrame->frameInfo.paddingY;
		stPixelConvertParam.eBayerDemosaic = demosaicNearestNeighbor;
		stPixelConvertParam.eDstPixelFormat = gvspPixelBGR8;
		stPixelConvertParam.pDstBuf = pDstBuf;
		stPixelConvertParam.nDstBufSize = nDstBufSize;

		ret = IMV_PixelConvert(currentUser->devHandle, &stPixelConvertParam);
		if (IMV_OK == ret)
		{
			cv::Mat src = cv::Mat(pFrame->frameInfo.height, pFrame->frameInfo.width, CV_8UC3, (uint8_t*)stPixelConvertParam.pDstBuf);
			srcImage = src.clone();
			printf("image convert to %s successfully! nDstDataLen (%u)\n",
				pConvertFormatStr, stPixelConvertParam.nDstBufSize);
			//hFile = fopen(pFileName, "wb");
			//if (hFile != NULL)
			//{
			//    fwrite((void*)pDstBuf, 1, stPixelConvertParam.nDstBufSize, hFile);
			//    fclose(hFile);
			//}
			//else
			//{
			//    // 如果打开失败，请用管理权限执行
			//    // If opefailed, Run as Administrator
			//    printf("Open file (%s) failed!\n", pFileName);
			//}
		}
		else
		{

			cv::Mat src = cv::Mat(pFrame->frameInfo.height, pFrame->frameInfo.width, CV_8UC3 , (uint8_t*)stPixelConvertParam.pDstBuf);
			srcImage = src.clone();

			printf("image convert to %s failed! ErrorCode[%d]\n", pConvertFormatStr, ret);
		}

		if (pDstBuf)
		{
			free(pDstBuf);
			pDstBuf = NULL;
		}

	}

	if (srcImage.empty())
	{
		srcImage = cv::Mat(5, 5, CV_8UC1).setTo(0);
	}
	Outmats.push_back(srcImage);
	if (currentUser->allowflag.load(std::memory_order::memory_order_acquire))
	{
		if (QString(currentUser->triggerType.str) =="Software")
		{
			currentUser->MatQueue.push(srcImage.clone());
		}
		else
		{
			currentUser->CallbackFuncVec.at(currentUser->Currentindex).GetimagescallbackFunc(currentUser, Outmats);
		}
		
	}
	int ret;
	IMV_ChunkDataInfo chunkDataInfo;
	unsigned int paramIndex = 0;
	for (inputIndex = 0; inputIndex < pFrame->frameInfo.chunkCount; inputIndex++)
	{
		ret = IMV_GetChunkDataByIndex(currentUser->devHandle, pFrame, inputIndex, &chunkDataInfo);
		if (IMV_OK != ret)
		{
			qWarning()<<("Get ChunkData failed! ErrorCode[%d]\n", ret);
			continue;
		}

		qDebug()<<("chunkID = %u\n", chunkDataInfo.chunkID);
		for (paramIndex = 0; paramIndex < chunkDataInfo.nParamCnt; paramIndex++)
		{
			qDebug()<<("paramName = %s\n", chunkDataInfo.pParamNameList[paramIndex].str);
		}
		
	}
	qDebug("Get frame blockId = %llu\n", pFrame->frameInfo.blockId);
	currentUser->Currentindex++;

	double time_End = (double)clock();
	//统计图片成像时间
	qDebug() << "getImage callback time" << time_End - time_Start<<"ms";
	return;
}


// 断线通知处理
// offLine notify processing
static void deviceOffLine(IMV_HANDLE devHandle)
{
	// 停止拉流 
	// Stop grabbing 
	IMV_StopGrabbing(devHandle);

	return;
}

// 上线通知处理
// onLine notify processing
static void deviceOnLine(IMV_HANDLE devHandle)
{
	int ret = IMV_OK;

	// 关闭相机
	// Close camera 
	IMV_Close(devHandle);

	do
	{

		ret = IMV_Open(devHandle);
		if (IMV_OK != ret)
		{
			printf("Retry open camera failed! ErrorCode[%d]\n", ret);
		}
		else
		{
			printf("Retry open camera successfully!\n");
			break;
		}

		Sleep(500);

	} while (true);

	// 重新设备连接状态事件回调函数
	// Device connection status event callback function again
	ret = IMV_SubscribeConnectArg(devHandle, onDeviceLinkNotify, devHandle);
	if (IMV_OK != ret)
	{
		printf("SubscribeConnectArg failed! ErrorCode[%d]\n", ret);
	}

	// 重新注册数据帧回调函数
	// Register data frame callback function again
	ret = IMV_AttachGrabbing(devHandle, onGetFrame, NULL);
	if (IMV_OK != ret)
	{
		printf("Attach grabbing failed! ErrorCode[%d]\n", ret);
	}

	// 开始拉流 
	// Start grabbing 
	ret = IMV_StartGrabbing(devHandle);
	if (IMV_OK != ret)
	{
		printf("Start grabbing failed! ErrorCode[%d]\n", ret);
	}
	else
	{
		printf("Start grabbing successfully\n");
	}

}

// 连接事件通知回调函数
// Connect event notify callback function
static void onDeviceLinkNotify(const IMV_SConnectArg* pConnectArg, void* pUser)
{
	int ret = IMV_OK;
	IMV_DeviceInfo devInfo;
	IMV_HANDLE devHandle = (IMV_HANDLE)pUser;

	if (NULL == devHandle)
	{
		printf("devHandle is NULL!");
		return;
	}

	memset(&devInfo, 0, sizeof(devInfo));
	ret = IMV_GetDeviceInfo(devHandle, &devInfo);
	if (IMV_OK != ret)
	{
		printf("Get device info failed! ErrorCode[%d]\n", ret);
		return;
	}

	// 断线通知
	// offLine notify 
	if (offLine == pConnectArg->event)
	{
		printf("------cameraKey[%s] : OffLine------\n", devInfo.cameraKey);
		deviceOffLine(devHandle);
	}
	// 上线通知
	// onLine notify 
	else if (onLine == pConnectArg->event)
	{
		printf("------cameraKey[%s] : OnLine------\n", devInfo.cameraKey);
		deviceOnLine(devHandle);
	}
}

static int setSoftTriggerConf(IMV_HANDLE devHandle)
{
	int ret = IMV_OK;

	// 设置触发源为软触发 
	// Set trigger source to Software 
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerSource", "Software");
	if (IMV_OK != ret)
	{
		printf("Set0 triggerSource value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发器 
	// Set trigger selector to FrameStart 
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerSelector", "FrameStart");
	if (IMV_OK != ret)
	{
		printf("Set1 triggerSelector value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发模式 
	// Set trigger mode to On 
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerMode", "On");
	if (IMV_OK != ret)
	{
		printf("Set2 triggerMode value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	return ret;
}
//硬触发
static int setLineTriggerConf(IMV_HANDLE devHandle)
{
	int ret = IMV_OK;

	// 设置触发源为外部触发 
	// Set trigger source to Line1 
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerSource", "Line1");
	if (IMV_OK != ret)
	{
		printf("Set triggerSource value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发器 
	// Set trigger selector to FrameStart 
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerSelector", "FrameStart");
	if (IMV_OK != ret)
	{
		printf("Set triggerSelector value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发模式 
	// Set trigger mode to On 
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerMode", "On");
	if (IMV_OK != ret)
	{
		printf("Set triggerMode value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置外触发为上升沿（下降沿为FallingEdge） 
	// Set trigger activation to RisingEdge(FallingEdge in opposite) 
	/*ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerActivation", "RisingEdge");
	if (IMV_OK != ret)
	{
		printf("Set triggerActivation value failed! ErrorCode[%d]\n", ret);
		return ret;
	}*/

	return ret;
}
//搜索所有设备
bool SearchDevice()
{
	int res = IMV_EnumDevices(&m_stDevList, interfaceTypeAll);//查找所有类型口的相机
	if (res != IMV_OK)
		return false;
	return true;
}

Hd_CameraModule_DaHua3::Hd_CameraModule_DaHua3(QString sn, QString path, int settype, QObject* parent)
	: PbGlobalObject(settype, parent), Sncode(sn), RootPath(path)
{
	//m_sdkFunc=std::make_shared<ThreadSafeQueue<std::vector<Mat>>>();

	famliy = CAMERA2D;
	JsonFilePath = RootPath + Sncode + ".json";
	//if (!QFile(JsonFilePath).exists())
	//    createAndWritefile(JsonFilePath, FirstCreateByte);
	//QJsonObject paramObj = load_JsonFile(JsonFilePath);
   /* for (auto objStr : paramObj.keys())
	{
		ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
	}*/
	CameraFunSDKfactoryCls* tempCls = new CameraFunSDKfactoryCls(sn, path, this);
	m_sdkFunc.reset(tempCls);
	connect(tempCls, &CameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });

}
//关闭设备
void CloseDevice(IMV_HANDLE devHandle)
{
	// 停止拉流 
	// Stop grabbing 
	int ret = IMV_StopGrabbing(devHandle);
	if (IMV_OK != ret)
	{
		printf("Stop grabbing failed! ErrorCode[%d]\n", ret);
		return;
	}

	// 关闭相机
	// Close camera 
	ret = IMV_Close(devHandle);
	if (IMV_OK != ret)
	{
		printf("Close camera failed! ErrorCode[%d]\n", ret);
		return;
	}
	if (devHandle != NULL)
	{
		// 销毁设备句柄
		// Destroy Device Handle
		IMV_DestroyHandle(devHandle);
	}
}
CameraFunSDKfactoryCls::CameraFunSDKfactoryCls(QString Sn, QString path, QObject* parent)
	: QObject(parent), SnCode(Sn.toStdString()), RootPath(path) {

}

CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
{
	CloseDevice(devHandle);
}

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{
	int ret = IMV_OK;
	ret = IMV_CreateHandle(&devHandle, modeByCameraKey, (void*)SnCode.c_str());//通过序列号创建设备句柄
	if (IMV_OK != ret)
	{
		qWarning() << ("Create devHandle failed! ErrorCode[%d]\n", ret);
		return false;
	}

	// 打开相机 
	// Open camera 
	ret = IMV_Open(devHandle);
	if (IMV_OK != ret)
	{
		qWarning() << ("Open camera failed! ErrorCode[%d]\n", ret);
		return false;
	}

	// 设备连接状态事件回调函数
		// Device connection status event callback function
	ret = IMV_SubscribeConnectArg(devHandle, onDeviceLinkNotify, this);
	if (IMV_OK != ret)
	{
		qWarning() << ("SubscribeConnectArg failed! ErrorCode[%d]\n", ret);
		return false;
	}
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerSource", "Software");
	if (IMV_OK != ret)
	{
		printf("Set0 triggerSource value failed! ErrorCode[%d]\n", ret);
		return ret;
	}
	ret = IMV_GetEnumFeatureSymbol(devHandle, "TriggerSource", &triggerType);//获取当前触发方式
	if (IMV_OK != ret)
	{
		qWarning() << ("IMV_GetEnumFeatureValue failed! ErrorCode[%d]\n", ret);
		return false;
	}
	qDebug() << "get TriggerSource" << triggerType.str;
	//打开触发模式
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerMode", "On");//0,off  ; 1 on
	if (IMV_OK != ret)
	{
		qWarning() << ("IMV_SetEnumFeatureValue failed! ErrorCode[%d]\n", ret);
		return false;
	}
	ret = IMV_SetEnumFeatureSymbol(devHandle, "AcquisitionMode", "Continuous");//0,连续  ; 1 单帧；2，多帧
	if (IMV_OK != ret)
	{
		qWarning() << ("IMV_SetEnumFeatureValue failed! ErrorCode[%d]\n", ret);
		return false;
	}
	ret = IMV_SetEnumFeatureSymbol(devHandle, "TriggerSelector", "FrameStart");//选择单帧模式/1,帧触发  0，采集开始触发
	if (IMV_OK != ret)
	{
		qWarning() << ("Set1 triggerSelector value failed! ErrorCode[%d]\n", ret);
		return ret;
	}
	//ret = IMV_SetIntFeatureValue(devHandle, "HeartbeatTimeout", 3000);//心跳日志 单词可能错误
	//if (IMV_OK != ret)
	//{
	//    qWarning() << ("set HeartbeatTimeout failed!![%d]\n", ret);
	//    //return -1;
	//}
	// 注册数据帧回调函数
	// Register data frame callback function
	ret = IMV_AttachGrabbing(devHandle, onGetFrame, this);
	if (IMV_OK != ret)
	{
		qWarning() << ("Attach grabbing failed! ErrorCode[%d]\n", ret);
		return false;

	}

	// 开始拉流 
	// Start grabbing 
	ret = IMV_StartGrabbing(devHandle);
	if (IMV_OK != ret)
	{
		qWarning() << ("Start grabbing failed! ErrorCode[%d]\n", ret);
		return false;

	}
	if (IMV_OK == ret)
	{
		return true;
	}
	return false;
}
Hd_CameraModule_DaHua3::~Hd_CameraModule_DaHua3()
{

}

//传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
//初始化
bool Hd_CameraModule_DaHua3::init()
{
	connect(this, &PbGlobalObject::trigged, [=](int Code) {
		if (Code == 1000)
		{
			m_sdkFunc->Currentindex = 0;
			m_sdkFunc->MatQueue.clear();
			m_sdkFunc->allowflag.store(true, std::memory_order::memory_order_release);
		}
		else if (Code == 1001)
		{
			m_sdkFunc->allowflag.store(false, std::memory_order::memory_order_release);
		}
		});
	bool flag = m_sdkFunc->initSdk(ParasValueMap);
	/*if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
		type1 = 1;
	else if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE < 4)
		type1 = 0;*/
	qDebug() << m_sdkFunc->devHandle;
	return flag;
}

bool Hd_CameraModule_DaHua3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
	Q_UNUSED(mats);
	if (mats.empty() && data.isEmpty())
	{
		int ret = IMV_ExecuteCommandFeature(m_sdkFunc->devHandle, "TriggerSoftware");
		if (IMV_OK != ret)
		{
			qWarning() << ("Execute TriggerSoftware failed! ErrorCode[%d]\n", ret);
			return  false;
		}
		emit trigged(501);
		return true;
	}
	return true;
}
//获取数据
bool Hd_CameraModule_DaHua3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
	cv::Mat image;
	m_sdkFunc->MatQueue.wait_for_pop(10000, image);
	if (image.empty())
	{
		ImgS.push_back(cv::Mat::zeros(100, 100, 0));
		qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
		return false;
	}
	ImgS.push_back(std::move(image));
	return true;
}

QMap<QString, QString> Hd_CameraModule_DaHua3::parameters()
{
	return ParasValueMap;
}
//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_DaHua3::setParameter(const QMap<QString, QString>& ParameterMap)
{
	ParasValueMap = ParasValueMap;

	m_sdkFunc->upDateParam();
	return true;
}
void Hd_CameraModule_DaHua3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent, const QString& getString)
{
	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;
	TempPack.GetimagescallbackFunc = func;
	m_sdkFunc->CallbackFuncVec.append(TempPack);
	qDebug() << getString;
}

void Hd_CameraModule_DaHua3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
	int index = getString.toInt();
	if (callBackFun == m_sdkFunc->CallbackFuncVec.at(index).GetimagescallbackFunc)
	{
		qDebug() << index;
		m_sdkFunc->CallbackFuncVec.removeAt(index);
	}
	else
	{
		int size = m_sdkFunc->CallbackFuncVec.size();
		for (int i = 0; i < size; i++)
		{
			if (m_sdkFunc->CallbackFuncVec.at(i).GetimagescallbackFunc == callBackFun)
			{
				m_sdkFunc->CallbackFuncVec.removeAt(i);
				qDebug() << i;
				return;
			}
		}
	}
	return;
}


bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
	if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
		return false;
	OnePb temp;
	temp.base = new Hd_CameraModule_DaHua3(DeviceSn, path + "/Hd_CameraModule_HIK3/");
	if (!temp.base->init())
		return false;
	temp.baseWidget = new mPrivateWidget(temp.base);
	temp.DeviceSn = DeviceSn;
	qDebug() << name.indexOf(':');
	if (name.split(':').size() > 2)
		TotalMap.insert(name.mid(0, name.lastIndexOf(':')), temp);
	else
	{
		TotalMap.insert(name.split(':').first(), temp);
	}
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
	IMV_DeviceInfo* pDevInfo = nullptr;
	for (int i = 0; i < m_stDevList.nDevNum; i++)
	{
		pDevInfo = &m_stDevList.pDevInfo[i];
		char* Cameraname = pDevInfo->cameraKey;// pDevInfo[i]
		//char* Sername = pDevInfo->serialNumber;// pDevInfo[i]
		string userName = static_cast<string>((LPCSTR)Cameraname);
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



mPrivateWidget::mPrivateWidget(void* handle)
{
	m_Camerahandle = reinterpret_cast<Hd_CameraModule_DaHua3*>(handle);
	InitWidget();
}

void mPrivateWidget::InitWidget()
{
	QVBoxLayout* MainLayout = new QVBoxLayout(this);

	SetDataBtn = new QPushButton(this);
	m_showimage = new ImageViewer(this);
	MainLayout->addWidget(m_showimage);
	MainLayout->addWidget(SetDataBtn);

	connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;  QStringList list;
		emit m_Camerahandle->trigged(1000);
		m_Camerahandle->setData(mats, list);
		m_Camerahandle->data(mats, list);
		cv::Mat tempMat = mats.at(0);
		m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
		});

}