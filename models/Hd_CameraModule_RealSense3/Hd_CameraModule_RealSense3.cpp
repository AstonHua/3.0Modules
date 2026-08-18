#include "Hd_CameraModule_RealSense3.h"
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#pragma execution_character_set("utf-8")

const QByteArray FirstCreateByte(R"({
    "depthWidth":    "848",
    "depthHeight":   "480",
    "depthFps":      "30",
    "colorWidth":    "1280",
    "colorHeight":   "720",
    "enableColor":   "true",
    "preset":        "HighAccuracy",
    "triggedType":   "1",
    "OnceSignalsGetImageCounts": "1",
    "OnceImageCounts":"2",
    "GetOnceImageTimes": "3000",
    "FirstState":    "true"
})");

struct OnePb
{
    PbGlobalObject* base = nullptr;
    QWidget* baseWidget = nullptr;
    QString DeviceSn;
};
QMap<QString, OnePb> TotalMap;

static cv::Mat frame_to_mat(const rs2::frame& f)
{
    using namespace cv;
    using namespace rs2;
    auto vf = f.as<video_frame>();
    int w = vf.get_width(), h = vf.get_height();
    if (f.get_profile().format() == RS2_FORMAT_BGR8)
        return Mat(Size(w, h), CV_8UC3, (void*)f.get_data(), Mat::AUTO_STEP);
    if (f.get_profile().format() == RS2_FORMAT_RGB8) {
        Mat r(Size(w, h), CV_8UC3, (void*)f.get_data(), Mat::AUTO_STEP);
        Mat b; cvtColor(r, b, COLOR_RGB2BGR); return b;
    }
    if (f.get_profile().format() == RS2_FORMAT_Z16)
        return Mat(Size(w, h), CV_16UC1, (void*)f.get_data(), Mat::AUTO_STEP);
    if (f.get_profile().format() == RS2_FORMAT_Y8)
        return Mat(Size(w, h), CV_8UC1, (void*)f.get_data(), Mat::AUTO_STEP);
    return Mat();
}

// ===== 构造 / 析构 =====

Hd_CameraModule_RealSense3::Hd_CameraModule_RealSense3(QString DeviceSn, QString RootPath,
                                                       int settype, QObject* parent)
    : PbGlobalObject(settype, parent)
{
    famliy = PGOFAMLIY::CAMERA3D;
    SnCode = DeviceSn;
    if (RootPath.endsWith('/') || RootPath.endsWith('\\'))
        JsonFilePath = RootPath + SnCode + ".json";
    else
        JsonFilePath = RootPath + "/" + SnCode + ".json";
    if (!QFile(JsonFilePath).exists())
        createAndWritefile(JsonFilePath, FirstCreateByte);
    QJsonObject obj = load_JsonFile(JsonFilePath);
    for (auto k : obj.keys())
        ParasValueMap.insert(k, obj[k].toString());
}

Hd_CameraModule_RealSense3::~Hd_CameraModule_RealSense3()
{
    qDebug() << "~Hd_CameraModule_RealSense3";
    closeCamera();
}

// ===== PbGlobalObject =====

bool Hd_CameraModule_RealSense3::setParameter(const QMap<QString, QString>& m)
{
    ParasValueMap = m;
    upDateParam();
    return true;
}

QMap<QString, QString> Hd_CameraModule_RealSense3::parameters()
{
    return ParasValueMap;
}

bool Hd_CameraModule_RealSense3::init()
{
    setParameter(ParasValueMap);
    connect(this, &PbGlobalObject::trigged, [=](int Code) {
        if (Code == 1000) {
            Currentindex = 0; MatQueue.clear();
            allowflag.store(true, std::memory_order_release);
            emit trigged(501);
        } else if (Code == 1001) {
            allowflag.store(false, std::memory_order_release);
        }
    });
    bool ok = initSdk();
    emit trigged(ok ? 0 : 1);
    type1 = triggedType;
    qDebug() << "RealSense init" << SnCode << ok << triggedType;
    return ok;
}

bool Hd_CameraModule_RealSense3::setData(const std::vector<cv::Mat>&, const QStringList& data)
{
    if (data.isEmpty()) {
        if (triggedType == 0)
            m_pendingCapture.store(true, std::memory_order_release);
        else if (triggedType == 1)
            MatQueue.clear();
        emit trigged(501);
    }
    return true;
}

bool Hd_CameraModule_RealSense3::data(std::vector<cv::Mat>& ImgS, QStringList&)
{
    MatQueue.wait_for_pop(timeOut, ImgS);
    if (ImgS.size() != (size_t)OnceGetImageNum) {
        qCritical() << "RealSense timeout, expected" << OnceGetImageNum << "got" << ImgS.size();
        return false;
    }
    return true;
}

void Hd_CameraModule_RealSense3::registerCallBackFun(PBGLOBAL_CALLBACK_FUN f, QObject* p, const QString& s)
{
    CallbackFuncPack t; t.callbackparent = p; t.cameraIndex = s; t.GetimagescallbackFunc = f;
    CallbackFuncMap.insert(s.toInt(), t);
}

