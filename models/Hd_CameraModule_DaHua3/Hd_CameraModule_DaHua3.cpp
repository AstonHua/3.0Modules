#include "Hd_CameraModule_DaHua3.h"
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QTextCodec>
#include <QDateTime>
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
void SetExposureTime(void* handle, float exposureValue)
{
	qDebug() << "[INFO] " << "exposureValue:" << exposureValue;
	IMV_SetEnumFeatureValue(handle, "ExposureAuto", 0);
	IMV_SetDoubleFeatureValue(handle, "ExposureTime", exposureValue);
}
//设置增益
void SetGain(void* handle, float GainValue)
{
	qDebug() << "[INFO] " << "GainValue:" << GainValue;
	IMV_SetEnumFeatureValue(handle, "GainAuto", 0);
	IMV_SetDoubleFeatureValue(handle, "GainRaw", GainValue);
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
	QList<cv::Mat> Outmats;
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

		ret = IMV_PixelConvert(currentUser->handle, &stPixelConvertParam);
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
    if (currentUser->triggerMode.load(std::memory_order::memory_order_acquire) == 0)
    {
        currentUser->triggerOffBack(srcImage);
        return;
    }
	//if (currentUser->allowflag.load(std::memory_order::memory_order_acquire))
	{
        Outmats.push_back(srcImage.clone());
		if (currentUser->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
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
			//currentUser->CallbackFuncVec.at(currentUser->Currentindex).GetimagescallbackFunc(currentUser, Outmats);
		}
		
	}
	int ret;
	IMV_ChunkDataInfo chunkDataInfo;
	unsigned int paramIndex = 0;
	for (inputIndex = 0; inputIndex < pFrame->frameInfo.chunkCount; inputIndex++)
	{
		ret = IMV_GetChunkDataByIndex(currentUser->handle, pFrame, inputIndex, &chunkDataInfo);
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
    if (currentUser->exposureTimeMap.count(currentUser->Currentindex) == 1)
    {
        IMV_SetDoubleFeatureValue(currentUser->handle, "ExposureTime", currentUser->exposureTimeMap[currentUser->Currentindex]);
    }
    if (currentUser->gainMap.count(currentUser->Currentindex) == 1)
    {
        IMV_SetDoubleFeatureValue(currentUser->handle, "Gain", currentUser->gainMap[currentUser->Currentindex]);
    }
    if (currentUser->gammaMap.count(currentUser->Currentindex) == 1)
    {
        IMV_SetDoubleFeatureValue(currentUser->handle, "Gamma", currentUser->gammaMap[currentUser->Currentindex]);
    }
    if (currentUser->OnceGetImageNum > 0 && currentUser->Currentindex >= currentUser->getImageMaxCoiunts / currentUser->OnceGetImageNum)	currentUser->Currentindex = 0;
	double time_End = (double)clock();
	//统计图片成像时间
	qDebug() << "getImage callback time" << time_End - time_Start<<"ms";


	return;
}

// 断线通知处理
// offLine notify processing
static void deviceOffLine(IMV_HANDLE handle)
{
	// 停止拉流 
	// Stop grabbing 
	IMV_StopGrabbing(handle);

	return;
}

// 上线通知处理
// onLine notify processing
static void deviceOnLine(IMV_HANDLE handle)
{
	int ret = IMV_OK;

	// 关闭相机
	// Close camera 
	IMV_Close(handle);

	do
	{

		ret = IMV_Open(handle);
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
	ret = IMV_SubscribeConnectArg(handle, onDeviceLinkNotify, handle);
	if (IMV_OK != ret)
	{
		printf("SubscribeConnectArg failed! ErrorCode[%d]\n", ret);
	}

	// 重新注册数据帧回调函数
	// Register data frame callback function again
	ret = IMV_AttachGrabbing(handle, onGetFrame, NULL);
	if (IMV_OK != ret)
	{
		printf("Attach grabbing failed! ErrorCode[%d]\n", ret);
	}

	// 开始拉流 
	// Start grabbing
	ret = IMV_StartGrabbing(handle);
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
	IMV_HANDLE handle = (IMV_HANDLE)pUser;

	if (NULL == handle)
	{
		printf("handle is NULL!");
		return;
	}

	memset(&devInfo, 0, sizeof(devInfo));
	ret = IMV_GetDeviceInfo(handle, &devInfo);
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
		deviceOffLine(handle);
	}
	// 上线通知
	// onLine notify 
	else if (onLine == pConnectArg->event)
	{
		printf("------cameraKey[%s] : OnLine------\n", devInfo.cameraKey);
		deviceOnLine(handle);
	}
}

static int setSoftTriggerConf(IMV_HANDLE handle)
{
	int ret = IMV_OK;

	// 设置触发源为软触发 
	// Set trigger source to Software 
	ret = IMV_SetEnumFeatureSymbol(handle, "TriggerSource", "Software");
	if (IMV_OK != ret)
	{
		printf("Set0 triggerSource value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发器 
	// Set trigger selector to FrameStart 
	ret = IMV_SetEnumFeatureSymbol(handle, "TriggerSelector", "FrameStart");
	if (IMV_OK != ret)
	{
		printf("Set1 triggerSelector value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发模式 
	// Set trigger mode to On 
	ret = IMV_SetEnumFeatureSymbol(handle, "TriggerMode", "On");
	if (IMV_OK != ret)
	{
		printf("Set2 triggerMode value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	return ret;
}
//硬触发
static int setLineTriggerConf(IMV_HANDLE handle)
{
	int ret = IMV_OK;

	// 设置触发源为外部触发 
	// Set trigger source to Line1 
	ret = IMV_SetEnumFeatureSymbol(handle, "TriggerSource", "Line1");
	if (IMV_OK != ret)
	{
		printf("Set triggerSource value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发器 
	// Set trigger selector to FrameStart 
	ret = IMV_SetEnumFeatureSymbol(handle, "TriggerSelector", "FrameStart");
	if (IMV_OK != ret)
	{
		printf("Set triggerSelector value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置触发模式 
	// Set trigger mode to On 
	ret = IMV_SetEnumFeatureSymbol(handle, "TriggerMode", "On");
	if (IMV_OK != ret)
	{
		printf("Set triggerMode value failed! ErrorCode[%d]\n", ret);
		return ret;
	}

	// 设置外触发为上升沿（下降沿为FallingEdge） 
	// Set trigger activation to RisingEdge(FallingEdge in opposite) 
	/*ret = IMV_SetEnumFeatureSymbol(handle, "TriggerActivation", "RisingEdge");
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
//关闭设备
void CloseDevice(IMV_HANDLE handle)
{
    IMV_StopGrabbing(handle);
    IMV_Close(handle);
	if (handle != NULL)
	{
		IMV_DestroyHandle(handle);
	}
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

CameraFunSDKfactoryCls::CameraFunSDKfactoryCls(QString Sn, QString path, QObject* parent)
	: QObject(parent), SnCode(Sn.toStdString()), RootPath(path) {

}

CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
{
	CloseDevice(handle);
}

int CameraFunSDKfactoryCls::FindCameraIndexBySN(const std::string& targetSN)
{
    IMV_DeviceList devList;
    // 枚举所有可用相机
    int error = IMV_EnumDevices(&devList,interfaceTypeGige);
    if (error != IMV_OK)
    {
        return -1;
    }

    // 遍历查找匹配的SN
    for (int i = 0; i < devList.nDevNum; ++i)
    {
        std::string devSN = devList.pDevInfo[i].serialNumber;
        if (devSN == targetSN)
        {
            return i;
        }
    }
    return -1;
}

bool CameraFunSDKfactoryCls:: initSdk(QMap<QString, QString>& insideValuesMaps)
{
	int ret = IMV_OK;
    //string Machine = "Huaray Technology:";
    //string MachineSnCode = Machine+ SnCode;
    int devIndex = FindCameraIndexBySN(SnCode);
    if (devIndex < 0)
    {
        return false;
    }
	//ret = IMV_CreateHandle(&handle, modeByCameraKey, (void*)MachineSnCode.c_str());//通过序列号创建设备句柄
    ret = IMV_CreateHandle(&handle, modeByIndex, &devIndex);
	if (IMV_OK != ret)
	{
		qWarning() << ("Create handle failed! ErrorCode[%d]\n", ret);
		return false;
	}

	// 打开相机 
	// Open camera 
	ret = IMV_Open(handle);
	if (IMV_OK != ret)
	{
		qWarning() << ("Open camera failed! ErrorCode[%d]\n", ret);
		return false;
	}
	// 设备连接状态事件回调函数
		// Device connection status event callback function
	ret = IMV_SubscribeConnectArg(handle, onDeviceLinkNotify, this);
	if (IMV_OK != ret)
	{
		qWarning() << ("SubscribeConnectArg failed! ErrorCode[%d]\n", ret);
		return false;
	}
    //IMV_SetEnumFeatureValue(handle, "TriggerMode", MV_TRIGGER_MODE_ON);
    IMV_SetEnumFeatureSymbol(handle, "TriggerMode","On");
    ret = IMV_ExecuteCommandFeature(handle, "UserSetLoad");
    if (IMV_OK != ret)
    {
        qWarning() << ("SubscribeConnectArg failed! ErrorCode[%d]\n", ret);
        return false;
    }

    MVCC_DOUBLEVALUE stEnumValue = { 0 };
    IMV_GetDoubleFeatureValue(handle, "TriggerSource", &stEnumValue.fCurValue);
    m_MV_CAM_TRIGGER_SOURCE = (MV_CAM_TRIGGER_SOURCE)stEnumValue.fCurValue;

	ret = IMV_AttachGrabbing(handle, onGetFrame, this);
	if (IMV_OK != ret)
	{
		qWarning() << ("Attach grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

	// 开始拉流 
	// Start grabbing 
	ret = IMV_StartGrabbing(handle);
	if (IMV_OK != ret)
	{
		qWarning() << ("Start grabbing failed! ErrorCode[%d]\n", ret);
		return false;
	}

    emit trigged(1);
    return true;

}

void CameraFunSDKfactoryCls::upDateParam()
{
    getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts").toInt();
    OnceGetImageNum = qMax(ParasValueMap.value("OnceImageCounts").toInt(), 1);
    timeOut = ParasValueMap.value("GetOnceImageTimes").toInt();

    qDebug() << getImageMaxCoiunts << OnceGetImageNum;
    return;
}

void CameraFunSDKfactoryCls::syncParamCycleList()
{
    paramCycleList.clear();
    // 收集三个map中所有的ID
    QSet<int> allIds;
    for (auto& p : exposureTimeMap) allIds.insert(p.first);
    for (auto& p : gainMap)           allIds.insert(p.first);
    for (auto& p : gammaMap)          allIds.insert(p.first);
    QList<int> sortedIds = allIds.toList();
    std::sort(sortedIds.begin(), sortedIds.end());
    for (int id : sortedIds)
    {
        ParamItem item;
        if (exposureTimeMap.count(id)) item.exposure = exposureTimeMap[id];
        if (gainMap.count(id))           item.gain = gainMap[id];
        if (gammaMap.count(id))          item.gamma = gammaMap[id];
        paramCycleList.append(item);
    }
}

Hd_CameraModule_DaHua3::~Hd_CameraModule_DaHua3()
{
    if (m_sdkFunc)
    {
        delete m_sdkFunc;
    }
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
	else if (m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE <= 4)
		type1 = 0;
    QJsonObject paramObj = load_JsonFile(JsonFilePath);
    if (paramObj.contains("ExposureTime"))
        m_sdkFunc->setArrayByte("ExposureTime", paramObj.value("ExposureTime").toArray());
    if (paramObj.contains("Gain"))
        m_sdkFunc->setArrayByte("Gain", paramObj.value("Gain").toArray());
    if (paramObj.contains("Gamma"))
        m_sdkFunc->setArrayByte("Gamma", paramObj.value("Gamma").toArray());
	qDebug() << m_sdkFunc->handle;
	return flag;
}

bool Hd_CameraModule_DaHua3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
	Q_UNUSED(mats);
	if (mats.empty() && data.isEmpty())
	{
        m_sdkFunc->syncParamCycleList();
        if (!m_sdkFunc->paramCycleList.isEmpty())
        {
            int idx = m_sdkFunc->setDataCycleIndex % m_sdkFunc->paramCycleList.size();
            const auto& item = m_sdkFunc->paramCycleList[idx];
            SetGain(m_sdkFunc->handle, item.gain);
            SetExposureTime(m_sdkFunc->handle, item.exposure);
            qWarning() << "currIndex" << idx << "Exposure:" << item.exposure << "Gain:" << item.gain << "Gamma:" << item.gamma;
            m_sdkFunc->setDataCycleIndex = (idx + 1) % m_sdkFunc->paramCycleList.size();  
        }
        int ret = IMV_ExecuteCommandFeature(m_sdkFunc->handle, "TriggerSoftware");
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
    QList<cv::Mat> Outmats;
    m_sdkFunc->MatQueue.wait_for_pop(m_sdkFunc->timeOut, Outmats);
    ImgS = Outmats.toVector().toStdVector();
    if (ImgS.empty())
    {
        ImgS.push_back(cv::Mat::zeros(100, 100, 0));
        qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
        return false;
    }
    return true;
}

QMap<QString, QString> Hd_CameraModule_DaHua3::parameters()
{
	return ParasValueMap;
}
//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_DaHua3::setParameter(const QMap<QString, QString>& ParameterMap)
{

    ParasValueMap = ParameterMap;
    m_sdkFunc->ParasValueMap = ParasValueMap;

    m_sdkFunc->upDateParam();
	return true;
}

void Hd_CameraModule_DaHua3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent, const QString& getString)
{
	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;
	TempPack.GetimagescallbackFunc = func;
    m_sdkFunc->CallbackFuncMap.insert(getString.toInt(), TempPack);
	qDebug() << getString;
}

void Hd_CameraModule_DaHua3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
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
    temp.base = new Hd_CameraModule_DaHua3(DeviceSn, path + "/Hd_CameraModule_DaHua3/");
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
    if (TotalMap.value(name).baseWidget) {

        return TotalMap.value(name).baseWidget;
    }
	return nullptr;
}

PbGlobalObject* getCameraPtr(const QString& name)
{
    if (TotalMap.value(name).base) {
        return TotalMap.value(name).base;
    }
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
		char* Cameraname = pDevInfo->serialNumber;// pDevInfo[i]
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
    connect(this, &mPrivateWidget::sendImage, this, [=](QImage img) {
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
    auto createPushButton = [this](const QString& text, int width = 50, int height = 30) -> QPushButton* {
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
    auto createScrollArea = [this]()->QScrollArea* {
        QScrollArea* area = new QScrollArea(this);
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
    BytePtr = QJsonDocument::fromJson(byte).object();

    this->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* mainHboxLayout = new QHBoxLayout(this);
    QVBoxLayout* MainLayout = new QVBoxLayout;
    QGridLayout* girlayout = new QGridLayout();
    QSplitter* Splitter = new QSplitter(Qt::Horizontal, this);
    QVBoxLayout* AlgParmLayout = new QVBoxLayout();
    //相机参数控件创建
    {
        QLabel* cameraTitle = createLabel(tr("相机参数设置"));
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
        if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
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
        BalanceWhiteAuto = createComboBox(QStringList() << tr("关闭") << tr("一次") << tr("连续"));
        BalanceRatioR = createSpinBox();
        BalanceRatioG = createSpinBox();
        BalanceRatioB = createSpinBox();

        triggerModel->hide();
        first->hide();
        //相机参数控件初值设置
        MVCC_DOUBLEVALUE stFloatValue = { 0 };
        IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", &stFloatValue.fCurValue);
        IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", &stFloatValue.fMin);
        IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "ExposureTime", &stFloatValue.fMax); 
        ExposureMin = stFloatValue.fMin;
        ExposureMax = stFloatValue.fMax;
        float ExposureCur = stFloatValue.fCurValue;
        IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "Gain", &stFloatValue.fCurValue);
        IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "Gain", &stFloatValue.fMin);
        IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "Gain", &stFloatValue.fMax);
        gainMin = stFloatValue.fMin;
        gainMax = stFloatValue.fMax;
        float gainCur = stFloatValue.fCurValue;
        IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "Gamma", &stFloatValue.fCurValue);
        IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "Gamma", &stFloatValue.fMin);
        IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "Gamma", &stFloatValue.fMax);
        GamaMin = stFloatValue.fMin;
        GamaMax = stFloatValue.fMax;
        float GamaCur = stFloatValue.fCurValue;

        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_OFF);
        MVCC_DOUBLEVALUE stIntValue = { 0 };
        //red
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 0);
        IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fCurValue);
        IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMin);
        IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMax);
        BalanceRatioR->setMaximum(stIntValue.fMax);
        BalanceRatioR->setMinimum(stIntValue.fMin);
        BalanceRatioR->setValue(stIntValue.fCurValue);
        //green
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 1);
        IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fCurValue);
        IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMin);
        IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMax);
        BalanceRatioG->setMaximum(stIntValue.fMax);
        BalanceRatioG->setMinimum(stIntValue.fMin);
        BalanceRatioG->setValue(stIntValue.fCurValue);
        //blue
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 2);
        IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fCurValue);
        IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMin);
        IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMax);
        BalanceRatioB->setMaximum(stIntValue.fMax);
        BalanceRatioB->setMinimum(stIntValue.fMin);
        BalanceRatioB->setValue(stIntValue.fCurValue);

        doubleValidator1 = createDoubleValidator(ExposureMin, ExposureMax);
        doubleValidator2 = createDoubleValidator(gainMin, gainMax);
        doubleValidator3 = createDoubleValidator(0, 4);
        Exposure->setValidator(doubleValidator1);
        Exposure->setText(QString::number(ExposureCur, 'f', 4));
        
        gain->setValidator(doubleValidator2);
        gain->setText(QString::number(gainCur, 'f', 4));
        
        Gama->setValidator(doubleValidator3);
        Gama->setText(QString::number(GamaCur, 'f', 4));

        QHBoxLayout* hbox = new QHBoxLayout();
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
        girlayout->addWidget(GammaDisableL, 3, 0);
        girlayout->addWidget(GamaDisable, 3, 1);
        girlayout->addWidget(GamaL, 4, 0);
        girlayout->addWidget(Gama, 4, 1);
        girlayout->addWidget(ExposureL, 5, 0);
        girlayout->addWidget(Exposure, 5, 1);
        girlayout->addWidget(BalanceWhiteAutoL, 6, 0);
        girlayout->addWidget(BalanceWhiteAuto, 6, 1);
        girlayout->addWidget(BalanceRatioL, 7, 0);
        girlayout->addLayout(hbox, 7, 1);
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

    //流程控件创建
    {
        QLabel* CountL = createLabel(tr("一次信号取图次数"));
        QLabel* TimeOutL = createLabel(tr("单张图超时时间"));
        QLabel* liucTitle = createLabel(tr("流程参数设置"));
        liucTitle->setObjectName("titleLabel1");
        Count = createSpinBox();
        timeout = createSpinBox();
        QStackedWidget* changeWidget = new QStackedWidget(this);

        saveBtn = new QPushButton(tr("保存"), this);
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
        takeEffect = createPushButton(tr("设定生效"), 110);

        QLabel* tips = createLabel(tr("注意:设定生效按钮点击时,当前界面设定生效,请在未生产时操作"));
        tips->setStyleSheet("color: rgb(255, 0, 0);");

        QStringList headers;
        headers << tr("id") << tr("Exposure") << tr("Gain") << tr("Gamma");
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
        girlayout_param->addWidget(TimeOutL, 0, 0);
        girlayout_param->addWidget(timeout, 0, 1);
        girlayout_param->addWidget(CountL, 1, 0);
        girlayout_param->addWidget(Count, 1, 1);
        girlayout_param->setColumnStretch(0, 1);  // 第一列拉伸因子为1
        girlayout_param->setColumnStretch(1, 2);  // 第二列拉伸因子为2
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

    //参数json显示界面（隐藏）
    {
        m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json");
        m_AlgParmWidget->hide();
    }

    //图片显示界面控件创建
    {
        SetDataBtn = createPushButton(tr("软触发"), 100);
        ContinuesBtn = createPushButton(tr("连续取图"), 100);
        Details = createPushButton(tr("详情"), 100);
        Details->hide();
        m_showimage = new viewWidget();
        m_showimage->setMinimumSize(500, 500);
        QLabel* title = createLabel("海康相机");

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
    algParmLayoutWidget->setContentsMargins(0, 0, 0, 0);

    Splitter->addWidget(mainLayoutWidget);
    Splitter->addWidget(algParmLayoutWidget);
    Splitter->addWidget(m_AlgParmWidget);
    QList<int> ratios;
    ratios << 4 << 3 << 0; // 三个子项的比例
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

    Splitter->setContentsMargins(0, 0, 0, 0);
    mainHboxLayout->addWidget(Splitter);
    mainHboxLayout->setContentsMargins(0, 0, 0, 0);
    createConnect();
    gainTable->initData();
    GamaTable->initData();
    ExposureTimeTable->initData();
}

void mPrivateWidget::createConnect()
{
    connect(first, &QComboBox::currentTextChanged, this, [=](QString text) {
        int mode = (text == tr("打开")) ? MV_TRIGGER_MODE_ON : MV_TRIGGER_MODE_OFF;
        int flag = (text == tr("打开")) ? 1 : 0;
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "TriggerMode", mode);
        m_Camerahandle->trigged(flag);
        });
    connect(Second, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        int source = MV_TRIGGER_SOURCE_LINE0;
        if (text == "Line1") source = MV_TRIGGER_SOURCE_LINE1;
        else if (text == "Line2") source = MV_TRIGGER_SOURCE_LINE2;
        else if (text == "Line3") source = MV_TRIGGER_SOURCE_LINE3;
        else if (text == tr("软触发")) source = MV_TRIGGER_SOURCE_SOFTWARE;
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "TriggerSource", source);
        m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE = (MV_CAM_TRIGGER_SOURCE)source;
        if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE == MV_TRIGGER_SOURCE_SOFTWARE)
            m_Camerahandle->type1 = 1;
        else if (m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE < 4)
            m_Camerahandle->type1 = 0;
        qDebug() << "MV_CAM_TRIGGER_SOURCE" << m_Camerahandle->m_sdkFunc->m_MV_CAM_TRIGGER_SOURCE << "\t" << m_Camerahandle->type1;
        });

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

    connect(gain, &QLineEdit::editingFinished, this, [=]() {
        SetGain(m_Camerahandle->m_sdkFunc->handle,float(gain->text().toFloat()));
        });
    connect(Gama, &QLineEdit::editingFinished, this, [=]() {
        IMV_SetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "Gamma", float(Gama->text().toFloat()));
        });
    connect(Exposure, &QLineEdit::editingFinished, this, [=]() {
        SetExposureTime(m_Camerahandle->m_sdkFunc->handle,float(Exposure->text().toFloat()));
        });

    connect(BalanceWhiteAuto, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        int nRet = 0;
        if (text == tr("关闭"))
        {
            BalanceRatioR->setEnabled(true);
            BalanceRatioG->setEnabled(true);
            BalanceRatioB->setEnabled(true);
            BalanceRatioR->blockSignals(true);
            BalanceRatioG->blockSignals(true);
            BalanceRatioB->blockSignals(true);
            nRet = IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_OFF);
            if (nRet == 0)
            {
                qDebug() << "设置BalanceWhiteAuto";
                MVCC_DOUBLEVALUE stIntValue = { 0 };
                IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 0);
                IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fCurValue);
                IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMin);
                IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMax);
                qDebug() << "Red" << stIntValue.fCurValue << stIntValue.fMax << stIntValue.fMin;
                BalanceRatioR->setMaximum(stIntValue.fMax);
                BalanceRatioR->setMinimum(stIntValue.fMin);
                BalanceRatioR->setValue(stIntValue.fCurValue);

                IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 1);
                IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fCurValue);
                IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMin);
                IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMax);
                qDebug() << "Green" << stIntValue.fCurValue << stIntValue.fMax << stIntValue.fMin;
                BalanceRatioR->setMaximum(stIntValue.fMax);
                BalanceRatioR->setMinimum(stIntValue.fMin);
                BalanceRatioR->setValue(stIntValue.fCurValue);

                IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 2);
                IMV_GetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fCurValue);
                IMV_GetDoubleFeatureMin(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMin);
                IMV_GetDoubleFeatureMax(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", &stIntValue.fMax);
                qDebug() << "Blue" << stIntValue.fCurValue << stIntValue.fMax << stIntValue.fMin;
                BalanceRatioR->setMaximum(stIntValue.fMax);
                BalanceRatioR->setMinimum(stIntValue.fMin);
                BalanceRatioR->setValue(stIntValue.fCurValue);
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
            nRet = IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_ONCE);
        }
        else
        {
            BalanceRatioR->setEnabled(false);
            BalanceRatioG->setEnabled(false);
            BalanceRatioB->setEnabled(false);
            nRet = IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceWhiteAuto", MV_BALANCEWHITE_AUTO_CONTINUOUS);
        }
        });
    connect(BalanceRatioR, &QSpinBox::editingFinished, this, [=]() {
        int value = BalanceRatioR->value();
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 0);
        IMV_SetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", value);
        });
    connect(BalanceRatioG, &QSpinBox::editingFinished, this, [=]() {
        int value = BalanceRatioG->value();
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 1);
        IMV_SetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", value);
        });
    connect(BalanceRatioB, &QSpinBox::editingFinished, this, [=]() {
        int value = BalanceRatioB->value();
        IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatioSelector", 2);
        IMV_SetDoubleFeatureValue(m_Camerahandle->m_sdkFunc->handle, "BalanceRatio", value);
        });

    connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
        std::vector<cv::Mat> mats;
        QStringList list;
        emit m_Camerahandle->trigged(1000);
        m_Camerahandle->setData(mats, list);
        m_Camerahandle->data(mats, list);
        cv::Mat tempMat = mats.at(0);
        QImage showImg = cvMatToQImage(tempMat);
        m_showimage->reciveImage("", showImg);
        });
    connect(Details, &QPushButton::clicked, this, [=]() {
        //

        //
        });

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
            else if (paramObj.value(objStr).isObject())
            {

            }
        }
        m_Camerahandle->setParameter(ParameterMap);
        });

    connect(GamaDisable, &QComboBox::currentTextChanged, this, [=](const QString& text) {
        int nRet = 0;
        if (text == tr("打开")) {
            nRet = IMV_SetEnumFeatureValue(m_Camerahandle->m_sdkFunc->handle, "GammaSelector", MV_GAMMA_SELECTOR_USER);
            if (nRet != 0) {
                qDebug() << "设置伽马选择器失败，错误码：" << nRet;
                return;
            }
            nRet = IMV_SetBoolFeatureValue(m_Camerahandle->m_sdkFunc->handle, "GammaEnable", true);
        }
        else {
            nRet = IMV_SetBoolFeatureValue(m_Camerahandle->m_sdkFunc->handle, "GammaEnable", false);
        }
        qDebug() << (nRet == 0 ? (text == tr("打开") ? tr("伽马功能已打开") : tr("伽马功能已关闭"))
            : tr("操作失败，错误码：") + QString::number(nRet));
        });

    connect(Count, &QSpinBox::editingFinished, this, [=]() {
        int currentValue = Count->value();
        m_Camerahandle->m_sdkFunc->getImageMaxCoiunts = currentValue;
        m_Camerahandle->m_sdkFunc->ParasValueMap["OnceSignalsGetImageCounts"] = QString::number(currentValue);
        //设置回去
        BytePtr["OnceSignalsGetImageCounts"] = QString::number(currentValue);
        m_AlgParmWidget->reLoadByte(QJsonDocument(BytePtr).toJson());
        });

    connect(timeout, &QSpinBox::editingFinished, this, [=]() {
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
    connect(saveBtn, &QPushButton::clicked, this, [=]() {
        if (m_AlgParmWidget)
            emit m_AlgParmWidget->save();
        });

    connect(takeEffect, &QPushButton::clicked, this, [=]() {
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
    connect(Add, &QPushButton::clicked, this, [=]() {
        auto createCenteredItem = [](const QString& text) -> QTableWidgetItem* {
            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // 水平+垂直居中
            return item;
            };
        int row = showTable->rowCount();
        showTable->setRowCount(row + 1);
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

    connect(Delete, &QPushButton::clicked, this, [=]() {
        QItemSelectionModel* selectionModel = showTable->selectionModel();
        QSet<int> selectedRows;
        QModelIndexList selectedIndexes = selectionModel->selectedIndexes();
        foreach(QModelIndex index, selectedIndexes) {
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
        if (col != 0)
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
                    .arg(col == 1 ? ExposureMin : (col == 2 ? gainMin : GamaMin))
                    .arg(col == 1 ? ExposureMax : (col == 2 ? gainMax : GamaMax)));
                item->setText(originalValue);
                item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                return;
            }
            cellOriginalValues[QPair<int, int>(row, col)] = text;
            if (col == 1)//曝光时间表格修改
            {
                ExposureTimeTable->setItemData(row, 1, text);
            }
            else if (col == 2)//增益修改
            {
                gainTable->setItemData(row, 1, text);
            }
            else if (col == 3) //伽马校正
            {
                GamaTable->setItemData(row, 1, text);
            }
        }
        else//图片列表只需要判断是不是int，并且同步更改三个列表
        {
            bool isInt = false;
            text.toInt(&isInt);
            if (!isInt && !text.isEmpty())
            {
                QString originalValue = cellOriginalValues.value(QPair<int, int>(row, col), "");
                DMessageBox::warning(this, tr("输入非法"), tr("请输入整数"));
                item->setText(originalValue);
                return;
            }
            //修改三个表格的第一行
            ExposureTimeTable->setItemData(row, 0, text);
            gainTable->setItemData(row, 0, text);
            GamaTable->setItemData(row, 0, text);
        }
        });

    connect(ExposureTimeTable, &MyTableWidget::addNewLine, this, [=](int row, int col, QString value) {
        showTable->blockSignals(true);
        qDebug() << "ExposureTimeTable:" << row << col << value;
        int rowCount = showTable->rowCount();
        if (rowCount < row + 1) showTable->setRowCount(row + 1);
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        showTable->setItem(row, col, item);
        showTable->blockSignals(false);
        });
    connect(gainTable, &MyTableWidget::addNewLine, this, [=](int row, int col, QString value) {
        showTable->blockSignals(true);
        qDebug() << "gainTable:" << row << col << value;
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
        qDebug() << "GamaTable:" << row << col << value;
        int rowCount = showTable->rowCount();
        if (rowCount < row + 1) showTable->setRowCount(row + 1);
        QTableWidgetItem* item = new QTableWidgetItem(value);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        if (col != 0)
            showTable->setItem(row, col + 2, item);
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
