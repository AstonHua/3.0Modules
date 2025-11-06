#include "Hd_CameraModule_Keyence25DVJ3.h"
#include <QDebug>
#include <QQueue>
#include <QTextCodec>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>
#include <QMap>
#include <QDir>
#pragma execution_character_set("utf-8")
const QByteArray FirstCreateByte
(R"({"OneSgnalsGetImageCounts": "1",
"SeralNum": "",
"OneGetImageTimeOut": "",
"LastUpdateTime": "",
"TriggerSource": "hard",
"OnceImageCounts":"2"})");
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

cameraFunSDKfactoryCls  GetDeviceInfo("","");
struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;
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

// Camera type


#pragma region Device control
bool cameraFunSDKfactoryCls::Connect(string name)
{
	KglSystem kglSystem;
	const KglDeviceInfo* deviceInfo;
	std::string inputkey;
	if (KGL_SUCCESS != kglSystem.find()) {
		qDebug() << "Failed to find camera device.";
		return false;
	}

	uint32_t count = kglSystem.getDeviceCount();
	if (count == 0)
	{
		qDebug() << ("No camera device can be found.\n");
		return false;
	}
	/*else
	{
		qDebug() <<("Select the camera device number.\n\n");
		for (uint32_t i = 0; i < count; ++i)
		{
			deviceInfo = kglSystem.getDeviceInfo(i);
			sModelName = deviceInfo->getModelName();
			std::string sMACAddress = (std::string)deviceInfo->getMACAddress();
			qDebug() <<("%d : %s_KEYENCE CORPORATION_%s\n", i + 1, sMACAddress.c_str(), sModelName.c_str());
		}
		std::cin >> inputkey;
		qDebug() <<("------------------------------\n");
	}*/

	//uint32_t selectNum = atoi(inputkey.c_str()) - 1;


	kglDevice = new KglDevice();
	kglStream = new KglStream();
	int i = 0;
	for (; i < count; i++)
	{
		deviceInfo = kglSystem.getDeviceInfo(i);
		string tempname;
		//deviceInfo->getSerialNumber();
		tempname = deviceInfo->getSerialNumber();
		if (tempname == name)
			break;
	}
	if (i == count)return false;
	sModelName = deviceInfo->getModelName();
	if ((sModelName == ("CA-HL02MX")) ||
		(sModelName == ("CA-HL04MX")) ||
		(sModelName == ("CA-HL08MX")))
	{
		iCameraType = CameraTypeLine;
	}
	else if ((sModelName == ("XT-024")) ||
		(sModelName == ("XT-060")))
	{
		iCameraType = CameraTypeXT;
	}
	else if ((sModelName == ("RB-500")) ||
		(sModelName == ("RB-800")) ||
		(sModelName == ("RB-1200")))
	{
		iCameraType = CameraTypeRB;
	}
	else {
		iCameraType = CameraTypeArea;
	}

	if (KGL_SUCCESS != kglDevice->connect(deviceInfo->getIPAddress())) {
		qDebug() << ("Cannot connect to the camera device.\n");
		return false;
	}

	if (KGL_SUCCESS != kglStream->open(*kglDevice)) {
		qDebug() << ("Cannot connect to the stream.\n");
		return false;
	}
	// Get FeatureNode of camera device
	kgllFeatureNodes = kglDevice->getDeviceParameters();
	kgllStreamFeatureNodes = kglStream->getStreamParameters();
	return true;
}
void cameraFunSDKfactoryCls::Disconnect()
{
	if ((kglStream != NULL) && (kglStream->isOpen()))
	{
		KglResult result = kglStream->close(*kglDevice);
		if (KGL_SUCCESS != result) {
			qDebug() << ("Cannot disconnect from the stream.\n");
			return;
		}
	}

	if ((kglDevice != NULL) && (kglDevice->isConnected()))
	{
		if (KGL_SUCCESS != kglDevice->disconnect()) {
			qDebug() << ("Cannot disconnect from the camera device.\n");
			return;
		}
	}
}
#pragma endregion

#pragma region Feature access

