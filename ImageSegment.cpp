#include "ImageSegment.h"


ImageSegment::ImageSegment()
{
}


ImageSegment::~ImageSegment()
{
}
cv::Mat ImageSegment::regiongrowth(const Mat &grayMat, const Mat &foreMat)
{//从前景图 扫描， 是前景，
	/*1:扫描前景，将==255的， 且上下左右有不是255的加入种子vector
		同时建立码表，初始化为-1.  前景点为1，
	2：循环每个种子：查看四周
		查码表： ==1，跳过
		==0，跳过，
		==-1. 判断：梯度《30，加入种子， mask标1
		否则  梯度》60 ，标0 。其他标-1.

		效果差， 背景反而加过来了。
	
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
	mask.setTo(3);//3表示未处理 1前景 0背景
	int rows = grayMat.rows;
	int cols = grayMat.cols;
	
	std::deque<cv::Point> seeddeque;// 先用链表代替， 之后改为栈操作


	//高了半天，uchar中 255 就是-1 啊，怎么可能区分呢
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
					{//一定要周围有不是前景的点才加入种子
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
		/*循环每个种子：查看四周
		查码表： ==1，跳过
		==0，跳过，
		==-1. 判断：梯度《30，加入种子， mask标1
		否则  梯度》60 ，标0 。其他标-1.*/
		cv::Point  p = seeddeque.front();// 得到种子
		seeddeque.pop_front();//
		int i = p.x; int j = p.y;
		//const uchar * foreMatData = foreMat.ptr<uchar>(i);
		const uchar * grayMatData = grayMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		uchar grayCenter = grayMatData[j];
		//	uchar maskCenter = maskData[j];
		if (j - 1 >= 0 && maskData[j - 1] == 3)//左边
		{
			if (abs(grayMatData[j - 1] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
			{
				maskData[j - 1] = 1;
				resultData[j - 1] = 255;
				seeddeque.push_back(cv::Point(i, j - 1));
			}
			else if (abs(grayMatData[j - 1] - grayCenter) > MaxYuzhi)//说明是梯度太 da
			{
				maskData[j - 1] = 0;
				resultData[j - 1] = 0;
			}
		}

		if (j + 1 < cols && maskData[j + 1] == 3)//右边
		{
			if (abs(grayMatData[j + 1] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
			{
				maskData[j + 1] = 1;
				resultData[j + 1] = 255;
				seeddeque.push_back(cv::Point(i, j + 1));
			}
			else if (abs(grayMatData[j + 1] - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
			if (maskData1[j] == 3)//上边
			{
				if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
				{
					maskData1[j] = 1;
					resultData1[j] = 255;
					seeddeque.push_back(cv::Point(i - 1, j));
				}
				else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
			if (maskData2[j] == 3)//下边
			{
				if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
				{
					maskData2[j] = 1;
					resultData2[j] = 255;
					seeddeque.push_back(cv::Point(i + 1, j));
				}
				else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
{//从前景图 扫描， 是前景，
	/*1:扫描前景，将==255的， 且上下左右有不是255的加入种子vector
	同时建立码表，初始化为-1.  前景点为1，
	2：循环每个种子：查看四周
	查码表： ==1，跳过
	==0，跳过，
	==-1. 判断：梯度《30，加入种子， mask标1
	否则  梯度》60 ，标0 。其他标-1.

	效果差， 背景反而嫁过来了。

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
	mask.setTo(3);//3表示未处理 1前景 0背景
	int rows = grayMat.rows;
	int cols = grayMat.cols;

	std::deque<cv::Point> seeddeque;// 先用链表代替， 之后改为栈操作


	//高了半天，uchar中 255 就是-1 啊，怎么可能区分呢
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
					{//一定要周围有不是前景的点才加入种子
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
		/*循环每个种子：查看四周
		查码表： ==1，跳过
		==0，跳过，
		==-1. 判断：梯度《30，加入种子， mask标1
		否则  梯度》60 ，标0 。其他标-1.*/
		cv::Point  p = seeddeque.front();// 得到种子
		seeddeque.pop_front();//
		int i = p.x; int j = p.y;
		//const uchar * foreMatData = foreMat.ptr<uchar>(i);
		const uchar * grayMatData = grayMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		uchar grayCenter = grayMatData[j*3];
		//	uchar maskCenter = maskData[j];
		if (j - 1 >= 0 && maskData[j - 1] == 3)//左边
		{
			if (abs(grayMatData[(j - 1)*3] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
			{
				maskData[j - 1] = 1;
				resultData[j - 1] = 255;
				seeddeque.push_back(cv::Point(i, j - 1));
			}
			else if (abs(grayMatData[(j - 1)*3] - grayCenter) > MaxYuzhi)//说明是梯度太 da
			{
				maskData[j - 1] = 0;
				resultData[j - 1] = 0;
			}
		}

		if (j + 1 < cols && maskData[j + 1] == 3)//右边
		{
			if (abs(grayMatData[(j + 1)*3] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
			{
				maskData[j + 1] = 1;
				resultData[j + 1] = 255;
				seeddeque.push_back(cv::Point(i, j + 1));
			}
			else if (abs(grayMatData[(j + 1)*3] - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
			if (maskData1[j] == 3)//上边
			{
				if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
				{
					maskData1[j] = 1;
					resultData1[j] = 255;
					seeddeque.push_back(cv::Point(i - 1, j));
				}
				else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
			if (maskData2[j] == 3)//下边
			{
				if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
				{
					maskData2[j] = 1;
					resultData2[j] = 255;
					seeddeque.push_back(cv::Point(i + 1, j));
				}
				else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
{//从前景图 扫描， 是前景，
	/*1:扫描前景，将==255的， 且上下左右有不是255的加入种子vector
	//增加 depth的均值计算。==》作为 其他点的判断标准。


	同时建立码表，初始化为-1.  前景点为1，
	2：循环每个种子：查看四周
	查码表： ==1，跳过
	==0，跳过，
	==-1. 判断：梯度《30，加入种子， mask标1
	否则  梯度》60 ，标0 。其他标-1.

	*/
	Mat grayMat;
	cv::cvtColor(color, grayMat, CV_BGR2GRAY);
	cv::erode(foreMat, foreMat, Mat(), cv::Point(-1, -1), 1);

	//Mat tempforeMat;
	//cv::threshold(foreMat, tempforeMat, 2);
	//将foremat二值化， 然后 gray 掩码+ 得到均值。
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
	mask.setTo(3);//3表示未处理 1前景 0背景
	int rows = grayMat.rows;
	int cols = grayMat.cols;

	std::deque<cv::Point> seeddeque;// 先用链表代替， 之后改为栈操作


	//高了半天，uchar中 255 就是-1 啊，怎么可能区分呢
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
					{//一定要周围有不是前景的点才加入种子
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
		/*循环每个种子：查看四周
		查码表： ==1，跳过
		==0，跳过，
		==-1. 判断：梯度《30，加入种子， mask标1
		否则  梯度》60 ，标0 。其他标-1.*/
		cv::Point  p = seeddeque.front();// 得到种子
		seeddeque.pop_front();//
		int i = p.x; int j = p.y;
		//const uchar * foreMatData = foreMat.ptr<uchar>(i);
		const uchar * grayMatData = grayMat.ptr<uchar>(i);
		uchar * resultData = result.ptr<uchar>(i);
		uchar * maskData = mask.ptr<uchar>(i);

		uchar grayCenter = grayMatData[j * 3];
		//	uchar maskCenter = maskData[j];
		if (j - 1 >= 0 && maskData[j - 1] == 3)//左边
		{
			if (abs(grayMatData[(j - 1) * 3] - grayCenter) < MinYuzhi )//说明是梯度小，加入种子
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
			else if (abs(grayMatData[(j - 1) * 3] - grayCenter) > MaxYuzhi)//说明是梯度太 da
			{
				maskData[j - 1] = 0;
				resultData[j - 1] = 0;
			}
		}

		if (j + 1 < cols && maskData[j + 1] == 3)//右边
		{
			if (abs(grayMatData[(j + 1) * 3] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
			else if (abs(grayMatData[(j + 1) * 3] - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
			if (maskData1[j] == 3)//上边
			{
				
				if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
				else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
			if (maskData2[j] == 3)//下边
			{
				if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
				else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//说明是梯度太 da
				{
					maskData2[j] = 0;
					resultData2[j] = 0;
				}
			}
		}
	}


	cout <<"修改的个数 -------------"<< numberOfadd<<endl;
	imshow("result", result);
	cvWaitKey(1);
	return result;
}


//用list做， 每次修不完后，次数++ 。 限制 10次。 
cv::Mat ImageSegment::regiongrowthIndepthWithmeanInList(Mat & color, Mat &depthMatIncolorSize, Mat &foreMat)
{//从前景图 扫描， 是前景，  
	/*1:扫描前景，将==255的， 且上下左右有不是255的加入种子vector
	//增加 depth的均值计算。==》作为 其他点的判断标准。


	同时建立码表，初始化为-1.  前景点为1，
	2：循环每个种子：查看四周
	查码表： ==1，跳过
	==0，跳过，
	==-1. 判断：梯度《30，加入种子， mask标1
	否则  梯度》60 ，标0 。其他标-1.

	*/
	Mat grayMat;
	cv::cvtColor(color, grayMat, CV_BGR2GRAY);
	//cv::erode(foreMat, foreMat, Mat(), cv::Point(-1, -1), 1);

	//Mat tempforeMat;
	//cv::threshold(foreMat, tempforeMat, 2);
	//将foremat二值化， 然后 gray 掩码+ 得到均值。
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
	mask.setTo(3);//3表示未处理 1前景 0背景
	int rows = grayMat.rows;
	int cols = grayMat.cols;

	std::list<cv::Point> seedlist;// 先用链表代替， 之后改为栈操作
	std::list<cv::Point>:: iterator itercont;

	std::list<cv::Point> seedlistnew;
	std::list<cv::Point>::iterator intercontnew;


	std::list<cv::Point>::iterator interswap;
	//高了半天，uchar中 255 就是-1 啊，怎么可能区分呢
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
					{//一定要周围有不是前景的点才加入种子
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
			/*循环每个种子：查看四周
			查码表： ==1，跳过
			==0，跳过，
			==-1. 判断：梯度《30，加入种子， mask标1
			否则  梯度》60 ，标0 。其他标-1.*/
			cv::Point  p = *(itercont);// 得到种子
		
			int i = p.x; int j = p.y;
			//const uchar * foreMatData = foreMat.ptr<uchar>(i);
			const uchar * grayMatData = grayMat.ptr<uchar>(i);
			uchar * resultData = result.ptr<uchar>(i);
			uchar * maskData = mask.ptr<uchar>(i);

			uchar grayCenter = grayMatData[j * 3];
			//	uchar maskCenter = maskData[j];
			if (j - 1 >= 0 && maskData[j - 1] == 3)//左边
			{
				if (abs(grayMatData[(j - 1) * 3] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
				else if (abs(grayMatData[(j - 1) * 3] - grayCenter) > MaxYuzhi)//说明是梯度太 da
				{
					maskData[j - 1] = 0;
					resultData[j - 1] = 0;
				}
			}

			if (j + 1 < cols && maskData[j + 1] == 3)//右边
			{
				if (abs(grayMatData[(j + 1) * 3] - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
				else if (abs(grayMatData[(j + 1) * 3] - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
				if (maskData1[j] == 3)//上边
				{

					if (abs(grayMatDatac1 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
					else if (abs(grayMatDatac1 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
				if (maskData2[j] == 3)//下边
				{
					if (abs(grayMatDatac2 - grayCenter) < MinYuzhi)//说明是梯度小，加入种子
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
					else if (abs(grayMatDatac2 - grayCenter) > MaxYuzhi)//说明是梯度太 da
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
	 	cout << "第"<<times <<"轮 修改的个数 -------------" << numberOfadd << endl;

	}
	
	seedlistnew.clear();
	seedlist.clear();


	imshow("result", result);
	cvWaitKey(1);
	return result;
}