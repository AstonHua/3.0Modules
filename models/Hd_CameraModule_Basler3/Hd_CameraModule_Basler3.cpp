#include "Hd_CameraModule_Basler3.h"
#include "qfuture.h"
#include <QtConcurrent/QtConcurrent>
#include <QTextCodec>


//判断是否为彩色相机
bool IsColor(EPixelType enType)
{
	switch (enType)
	{
	case PixelType_RGB8packed:
	case PixelType_BayerGR8:
	case PixelType_BayerRG8:
	case PixelType_BayerGB8:
	case PixelType_BayerBG8:
	case PixelType_BayerGR10:
	case PixelType_BayerRG10:
	case PixelType_BayerGB10:
	case PixelType_BayerBG10:
	case PixelType_BayerGR12:
	case PixelType_BayerRG12:
	case PixelType_BayerGB12:
	case PixelType_BayerBG12:
	case PixelType_RGB10packed:
	case PixelType_BGR10packed:
	case PixelType_RGB12packed:
	case PixelType_BGR12packed:
	case PixelType_YUV422_YUYV_Packed:
	case PixelType_BayerGR12Packed:
	case PixelType_BayerRG12Packed:
	case PixelType_BayerGB12Packed:
	case PixelType_BayerBG12Packed:
		return true;
	default:
		return false;
	}
}
//搜索设备
int Hd_CameraModule_Basler3::SearchDevice()
{
	//Pylon::DeviceInfoList_t devices; //还是设置曝光崩溃
 //   try
 //   {
 //       // Get the transport layer factory.
 //       Pylon::CTlFactory& TlFactory = Pylon::CTlFactory::GetInstance();

 //       // Get all attached cameras.
 //       TlFactory.EnumerateDevices( devices );
 //   }


	// Before using any pylon methods, the pylon runtime must be initialized.
	Pylon::PylonAutoInitTerm autoInitTerm;
	// Create GigE transport layer.
	Pylon::CTlFactory& TlFactory = Pylon::CTlFactory::GetInstance();
	Pylon::IGigETransportLayer* pTl = dynamic_cast<Pylon::IGigETransportLayer*>(TlFactory.CreateTl(Pylon::BaslerGigEDeviceClass));
	if (pTl == NULL)
	{
		cerr << "Error: No GigE transport layer installed." << endl;
		cerr << "       Please install GigE support as it is required for this sample." << endl;
		return 1;
	}

	// Enumerate devices.
	//Pylon::DeviceInfoList_t devices;
	pTl->EnumerateAllDevices(m_devices);
	//m_devices = devices;
	int mindex = 0;
	// Print information table.
	for (Pylon::DeviceInfoList_t::const_iterator it = m_devices.begin(); it != m_devices.end(); ++it)
	{
		// Add the friendly name to the list. 
		Pylon::String_t SerialNumber = it->GetSerialNumber();
		std::string s = SerialNumber;
		if (s == Username)
		{
			index = mindex;
			//自动IP
			{
				// Determine current configuration mode.
				Pylon::String_t activeMode;
				if (it->IsPersistentIpActive())
				{
					activeMode = "Static";
				}
				else if (it->IsDhcpActive())
				{
					activeMode = "DHCP";
				}
				else
				{
					activeMode = "AutoIP";
				}

				cout.width(32);
				cout << it->GetFriendlyName();
				cout.width(14);
				cout << it->GetMacAddress();
				cout.width(17);
				cout << it->GetIpAddress();
				cout.width(17);
				cout << it->GetSubnetMask();
				cout.width(15);
				cout << it->GetDefaultGateway();
				cout.width(8);
				cout << activeMode;
				cout.width(4);
				cout << it->IsPersistentIpSupported();
				cout.width(6);
				cout << it->IsDhcpSupported();
				cout.width(5);
				cout << it->IsAutoIpSupported();
				cout << endl;

				// ch:判断设备IP是否可达
				bool bAccessible = true;// = MV_CC_IsDeviceAccessible(m_stDevList.pDeviceInfo[index], MV_ACCESS_Exclusive);
				QStringList nNetExport = QString::fromStdString(std::string(it->GetInterface())).split(".");
				QStringList nCurrentIp = QString::fromStdString(std::string(it->GetIpAddress())).split(".");
				if (nNetExport[0] != nCurrentIp[0] || nNetExport[1] != nCurrentIp[1] || nNetExport[2] != nCurrentIp[2])//nCurrentIp相机IP；nNetExport网口IP;判断前3位
					bAccessible = false;
				if (!bAccessible)
				{
					Pylon::String_t auto_dwIpaddress;
					for (int i = 2; i < 253; i++)//自动IP
					{
						QString qIP = nNetExport[0] + "." + nNetExport[1] + "." + nNetExport[2] + "." + QString::number(i);
						std::string sIp = qIP.toStdString();
						auto_dwIpaddress = sIp.c_str();

						bool ifHas = false;
						for (Pylon::DeviceInfoList_t::const_iterator mit = m_devices.begin(); mit != m_devices.end(); ++mit)
						{
							if (auto_dwIpaddress == mit->GetIpAddress())
							{
								ifHas = true;
								break;
							}
						}
						if (ifHas)
							continue;
					}
					//Pylon::String_t sIP = "192.168.4.53";
					Pylon::String_t subnetMask = "255.255.255.0";
					Pylon::String_t defaultGateway = "0.0.0.0";

					// Check if configuration mode is AUTO, DHCP, or IP address.
					bool isAuto = (strcmp(auto_dwIpaddress, "AUTO") == 0);
					bool isDhcp = (strcmp(auto_dwIpaddress, "DHCP") == 0);
					bool isStatic = !isAuto && !isDhcp;
					// Set new IP configuration.
					bool setOk = pTl->BroadcastIpConfiguration(it->GetMacAddress(), isStatic, isDhcp,
						auto_dwIpaddress, subnetMask, defaultGateway, it->GetUserDefinedName());

					// Show result message.
					if (setOk)
					{
						pTl->RestartIpConfiguration(it->GetMacAddress());
						cout << "Successfully changed IP configuration via broadcast for device " << it->GetMacAddress() << " to " << auto_dwIpaddress << endl;
					}
					else
					{
						cout << "Failed to change IP configuration via broadcast for device " << it->GetMacAddress() << endl;
						cout << "This is not an error. The device may not support broadcast IP configuration." << endl;
					}
				}
			}
			break;
		}
		mindex++;

	}

	return (int)m_devices.size();
}
//回调函数
//当有新图像可用时，从CInstantCamera抓取线程调用此函数。
//在此处执行图像处理。
//之后，我们将把图像转换为RGB位图，并通知GUI关于新图像的信息。
//
//注意：您不能访问任何UI对象，因为此函数不是从GUI线程调用的。
//为了更新GUI，我们将在该功能结束时向主窗口发布一条消息。
//GUI线程将处理消息并更新GUI。
void Hd_CameraModule_Basler3::OnImageGrabbed(Pylon::CInstantCamera& camera, const CGrabResultPtr& grabResult)
{
	n_mutex->lock();
	// The following line is commented out as this function will be called very often
	// filling up the debug output.
	//TRACE(_T("%s\n"), __FUNCTIONW__);
	// When overwriting the current CGrabResultPtr, the previous result will automatically be
	// released and reused by CInstantCamera.
	double time_Start = (double)clock();

	//m_ptrGrabResult = grabResult;
	//CBaslerUniversalGrabResultPtr ptrGrabResult;CGrabResultPtr

	// First check whether the smart pointer is valid.
	// Then call GrabSucceeded() on the CGrabResultData to test whether the grab resulut conatains
	// an sucessfully grabbed image.
	// In case of i.e. transmission errors the result may be invalid
	cv::Mat srcImage = cv::Mat();
	if (grabResult.IsValid() && grabResult->GrabSucceeded())
	{
		// This is where you would do image processing
		// and do other tasks.
		// --- Start of sample image processing --- (only works for 8-bit formats)
		if (m_invertImage && grabResult->GetPixelType() == PixelType_Mono8)
		{
			srcImage = cv::Mat(grabResult->GetHeight(), grabResult->GetWidth(), CV_8UC1, grabResult->GetBuffer());

			//size_t imageSize = Pylon::ComputeBufferSize(grabResult->GetPixelType(), grabResult->GetWidth(), grabResult->GetHeight(), grabResult->GetPaddingX());

			//uint8_t* p = reinterpret_cast<uint8_t*>(grabResult->GetBuffer());
			//uint8_t* const pEnd = p + (imageSize / sizeof(uint8_t));
			//for (; p < pEnd; ++p)
			//{
			//	*p = 255 - *p; //For demonstration purposes only, invert the image.
			//}
		}
		else if (m_invertImage && grabResult->GetPixelType() == PixelType_BGR8packed)
		{
			srcImage = cv::Mat(grabResult->GetHeight(), grabResult->GetWidth(), CV_8UC3, grabResult->GetBuffer());
		}
		else if (IsColor(grabResult->GetPixelType()))
		{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
			// Create a target image.
			CPylonImage targetImage;

			// Create the converter and set parameters.
			CImageFormatConverter converter;
			converter.OutputPixelFormat = PixelType_BGR8packed;
			// Now we can check if conversion is required.
			if (converter.ImageHasDestinationFormat(grabResult))
			{
				;
			}
			else
			{
				converter.Convert(targetImage, grabResult);
			}
			srcImage = cv::Mat(targetImage.GetHeight(), targetImage.GetWidth(), CV_8UC3, targetImage.GetBuffer());
		}
		//++m_cntGrabbedImages;

		ifGetImage = true;
		//if (srcImage.data)
		//	queuepic.push_back(srcImage.clone());
	}
	else
	{
		// If the grab result is invalid, we also mark the bitmap as invalid.
		m_bitmapImage.Release();
		ifGetImage = true;
		// The some TLs provide an error code why the grab failed.
		//TRACE(_T("%s Grab failed. Error code. %x\n"), __FUNCTIONW__, (int)grabResult->GetErrorCode());
		//CSingleLock lockErrorCount(&m_MemberLock, TRUE);
		//++m_cntGrabErrors;
		//lockErrorCount.Unlock();
	}
	if (srcImage.data)
		queuepic.push_back(srcImage.clone());
	else
		queuepic.push_back(cv::Mat(5, 5, CV_8UC1).setTo(0));

	//cv::Mat resizeM;
	//cv::resize(srcImage, resizeM, cv::Size(), 0.1, 0.1);
	//cv::imwrite("../runtime/Bin_DEVICE/testImg/"+Username + ".png", resizeM);

	m_flag = true;
	//callBackNum++;
	QString tmpDate = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss-zzz");
	if (currentTriggerSource == Basler_UniversalCameraParams::TriggerSource_Line1)//硬触发，回调
	{
		std::vector<cv::Mat> mat;
		mat.push_back(queuepic.dequeue());
		//触发回调
		for (int i = 0; i < QQueue_myPBGLOBAL_callBack.size(); i++)
			if (QQueue_myPBGLOBAL_callBack[i].cameraIndex == QString::number(imgIndex))
				(*QQueue_myPBGLOBAL_callBack[i].callBackFun)(QQueue_myPBGLOBAL_callBack[i].m_QObject, mat);
		imgIndex++;
		if (expourseTimeList.size() > 1 && expourseTimeList[imgIndex % expourseTimeList.size()]
			!= expourseTimeList[(imgIndex - 1) % expourseTimeList.size()])
			SetExposureTime(expourseTimeList[imgIndex % expourseTimeList.size()]);
		//Sleep(1);
		if (gainList.size() > 1 && gainList[imgIndex % gainList.size()]
			!= gainList[(imgIndex - 1) % gainList.size()])
			SetGain(gainList[imgIndex % gainList.size()]);

		if (imgIndex >= expourseTimeList.size())
			imgIndex = 0;
	}
	n_mutex->unlock();
	// Tell the main window that there is a new image available so it can update the image window.
	double time_End = (double)clock();
	qDebug() << "[INFO] " << __FUNCTION__ << " line:" << __LINE__ << " checkImg getImg,Time:" << time_End - time_Start << "ms"
		<< "--CameraName:" << QString::fromStdString(Username) << "--imgIndex : " << imgIndex;// << " triggedNum:" << ;
}
//断线回调
// This will be called when the camera has been removed/disconnected.
// We'll post a message to the GUI about this event and return.
// In response to this message, the GUI will be updated and the camera will be closed.
void Hd_CameraModule_Basler3::OnCameraDeviceRemoved(Pylon::CInstantCamera& camera)
{
	moduleStatus = false;
	qDebug() << "[Hd_CameraModule_Basler3] " << " Camera disconnected!  try auto connect";
	
	//运行重连线程
	QFuture<bool> future2 = QtConcurrent::run(this, &Hd_CameraModule_Basler3::reConnctDevice);
}
//OnImageGrabbed和OnCameraDeviceRemoved函数，相机创建时自动绑定；相机释放时，自动解除绑定。
//重连
bool Hd_CameraModule_Basler3::reConnctDevice()
{
	m_camera.StopGrabbing();// Clear the pointers to the features we set manually in Open().
	m_exposureTime.Release();
	m_gain.Release();
	m_camera.DestroyDevice();
	m_camera.Close();
	//delete m_camera;
	//m_camera = nullptr;

	while (1)
	{
		if (ifmoduleRun == false)
			return false;
		Sleep(100);
		//m_mutex->lock();
		if (ConnctDevice() == true)
		{
			qWarning() << "[Hd_CameraModule_Basler3] " << "  Camera create success again! ";
			moduleStatus = true;
			//m_mutex->unlock();
			return true;
		}
	}
}
//连接设备
bool Hd_CameraModule_Basler3::ConnctDevice()
{
	index = -1;
	SearchDevice();
	if (index < 0)
		return false;

	//m_camera = new CBaslerUniversalInstantCamera;

	//因为用的new方式
	{
		// Register this object as an image event handler in order to get notified of new images.
		// See Pylon::CImageEventHandler for details.
		m_camera.RegisterImageEventHandler(this, Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_None);

		// Register this object as a configuration event handler in order to get notified of camera state changes.
		// See Pylon::CConfigurationEventHandler for details.
		m_camera.RegisterConfiguration(new Pylon::CAcquireContinuousConfiguration(), Pylon::RegistrationMode_ReplaceAll, Pylon::Cleanup_Delete);
		m_camera.RegisterConfiguration(this, Pylon::RegistrationMode_Append, Pylon::Cleanup_None);
	}
	// Get the pointer to Pylon::CDeviceInfo selected.
	const Pylon::CDeviceInfo* pDeviceInfo = &m_devices[index];// (const Pylon::CDeviceInfo*)m_cameraBox.GetItemData(index);

	try
	{
		// Add the AutoPacketSizeConfiguration and let pylon delete it when not needed anymore.
		//m_camera.RegisterConfiguration(new CAutoPacketSizeConfiguration(), Pylon::RegistrationMode_Append, Pylon::Cleanup_Delete);

		// Create the device and attach it to CInstantCamera.
		// Let CInstantCamera take care of destroying the device.
		Pylon::IPylonDevice* pDevice = Pylon::CTlFactory::GetInstance().CreateDevice(*pDeviceInfo);
		m_camera.Attach(pDevice, Pylon::Cleanup_Delete);
		if (m_camera.IsPylonDeviceAttached() == false)
			return false;
		// Open camera.
		m_camera.Open(); 
		
		//设置心跳；快速进断线回调函数
		CIntegerParameter heartbeat(m_camera.GetTLNodeMap(), "HeartbeatTimeout");
		heartbeat.TrySetValue(3000, IntegerValueCorrection_Nearest);  // set to 1000 ms timeout if writable

		// Get the ExposureTime feature.
		// On GigE cameras, the feature is called 'ExposureTimeRaw'.
		// On USB cameras, it is called 'ExposureTime'.
		if (m_camera.ExposureTime.IsValid())
		{
			// We need the integer representation because the GUI controls can only use integer values.
			// If it doesn't exist, return an empty parameter.
			m_camera.ExposureTime.GetAlternativeIntegerRepresentation(m_exposureTime);
		}
		else if (m_camera.ExposureTimeRaw.IsValid())
		{
			m_exposureTime.Attach(m_camera.ExposureTimeRaw.GetNode());
		}
		// Get the Gain feature.
		// On GigE cameras, the feature is called 'GainRaw'.
		// On USB cameras, it is called 'Gain'.
		if (m_camera.Gain.IsValid())
		{
			// We need the integer representation for the this sample
			// If it doesn't exist, return an empty parameter.
			m_camera.Gain.GetAlternativeIntegerRepresentation(m_gain);
		}
		else if (m_camera.GainRaw.IsValid())
		{
			m_gain.Attach(m_camera.GainRaw.GetNode());
		}

		// Add event handlers that will be called when the feature changes.

		if (m_exposureTime.IsValid())
		{
			// If we must use the alternative integer representation, we don't know the name of the node as it defined by the camera
			m_camera.RegisterCameraEventHandler(this, m_exposureTime.GetNode()->GetName(), 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
		}

		if (m_gain.IsValid())
		{
			// If we must use the alternative integer representation, we don't know the name of the node as it defined by the camera
			m_camera.RegisterCameraEventHandler(this, m_gain.GetNode()->GetName(), 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
		}
		////设置曝光，GigE相机为ExposureTimeRaw；USB相机为ExposureTime；方法可行,断电不保存
		//if (m_camera.ExposureTime.IsValid())
		//{
		//	m_camera.ExposureTime.GetAlternativeIntegerRepresentation(m_exposureTime);
		//	int64_t myExposureTimeRaw=m_camera.ExposureTimeRaw.GetValue();
		//	m_camera.ExposureTimeRaw.SetValue(20);
		//}
		//else if (m_camera.ExposureTimeRaw.IsValid())
		//{
		//	m_exposureTime.Attach(m_camera.ExposureTimeRaw.GetNode());
		//	m_camera.ExposureTimeRaw.SetValue(20);
		//}

		//线扫相机，设置增益有问题
		////设置增益，GigE相机为GainRaw；USB相机为Gain
		//if (m_camera.Gain.IsValid())
		//{
		//	m_camera.Gain.GetAlternativeIntegerRepresentation(m_gain);
		//	m_camera.Gain.SetValue(2);
		//}
		//else if (m_camera.GainRaw.IsValid())
		//{
		//	m_gain.Attach(m_camera.GainRaw.GetNode());
		//	m_camera.GainRaw.SetValue(2);
		//}

		//这几行会把相机设置重置
		//像素格式
		//m_camera.RegisterCameraEventHandler(this, "PixelFormat", 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
		//触发模式
		//m_camera.RegisterCameraEventHandler(this, "TriggerMode", 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);
		//触发源
		//m_camera.RegisterCameraEventHandler(this, "TriggerSource", 0, Pylon::RegistrationMode_Append, Pylon::Cleanup_None, Pylon::CameraEventAvailability_Optional);

		//触发模式
		m_camera.TriggerSource.SetValue( Basler_UniversalCameraParams::TriggerSource_Software);
		//触发源
		m_camera.TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_On);
		//m_camera.PixelFormat.SetValue();//

		//连续采集
		// Camera may have been disconnected.
		if (!m_camera.IsOpen() || m_camera.IsGrabbing())
		{
			return false;
		}
		// Try set continuous frame mode if available
		m_camera.AcquisitionMode.TrySetValue(Basler_UniversalCameraParams::AcquisitionMode_Continuous);

		// Start grabbing until StopGrabbing() is called.
		m_camera.StartGrabbing(Pylon::GrabStrategy_OneByOne, Pylon::GrabLoop_ProvidedByInstantCamera);
		qDebug() << "[StartGrabbing] " << " basler camera StartGrabbing!";
	}
	catch (const Pylon::GenericException& e)
	{
		qDebug() << "[error] " << " basler camera disconnects!";
		return false;
	}
	return true;
	////创建相机对象（以最先识别的相机）
	////camera = CTlFactory::GetInstance().CreateFirstDevice();
	//CInstantCameraArray cameras(min(devices.size(), c_maxCamerasToUse));
	//// 打印相机的名称
	////std::cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;
	//
	////获取相机节点映射以获得相机参数
	//GenApi::INodeMap& nodemap = camera.GetNodeMap();
	////打开相机
	//camera.Open();
	////加载用户集，相机设置应当userset1
	//camera.UserSetSelector.SetValue(UserSetSelector_UserSet1);
	//camera.RegisterImageEventHandler(new CSampleImageEventHandler, RegistrationMode_Append, Cleanup_Delete);
	//camera.RegisterExceptionCallBack();
	// 设置相机参数
	/*GenApi::cenumerationptr ptracquisitionmode = camera.getparameter("acquisitionmode");
	ptracquisitionmode->fromstring("continuous");

	genapi::cenumerationptr ptrtriggerselector = camera.getparameter("triggerselector");
	ptrtriggerselector->fromstring("framestart");

	genapi::cenumerationptr ptrtriggermode = camera.getparameter("triggermode");
	ptrtriggermode->fromstring("on");

	genapi::cenumerationptr ptrtriggersource = camera.getparameter("triggersource");
	ptrtriggersource->fromstring("software");

	CIntegerParameter heartbeat( camera.GetTLNodeMap(), "HeartbeatTimeout" );
    heartbeat.TrySetValue( 1000, IntegerValueCorrection_Nearest );
	*/
}
//错误
void Hd_CameraModule_Basler3::OnGrabError(Pylon::CInstantCamera& camera, const char* errorMessage)
{
	std::cout << "Hd_CameraModule_Basler3 err:: " << errorMessage << std::endl;

	moduleStatus = false;
}


Hd_CameraModule_Basler3* create(int settype)
{
	qDebug() << __FUNCTION__ << "  line:" << __LINE__ << "Hd_CameraModule_Basler3 create success!";
	return new Hd_CameraModule_Basler3(settype);
}

void destory(Hd_CameraModule_Basler3* ptr)
{
	if (ptr)
	{
		delete  ptr;
		ptr = nullptr;
	}
	qDebug() << "[INFO] " << "Hd_CameraModule_Basler3  destory success!";
}

//int myIDS[10000] = { 0 };

//类创建
Hd_CameraModule_Basler3::Hd_CameraModule_Basler3(int settype, QObject* parent) : PbGlobalObject(settype, parent)
{
	/*m_MyData = new MyData();
	MyThread* newMyThread = new MyThread();
	newMyThread->camera = this;

	m_MyData->n_mutex = new QMutex();
	m_MyData->ifRunning = true;*/

	PylonInitialize();
	n_mutex = new QMutex();
	m_mutex = new QMutex();
	//QFuture<void> myF= QtConcurrent::run(this, &Hd_CameraModule_Basler3::threadCheckCallBack);
	if (type1 < 15)
		famliy = PGOFAMLIY::CAMERA2D;
	else if (type1 == 15)
		famliy = PGOFAMLIY::CAMERA2_5D;
	else if (type1 < 100)
		famliy = PGOFAMLIY::CAMERA3D;
	//id = myIDS[type1]++;
	qDebug() << "famliy::" << famliy;
}


Hd_CameraModule_Basler3::~Hd_CameraModule_Basler3()
{
	//myIDS[type1]--;
	/*m_MyData->ifFirst = true;
	m_MyData->ifRunning = false;
	m_MyData->m_flag = false;
	Sleep(10);
	CloseDevice(m_MyData);
	if (m_MyData->n_mutex)
	{
		delete m_MyData->n_mutex;
		m_MyData->n_mutex = nullptr;
	}*/
	ifFirst = true;
	ifmoduleRun = false;
	m_mutex->lock();
	CloseDevice();
	m_mutex->unlock();
	//PylonTerminate();
	if (m_mutex)
	{
		delete m_mutex;
		m_mutex = nullptr;
	}
	qDebug() << __FUNCTION__ << "line:" << __LINE__ << " delete success!";
}

QJsonObject load_camera_Example()
{
	QDir dir = QCoreApplication::applicationDirPath();
	QString currPath = dir.path();
	currPath = "../runtime/Bin_DEVICE";
	QString json_cfg_file_path = currPath + "/camera_Example.json";

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
//setParameter之后再调用，返回当前参数
	//相机：获取默认参数；
	//通信：获取初始化示例参数
QMap<QString, QString> Hd_CameraModule_Basler3::parameters()
{
	if (ifFirst == true)
	{
		ifFirst = false;

		moduleName = "Hd_CameraModule_Basler3#" + QString::number(id); //区别哪个相机

		QJsonObject ExampleObj = load_camera_Example();
		QJsonObject cameraObj;
		QJsonArray  ExampleArray = ExampleObj.value("相机模块列表").toArray();
		for (int i = 0; i < ExampleArray.size(); i++)
		{
			QString name = ExampleArray[i].toObject().value("模块名称").toString();
			if (moduleName.split("#")[0] == name)
				cameraObj = ExampleArray[i].toObject();
		} //参数设置的数据
		QMap<QString, QString> myParasValueMap;
		myParasValueMap.insert("相机key", "输入SN");
		//默认参数显示到界面
		QJsonArray ParameterArray = cameraObj.value("相机参数").toArray();
		for (int T = 0; T < ParameterArray.size(); T++)
		{
			QJsonObject ParameterObj = ParameterArray[T].toObject();
			QString ParameterKey = ParameterObj.value("参数名").toString();
			QString ParameterValue;
			QJsonArray ParameterValueArray = ParameterObj.value("相机参数值").toArray();
			for (int P = 0; P < ParameterValueArray.size(); P++)
			{
				if (P > 0)ParameterValue.append(",");
				ParameterValue.append(ParameterValueArray[P].toString());
			}
			myParasValueMap.insert(ParameterKey, ParameterValue);
		}
		return myParasValueMap;
	}
	else return m_ParasValueMap;
}

//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_Basler3::setParameter(const QMap<QString, QString>& ParameterMap)
{
	m_ParasValueMap = ParameterMap;
	//if (m_module.moduleName.isEmpty())
	//m_module.moduleName = getmoduleName();// = ParasValueMap.value()
	//if(id=)
	moduleName = "Hd_CameraModule_Basler3#" + QString::number(id);
	return true;
}

//初始化(加载模块待内存)
bool Hd_CameraModule_Basler3::init()
{
	////传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
	QString initByte = m_ParasValueMap.value("相机key");
	foreach(QString key, m_ParasValueMap.keys())
	{
		if (key == "相机key")
			continue;
		initByte += "+" + key + "#";
		QString ParameterValue = m_ParasValueMap.value(key);
		initByte += ParameterValue.replace(",", "#");
	}

	this->disconnect();
	if (initParas(initByte.toLocal8Bit()))
	{
		qDebug() << __FUNCTION__ << " line: " << __LINE__ << " key: " << m_ParasValueMap.value("相机key") << " initParas   success! ";
		//handdleAI3.0,重置计数，emit trigged(1000);
		connect(this, &PbGlobalObject::trigged, [=](int Code) {
			if (Code == 1000)
			{
				std::vector<cv::Mat> mat;
				QByteArray data = "rest_newProductIn";
				writeData(mat, data);
			}
			});

	}
	else
	{
		qCritical() << __FUNCTION__ << "  line: " << __LINE__ << " moduleName: " << m_ParasValueMap.value("相机key") << " initParas failed! ";
		return false;
	}
	//}
	//else
	//{
	//    delete m_lib;
	//    m_lib = nullptr;
	//    return false;
	//}
	return true;
}


//设置数据
	//相机：第一张图的参数，QStringList：00 曝光值 增益值；
	//相机：第N张图的参数，QStringList：非00 曝光值 增益值；
bool Hd_CameraModule_Basler3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
{
	//foreach(QString kidData,data)
		//if (kidData.contains("SetExposureTime"))
	{
		std::vector<cv::Mat> ImgS;
		QByteArray bData;
		for (int i = 0; i < data.size(); i++)
			//if(i>0)
			//    bData +=","+ data[i].toLocal8Bit();
			//else
			bData += data[i].toLocal8Bit();
		qDebug() << "Hd_CameraModule_Basler3::setData::" << bData;
		writeData(ImgS, bData);
		//if (m_module.module_ptr)
		 //   m_module.module_ptr->writeData(ImgS, bData);
		//return true;
	}
	//if(data.isEmpty())
	//	return true;
	foreach(QString kidData, data)
		if (kidData.contains("SetExposureTime") || kidData.contains("rest_newProductIn"))
			return true;
	bool runResult = run(); //m_module.module_ptr->run();
	return runResult;
}
//获取数据
bool Hd_CameraModule_Basler3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
	//std::vector<cv::Mat> ImgS;
	QByteArray data;
	//m_module.module_ptr->readData(ImgS, data);
	readData(ImgS, data);
	if (!data.isEmpty())QStringListdata.append(byteArrayToUnicode(data));
	return true;
}

//cameraIndex，函数指针的map
//QMap< QString, QObject*> mMap_cameraIndex_QObject;

//注册回调 string对应自身的参数协议 （自定义）
void Hd_CameraModule_Basler3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* m_QObject, const QString& cameraIndex)
{
	n_mutex->lock();
	myPBGLOBAL_callBack m_myPBGLOBAL_callBack;
	m_myPBGLOBAL_callBack.callBackFun = callBackFun;
	m_myPBGLOBAL_callBack.m_QObject = m_QObject;
	m_myPBGLOBAL_callBack.cameraIndex = cameraIndex;
	QQueue_myPBGLOBAL_callBack.enqueue(m_myPBGLOBAL_callBack);
	n_mutex->unlock();
}
//注销回调 string对应自身的参数协议 （自定义）--->注销后还得取消连接状态
void Hd_CameraModule_Basler3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* m_QObject, const QString& cameraIndex)
{
	n_mutex->lock();
	qDebug() << __FUNCTION__ << " line:" << __LINE__ << "try cancelCallBackFun";
	for (int i = 0; i < QQueue_myPBGLOBAL_callBack.size(); i++)
	{
		if (QQueue_myPBGLOBAL_callBack[i].callBackFun == NULL)
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "QQueue_myPBGLOBAL_callBack[i].callBackFun == NULL";
		if (QQueue_myPBGLOBAL_callBack[i].m_QObject == NULL)
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "QQueue_myPBGLOBAL_callBack[i].m_QObject == NULL";

		if (QQueue_myPBGLOBAL_callBack[i].callBackFun == callBackFun &&
			QQueue_myPBGLOBAL_callBack[i].m_QObject == m_QObject &&
			QQueue_myPBGLOBAL_callBack[i].cameraIndex == cameraIndex)
		{
			qDebug() << __FUNCTION__ << " line:" << __LINE__ << "try QQueue_myPBGLOBAL_callBack.removeAt(i)";
			QQueue_myPBGLOBAL_callBack.removeAt(i);
			break;
		}
	}
	qDebug() << __FUNCTION__ << " line:" << __LINE__ << "try cancelCallBackFun done!";
	n_mutex->unlock();
}


