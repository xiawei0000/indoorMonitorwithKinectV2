#include "BaseImageTools.h"


BaseImageTools::BaseImageTools()
{

}


BaseImageTools::~BaseImageTools()
{
}
 
Mat		BaseImageTools::ProduceAndOrMatrix(Mat and, Mat or)
{//��Ϊֱ�Ӵ���ɣ����or�ĺ͡���and��3������˵���ǹ������ˡ�
	//ʹ��and���������ʹ��or���
	Mat result;// = and.clone();
	CvScalar sumand = cv::sum(and);
	CvScalar sumor = cv::sum(or);
	if (sumor.val[0] / sumand.val[0] > 4)
	{//����
		result = and.clone();
	}
	else{
		result = or.clone();
	}
	imshow("result", result);
	return result;
}

Mat 	BaseImageTools::ProduceAndOrMatrix_1(Mat& and, Mat &or)
{//	������У�ÿ����255ǰ���ĵ�==�����4�ķ�Χ and���� û��һ��=255 �����ж�Ϊ����㡣
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
				{//������
					if (ki + i >= 0 && ki + i < and.rows)
					{
						uchar * andptr = and.ptr<uchar>(ki + i);
						for (int k = -colchange; k < colchange; k++)
						{//�б任
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
	//��ʼ�� Ҫ�������Ƶ
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
	//���ﶼû���ͷţ�============================================
	//����Ҫ���� Ҫ����ͷ�

	/*
	//��ס �õ���ͼ����RGBA�ռ�ģ���������Ƶֻ����RGB������Ҫת��
	//cvtcolorת���ռ�ֻ֧�ִ�mat arr��ת ������Ҫ����һ�¡�
	Mat  outputImage;
	cvtColor(showImage, outputImage, CV_RGBA2RGB);
	//colorImage��RGBA�Ŀռ䣬 Ҫת��RGB
	IplImage originalImage(outputImage);


	int isWellWriter = 0;
	isWellWriter = cvWriteFrame(rgbWriter, &originalImage);//д����Ƶ
	if (isWellWriter == 0)
	{
	printf("д����Ƶ����");
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

	//���ﶼû���ͷţ�============================================
	//����Ҫ���� Ҫ����ͷ�

	Mat  outputImage;
	cvtColor(DepthImage, outputImage, CV_RGBA2RGB);
	//colorImage��RGBA�Ŀռ䣬 Ҫת��RGB
	IplImage originalImage(outputImage);

	int isWellWriter = 0;
	isWellWriter = cvWriteFrame(depthWriter, &originalImage);//д����Ƶ
	if (isWellWriter == 0)
	{
		printf("д����Ƶ����");
		return false;
	}
	return true;
	*/
	 return true;
}