#pragma once
#ifndef AlgParm_H
#define AlgParm_H
#include <QGroupBox>
#include "qjsonmodel.h"
#include <QWidget>
#include <qlayout.h>
#include <qtreeview.h>
#include <qfile.h>
#include <qtoolbutton.h>
#include <qtextedit.h>
#include <qtexttable.h>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <qsplitter.h>
#include <qfont.h>
#include <qstringlist.h>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QRegExp>
#include <QComboBox>
#include <qfiledialog.h>
#include <qdebug.h>
#include <QLabel>
#pragma execution_character_set("utf-8")

class TextEditSelf : public QWidget
{
	Q_OBJECT;
public:
	TextEditSelf();
	~TextEditSelf();
	void setJsonByte(QByteArray);
	void highlightText(const QString& textToHighlight);
	QString text() const;
private:
	QLineEdit* beforeLine = nullptr;
	QLineEdit* behandLine = nullptr;
	QToolButton* findButton = nullptr;
	QTextEdit* textEdit = nullptr;
signals:
	void jsonchange(QByteArray);
};
class modelTreeView : public QWidget
{
	Q_OBJECT;
public:
	modelTreeView();
	~modelTreeView();
	void loadJson(QByteArray);
	QJsonDocument json() const;
	void reset();
private:
	QTreeView * view = nullptr;
	QJsonModel* model = nullptr;
	QLineEdit * before = nullptr;
	QLineEdit * behand = nullptr;
	QToolButton* action = nullptr;
	QComboBox* m_comBox = nullptr;
	QLabel* lmitLabel = nullptr;
	QComboBox* lmit_comBox = nullptr;
	QLineEdit* lmit_Edit = nullptr;
signals:
	void jsonchange(QByteArray);

};
class AlgParmWidget : public QWidget
{
	Q_OBJECT;
signals:
	void SengCurrentByte(QByteArray);
    void save();
public:
	AlgParmWidget(std::shared_ptr<QByteArray>,QString currentFilePath);
	AlgParmWidget( QString currentFilePath);
	~AlgParmWidget ();
	inline void SetFilePath(const QString& path) { currentFilePath = path; }
	void reLoadByte(QByteArray);
private:
	void init(QByteArray);
	QString currentFilePath;
	QByteArray treeViewByte;
	QByteArray TextByte;
	QMessageBox tmpbox;
	modelTreeView * m_modelTreeView = nullptr;
	TextEditSelf* m_TextEdit = nullptr;
	std::shared_ptr<QByteArray>	QByteArrayPtr;
};

#endif // !AlgParm_H
