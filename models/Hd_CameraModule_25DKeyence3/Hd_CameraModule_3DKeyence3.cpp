#include "Hd_CameraModule_3DKeyence3.h"
#include <exception>

#include <QTextCodec>
#include <qqueue.h>
#include <qmutex.h>

#pragma execution_character_set("utf-8")

const int MAX_LJXA_XDATANUM = 3200;
const unsigned int BUFFER_FULL_COUNT = 30000;

int _imageAvailable[MAX_LJXA_DEVICENUM] = { 0 };
unsigned short* _luminanceBuf[MAX_LJXA_DEVICENUM] = { 0 };
unsigned short* _heightBuf[MAX_LJXA_DEVICENUM] = { 0 };
int _lastImageSizeHeight[MAX_LJXA_DEVICENUM];

int imgIndex = 0;
QMutex* m_mutex;
QQueue< cv::Mat> heightMatS;//高度图
QQueue<cv::Mat> luminanceMatS;//灰度图
//data == "rest_newProductIn"时要结束run；机台报警，取图超时，复位后重新拍的情况。
bool ifBreak = false;
//ifRunning减少ifBreak的判断；"rest_newProductIn"时，Logic_camera_thread框架会结束未处理完的相相机取图
bool ifRunning = false;
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


struct myPBGLOBAL_callBack
{
    void (*callBackFun)(QObject*, const std::vector<cv::Mat>&);
    QObject* m_QObject;
    QString cameraIndex;
};
QQueue< myPBGLOBAL_callBack> QQueue_myPBGLOBAL_callBack;
//注册回调 string对应自身的参数协议 （自定义）
void Hd_CameraModule_3DKeyence3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN callBackFun, QObject* m_QObject, const QString& cameraIndex)
{
    myPBGLOBAL_callBack m_myPBGLOBAL_callBack;
    m_myPBGLOBAL_callBack.callBackFun = callBackFun;
    m_myPBGLOBAL_callBack.m_QObject = m_QObject;
    m_myPBGLOBAL_callBack.cameraIndex = cameraIndex;
    QQueue_myPBGLOBAL_callBack.enqueue(m_myPBGLOBAL_callBack);
}


std::mutex mtx;
bool JumpFlag = false;
int yCounts;
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

        std::lock_guard<std::mutex> lock(mtx);

        // _heightBuf 代表的含义:程序里的高度缓存区，pHeightProfileArray 回调函数里的数据 
        // 此处崩溃的可能性：xImageSize 配置文件中设置的值和3D相机调试软件设置的值不一致，会崩溃。
        if (JumpFlag)
        {
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << "JumpFlag";
            return;
        }
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
            _heightBuf[dwUser] = (unsigned short*)malloc(yCounts * MAX_LJXA_XDATANUM * 2);
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

int Hd_CameraModule_3DKeyence3::LJXA_ACQ_OpenDevice(int lDeviceId, LJX8IF_ETHERNET_CONFIG* EthernetConfig, int HighSpeedPortNo)
{
    int errCode = LJX8IF_EthernetOpen(lDeviceId, EthernetConfig);

    _ethernetConfig[lDeviceId] = *EthernetConfig;
    _highSpeedPortNo[lDeviceId] = HighSpeedPortNo;

    qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Open device ! errCode:" << errCode;
    return errCode;
}

void Hd_CameraModule_3DKeyence3::LJXA_ACQ_CloseDevice(int lDeviceId)
{
    LJX8IF_FinalizeHighSpeedDataCommunication(lDeviceId);
    LJX8IF_CommunicationClose(lDeviceId);
    qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  Close device!";
}


