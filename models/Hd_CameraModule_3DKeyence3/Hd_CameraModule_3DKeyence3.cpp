#include "Hd_CameraModule_3DKeyence3.h"
#include <exception>
#include <QTextCodec>
#include <qqueue.h>
#include <QWidget>
const QByteArray FirstCreateByte(R"({"DeviceId": "0",
  "GetOnceImageTimes": "5000",
  "Ip": "192.168.0.1",
  "Port": "24691",
  "xImageSize": "3200",
  "yImageSize": "1000",
  "y_pitch_um": "20.0",
  "OneceGetImageCounts": "2"})");
//初始化数据，创建json文件数据
/*
DeviceId  设备ID，在数组中的位置
GetOnceImageTimes  一次出图超时时间
Ip 设备IP
Port 端口号
xImageSize 图像X方向长度(宽)
yImageSize 图像y方向长度(高)
y_pitch_um y方向精度
OneceGetImageCounts 一次出图数量 (一般是两张，一张亮度图，一张高度图)
*/


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
#pragma execution_character_set("utf-8")
struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;



const int MAX_LJXA_XDATANUM = 3200;
const unsigned int BUFFER_FULL_COUNT = 30000;

int _imageAvailable[MAX_LJXA_DEVICENUM] = { 0 };
unsigned short* _luminanceBuf[MAX_LJXA_DEVICENUM] = { 0 };
unsigned short* _heightBuf[MAX_LJXA_DEVICENUM] = { 0 };
int _lastImageSizeHeight[MAX_LJXA_DEVICENUM];
int _highSpeedPortNo[MAX_LJXA_DEVICENUM];
LJX8IF_ETHERNET_CONFIG _ethernetConfig[MAX_LJXA_DEVICENUM];
LJXA_ACQ_GETPARAM _getParam[MAX_LJXA_DEVICENUM];
QString byteArrayToUnicode(const QByteArray array) {

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
void Hd_CameraModule_3DKeyence3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;
	TempPack.GetimagescallbackFunc = callBackFun;
	m_sdkFunc->CallbackFuncVec.append(TempPack);
	qDebug() << getString;
}
void Hd_CameraModule_3DKeyence3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
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
cameraFunSDKfactoryCls::cameraFunSDKfactoryCls(int id, QString RootPath, QObject* praent) :deviceId(id), parent(praent), RootPath(RootPath)
{

}
cameraFunSDKfactoryCls::~cameraFunSDKfactoryCls()
{
	Sleep(10);
	if (isopen)
	{
		isopen = false;

		// Stop HighSpeed
		errCode = LJX8IF_StopHighSpeedDataCommunication(deviceId);
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Stop HighSpeed errCode:" << errCode;

		LJXA_ACQ_CloseDevice(deviceId);
		qDebug() << __FUNCTION__ << "line:" << __LINE__ << " CloseDevice success!";
		if (heightImage)
		{
			free(heightImage);

		}
		if (luminanceImage)
		{
			free(luminanceImage);
		}
	}

	// Free memory
	if (_heightBuf[deviceId] != NULL)
	{
		free(_heightBuf[deviceId]);
	}

	if (_luminanceBuf[deviceId] != NULL)
	{
		free(_luminanceBuf[deviceId]);
	}

	if (startReq_ptr)
	{
		delete startReq_ptr;
		startReq_ptr = nullptr;
	}

	if (profileInfo_ptr)
	{
		delete profileInfo_ptr;
		profileInfo_ptr = nullptr;
	}

	if (setParam_Ptr)
	{
		delete setParam_Ptr;
		setParam_Ptr = nullptr;
	}

	if (getParam_Ptr)
	{
		delete getParam_Ptr;
		getParam_Ptr = nullptr;
	}
}

