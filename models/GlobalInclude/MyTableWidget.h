#pragma once
#include <qwidget.h>
#include <QtWidgets/QApplication>
#include <qmainwindow.h>
#include <qtablewidget.h>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QToolButton>
#include <QFileDialog>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QObject>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QtCharts>
#include <iostream>
#pragma execution_character_set("utf-8")

class MyTableWidget : public QWidget
{
	Q_OBJECT;
signals:
	void SendCurrentResult(QByteArray);
private:

	QString currentFileName;
public:
	MyTableWidget(QWidget* parent, QString currentFileName)
		: QWidget(parent), currentFileName(currentFileName)
	{
		formulaTable = new QTableWidget(this);
		// 设置选中整行模式
		formulaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
		formulaTable->setSelectionMode(QAbstractItemView::ExtendedSelection); // 支持多选
		//this->setFocusPolicy(Qt::NoFocus);       // 禁用焦点（键盘导航仍可能触发）
		//this->setStyleSheet(
		//    "QTableWidget::item:focus {"
		//    "border: none;"           // 去掉选中项的边框
		//    "outline: none;"           // 去除虚线框（关键属性）
		//    "}"
		//);
		InitFormulaWidget();
		loadCsvToTable();
		formulaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	}
	QByteArray GetCurrentByte()
	{
		QJsonArray OutArray;
		int rowCount = formulaTable->rowCount();
		int colCount = formulaTable->columnCount();
		for (int row = 0; row < rowCount - 1; ++row) 
		{
			QStringList rowContents;
			for (int col = 0; col < colCount; ++col) 
			{
				QTableWidgetItem* item = formulaTable->item(row, col);
				QCheckBox* temp = (QCheckBox*)formulaTable->cellWidget(row, col);
				if (item) {
					rowContents << item->text();
				}
				else if (temp) {
					if (temp->isChecked())
						rowContents << "TRUE"; // 空单元格
					else
					{
						rowContents << "FALSE";
					}
				}
				else
				{
					rowContents << ""; // 空单元格
				}
			}
		
			OutArray << rowContents.join(","); // 每行用逗号分隔
		}
			return QJsonDocument(OutArray).toJson();
	}
protected:
	void keyPressEvent(QKeyEvent* event) override
	{
		if (event->key() == Qt::Key_Delete) {
			// 获取所有选中的行（可能有多个）
			QList<QTableWidgetSelectionRange> ranges = formulaTable->selectedRanges();

			// 从最后一行开始删除，避免行号变化导致的问题
			for (int i = ranges.count() - 1; i >= 0; --i) {
				int startRow = ranges.at(i).topRow();
				int endRow = ranges.at(i).bottomRow();
				if (endRow == formulaTable->rowCount() - 1)
					endRow -= 1;
				// 删除范围内的所有行
				for (int row = endRow; row >= startRow; --row) {
					formulaTable->removeRow(row);
				}
			}
		}
		else {
			// 其他按键交给父类处理
			QWidget::keyPressEvent(event);
		}
	}
	void dragEnterEvent(QDragEnterEvent* event) override {
		if (event->mimeData()->hasFormat("text/plain")) {
			event->acceptProposedAction(); // 接受文本数据
		}
	}
	void dragMoveEvent(QDragMoveEvent* event) override
	{
		if (event->mimeData()->hasText()) {
			event->acceptProposedAction();
		}
	}


private:
	void ClearTableData()
	{
		int Allcounts = formulaTable->rowCount();
		for (int row = Allcounts - 2; row >= 0; --row) {
			formulaTable->removeRow(row);
		}
	}