//Hd_CameraModule_Basler3* create()
//{
//	qDebug() << "[info] " << "Hd_CameraModule_Basler3 create success!";
//	return new Hd_CameraModule_Basler3;
//}
//
//void destory(Hd_CameraModule_Basler3* ptr)
//{
//	if (ptr)
//	{
//		delete  ptr;
//		ptr = nullptr;
//	}
//	qDebug() << "[info] " << "Hd_CameraModule_Basler3  destory success!";
//}

//Hd_CameraModule_Basler3::Hd_CameraModule_Basler3()
//{
//	PylonInitialize();
//	n_mutex = new QMutex();
//	m_mutex = new QMutex();
//}
//关闭设备
void Hd_CameraModule_Basler3::CloseDevice()
{
	m_camera.StopGrabbing();
	//m_bitmapImage.Release();

	//// Free the grab result, if present.
	//m_ptrGrabResult.Release();

	//// Remove the event handlers that will be called when the feature changes.
	//m_camera.DeregisterCameraEventHandler(this, "TriggerSource");

	//m_camera.DeregisterCameraEventHandler(this, "TriggerMode");

	//m_camera.DeregisterCameraEventHandler(this, "PixelFormat");

	//if (m_gain.IsValid())
	//{
	//	// If we must use the alternative integer representation, we don't know the name of the node as it defined by the camera
	//	m_camera.DeregisterCameraEventHandler(this, m_gain.GetNode()->GetName());
	//}

	//if (m_exposureTime.IsValid())
	//{
	//	// If we must use the alternative integer representation, we don't know the name of the node as it defined by the camera
	//	m_camera.DeregisterCameraEventHandler(this, m_exposureTime.GetNode()->GetName());
	//}

	//// Clear the pointers to the features we set manually in Open().
	//m_exposureTime.Release();
	//m_gain.Release();

	// Close the camera and free all ressources.
	// This will also unregister all 
	m_camera.DestroyDevice();
	m_camera.Close();
	//delete m_camera;
	//m_camera = nullptr;

	/*camera.StopGrabbing();
	camera.DestroyDevice();
	camera.Close();*/
}