bool cameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{

	timeout_ms = insideValuesMaps.value("GetOnceImageTimes").toInt();
	QStringList IpList = insideValuesMaps.value("GetOnceImageTimes").split('.');
	if (IpList.size() != 4)
		return false;
	EthernetConfig.abyIpAddress[0] = IpList.at(0).toInt();
	EthernetConfig.abyIpAddress[1] = IpList.at(1).toInt();
	EthernetConfig.abyIpAddress[2] = IpList.at(2).toInt();
	EthernetConfig.abyIpAddress[3] = IpList.at(3).toInt();
	EthernetConfig.wPortNo = insideValuesMaps.value("Port").toInt();
	xImageSize = insideValuesMaps.value("xImageSize").toInt();
	yImageSize = insideValuesMaps.value("yImageSize").toInt();
	y_pitch_um = insideValuesMaps.value("y_pitch_um").toFloat();
	deviceId = insideValuesMaps.value("DeviceId").toInt();


	startReq_ptr = new LJX8IF_HIGH_SPEED_PRE_START_REQ;
	profileInfo_ptr = new LJX8IF_PROFILE_INFO;
	startReq_ptr->bySendPosition = 2;

	setParam_Ptr = new LJXA_ACQ_SETPARAM;
	getParam_Ptr = new LJXA_ACQ_GETPARAM;

	setParam_Ptr->y_linenum = yImageSize;
	setParam_Ptr->y_pitch_um = y_pitch_um;
	setParam_Ptr->timeout_ms = timeout_ms;
	setParam_Ptr->use_external_batchStart = use_external_batchStart;

	// Allocate user memory
	heightImage = (unsigned short*)malloc(sizeof(unsigned short) * xImageSize * yImageSize);
	luminanceImage = (unsigned short*)malloc(sizeof(unsigned short) * xImageSize * yImageSize);

	errCode = LJXA_ACQ_OpenDevice(deviceId, &EthernetConfig, HighSpeedPortNo);
	if (errCode != LJX8IF_RC_OK)
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Failed to open device ";
		//Free user memory
		if (heightImage)
		{
			free(heightImage);
		}

		if (luminanceImage)
		{
			free(luminanceImage);
		}
		return false;
	}

	isopen = true;

	// Allocate memory
	_heightBuf[deviceId] = (unsigned short*)malloc(yImageSize * MAX_LJXA_XDATANUM * 2);
	if (_heightBuf[deviceId] == NULL)
	{
		return LJX8IF_RC_ERR_NOMEMORY;
	}

	_luminanceBuf[deviceId] = (unsigned short*)malloc(yImageSize * MAX_LJXA_XDATANUM * 2);
	if (_luminanceBuf[deviceId] == NULL)
	{
		return LJX8IF_RC_ERR_NOMEMORY;
	}

	// Initialize
	if (!InitHighSpeed())
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  InitHighSpeed error！ ";
		return false;
	}

	return true;
}

