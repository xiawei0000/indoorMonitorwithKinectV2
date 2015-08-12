#pragma once
#include "opencv2/core/core.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/legacy/legacy.hpp"

using namespace  cv;
using namespace std;
class CodeBookBackGround
{
public:
	CodeBookBackGround();
	~CodeBookBackGround();
	Mat process(Mat inputMat, Mat &foreMatl);

	CvBGCodeBookModel* model ;
	const int NCHANNELS = 3;
	bool ch[3];// = { true, true, true };
	//bool ch[NCHANNELS] = { true, true, true };
	IplImage* rawImage;
	IplImage *yuvImage; //yuvImage is for codebook method
	IplImage *ImaskCodeBook;
	IplImage*ImaskCodeBookCC ;
	int nframesToLearnBG;
	int nframes ;
};