	//MyTableWidget* formulaTable = nullptr;
	bool loadCsvToTable()
	{

		QFile file(currentFileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QMessageBox::warning(this, tr("错误"), tr("无法打开文件: ") + currentFileName);
			return false;
		}
		ClearTableData();  // 清除已有内容
		QTextStream in(&file);
		int row = 0;

		while (!in.atEnd()) {
			QString line = in.readLine();
			QStringList fields = line.split(',');

			// 如果需要设置列数，可以在第一行时设置
			/*if (row == 0) {
				tableWidget->setColumnCount(fields.size());
			}*/

			formulaTable->insertRow(row);
			for (int col = 0; col < fields.size(); ++col) {

				if (col == fields.size() - 1)
				{
					QCheckBox* tempCHeck = new QCheckBox(formulaTable);

					if (fields[col] == "TRUE")
						tempCHeck->setCheckState(Qt::CheckState::Checked);
					else
					{
						tempCHeck->setCheckState(Qt::CheckState::Unchecked);
					}
					formulaTable->setCellWidget(row, col, tempCHeck);
				}
				else
				{
					formulaTable->setItem(row, col, new QTableWidgetItem(fields[col]));
				}
			}
			++row;
		}

		file.close();
		return true;
	}
	bool loadCsvToTable(QString filename)
	{

		QFile file(filename);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QMessageBox::warning(this, tr("错误"), tr("无法打开文件: ") + filename);
			return false;
		}
		ClearTableData();  // 清除已有内容
		QTextStream in(&file);
		int row = 0;

		while (!in.atEnd()) {
			QString line = in.readLine();
			QStringList fields = line.split(',');

			// 如果需要设置列数，可以在第一行时设置
			/*if (row == 0) {
				tableWidget->setColumnCount(fields.size());
			}*/

			formulaTable->insertRow(row);
			for (int col = 0; col < fields.size(); ++col) {

				if (col == fields.size() - 1)
				{
					QCheckBox* tempCHeck = new QCheckBox(formulaTable);

					if (fields[col] == "TRUE")
						tempCHeck->setCheckState(Qt::CheckState::Checked);
					else
					{
						tempCHeck->setCheckState(Qt::CheckState::Unchecked);
					}
					formulaTable->setCellWidget(row, col, tempCHeck);
				}
				else
				{
					formulaTable->setItem(row, col, new QTableWidgetItem(fields[col]));
				}
			}
			++row;
		}

