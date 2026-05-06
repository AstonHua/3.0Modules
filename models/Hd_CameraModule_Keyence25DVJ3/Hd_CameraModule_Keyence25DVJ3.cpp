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

CameraFunSDKfactoryCls  GetDeviceInfo("","");
struct OnePb
{
	PbGlobalObject* base = nullptr;
	QWidget* baseWidget = nullptr;
	QString DeviceSn;
};
QMap<QString, OnePb>  TotalMap;

// Camera type


#pragma region Device control
bool CameraFunSDKfactoryCls::Connect(string name)
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
void CameraFunSDKfactoryCls::Disconnect()
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

void CameraFunSDKfactoryCls::setFeatureNodes(const std::string sFeatureName, const int64_t value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	result = kgllStreamFeatureNodes->setIntegerValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}

}
void CameraFunSDKfactoryCls::setIntegerValue(const std::string sFeatureName, const int64_t value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	result = kgllFeatureNodes->setIntegerValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}

}
void CameraFunSDKfactoryCls::setFloatValue(const std::string sFeatureName, const double value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

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
void CameraFunSDKfactoryCls::setBooleanValue(const std::string sFeatureName, const bool value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

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
void CameraFunSDKfactoryCls::setEnumValue(const std::string sFeatureName, std::string value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	result = kgllFeatureNodes->setEnumValue(sFeatureName.c_str(), value.c_str());
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}
}
void CameraFunSDKfactoryCls::setStringValue(const std::string sFeatureName, const std::string value)
{
	KglResult result;
	KglString sConfigurationLastFailureCause;

	result = kgllFeatureNodes->setStringValue(sFeatureName.c_str(), value.c_str());
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : setParam\n");
		return;
	}
}
void CameraFunSDKfactoryCls::executeCommand(const std::string sFeatureName)
{
	KglResult result;
	result = kgllFeatureNodes->executeCommand(sFeatureName.c_str());
	if (KGL_SUCCESS != result)
	{
		qDebug() << "(2.5Dmodel) " << ("Error : execCommand\n");
		return;
	}
}

void CameraFunSDKfactoryCls::getIntegerValue(const std::string sFeatureName, int64_t& value)
{
	KglResult result;

	result = kgllFeatureNodes->getIntegerValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}
void CameraFunSDKfactoryCls::getFloatValue(const std::string sFeatureName, double& value)
{
	KglResult result;

	result = kgllFeatureNodes->getFloatValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}

void CameraFunSDKfactoryCls::getBooleanValue(const std::string sFeatureName, bool& value)
{
	KglResult result;

	result = kgllFeatureNodes->getBooleanValue(sFeatureName.c_str(), value);
	if (KGL_SUCCESS != result)
	{
		qDebug() << ("Error : getParam\n");
		return;
	}
}

void CameraFunSDKfactoryCls::getEnumValue(const std::string sFeatureName, std::string& value)
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

void CameraFunSDKfactoryCls::getStringValue(const std::string sFeatureName, std::string& value)
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

#pragma endregion

#pragma region  Import XML 
void CameraFunSDKfactoryCls::ImportDeviceParameters(std::string sFilePath)
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
std::vector<std::string> CameraFunSDKfactoryCls::GetEnableImageType(std::string sImageType)
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

void CameraFunSDKfactoryCls::ConvertRGBA8toBGRA8(void* buffer, int pixelSize)
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

bool CameraFunSDKfactoryCls::AcquisitionStart()
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

void CameraFunSDKfactoryCls::AcquisitionStop()
{
	KglResult result;

	result = kglStream->stopAcquisition(*kglDevice);
	if (KGL_SUCCESS != result) {
		qDebug() << ("Error : acquisitionStop\n");
		return;
	}
}

bool CameraFunSDKfactoryCls::QueueBuffer(KglBuffer* kglBuffer)
{
	KglResult result;
	KglResult operationResult;

	uint32_t payloadsize = kglDevice->getPayloadSize();
	if (payloadsize == 0)
	{
		qWarning() << ("Error : getPayloadSize\n");
		return false;
	}

	kglBuffer = new KglBuffer();
	result = kglBuffer->allocate(payloadsize);
	if (KGL_SUCCESS != result)
	{
		qWarning() << ("Error : allocate\n");
		return false;
	}

	result = kglStream->queueBuffer(kglBuffer);
	if ((KGL_SUCCESS != result) && (KGL_PENDING != result))
	{
		qWarning() << ("Error : queueBuffer\n");
		return false;
	}
	return true;
}

cv::Mat CameraFunSDKfactoryCls::RetrieveBuffer(KglBuffer* kglBuffer, cv::Mat& bmpMat)
{
	KglResult result;
	KglResult operationResult;

	result = kglStream->retrieveBuffer(&kglBuffer, operationResult, 10000);
	if ((KGL_SUCCESS != result) || (KGL_SUCCESS != operationResult))
	{
		
		qWarning() << "[retrieveBuffer] " << "result:" << result;
		qWarning() << "[retrieveBuffer] " << "operationResult:" << operationResult;
		if (kglBuffer != NULL)
		{
			kglBuffer->free();//设置成只执行一次
			delete(kglBuffer);
			kglBuffer = NULL;
		}
		qWarning() << ("Error : retrieveBuffer\n");
		return cv::Mat::zeros(500, 500, CV_8UC1);
	}

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
		mat = cv::Mat(height, width, CV_8UC4, data);
		cv::cvtColor(mat, mat, COLOR_BGRA2RGBA);
	}
	if (kglBuffer != NULL)
	{
		kglBuffer->free();
		delete(kglBuffer);
		kglBuffer = NULL;
	}

	mat.copyTo(bmpMat);
	if (data)
	{
		free(data);
	}
	return bmpMat;
}

