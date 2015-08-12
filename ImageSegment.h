#pragma once
#include <opencv2/opencv.hpp>

#include "opencv2/core/core.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/legacy/legacy.hpp"

#include <vector>
#include <deque>

using namespace std;
using namespace  cv;
class ImageSegment
{
public:
	ImageSegment();
	~ImageSegment();

	cv::Mat regiongrowth(const Mat &grayMat, const Mat &foreMat); 
	cv::Mat regiongrowthIndepth3(const Mat &grayMat,  Mat &foreMat);
	cv::Mat regiongrowthIndepthWithmean(Mat & color, Mat &depthMatIncolorSize, Mat &foreMat);
	cv::Mat regiongrowthIndepthWithmeanInList(Mat & color, Mat &depthMatIncolorSize, Mat &foreMat);

	

};

