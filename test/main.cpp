#include <qlibrary.h>
#include <pbglobalobject.h>
#include <QApplication>
#include <qDebug>
#include <QWidget>
#include <qmainwindow.h>
#include <QTextCodec>
#include <QFile>
#include <Windows.h>
using CreateFuncPtr = bool (*)(const QString&, const QString&, const QString&);
using destroyFuncPtr = void (*)(const QString& name);
using getCameraWidgetPtr =QWidget* (*)(const QString& name);
using getCameraPtr = PbGlobalObject* (*)(const QString& name);
using getCameraSnListPtr = QStringList(*)();
//QStringList getCameraSnList();
bool loadDLLWithWindowsAPI() {
	// 将DLL路径转换为宽字符串
	LPCSTR dllPath = ".\\Hd_CameraModule_DaHua3.dll";

	// 使用LoadLibrary加载DLL
	HMODULE hDll = LoadLibrary(dllPath);

	if (hDll == NULL) {
		DWORD error = GetLastError();
		LPVOID errorMsg;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			0, // Default language
			(LPTSTR)&errorMsg,
			0,
			NULL
		);
		qDebug() << "LoadLibrary failed with error" << error << ":" <<errorMsg;
		LocalFree(errorMsg);
		return false;
	}
	else {
		qDebug() << "DLL loaded successfully!";
		FreeLibrary(hDll);
		return true;
	}
}
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	//QCoreApplication::addLibraryPath("./");
	//loadDLLWithWindowsAPI();
	//QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
	QLibrary* m_lib = new QLibrary("./Hd_CameraModule_HIK3.dll");
	bool flag = m_lib->load();
	if (!m_lib->isLoaded())
	{
		qCritical() << "Loaded error:" << m_lib->errorString();
		delete m_lib;
		m_lib = nullptr;
		return false;
	}
	QWidget* centerwidget = nullptr;
	if (flag)
	{
		try
		{
			CreateFuncPtr createFun = reinterpret_cast<CreateFuncPtr>(m_lib->resolve("create"));
			destroyFuncPtr destroyFunc = reinterpret_cast<destroyFuncPtr>(m_lib->resolve("destroy"));
			getCameraWidgetPtr getCameraWidget = reinterpret_cast<getCameraWidgetPtr>(m_lib->resolve("getCameraWidgetPtr"));
			getCameraPtr getCamera = reinterpret_cast<getCameraPtr>(m_lib->resolve("getCameraPtr"));
			getCameraSnListPtr getCameraSnList = reinterpret_cast<getCameraSnListPtr>(m_lib->resolve("getCameraSnList"));
			if (createFun)
			{
				QStringList list = getCameraSnList();
				createFun(list.first(), list.first()+":new", list.first());
				std::cout << "success";
				PbGlobalObject* ptr = getCamera(list.first());
				//bool flag = ptr->init();
				centerwidget = getCameraWidget(list.first());
				qDebug() << flag;
				//DataDealWithPtr = createFun();
				//if (CommunicationPtr)
				{
					//DataDealWithPtr->CommunicationPtr = CommunicationPtr;
				}
				//return DataDealWithPtr->initParas(QJsonDocument(obj.toObject().value("初始化数据").toObject()).toJson());
			}
		}
		catch (const std::exception&)
		{
			std::cout << "Exception occurred while loading function." << std::endl;
		}

		//ReleaseFunction releaseFun = (ReleaseFunction)m_lib->resolve("destory");
		
	}


	QMainWindow w;
	w.setCentralWidget(centerwidget);
	w.show();

	return a.exec();
}