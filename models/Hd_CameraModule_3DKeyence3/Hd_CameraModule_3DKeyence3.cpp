#include "Hd_CameraModule_3DKeyence3.h"
#include <exception>
#include <QTextCodec>
#include <qqueue.h>
#include <QWidget>
#include <QProcess>
#include <QNetworkInterface>
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
QStringList getConnectedDevicesFromARP() {
	QStringList devices;
	//QTextCodec* codec = QTextCodec::codecForName("GBK");
	//QTextCodec::setCodecForLocale(codec);
#ifdef Q_OS_WIN
	// Windows系统 - 使用 arp -a 命令
	QProcess process;
	process.start("arp", QStringList() << "-a");
	process.waitForFinished();
	QString output = byteArrayToUnicode(process.readAllStandardOutput());
	//QString output = QString::fromLocal8Bit(process.readAllStandardOutput());

	// 解析ARP表输出，格式如: 192.168.1.2    00-11-22-33-44-55    动态
	QRegularExpression regex(QString(R"((\d+\.\d+\.\d+\.\d+)\s+([0-9a-fA-F-]+))").toLocal8Bit());
	QRegularExpressionMatchIterator matches = regex.globalMatch(output);
	while (matches.hasNext()) {
		QRegularExpressionMatch match = matches.next();
		QString ip = match.captured(1);
		//qDebug() << ip;
		// 排除本机IP和广播地址
		if (!ip.endsWith(".255") && !ip.endsWith(".0") 
			 && ip.split('.').at(0) == "192"
			&& ip.split('.').at(1) == "168")//基恩士默认第三位是0配置ip时设置静态ip第三位为0
		{
			devices << ip;

		}
	}

#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
	// Linux/Mac系统 - 使用 arp -n 命令
	QProcess process;
	process.start("arp", QStringList() << "-n");
	process.waitForFinished();
	QString output = process.readAllStandardOutput();

	// 解析ARP表输出
	QRegularExpression regex(R"((\d+\.\d+\.\d+\.\d+)\s+\S+\s+\S+\s+([0-9a-fA-F:]+))");
	QRegularExpressionMatchIterator matches = regex.globalMatch(output);

	while (matches.hasNext()) {
		QRegularExpressionMatch match = matches.next();
		QString ip = match.captured(1);
		devices << ip;
	}
#endif
	qDebug() << devices;
	return devices;
}