void Hd_CameraModule_RealSense3::cancelCallBackFun(PBGLOBAL_CALLBACK_FUN f, QObject*, const QString& s)
{
    int idx = s.toInt();
    if (CallbackFuncMap.contains(idx) && CallbackFuncMap[idx].GetimagescallbackFunc == f)
        CallbackFuncMap.remove(idx);
}

// ===== SDK =====

void Hd_CameraModule_RealSense3::upDateParam()
{
    triggedType     = ParasValueMap.value("triggedType", "1").toInt();
    getImageMaxCoiunts = ParasValueMap.value("OnceSignalsGetImageCounts").toInt();
    OnceGetImageNum = ParasValueMap.value("OnceImageCounts").toInt();
    timeOut         = ParasValueMap.value("GetOnceImageTimes").toInt();
    allowflag       = ParasValueMap.value("FirstState") == "true";
}

bool Hd_CameraModule_RealSense3::initSdk()
{
    QMutexLocker l(&sdkMutex);
    isRunning.store(false, std::memory_order_release);

    rsPipeline = std::make_unique<rs2::pipeline>();
    rsConfig   = std::make_unique<rs2::config>();
    rsConfig->enable_device(SnCode.toStdString());

    int dw = ParasValueMap.value("depthWidth", "848").toInt();
    int dh = ParasValueMap.value("depthHeight", "480").toInt();
    int fps = ParasValueMap.value("depthFps", "30").toInt();
    rsConfig->enable_stream(RS2_STREAM_DEPTH, dw, dh, RS2_FORMAT_Z16, fps);

    if (ParasValueMap.value("enableColor", "true") == "true") {
        int cw = ParasValueMap.value("colorWidth", "1280").toInt();
        int ch = ParasValueMap.value("colorHeight", "720").toInt();
        rsConfig->enable_stream(RS2_STREAM_COLOR, cw, ch, RS2_FORMAT_BGR8, fps);
    }

    try {
        rsPipeline->start(*rsConfig, [this](const rs2::frame& f) { onFrameCallback(f); });
    } catch (const rs2::error& e) {
        qCritical() << "RealSense start failed:" << e.what();
        return false;
    }

    applySensorPreset(ParasValueMap.value("preset", "HighAccuracy"));
    isRunning.store(true, std::memory_order_release);
    qDebug() << "RealSense SDK ok, Sn:" << SnCode;
    return true;
}

void Hd_CameraModule_RealSense3::onFrameCallback(const rs2::frame& frame)
{
    if (!isRunning.load(std::memory_order_acquire))
        return;

    if (triggedType == 0) {
        if (!allowflag.load(std::memory_order_acquire)
            || !m_pendingCapture.load(std::memory_order_acquire))
            return;
        m_pendingCapture.store(false, std::memory_order_release);
    } else {
        if (!allowflag.load(std::memory_order_acquire))
            return;
    }
    rs2::frameset fs = frame.as<rs2::frameset>();
    if (!fs) return;
    rs2::depth_frame df = fs.get_depth_frame();
    rs2::video_frame cf = fs.get_color_frame();
    if (!df || !cf) return;

    cv::Mat dm = frame_to_mat(df);
    cv::Mat cm = frame_to_mat(cf);
    if (dm.empty() || cm.empty()) return;

    if (triggedType == 0) {
        QList<cv::Mat> imgs; imgs << dm.clone() << cm.clone();
        int base = Currentindex * OnceGetImageNum;
        for (int i = 0; i < OnceGetImageNum; i++) {
            int id = base + i + 1;
            if (CallbackFuncMap.contains(id)) {
                QObject* obj = CallbackFuncMap[id].callbackparent;
                obj->setProperty("cameraIndex", QString::number(id));
                QList<cv::Mat> s; s << imgs[i];
                CallbackFuncMap[id].GetimagescallbackFunc(obj, s);
            }
        }
        std::vector<cv::Mat> v; v.push_back(dm.clone()); v.push_back(cm.clone());
        MatQueue.push(v);
        emit trigged(501);
    } else {
        std::vector<cv::Mat> v; v.push_back(dm.clone()); v.push_back(cm.clone());
        MatQueue.push(v);
        emit trigged(501);
    }
    Currentindex++;
    if (Currentindex >= getImageMaxCoiunts / OnceGetImageNum) Currentindex = 0;
}