		file.close();
		return true;
	}
	QToolButton* AddRow;
	QToolButton* SaveBtn;
	QToolButton* LoadProTypeBtn;
	QLabel* LastTipLabel;
	QLabel* OtherTipLabel;
	QTableWidget* formulaTable;
	void InitFormulaWidget()
	{
		OtherTipLabel = new QLabel(this);
		OtherTipLabel->setText(tr("注意:设定生效按钮点击时,当前界面设定生效,请在未生产时操作"));
		OtherTipLabel->setStyleSheet("color: rgb(255, 0, 0);");
		LastTipLabel = new QLabel(this);
		LastTipLabel->setText(tr("注意:键盘Ctrl+S，保存当前界面配置到默认文件,程序重新启动时以默认配置为基准"));
		LastTipLabel->setStyleSheet("color: rgb(0, 0, 255);");
		LoadProTypeBtn = new QToolButton(this);
		LoadProTypeBtn->setText(tr("加载其他参数文件"));
		connect(LoadProTypeBtn, &QToolButton::clicked, this, [this]() {
			QString CsvPath = QFileDialog::getOpenFileName(this, tr("选择产品参数文件夹"), "./config/", "*.csv");
			if (CsvPath.isEmpty())
			{
				QMessageBox::warning(this, tr("警告"), tr("无效路径"));
				return;
			}
			loadCsvToTable(CsvPath);
			});

		QHBoxLayout* buttonLayout = new QHBoxLayout();
		buttonLayout->addWidget(LastTipLabel);
		buttonLayout->addWidget(LoadProTypeBtn);
		QStringList headList;
		headList << tr("参数名") << tr("参数值") << tr("是否启用");
		formulaTable->setColumnCount(headList.size());
		AddRow = new QToolButton(this);
		SaveBtn = new QToolButton(this);
		AddRow->setText(tr("添加一行"));
		SaveBtn->setText(tr("设定生效"));
		formulaTable->setHorizontalHeaderLabels(headList);

		formulaTable->setRowCount(formulaTable->rowCount() + 1);
		int currentRow = formulaTable->rowCount();
		formulaTable->setCellWidget(currentRow - 1, 0, AddRow);
		formulaTable->setCellWidget(currentRow - 1, 1, SaveBtn);
		connect(SaveBtn, &QToolButton::clicked, this, [=]() {

			QJsonArray OutArray;
			int rowCount = formulaTable->rowCount();
			int colCount = formulaTable->columnCount();
			for (int row = 0; row < rowCount - 1; ++row) {
				QStringList rowContents;
				for (int col = 0; col < colCount; ++col) {
					QTableWidgetItem* item = formulaTable->item(row, col);
					QCheckBox* temp = (QCheckBox*)formulaTable->cellWidget(row, col);
					if (item) {
						rowContents << item->text();
					}
					else if (temp) {
						if (temp->isChecked())
							rowContents << "TRUE"; // 空单元格
						else
						{
							rowContents << "FALSE";
						}
					}
					else
					{
						rowContents << ""; // 空单元格
					}
				}
				OutArray << rowContents.join(","); // 每行用逗号分隔
			}
			emit SendCurrentResult(QJsonDocument(OutArray).toJson());
			});
		connect(AddRow, &QToolButton::clicked, this, [=]() {

			int buttonRow = formulaTable->rowCount() - 1;
			formulaTable->insertRow(buttonRow);
			formulaTable->removeCellWidget(formulaTable->rowCount() - 2, 0);
			int currentRow = formulaTable->rowCount();
			QCheckBox* Second = new QCheckBox(this);

			// 将项添加到表格

			formulaTable->setCellWidget(currentRow - 2, formulaTable->columnCount() - 1, Second);
			formulaTable->setCellWidget(currentRow - 1, 0, AddRow);
			formulaTable->setCellWidget(currentRow - 1, 1, SaveBtn);
			//formulaTable->setCellWidget(currentRow - 1, 3, LastTipLabel);
			});

		QShortcut* saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
		connect(saveShortcut, &QShortcut::activated, this, [=]() {
			QFile file(currentFileName);
			if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
				qDebug() << "Cannot open file for writing!";
				QMessageBox::warning(this, tr("警告"), tr("文件") + currentFileName + tr("打开失败，保存失败，请关闭配置文件"));
				return false;
			}
			QTextStream out(&file);

			int rowCount = formulaTable->rowCount();
			int colCount = formulaTable->columnCount();
			for (int row = 0; row < rowCount - 1; ++row) {
				QStringList rowContents;
				for (int col = 0; col < colCount; ++col) {
					QTableWidgetItem* item = formulaTable->item(row, col);
					QCheckBox* temp = (QCheckBox*)formulaTable->cellWidget(row, col);
					if (item) {
						rowContents << item->text();
					}
					else if (temp) {
						if (temp->isChecked())
							rowContents << "TRUE"; // 空单元格
						else
						{
							rowContents << "FALSE";
						}
					}
					else
					{
						rowContents << ""; // 空单元格
					}
				}
				out << rowContents.join(",") << "\n"; // 每行用逗号分隔
			}

			file.close();
			QMessageBox::information(this, tr("提示"), tr("文件") + currentFileName + tr("保存成功"));

			//qDebug() << "Table data saved to" << filename;
			return true;
			});
		QVBoxLayout* SignalAlgConfigLayout = new QVBoxLayout(this);
		SignalAlgConfigLayout->addWidget(formulaTable);
		SignalAlgConfigLayout->addWidget(OtherTipLabel);
		SignalAlgConfigLayout->addLayout(buttonLayout);
		this->setLayout(SignalAlgConfigLayout);
	}
};