QJsonObject load_JsonObjectFile(QString filename)
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
QJsonArray load_JsonArrayFile(QString filename)
{
	QString json_cfg_file_path = filename;

	QJsonArray JaonArray;
	try
	{
		QJsonParseError jsonError;
		if (json_cfg_file_path.isEmpty())
		{
			qCritical() << __FUNCTION__ << " line:" << __LINE__ << " JsonPath is null!";
			return JaonArray;
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
			return JaonArray;
		}

		QJsonDocument jsonDocument(QJsonDocument::fromJson(m_Byte, &jsonError));

		if (!jsonDocument.isNull() && jsonError.error == QJsonParseError::NoError)
		{
			if (jsonDocument.isArray())
			{
				JaonArray = jsonDocument.array();
				JsonFile.close();
				return JaonArray;
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
	return JaonArray;
}
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
	QString ip;
	QString index;
};
QMap<QString, OnePb>  TotalMap;
QVector<QPair<QString, QString>>TotalSnIpVec;


const int MAX_LJXA_XDATANUM = 3200;
const unsigned int BUFFER_FULL_COUNT = 30000;

int _imageAvailable[MAX_LJXA_DEVICENUM] = { 0 };
//unsigned short* _luminanceBuf[MAX_LJXA_DEVICENUM] = { 0 };
//unsigned short* _heightBuf[MAX_LJXA_DEVICENUM] = { 0 };
LJX8IF_HIGH_SPEED_PRE_START_REQ* startReq_ptr[MAX_LJXA_DEVICENUM];// = nullptr;
LJX8IF_PROFILE_INFO* profileInfo_ptr[MAX_LJXA_DEVICENUM];// = nullptr;
CameraFunSDKfactoryCls* SDKFunc[MAX_LJXA_DEVICENUM];
int _lastImageSizeHeight[MAX_LJXA_DEVICENUM];
int _highSpeedPortNo[MAX_LJXA_DEVICENUM];
LJX8IF_ETHERNET_CONFIG _ethernetConfig[MAX_LJXA_DEVICENUM];
LJXA_ACQ_GETPARAM _getParam[MAX_LJXA_DEVICENUM];
//注册回调 string对应自身的参数协议 （自定义）
void Hd_CameraModule_3DKeyence3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
{
	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;
	TempPack.GetimagescallbackFunc = callBackFun;
	m_sdkFunc->CallbackFuncMap.insert(getString.toInt(), TempPack);
	qDebug() << "registerCallBackFun" << getString;
}
void Hd_CameraModule_3DKeyence3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
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
CameraFunSDKfactoryCls::CameraFunSDKfactoryCls(int id, QString RootPath, QObject* praent)
	:deviceId(id), parent(praent), RootPath(RootPath)
{
}
CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
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
	}

	// Free memory


	if (startReq_ptr[deviceId])
	{
		delete startReq_ptr[deviceId];
		startReq_ptr[deviceId] = nullptr;
	}

	if (profileInfo_ptr[deviceId])
	{
		delete profileInfo_ptr[deviceId];
		profileInfo_ptr[deviceId] = nullptr;
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

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{

	/*timeout_ms = insideValuesMaps.value("GetOnceImageTimes").toInt();
	QStringList IpList = insideValuesMaps.value("Ip").split('.');
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
	use_external_batchStart = insideValuesMaps.value("use_external_batchStart").toInt();
	getImageMaxCoiunts = insideValuesMaps.value("OnceSignalsGetImageCounts").toInt();
	OnceGetImageNum = insideValuesMaps.value("OnceImageCounts").toInt();*/

	startReq_ptr[deviceId] = new LJX8IF_HIGH_SPEED_PRE_START_REQ;
	profileInfo_ptr[deviceId] = new LJX8IF_PROFILE_INFO;
	startReq_ptr[deviceId]->bySendPosition = 2;

	setParam_Ptr = new LJXA_ACQ_SETPARAM;
	getParam_Ptr = new LJXA_ACQ_GETPARAM;

	setParam_Ptr->y_linenum = yImageSize;
	setParam_Ptr->y_pitch_um = y_pitch_um;
	setParam_Ptr->timeout_ms = timeout_ms;
	setParam_Ptr->use_external_batchStart = use_external_batchStart;

	// Allocate user memory
	//heightImage = (unsigned short*)malloc(sizeof(unsigned short) * xImageSize * yImageSize);
	//luminanceImage = (unsigned short*)malloc(sizeof(unsigned short) * xImageSize * yImageSize);

	errCode = LJXA_ACQ_OpenDevice(deviceId, &EthernetConfig, HighSpeedPortNo);
	if (errCode != LJX8IF_RC_OK)
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Failed to open device ";
		//Free user memory		
		return false;
	}
	//char pControllerSerialNo [20] ; char pHeadSerialNo[20];
	//LJX8IF_GetSerialNumber(deviceId, pControllerSerialNo, pHeadSerialNo);
	//qDebug() << pControllerSerialNo << pHeadSerialNo;
	isopen = true;

	// Allocate memory
	/*_heightBuf[deviceId] = (unsigned short*)malloc(yImageSize * MAX_LJXA_XDATANUM * 2);
	if (_heightBuf[deviceId] == NULL)
	{
		return LJX8IF_RC_ERR_NOMEMORY;
	}

	_luminanceBuf[deviceId] = (unsigned short*)malloc(yImageSize * MAX_LJXA_XDATANUM * 2);
	if (_luminanceBuf[deviceId] == NULL)
	{
		return LJX8IF_RC_ERR_NOMEMORY;
	}*/

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
		unsigned short* heightBuf = (unsigned short*)malloc(dwProfileDataCount * dwCount * 2);
		if (heightBuf == NULL)
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "heightBuf == NULL";
			return;
		}
		if (pHeightProfileArray == NULL)
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "pHeightProfileArray== NULL";
			return;
		}

		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << "heightBuf " << heightBuf;
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << "pHeightProfileArray: " << pHeightProfileArray;

		//判断内存是否重叠
		if (heightBuf <= pHeightProfileArray || (unsigned short*)heightBuf >= (WORD*)pHeightProfileArray + dwProfileDataCount * dwCount * 2)
		{
			memcpy(heightBuf, pHeightProfileArray, dwProfileDataCount * dwCount * 2);
		}
		else
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "first memcpy _heightBuf error";
			if (heightBuf != NULL)
			{
				free(heightBuf);
				heightBuf = NULL;
			}
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  memcpy _heightBuf again ";
			heightBuf = (unsigned short*)malloc(dwCount * MAX_LJXA_XDATANUM * 2);
			memcpy(heightBuf, pHeightProfileArray, dwProfileDataCount * dwCount * 2);
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Second memcpy _heightBuf success ";

		}
		unsigned short* luminanceBuf = (unsigned short*)malloc(dwProfileDataCount * dwCount * 2);
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << "_luminanceBuf[dwUser]: " << luminanceBuf;
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << "pLuminanceProfileArray: " << pLuminanceProfileArray;
		if (dwLuminanceEnable == 1)
		{
			if (luminanceBuf == NULL)
			{
				qDebug() << __FUNCTION__ << " line:" << __LINE__ << "_luminanceBuf[dwUser]== NULL";
				return;
			}
			if (luminanceBuf <= pLuminanceProfileArray || (unsigned short*)luminanceBuf >= (WORD*)pLuminanceProfileArray + dwProfileDataCount * dwCount * 2)
				memcpy(luminanceBuf, pLuminanceProfileArray, dwProfileDataCount * dwCount * 2);
			else
				qDebug() << __FUNCTION__ << " line:" << __LINE__ << "memcpy _luminanceBuf error";
		}
		// 扫描的行数，  dwCount 和 yImageSize 一致
		CameraFunSDKfactoryCls* tempsdk = SDKFunc[dwUser];
		//if (tempsdk->allowflag.load(std::memory_order::memory_order_acquire))
		{
			int xDatasize = profileInfo_ptr[dwUser]->wProfileDataCount;

			cv::Mat luminanceMat = cv::Mat(tempsdk->yImageSize, xDatasize, CV_16UC1, luminanceBuf);
			cv::normalize(luminanceMat, luminanceMat, 0, 255, cv::NORM_MINMAX);
			cv::convertScaleAbs(luminanceMat, luminanceMat);
			cv::Mat heightMat = cv::Mat(tempsdk->yImageSize, xDatasize, CV_16UC1, heightBuf);
			if (tempsdk->use_external_batchStart > 0)
			{
				vector<cv::Mat> Getimagevector;
				int realIndex = tempsdk->Currentindex * tempsdk->OnceGetImageNum;
				qDebug() << "callback index" << realIndex;
				Getimagevector.push_back(luminanceMat.clone());
				if (tempsdk->CallbackFuncMap.keys().contains(realIndex))
				{
					qDebug() << "Mat Type" << "luminanceMat" << "out Mat callback" << tempsdk->CallbackFuncMap.keys() << realIndex << tempsdk->getImageMaxCoiunts;
					QObject* obj = tempsdk->CallbackFuncMap.value(realIndex).callbackparent;
					obj->setProperty("cameraIndex", QString::number(realIndex));

					tempsdk->CallbackFuncMap.value(tempsdk->Currentindex).GetimagescallbackFunc(obj, Getimagevector);
				}
				Getimagevector.clear();
				realIndex++;
				Getimagevector.push_back(heightMat.clone());
				//cv::imwrite("D:/callback/" + QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz").toStdString() + ".tiff", heightMat);
				if (tempsdk->CallbackFuncMap.keys().contains(realIndex))
				{
					qDebug() << "Mat Type" << "heightMat" << "out Mat callback" << tempsdk->CallbackFuncMap.keys() << realIndex << tempsdk->getImageMaxCoiunts;
					QObject* obj = tempsdk->CallbackFuncMap.value(realIndex).callbackparent;
					obj->setProperty("cameraIndex", QString::number(realIndex));
					tempsdk->CallbackFuncMap.value(tempsdk->Currentindex).GetimagescallbackFunc(obj, Getimagevector);
				}
			}
			else
			{
				if (tempsdk->allowflag.load(std::memory_order::memory_order_acquire))//软触发插入两次需要调两次data接口
				{
					vector<cv::Mat> Getimagevector;
					Getimagevector.push_back(luminanceMat.clone());
					Getimagevector.push_back(heightMat.clone());
					tempsdk->ImageMats.push(Getimagevector);
				}
				else
				{
					qDebug() << __FUNCTION__ << " line:" << __LINE__ << "allowflag is false" << "Not Allow to getimage";

				}

			}
			/*	if (tempsdk->use_external_batchStart > 0)
				{
					if (tempsdk->CallbackFuncMap.size() > tempsdk->Currentindex)
					{
						qDebug() << "out Mat callback" << tempsdk->CallbackFuncMap.size() << tempsdk->Currentindex<< tempsdk->getImageMaxCoiunts;
						QObject* obj = tempsdk->CallbackFuncMap.value(tempsdk->Currentindex).callbackparent;
						obj->setProperty("cameraIndex", QString::number(tempsdk->Currentindex));
						qDebug() << "Mat size" << Getimagevector.size();
						for (const auto& mat : Getimagevector)
						{
							qDebug() << mat.cols << mat.rows;
						}
						tempsdk->CallbackFuncMap.value(tempsdk->Currentindex).GetimagescallbackFunc(obj, Getimagevector);
					}

				}
				else
				{
					if (tempsdk->allowflag.load(std::memory_order::memory_order_acquire))
						tempsdk->ImageMats.push(Getimagevector);
				}*/

			tempsdk->Currentindex++;
			if (tempsdk->Currentindex >= tempsdk->getImageMaxCoiunts / tempsdk->OnceGetImageNum)	tempsdk->Currentindex = 0;
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << " success to acquire 3d image! camera_name: " << dwUser << tempsdk->Currentindex;

		}
		
		if (luminanceBuf)
		{
			free(luminanceBuf);
			luminanceBuf = NULL;
		}
		if (heightBuf)
		{
			free(heightBuf);
			heightBuf = NULL;
		}
		//_imageAvailable[dwUser] = 1;                 // 图像获取成功  dwUser  和 deviceId   一致
		_lastImageSizeHeight[dwUser] = dwCount;
	}
	catch (std::exception e)
	{
		qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  --> execute callback function error";
	}
}