//Hd_CameraModule_Basler3::~Hd_CameraModule_Basler3()
//{
//	ifmoduleRun = false;
//	m_mutex->lock();
//	CloseDevice();
//	m_mutex->unlock();
//	//PylonTerminate();
//	if (m_mutex)
//	{
//		delete m_mutex;
//		m_mutex = nullptr;
//	}
//
//	qDebug() << "[info] " << " delete success!";
//}
//从类中读数据到实例对象
bool Hd_CameraModule_Basler3::readData(std::vector<cv::Mat>& mat, QByteArray& data)
{
	if (queuepic.isEmpty())
	{
		//qCritical() << "[INFO] " << " srcImage is null!";
		return false;
	}
	n_mutex->lock();
	//cv::Mat img = cv::Mat();
	//queuepic.dequeue().copyTo(img);
	//mat.push_back(img.clone());
	mat.push_back(queuepic.dequeue());
	QString qimgIndex = "imgIndex_" + QString::number(imgIndex - 1) + "_中文测试";
	data = qimgIndex.toLocal8Bit();
	n_mutex->unlock();
	qDebug() << "[INFO] " << __FUNCTION__ << " line:" << __LINE__ << " checkImg readData--CameraName:" << QString::fromStdString(Username) << "--imgIndex : " << imgIndex;
	return true;
}
//实例对象把数据写入到类
bool Hd_CameraModule_Basler3::writeData(std::vector<cv::Mat>& mat, QByteArray& data)
{
	//2024.02.18更新，相机对应的读SN信号时，表示新产品进入，设置初始值
	if (data == "rest_newProductIn")
	{
		//根据实际项目，设置初始值：曝光、增益，ROI区域等
		{
			if (expourseTimeList.size() > 0)
				SetExposureTime(expourseTimeList[0]);
			if (gainList.size() > 0)
				SetGain(gainList[0]);
		}
		if (queuepic.size() > 0)
		{
			n_mutex->lock();
			queuepic.clear();
			n_mutex->unlock();
			std::cout << "rest_newProductIn , queuePic.size() > 0 ,signal maybe too fast " << Username << std::endl;
			qWarning() << __FUNCTION__ << "  line: " << __LINE__ << "rest_newProductIn , queuePic.size() > 0 ,signal maybe too fast " << QString::fromStdString(Username);
		}
		if (ifRunning == true)
		{
			ifBreak = true;
			Sleep(10);
		}
		imgIndex = 0;
		getImageNum = 0;
		return true;
	}

	if (data == "ifChangeExpourseAndGain_false")
		ifChangeExpourseAndGain = false;
	else
		ifChangeExpourseAndGain = true;
	n_mutex->lock();
	if (data.contains("SetExposureTime_"))
	{
		QByteArrayList dataList = data.split('#');
		for (int i = 0; i < dataList.size(); i++)
		{
			if (dataList[i].contains("SetExposureTime_"))
			{
				int value = byteArrayToUnicode(dataList[i]).split("SetExposureTime_")[1].toInt();
				SetExposureTime(value);
			}
			if (dataList[i].contains("SetGain_"))
			{
				double value = byteArrayToUnicode(dataList[i]).split("SetGain_")[1].toInt();
				SetGain(value);
			}
			if (dataList[i].contains("TriggerMode_"))
			{
				QString value = byteArrayToUnicode(dataList[i]).split("TriggerMode_")[1];
				if (value == "软触发" || value == "soft")
					////触发模式
				{
					qDebug() << "SetValue TriggerSource_Software";
					if (currentTriggerSource != Basler_UniversalCameraParams::TriggerSource_Software)
					{
						//触发模式
						m_camera.TriggerSource.SetValue(Basler_UniversalCameraParams::TriggerSource_Software);
						currentTriggerSource = Basler_UniversalCameraParams::TriggerSource_Software;
					}
				}
				else if (value == "硬触发" || value == "hard")
				{
					qDebug() << "SetValue TriggerSource_Line1";
					if (currentTriggerSource != Basler_UniversalCameraParams::TriggerSource_Line1)
					{
						m_camera.TriggerSource.SetValue(Basler_UniversalCameraParams::TriggerSource_Line1);
						currentTriggerSource = Basler_UniversalCameraParams::TriggerSource_Line1;
					}
				}
				else
					qDebug() << "TriggerMode err ,may Chinese Name!";

				Basler_UniversalCameraParams::TriggerSourceEnums nCurValue = m_camera.TriggerSource.GetValue();
				//MV_CC_GetEnumValue(m_MyData->handle, "TriggerSource", &stEnumValue); //获取当前触发方式
				if (currentTriggerSource != nCurValue)
					m_camera.TriggerSource.SetValue(currentTriggerSource);
			}
		}
	}
	//调试模式下，data==ifChangeExpourseAndGain_false；非调试模式运行时，data不包含SetExposureTime_
	else if (data != "ifChangeExpourseAndGain_false" && initTriggerSource != currentTriggerSource)
	{
		m_camera.TriggerSource.SetValue(initTriggerSource);
		currentTriggerSource = initTriggerSource;
	}
	n_mutex->unlock();

	qDebug() << "Hd_CameraModule_Basler3::writeData::" << data;
	return true;
}

