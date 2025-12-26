#include "Hd_CameraModule_3DLMI3.h"
#include <QDebug>
#include <QQueue>
#include <QTextCodec>
#include <qfuture.h>
#include <QtConcurrent/qtconcurrent>
const QByteArray FirstCreateByte(R"({"DeviceId": "0",
  "GetOnceImageTimes": "5000",
  "Ip": "192.168.0.1",
  "Port": "24691",
  "xImageSize": "3200",
  "yImageSize": "1000",
  "y_pitch_um": "20.0",
  "OnceSignalsGetImageCounts": "20",
"OnceImageCounts":"2",
"triggedType":"1"})");

#define SYSTEMGO GoSystemOnceExplem::getInstance()->getsystem1()//
#define SYSTEMAPI GoSystemOnceExplem::getInstance()->getkAssembly()//初始化SDKapi
QMutex GoSystemOnceExplem::mutex;
QScopedPointer<GoSystemOnceExplem> GoSystemOnceExplem::instance;
GoSystemOnceExplem* GoSystemOnceExplem::getInstance() {
	if (instance.isNull()) {
		QMutexLocker locker(&mutex);
		if (instance.isNull()) {
			instance.reset(new GoSystemOnceExplem());
		}
	}
	return instance.data();
}

void GoSystemOnceExplem::destroyInstance() {
	QMutexLocker locker(&mutex);
	instance.reset();
}
#pragma execution_character_set("utf-8")
struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;
QMap<GoSystem, CameraFunSDKfactoryCls*> CallBackMap;//回调里面只传GoSystem*,通过GoSystem*绑定实际操作类
static QMutex g_callBackMapMutex;
#pragma region LMI
//注册回调 string对应自身的参数协议 （自定义）
void Hd_CameraModule_3DLMI3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;

	TempPack.GetimagescallbackFunc = callBackFun;
	m_sdkFunc->CallbackFuncMap.insert(TempPack.cameraIndex.toInt(), TempPack);
	//m_sdkFunc->CallbackFuncVec.append(TempPack);
	qDebug() << getString;
}