bool Hd_CameraModule_3DKeyence3::decodeInitData(QByteArray byte,QVector<QByteArray> &vector_data)
{
    while (!byte.isEmpty())
    {
        int type = 0;
        QByteArray m_type = byte.mid(0,4);
        memcpy((void*)&type,(void*)m_type.data(), 4);
        byte.remove(0,4);

        int len = 0;
        QByteArray m_len = byte.mid(0,4);
        memcpy((void*)&len,(void*)m_len.data(), 4);
        byte.remove(0,4);

        QByteArray mm_data = byte.mid(0,len);
        vector_data.append(mm_data);
        byte.remove(0,len);
    }
    return true;
}

Hd_CameraModule_3DKeyence3* create(int settype)
{
    qDebug() << __FUNCTION__ << "  line:" << __LINE__ << "Hd_CameraModule_All create success!";
    return new Hd_CameraModule_3DKeyence3(settype);
}

void destory(Hd_CameraModule_3DKeyence3* ptr)
{
    if (ptr)
    {
        delete  ptr;
        ptr = nullptr;
    }
    qDebug() << "[INFO] " << "Hd_CameraModule_3DLMI3  destory success!";
}

//int myIDS[10000] = { 0 };

//类创建
Hd_CameraModule_3DKeyence3::Hd_CameraModule_3DKeyence3(int settype, QObject* parent) : PbGlobalObject(settype, parent)
{
    m_mutex = new QMutex();
    ifRunning = true;
    //QFuture<void> myF= QtConcurrent::run(this, &Hd_CameraModule_3DLMI3::threadCheckCallBack);
    if (type1 < 15)
        famliy = PGOFAMLIY::CAMERA2D;
    else if (type1 == 15)
        famliy = PGOFAMLIY::CAMERA2_5D;
    else if (type1 < 100)
        famliy = PGOFAMLIY::CAMERA3D;
    //id = myIDS[type1]++;
    qDebug() << "famliy::" << famliy;
}


Hd_CameraModule_3DKeyence3::~Hd_CameraModule_3DKeyence3()
{
    //myIDS[type1]--;
    //ifFirst = true;
    ifRunning = false;
    //m_flag = false;
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
    if (m_mutex)
    {
        delete m_mutex;
        m_mutex = nullptr;
    }
    qDebug() << __FUNCTION__ << "line:" << __LINE__ << " delete success!";
    /*if(m_thread)
    {
        m_thread->exit();
        m_thread->wait(1);
        delete m_thread;
        m_thread = nullptr;
    }*/

    qDebug() << __FUNCTION__ << "line:" << __LINE__ << " delete success!";
}


//模块名称
QString moduleName;

//setParameter之后再调用，返回当前参数
    //相机：获取默认参数；
    //通信：获取初始化示例参数
