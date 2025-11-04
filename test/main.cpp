#include <qlibrary.h>
#include <pbglobalobject.h>
#include <QApplication>
#include <qDebug>
#include <QWidget>
#include <qmainwindow.h>
using CreateFuncPtr = bool (*)(const QString&, const QString&, const QString&);
using destroyFuncPtr = void (*)(const QString& name);
using getCameraWidgetPtr =QWidget* (*)(const QString& name);
using getCameraPtr = PbGlobalObject* (*)(const QString& name);
using getCameraSnListPtr = QStringList(*)();
//QStringList getCameraSnList();
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);

	QLibrary* m_lib = new QLibrary("./Hd_CameraModule_3DLMI3.dll");
	bool flag = m_lib->load();
	if (!m_lib->isLoaded())
	{
		//qCritical() << "Loaded error:" << m_lib->errorString();
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
				createFun(list.first(), list.first(), list.first());
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