void cameraFunSDKfactoryCls::setFeatureNodes(const std::string sFeatureName, const int64_t value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	qDebug() << "(2.5Dmodel) " << "[setFeatureNodes] " << sFeatureName.c_str() << value;
	result = kgllStreamFeatureNodes->setIntegerValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}

}
void cameraFunSDKfactoryCls::setIntegerValue(const std::string sFeatureName, const int64_t value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	qDebug() << "(2.5Dmodel) " << "[setIntegerValue] " << sFeatureName.c_str() << value;
	result = kgllFeatureNodes->setIntegerValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}

}
void cameraFunSDKfactoryCls::setFloatValue(const std::string sFeatureName, const double value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	qDebug() << "(2.5Dmodel) " << "[setFloatValue] " << sFeatureName.c_str() << value;
	result = kgllFeatureNodes->setFloatValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		if (KGL_SUCCESS != result)
		{
			qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
			return;
		}
	}
}
void cameraFunSDKfactoryCls::setBooleanValue(const std::string sFeatureName, const bool value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	qDebug() << "(2.5Dmodel) " << "[setBooleanValue] " << sFeatureName.c_str() << value;
	result = kgllFeatureNodes->setBooleanValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		if (KGL_SUCCESS != result)
		{
			qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
			return;
		}
	}
}
void cameraFunSDKfactoryCls::setEnumValue(const std::string sFeatureName, std::string value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	qDebug() << "(2.5Dmodel) " << "[setEnumValue] " << sFeatureName.c_str() << value.c_str();
	result = kgllFeatureNodes->setEnumValue(sFeatureName.c_str(), value.c_str());
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}
}
void cameraFunSDKfactoryCls::setStringValue(const std::string sFeatureName, const std::string value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	qDebug() << "(2.5Dmodel) " << "[setStringValue] " << sFeatureName.c_str() << value.c_str();
	result = kgllFeatureNodes->setStringValue(sFeatureName.c_str(), value.c_str());
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}
}
void cameraFunSDKfactoryCls::executeCommand(const std::string sFeatureName)
{
	KglResult result;
	qDebug() << "(2.5Dmodel) " << "[executeCommand] " << sFeatureName.c_str();
	result = kgllFeatureNodes->executeCommand(sFeatureName.c_str());
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : execCommand\n");
		return;
	}
}

void cameraFunSDKfactoryCls::getIntegerValue(const std::string sFeatureName, int64_t& value)
{
	KglResult result;

	result = kgllFeatureNodes->getIntegerValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}
void cameraFunSDKfactoryCls::getFloatValue(const std::string sFeatureName, double& value)
{
	KglResult result;

	result = kgllFeatureNodes->getFloatValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}

void cameraFunSDKfactoryCls::getBooleanValue(const std::string sFeatureName, bool& value)
{
	KglResult result;

	result = kgllFeatureNodes->getBooleanValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}

void cameraFunSDKfactoryCls::getEnumValue(const std::string sFeatureName, std::string& value)
{
	KglResult result;
	KglString featurevalue;

	result = kgllFeatureNodes->getEnumValue(sFeatureName.c_str(), featurevalue);
	value = featurevalue;

	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}

void cameraFunSDKfactoryCls::getStringValue(const std::string sFeatureName, std::string& value)
{
	KglResult result;
	KglString featurevalue;

	result = kgllFeatureNodes->getStringValue(sFeatureName.c_str(), featurevalue);
	value = featurevalue;
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}
//
//bool setSystemFeatureValue(const std::string sFeatureName, const int64_t value)
//{
//	KglResult result;
//	KglString sConfigurationLastFailureCause;
//
//	qDebug() << ("2.5Dmodel\t") << "[setSystemFeatureValue] " << sFeatureName.c_str() << value;
//	result = kgllFeatureNodes->setIntegerValue(sFeatureName.c_str(), value);
//	if (KGL_SUCCESS != result)
//	{
//		qDebug() << ("2.5Dmodel\t") << ("Error : setParam\n");
//		QString str = QString::fromStdString(sFeatureName) + "_" + QString::number(result, 16);
//		throw exception(str.toStdString().c_str());
//		return;
//	}
//}
#pragma endregion

#pragma region  Import XML 
void cameraFunSDKfactoryCls::ImportDeviceParameters(std::string sFilePath)
{
	setEnumValue("OperationMode", "SetupMode");

	if (!sFilePath.empty())
	{
		KglTargetDataSet target = (KglTargetDataSet)(KglTargetData::CameraParameters)
			| (KglTargetDataSet)(KglTargetData::ParameterSetOfCameraParameter)
			| (KglTargetDataSet)(KglTargetData::ModelImageOfCameraParameter)
			| (KglTargetDataSet)(KglTargetData::CameraParametersOfEnvironmentSettings)
			| (KglTargetDataSet)(KglTargetData::CommonParametersOfEnvironmentSettings)
			| (KglTargetDataSet)(KglTargetData::CommonCameraParameters)
			| (KglTargetDataSet)(KglTargetData::CalibrationOfCameraParameter);
		KglImportDeviceParamOption option = KglImportDeviceParamOption();
		option.bOverwriteModelImage = true;
		option.bOverwriteCalibration = true;
		KglString failFeature;

		KglResult ret = kglDevice->importDeviceParameters(sFilePath.c_str(), failFeature, target, option);
		if (ret != KGL_SUCCESS)
		{
			qDebug() << ("Failed to load a Feature List file.\n");
			return;
		}
	}

	setEnumValue("OperationMode", "RunMode");
}
#pragma endregion

