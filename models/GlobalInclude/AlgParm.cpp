#include "AlgParm.h"
AlgParmWidget::AlgParmWidget(QString Path):currentFilePath(Path)
{

	QFile file(currentFilePath);
	file.open(QIODevice::ReadOnly);
	QByteArray byte = file.readAll();
	file.close();
	m_modelTreeView = new modelTreeView;
	m_modelTreeView->loadJson(byte);
	m_TextEdit = new TextEditSelf;
	m_TextEdit->setJsonByte(byte);

	QVBoxLayout* topVBboxLayout = new QVBoxLayout;
	QHBoxLayout* firstHbox = new QHBoxLayout;
	QSplitter* m_spliter = new QSplitter;
	m_spliter->addWidget(m_modelTreeView);
	m_spliter->addWidget(m_TextEdit);
	firstHbox->addWidget(m_spliter);
	topVBboxLayout->addLayout(firstHbox);

	QHBoxLayout* underHbox = new QHBoxLayout;

	QToolButton* saveButton = new QToolButton;
	QToolButton* flushButton = new QToolButton;
	flushButton->setText(tr("刷新"));
	saveButton->setText(tr("保存"));
	underHbox->addWidget(flushButton);

	underHbox->addWidget(saveButton);
	topVBboxLayout->addLayout(underHbox);
	connect(flushButton, &QToolButton::clicked, this, [=]() {
		//m_modelTreeView->reset();
		QByteArray treebyte = m_modelTreeView->json().toJson();

		if (treeViewByte != treebyte)
		{
			treeViewByte = treebyte;
			m_modelTreeView->loadJson(treebyte);
			emit m_modelTreeView->jsonchange(treebyte);
		}
		QByteArray textByte = m_TextEdit->text().toUtf8();
		if (TextByte != textByte)
		{
			TextByte = textByte;
			//m_TextEdit->setJsonByte(TextByte);
			emit m_TextEdit->jsonchange(textByte);
			//treebyte = m_modelTreeView->json().toJson();
		}
		//if (treeViewByte == treebyte && TextByte == textByte)
		//{
		//	emit m_modelTreeView->jsonchange(treebyte);
		//}
		});
	connect(saveButton, &QToolButton::clicked, this, [=]()
		{

			tmpbox.setIcon(QMessageBox::Information);
			tmpbox.setText(tr("是否备份"));
			tmpbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
			tmpbox.setWindowTitle(QString(tr("备份确认")));
			tmpbox.setButtonText(QMessageBox::Yes, QString(tr("是")));
			tmpbox.setButtonText(QMessageBox::No, QString(tr("否")));
			tmpbox.setButtonText(QMessageBox::Cancel, QString(tr("取消")));
			int res = tmpbox.exec();

			if (res == QMessageBox::Yes)
			{
				QString name = QFileDialog::getSaveFileName(nullptr, tr("保存json文件"), "./", "(*.json)");
				QFile file(currentFilePath);
				file.copy(name);
				QMessageBox::information(this, tr("提示"), tr("备份成功"));
				QFile JsonFile(currentFilePath);
				JsonFile.open(QIODevice::WriteOnly);
				QTextStream out(&JsonFile);
				out.setCodec("UTF-8"); // 设置输出编码为UTF-8
				QByteArray writeArray = m_modelTreeView->json().toJson();
				out << writeArray;
				//JsonFile.write(m_modelTreeView->json().toJson());
				JsonFile.close();
				emit SengCurrentByte(writeArray);

				QMessageBox::information(this, tr("提示"), tr("保存成功"));
				return;
			}
			else if (res == QMessageBox::No)
			{
				QFile JsonFile(currentFilePath);
				JsonFile.open(QIODevice::WriteOnly);

				QTextStream out(&JsonFile);
				out.setCodec("UTF-8"); // 设置输出编码为UTF-8
				QByteArray writeArray = m_modelTreeView->json().toJson();
				out << writeArray;
				//JsonFile.write(m_modelTreeView->json().toJson());
				JsonFile.close();
				emit SengCurrentByte(writeArray);
				QMessageBox::information(this, tr("提示"), tr("保存成功"));
				
				return;
			}
			else if (res == QMessageBox::Cancel)
			{
				QMessageBox::warning(this, tr("提示"), tr("未保存"));
			}

		});

	connect(m_TextEdit, &TextEditSelf::jsonchange, this, [=](QByteArray byte) {
		m_modelTreeView->loadJson(byte);
		});
	connect(m_modelTreeView, &modelTreeView::jsonchange, this, [=](QByteArray byte) {
		m_TextEdit->setJsonByte(byte);
		});
	this->setLayout(topVBboxLayout);
}

