#include "CodeBookBackGround.h"


CodeBookBackGround::CodeBookBackGround()
{
	ch[1] = ch[2]=ch[0]=true;
	nframes = 1;
	nframesToLearnBG = 10;
	model = cvCreateBGCodeBookModel();

	//Set color thresholds to default values
	model->modMin[0] = 30;
		model->modMin[1] = model->modMin[2] = 20;
		model->modMax[0] = 35;
		model->modMax[1] = model->modMax[2] = 25;
		model->cbBounds[0] = 15;
		model->cbBounds[1] = model->cbBounds[2] = 10;// 用于确定码元各通道的阀值
}

Mat CodeBookBackGround::process(Mat inputMat, Mat &foreMat)
{
	Mat temp = inputMat.clone();
	IplImage * tempRGB  = &IplImage(temp);//
	IplImage * rawImage = cvCreateImage(cvGetSize(tempRGB), 8, 3);
	cvCvtColor(tempRGB, rawImage, CV_RGBA2BGR);
	if (nframes == 1 && !inputMat.empty())
	{

		yuvImage = cvCloneImage(rawImage);
		//对于mat到IplImage指针的转换，不能直接将mat的地址传给IplImage*；要用函数先转IplImage
		ImaskCodeBook = cvCreateImage(cvGetSize(yuvImage), IPL_DEPTH_8U, 1);
		ImaskCodeBookCC = cvCreateImage(cvGetSize(yuvImage), IPL_DEPTH_8U, 1);
		cvSet(ImaskCodeBook, cvScalar(0));

		//cvNamedWindow("Raw", 1);
	//	cvNamedWindow("ForegroundCodeBook", 1);
	//	cvNamedWindow("CodeBook_ConnectComp", 1);
	}
	else{


		cvCvtColor(rawImage, yuvImage, CV_BGR2YCrCb);//YUV For codebook method
		//This is where we build our background model
		if ( nframes - 1 < nframesToLearnBG)//这nframesToLearnBG帧之前都是跟新
			cvBGCodeBookUpdate(model, yuvImage);

		if (nframes - 1 == nframesToLearnBG)
			cvBGCodeBookClearStale(model, model->t / 2);//根据时间跟新

		//Find the foreground if any
		if (nframes - 1 >= nframesToLearnBG)
		{
			// Find foreground by codebook method
			cvBGCodeBookDiff(model, yuvImage, ImaskCodeBook);//判断是否在码本范围内

			// This part just to visualize bounding boxes and centers if desired
			cvCopy(ImaskCodeBook, ImaskCodeBookCC);
			cvSegmentFGMask(ImaskCodeBookCC);//对前景做连通域分割
		}
		//Display
	//	cvShowImage("Raw", rawImage);
	//	cvShowImage("ForegroundCodeBook", ImaskCodeBook);
	//	cvShowImage("CodeBook_ConnectComp", ImaskCodeBookCC);

	}

	foreMat = ImaskCodeBook;
	++nframes;

//	cout << "coodebook  第" << nframes << "帧   " << endl;

	return foreMat;
}


CodeBookBackGround::~CodeBookBackGround()
{
}