#pragma region Optional Functions
std::vector<std::string> cameraFunSDKfactoryCls::GetEnableImageType(std::string sImageType)
{
	std::vector<std::string> EnableImageType;

	std::string StdImageTypeTable[1][2] =
	{
		{ "StdNormalImageEnable",       "StdNormalImage" }
	};

	std::string LtrxNmlImageTypeTable[15][2] =
	{
		{ "LtrxNormalImageEnable",      "LtrxNormalImage" },
	{ "LtrxUpperImageEnable",       "LtrxUpperImage" },
	{ "LtrxUpperRightImageEnable",  "LtrxUpperRightImage" },
	{ "LtrxRightImageEnable",       "LtrxRightImage" },
	{ "LtrxLowerRightImageEnable",  "LtrxLowerRightImage" },
	{ "LtrxLowerImageEnable",       "LtrxLowerImage" },
	{ "LtrxLowerLeftImageEnable",   "LtrxLowerLeftImage" },
	{ "LtrxLeftImageEnable",        "LtrxLeftImage" },
	{ "LtrxUpperLeftImageEnable",   "LtrxUpperLeftImage" },
	{ "LtrxShapeImage1Enable",      "LtrxShapeImage1" },
	{ "LtrxShapeImage2Enable",      "LtrxShapeImage2" },
	{ "LtrxShapeImage3Enable",      "LtrxShapeImage3" },
	{ "LtrxTextureImageEnable",     "LtrxTextureImage" },
	{ "LtrxGradientXImageEnable",   "LtrxGradientXImage" },
	{ "LtrxGradientYImageEnable",   "LtrxGradientYImage" },
	};

	std::string MlspImageTypeTable[11][2] =
	{
		{ "MlspUVImageEnable",          "MlspUVImage" },
	{ "MlspBlueImageEnable",        "MlspBlueImage" },
	{ "MlspGreenImageEnable",       "MlspGreenImage" },
	{ "MlspAmberImageEnable",       "MlspAmberImage" },
	{ "MlspRedImageEnable",         "MlspRedImage" },
	{ "MlspFarRedImageEnable",      "MlspFarRedImage" },
	{ "MlspIRImageEnable",          "MlspIRImage" },
	{ "MlspWhiteImageEnable",       "MlspWhiteImage" },
	{ "MlspAverageGrayscaleImageEnable",    "MlspAverageGrayscaleImage" },
	{ "MlspColorImageEnable",       "MlspColorImage" },
	{ "MlspColorDifferenceImageEnable",     "MlspColorDifferenceImage" }
	};

	std::string SprfImageTypeTable[16][2] =
	{
		{ "SprfX1ImageEnable",          "SprfX1Image" },
	{ "SprfX2ImageEnable",          "SprfX2Image" },
	{ "SprfX3ImageEnable",          "SprfX3Image" },
	{ "SprfX4ImageEnable",          "SprfX4Image" },
	{ "SprfY1ImageEnable",          "SprfY1Image" },
	{ "SprfY2ImageEnable",          "SprfY2Image" },
	{ "SprfY3ImageEnable",          "SprfY3Image" },
	{ "SprfY4ImageEnable",          "SprfY4Image" },
	{ "SprfSpecularReflectionImageEnable",  "SprfSpecularReflectionImage" },
	{ "SprfDiffuseReflectionImageEnable",   "SprfDiffuseReflectionImage" },
	{ "SprfShapeImage1Enable",      "SprfShapeImage1" },
	{ "SprfShapeImage2Enable",      "SprfShapeImage2" },
	{ "SprfPhaseXImageEnable",      "SprfPhaseXImage" },
	{ "SprfPhaseYImageEnable",      "SprfPhaseYImage" },
	{ "SprfGlossRatioImageEnable",  "SprfGlossRatioImage" },
	{ "SprfNormalImageEnable",      "SprfNormalImage" }
	};

	std::string Cap3DImageTypeTable[2][2] =
	{
		{ "Cap3DAreascan3DImageEnable", "Areascan3DImage" },
	{ "Cap3DAreascan2DImageEnable", "Areascan2DImage" },
	};

	std::string RBImageTypeTable[2][2] =
	{
		{ "Areascan3DImageEnable", "Areascan3DImage" },
	{ "Areascan2DGrayscaleImageEnable", "Areascan2DGrayscaleImage" },
	};

	std::string XTImageTypeTable[3][2] =
	{
		{ "Areascan3DImageEnable", "Areascan3DImage" },
	{ "Areascan2DColorImageEnable", "Areascan2DColorImage" },
	{ "Areascan2DGrayscaleImageEnable", "Areascan2DGrayscaleImage" }
	};

	std::string FilteredImageTable[8][2] =
	{
		{ "FilteredImageEnable",       "FilteredImage1" },
	{ "FilteredImageEnable",       "FilteredImage2" },
	{ "FilteredImageEnable",       "FilteredImage3" },
	{ "FilteredImageEnable",       "FilteredImage4" },
	{ "FilteredImageEnable",       "FilteredImage5" },
	{ "FilteredImageEnable",       "FilteredImage6" },
	{ "FilteredImageEnable",       "FilteredImage7" },
	{ "FilteredImageEnable",       "FilteredImage8" }
	};

	int64_t sTargetParameterSet;
	getIntegerValue("TargetParameterSet", sTargetParameterSet);
	setIntegerValue("ParameterSetSelector", sTargetParameterSet);

	if (iCameraType == CameraTypeLine)
	{
		std::string sCaptureMode;
		getEnumValue("CaptureMode", sCaptureMode);
		if (sCaptureMode == "Fixed_MultipleImages")
		{
			bool sEnable;
			getBooleanValue("IndividualFilterPerFixedCaptureNoEnable", sEnable);
			if (sEnable)
			{
				int64_t sTargetCaptureCount;
				getIntegerValue("TargetFixedCaptureNo", sTargetCaptureCount);
				setIntegerValue("FixedCaptureNoSelector", sTargetCaptureCount);
			}
		}
	}

	bool sEnable;
	if (iCameraType == CameraTypeRB)
	{
		for (UINT i = 0; i < 2; i++)
		{
			std::string sFeatureName = RBImageTypeTable[i][0];
			std::string sImageName = RBImageTypeTable[i][1];
			getBooleanValue(sFeatureName, sEnable);
			if (sEnable)
			{
				EnableImageType.push_back(sImageName);
			}
		}
	}
	else if (iCameraType == CameraTypeXT)
	{
		for (UINT i = 0; i < 3; i++)
		{
			std::string sFeatureName = XTImageTypeTable[i][0];
			std::string sImageName = XTImageTypeTable[i][1];
			getBooleanValue(sFeatureName, sEnable);
			if (sEnable)
			{
				EnableImageType.push_back(sImageName);
			}
		}
	}
	else
	{
		std::string sLightingMode = "";
		getEnumValue("ImagingMode", sLightingMode);

		if (sLightingMode == "StandardLighting")
		{
			for (UINT i = 0; i < 1; i++)
			{
				std::string sFeatureName = StdImageTypeTable[i][0];
				std::string sImageName = StdImageTypeTable[i][1];
				getBooleanValue(sFeatureName, sEnable);
				if (sEnable)
				{
					EnableImageType.push_back(sImageName);
				}
			}
		}
		else if (sLightingMode == "LumiTrax")
		{
			for (UINT i = 0; i < 15; i++)
			{
				std::string sFeatureName = LtrxNmlImageTypeTable[i][0];
				std::string sImageName = LtrxNmlImageTypeTable[i][1];
				getBooleanValue(sFeatureName, sEnable);
				if (sEnable)
				{
					EnableImageType.push_back(sImageName);
				}
			}
		}
		else if (sLightingMode == "MultiSpectrum")
		{
			for (UINT i = 0; i < 11; i++)
			{
				std::string sFeatureName = MlspImageTypeTable[i][0];
				std::string sImageName = MlspImageTypeTable[i][1];
				getBooleanValue(sFeatureName, sEnable);
				if (sEnable)
				{
					EnableImageType.push_back(sImageName);
				}
			}
		}
		else if (sLightingMode == "LumiTraxSpecularReflection")
		{
			for (UINT i = 0; i < 16; i++)
			{
				std::string sFeatureName = SprfImageTypeTable[i][0];
				std::string sImageName = SprfImageTypeTable[i][1];
				getBooleanValue(sFeatureName, sEnable);
				if (sEnable)
				{
					EnableImageType.push_back(sImageName);
				}
			}
		}
		else if (sLightingMode == "Capture3D")
		{
			for (UINT i = 0; i < 2; i++)
			{
				std::string sFeatureName = Cap3DImageTypeTable[i][0];
				std::string sImageName = Cap3DImageTypeTable[i][1];
				getBooleanValue(sFeatureName, sEnable);
				if (sEnable)
				{
					EnableImageType.push_back(sImageName);
				}
			}
		}
	}

	for (UINT i = 0; i < 8; i++)
	{
		setIntegerValue("FilteredImageSelector", (i + 1));

		std::string sFeatureName = FilteredImageTable[i][0];
		std::string sImageName = FilteredImageTable[i][1];
		getBooleanValue(sFeatureName, sEnable);
		if (sEnable)
		{
			EnableImageType.push_back(sImageName);
		}
	}

	UINT ImageTypeSize = (UINT)EnableImageType.size();
	/*if (ImageTypeSize < 5)
	{
		for (int i = ImageTypeSize; i < 5; i++)
		{
			EnableImageType.push_back("");
		}
	}*/

	qDebug() << ("Select an image type.\n\n");

	for (UINT i = 0; i < ImageTypeSize; ++i)
	{
		qDebug() << ((std::to_string(i + 1) + " :" + EnableImageType[i] + "\n").c_str());
	}

	uint32_t iSelected = atoi(sImageType.c_str());
	if ((iSelected == 0) || (iSelected > ImageTypeSize))
	{
		qDebug() << ("The entry is invalid.\n");
		return EnableImageType;
	}
	return EnableImageType;
}