AlgParmWidget::~AlgParmWidget()
{
}
void TextEditSelf::highlightText(const QString& textToHighlight) {
	QTextDocument* document = textEdit->document();
	QTextCharFormat format;
	format.setForeground(Qt::red); // 设置字体颜色为红色

	QRegExp expression(textToHighlight);
	int offset = 0;

	// 循环查找所有匹配的字符串
	while (offset >= 0) {
		offset = expression.indexIn(document->toPlainText(), offset);
		if (offset >= 0) {
			int length = expression.matchedLength();
			QTextCursor cursor = textEdit->textCursor();
			cursor.setPosition(offset, QTextCursor::MoveAnchor);
			cursor.setPosition(offset + length, QTextCursor::KeepAnchor);
			cursor.setCharFormat(format);
			offset += length;
		}
	}
}
QString TextEditSelf::text() const
{
	return textEdit->toPlainText();

}
TextEditSelf::TextEditSelf()
{

	beforeLine = new QLineEdit;
	behandLine = new QLineEdit;
	findButton = new QToolButton;
	beforeLine->setPlaceholderText(tr("查询"));
	behandLine->setPlaceholderText(tr("替换"));
	findButton->setText(tr("执行"));
	QHBoxLayout* topHboxLayout = new QHBoxLayout;
	topHboxLayout->addWidget(beforeLine);
	topHboxLayout->addWidget(behandLine);
	topHboxLayout->addWidget(findButton);
	textEdit = new QTextEdit;
	textEdit->setStyleSheet("QTextEdit{border:none;}");

	textEdit->verticalScrollBar()->setSingleStep(10); // 设置单步长
	textEdit->verticalScrollBar()->setPageStep(50); // 设置页步长
	textEdit->verticalScrollBar()->setValue(50); // 设置滚动条的值

	// 设置水->平滚动条的属性

	textEdit->horizontalScrollBar()->setSingleStep(10);
	textEdit->horizontalScrollBar()->setPageStep(80);
	textEdit->horizontalScrollBar()->setValue(40);
	QVBoxLayout* topVBox = new QVBoxLayout;

	topVBox->addWidget(textEdit);
	topVBox->addLayout(topHboxLayout);
	connect(findButton, &QToolButton::clicked, this, [=]() {


		QString text = textEdit->toPlainText();
		if (beforeLine->text().isEmpty() && behandLine->text().isEmpty())
		{
			QMessageBox::warning(this, tr("提示"), tr("查找或替换字符为空"));
			return;
		}
		if (!beforeLine->text().isEmpty() && behandLine->text().isEmpty())
		{
			QString beforeText = beforeLine->text();
			QTextDocument* document = textEdit->document();
			QTextCharFormat format;
			format.setForeground(Qt::red); // 设置字体颜色为红色
			QTextCursor cursor = textEdit->textCursor();
			QRegExp expression(beforeText);
			int start = cursor.selectionStart();
			int length = beforeText.length();
			// 循环查找所有匹配的字符串

			int currentoffset = expression.indexIn(document->toPlainText(), start + length);
			if (currentoffset == -1)
			{
				cursor.setPosition(QTextCursor::Start);
				textEdit->setTextCursor(cursor);
				return;
			}


			cursor.setPosition(currentoffset, QTextCursor::MoveAnchor);
			cursor.setPosition(currentoffset + length, QTextCursor::KeepAnchor);
			cursor.setCharFormat(format);
			textEdit->setTextCursor(cursor);



			//QString beforeText = beforeLine->text();
			//textEdit->setFocus();
			////highlightText(beforeText);
			//QTextCursor cursor = textEdit->textCursor();
			//// 将光标移动到文本的开始
			////cursor.movePosition(QTextCursor::Start);
			//cursor.setPosition(text.indexOf(beforeLine->text()));
			//// 将更改应用到QTextEdit
			//textEdit->setTextCursor(cursor);
			return;
		}
		if (text.contains(beforeLine->text()))
		{
			text.replace(beforeLine->text(), behandLine->text());
			textEdit->setPlainText(text);
			QMessageBox::information(this, tr("提示"), tr("执行成功"));
			emit jsonchange(text.toUtf8());
		}
		else
		{
			QMessageBox::warning(this, tr("提示"), tr("替换字符不存在"));
		}
		});
	this->setLayout(topVBox);
}

