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
  "OneceGetImageCounts": "2"})");

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

QJsonObject load_JsonFile(QString filename)
{
	QString json_cfg_file_path = filename;

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
#pragma execution_character_set("utf-8")
bool createAndWritefile(const QString& filename, const QByteArray& writeByte)
{
	QString path = filename.toLocal8Bit();
	path = path.mid(0, path.lastIndexOf("/"));
	QDir dir(path);
	dir.mkpath(dir.path());
	QFile file(filename);
	if (file.exists())
	{
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
			qWarning() << "错误,无法创建文件" << filename << file.errorString();
			return false;
		}
	}
	else
	{
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
			qWarning() << "错误,无法创建文件" << filename << file.errorString();
			return false;
		}
	}
	QTextStream out(&file);
	out.setCodec("utf-8");
	//for (auto str : inputData)
	{
		out << writeByte;
	}
	file.close();
	return true;
}

struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;
QMap<GoSystem, CameraFunSDKfactoryCls*> CallBackMap;//回调里面只传GoSystem*
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
void Hd_CameraModule_3DLMI3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;
	TempPack.GetimagescallbackFunc = callBackFun;
	m_sdkFunc->CallbackFuncVec.append(TempPack);
	qDebug() << getString;
}

kStatus kCall onData(void* ctx, void* sys, void* dataset)
{
	CameraFunSDKfactoryCls* Data = CallBackMap.value(sys);// (Hd_CameraModule_3DLMI3*)sys;
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
			if (Data->allowflag.load(std::memory_order::memory_order_acquire))
				picVec.push_back(image.clone());
			QDateTime cut = QDateTime::currentDateTime();
			qDebug() << Data->k32u_id << "get heightmat width:" << Width << " height:" << Height << " time " << cut.toString("hh:mm:ss.zzz");

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
			if (Data->allowflag.load(std::memory_order::memory_order_acquire))
				picVec.push_back(image.clone());
			QDateTime cut = QDateTime::currentDateTime();
			qDebug() << Data->k32u_id << "get lumiimage width:" << Width << " height:" << Height << " time " << cut.toString("hh:mm:ss.zzz");

		}
		break;
		}


		if (Data->allowflag.load(std::memory_order::memory_order_acquire))
			Data->ImageMats.push(picVec);
	}
	if (Data->triggedType)//硬触发，回调
	{
		Data->CallbackFuncVec.at(Data->Currentindex).GetimagescallbackFunc(Data->CallbackFuncVec.at(Data->Currentindex).callbackparent, picVec);
	}
	Data->Currentindex++;

	GoDestroy(dataset);
	double time_End = (double)clock();

	return kOK;
}
//初始化流程
bool InitLmi(k32u ID, CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
	kStatus status;
	if ((status = GoSystem_FindSensorById(m_CameraFunSDKfactoryCls->system1, ID, &m_CameraFunSDKfactoryCls->sensor)) != kOK)
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
	kStatus status;
	if ((status = GoSystem_Start(m_CameraFunSDKfactoryCls->system1)) != kOK)
	{
		//printf("Error: GoSystem_Stop:%d\n", status);
		qDebug() << "[Error] " << "Failed to start LMI ";
		return false;
	}
	return true;
}

bool StopLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
	kStatus status;
	if ((status = GoSystem_Stop(m_CameraFunSDKfactoryCls->system1)) != kOK)
	{
		qDebug() << "[Error] " << "Failed to stop LMI ";
		return false;
	}
	return true;
}

void CloseLmi(CameraFunSDKfactoryCls* m_CameraFunSDKfactoryCls)
{
	// destroy handles
	GoDestroy(m_CameraFunSDKfactoryCls->system1);
}
#pragma endregion
//类创建
Hd_CameraModule_3DLMI3::Hd_CameraModule_3DLMI3(QString DeviceSn, QString RootPath, int settype, QObject* parent)
	: PbGlobalObject(settype, parent)
{
	QFuture<void> myF = QtConcurrent::run(this, &Hd_CameraModule_3DLMI3::threadCheckState);

	famliy = PGOFAMLIY::CAMERA3D;	
	SnName = DeviceSn;
	JsonFile = RootPath + SnName + ".json";
	if (!QFile(JsonFile).exists())
		createAndWritefile(JsonFile, FirstCreateByte);
	QJsonObject paramObj = load_JsonFile(JsonFile);
	for (auto objStr : paramObj.keys())
	{
		ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
	}
	m_sdkFunc = new CameraFunSDKfactoryCls(DeviceSn, RootPath, this);
	connect(m_sdkFunc, &CameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });
}