bool Hd_CameraModule_RealSense3::applySensorPreset(const QString& name)
{
    if (!rsPipeline || !isRunning.load(std::memory_order_acquire)) return false;

    static float presets[5];
    static bool init = false;
    if (!init) {
        presets[0] = RS2_RS400_VISUAL_PRESET_DEFAULT;
        presets[1] = RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY;
        presets[2] = RS2_RS400_VISUAL_PRESET_MEDIUM_DENSITY;
        presets[3] = RS2_RS400_VISUAL_PRESET_HIGH_DENSITY;
        presets[4] = RS2_RS400_VISUAL_PRESET_HAND;
        init = true;
    }
    int idx = 0;
    if (name == "HighAccuracy") idx = 1;
    else if (name == "MediumDensity") idx = 2;
    else if (name == "HighDensity") idx = 3;
    else if (name == "Hand") idx = 4;

    try {
        rs2::depth_sensor ds =
            rsPipeline->get_active_profile().get_device().first<rs2::depth_sensor>();
        if (ds.supports(RS2_OPTION_VISUAL_PRESET)) {
            ds.set_option(RS2_OPTION_VISUAL_PRESET, presets[idx]);
            qDebug() << "RealSense preset:" << name;
            return true;
        }
    } catch (const rs2::error&) {}
    qWarning() << "RealSense preset failed:" << name;
    return false;
}

// ===== closeCamera / checkStatus =====

bool Hd_CameraModule_RealSense3::closeCamera()
{
    isRunning.store(false, std::memory_order_release);
    QMutexLocker l(&sdkMutex);
    rsPipeline.reset();
    rsConfig.reset();
    MatQueue.clear();
    return true;
}

bool Hd_CameraModule_RealSense3::checkStatus()
{
    return isRunning.load(std::memory_order_acquire);
}

// ===== DLL =====

bool create(const QString& sn, const QString& name, const QString& path)
{
    OnePb t;
    t.base = new Hd_CameraModule_RealSense3(sn, path + "/Hd_CameraModule_RealSense3/");
    if (!t.base->init()) { delete t.base; return false; }
    t.baseWidget = new mPrivateWidget(t.base);
    t.DeviceSn = sn;
    TotalMap.insert(name.split(':').first(), t);
    return true;
}

void destroy(const QString& name)
{
    auto t = TotalMap.take(name);
    if (t.base)      { delete t.base;      t.base = nullptr; }
    if (t.baseWidget) { delete t.baseWidget; t.baseWidget = nullptr; }
}

QWidget* getCameraWidgetPtr(const QString& name) { return TotalMap.value(name).baseWidget; }
PbGlobalObject* getCameraPtr(const QString& name) { return TotalMap.value(name).base; }

QStringList getCameraSnList()
{
    QStringList r;
    rs2::context ctx;
    auto devs = ctx.query_devices();
    for (auto d : devs) {
        if (d.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) {
            QString sn = QString::fromStdString(d.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
            bool used = false;
            for (auto& t : TotalMap) { if (t.DeviceSn == sn) { used = true; break; } }
            if (!used) r << sn;
        }
    }
    return r;
}

// ===== UI =====

mPrivateWidget::mPrivateWidget(void* h)
{
    m_Camerahandle = reinterpret_cast<Hd_CameraModule_RealSense3*>(h);
    InitWidget();
}

void mPrivateWidget::InitWidget()
{
    QHBoxLayout* ml = new QHBoxLayout(this);
    QVBoxLayout* ll = new QVBoxLayout;

    depthView = new ImageViewer(this);
    colorView = new ImageViewer(this);
    softTriggerBtn = new QPushButton(QObject::tr("Soft Trigger"), this);
    allowGrabBtn   = new QPushButton(QObject::tr("Allow Grab"), this);
    stopGrabBtn    = new QPushButton(QObject::tr("Stop Grab"), this);
    presetCombo    = new QComboBox(this);
    presetCombo->addItems({"HighAccuracy","MediumDensity","HighDensity","Hand","Default"});

    m_AlgParmWidget = new AlgParmWidget(m_Camerahandle->GetRootPath());
    connect(m_AlgParmWidget, &AlgParmWidget::SengCurrentByte, this, [=](QByteArray b) {
        QJsonObject o = QJsonDocument::fromJson(b).object();
        QMap<QString,QString> m;
        for (auto k : o.keys()) m.insert(k, o[k].toString());
        m_Camerahandle->setParameter(m);
    });

    ll->addWidget(depthView);
    ll->addWidget(colorView);
    ll->addWidget(presetCombo);
    ll->addWidget(softTriggerBtn);
    ll->addWidget(allowGrabBtn);
    ll->addWidget(stopGrabBtn);
    ml->addLayout(ll, 4);
    ml->addWidget(m_AlgParmWidget, 3);

    connect(allowGrabBtn, &QPushButton::clicked, this, [=]() { emit m_Camerahandle->trigged(1000); });
    connect(stopGrabBtn,  &QPushButton::clicked, this, [=]() { emit m_Camerahandle->trigged(1001); });
    connect(softTriggerBtn, &QPushButton::clicked, this, [=]() {
        std::vector<cv::Mat> m; QStringList l;
        emit m_Camerahandle->trigged(1000);
        m_Camerahandle->setData(m, l);
        m_Camerahandle->data(m, l);
        if (m.size() >= 1) depthView->loadImage(QPixmap::fromImage(cvMatToQImage(m[0])));
        if (m.size() >= 2) colorView->loadImage(QPixmap::fromImage(cvMatToQImage(m[1])));
    });
}