TextEditSelf::~TextEditSelf()
{
	if (beforeLine)
	{
		delete beforeLine;
		beforeLine = nullptr;
	}
	if (behandLine)
	{
		delete behandLine;
		behandLine = nullptr;
	}
	if (findButton)
	{
		delete findButton;
		findButton = nullptr;
	}
	if (textEdit)
	{
		delete textEdit;
		textEdit = nullptr;
	}
}

void TextEditSelf::setJsonByte(QByteArray byte)
{
	if (textEdit)
	{
		textEdit->clear();
		QFont font;
		font.setFamily("微软雅黑"); // 设置字体名称
		font.setPointSize(9);            // 设置字号
		//font.setBold(true);			//加粗
		textEdit->setStyleSheet("color:black;");
		textEdit->setStyleSheet("border:none;");
		textEdit->setFont(font);
		textEdit->setText(byte);

	}
}

modelTreeView::modelTreeView()
{
	view = new QTreeView;
	view->setStyleSheet("QTreeView{border:none;}");
	model = new QJsonModel;
	view->setModel(model);
	m_comBox = new QComboBox;
	before = new QLineEdit;
	before->setPlaceholderText(tr("索引值"));
	behand = new QLineEdit;
	behand->setPlaceholderText(tr("替换值"));
	action = new QToolButton;
	action->setText(tr("替换"));

	lmitLabel = new QLabel;
	lmitLabel->setText(tr("限定值"));
	lmit_comBox = new QComboBox;
	lmit_Edit = new QLineEdit;
	lmit_Edit->setPlaceholderText(tr("限定值"));
	QHBoxLayout* HboxLayout = new QHBoxLayout;

	HboxLayout->addWidget(m_comBox);
	HboxLayout->addWidget(before);
	HboxLayout->addWidget(behand);
	HboxLayout->addWidget(action);

	HboxLayout->addWidget(lmitLabel);
	HboxLayout->addWidget(lmit_comBox);
	HboxLayout->addWidget(lmit_Edit);
	QVBoxLayout* TopVbox = new QVBoxLayout;
	TopVbox->addWidget(view);
	TopVbox->addLayout(HboxLayout);
	connect(action, &QToolButton::clicked, this, [=]() {
		if (before->text().isEmpty() || behand->text().isEmpty())
		{
			QMessageBox::warning(this, tr("提示"), tr("参数有误"));
			return;
		}

		if (lmit_Edit->text().isEmpty())
		{
			QByteArray array = model->changstr(m_comBox->currentText(), before->text(), behand->text()).toJson();
			view->reset();
			emit jsonchange(array);
		}
		else
		{
			QByteArray array = model->Lmitchangstr(m_comBox->currentText(), before->text(), behand->text(), lmit_comBox->currentText(), lmit_Edit->text()).toJson();
			view->reset();
			emit jsonchange(array);
		}

		QMessageBox::information(this, tr("提示"), tr("响应成功"));
		return;

		});

	this->setLayout(TopVbox);
}

modelTreeView::~modelTreeView()
{
}

void modelTreeView::loadJson(QByteArray byte)
{
	model->loadJson(byte);
	//view->reset();
	QStringList mmm = model->genAllKey();
	/*qDebug() << mmm << m_comBox->count();
	for ( int i = 0; i < m_comBox->count(); i++)
	{
		m_comBox->removeItem(i);
	}*/

	m_comBox->clear();
	lmit_comBox->clear();
	for (auto &str : mmm)
	{
		m_comBox->addItem(str);
		lmit_comBox->addItem(str);
	}

}

QJsonDocument modelTreeView::json() const
{
	if (model)
	{
		return model->json();
	}
}

void modelTreeView::reset()
{
	view->reset();
}