void cameraFunSDKfactoryCls::ConvertRGBA8toBGRA8(void* buffer, int pixelSize)
{
	byte dataR;
	byte dataB;

	for (int i = 0; i < pixelSize; i++)
	{
		dataR = *((byte*)(buffer)+(i * 4));
		dataB = *((byte*)(buffer)+(i * 4) + 2);

		*((byte*)(buffer)+(i * 4)) = dataB;
		*((byte*)(buffer)+(i * 4) + 2) = dataR;
	}
}
#pragma endregion

#pragma region Acquisition control

bool cameraFunSDKfactoryCls::AcquisitionStart()
{
	KglResult result;

	result = kglStream->startAcquisition(*kglDevice);
	if (KGL_SUCCESS != result) {
		qDebug() << ("Error : acquisitionStart\n");
		return false;
	}

	qDebug() << "[AcquisitionStart] " << result;
	return true;
}

void cameraFunSDKfactoryCls::AcquisitionStop()
{
	KglResult result;

	result = kglStream->stopAcquisition(*kglDevice);
	if (KGL_SUCCESS != result) {
		qDebug() << ("Error : acquisitionStop\n");
		return;
	}
}

bool cameraFunSDKfactoryCls::QueueBuffer()
{
	KglResult result;
	KglResult operationResult;

	uint32_t payloadsize = kglDevice->getPayloadSize();
	if (payloadsize == 0)
	{
		qDebug() << ("Error : getPayloadSize\n");
		return false;
	}

	kglBuffer = new KglBuffer();
	result = kglBuffer->allocate(payloadsize);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : allocate\n");
		return false;
	}

	result = kglStream->queueBuffer(kglBuffer);
	if ((KGL_SUCCESS != result) && (KGL_PENDING != result))
	{
		qDebug() << ("Error : queueBuffer\n");
		return false;
	}
	return true;
}