Pylon::IIntegerEx& Hd_CameraModule_Basler3::GetExposureTime()
{
	return m_exposureTime;
}

Pylon::IIntegerEx& Hd_CameraModule_Basler3::GetGain()
{
	return m_gain;
}

void Hd_CameraModule_Basler3::SetExposureTime(float exposureValue)
{
	qDebug() << "[INFO] " << "exposureValue:" << exposureValue;
	//设置曝光，GigE相机为ExposureTimeRaw；USB相机为ExposureTime；方法可行,断电不保存
	if (m_camera.ExposureTime.IsValid())
	{
		//m_camera.ExposureTime.GetAlternativeIntegerRepresentation(m_exposureTime);
		//int64_t myExposureTimeRaw = m_camera.ExposureTimeRaw.GetValue();
		//m_camera.ExposureTimeRaw.SetValue(exposureValue);
		GetExposureTime().SetValue(exposureValue);
	}
	else if (m_camera.ExposureTimeRaw.IsValid())
	{
		//m_exposureTime.Attach(m_camera.ExposureTimeRaw.GetNode());
		//m_camera.ExposureAuto.TrySetValue(ExposureAuto_Off);
		//m_camera.ExposureTimeRaw.SetValue(exposureValue);

		const int64_t minimum = GetExposureTime().GetMin();
		const int64_t maximum = GetExposureTime().GetMax();
		try
		{
			if(exposureValue> GetExposureTime().GetMin())
				GetExposureTime().SetValue(int(exposureValue)/ minimum * minimum);
			// Set the value in the control again in case it was altered by roundValue.
			//pCtrl->SetPos((int)roundvalue);
		}
		catch (const Pylon::GenericException& e)
		{
			qDebug() << "Failed to set:" << GetExposureTime().GetInfo(Pylon::ParameterInfo_DisplayName).c_str() << " e." << e.GetDescription();
			//TRACE("Failed to set '%hs':%hs", integerParameter.GetInfo(Pylon::ParameterInfo_DisplayName).c_str(), e.GetDescription());
			//UNUSED(e);
		}
		catch (...)
		{
			qDebug() << "Failed to set:" << GetExposureTime().GetInfo(Pylon::ParameterInfo_DisplayName).c_str();
			//TRACE("Failed to set '%hs'", integerParameter.GetInfo(Pylon::ParameterInfo_DisplayName).c_str());
		}

		//if (GetExposureTime().IsWritable())
		//	GetExposureTime().SetValue(30000.00);
		Sleep(10);
	}
}