bool CameraFunSDKfactoryCls::TriggerSoftware()
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

bool CameraFunSDKfactoryCls::AcquisitionStartEx_SingleFrame(bool bMultiCaptureUpdateImage, std::string sMultiCaptureImageType, std::vector<cv::Mat>& matVec)
{
	std::string sPixelFormat;
	setBooleanValue("MultiCaptureUpdateImage", bMultiCaptureUpdateImage);
	setEnumValue("MultiCaptureImageType", sMultiCaptureImageType);

	cv::Mat bmp;
	KglBuffer* kglBuffer = nullptr;
	if (!QueueBuffer(kglBuffer))
		return false;
	AcquisitionStart();

	RetrieveBuffer(kglBuffer, bmp);
	if (!bmp.empty())
	{
		//AcquisitionStop();
		matVec.push_back(bmp);
		return true;
	}
	else
	{
		//AcquisitionStop();
		return false;
	}

}

CameraFunSDKfactoryCls::CameraFunSDKfactoryCls(QString sn,QString path ) :snName(sn.toStdString()),RootPath(path)
{
	MatVecQueue = std::make_shared<ThreadSafeQueue<std::vector<Mat>>>();
}

CameraFunSDKfactoryCls::~CameraFunSDKfactoryCls()
{
	stopbit = true;
	//getpicturethreadqueue->end();
	if (getpicturethread.joinable())
		getpicturethread.join();
	Disconnect();
}

void CameraFunSDKfactoryCls::upDateParam()
{
	GetImageNums = ParasValueMap.value("OneSgnalsGetImageCounts").toInt();
	MaxTimeOut = ParasValueMap.value("OneGetImageTimeOut").toInt();
}

bool CameraFunSDKfactoryCls::initSdk(QMap<QString, QString>& insideValuesMaps)
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
	getpicturethread = std::thread(&CameraFunSDKfactoryCls::getPictureThread, this);
	return true;
}

bool CameraFunSDKfactoryCls::setParamMap(const QMap<QString, QString>& ParasValueMap)
{
	this->ParasValueMap = ParasValueMap; 

	upDateParam();
	
	return true;
}

void CameraFunSDKfactoryCls::getPictureThread()
{
	while (!stopbit)
	{
		if (!ThreadRunningflag)
		{
			Sleep(1);
			continue;
		}
			
		std::vector<cv::Mat> matvec;
		if (AcquisitionStartEx_SingleFrame(true, sImageType[0], matvec) ==true)
		{

			for (int i = 1; i < sImageType.size(); i++)
			{
				if (sImageType[i] != "" && sImageType[i] != sImageType[i - 1])	
				{
					AcquisitionStartEx_SingleFrame(false, sImageType[i], matvec);
				}
			}
			//if (allowflag.load(std::memory_order::memory_order_acquire))
			{
				if (CurrentsTriggerSource == "Software")
				{
					//CallbackFuncVec.at(Currentindex).GetimagescallbackFunc(CallbackFuncVec.at(Currentindex).callbackparent, matvec);
					MatVecQueue->push(matvec);
				}
				else
				{
					int index = Currentindex* matvec.size();

					for (auto& elem : matvec) 
					{
						QList<cv::Mat> dst;
						dst.push_back(std::move(elem)); // 移动而非拷贝，elem 变为有效但未指定的状态
						QObject* obj = CallbackFuncMap.value(index).callbackparent;
						obj->setProperty("cameraIndex", QString::number(index));

						CallbackFuncMap.value(index).GetimagescallbackFunc(obj, dst);
						index++;
					}
		
				}
			
			}
			Currentindex++;
		}
		if (Currentindex >= GetImageNums)
		{
			Currentindex = 0;
			ThreadRunningflag = false;
		}
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
	m_sdkFunc = new CameraFunSDKfactoryCls(SnName, RootPath);
	connect(m_sdkFunc, &CameraFunSDKfactoryCls::trigged, this, [=](int value) {emit trigged(value); });
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
			m_sdkFunc->executeCommand("BufferCaptureAcquisitionStop");
			m_sdkFunc->executeCommand("ImageCaptureBufferClear");
			m_sdkFunc->executeCommand("BufferCaptureAcquisitionStart");
			m_sdkFunc->allowflag.store(true, std::memory_order::memory_order_release);
			m_sdkFunc->ThreadRunningflag = true;
			emit trigged(501);
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
	m_sdkFunc->CallbackFuncMap.insert(getString.toInt(), TempPack);
	qDebug() << m_sdkFunc << "registerCallBackFun" << getString;
}
void Hd_25DCameraVJ_module::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* parent, const QString& getString)
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
	//temp.base->registerCallBackFun(GetCallbackMat, temp.baseWidget,"0");
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