Hd_CameraModule_3DLMI3::~Hd_CameraModule_3DLMI3()
{
	this->disconnect();
	
	CloseLmi(m_sdkFunc);
}
//setParameter之后再调用，返回当前参数
	//相机：获取默认参数；
	//通信：获取初始化示例参数
QMap<QString, QString> Hd_CameraModule_3DLMI3::parameters()
{
	 return m_sdkFunc->ParasValueMap;
}
//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_3DLMI3::setParameter(const QMap<QString, QString>& ParameterMap)
{

	ParasValueMap = ParameterMap;
	m_sdkFunc->ParasValueMap = ParasValueMap;
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
	bool flag = m_sdkFunc->initSdk(ParasValueMap);
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
		return true;
	}
	else//其他调用，界面等
	{

	}
	return true;
}

bool Hd_CameraModule_3DLMI3::data(std::vector<cv::Mat>& ImgS, QStringList& data)
{
	m_sdkFunc->ImageMats.wait_for_pop(3000, ImgS);
	if (ImgS.empty())
	{
		ImgS.push_back(cv::Mat::zeros(100, 100, 0));
		qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
		return false;
	}
	return true;
}
//相机连接状态：正常连接或断开
bool Hd_CameraModule_3DLMI3::checkStatus()
{
	return true;
}

void Hd_CameraModule_3DLMI3::threadCheckState() //线程检查相机状态
{
	int checkNum = 0;
	//while (m_sdkFunc->ifRunning == true)
	//{
	//	if (checkNum++ == 50)
	//	{
	//		if (m_sdkFunc->sensor != kNULL)
	//		{
	//			bool currentState = false;
	//			int mstate = GoSensor_State(m_sdkFunc->sensor);
	//			if (mstate == 6)
	//				currentState = false;
	//			else
	//				currentState = true;
	//			if (currentState != m_sdkFunc->moduleStatus)
	//			{
	//				qDebug() << "[INFO] " << "moduleStatus changed emit trigged:" << m_sdkFunc->moduleStatus;
	//				if (currentState == true) //重连成功
	//				{
	//					emit trigged(0);
	//				}
	//				else //掉线
	//				{
	//					emit trigged(1);
	//				}
	//				m_sdkFunc->moduleStatus = currentState;
	//			}
	//			if (currentState == false) //掉线，重新连接
	//			{
	//				if (init())
	//				{
	//					qDebug() << "[INFO] " << "moduleStatus changed emit trigged:" << m_sdkFunc->moduleStatus;
	//					emit trigged(0);
	//				}
	//			}
	//		}
	//		checkNum = 0;
	//	}
	//	Sleep(10);
	//}
}

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
	int index = 0;

	OnePb temp;
	temp.base = new Hd_CameraModule_3DLMI3(DeviceSn, path + "/Hd_CameraModule_3DKeyence3/");
	if (!temp.base->init())
		return false;
	temp.baseWidget = new QWidget();
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
	for (int i = 0; i < GoSystem_SensorCount(SYSTEMGO); i++)
	{
		GoSensor tempSenor = GoSystem_SensorAt(SYSTEMGO, i);
		k32u getId = GoSensor_BuddyId(tempSenor);
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
	: k32u_id(sn.toUShort()), parent(parent), RootPath(path)
{

}

void CameraFunSDKfactoryCls::upDateParam()
{
	return;
}

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{
	bool flag;
	flag =InitLmi(k32u_id,this);

	flag ? false : StartLmi(this);

	return flag;
}