void Hd_CameraModule_Basler3::SetGain(float GainValue)
{
	qDebug() << "[INFO] " << "GainValue:" << GainValue;
	//线扫相机，设置增益有问题
	//设置增益，GigE相机为GainRaw；USB相机为Gain
	if (m_camera.Gain.IsValid())
	{
		//m_camera.Gain.GetAlternativeIntegerRepresentation(m_gain);
		//m_camera.Gain.SetValue(GainValue);
		GetGain().SetValue(GainValue);
	}
	else if (m_camera.GainRaw.IsValid())
	{
		//m_gain.Attach(m_camera.GainRaw.GetNode());
		//m_camera.GainRaw.SetValue(GainValue);
		//GetGain().SetValue(GainValue);
		const int64_t minimum = GetExposureTime().GetMin();
		const int64_t maximum = GetExposureTime().GetMax();

		try
		{
			if (GainValue >= minimum && GainValue <= maximum)
				GetGain().SetValue(GainValue);
			// Set the value in the control again in case it was altered by roundValue.
			//pCtrl->SetPos((int)roundvalue);
		}
		catch (const Pylon::GenericException& e)
		{
			qDebug() << "Failed to set:" << GetGain().GetInfo(Pylon::ParameterInfo_DisplayName).c_str() << " e." << e.GetDescription();
			//TRACE("Failed to set '%hs':%hs", integerParameter.GetInfo(Pylon::ParameterInfo_DisplayName).c_str(), e.GetDescription());
			//UNUSED(e);
		}
		catch (...)
		{
			qDebug() << "Failed to set:" << GetGain().GetInfo(Pylon::ParameterInfo_DisplayName).c_str();
			//TRACE("Failed to set '%hs'", integerParameter.GetInfo(Pylon::ParameterInfo_DisplayName).c_str());
		}
	}
}