kStatus kCall onData(void* ctx, void* sys, void* dataset)
{
	// 先检查回调映射和句柄有效性
	if (CallBackMap.isEmpty() || sys == kNULL || dataset == kNULL)
	{
		GoDestroy(dataset); // 避免 dataset 泄漏
		return kERROR;
	}

	CameraFunSDKfactoryCls* Data = CallBackMap.value((GoSystem)sys);
	if (Data == nullptr || Data->isDestroying.load(std::memory_order_acquire))
	{
		GoDestroy(dataset);
		return kERROR;
	}

	// 加锁访问 Data 中的成员（避免主线程销毁时并发访问）
	QMutexLocker locker(&Data->sdkMutex);
	if (Data->system1 == kNULL || Data->sensor == kNULL)
	{
		GoDestroy(dataset);
		return kERROR;
	}


	double time_Start = (double)clock();
	unsigned int i;

	std::vector<cv::Mat> picVec;
	for (i = 0; i < GoDataSet_Count(dataset); ++i)
	{
		GoDataMsg dataObj = GoDataSet_At(dataset, i);

		switch (GoDataMsg_Type(dataObj))
		{
		case GO_DATA_MESSAGE_TYPE_UNIFORM_SURFACE:
		{
			GoUniformSurfaceMsg surfaceMsg = dataObj;
			double XResolution = NM_TO_MM(GoSurfaceMsg_XResolution(surfaceMsg));
			double YResolution = NM_TO_MM(GoSurfaceMsg_YResolution(surfaceMsg));
			double ZResolution = NM_TO_MM(GoSurfaceMsg_ZResolution(surfaceMsg));
			double XOffset = UM_TO_MM(GoSurfaceMsg_XOffset(surfaceMsg));
			double YOffset = UM_TO_MM(GoSurfaceMsg_YOffset(surfaceMsg));
			double ZOffset = UM_TO_MM(GoSurfaceMsg_ZOffset(surfaceMsg));

			int Width = GoSurfaceMsg_Width(surfaceMsg);
			int Height = GoSurfaceMsg_Length(surfaceMsg);
			if (Width == 1 || Height == 1)
				break;
			cv::Mat image(Height, Width, CV_16SC1);
			for (int rowIdx = 0; rowIdx < Height; rowIdx++)
			{
				k16s* data = GoSurfaceMsg_RowAt(surfaceMsg, rowIdx);
				auto ptr = image.ptr<short>(rowIdx);
				memcpy(ptr, data, sizeof(short) * Width);
			}
			image.convertTo(image, CV_16UC1, 1.0, 32768);
			//if (Data->allowflag.load(std::memory_order::memory_order_acquire))
			{
				if (Data->triggedType == 0)
				{
					vector<cv::Mat> Getimagevector;
					int realIndex = Data->Currentindex * 2;
					realIndex++;
					Getimagevector.push_back(image.clone());
					if (Data->CallbackFuncMap.keys().contains(realIndex))
					{
						qDebug() << "Mat Type" << "heightMat" << "out Mat callback" << Data->CallbackFuncMap.keys() << realIndex << Data->getImageMaxCoiunts;
						QObject* obj = Data->CallbackFuncMap.value(realIndex).callbackparent;
						obj->setProperty("cameraIndex", QString::number(realIndex));

						Data->CallbackFuncMap.value(realIndex).GetimagescallbackFunc(obj, Getimagevector);
					}
				}
				else
				{
					picVec.push_back(image.clone());
					QDateTime cut = QDateTime::currentDateTime();
					qDebug() << Data->k32u_id << "get heightimage width:" << Width << " height:" << Height << " time " << cut.toString("hh:mm:ss.zzz");

				}

			}
		}
		break;
		case GO_DATA_MESSAGE_TYPE_SURFACE_INTENSITY:
		{
			GoSurfaceIntensityMsg surfaceIntMsg = dataObj;
			double XResolution = NM_TO_MM(GoSurfaceIntensityMsg_XResolution(surfaceIntMsg));
			double YResolution = NM_TO_MM(GoSurfaceIntensityMsg_YResolution(surfaceIntMsg));
			double XOffset = UM_TO_MM(GoSurfaceIntensityMsg_XOffset(surfaceIntMsg));
			double YOffset = UM_TO_MM(GoSurfaceIntensityMsg_YOffset(surfaceIntMsg));

			int Width = GoSurfaceIntensityMsg_Width(surfaceIntMsg);
			int Height = GoSurfaceIntensityMsg_Length(surfaceIntMsg);

			cv::Mat image(Height, Width, CV_8UC1);
			for (int rowIdx = 0; rowIdx < Height; rowIdx++)
			{
				k8u* data = GoSurfaceIntensityMsg_RowAt(surfaceIntMsg, rowIdx);
				auto ptr = image.ptr<uchar>(rowIdx);
				for (int colIdx = 0; colIdx < Width; colIdx++)
				{
					memcpy(ptr, data, Width);
				}
			}
			//if (Data->allowflag.load(std::memory_order::memory_order_acquire))
			{
				if (Data->triggedType == 0)
				{
					vector<cv::Mat> Getimagevector;
					int realIndex = Data->Currentindex * 2;
					Getimagevector.push_back(image.clone());
					if (Data->CallbackFuncMap.keys().contains(realIndex))
					{
						qDebug() << "Mat Type" << "luminanceMat" << "out Mat callback" << Data->CallbackFuncMap.keys() << realIndex << Data->getImageMaxCoiunts;
						QObject* obj = Data->CallbackFuncMap.value(realIndex).callbackparent;
						obj->setProperty("cameraIndex", QString::number(realIndex));

						Data->CallbackFuncMap.value(realIndex).GetimagescallbackFunc(obj, Getimagevector);
					}
				}
				else
				{
					picVec.push_back(image.clone());
					QDateTime cut = QDateTime::currentDateTime();
					qDebug() << Data->k32u_id << "get lumiimage width:" << Width << " height:" << Height << " time " << cut.toString("hh:mm:ss.zzz");

				}
			}
		}
		break;
		}


		//if (Data->allowflag.load(std::memory_order::memory_order_acquire))
		{
			if (Data->triggedType == 1)
			{
				//qDebug() << Data->k32u_id << "get lumiimage width:" << endl;
				/*for (int i=0;i<picVec.size();i++)
				{
					vector<cv::Mat> pic;
					pic.push_back(picVec[i].clone());
					Data->ImageMats.push(pic);
				}*/
				Data->ImageMats.push(picVec);

			}
		}
	}
	Data->Currentindex++;
	if (Data->Currentindex >= Data->getImageMaxCoiunts / Data->OnceGetImageNum)	Data->Currentindex = 0;

	GoDestroy(dataset);
	double time_End = (double)clock();

	return kOK;
}
//初始化流程
bool InitLmi(k32u ID, CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
	kStatus status;

	if ((status = GoSdk_Construct(&m_CameraFunSDKfactoryCls->api)) != kOK)
	{
		printf("Error: GoSdk_Construct:%d\n", status);
		return false;
	}
	if ((status = GoSystem_Construct(&m_CameraFunSDKfactoryCls->system1, kNULL)) != kOK)
	{
		printf("Error: GoSystem_Construct:%d\n", status);
		return false;
	}
	if ((status = GoSystem_FindSensorById(m_CameraFunSDKfactoryCls->system1, m_CameraFunSDKfactoryCls->k32u_id, &m_CameraFunSDKfactoryCls->sensor)) != kOK)
	{
		//printf("Error: GoSystem_FindSensor:%d\n", status);
		qDebug() << "[Error] " << "Error: GoSystem_FindSensor:%d\n", status;
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
	CallBackMap.insert(m_CameraFunSDKfactoryCls->system1, m_CameraFunSDKfactoryCls);
	// enable sensor data channel
	if ((status = GoSystem_EnableData(m_CameraFunSDKfactoryCls->system1, kTRUE)) != kOK)
	{
		//printf("Error: GoSensor_EnableData:%d\n", status);
		qDebug() << "[Error] " << "Error: GoSensor_EnableData:%d\n", status;
		return false;
	}
	// set data handler to receive data asynchronously
	if ((status = GoSystem_SetDataHandler(m_CameraFunSDKfactoryCls->system1, onData, m_CameraFunSDKfactoryCls->contextPointer)) != kOK)
	{
		//printf("Error: GoSystem_SetDataHandler:%d\n", status);
		qDebug() << "[Error] " << "Error: GoSystem_SetDataHandler:%d\n", status;
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
	//QMutexLocker locker(&m_CameraFunSDKfactoryCls->sdkMutex); // 加锁保护

	 //检查是否正在销毁、未初始化，或已运行
	if (m_CameraFunSDKfactoryCls->isDestroying.load(std::memory_order_acquire) ||
		!m_CameraFunSDKfactoryCls->isInited.load(std::memory_order_acquire) ||
		m_CameraFunSDKfactoryCls->isRunning.load(std::memory_order_acquire))
	{
		qDebug() << "[Warn] StartLmi: 无需启动（已运行/未初始化/正在销毁）";
		return false;
	}

	kStatus status;
	status = GoSystem_Start(m_CameraFunSDKfactoryCls->system1);
	if (status != kOK)
	{
		qDebug() << "[Error] Failed to start LMI，错误码:" << status;
		return false;
	}

	// 启动成功，设置 isRunning 为 true
	m_CameraFunSDKfactoryCls->isRunning.store(true, std::memory_order_release);
	qDebug() << "[INFO] LMI 启动成功，isRunning = true";
	return true;
}

bool StopLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
	QMutexLocker locker(&m_CameraFunSDKfactoryCls->sdkMutex); // 加锁保护

	// 检查是否正在销毁，或未运行
	if (m_CameraFunSDKfactoryCls->isDestroying.load(std::memory_order_acquire) ||
		!m_CameraFunSDKfactoryCls->isRunning.load(std::memory_order_acquire))
	{
		qDebug() << "[Warn] StopLmi: 无需停止（未运行/正在销毁）";
		// 即使未运行，也清理数据（避免残留）
		if (m_CameraFunSDKfactoryCls->system1 != kNULL)
		{
			GoSystem_ClearData(m_CameraFunSDKfactoryCls->system1);
		}
		return true;
	}

	kStatus status = GoSystem_Stop(m_CameraFunSDKfactoryCls->system1);
	if (status != kOK)
	{
		qDebug() << "[Error] GoSystem_Stop 失败，错误码:" << status;
		return false;
	}

	// 停止成功，设置 isRunning 为 false
	m_CameraFunSDKfactoryCls->isRunning.store(false, std::memory_order_release);
	qDebug() << "[INFO] LMI 停止成功，isRunning = false";

	// 清理数据
	status = GoSystem_ClearData(m_CameraFunSDKfactoryCls->system1);
	if (status != kOK)
	{
		qDebug() << "[Error] GoSystem_ClearData 失败，错误码:" << status;
		return false;
	}
	status = GoSystem_Cancel(m_CameraFunSDKfactoryCls->system1);
	if (status != kOK)
	{
		qDebug() << "[Error] GoSystem_Cancel 失败，错误码:" << status;
		return false;
	}
	return true;
}

void CloseLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
	kStatus status;
	if (m_CameraFunSDKfactoryCls->sensor != kNULL)
	{
		GoSensor_Disconnect(m_CameraFunSDKfactoryCls->sensor); // 断开传感器连接
		m_CameraFunSDKfactoryCls->sensor = kNULL;
	}
	if (m_CameraFunSDKfactoryCls->system1 != kNULL)
	{
		status = GoSystem_ClearData(m_CameraFunSDKfactoryCls->system1); // 清理残留数据
		if (status != kOK)
		{
			qDebug() << "[Error] GoSystem_ClearData 失败，错误码:" << status;

		}
		status = GoDestroy(m_CameraFunSDKfactoryCls->system1); // 销毁系统句柄（原GoDestroy可能是宏定义，需确认是否对应Destroy）
		if (status != kOK)
		{
			qDebug() << "[Error] GoDestroy system1 失败，错误码:" << status;

		}
		m_CameraFunSDKfactoryCls->system1 = kNULL;
	}

	status = GoDestroy(m_CameraFunSDKfactoryCls->api); // 销毁SDK句柄
	if (status != kOK)
	{
		qDebug() << "[Error] GoDestroy api 失败，错误码:" << status;

	}
	m_CameraFunSDKfactoryCls->api = kNULL;

	m_CameraFunSDKfactoryCls->ImageMats.clear();
	if (m_CameraFunSDKfactoryCls->contextPointer)
	{
		delete m_CameraFunSDKfactoryCls->contextPointer;
		m_CameraFunSDKfactoryCls->contextPointer = nullptr;
	}
	qDebug() << "[INFO] LMI 关闭成功";

}
#pragma endregion
//类创建
Hd_CameraModule_3DLMI3::Hd_CameraModule_3DLMI3(QString DeviceSn, QString RootPath, int settype, QObject* parent)
	: PbGlobalObject(settype, parent)
{
	famliy = PGOFAMLIY::CAMERA3D;
	SnName = DeviceSn;
	if (RootPath.at(RootPath.length() - 1) == "\\" || RootPath.at(RootPath.length() - 1) == "/")
		JsonFile = RootPath + SnName + ".json";
	else
		JsonFile = RootPath + "/" + SnName + ".json";

	if (!QFile(JsonFile).exists())
		createAndWritefile(JsonFile, FirstCreateByte);
	QJsonObject paramObj = load_JsonFile(JsonFile);
	for (auto objStr : paramObj.keys())
	{
		ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
	}

	m_sdkFunc = new CameraFunSDKfactoryCls(DeviceSn, RootPath, this);
	//m_sdkFunc->k32u_id = SnName.toInt();
	setParameter(ParasValueMap);

	connect(m_sdkFunc, &CameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });
}

