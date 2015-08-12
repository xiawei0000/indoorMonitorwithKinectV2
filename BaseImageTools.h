#pragma once

#include <stdio.h>
#include <tchar.h>
//#include <windows.h>
#include <Kinect.h>// Kinect Header files
#include <opencv2\opencv.hpp>//用这个 +opencv文件夹 和opencv2就可以了
using namespace cv;
class BaseImageTools
{
private:
	static const int        cColorWidth = 1920;
	static const int        cColorHeight = 1080;
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;
public:
	BaseImageTools();
	~BaseImageTools();
	static VideoWriter NewWrite(int nWidth, int nHeight, char * fileName);
	static boolean		SaveColorImageToAVI(Mat ColorImage, int nWidth, int nHeight, VideoWriter   rgbWriter);
	static boolean		SaveDepthImageToAVI(Mat DepthImage, int nWidth, int nHeight, VideoWriter  depthWriter);
	static Mat			ProduceAndOrMatrix(Mat and,Mat or);
	//static Mat			ProduceAndOrMatrix_1(Mat and, Mat or);
	static Mat 		ProduceAndOrMatrix_1(Mat& and, Mat &or);
	
	CvVideoWriter *depthWriter;
	CvVideoWriter *rgbWriter;
};