//传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
bool Hd_CameraModule_Basler3::initParas(QByteArray& initData)
{
	QByteArrayList valueList = initData.split('+');
	Username = valueList[0];

	getImageNum = 0;
	expourseTimeList.clear();
	gainList.clear();

	bool ifOk = true;
	if (ifInit == false)//(m_camera == nullptr)
		ifOk = ConnctDevice();
	if (ifOk == false)
	{
		qDebug() << "try connctDevice again!" << QString::fromStdString(Username);
		ifOk = ConnctDevice();
	}
	if (ifOk == false)
		return false;
	ifInit = true;
	if (valueList.size() > 1)
		for (int i = 1; i < valueList.size(); i++)
		{
			QString valuei = byteArrayToUnicode(valueList[i]);
			//valuei.prepend(valueList[i]);
			if (valuei.contains("expourseTime"))
			{
				QStringList expourseTimeL = valuei.split('#');
				if (expourseTimeL.size() > 1)
					for (int j = 1; j < expourseTimeL.size(); j++)
						expourseTimeList.enqueue(expourseTimeL[j].toFloat());
			}
			if (valuei.contains("gain"))
			{
				QStringList gainL = valuei.split('#');
				if (gainL.size() > 1)
					for (int j = 1; j < gainL.size(); j++)
						gainList.enqueue(gainL[j].toFloat());
			}

			if (valuei.contains("是否为标准通信流程"))
			{
				if (valuei.contains("true"))
					ifStandard = true;
			}
			if (valuei.contains("取图超时") || valuei.contains("getImageTimeOut"))
			{
				QStringList gainL = valuei.split('#');
				if (gainL.size() > 1)
					getImageTimeOut = gainL[1].toInt();
				if (getImageTimeOut < 50)
					getImageTimeOut = 1000;
			}
			
			if (valuei.contains("触发方式") || valuei.contains("TriggerMode"))
			{
				QStringList valueL = valuei.split('#');
				if (valueL.size() > 1)
					if (valueL[1].toInt() >= 0)
						if (valueL[1] == "软触发" || valueL[1] == "soft")
						{
							//触发模式
							m_camera.TriggerSource.SetValue(Basler_UniversalCameraParams::TriggerSource_Software);
							currentTriggerSource = Basler_UniversalCameraParams::TriggerSource_Software;
							initTriggerSource = Basler_UniversalCameraParams::TriggerSource_Software;
						}
						else if (valueL[1] == "硬触发" || valueL[1] == "hard")
						{
							m_camera.TriggerSource.SetValue(Basler_UniversalCameraParams::TriggerSource_Line1);
							currentTriggerSource = Basler_UniversalCameraParams::TriggerSource_Line1;
							initTriggerSource = Basler_UniversalCameraParams::TriggerSource_Line1;
						}
			}
		}
	imgIndex = 0;
	
	if (ifOk == true)
	{
		if (expourseTimeList.size() > 0)
			SetExposureTime(expourseTimeList[0]);
		if (gainList.size() > 0)
			SetGain(gainList[0]);
	}

	getImageNum = 0;
	return ifOk;
}