Hd_CameraModule_3DLMI3::~Hd_CameraModule_3DLMI3()
{
	qDebug() << "~Hd_CameraModule_3DLMI3";

	closeCamera(); // 调用关闭相机逻辑，释放SDK资源
	m_sdkFunc->disconnect();
	if (m_sdkFunc)
	{
		delete m_sdkFunc;
		m_sdkFunc = nullptr;
	}
}
//setParameter之后再调用，返回当前参数
	//相机：获取默认参数；
	//通信：获取初始化示例参数
QMap<QString, QString> Hd_CameraModule_3DLMI3::parameters()
{
	return ParasValueMap;
}
//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_3DLMI3::setParameter(const QMap<QString, QString>& ParameterMap)
{

	ParasValueMap = ParameterMap;
	m_sdkFunc->ParasValueMap = ParasValueMap;
	m_sdkFunc->triggedType = ParasValueMap["triggedType"].toInt();
	type1 = m_sdkFunc->triggedType;
	m_sdkFunc->upDateParam();
	return true;
}
//初始化(加载模块待内存)
bool Hd_CameraModule_3DLMI3::init()
{
	connect(this, &PbGlobalObject::trigged, [=](int Code) {
		if (Code == 1000)
		{
			m_sdkFunc->Currentindex = 0;
			m_sdkFunc->ImageMats.clear();
			m_sdkFunc->allowflag.store(true, std::memory_order::memory_order_release);
		}
		else if (Code == 1001)
		{
			m_sdkFunc->allowflag.store(false, std::memory_order::memory_order_release);
		}
		});

	setParameter(ParasValueMap);

	bool flag = m_sdkFunc->initSdk(ParasValueMap);
	m_sdkFunc->triggedType = ParasValueMap["triggedType"].toInt();

	if (flag)
	{
		emit trigged(0);
	}
	else
	{
		emit trigged(1);
		//return false;
	}

	if (m_sdkFunc->triggedType > 0)
	{
		type1 = 0;
	}
	else
	{
		type1 = 1;
	}

	qDebug() << SnName;
	return flag;
}

