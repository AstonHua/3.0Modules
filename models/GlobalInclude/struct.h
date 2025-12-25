#pragma once
#include <imageView.h>
#include <QTableWidget>
#include <QDir>
#include <QTextCodec>
#include <QThread>
#include <QtConcurrent/qtconcurrent>
#include <qfuture.h>
#include <QVector>
 QImage cvMatToQImage(const cv::Mat& src);
 bool createAndWritefile(const QString& filename, const QByteArray& writeByte);
 QJsonObject load_JsonFile(QString filename);
 QJsonArray load_JsonArrayFile(QString filename);
 QString byteArrayToUnicode(const QByteArray array);