QMap<QString, QString> Hd_CameraModule_3DKeyence3::parameters()
{
    if (ifFirst == true)
    {
        ifFirst = false;

        moduleName = "Hd_CameraModule_3DKeyence3#" + QString::number(id);

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
    else return ParasValueMap;
}


QJsonObject Hd_CameraModule_3DKeyence3::load_camera_Example()
{
    QDir dir = QApplication::applicationDirPath();
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


//初始化参数；通信/相机的初始化参数
bool Hd_CameraModule_3DKeyence3::setParameter(const QMap<QString, QString>& ParameterMap)
{
    ParasValueMap = ParameterMap;
    //if (m_module.moduleName.isEmpty())
    //m_module.moduleName = getmoduleName();// = ParasValueMap.value()
    //if(id=)
    moduleName = "Hd_CameraModule_3DKeyence3#" + QString::number(id);
    return true;
}

//初始化(加载模块待内存)
bool Hd_CameraModule_3DKeyence3::init()
{
    ////传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
    QString initByte = ParasValueMap.value("相机key");
    foreach(QString key, ParasValueMap.keys())
    {
        if (key == "相机key")
            continue;
        initByte += "+" + key + "#";
        QString ParameterValue = ParasValueMap.value(key);
        initByte += ParameterValue.replace(",", "#");
    }

    if (initParas(initByte.toLocal8Bit()))
    {
        qDebug() << __FUNCTION__ << " line: " << __LINE__ << " key: " << ParasValueMap.value("相机key") << " initParas   success! ";
        /*Base* myPtr = m_module.module_ptr;
        connect(myPtr, &Base::trigged, [=](bool ifConnect) {
            qDebug() << __FUNCTION__ << " line: " << __LINE__ << " key: " << ParasValueMap.value("相机key") << " m_module.module_ptr, &Base::trigged! ifConnect" << ifConnect;
            emit trigged(ifConnect);
            });*/
    }
    else
    {
        qCritical() << __FUNCTION__ << "  line: " << __LINE__ << " moduleName: " << ParasValueMap.value("相机key") << " initParas failed! ";
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
    //完成后发送信号trigged(bool);
bool Hd_CameraModule_3DKeyence3::setData(const std::vector<cv::Mat>& mats, const QStringList& data)
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
        qDebug() << "Hd_CameraModule_3DKeyence3::setData::" << bData;
        writeData(ImgS, bData);
        //if (m_module.module_ptr)
         //   m_module.module_ptr->writeData(ImgS, bData);
        //return true;
    }
    foreach(QString kidData, data)
        if (kidData.contains("SetExposureTime") || kidData.contains("rest_newProductIn"))
            return true;
    bool runResult = run(); //m_module.module_ptr->run();
    //emit trigged(runResult);
    return runResult;
}
//获取数据
bool Hd_CameraModule_3DKeyence3::data(std::vector<cv::Mat>& ImgS, QStringList& QStringListdata)
{
    //std::vector<cv::Mat> ImgS;
    QByteArray data;
    //m_module.module_ptr->readData(ImgS, data);
    readData(ImgS, data);
    if (!data.isEmpty())QStringListdata.append(byteArrayToUnicode(data));
    return true;
}

bool Hd_CameraModule_3DKeyence3::readData(std::vector<cv::Mat> &mats,QByteArray &data)
{
    Q_UNUSED(data);

    m_mutex->lock();
    if (luminanceMatS.empty() || heightMatS.empty())
    {
        qDebug() << " [Error] " << " srcImage is null";
        m_mutex->unlock();
        return false;
    }
    cv::Mat mheightMat2 = cv::Mat();
    heightMatS.dequeue().copyTo(mheightMat2);
    cv::Mat mluminanceMat2 = cv::Mat();
    luminanceMatS.dequeue().copyTo(mluminanceMat2);
    mats.push_back(mheightMat2.clone());
    mats.push_back(mluminanceMat2.clone());

    m_mutex->unlock();
    /*for (int i = 0; i < vector_mat.size(); i++)
    {
        cv::Mat img = cv::Mat();
        vector_mat[i].copyTo(img);
        mats.push_back(img);
    }*/
    //mats = vector_mat;
    qCritical() << " [info] " << __FUNCTION__ << " line:" << __LINE__ << " checkImg readData,deviceId" << deviceId << " imgIndex" << imgIndex;
    return true;
}

bool Hd_CameraModule_3DKeyence3::writeData(std::vector<cv::Mat> &mat,QByteArray &data)
{
    //2024.02.18更新，相机对应的读SN信号时，表示新产品进入，设置初始值
    if (data == "rest_newProductIn")
    {
        //根据实际项目，设置初始值：曝光、增益，ROI区域等
        /*{
            if (expourseTimeList.size() > 0)
                SetExposureTime(expourseTimeList[0]);
            if (gainList.size() > 0)
                SetGain(gainList[0]);
        }
        getImageNum = 0;*/
        if (luminanceMatS.size() > 0 || heightMatS.size() > 0)
        {
            m_mutex->lock();
            luminanceMatS.clear();
            heightMatS.clear();
            m_mutex->unlock();
            std::cout << "rest_newProductIn , queuePic.size() > 0 ,signal maybe too fast " << camera_name.toStdString() << std::endl;
            qWarning() << __FUNCTION__ << "  line: " << __LINE__ << "rest_newProductIn , queuePic.size() > 0 ,signal maybe too fast " << camera_name;
        }
        if (ifRunning == true)
        {
            ifBreak = true;
            Sleep(10);
        }
        imgIndex = 0;
        //getImageNum = 0;
        return true;
    }
    

    return true;
}

//传入initParas函数，格式为：相机key+参数名1#参数值1#参数值2+参数名2#参数值1...
bool Hd_CameraModule_3DKeyence3::initParas(QByteArray& initData)
{
    QByteArrayList valueList = initData.split('+');
    camera_name = valueList[0];
    deviceId = valueList[0].toInt();  //相机头序号
    if (valueList.size() > 1)
        for (int i = 1; i < valueList.size(); i++)
        {
            /*if (valueList[i].contains("是否为标准通信流程"))
            {
                if (valueList[i].contains("true"))
                    ifStandard = true;
            }
            if (valueList[i].contains("取图超时"))
            {
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() > 1)
                    getImageTimeOut = valueL[1].toInt();
                if (getImageTimeOut < 50)
                    getImageTimeOut = 1000;
            }*/
            if (valueList[i].contains("ip"))
            {
                //ip#192.168.0.1
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() == 2)
                {
                    QByteArrayList valueL1 = valueL[1].split('.');
                    if (valueL1.size() == 4)
                    {
                        EthernetConfig.abyIpAddress[0] = valueL1.at(0).toInt();
                        EthernetConfig.abyIpAddress[1] = valueL1.at(1).toInt();
                        EthernetConfig.abyIpAddress[2] = valueL1.at(2).toInt();
                        EthernetConfig.abyIpAddress[3] = valueL1.at(3).toInt();
                    }
                }
            }
            if (valueList[i].contains("port"))
            {
                //port#24691
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() == 2)
                {
                    EthernetConfig.wPortNo = valueL[1].toInt();
                }
            }
            if (valueList[i].contains("xImageSize"))
            {
                //xImageSize#1600
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() == 2)
                {
                    xImageSize = valueL[1].toInt();
                }
            }
            if (valueList[i].contains("yImageSize"))
            {
                //yImageSize#2000
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() == 2)
                {
                    yImageSize = valueL[1].toInt();
                }
            }
            if (valueList[i].contains("y_pitch_um"))
            {
                //y_pitch_um#40
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() == 2)
                {
                    y_pitch_um = valueL[1].toInt();
                }
            }
            if (valueList[i].contains("取图超时"))
            {
                //timeout_ms#2900
                QByteArrayList valueL = valueList[i].split('#');
                if (valueL.size() == 2)
                {
                    timeout_ms = valueL[1].toInt();
                }
            }
        }

    yCounts = yImageSize;
    qDebug() << __FUNCTION__ << " line:" << __LINE__ << " camera_name:" << camera_name << " paras init is success!";

    // 初始化申请空间 20230214
    startReq_ptr    = new LJX8IF_HIGH_SPEED_PRE_START_REQ;
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
    if ( errCode != LJX8IF_RC_OK )
    {
        qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Failed to open device ";
        //Free user memory
        if ( heightImage   )
        {
            free(heightImage);
        }

        if ( luminanceImage )
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
        qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  InitHighSpeed error！ " ;
    }
    return true;
}
bool Hd_CameraModule_3DKeyence3::InitHighSpeed()
{
    try
    {
        if ( startReq_ptr == nullptr )
        {
            startReq_ptr = new LJX8IF_HIGH_SPEED_PRE_START_REQ;
            startReq_ptr->bySendPosition = 2;
            qWarning() << __FUNCTION__ << "  line: " << __LINE__ << " startReq_ptr is null! ";
        }
        if ( profileInfo_ptr == nullptr )
        {
            profileInfo_ptr = new LJX8IF_PROFILE_INFO;
            qWarning() << __FUNCTION__ << "  line: " << __LINE__ << " profileInfo_ptr is null! ";
        }
        errCode = LJX8IF_InitializeHighSpeedDataCommunicationSimpleArray(deviceId, &_ethernetConfig[deviceId], _highSpeedPortNo[deviceId], &myCallbackFunc, yImageSize, deviceId); // 初始化高速通讯
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
        qDebug() << __FUNCTION__ << " line:" << __LINE__ << "  ev: " <<  ev;
        return false;
    }
    return true;
   
}
bool Hd_CameraModule_3DKeyence3::run()
{
    ifRunning = true;
    ifBreak = false;
    if ( !isopen )
    {
        qCritical()  << __FUNCTION__ << " line:" << __LINE__ << " device is not opened! ";
        return false;
    }

    try
    {
            JumpFlag = false;
            errCode = LJX8IF_StartHighSpeedDataCommunication(deviceId);           // 正式开始高速通讯
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << " errCode: " << errCode;
            if (errCode != 0x80A1 && errCode != 0x0000)
            {
                qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Restart HighSpeedDataCommunication";
                errCode = LJX8IF_StopHighSpeedDataCommunication(deviceId);
                qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Stop HSC errCode:" << errCode;
                errCode = LJX8IF_FinalizeHighSpeedDataCommunication(deviceId);
                qDebug() << __FUNCTION__ << " line:" << __LINE__ << "   Fini HSC errCode:" << errCode;
                errCode = LJX8IF_InitializeHighSpeedDataCommunicationSimpleArray(deviceId, &_ethernetConfig[deviceId], _highSpeedPortNo[deviceId], &myCallbackFunc, yImageSize, deviceId); // 初始化高速通讯
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
            DWORD start = timeGetTime();
            while (true)
            {
                DWORD ts = timeGetTime() - start;
                if ((DWORD)timeout_ms < ts || ifBreak == true)
                {
                    qCritical() << __FUNCTION__ << "  line:" << __LINE__ << "   timeout_ms " << ts;
                    JumpFlag = true;
                    break;
                }
                if (_imageAvailable[deviceId])
                {
                    qCritical() << " [info] " << __FUNCTION__ << " line:" << __LINE__ <<" deviceId::" << deviceId << " checkImg getImg time" << ts<<" imgIndex" << imgIndex++ ;
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
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << "test";
            _getParam[deviceId].luminance_enabled = profileInfo_ptr->byLuminanceOutput;
            _getParam[deviceId].x_pointnum = profileInfo_ptr->wProfileDataCount;
            _getParam[deviceId].y_linenum_acquired = _lastImageSizeHeight[deviceId];
            _getParam[deviceId].x_pitch_um = profileInfo_ptr->lXPitch / 100.0f;
            _getParam[deviceId].y_pitch_um = y_pitch_um;
            _getParam[deviceId].z_pitch_um = zUnit / 100.0f;

            *getParam_Ptr = _getParam[deviceId];
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << " getPara Finished: ";
            int xDataNum = _getParam[deviceId].x_pointnum;

            unsigned short* dwHeightBuf = (unsigned short*)&_heightBuf[deviceId][0];
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << " dwHeightBuf Init Finished: ";
            memcpy(heightImage, dwHeightBuf, xDataNum * yImageSize * 2);

            qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Height Img memcpy Finished: ";
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
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << " Free memory Finished: ";

            m_mutex->lock();
            //vector_mat.clear();
            luminanceMatS.enqueue(cv::Mat(yImageSize, xImageSize, CV_16U, luminanceImage));
            heightMatS.enqueue(cv::Mat(yImageSize, xImageSize, CV_16U, heightImage));
            m_mutex->unlock();
            qDebug() << __FUNCTION__ << " line:" << __LINE__ << " success to acquire 3d image! camera_name: " << camera_name;
            return true;
        }
        catch (QString ev)
        {
            qCritical() << __FUNCTION__ << "  line: " << __LINE__ << "  ev: " << ev;
            return false;
        }
}