cv::Mat cameraFunSDKfactoryCls::RetrieveBuffer(std::string imagetype)
{
	KglResult result;
	KglResult operationResult;

	clock_t startc = clock();
	result = kglStream->retrieveBuffer(&kglBuffer, operationResult, MaxTimeOut);
	clock_t endc = clock();
	if ((KGL_SUCCESS != result) || (KGL_SUCCESS != operationResult))
	{
		//qDebug() << "[retrieveBuffer] " << "result:" << result;
		//qDebug() << "[retrieveBuffer] " << "operationResult:" << operationResult;
		if (kglBuffer != NULL)
		{
			kglBuffer->free();//设置成只执行一次
			delete(kglBuffer);
			kglBuffer = NULL;
		}
		//qDebug() << ("Error : retrieveBuffer\n");
		return Mat();
	}
	//qDebug() << "kglStream->retrieveBuffer time " << endc - startc;
	UINT bmpSize = kglBuffer->getAcquiredSize();
	char* data = (char*)malloc(bmpSize);

	CopyMemory((void*)data, kglBuffer->getDataPointer(), bmpSize);

	cv::Mat mat;
	int64_t width;
	int64_t height;
	std::string sPixelFormat;
	getIntegerValue("Width", width);
	getIntegerValue("Height", height);
	getEnumValue("PixelFormat", sPixelFormat);

	if (sPixelFormat == "Mono8")
	{
		mat = cv::Mat(height, width, CV_8UC1, data);
	}

	else if (sPixelFormat == "Mono16")
	{
		mat = cv::Mat(height, width, CV_16UC1, data);
	}

	else if (sPixelFormat == "BGR8Packed")
	{
		mat = cv::Mat(height, width, CV_8UC3, data);
	}

	else if (sPixelFormat == "RGB8Packed")
	{
		mat = cv::Mat(height, width, CV_8UC3, data);
		cv::cvtColor(mat, mat, COLOR_BGR2RGB);

	}
	else if (sPixelFormat == "BGRA8Packed")
	{
		mat = cv::Mat(height, width, CV_8UC4, data);
	}

	else if (sPixelFormat == "RGBA8Packed")
	{
		mat = cv::Mat(height, width, CV_8UC3, data);
		cv::cvtColor(mat, mat, COLOR_BGRA2RGBA);
	}
	//kglBuffer->free();
	if (kglBuffer != NULL)
	{
		kglBuffer->free();
		delete(kglBuffer);
		kglBuffer = NULL;
	}

	cv::Mat bmpMat;
	mat.copyTo(bmpMat);
	if (data)
	{
		free(data);
	}
	qDebug() << " 2.5D get " << QString::fromStdString(imagetype) << " image size:" << bmpMat.cols << "," << bmpMat.rows;
	//imwrite("C:\\pic\\Img" + to_string(imgNum) + ".bmp", bmpMat);
	//imgNum++;
	return bmpMat;
}

void cameraFunSDKfactoryCls::SaveBMP(cv::Mat bmp)
{
	std::string inputkey;
	std::string sFilePath;

	qDebug() << ("Do you want to save the image as a BMP file?\n\n");
	qDebug() << ("1 :Yes\n");
	qDebug() << ("2 : No\n");
	std::cin >> inputkey;
	qDebug() << ("------------------------------\n");
	switch (atoi(inputkey.c_str()))
	{
	case 1:
		qDebug() << ("Enter the full path of the BMP file.\n\n");
		std::cin >> sFilePath;
		qDebug() << ("------------------------------\n");
		break;
	case 2:
		return;
		break;
	default:
		qDebug() << ("The entry is invalid.\n");
		return;
		break;
	}
	try
	{
		/*System::String^ FilePath = gcnew System::String(sFilePath.c_str());
		FileStream^ stream = gcnew FileStream(FilePath, FileMode::Create, FileAccess::Write);
		TiffBitmapEncoder^ encoder = gcnew TiffBitmapEncoder();
		encoder->Compression = TiffCompressOption::None;
		encoder->Frames->Add(BitmapFrame::Create(bmp));
		encoder->Save(stream);*/
		//cv::imwrite(sFilePath, bmp);
	}
	catch (char)
	{
		qDebug() << ("Failed to save the BMP file.\n");
		qDebug() << ("------------------------------");
	}
}