void myCallbackFunc(LJX8IF_PROFILE_HEADER* pProfileHeaderArray, WORD* pHeightProfileArray, WORD* pLuminanceProfileArray, DWORD dwLuminanceEnable, DWORD dwProfileDataCount, DWORD dwCount, DWORD dwNotify, DWORD dwUser)
{
	try
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  --> Enter callback function";
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "dwProfileDataCount = " << dwProfileDataCount;
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "dwCount = " << dwCount;
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "dwNotify = " << dwNotify;
		if ((dwNotify != 0) || (dwNotify & 0x10000) != 0) return;
		if (dwCount == 0) return;
		if (_imageAvailable[dwUser] == 1) return;
		// _heightBuf 代表的含义:程序里的高度缓存区，pHeightProfileArray 回调函数里的数据 
		// 此处崩溃的可能性：xImageSize 配置文件中设置的值和3D相机调试软件设置的值不一致，会崩溃。
		if (_heightBuf[dwUser] == NULL)
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "_heightBuf[dwUser] == NULL";
			return;
		}
		if (pHeightProfileArray == NULL)
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "pHeightProfileArray== NULL";
			return;
		}

		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "_heightBuf[dwUser]: " << _heightBuf[dwUser];
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "pHeightProfileArray: " << pHeightProfileArray;

		//判断内存是否重叠
		if (_heightBuf[dwUser] <= pHeightProfileArray || (unsigned short*)_heightBuf[dwUser] >= (WORD*)pHeightProfileArray + dwProfileDataCount * dwCount * 2)
		{
			memcpy(&_heightBuf[dwUser][0], pHeightProfileArray, dwProfileDataCount * dwCount * 2);
		}
		else
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "first memcpy _heightBuf error";
			if (_heightBuf[dwUser] != NULL)
			{
				free(_heightBuf[dwUser]);
				_heightBuf[dwUser] = NULL;
			}
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  memcpy _heightBuf again ";
			_heightBuf[dwUser] = (unsigned short*)malloc(dwCount * MAX_LJXA_XDATANUM * 2);
			memcpy(&_heightBuf[dwUser][0], pHeightProfileArray, dwProfileDataCount * dwCount * 2);
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Second memcpy _heightBuf success ";

		}

		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "_luminanceBuf[dwUser]: " << _luminanceBuf[dwUser];
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "pLuminanceProfileArray: " << pLuminanceProfileArray;
		if (dwLuminanceEnable == 1)
		{
			if (_luminanceBuf[dwUser] == NULL)
			{
				qDebug() << __FUNCTION__ << " line:" << __LINE__ << "_luminanceBuf[dwUser]== NULL";
				return;
			}
			if (_luminanceBuf[dwUser] <= pLuminanceProfileArray || (unsigned short*)_luminanceBuf[dwUser] >= (WORD*)pLuminanceProfileArray + dwProfileDataCount * dwCount * 2)
				memcpy(&_luminanceBuf[dwUser][0], pLuminanceProfileArray, dwProfileDataCount * dwCount * 2);
			else
				qDebug() << __FUNCTION__ << " line:" << __LINE__ << "memcpy _luminanceBuf error";
		}
		_imageAvailable[dwUser] = 1;                 // 图像获取成功  dwUser  和 deviceId   一致
		_lastImageSizeHeight[dwUser] = dwCount;      // 扫描的行数，  dwCount 和 yImageSize 一致
	}
	catch (std::exception e)
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  --> execute callback function error";
	}
}

int cameraFunSDKfactoryCls::LJXA_ACQ_OpenDevice(int lDeviceId, LJX8IF_ETHERNET_CONFIG* EthernetConfig, int HighSpeedPortNo)
{
	int errCode = LJX8IF_EthernetOpen(lDeviceId, EthernetConfig);

	_ethernetConfig[lDeviceId] = *EthernetConfig;
	HighSpeedPortNo = HighSpeedPortNo;

	qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Open device ! errCode:" << errCode;
	return errCode;
}

void cameraFunSDKfactoryCls::LJXA_ACQ_CloseDevice(int lDeviceId)
{
	LJX8IF_FinalizeHighSpeedDataCommunication(lDeviceId);
	LJX8IF_CommunicationClose(lDeviceId);
	qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Close device!";
}

Hd_CameraModule_3DKeyence3::Hd_CameraModule_3DKeyence3(int DevicedID, QString RootPath, int settype, QObject* parent) : PbGlobalObject(settype, parent), deviceId(DevicedID)
{
	famliy = PGOFAMLIY::CAMERA3D;
	CHAR* pControllerSerialNo = nullptr; CHAR* pHeadSerialNo = nullptr;
	LJX8IF_GetSerialNumber(DevicedID, pControllerSerialNo, pHeadSerialNo);
	SnName = pHeadSerialNo;
	RootPath = RootPath + "/Hd_CameraModule_3DKeyence3/";
	JsonFile = RootPath + SnName + ".json";
	if (!QFile(JsonFile).exists())
		createAndWritefile(JsonFile, FirstCreateByte);
	QJsonObject paramObj = load_JsonFile(JsonFile);
	for (auto objStr : paramObj.keys())
	{
		ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
	}
	m_sdkFunc = new cameraFunSDKfactoryCls(DevicedID, RootPath, this);
	connect(m_sdkFunc, &cameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });

}

Hd_CameraModule_3DKeyence3::~Hd_CameraModule_3DKeyence3()
{
	if (m_sdkFunc)
	{
		delete m_sdkFunc;
		m_sdkFunc = nullptr;
	}
	qDebug() << __FUNCTION__ << "line:" << __LINE__ << " delete success!";
}