int CameraFunSDKfactoryCls::LJXA_ACQ_OpenDevice(int lDeviceId, LJX8IF_ETHERNET_CONFIG* EthernetConfig, int HighSpeedPortNo)
{
	int errCode = LJX8IF_EthernetOpen(lDeviceId, EthernetConfig);

	_ethernetConfig[lDeviceId] = *EthernetConfig;
	HighSpeedPortNo = HighSpeedPortNo;

	qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Open device ! errCode:" << errCode;
	return errCode;
}

void CameraFunSDKfactoryCls::LJXA_ACQ_CloseDevice(int lDeviceId)
{
	LJX8IF_FinalizeHighSpeedDataCommunication(lDeviceId);
	LJX8IF_CommunicationClose(lDeviceId);
	qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Close device!";
}

Hd_CameraModule_3DKeyence3::Hd_CameraModule_3DKeyence3(int DevicedID, QString ip, QString RootPath, int settype, QObject* parent) :
	PbGlobalObject(settype, parent), deviceId(DevicedID), RootPath(RootPath), ip(ip)
{

	famliy = PGOFAMLIY::CAMERA3D;
	char pControllerSerialNo[20]; char pHeadSerialNo[20];
	LJX8IF_GetSerialNumber(DevicedID, pControllerSerialNo, pHeadSerialNo);
	SnName = pHeadSerialNo;
	JsonFile = RootPath + SnName + ".json";
	QString FirstCreateByte(R"({"DeviceId": ")" + QString::number(DevicedID) + R"(",
	"GetOnceImageTimes": "10000",
	"Ip": ")" + ip + R"(",
	"Port": "24691",
	"xImageSize": "3200",
	"yImageSize": "1000",
	"y_pitch_um": "20.0",
	"OnceImageCounts":"2",
	"use_external_batchStart":"1",
	"OnceSignalsGetImageCounts":"6"})");
	if (!QFile(JsonFile).exists())
		createAndWritefile(JsonFile, FirstCreateByte.toUtf8());
	QJsonObject paramObj = load_JsonObjectFile(JsonFile);
	for (auto objStr : paramObj.keys())
	{
		ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
	}
	m_sdkFunc = new CameraFunSDKfactoryCls(DevicedID, RootPath, this);
	SDKFunc[deviceId] = m_sdkFunc;
	connect(m_sdkFunc, &CameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });

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
	return  m_sdkFunc->ParasValueMap;
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
	if (flag)
	{
		emit trigged(0);
	}
	else
	{
		emit trigged(1);
		//return false;
	}
	qDebug() << m_sdkFunc->use_external_batchStart;
	if (m_sdkFunc->getTrigger())
	{
		type1 = 0;
		type2 = 0;//不需要需要触发器，外部设置轴到某个位置通过io点触发或者编码器触发，有plc端控制
	}
	else
	{
		type1 = 1;
		type2 = 1;//软触发需要等待内部打开批处理成功后方可进行扫描，需要触发器
	}
	qDebug() << deviceId << SnName;
	return flag;
}

