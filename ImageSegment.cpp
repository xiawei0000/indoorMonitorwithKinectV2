#include "ImageSegment.h"


ImageSegment::ImageSegment()
{
}


ImageSegment::~ImageSegment()
{
}
cv::Mat ImageSegment::regiongrowth(const Mat &grayMat, const Mat &foreMat)
{//��ǰ��ͼ ɨ�裬 ��ǰ����
	/*1:ɨ��ǰ������==255�ģ� �����������в���255�ļ�������vector
		ͬʱ���������ʼ��Ϊ-1.  ǰ����Ϊ1��
	2��ѭ��ÿ�����ӣ��鿴����
		����� ==1������
		==0��������
		==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
		����  �ݶȡ�60 ����0 ��������-1.

		Ч��� ���������ӹ����ˡ�
	
	*/
	//cv::erode(outputMat, outputMat, Mat(), cv::Point(-1, -1));


	if ( grayMat.rows != foreMat. rows)
	{
		cout << "error !!!  grayMat.rows != foreMat. rows " << endl;
	}

	const int MinYuzhi = 3;
	const int MaxYuzhi = 25;

	Mat result = foreMat.clone();
	result.setTo(0);
	Mat mask = foreMat.clone();
	mask.setTo(3);//3��ʾδ���� 1ǰ�� 0����
	int rows = grayMat.rows;
	int cols = grayMat.cols;
	
	std::deque<cv::Point> seeddeque;// ����������棬 ֮���Ϊջ����


	//���˰��죬uchar�� 255 ����-1 ������ô����������
	int sss = 0;
	int ttt = 0;
	for (int i = 0; i < rows;i++)
	{
		const uchar * foreMatData = foreMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		for (int j = 0; j < cols; j++)
		{
			if (foreMatData[j] >=1)//== 255)
			{	sss++;
				maskData[j] = 1;
				resultData[j] = 255;
				if (j - 1 >= 0 && j + 1< cols && i - 1 > 0 && i + 1 < rows)
				{
					if (foreMatData[j - 1] == 0 || foreMatData[j + 1] == 0 || foreMat.at<uchar>(i + 1, j) == 0 || foreMat.at<uchar>(i - 1,j)==0 )
					{//һ��Ҫ��Χ�в���ǰ���ĵ�ż�������
						seeddeque.push_back(cv::Point(i,j));
					
					}else{
				
				}
				}
				
			}
			else{
				ttt++;
				maskData[j] = 3;
				resultData[j] = 0;
			}			
		}
	}
	cout << "ssss " << sss << "    tttt" << ttt << endl;

	while (!seeddeque.empty())
	{
		/*ѭ��ÿ�����ӣ��鿴����
		����� ==1������
		==0��������
		==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
		����  �ݶȡ�60 ����0 ��������-1.*/
		cv::Point  p = seeddeque.front();// �õ�����
		seeddeque.pop_front();//
		int i = p.x; int j = p.y;
		//const uchar * foreMatData = foreMat.ptr<uchar>(i);
		const uchar * grayMatData = grayMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		uchar grayCenter = grayMatData[j];
		//	uchar maskCenter = maskData[j];
		if (j - 1 >= 0 && maskData[j - 1] == 3)//���
		{
			if (abs(grayMatData[j - 1] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
			{
				maskData[j - 1] = 1;
				resultData[j - 1] = 255;
				seeddeque.push_back(cv::Point(i, j - 1));
			}
			else if (abs(grayMatData[j - 1] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
			{
				maskData[j - 1] = 0;
				resultData[j - 1] = 0;
			}
		}

		if (j + 1 < cols && maskData[j + 1] == 3)//�ұ�
		{
			if (abs(grayMatData[j + 1] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
			{
				maskData[j + 1] = 1;
				resultData[j + 1] = 255;
				seeddeque.push_back(cv::Point(i, j + 1));
			}
			else if (abs(grayMatData[j + 1] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
			{
				maskData[j + 1] = 0;
				resultData[j + 1] = 0;
			}
		}



		if (i - 1 >= 0)
		{
			const uchar  grayMatDatac1 = grayMat.ptr<uchar>(i - 1)[j];
			uchar * resultData1 = result.ptr<uchar>(i - 1);
			uchar * maskData1 = mask.ptr<uchar>(i - 1);
			if (maskData1[j] == 3)//�ϱ�
			{
				if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					maskData1[j] = 1;
					resultData1[j] = 255;
					seeddeque.push_back(cv::Point(i - 1, j));
				}
				else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData1[j] = 0;
					resultData1[j] = 0;
				}
			}
		}

		if (i + 1 < rows)
		{
			const uchar  grayMatDatac2 = grayMat.ptr<uchar>(i + 1)[j];
			uchar * resultData2 = result.ptr<uchar>(i + 1);
			uchar * maskData2 = mask.ptr<uchar>(i + 1);
			if (maskData2[j] == 3)//�±�
			{
				if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					maskData2[j] = 1;
					resultData2[j] = 255;
					seeddeque.push_back(cv::Point(i + 1, j));
				}
				else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData2[j] = 0;
					resultData2[j] = 0;
				}
			}
		}
	}
	return result;
}


cv::Mat ImageSegment::regiongrowthIndepth3(const Mat &grayMat,  Mat &foreMat)
{//��ǰ��ͼ ɨ�裬 ��ǰ����
	/*1:ɨ��ǰ������==255�ģ� �����������в���255�ļ�������vector
	ͬʱ���������ʼ��Ϊ-1.  ǰ����Ϊ1��
	2��ѭ��ÿ�����ӣ��鿴����
	����� ==1������
	==0��������
	==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
	����  �ݶȡ�60 ����0 ��������-1.

	Ч��� ���������޹����ˡ�

	*/
	cv::erode(foreMat, foreMat, Mat(), cv::Point(-1, -1),5);


	if (grayMat.rows != foreMat.rows)
	{
		cout << "error !!!  grayMat.rows != foreMat. rows " << endl;
	}

	const int MinYuzhi = 2;
	const int MaxYuzhi = 25;

	Mat result = foreMat.clone();
	result.setTo(0);
	Mat mask = foreMat.clone();
	mask.setTo(3);//3��ʾδ���� 1ǰ�� 0����
	int rows = grayMat.rows;
	int cols = grayMat.cols;

	std::deque<cv::Point> seeddeque;// ����������棬 ֮���Ϊջ����


	//���˰��죬uchar�� 255 ����-1 ������ô����������
	int sss = 0;
	int ttt = 0;
	for (int i = 0; i < rows; i++)
	{
		uchar * foreMatData = foreMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		for (int j = 0; j < cols; j++)
		{
			if (foreMatData[j] >= 1)//== 255)
			{
				sss++;
				maskData[j] = 1;
				resultData[j] = 255;
				if (j - 1 >= 0 && j + 1< cols && i - 1 > 0 && i + 1 < rows)
				{
					if (foreMatData[j - 1] == 0 || foreMatData[j + 1] == 0 || foreMat.at<uchar>(i + 1, j) == 0 || foreMat.at<uchar>(i - 1, j) == 0)
					{//һ��Ҫ��Χ�в���ǰ���ĵ�ż�������
						seeddeque.push_back(cv::Point(i, j));

					}
					else{

					}
				}

			}
			else{
				ttt++;
				maskData[j] = 3;
				resultData[j] = 0;
			}
		}
	}
	cout << "ssss " << sss << "    tttt" << ttt << endl;

	while (!seeddeque.empty())
	{
		/*ѭ��ÿ�����ӣ��鿴����
		����� ==1������
		==0��������
		==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
		����  �ݶȡ�60 ����0 ��������-1.*/
		cv::Point  p = seeddeque.front();// �õ�����
		seeddeque.pop_front();//
		int i = p.x; int j = p.y;
		//const uchar * foreMatData = foreMat.ptr<uchar>(i);
		const uchar * grayMatData = grayMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		uchar grayCenter = grayMatData[j*3];
		//	uchar maskCenter = maskData[j];
		if (j - 1 >= 0 && maskData[j - 1] == 3)//���
		{
			if (abs(grayMatData[(j - 1)*3] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
			{
				maskData[j - 1] = 1;
				resultData[j - 1] = 255;
				seeddeque.push_back(cv::Point(i, j - 1));
			}
			else if (abs(grayMatData[(j - 1)*3] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
			{
				maskData[j - 1] = 0;
				resultData[j - 1] = 0;
			}
		}

		if (j + 1 < cols && maskData[j + 1] == 3)//�ұ�
		{
			if (abs(grayMatData[(j + 1)*3] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
			{
				maskData[j + 1] = 1;
				resultData[j + 1] = 255;
				seeddeque.push_back(cv::Point(i, j + 1));
			}
			else if (abs(grayMatData[(j + 1)*3] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
			{
				maskData[j + 1] = 0;
				resultData[j + 1] = 0;
			}
		}



		if (i - 1 >= 0)
		{
			const uchar  grayMatDatac1 = grayMat.ptr<uchar>(i - 1)[j*3];
			uchar * resultData1 = result.ptr<uchar>(i - 1);
			uchar * maskData1 = mask.ptr<uchar>(i - 1);
			if (maskData1[j] == 3)//�ϱ�
			{
				if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					maskData1[j] = 1;
					resultData1[j] = 255;
					seeddeque.push_back(cv::Point(i - 1, j));
				}
				else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData1[j] = 0;
					resultData1[j] = 0;
				}
			}
		}

		if (i + 1 < rows)
		{
			const uchar  grayMatDatac2 = grayMat.ptr<uchar>(i + 1)[j*3];
			uchar * resultData2 = result.ptr<uchar>(i + 1);
			uchar * maskData2 = mask.ptr<uchar>(i + 1);
			if (maskData2[j] == 3)//�±�
			{
				if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					maskData2[j] = 1;
					resultData2[j] = 255;
					seeddeque.push_back(cv::Point(i + 1, j));
				}
				else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData2[j] = 0;
					resultData2[j] = 0;
				}
			}
		}
	}
	return result;
}




cv::Mat ImageSegment::regiongrowthIndepthWithmean(Mat & color,  Mat &depthMatIncolorSize, Mat &foreMat)
{//��ǰ��ͼ ɨ�裬 ��ǰ����
	/*1:ɨ��ǰ������==255�ģ� �����������в���255�ļ�������vector
	//���� depth�ľ�ֵ���㡣==����Ϊ ��������жϱ�׼��


	ͬʱ���������ʼ��Ϊ-1.  ǰ����Ϊ1��
	2��ѭ��ÿ�����ӣ��鿴����
	����� ==1������
	==0��������
	==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
	����  �ݶȡ�60 ����0 ��������-1.

	*/
	Mat grayMat;
	cv::cvtColor(color, grayMat, CV_BGR2GRAY);
	cv::erode(foreMat, foreMat, Mat(), cv::Point(-1, -1), 1);

	//Mat tempforeMat;
	//cv::threshold(foreMat, tempforeMat, 2);
	//��foremat��ֵ���� Ȼ�� gray ����+ �õ���ֵ��
//	imshow("grayMat", grayMat);
//	imshow("foremat", foreMat);
	cvWaitKey(1);
	Scalar cs = cv::mean(depthMatIncolorSize, foreMat);
	
	//CvScalar   cvAvg(const  CvA rr*  arr, const  CvA rr*  mask  =  NULL); 



	if (grayMat.rows != foreMat.rows)
	{
		cout << "error !!!  grayMat.rows != foreMat. rows " << endl;
	}

	const int MinYuzhi = 10;
	const int MaxYuzhi = 15;
	const int MinYuzhiDEPTH =10;
	Mat result = foreMat.clone();
	result.setTo(0);
	Mat mask = foreMat.clone();
	mask.setTo(3);//3��ʾδ���� 1ǰ�� 0����
	int rows = grayMat.rows;
	int cols = grayMat.cols;

	std::deque<cv::Point> seeddeque;// ����������棬 ֮���Ϊջ����


	//���˰��죬uchar�� 255 ����-1 ������ô����������
	int sss = 0;
	int ttt = 0;
	for (int i = 0; i < rows; i++)
	{
		uchar * foreMatData = foreMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		for (int j = 0; j < cols; j++)
		{
			if (foreMatData[j] >= 1)//== 255)
			{
				sss++;
				maskData[j] = 1;
				resultData[j] = 255;
				if (j - 1 >= 0 && j + 1< cols && i - 1 > 0 && i + 1 < rows)
				{
					if (foreMatData[j - 1] == 0 || foreMatData[j + 1] == 0 || foreMat.at<uchar>(i + 1, j) == 0 || foreMat.at<uchar>(i - 1, j) == 0)
					{//һ��Ҫ��Χ�в���ǰ���ĵ�ż�������
						seeddeque.push_back(cv::Point(i, j));

					}
					else{

					}
				}

			}
			else{
				ttt++;
				maskData[j] = 3;
				resultData[j] = 0;
			}
		}
	}
	cout << "ssss " << sss << "    tttt" << ttt << endl;


	int numberOfadd = 0;
	int tempdiff;
	while (!seeddeque.empty())
	{
		/*ѭ��ÿ�����ӣ��鿴����
		����� ==1������
		==0��������
		==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
		����  �ݶȡ�60 ����0 ��������-1.*/
		cv::Point  p = seeddeque.front();// �õ�����
		seeddeque.pop_front();//
		int i = p.x; int j = p.y;
		//const uchar * foreMatData = foreMat.ptr<uchar>(i);
		const uchar * grayMatData = grayMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		uchar grayCenter = grayMatData[j * 3];
		//	uchar maskCenter = maskData[j];
		if (j - 1 >= 0 && maskData[j - 1] == 3)//���
		{
			if (abs(grayMatData[(j - 1) * 3] - grayCenter) < MinYuzhi )//˵�����ݶ�С����������
			{
				tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i, j - 1)[0]);
				cout << "    " << tempdiff;
				if (tempdiff < MinYuzhiDEPTH)
				{
					maskData[j - 1] = 1;
					resultData[j - 1] = 255;
					seeddeque.push_back(cv::Point(i, j - 1));
					numberOfadd++;
				}
				
			}
			else if (abs(grayMatData[(j - 1) * 3] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
			{
				maskData[j - 1] = 0;
				resultData[j - 1] = 0;
			}
		}

		if (j + 1 < cols && maskData[j + 1] == 3)//�ұ�
		{
			if (abs(grayMatData[(j + 1) * 3] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
			{
				tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i, j + 1)[0]);
				cout << "    " << tempdiff;
				if (tempdiff  < MinYuzhiDEPTH)
				{
					maskData[j + 1] = 1;
					resultData[j + 1] = 255;
					seeddeque.push_back(cv::Point(i, j + 1));
					numberOfadd++;
				}
				
			}
			else if (abs(grayMatData[(j + 1) * 3] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
			{
				maskData[j + 1] = 0;
				resultData[j + 1] = 0;
			}
		}



		if (i - 1 >= 0)
		{
			const uchar  grayMatDatac1 = grayMat.ptr<uchar>(i - 1)[j * 3];
			uchar * resultData1 = result.ptr<uchar>(i - 1);
			uchar * maskData1 = mask.ptr<uchar>(i - 1);
			if (maskData1[j] == 3)//�ϱ�
			{
				
				if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i - 1, j)[0]);
					cout << "    " << tempdiff;
					if ( tempdiff< MinYuzhiDEPTH)
					{
						maskData1[j] = 1;
						resultData1[j] = 255;
						seeddeque.push_back(cv::Point(i - 1, j));
						numberOfadd++;
					}
					
				}
				else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData1[j] = 0;
					resultData1[j] = 0;
				}
			}
		}

		if (i + 1 < rows)
		{
			const uchar  grayMatDatac2 = grayMat.ptr<uchar>(i + 1)[j * 3];
			uchar * resultData2 = result.ptr<uchar>(i + 1);
			uchar * maskData2 = mask.ptr<uchar>(i + 1);
			if (maskData2[j] == 3)//�±�
			{
				if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i + 1, j)[0]);
					cout << "    " << tempdiff;
					if ( tempdiff< MinYuzhiDEPTH)
					{
						maskData2[j] = 1;
						resultData2[j] = 255;
						seeddeque.push_back(cv::Point(i + 1, j));
						numberOfadd++;
					}
					
				}
				else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData2[j] = 0;
					resultData2[j] = 0;
				}
			}
		}
	}


	cout <<"�޸ĵĸ��� -------------"<< numberOfadd<<endl;
	imshow("result", result);
	cvWaitKey(1);
	return result;
}


//��list���� ÿ���޲���󣬴���++ �� ���� 10�Ρ� 
cv::Mat ImageSegment::regiongrowthIndepthWithmeanInList(Mat & color, Mat &depthMatIncolorSize, Mat &foreMat)
{//��ǰ��ͼ ɨ�裬 ��ǰ����  
	/*1:ɨ��ǰ������==255�ģ� �����������в���255�ļ�������vector
	//���� depth�ľ�ֵ���㡣==����Ϊ ��������жϱ�׼��


	ͬʱ���������ʼ��Ϊ-1.  ǰ����Ϊ1��
	2��ѭ��ÿ�����ӣ��鿴����
	����� ==1������
	==0��������
	==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
	����  �ݶȡ�60 ����0 ��������-1.

	*/
	Mat grayMat;
	cv::cvtColor(color, grayMat, CV_BGR2GRAY);
	//cv::erode(foreMat, foreMat, Mat(), cv::Point(-1, -1), 1);

	//Mat tempforeMat;
	//cv::threshold(foreMat, tempforeMat, 2);
	//��foremat��ֵ���� Ȼ�� gray ����+ �õ���ֵ��
	//	imshow("grayMat", grayMat);
	//	imshow("foremat", foreMat);
	cvWaitKey(1);
	Scalar cs = cv::mean(depthMatIncolorSize, foreMat);

	//CvScalar   cvAvg(const  CvA rr*  arr, const  CvA rr*  mask  =  NULL); 



	if (grayMat.rows != foreMat.rows)
	{
		cout << "error !!!  grayMat.rows != foreMat. rows " << endl;
	}

	const int MinYuzhi = 15;
	const int MaxYuzhi = 25;
	const int MinYuzhiDEPTH = 15;
	Mat result = foreMat.clone();
	result.setTo(0);
	Mat mask = foreMat.clone();
	mask.setTo(3);//3��ʾδ���� 1ǰ�� 0����
	int rows = grayMat.rows;
	int cols = grayMat.cols;

	std::list<cv::Point> seedlist;// ����������棬 ֮���Ϊջ����
	std::list<cv::Point>:: iterator itercont;

	std::list<cv::Point> seedlistnew;
	std::list<cv::Point>::iterator intercontnew;


	std::list<cv::Point>::iterator interswap;
	//���˰��죬uchar�� 255 ����-1 ������ô����������
	int sss = 0;
	int ttt = 0;
	for (int i = 0; i < rows; i++)
	{
		uchar * foreMatData = foreMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		for (int j = 0; j < cols; j++)
		{
			if (foreMatData[j] >= 1)//== 255)
			{
				sss++;
				maskData[j] = 1;
				resultData[j] = 255;
				if (j - 1 >= 0 && j + 1< cols && i - 1 > 0 && i + 1 < rows)
				{
					if (foreMatData[j - 1] == 0 || foreMatData[j + 1] == 0 || foreMat.at<uchar>(i + 1, j) == 0 || foreMat.at<uchar>(i - 1, j) == 0)
					{//һ��Ҫ��Χ�в���ǰ���ĵ�ż�������
						seedlist.push_back(cv::Point(i, j));

					}
					else{

					}
				}

			}
			else{
				ttt++;
				maskData[j] = 3;
				resultData[j] = 0;
			}
		}
	}
	cout << "ssss " << sss << "    tttt" << ttt << endl;


	int numberOfadd = 0;
	int tempdiff;

	for (int times = 0; times < 10; times ++)
	{
		numberOfadd = 0;

		itercont = seedlist.begin();
		while (itercont != seedlist.end())
		{
			/*ѭ��ÿ�����ӣ��鿴����
			����� ==1������
			==0��������
			==-1. �жϣ��ݶȡ�30���������ӣ� mask��1
			����  �ݶȡ�60 ����0 ��������-1.*/
			cv::Point  p = *(itercont);// �õ�����
		
			int i = p.x; int j = p.y;
			//const uchar * foreMatData = foreMat.ptr<uchar>(i);
			const uchar * grayMatData = grayMat.ptr<uchar>(i);
			uchar * resultData = result.ptr<uchar>(i);
			uchar * maskData = mask.ptr<uchar>(i);

			uchar grayCenter = grayMatData[j * 3];
			//	uchar maskCenter = maskData[j];
			if (j - 1 >= 0 && maskData[j - 1] == 3)//���
			{
				if (abs(grayMatData[(j - 1) * 3] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i, j - 1)[0]);
					cout << "    " << tempdiff;
					if (tempdiff < MinYuzhiDEPTH)
					{
						maskData[j - 1] = 1;
						resultData[j - 1] = 255;
						seedlistnew.push_back(cv::Point(i, j - 1));
						numberOfadd++;
					}

				}
				else if (abs(grayMatData[(j - 1) * 3] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData[j - 1] = 0;
					resultData[j - 1] = 0;
				}
			}

			if (j + 1 < cols && maskData[j + 1] == 3)//�ұ�
			{
				if (abs(grayMatData[(j + 1) * 3] - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
				{
					tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i, j + 1)[0]);
					cout << "    " << tempdiff;
					if (tempdiff  < MinYuzhiDEPTH)
					{
						maskData[j + 1] = 1;
						resultData[j + 1] = 255;
						seedlistnew.push_back(cv::Point(i, j + 1));
						numberOfadd++;
					}

				}
				else if (abs(grayMatData[(j + 1) * 3] - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
				{
					maskData[j + 1] = 0;
					resultData[j + 1] = 0;
				}
			}



			if (i - 1 >= 0)
			{
				const uchar  grayMatDatac1 = grayMat.ptr<uchar>(i - 1)[j * 3];
				uchar * resultData1 = result.ptr<uchar>(i - 1);
				uchar * maskData1 = mask.ptr<uchar>(i - 1);
				if (maskData1[j] == 3)//�ϱ�
				{

					if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
					{
						tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i - 1, j)[0]);
						cout << "    " << tempdiff;
						if (tempdiff< MinYuzhiDEPTH)
						{
							maskData1[j] = 1;
							resultData1[j] = 255;
							seedlistnew.push_back(cv::Point(i - 1, j));
							numberOfadd++;
						}

					}
					else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
					{
						maskData1[j] = 0;
						resultData1[j] = 0;
					}
				}
			}

			if (i + 1 < rows)
			{
				const uchar  grayMatDatac2 = grayMat.ptr<uchar>(i + 1)[j * 3];
				uchar * resultData2 = result.ptr<uchar>(i + 1);
				uchar * maskData2 = mask.ptr<uchar>(i + 1);
				if (maskData2[j] == 3)//�±�
				{
					if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//˵�����ݶ�С����������
					{
						tempdiff = abs(cs[0] - depthMatIncolorSize.at<Vec3b>(i + 1, j)[0]);
						cout << "    " << tempdiff;
						if (tempdiff< MinYuzhiDEPTH)
						{
							maskData2[j] = 1;
							resultData2[j] = 255;
							seedlistnew.push_back(cv::Point(i + 1, j));
							numberOfadd++;
						}

					}
					else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//˵�����ݶ�̫ da
					{
						maskData2[j] = 0;
						resultData2[j] = 0;
					}
				}
			}

			itercont++;
		}

		seedlist.clear();

		interswap = seedlistnew.begin();

		while ( interswap != seedlistnew.end())
		{
			seedlist.push_back((*interswap));
			interswap++;
		}
		seedlistnew.clear();
		cout << "*******##############################################/n/n";
	 	cout << "��"<<times <<"�� �޸ĵĸ��� -------------" << numberOfadd << endl;

	}
	
	seedlistnew.clear();
	seedlist.clear();


	imshow("result", result);
	cvWaitKey(1);
	return result;
}