QMap<QString, QString> Hd_CameraModule_3DKeyence3::parameters()
{
	return ParasValueMap;
}

QJsonObject Hd_CameraModule_3DKeyence3::load_JsonFile(QString filename)
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

bool Hd_CameraModule_3DKeyence3::setParameter(const QMap<QString, QString>& ParameterMap)
{

	ParasValueMap = ParameterMap;
	m_sdkFunc->ParasValueMap = ParasValueMap;
	m_sdkFunc->upDateParam();
	return true;
}
//初始化(加载模块待内存)
bool Hd_CameraModule_3DKeyence3::init()
{
	connect(this, &PbGlobalObject::trigged, [=](int Code) {
		if (Code == 1000)
		{
			m_sdkFunc->Currentindex = 0;
			m_sdkFunc->heightMatS.clear();
			m_sdkFunc->luminanceMatS.clear();
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

	if (m_sdkFunc->use_external_batchStart > 0)
	{
		type1 = 0;
	}
	else
	{
		type1 = 1;
	}
	qDebug() << deviceId << SnName;
	return flag;
}

bool Hd_CameraModule_3DKeyence3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{


	return true;
}
//获取数据
bool Hd_CameraModule_3DKeyence3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
	return true;
}
bool cameraFunSDKfactoryCls::InitHighSpeed()
{
	try
	{
		if (startReq_ptr == nullptr)
		{
			startReq_ptr = new LJX8IF_HIGH_SPEED_PRE_START_REQ;
			startReq_ptr->bySendPosition = 2;
			qWarning() << __FUNCTION__ << "  line: " << __LINE__ << " startReq_ptr is null! ";
		}
		if (profileInfo_ptr == nullptr)
		{
			profileInfo_ptr = new LJX8IF_PROFILE_INFO;
			qWarning() << __FUNCTION__ << "  line: " << __LINE__ << " profileInfo_ptr is null! ";
		}
		errCode = LJX8IF_InitializeHighSpeedDataCommunicationSimpleArray(deviceId, &_ethernetConfig[deviceId], HighSpeedPortNo, &myCallbackFunc, yImageSize, deviceId); // 初始化高速通讯
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   errCode:" << errCode;
		errCode = LJX8IF_PreStartHighSpeedDataCommunication(deviceId, startReq_ptr, profileInfo_ptr);              // 预开始启动高速通信，作用 做预连接
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   errCode:" << errCode;

		// zUnit
		errCode = LJX8IF_GetZUnitSimpleArray(deviceId, &zUnit);
		if (errCode != 0 || zUnit == 0)
		{
			qCritical() << __FUNCTION__ << "  line:" << __LINE__ << " Failed to acquire zunit! ";
			return false;
		}
		errCode = LJX8IF_StartHighSpeedDataCommunication(deviceId);           // 正式开始高速通讯
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << " errCode: " << errCode;
	}
	catch (const QString ev)
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  ev: " << ev;
		return false;
	}
	return true;

}
void cameraFunSDKfactoryCls::upDateParam()
{
	timeout_ms = ParasValueMap.value("GetOnceImageTimes").toInt();
	QStringList IpList = ParasValueMap.value("GetOnceImageTimes").split('.');
	if (IpList.size() != 4)
		return;
	EthernetConfig.abyIpAddress[0] = IpList.at(0).toInt();
	EthernetConfig.abyIpAddress[1] = IpList.at(1).toInt();
	EthernetConfig.abyIpAddress[2] = IpList.at(2).toInt();
	EthernetConfig.abyIpAddress[3] = IpList.at(3).toInt();
	EthernetConfig.wPortNo = ParasValueMap.value("Port").toInt();
	xImageSize = ParasValueMap.value("xImageSize").toInt();
	yImageSize = ParasValueMap.value("yImageSize").toInt();
	y_pitch_um = ParasValueMap.value("y_pitch_um").toFloat();
	deviceId = ParasValueMap.value("DeviceId").toInt();
	return;
}
bool cameraFunSDKfactoryCls::run()
{
	if (!isopen)
	{
		qCritical() << __FUNCTION__ << " line:" << __LINE__ << " device is not opened! ";
		return false;
	}

	try
	{
		errCode = LJX8IF_StartHighSpeedDataCommunication(deviceId);           // 正式开始高速通讯
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << " errCode: " << errCode;
		if (errCode != 0x80A1 && errCode != 0x0000)
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Restart HighSpeedDataCommunication";
			errCode = LJX8IF_StopHighSpeedDataCommunication(deviceId);
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Stop HSC errCode:" << errCode;
			errCode = LJX8IF_FinalizeHighSpeedDataCommunication(deviceId);
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Fini HSC errCode:" << errCode;
			errCode = LJX8IF_InitializeHighSpeedDataCommunicationSimpleArray(deviceId, &_ethernetConfig[deviceId], HighSpeedPortNo, &myCallbackFunc, yImageSize, deviceId); // 初始化高速通讯
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Init HSC errCode:" << errCode;
			errCode = LJX8IF_PreStartHighSpeedDataCommunication(deviceId, startReq_ptr, profileInfo_ptr);              // 预开始启动高速通信，作用 做预连接
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   PreStart HSC errCode:" << errCode;
			errCode = LJX8IF_StartHighSpeedDataCommunication(deviceId);           // 正式开始高速通讯
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Start HSC errCode: " << errCode;
		}

		// Allocate memory
		if (_heightBuf[deviceId])
		{
			free(_heightBuf[deviceId]);
			_heightBuf[deviceId] = NULL;
		}

		if (_luminanceBuf[deviceId])
		{
			free(_luminanceBuf[deviceId]);
			_luminanceBuf[deviceId] = NULL;
		}
		int yDataNum = setParam_Ptr->y_linenum;
		_heightBuf[deviceId] = (unsigned short*)malloc(yDataNum * MAX_LJXA_XDATANUM * 2);
		_luminanceBuf[deviceId] = (unsigned short*)malloc(yDataNum * MAX_LJXA_XDATANUM * 2);

		if (_heightBuf[deviceId] == NULL)
		{
			return false;
		}

		if (_luminanceBuf[deviceId] == NULL)
		{
			return false;
		}

		int errCode;
		//Start HighSpeed
		_imageAvailable[deviceId] = 0;
		_lastImageSizeHeight[deviceId] = 0;

		//StartMeasure(Batch Start)
		if (use_external_batchStart > 0)
		{
		}
		else
		{
			errCode = LJX8IF_StartMeasure(deviceId);  // 启动软件触发,等同于软件中点击开始批处理操作
			qCritical() << __FUNCTION__ << " line:" << __LINE__ << "  Measure Start(Batch Start) ! errCode:" << errCode;
		}

		qDebug() << __FUNCTION__ << "  line:" << __LINE__ << " acquring image...! ";
		Hd_CameraModule_3DKeyence3* CurrentCamera = reinterpret_cast<Hd_CameraModule_3DKeyence3*>(parent);
		emit CurrentCamera->trigged(501);
		DWORD start = timeGetTime();
		while (true)
		{
			DWORD ts = timeGetTime() - start;
			if ((DWORD)timeout_ms < ts)
			{
				qCritical() << __FUNCTION__ << "  line:" << __LINE__ << "   timeout_ms " << ts;
				break;
			}
			if (_imageAvailable[deviceId])
			{
				qDebug() << " [info] " << __FUNCTION__ << " line:" << __LINE__ << " deviceId::" << deviceId << " checkImg getImg time" << ts << " imgIndex" << Currentindex++;
				break;
			}
			Sleep(1);
		}
		Sleep(1);
		if (_imageAvailable[deviceId] != 1)
		{
			qCritical() << __FUNCTION__ << "  line:" << __LINE__ << "_imageAvailable[deviceId] != 1";
			//Free memory
			if (_heightBuf[deviceId] != NULL)
			{
				free(_heightBuf[deviceId]);
				_heightBuf[deviceId] = NULL;
			}

			if (_luminanceBuf[deviceId] != NULL)
			{
				free(_luminanceBuf[deviceId]);
				_luminanceBuf[deviceId] = NULL;
			}
			// KeyModify   20230301 清理控制器里的缓存数据，此处是考虑超时情况下，对数据的清理操作。
			errCode = LJX8IF_ClearMemory(deviceId);
			qCritical() << __FUNCTION__ << "  line:" << __LINE__ << "   errCode:" << errCode;
			return false;
		}
		// qDebug() << __FUNCTION__ << " line:" << __LINE__ << "test";
		_getParam[deviceId].luminance_enabled = profileInfo_ptr->byLuminanceOutput;
		_getParam[deviceId].x_pointnum = profileInfo_ptr->wProfileDataCount;
		_getParam[deviceId].y_linenum_acquired = _lastImageSizeHeight[deviceId];
		_getParam[deviceId].x_pitch_um = profileInfo_ptr->lXPitch / 100.0f;
		_getParam[deviceId].y_pitch_um = y_pitch_um;
		_getParam[deviceId].z_pitch_um = zUnit / 100.0f;

		*getParam_Ptr = _getParam[deviceId];
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << " getPara Finished: ";
		int xDataNum = _getParam[deviceId].x_pointnum;

		unsigned short* dwHeightBuf = (unsigned short*)&_heightBuf[deviceId][0];
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << " dwHeightBuf Init Finished: ";
		memcpy(heightImage, dwHeightBuf, xDataNum * yImageSize * 2);

		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Height Img memcpy Finished: ";
		if (_getParam[deviceId].luminance_enabled > 0)
		{
			unsigned short* dwLuminanceBuf = (unsigned short*)&_luminanceBuf[deviceId][0];
			memcpy(luminanceImage, dwLuminanceBuf, xDataNum * yImageSize * 2);
		}

		// Free memory
		if (_heightBuf[deviceId] != NULL)
		{
			free(_heightBuf[deviceId]);
			_heightBuf[deviceId] = NULL;
		}

		if (_luminanceBuf[deviceId] != NULL)
		{
			free(_luminanceBuf[deviceId]);
			_luminanceBuf[deviceId] = NULL;
		}
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Free memory Finished: ";
		if (allowflag.load(std::memory_order::memory_order_acquire))
		{
			luminanceMatS.push(cv::Mat(yImageSize, xImageSize, CV_16U, luminanceImage));
			heightMatS.push(cv::Mat(yImageSize, xImageSize, CV_16U, heightImage));
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << " success to acquire 3d image! camera_name: " << deviceId;

		}
		else
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "allowflag is false" << "Not Allow to getimage";

		}


		return true;
	}
	catch (QString ev)
	{
		qCritical() << __FUNCTION__ << "  line: " << __LINE__ << "  ev: " << ev;
		return false;
	}
}

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
	int index = 0;
	for (; index < MAX_LJXA_DEVICENUM; index++)
	{
		CHAR* pControllerSerialNo = nullptr; CHAR* pHeadSerialNo = nullptr;
		LJX8IF_GetSerialNumber(index, pControllerSerialNo, pHeadSerialNo);
		if (pControllerSerialNo == nullptr || pHeadSerialNo == nullptr)
		{
			qWarning() << "NOT Found Device ID" << index << DeviceSn;
		}
		break;
		if (pHeadSerialNo == DeviceSn)
		{
			OnePb temp;
			temp.base = new Hd_CameraModule_3DKeyence3(index, path);
			if (!temp.base->init())
				return false;
			temp.baseWidget = new QWidget();
			temp.DeviceSn = DeviceSn;
			TotalMap.insert(name.split(':').first(), temp);
			return  true;
		}
	}
	return false;
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
	for (int i = 0; i < MAX_LJXA_DEVICENUM; i++)
	{
		CHAR* pControllerSerialNo = nullptr; CHAR* pHeadSerialNo = nullptr;
		LJX8IF_GetSerialNumber(i, pControllerSerialNo, pHeadSerialNo);
		if (pControllerSerialNo == nullptr || pHeadSerialNo == nullptr)
			break;
		temp << pHeadSerialNo;
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