bool Hd_CameraModule_3DKeyence3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
	if (mats.empty() && data.isEmpty())//外部调用只做触发
	{
		int errCode;
		errCode = LJX8IF_StartMeasure(deviceId);//开始批处理

		if (errCode == 0)
		{
			emit trigged(501);
			return true;
		}

		else
		{
			qWarning() << "LJX8IF_StartMeasure start error" << errCode;
			return false;
		}
		//return m_sdkFunc->run();
	}
	return true;
}
//获取数据
bool Hd_CameraModule_3DKeyence3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
	m_sdkFunc->ImageMats.wait_for_pop(m_sdkFunc->timeout_ms, ImgS);
	//ImgS = std::move(Imgout);
	if (ImgS.empty())
	{
		ImgS.push_back(cv::Mat::zeros(100, 100, 0));
		qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
		return false;
	}
	return true;
}

bool CameraFunSDKfactoryCls::InitHighSpeed()
{
	try
	{
		if (startReq_ptr == nullptr)
		{
			startReq_ptr[deviceId] = new LJX8IF_HIGH_SPEED_PRE_START_REQ;
			startReq_ptr[deviceId]->bySendPosition = 2;
			qWarning() << __FUNCTION__ << "  line: " << __LINE__ << " startReq_ptr is null! ";
		}
		if (profileInfo_ptr == nullptr)
		{
			profileInfo_ptr[deviceId] = new LJX8IF_PROFILE_INFO;
			qWarning() << __FUNCTION__ << "  line: " << __LINE__ << " profileInfo_ptr is null! ";
		}
		errCode = LJX8IF_InitializeHighSpeedDataCommunicationSimpleArray(deviceId, &_ethernetConfig[deviceId], HighSpeedPortNo, &myCallbackFunc, yImageSize, deviceId); // 初始化高速通讯
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   errCode:" << errCode;
		errCode = LJX8IF_PreStartHighSpeedDataCommunication(deviceId, startReq_ptr[deviceId], profileInfo_ptr[deviceId]);              // 预开始启动高速通信，作用 做预连接
		//qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   errCode:" << errCode;

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

void CameraFunSDKfactoryCls::upDateParam()
{
	timeout_ms = ParasValueMap.value("GetOnceImageTimes").toInt();
	QStringList IpList = ParasValueMap.value("Ip").split('.');
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
	use_external_batchStart = ParasValueMap.value("use_external_batchStart").toInt();
	getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts").toInt();
	OnceGetImageNum = ParasValueMap.value("OnceImageCounts").toInt();
	return;
}

bool CameraFunSDKfactoryCls::run()
{
	if (!isopen)
	{
		qCritical() << __FUNCTION__ << " line:" << __LINE__ << " device is not opened! ";
		return false;
	}

	try
	{
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
	if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
		return false;
	if (TotalMap.keys().contains(name.split(':').first())) return true;
	int indexSn = -1;
	if (name.endsWith("old"))
	{
		getCameraSnList();//后面创建重新查询

	}
	for (int i = 0; i < TotalSnIpVec.size(); i++)
	{
		if (TotalSnIpVec.at(i).first == DeviceSn)
			indexSn = i;
	}
	if (indexSn == -1)
	{
		qCritical() << "NOT Found Device ID" << DeviceSn;
		return false;
	}
	qDebug() << DeviceSn << name << path;
	OnePb temp;
	temp.base = new Hd_CameraModule_3DKeyence3(indexSn, TotalSnIpVec.at(indexSn).second, path + "/Hd_CameraModule_3DKeyence3/");
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
	QVector<QString > resVec;
	QStringList list = getConnectedDevicesFromARP();
	int index = 0;
	TotalSnIpVec.clear();
	for (auto str : list)
	{
		LJX8IF_ETHERNET_CONFIG EthernetConfig;
		EthernetConfig.abyIpAddress[0] = str.split('.').at(0).toInt();
		EthernetConfig.abyIpAddress[1] = str.split('.').at(1).toInt();
		EthernetConfig.abyIpAddress[2] = str.split('.').at(2).toInt();
		EthernetConfig.abyIpAddress[3] = str.split('.').at(3).toInt();
		EthernetConfig.wPortNo = 24691;
		int errCode = LJX8IF_EthernetOpen(index, &EthernetConfig);
		if (errCode == 0)
		{
			resVec.push_back(str);
			if (resVec.size() >= MAX_LJXA_DEVICENUM) break;
			index++;
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Open device ! errCode:" << errCode;
		}


	}
	for (int o = 0; o < resVec.size(); o++)
	{
		//if (resVec.at(o) == 0)
		{
			char pControllerSerialNo[20]; char pHeadSerialNo[20];
			LJX8IF_GetSerialNumber(o, pControllerSerialNo, pHeadSerialNo);
			//qDebug() << sizeof(pControllerSerialNo);
			//if (sizeof(pControllerSerialNo)!=20 || sizeof(pControllerSerialNo) != 20)
				//continue;
			qDebug() << "Get Device" << "index" << o << "\t" << "pControllerSerialNo" << pControllerSerialNo << "pHeadSerialNo" << pHeadSerialNo;
			QPair<QString, QString> tempPair;
			tempPair.first = pHeadSerialNo;
			tempPair.second = resVec.at(o);
			TotalSnIpVec.push_back(tempPair);
			temp << pHeadSerialNo;
		}
	}
	qDebug() << TotalSnIpVec;
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

mPrivateWidget::mPrivateWidget(void* handle)
{
	m_Camerahandle = reinterpret_cast<Hd_CameraModule_3DKeyence3*>(handle);
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
	connect(OpenGrapMat, &QPushButton::clicked, this, [=]() {emit m_Camerahandle->trigged(1000); });
	connect(NotGrapMat, &QPushButton::clicked, this, [=]() {emit m_Camerahandle->trigged(1001); });
	connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;  QStringList list;
		emit m_Camerahandle->trigged(1000);
		m_Camerahandle->setData(mats, list);
		m_Camerahandle->data(mats, list);
		cv::Mat tempMat = mats.at(0);
		m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
		});
	mainHboxLayout->addLayout(MainLayout, 4);
	mainHboxLayout->addWidget(m_AlgParmWidget, 3);

	/*connect(m_Camerahandle, &Hd_CameraModule_3DKeyence3::sendMats, this, [=](cv::Mat getMat) {

		m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(getMat)));
		});*/
}