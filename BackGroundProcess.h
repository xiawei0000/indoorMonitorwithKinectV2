#pragma once
#include <opencv2\opencv.hpp>//用这个 +opencv文件夹 和opencv2就可以了
#include "BaseImageTools.h"
#include<iomanip>

#include <iostream>
#include <opencv2/opencv.hpp>
 
#include "package_bgs/MixtureOfGaussianV1BGS.h"
#include "package_bgs/MixtureOfGaussianV2BGS.h"
 

#if CV_MAJOR_VERSION >= 2 && CV_MINOR_VERSION >= 4 && CV_SUBMINOR_VERSION >= 3
#include "package_bgs/GMG.h"
#endif



#include "package_bgs/lb/LBSimpleGaussian.h"
#include "package_bgs/lb/LBMixtureOfGaussians.h"


// The PBAS algorithm was removed from BGSLibrary because it is
// based on patented algorithm ViBE
// http://www2.ulg.ac.be/telecom/research/vibe/
//#include "package_bgs/pt/PixelBasedAdaptiveSegmenter.h"
#include "package_bgs/av/VuMeter.h"


#include "opencv2/core/core.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/legacy/legacy.hpp"


#include "CodeBookBackGround.h"


using namespace std;
using namespace cv;

class BackGroundProcess
{
public:
	BackGroundProcess();
	//BackGroundProcess(Mat inputMat,Mat outputMat);
	~BackGroundProcess();
	
	cv::BackgroundSubtractor backtractor;
	cv::BackgroundSubtractorGMG mog;
	cv::BackgroundSubtractorMOG2 mog2;
	cv::BackgroundSubtractorGMG mg;

	int numberOfFrame;
	cv::Mat process_Mog(Mat inputMat, Mat &outputMat);//用高斯方法1
	Mat process_Mog2(Mat inputMat, Mat &outputMat);//用高斯方法2 快速
	Mat process_codeBook(Mat inputMat, Mat &outputMat);
	Mat testManyMethod(Mat inputMat, Mat &outputMat);//测试各种方法
	void setMyMethod();

	Mat depthProcessBackGround(Mat inputMat, Mat &outputMat);
	

	//colorprocess

	CodeBookBackGround codeBook;
	Mat   colorProcessBackGround(Mat inputMat, Mat &outputMat);
	

	IBGS * depthProcess;
	IBGS * colorProcess;

	IBGS *bestbgs;
	IBGS *bgs1;
	IBGS *bgs2;
	IBGS *bgs3;
	IBGS *bgs4;
	IBGS *bgs5;
	IBGS *bgs6;
	IBGS *bgs7;
};