bool Hd_CameraModule_3DLMI3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
	if (mats.empty() && data.isEmpty())//外部调用只做触发
	{

		emit trigged(501);
		return true;
	}
	else//其他调用，界面等
	{

	}
	return true;
}

bool Hd_CameraModule_3DLMI3::data(std::vector<cv::Mat>& ImgS, QStringList& data)
{

	while (true)
	{
		Sleep(1);
		m_sdkFunc->ImageMats.wait_for_pop(3000, ImgS);
		if (ImgS.size() == 2)
		{
			break;
		}
	}
	/*std::vector<cv::Mat> tempimg;
   while( true)
   {
	   Sleep(1);
	   m_sdkFunc->ImageMats.wait_for_pop(3000, tempimg);
	   if (tempimg.size()>0)
	   {
		   ImgS.push_back(tempimg[0].clone());
	   }

	   if (ImgS.size()>1)
	   {
		   break;
	   }
   }*/

	qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " ImgS Size" << ImgS.size();
	if (ImgS.empty())
	{
		//ImgS.push_back(cv::Mat::zeros(100, 100, 0));
		qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
		return false;
	}
	return true;
}
void Hd_CameraModule_3DLMI3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
	int index = getString.toInt();
	qDebug() << "cancelCallBackFun" << " Current Index" << getString;
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
//相机连接状态：正常连接或断开
bool Hd_CameraModule_3DLMI3::checkStatus()
{
	return true;
}