bool cameraFunSDKfactoryCls::TriggerSoftware()
{
	//Sleep(3000);
	/*KglResult result;
	result = kgllFeatureNodes->executeCommand("TriggerSoftware");
	if (KGL_SUCCESS != result) {
		qDebug() << ("Error : TriggerSoftware\n");
		return false;
	}*/
	return true;
}

#pragma endregion

bool cameraFunSDKfactoryCls::AcquisitionStartEx_SingleFrame(bool bMultiCaptureUpdateImage, std::string sMultiCaptureImageType, cv::Mat& result)
{
	{
		setBooleanValue("MultiCaptureUpdateImage", bMultiCaptureUpdateImage);
		setEnumValue("MultiCaptureImageType", sMultiCaptureImageType);

		if (!QueueBuffer())
		{
			return false;
		}
		AcquisitionStart();
	}
	cv::Mat bmp;
	clock_t startc = clock();
	bmp = RetrieveBuffer(sMultiCaptureImageType);
	clock_t endc = clock();
	//qDebug() << " retrievetime " << endc - startc;
	if (!bmp.empty())
	{
		result = bmp;
		AcquisitionStop();
		return true;
	}
	else
	{
		AcquisitionStop();
		return false;
	}
}

cameraFunSDKfactoryCls::cameraFunSDKfactoryCls(QString sn,QString path ) :snName(sn.toStdString()),RootPath(path)
{
	MatVecQueue = std::make_shared<ThreadSafeQueue<std::vector<Mat>>>();
}

cameraFunSDKfactoryCls::~cameraFunSDKfactoryCls()
{
	stopbit = true;
	//getpicturethreadqueue->end();
	if (getpicturethread.joinable())
		getpicturethread.join();
	Disconnect();
}

void cameraFunSDKfactoryCls::upDateParam()
{
	GetImageNums = ParasValueMap.value("OneSgnalsGetImageCounts").toInt();
	MaxTimeOut = ParasValueMap.value("OneGetImageTimeOut").toInt();
}

bool cameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
{
	//QString jsonpath = insideValuesMaps["jsonPath"];
	//QString json_cfg_file_path = jsonpath + "/camera_Example.json";
	//QJsonObject obj = GetJsonObject(json_cfg_file_path);
	//string snName = obj["SN"].toString().toLocal8Bit().toStdString();
	bool connectSuccess = Connect(snName);
	if (connectSuccess == false)
		return false;
	ab = new LocalKglDeviceEventSink();
	kglDevice->registerEventSink(ab);
	setBooleanValue("GevGVCPPendingAck", TRUE);
	//ImportDeviceParameters(xmlPath);
	//setEnumValue("AcquisitionMode", "SingleFrame");
	setIntegerValue("GevHeartbeatTimeout", 10000);
	setFeatureNodes("MaximumPendingResends", 0);
	setFeatureNodes("MaximumResendGroupSize", 0);//解决缺图问题26
	setFeatureNodes("ResetOnIdle", 0);
	setFeatureNodes("RequestTimeout", 0);
	setFeatureNodes("MaximumResendRequestRetryByPacket", 0);
	//setSystemFeatureValue("MaximumPendingResends", 0);
	//setSystemFeatureValue("MaximumResendGroupSize", 0);//解决缺图问题26
	//setSystemFeatureValue("ResetOnIdle", 0);
	//setSystemFeatureValue("RequestTimeout", 0);
	//setSystemFeatureValue("MaximumResendRequestRetryByPacket", 0);
	//setIntegerValue("GevSCPSPacketsize", 7716);//解决图像横纹问题,减小cpu使用率
	//setIntegerValue("GevSCPD", 344);//减小延迟


	getBooleanValue("ImageCaptureBufferEnable", bBGEnable);
	if (bBGEnable)
	{
		executeCommand("ImageCaptureBufferClear");
		executeCommand("BufferCaptureAcquisitionStart");
		isBufferCapturestart = true;
	}
	std::string sImageType1 = "1";
	sImageType = GetEnableImageType(sImageType1); //获取图像类别 6张

	std::string sTriggerMode;
	getEnumValue("TriggerMode", sTriggerMode);
	std::string sTriggerSource = "";
	if (sTriggerMode == "On")
	{
		getEnumValue("TriggerSource", sTriggerSource);
		CurrentsTriggerSource = sTriggerSource;
		qDebug() << __FUNCTION__ << __LINE__ << "sTriggerSource:" << QString::fromStdString(sTriggerSource);
	}
	else
		qDebug() << __FUNCTION__ << __LINE__ << "hardTrigger";
	insideValuesMaps["OnceImageCounts"] = QString::number(sImageType.size());
	insideValuesMaps["ImageIndexType"] = QString::number(sImageType.size());
	insideValuesMaps["TriggerSource"] = QString::fromStdString(CurrentsTriggerSource);
	getpicturethread = std::thread(&cameraFunSDKfactoryCls::getPictureThread, this);
	return true;
}

bool cameraFunSDKfactoryCls::setParamMap(const QMap<QString, QString>& ParasValueMap)
{
	this->ParasValueMap = ParasValueMap; 

	upDateParam();
	
	return true;
}

