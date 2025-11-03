#include <opencv.hpp>
#include <iostream>
using namespace cv;
using namespace std;


/**
 * @brief 安全裁剪图像中的矩形区域
 *
 * @param src 输入图像
 * @param roi 要裁剪的矩形区域
 * @return cv::Mat 裁剪后的图像，如果ROI无效则返回空矩阵
 */
cv::Mat safeCrop(const cv::Mat& src, const cv::Rect& roi) {
    // 检查源图像是否有效
    if (src.empty()) {
        std::cerr << "Error: Source image is empty!" << std::endl;
        return cv::Mat();
    }

    // 计算安全的ROI边界
    int x = std::max(roi.x, 0);
    int y = std::max(roi.y, 0);
    int width = std::min(roi.width, src.cols - x);
    int height = std::min(roi.height, src.rows - y);

    // 检查ROI是否有效
    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Invalid ROI (width: " << width
            << ", height: " << height << ")" << std::endl;
        return cv::Mat();
    }

    // 创建安全的ROI矩形
    cv::Rect safeRoi(x, y, width, height);

    // 执行裁剪
    return src(safeRoi).clone(); // 使用clone确保返回独立的内存
}
int main()
{
	cv::Mat readImage = cv::imread("E:\\localImage\\new1\\0.bmp");

    cv::Rect rect{10,10,2000,5000};

    cv::Mat safemat = safeCrop(readImage, rect)


	return 0;
}