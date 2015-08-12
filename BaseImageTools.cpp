#include "BaseImageTools.h"


BaseImageTools::BaseImageTools()
{

}


BaseImageTools::~BaseImageTools()
{
}
 
Mat		BaseImageTools::ProduceAndOrMatrix(Mat and, Mat or)
{//改为直接处理吧：如果or的和》》and的3倍，就说明是光照来了。
	//使用and结果，否则使用or结果
	Mat result;// = and.clone();
	CvScalar sumand = cv::sum(and);
	CvScalar sumor = cv::sum(or);
	if (sumor.val[0] / sumand.val[0] > 4)
	{//光照
		result = and.clone();
	}
	else{
		result = or.clone();
	}
	imshow("result", result);
	return result;
}

Mat 	BaseImageTools::ProduceAndOrMatrix_1(Mat& and, Mat &or)
{//	或矩阵中，每个是255前景的点==》如果4的范围 and矩阵 没有一个=255 ，就判断为错误点。
	Mat result=and.clone();
	for (int i = 0; i < and.rows; i++)
	{
		int rowchange = 5; int colchange = 5;
		uchar * orptr = or.ptr<uchar>(i);
		uchar * resultptr = result.ptr<uchar>(i);
		for (int j = 0; j < and.cols; j++)
		{
			bool isright=false;
			if (orptr[j] == 255)
			{	
				for (int ki = -rowchange; ki < rowchange && isright==false; ki++)
				{//行上下
					if (ki + i >= 0 && ki + i < and.rows)
					{
						uchar * andptr = and.ptr<uchar>(ki + i);
						for (int k = -colchange; k < colchange; k++)
						{//列变换
							if (j + k >= 0 && j + k < and.cols)
							{
								if (andptr[j + k] == 255)
								{
									isright = true;
									break;
								}
							}
						}
					}
				}	
			}
			if (isright)
			{
				resultptr[j] = 255;
			}
		}
	}
	//imshow("result", result);
	return result;
}
VideoWriter BaseImageTools::NewWrite(int nWidth, int nHeight, char * fileName)
{
	//初始化 要保存的视频
	int fps = 30;
	CvSize size = cvSize(nWidth , nHeight );
	VideoWriter Writer(fileName, CV_FOURCC('M', 'J', 'P', 'G'), fps, size);

	return Writer;
}
boolean BaseImageTools::SaveColorImageToAVI(Mat ColorImage, int nWidth, int nHeight, VideoWriter   rgbWriter)
{
	Mat showImage;
	resize(ColorImage, showImage, Size(nWidth / 2, nHeight / 2));
//	imshow("ColorImage", showImage);////imshow("ColorImage", ColorImage);
	//这里都没有释放，============================================
	//这里要看看 要如何释放

	/*
	//记住 得到的图像是RGBA空间的，而保存视频只能是RGB的所以要转换
	//cvtcolor转换空间只支持从mat arr等转 ，所以要过渡一下。
	Mat  outputImage;
	cvtColor(showImage, outputImage, CV_RGBA2RGB);
	//colorImage是RGBA的空间， 要转到RGB
	IplImage originalImage(outputImage);


	int isWellWriter = 0;
	isWellWriter = cvWriteFrame(rgbWriter, &originalImage);//写入视频
	if (isWellWriter == 0)
	{
	printf("写入视频有误");
	return false;
	}
	return true;
*/
	rgbWriter << showImage;
	return true;
	
}
 boolean  BaseImageTools::SaveDepthImageToAVI(Mat DepthImage, int nWidth, int nHeight, VideoWriter  depthWriter)
{
	 depthWriter << DepthImage;
	 /*
	Mat show = DepthImage.clone();
	imshow("DepthImage", show);

	//这里都没有释放，============================================
	//这里要看看 要如何释放

	Mat  outputImage;
	cvtColor(DepthImage, outputImage, CV_RGBA2RGB);
	//colorImage是RGBA的空间， 要转到RGB
	IplImage originalImage(outputImage);

	int isWellWriter = 0;
	isWellWriter = cvWriteFrame(depthWriter, &originalImage);//写入视频
	if (isWellWriter == 0)
	{
		printf("写入视频有误");
		return false;
	}
	return true;
	*/
	 return true;
}