void cameraFunSDKfactoryCls::getPictureThread()
{
	while (!stopbit)
	{
		if (!ThreadRunningflag)
		{
			Sleep(1);
			continue;
		}
			
		std::vector<cv::Mat> matvec;
		cv::Mat image1;
		if (AcquisitionStartEx_SingleFrame(true, sImageType[0], image1))
		{
			matvec.push_back(image1);
			for (int i = 1; i < sImageType.size(); i++)
			{
				cv::Mat image2;
				if (AcquisitionStartEx_SingleFrame(false, sImageType[i], image2))
				{
					matvec.push_back(image2);
				}
			}
			if (allowflag.load(std::memory_order::memory_order_acquire))
			{
				if (CurrentsTriggerSource == "Software")
				{
					//CallbackFuncVec.at(Currentindex).GetimagescallbackFunc(CallbackFuncVec.at(Currentindex).callbackparent, matvec);
					MatVecQueue->push(matvec);
				}
				else
				{
					CallbackFuncVec.at(Currentindex).GetimagescallbackFunc(CallbackFuncVec.at(Currentindex).callbackparent, matvec);
				}
			
			}
			Currentindex++;
		}
		if (GetImageNums == Currentindex)ThreadRunningflag=false;
	}
}

Hd_25DCameraVJ_module::Hd_25DCameraVJ_module(QString SnName,QString path,int settype, QObject* parent) : 
	PbGlobalObject(settype, parent),SnCode(SnName),RootPath(path)
{
	famliy = PGOFAMLIY::CAMERA2_5D;
	//RootPath = RootPath + "/Hd_CameraModule_3DKeyence3/";
	JsonFilePath = RootPath + SnName + ".json";
	if (!QFile(JsonFilePath).exists())
		createAndWritefile(JsonFilePath, FirstCreateByte);
	QJsonObject paramObj = load_JsonFile(JsonFilePath);
	for (auto objStr : paramObj.keys())
	{
		ParasValueMap.insert(objStr, paramObj.value(objStr).toString());
	}
	m_sdkFunc = new cameraFunSDKfactoryCls(SnName, RootPath);
	connect(m_sdkFunc, &cameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });
}

Hd_25DCameraVJ_module::~Hd_25DCameraVJ_module()
{
	if (m_sdkFunc)
	{
		delete m_sdkFunc;
		m_sdkFunc = nullptr;
	}
	qDebug() << "[info]" << __FUNCTION__ << "line:" << __LINE__ << " delete success!";
}

bool Hd_25DCameraVJ_module::data(std::vector<cv::Mat>& outmats, QStringList& outdata)
{
	//qDebug() << m_sdkFunc->MaxTimeOut<< outmats.size();
	auto ret = m_sdkFunc->MatVecQueue->wait_for_pop(m_sdkFunc->MaxTimeOut, outmats);

	if (outmats.size() != m_sdkFunc->sImageType.size() || ret == false)
	{
		cv::Mat mat = cv::Mat::zeros(100,100,0);
		outmats.push_back(mat);
		qCritical() << __FUNCTION__ << "   line:" << __LINE__ << " srcImage is null";
		return false;
	}
	return true;
}

bool Hd_25DCameraVJ_module::setData(const std::vector<cv::Mat>& mat, const QStringList& data)
{
	Q_UNUSED(mat);
	if (mat.empty() && data.isEmpty())
	{
		bool flag = m_sdkFunc->TriggerSoftware();
		emit trigged(501);
		return flag;
	}

	return true;
}

bool Hd_25DCameraVJ_module::init()
{
	connect(this, &PbGlobalObject::trigged, [=](int Code) {
		if (Code == 1000)
		{
			m_sdkFunc->Currentindex = 0;
			m_sdkFunc->MatVecQueue->clear();
			m_sdkFunc->allowflag.store(true, std::memory_order::memory_order_release);
			m_sdkFunc->ThreadRunningflag = true;
		}
		else if (Code == 1001)
		{
			m_sdkFunc->allowflag.store(false, std::memory_order::memory_order_release);
			m_sdkFunc->ThreadRunningflag = false;
		}
		});
	bool flag = m_sdkFunc->initSdk(ParasValueMap);
	if (flag)
	{
		if (m_sdkFunc->CurrentsTriggerSource == "Software")
		{
			type1 = 1;
		}
		else
		{
			type1 = 0;
		}
		emit trigged(0);
	}
	else
	{
		emit trigged(1);

	}

	return flag;
}

bool Hd_25DCameraVJ_module::setParameter(const QMap<QString, QString>& insideValuesMaps)
{
	ParasValueMap = insideValuesMaps;
	m_sdkFunc->setParamMap(ParasValueMap);
	return true;
}

QMap<QString, QString> Hd_25DCameraVJ_module::parameters()
{
	return m_sdkFunc->ParasValueMap;
}

void Hd_25DCameraVJ_module::registerCallBackFun(PBGLOBAL_CALLBACK_FUN func, QObject* parent, const QString& getString)
{

	CallbackFuncPack TempPack;
	TempPack.callbackparent = parent;
	TempPack.cameraIndex = getString;
	TempPack.GetimagescallbackFunc = func;
	m_sdkFunc->CallbackFuncVec.append(TempPack);
	qDebug() << getString;
}