bool Hd_CameraModule_3DLMI3::closeCamera()
{
	qDebug() << "~closeCamera";
	if (m_sdkFunc)
	{
		StopLmi(m_sdkFunc); // 停止数据传输
		m_sdkFunc->statusRunning = false; // 确保线程退出标志位设置
		const int timeoutMs = 3000; // 超时时间：3秒
		const int checkIntervalMs = 50; // 检查间隔：50毫秒
		int elapsedMs = 0;

		while (!m_sdkFunc->StateResult.isFinished() && elapsedMs < timeoutMs)
		{
			QThread::msleep(checkIntervalMs); // 短睡眠，避免CPU占用过高
			elapsedMs += checkIntervalMs;
		}
		CloseLmi(m_sdkFunc); // 释放SDK资源
	}
	return true;
}

void CameraFunSDKfactoryCls::threadCheckState() //线程检查相机状态
{
	int checkNum = 0;
	while (statusRunning.load(std::memory_order_acquire))
	{
		QThread::msleep(1);

		if (checkNum++ == 50)
		{
			QMutexLocker locker(&sdkMutex); // 加锁保护
			if (sensor != kNULL)
			{
				bool currentState = false;
				int mstate = GoSensor_State(sensor);
				if (mstate == 6)
					currentState = false;
				else
					currentState = true;
				if (currentState != CameraStatus)
				{
					qDebug() << "[INFO] " << "moduleStatus changed emit trigged:" << CameraStatus;
					if (currentState == true) //重连成功
					{
						emit trigged(0);
					}
					else //掉线
					{
						emit trigged(1);
					}
					CameraStatus = currentState;
				}
				if (currentState == false) //掉线，重新连接
				{
					locker.unlock();
					if (initSdk(ParasValueMap))
					{
						qDebug() << "[INFO] " << "moduleStatus changed emit trigged:" << CameraStatus;
						emit trigged(0);
					}
					locker.relock();
				}
			}
			checkNum = 0;
		}

	}
	return;
}

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
	int index = 0;

	OnePb temp;
	temp.base = new Hd_CameraModule_3DLMI3(DeviceSn, path + "/Hd_CameraModule_3DLMI3/");
	if (!temp.base->init())
		return false;
	temp.baseWidget = new mPrivateWidget(temp.base);
	temp.DeviceSn = DeviceSn;
	TotalMap.insert(name.split(':').first(), temp);
	return  true;
}

