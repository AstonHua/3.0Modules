#include "struct.h"
extern QImage cvMatToQImage(const cv::Mat& src)
{

	if (src.channels() == 1) { // if grayscale image
		return QImage((uchar*)src.data, src.cols, src.rows, static_cast<int>(src.step), QImage::Format_Grayscale8).copy();
	}
	if (src.channels() == 3) { // if 3 channel color image
		cv::Mat rgbMat;
		cv::cvtColor(src, rgbMat, cv::COLOR_BGR2RGB); // invert BGR to RGB
		return QImage((uchar*)rgbMat.data, src.cols, src.rows, static_cast<int>(src.step), QImage::Format_RGB888).copy();
	}
	return QImage();
}