LocalKglDeviceEventSink::LocalKglDeviceEventSink()
{

}

LocalKglDeviceEventSink::~LocalKglDeviceEventSink()
{
}

void LocalKglDeviceEventSink::onLinkDisconnected(const KglDevice* pDevice)
{
	//it is required to write action when event received.
	int a = 3;
	a = a + 3;
	return;
}

void LocalKglDeviceEventSink::onEventGenICam(const KglDevice* pDevice, const uint16_t eventID, const uint16_t channel, const uint64_t blockID, const uint64_t timestamp, const uint32_t nodenum, const KglGenParameter** pNode)
{
	//string sEventMessage = "";
	//string sDevicePAddress = pDevice.getiPAdaress()
	//	sEventMessage += "eventID: Ox" + eventID.ToString("X2");

	switch (eventID)
	{
	case 0x9000:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , Trigger";
		//kCamera->EventQueue->push(eventID);
		break;
	}
	case 0x9001:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , TriggerMissed";
		break;
	}
	case 0x9002:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , TriggerWaitStart";
		break;
	}
	case 0x9003:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , TriggerWaitEnd";
		break;
	}
	case 0x9004:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , ExposuerStart";
		break;
	}
	case 0x9005:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , ExposuerEnd";
		break;
	}
	case 0x9006:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , transfer start";
		break;
	}
	case 0x9007:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , transferend: ";
		//kCamera->EventQueue->push(eventID);
		break;
	}
	case 0x9008:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , transfer ready";
		break;
	}
	case 0x900A:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , ImageCaptureBufferFull";
		break;
	}
	case 0x900B:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , ImageCaptureBufferOverflow";
		break;
	}
	case 0x900C:
	{
		//qDebug() << __FUNCTION__ << "line:" << __LINE__ << pDevice << " , ImageCaptureBufferEmpty";
		break;
	}
	default:
		break;
	}
}

bool create(const QString& DeviceSn, const QString& name, const QString& path)
{
	if (DeviceSn.isEmpty() || name.isEmpty() || path.isEmpty())
		return false;
	OnePb temp;
	temp.base = new Hd_25DCameraVJ_module(DeviceSn, path+"/Hd_CameraModule_Keyence25DVJ3/");
	temp.baseWidget = new mPrivateWidget(temp.base);
	temp.base->registerCallBackFun(GetCallbackMat, temp.baseWidget,"0");
	temp.DeviceSn = DeviceSn;
	TotalMap.insert(name.split(':').first(), temp);
	return  temp.base->init();
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
	KglSystem::initialize();
	QStringList temp;
	KglSystem kglSystem;
	if (KGL_SUCCESS != kglSystem.find()) {
		qDebug() << "Failed to find camera device.";
		return temp;
	}

	uint32_t count = kglSystem.getDeviceCount();
	if (count == 0)
	{
		qDebug() << ("No camera device can be found.\n");
		return temp;
	}
	int i = 0;
	for (; i < count; i++)
	{
		string tempStr = kglSystem.getDeviceInfo(i)->getSerialNumber();
		temp << QString::fromStdString(tempStr);
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

mPrivateWidget::mPrivateWidget(void* handle)
{
	m_Camerahandle = reinterpret_cast<Hd_25DCameraVJ_module*>(handle);
	InitWidget();
}

void mPrivateWidget::InitWidget()
{
	QVBoxLayout* MainLayout = new QVBoxLayout(this);

	SetDataBtn = new QPushButton(this);
	SetStopBtn = new QPushButton(this);
	SetDataBtn->setText(tr("start"));
	SetStopBtn->setText(tr("stop"));
	//m_showimage = new ImageViewer(this);
	m_label = new QLabel(this);
	m_label->setFixedSize(800, 640);
	MainLayout->addWidget(m_label);
	MainLayout->addWidget(SetDataBtn);
	MainLayout->addWidget(SetStopBtn);
	connect(SetDataBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;  QStringList list;
		emit m_Camerahandle->trigged(1000);
		m_Camerahandle->setData(mats, list);
		
		//m_Camerahandle->setData(mats, list);
		m_Camerahandle->data(mats, list);
		cv::Mat tempMat = mats.at(0);
		//m_showimage->loadImage(QPixmap::fromImage(cvMatToQImage(tempMat)));
		m_label->setPixmap(QPixmap::fromImage(cvMatToQImage(tempMat)));
		});
	connect(SetStopBtn, &QPushButton::clicked, this, [=]() {
		std::vector<cv::Mat> mats;  QStringList list;
		emit m_Camerahandle->trigged(1001);
		});
	
}

void GetCallbackMat(QObject* widget, const std::vector<cv::Mat>& Mats)
{
	mPrivateWidget* realWidget = (mPrivateWidget*)widget;
	cv::Mat tempMat = Mats.at(0);
	QImage map = cvMatToQImage(tempMat);
	map.scaled(800, 640);
	realWidget->m_label->setPixmap(QPixmap::fromImage(map));
}