void destroy(const QString& name)
{
	qDebug() << "____________________name_______________________" << name;
	// 清理当前相机的资源
	auto temp = TotalMap.take(name);
	if (temp.base)
	{
		//temp.base->closeCamera(); // 确保SDK资源释放
		delete temp.base;
		temp.base = nullptr;
	}
	if (temp.baseWidget)
	{
		delete temp.baseWidget;
		temp.baseWidget = nullptr;
	}

	// 清理全局回调映射（仅当所有相机都销毁时）
	if (TotalMap.isEmpty())
	{
		QMutexLocker locker(&g_callBackMapMutex); // 加全局锁

		// 仅清 

		GoSystemOnceExplem::destroyInstance(); // 销毁单例
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
	kStatus status;
	GoSystem system1 = kNULL;
	kAssembly api = kNULL;
	if ((status = GoSdk_Construct(&api)) != kOK)
	{
		printf("Error: GoSdk_Construct:%d\n", status);
	}
	if ((status = GoSystem_Construct(&system1, kNULL)) != kOK)
	{
		printf("Error: GoSystem_Construct:%d\n", status);
	}
	for (int i = 0; i < GoSystem_SensorCount(SYSTEMGO); i++)
	{
		GoSensor tempSenor = GoSystem_SensorAt(system1, i);
		k32u getId = GoSensor_Id(tempSenor);
		temp << QString::number(getId);
	}
	// 查询已经使用的
	foreach(const auto& tmp, TotalMap)
	{
		if (temp.contains(tmp.DeviceSn))
		{
			temp.removeOne(tmp.DeviceSn);
		}
	}
	return temp;
}

CameraFunSDKfactoryCls::CameraFunSDKfactoryCls(QString sn, QString path, QObject* parent)
	: k32u_id(sn.toInt()), parent(parent), RootPath(path)
{

}

CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
{

	// 1. 标记为正在销毁，阻止其他线程继续操作（新增成员已声明）
	isDestroying.store(true, std::memory_order_release);

	// 2. 停止状态监控线程
	statusRunning.store(false, std::memory_order_release);
	const int timeoutMs = 3000; // 超时时间：3秒
	const int checkIntervalMs = 50; // 检查间隔：50毫秒
	int elapsedMs = 0;

	while (!StateResult.isFinished() && elapsedMs < timeoutMs)
	{
		QThread::msleep(checkIntervalMs); // 短睡眠，避免CPU占用过高
		elapsedMs += checkIntervalMs;
	}

	// 3. 加锁释放 SDK 资源
	QMutexLocker locker(&sdkMutex);
	if (parent) {
		disconnect(parent, nullptr, this, nullptr);
	}
	// 4. 停止 SDK（若运行中）
	if (isRunning.load(std::memory_order_acquire))
	{
		GoSystem_Stop(system1); // 停止数据传输
		GoSystem_ClearData(system1); // 清理数据
		isRunning.store(false, std::memory_order_release);
	}


	// 移除全局回调映射（加全局锁）
	static QMutex callBackMapMutex;
	QMutexLocker callBackLocker(&callBackMapMutex);
	CallBackMap.remove((GoSystem)system1); // 只删自己的条目，不删全局
	//for (auto it = CallBackMap.begin(); it != CallBackMap.end(); ++it)
	//{
	//	delete it.value(); // 释放 CameraFunSDKfactoryCls*
	//}
	//CallBackMap.clear();
	GoSystemOnceExplem::destroyInstance(); // 销毁单例

	this->disconnect();

}

void CameraFunSDKfactoryCls::upDateParam()
{
	getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts").toInt();
	OnceGetImageNum = ParasValueMap.value("OnceImageCounts").toInt();
	timeOut = ParasValueMap.value("GetOnceImageTimes").toInt();
	return;
}

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{
	QMutexLocker locker(&sdkMutex); // 加锁初始化

	if (isDestroying.load(std::memory_order_acquire))
	{
		qDebug() << "[Error] initSdk: 正在销毁中，禁止初始化";
		return false;
	}

	// 重置运行状态（初始化前确保 isRunning 为 false）
	isRunning.store(false, std::memory_order_release);

	bool flag = InitLmi(k32u_id, this);
	if (flag)
	{
		statusRunning = true;
		StateResult = QtConcurrent::run(this, &CameraFunSDKfactoryCls::threadCheckState);
		isInited.store(true, std::memory_order_release);
		StartLmi(this); // 初始化成功后启动 SDK
	}
	return flag;
}
mPrivateWidget::mPrivateWidget(void* handle)
{
	m_Camerahandle = reinterpret_cast<Hd_CameraModule_3DLMI3*>(handle);
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
	//qDebug() << m_Camerahandle->GetRootPath() + "/" + m_Camerahandle->GetSn() + ".json";
	m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath());
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
	connect(OpenGrapMat, &QPushButton::clicked, this, [=]() {emit m_Camerahandle->trigged(1000); });
	connect(NotGrapMat, &QPushButton::clicked, this, [=]() {emit m_Camerahandle->trigged(1001); });
	connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;  QStringList list;
		emit m_Camerahandle->trigged(1000);
		m_Camerahandle->setData(mats, list);
		m_Camerahandle->data(mats, list);
		if (!mats.empty())
		{
			if (mats.size() == 1)
			{
				cv::Mat tempMat = mats.at(0);
				m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
			}
			else
			{
				cv::Mat tempMat = mats.at(1);
				m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
			}

		}

		});
	mainHboxLayout->addLayout(MainLayout, 4);
	mainHboxLayout->addWidget(m_AlgParmWidget, 3);
}