bool Hd_CameraModule_Basler3::run()
{
	bool result = false; 
	ifGetImage = false;
	ifRunning = true;
	ifBreak = false;
	//srcImage.release();
	try
	{
		m_mutex->lock();

		/*if (!m_camera.IsGrabbing())
		{
			m_mutex->unlock();
			return result;
		}*/

		//软触发一次
		// Only wait if software trigger is currently turned on.
		if (m_camera.TriggerSource.GetValue() == Basler_UniversalCameraParams::TriggerSource_Software
			&& m_camera.TriggerMode.GetValue() == Basler_UniversalCameraParams::TriggerMode_On)
		{
			// If the camera is currently processing a previous trigger command,
			// it will silently discard trigger commands.
			// We wait until the camera is ready to process the next trigger.
			if (moduleStatus == false)
			{
				m_mutex->unlock(); 
				return result;
			}
			if (m_camera.WaitForFrameTriggerReady(3000, Pylon::TimeoutHandling_ThrowException) == true)
			{
				// Send trigger
				m_camera.ExecuteSoftwareTrigger();
			}
			else {
				m_mutex->unlock();
				return result;
			}
		}
		else {
			m_mutex->unlock();
			return result;
		}
		m_mutex->unlock();

		int mytime = 0;
		while (true)
		{
			if (moduleStatus == false || mytime++ > getImageTimeOut || ifBreak == true)
			{
				result = false;
				break;
			}
			n_mutex->lock();
			if (queuepic.size() > 0)
			{
				n_mutex->unlock();
				result = true;
				break;
			}
			n_mutex->unlock();
			Sleep(1);
		}
		//软触发，不考虑误触发、漏触发
		if (ifChangeExpourseAndGain == true)
		{
			getImageNum++;
			if (expourseTimeList.size() > 1 && expourseTimeList[getImageNum % expourseTimeList.size()] != expourseTimeList[(getImageNum - 1) % expourseTimeList.size()])
				SetExposureTime(expourseTimeList[getImageNum % expourseTimeList.size()]);
			if (gainList.size() > 1 && gainList[getImageNum % gainList.size()] != gainList[(getImageNum - 1) % gainList.size()])
				SetGain(gainList[getImageNum % gainList.size()]);
		}
	}
	catch (...)
	{
		qDebug() << "[error] " << " Hd_CameraModule_Basler3:" ;
	}
	return result;
}
//相机连接状态：正常连接或断开
bool Hd_CameraModule_Basler3::checkStatus()
{
	//m_mutex->lock();
	//status = modulestatus;
	return moduleStatus;
	//m_mutex->unlock();
}

