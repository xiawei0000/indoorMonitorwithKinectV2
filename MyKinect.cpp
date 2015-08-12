#include "MyKinect.h"
#define  DEBUG 1

MyKinect::MyKinect() :m_pDepthBackProcess()
{
	colorLostFrames = 0;
	depthLostFrames = 0;
	nFrames = 0;
	depthFrames = 0;
	colorFrames = 0;
	prcossedFrames = 0;

	cDepthMinReliableDistance = (USHORT)500;
	cDepthMaxDistance =(USHORT) 4500;


	m_pKinectSensor = NULL;
	m_pColorFrameReader = NULL;
	m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];// create heap storage for color pixel data in RGBX format
	m_pColorBuffer = new RGBQUAD[cColorWidth * cColorHeight];

	m_pColorCoordinates = new DepthSpacePoint[cColorWidth * cColorHeight];
	m_pDepthBuffer = new UINT16[cDepthWidth * cDepthHeight];
	isFirstFrame = true;

	m_pDepthFrameReader = NULL;
	m_pDepthRGBX = new RGBQUAD[cDepthWidth * cColorHeight];//RGBQUAD是多维度的
	m_pDepthRGBX_Last = new RGBQUAD[cDepthWidth*cColorHeight];//上一帧的保存

	//Mat 的实现是namespace实现的，函数都是inline函数，不是类 不能new，new出来的都是指针了。
	//m_pDepthMat_Last= new cv::Mat(cv::Size(cDepthWidth, cColorHeight), CV_8UC4,cvScalar(0));
	//直接写成mat就可以了
	//m_pDepthMat_Last = Mat(cv::Size(cDepthWidth, cColorHeight), CV_8UC4, cvScalar(0));
	 //调用类的create函数也可以
	//m_pDepthMat_Last.create(cv::Size(cDepthWidth, cColorHeight), CV_8UC4);
	m_pDepthMat_Last = cv::Mat::zeros(cDepthHeight,cDepthWidth,  CV_8UC3);
	m_pDepthMat_LastMask = cv::Mat::zeros(cDepthHeight, cDepthWidth, CV_8UC1);
	//m_pDepthBackProcess =  BackGroundProcess();//这是赋值，类的初始化只能在申明处

	memset(m_pDepthRGBX_Last, 0, sizeof(RGBQUAD));//初始为0

//	rgbWriter = BaseImageTools::NewWrite(cColorWidth/2,cColorHeight/2,"Color.avi");
	depthWriter = BaseImageTools::NewWrite(cDepthWidth, cDepthHeight, "Depth.avi");
	//depthWriter2 = BaseImageTools::NewWrite(cDepthWidth, cDepthHeight, "Depth2.avi");

	//opencv得这些函数不支持多线程啊
	/*cvNamedWindow("color"); 
	cv::moveWindow("color", 500, 0);
	cvNamedWindow("gaussianBlur");
	cvMoveWindow("gaussianBlur", 0, 0);
	*/


	m_pBodyIndexBuffer = new BYTE[cBodyIndexWidth * cBodyIndexHeight];
	for (int i = 0; i < cBodyIndexWidth * cBodyIndexHeight;i++)
	{
		(uchar)m_pBodyIndexBuffer[i] = 0xff;
	}
	pTracker = (CvBlobTrackerAuto1*)new CvBlobTrackerAuto1(); //cvCreateBlobTrackerAuto1();//初始化跟踪模块。
	//类直接当指针用？？？？
}

MyKinect::~MyKinect()
{
	if (m_pColorRGBX)
	{//释放 RGB缓存数组
		delete[] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}
	//释放reader类
	SafeRelease(m_pColorFrameReader);// done with color frame reader

	if (m_pKinectSensor)
	{//关闭kinect
		m_pKinectSensor->Close();// close the Kinect Sensor
	}
	SafeRelease(m_pKinectSensor);

	if (m_pDepthRGBX)
	{
		delete[] m_pDepthRGBX;
		m_pDepthRGBX = NULL;
	}
	SafeRelease(m_pDepthFrameReader);


	//cvReleaseVideoWriter(&depthWriter);
	//cvReleaseVideoWriter(&rgbWriter);
}

HRESULT	MyKinect::InitKinect()
{
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return hr;
	}

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get the color reader


		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{//获取映射关系，  可以放在open前面
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
		}

		if (SUCCEEDED(hr))
		{//注意 如果一个源没有开启，那么都不会成功
			hr = m_pKinectSensor->OpenMultiSourceFrameReader(//FrameSourceTypes::FrameSourceTypes_Audio|
				FrameSourceTypes::FrameSourceTypes_Body|
				FrameSourceTypes::FrameSourceTypes_BodyIndex|FrameSourceTypes::FrameSourceTypes_Color|
				FrameSourceTypes::FrameSourceTypes_Depth|FrameSourceTypes::FrameSourceTypes_Infrared,
				&m_pMultiSourceFrameReader);
		}
		//MUlti没有source，是先mutilreader，然后从这个reader-》分别获取frame
		//所以没有source释放 

		/* 音频增加和上面的不一样
		//也是source -》render-》再注册事件
		// 获取音频源(AudioSource)
		if (SUCCEEDED(hr)){
			hr = m_pKinectSensor->get_AudioSource(&m_pAudioSource);
		}
		// 再获取音频帧读取器
		if (SUCCEEDED(hr)){
			hr = m_pAudioSource->OpenReader(&m_pAudioBeamFrameReader);
		}
		m_pAudioBeamFrameReader->AcquireLatestBeamFrames();*/
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		printf("No ready Kinect found! \n");
		return E_FAIL;
	}

	return hr;
}

void MyKinect::Update()
{

	//多源的是先获取一个总的源frame，然后通过获取各自的reference，提取各自的frame

	if (!m_pMultiSourceFrameReader)
	{
		return;
	}
	IMultiSourceFrame * pMultiSourceFrame = NULL;
	HRESULT hr = m_pMultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);

	if (SUCCEEDED(hr))
	{
		std::thread thread_Color_Process;
		std::thread thread_Depth_Process;
		std::thread thread_BodyIndex_Process;
		std::thread thread_Body_Process;

	//	std::thread thread_Audio_Process;

		//最好改成4个线程并行
		//彩色帧放前面，就读不到深度帧了？？
		bool *colorFrameReturn ; bool tcolor = false; colorFrameReturn = &tcolor;
		bool *depthFrameReturn ; bool tdepth = false; depthFrameReturn = &tdepth;
		bool *bodyIndexFrameReturn; bool tbodyIndex = false; bodyIndexFrameReturn = &tbodyIndex;
		bool * bodyFrameReturn; bool tbody = false; bodyFrameReturn = &tbody;

	//	bool *audioFrameReturn; bool taudio = false; audioFrameReturn = &taudio;


	//	bool * te = false;
		std::thread thread_Color_Copy(&MyKinect::Process_ColorFrame, this, pMultiSourceFrame, colorFrameReturn);
	//	bool colorFrameReturn = Produce_ColorFrame(pMultiSourceFrame);
		std::thread thread_Depth_Copy(&MyKinect::Process_DepthFrame, this, pMultiSourceFrame, depthFrameReturn);
		//传入引用无法获得结果，只能用指针，获取值

	//	bool depthFrameReturn = Process_DepthFrame(pMultiSourceFrame);
		
		std::thread thread_Body_Copy(&MyKinect::Process_BodyFrame, this, pMultiSourceFrame, bodyFrameReturn);

		std::thread thread_BodyIndex_Copy(&MyKinect::Process_BodyIndexFrame, this, pMultiSourceFrame, bodyIndexFrameReturn);

		//std::thread thread_Audio_Process

		/* 
		MyTime mt;
		mt.Reset();
		mt.Start();
		bool colorFrameReturn = Produce_ColorFrame(pMultiSourceFrame);
		mt.End();
		cout << "复制color buffer 运行时" << mt.costTime << endl;
		mt.Reset();
		mt.Start();
		bool depthFrameReturn=Process_DepthFrame(pMultiSourceFrame);
		//处理结果保存到全局变量中。
		mt.End();
		cout << "复制depth buffer 运行时" << mt.costTime << endl;
		*/
		
		thread_Color_Copy.join();//等待子线程结束
		thread_Depth_Copy.join();
		thread_Body_Copy.join();
		thread_BodyIndex_Copy.join();
	
	//	Process_BodyFrame(pMultiSourceFrame);
	//	Process_BodyIndexFrame(pMultiSourceFrame);
		if (*colorFrameReturn)
		{
			//这一步，使得图片缩小了3
			thread_Color_Process=std::thread(&MyKinect::ProcessColor, this, m_pColorBuffer, cColorWidth, cColorHeight);
		//	MyTime mt;
		//	mt.Reset();
		//	mt.Start();
		//	ProcessColor(m_pColorBuffer, cColorWidth, cColorHeight);			
		//	mt.End();
		//	cout << "color处理 运行时" << mt.costTime << endl;
		}
		
		if (*depthFrameReturn)
		{
			thread_Depth_Process=std::thread(&MyKinect::ProcessDepth, this, m_pDepthBuffer, cDepthWidth, cDepthHeight, cDepthMinReliableDistance, cDepthMaxDistance);
		/*	MyTime mt;
			mt.Reset();
			mt.Start(); 
		 	ProcessDepth(m_pDepthBuffer, cDepthWidth, cDepthHeight, cDepthMinReliableDistance, cDepthMaxDistance);
		//	mt.End();
		//	cout << "depth处理运行时" << mt.costTime << endl;*/
		}

		if (*bodyFrameReturn)
		{
			thread_Body_Process = std::thread(&MyKinect::ProcessBody, this, m_nTime, BODY_COUNT,m_ppBodies);
		}
		if (*bodyIndexFrameReturn)
		{
			thread_BodyIndex_Process = std::thread(&MyKinect::ProcessBodyIndex, this, m_pBodyIndexBuffer, cBodyIndexWidth, cBodyIndexHeight);
		}
		if (thread_Color_Process.joinable())
		{
			thread_Color_Process.join();
		}
		if (thread_Depth_Process.joinable())
		{
			thread_Depth_Process.join();
		}
		if (thread_Body_Process.joinable())
		{
			thread_Body_Process.join();
		}
		if (thread_BodyIndex_Process.joinable())
		{
			thread_BodyIndex_Process.join();
		}


		imshow("body", m_bodyMat);

		imshow("bodyindex", m_bodyIndex);

		isFirstFrame = false;
		//Mat smallColor;
		//cv::resize(m_pColorMat_Last, smallColor, cvSize(m_pColorMat_Last.cols / RESIZE_FACTOR, m_pColorMat_Last.rows / RESIZE_FACTOR));
		//cv::resize(m_pColorMat_Last, smallColor, cvSize(cColorWidth_RESIZE, cColorHeight_RESIZE));
		//cv::moveWindow("color", 500, 0);
	/*		imshow("color", m_pColorMat_Last_Resize);
		cvMoveWindow("color", 0, 0);
	cvMoveWindow("depth", 640, 0);		imshow("depth", m_pDepthMat_Last);*/	
		/*imshow("rewrwe", m_pDepthMat_LastMask);
		imshow("resurewrewrewtl ", m_pTempshow);
		imshow("colorFore", m_pColorMat_LastMask_Resize);//small picture
		cvMoveWindow("colorFore", 0, cColorHeight_RESIZE);// m_pColorMat_Last.rows / RESIZE_FACTOR);
		imshow("depthFore", m_pDepthMat_LastMask);
		cvMoveWindow("depthFore", 640, cColorHeight_RESIZE);// m_pColorMat_Last.rows / RESIZE_FACTOR);		
*/		

		// 1  分线程 color 和depth 分开跟踪。


		
		//先跟踪处理
		if (prcossedFrames > 10 && m_pColorMat_Last_Resize.rows > 0 && m_pColorMat_LastMask_Resize.rows > 0)
		{
			IplImage ipl_pColorMat_Last = m_pColorMat_Last_Resize;
			IplImage ipl_pColorMat_LastMask = m_pColorMat_LastMask_Resize;
			pTracker->ProcessTracker(&ipl_pColorMat_Last, &ipl_pColorMat_LastMask);
			//	pTracker->ProcessNewBlob(&ipl_pColorMat_Last, &ipl_pColorMat_LastMask);
			pTracker->showAndSaveResult(&ipl_pColorMat_Last);
		}
/**/

		/**同步帧 ：
		1:提取混合背景， 
		2:检测入口点 ： 保存新一帧的团块， 将新的添加到 跟踪链表里
		3：.
		*/

		
		//这里colorlast是原来大小的,但是color背景缩小了,
		if ((*depthFrameReturn) && (*colorFrameReturn))//必须同步才行
		{
			prcossedFrames++;//前几帧有问题
			
		//	m_pSyncMat_MASK=ProcessCoordinates(m_pColorMat_LastMask_Resize, m_pDepthMat_LastMask, m_pColorCoordinates);
			m_pSyncMat_MASK = ProcessCoordinates(m_pColorMat_LastMask_Resize, m_pDepthMat_LastMask_Resize, m_pColorCoordinates);
			//要返回前景。
			if (prcossedFrames > 5 && m_pSyncMat_MASK.rows > 0 && m_pSyncMat_MASK.rows > 0)
			{
			
				IplImage ipl_syn= m_pSyncMat_MASK;
				IplImage ipl_pColorMat_Last = m_pColorMat_Last_Resize;
				pTracker->ProcessNewBlob(&ipl_pColorMat_Last, &ipl_syn);
			}	
		}
		
	/*	*/		
		//cvWaitKey(500); 采用获取模式，等500也没问题
	}

	++nFrames;
//	cout << "update  第" << nFrames << "帧   " << endl;
}
/*
bool  MyKinect::Process_AudioFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool)
{
	if (!pMultiSourceFrame) {
		*returnbool = false;
		return false;
	}
	*returnbool = false;
	IDepthFrame * pDepthFrame = NULL;
	IDepthFrameReference *pDepthFrameReference = NULL;
	HRESULT hr = pMultiSourceFrame->get_(&pDepthFrameReference);

	if (SUCCEEDED(hr))
	{
		hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
	}
	SafeRelease(pDepthFrameReference);
 
	if (SUCCEEDED(hr))
	{
		IFrameDescription * pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		//	USHORT nDepthMinReliableDistance = 0;
		//USHORT nDepthMaxDistance = 0;
		UINT nBufferSize = 0;
		UINT16 *pBuffer = NULL;

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
		}
		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Width(&nWidth);
		}
		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Height(&nHeight);
		}
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_DepthMinReliableDistance(&cDepthMinReliableDistance);
		}
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_DepthMaxReliableDistance(&cDepthMaxDistance);
		}

		if (SUCCEEDED(hr))
		{//这里是将指针buffer提取出来了，没有拷贝
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);//这里的size是ushort而言的，memcopy是uchar来的。
		}

		if (SUCCEEDED(hr))
		{
			//int tempsize = cDepthHeight*cDepthWidth *sizeof(USHORT);
			memcpy(m_pDepthBuffer, pBuffer, nBufferSize*sizeof(USHORT));

			*returnbool = true;
			//ProcessDepth(pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
		}

		SafeRelease(pFrameDescription);//Description 和Frame 都要释放的
	}
 
	SafeRelease(pDepthFrame);

	return *returnbool;
}*/
bool  MyKinect::Process_DepthFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool)
{
	if (!pMultiSourceFrame) {
		*returnbool = false;
		return false;
	}
	 *returnbool = false;
	IDepthFrame * pDepthFrame = NULL;
	IDepthFrameReference *pDepthFrameReference = NULL;
	HRESULT hr = pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);

	if (SUCCEEDED(hr))
	{
		hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
	}
	SafeRelease(pDepthFrameReference);
	/*if (!SUCCEEDED(hr))
	{
		cout << "    深度帧丢失" << ++depthLostFrames << endl;
	}*/
	

	if (SUCCEEDED(hr))
	{
		IFrameDescription * pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
	//	USHORT nDepthMinReliableDistance = 0;
		//USHORT nDepthMaxDistance = 0;
		UINT nBufferSize = 0;
		UINT16 *pBuffer = NULL;
		
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
		}
		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Width(&nWidth);
		}
		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Height(&nHeight);
		}
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_DepthMinReliableDistance(&cDepthMinReliableDistance);
		}
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_DepthMaxReliableDistance(&cDepthMaxDistance);
		}

		if (SUCCEEDED(hr))
		{//这里是将指针buffer提取出来了，没有拷贝
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);//这里的size是ushort而言的，memcopy是uchar来的。
		}

		if (SUCCEEDED(hr))
		{
			//int tempsize = cDepthHeight*cDepthWidth *sizeof(USHORT);
			memcpy(m_pDepthBuffer, pBuffer, nBufferSize*sizeof(USHORT));

			*returnbool = true;
			//ProcessDepth(pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
		}

		SafeRelease(pFrameDescription);//Description 和Frame 都要释放的
	}
/*	else if (colorDepthFramesynchronization)//深度帧没了，但是彩色帧处理了，就要用上一帧来
	{//没用的， 已经用上一帧的coordinates处理了，这就多余了
	}
	*/
	SafeRelease(pDepthFrame);
	
	return *returnbool;

}

void MyKinect::ProcessDepth(const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth)
{//处理深度，并修复,产生掩码
	
#ifdef DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif


	// Make sure we've received valid data
	if (m_pDepthRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
	{

		depthFrames++;
	 	cout << "depthFrames第" << depthFrames << "帧" << endl;
		RGBQUAD* pRGBX = m_pDepthRGBX;
		const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);
		int k1 = 0, k2 = 0; int k3 = 0, k4 = 0;

		Mat inpaintResult;//修图结果
		Mat inpaintMask;//掩码		
		Mat badMask;//错误点的掩码

		badMask = Mat::zeros(Size(nWidth, nHeight), CV_8U);
		inpaintMask = Mat::zeros(Size(nWidth, nHeight), CV_8U);
	
		/*uchar * inpaintDataMask = inpaintMask.data;
		uchar * badMaskData = badMask.data;//标记当前帧 错误点掩码
		uchar * depthMat_LastData= m_pDepthMat_Last.data;
		uchar * depthMat_LastDataMask = m_pDepthMat_LastMask.data;//前景的掩码
		*/
	/*	int a = m_pDepthMat_Last.channels();
		int b = m_pDepthMat_LastMask.channels();
		int c = badMask.channels();
		int d = inpaintMask.channels();
		cout << a << "   " << b << "   " << c << "   " << d << endl;
		*/
		//考虑到mat字节对齐，改为for
		for (int i = 0; i < nHeight;i++)
		{
			uchar * inpaintDataMask = inpaintMask.ptr<uchar>(i);
			uchar * badMaskData = badMask.ptr<uchar>(i);//标记当前帧 错误点掩码
			uchar * depthMat_LastData = m_pDepthMat_Last.ptr<uchar>(i);
			uchar * depthMat_LastDataMask = m_pDepthMat_LastMask.ptr<uchar>(i);//前景的掩码

			for (int j = 0; j < nWidth;j++)
			{
				USHORT depth = *pBuffer;
				bool maskOfPixel = 0;
				if (depth <= 0)//must error pixels
				{
					depth = 0;
					badMaskData[j] = (uchar)255;
					maskOfPixel = 1;
					k1++;
				}
				if (depth > 4500) k2++;
				depth = depth >> 4;//如何最好的将 450 -- 5500映射到 0 -255里面
				//有大量=0的点，2W多 ,

				if (depth >= 255) {//掩码赋值
					depth = 255;
				}
				BYTE intensity = static_cast<BYTE>(depth);

				//用上一帧修正
				if (maskOfPixel)
				{//深度有误
					k3++;
					
					//上一帧的修复只能对背景用，不能对运动的用
					//if ((depthMat_LastDataMask[j]) == 0)//是背景
					//重要的地方就是 人运动的边缘，而这都是前景区域了。		
					
					{//这两个是8u3c的
						intensity = (depthMat_LastData[j*3]);//采用上一帧
					}

					if (intensity == 0)// || intensity == 255))//结果表明 摄像头固定，有的点会一直错误
					{//上一帧也是错误点，且没修复,接着修复					
						inpaintDataMask[j] = (uchar)255;
						k4++;
					}
				}
				pRGBX->rgbRed = intensity;
				pRGBX->rgbGreen = intensity;
				pRGBX->rgbBlue = intensity;
				++pRGBX;
				++pBuffer;
			}
		}

	// std::cout << setw(5)<< k1 << setw(5) << k2  <<"为0点："<<setw(5)<<k3<<"  修正："<<setw(5)<<k4<<"  "<<" "<<std::endl ;
		//复制上一帧
	//	memcpy( m_pDepthRGBX_Last,m_pDepthRGBX, sizeof(RGBQUAD)*cDepthHeight*cDepthWidth);

		// Draw the data with OpenCV
		Mat DepthImage(nHeight, nWidth, CV_8UC4, m_pDepthRGBX);
		//imshow("orignal", DepthImage);
		//cvMoveWindow("orignal",0,0);
		Mat depthImageRGB;
		cvtColor(DepthImage,depthImageRGB,CV_RGBA2RGB);
		cv::inpaint(depthImageRGB, inpaintMask, inpaintResult, 3, CV_INPAINT_TELEA);// CV_INPAINT_NS);//空洞太大了效果不好
		//使用了inpaint后，就是第一帧会修复，其他的都不会修复了。
	//	imshow("inpaint", inpaintResult);
	//	cvMoveWindow("inpaint", 500, 0);

		//cvSmooth(&inpaintResult, &m_pDepthMat_Last);这是c格式的支持cvmat的，下面的才是c++的
	//	Mat  blurMat;
	//	blur(inpaintResult, blurMat, Size(3, 3), Point(-1, -1)); //Size( w,h ):
	//	imshow("blur", blurMat);
		//cvMoveWindow("blur",500,500);
	
		//2015-5-10 晚 ，修改为缩小版
		cv::resize(m_pDepthMat_Last, m_pDepthMat_Last_Resize, cv::Size(cDepthWidth_RESIZE, cDepthHeight_RESIZE));

		//对 fuliye版本 调整位置
		cv::GaussianBlur(inpaintResult, m_pDepthMat_Last, Size(3, 3), 0,0); //Size( w,h ):
	
	


	//	m_pDepthMat_Last = inpaintResult.clone();//保存上一帧 

	//	imshow("gaussianBlur", m_pDepthMat_Last);
		//cvMoveWindow("gaussianBlur",0,500);
	//	cvMoveWindow("gaussianBlur", 0, 0);
	//	cvWaitKey(1);

	//	cout << "m_pDepthMat_Last :" << m_pDepthMat_Last.channels();
	 	 Mat foreMat;//提取前景
	 	//m_pDepthBackProcess.process_Mog2(m_pDepthMat_Last, foreMat);
		// m_pDepthBackProcess.depthProcessBackGround(m_pDepthMat_Last, m_pDepthMat_LastMask);
	// 	 m_pDepthBackProcess.testManyMethod(m_pDepthMat_Last, foreMat);

		 m_pDepthBackProcess.depthProcessBackGround(m_pDepthMat_Last_Resize, m_pDepthMat_LastMask_Resize);


		 /*
		 测试分割 	 
		 ImageSegment iseg;
		 m_pTempshow=iseg.regiongrowthIndepth3(m_pDepthMat_Last, m_pDepthMat_LastMask);
*/
		/* if( (foreMat.rows !=0 )&& (foreMat.cols !=0))
		 {	
			 m_pDepthMat_LastMask = foreMat.clone();//保存前景的掩码
		 }*/
	 


	//	cout <<"通道数目"<< inpaintResult.channels();
	//	BaseImageTools::SaveDepthImageToAVI(inpaintResult, nWidth, nHeight, depthWriter);

	//	BaseImageTools::SaveDepthImageToAVI(inpaintResult, nWidth, nHeight, depthWriter2);

		//BaseImageTools::SaveDepthImageToAVI(DepthImage, nWidth, nHeight, depthWriter);
/*		Mat show = DepthImage.clone();
		imshow("DepthImage2", show);
		*/
	}

#ifdef DEBUG
	mt.End();
	cout << "depth处理运行时           ：" << mt.costTime << endl;
#endif // DEBUG


}

/*完成采用 图片 保存上一帧，用 inpaint填充，用blur平滑。
inpaint的使用有待提高。
上方大块的空洞如何处理？？*/


bool	MyKinect::Process_BodyIndexFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool)
{
	*returnbool = false;
	if (!pMultiSourceFrame)	{ return  *returnbool; }

	IBodyIndexFrameReference* pBodyIndexFrameReference = NULL;
	IBodyIndexFrame* pBodyIndexFrame = NULL;

	HRESULT hr = pMultiSourceFrame->get_BodyIndexFrameReference(&pBodyIndexFrameReference);
	if (SUCCEEDED(hr))
	{
		hr = pBodyIndexFrameReference->AcquireFrame(&pBodyIndexFrame);
	}
	SafeRelease(pBodyIndexFrameReference);


	IFrameDescription* pBodyIndexFrameDescription = NULL;
	int nBodyIndexWidth = 0;
	int nBodyIndexHeight = 0;
	
	
	UINT &nBodyIndexBufferSize=m_IBodyIndexBufferSize;;// = 0;
	
	//BYTE * &pBodyIndexBuffer = m_pBodyIndexBuffer;
	BYTE * pBodyIndexBuffer=NULL;

	if (SUCCEEDED(hr))
	{
		hr = pBodyIndexFrame->get_FrameDescription(&pBodyIndexFrameDescription);


		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameDescription->get_Width(&nBodyIndexWidth);
		}
		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrameDescription->get_Height(&nBodyIndexHeight);
		}
		if (SUCCEEDED(hr))
		{
			hr = pBodyIndexFrame->AccessUnderlyingBuffer(&nBodyIndexBufferSize, &pBodyIndexBuffer);
		}
		if (SUCCEEDED(hr))
		{	//这里一定要赋值出去才行， 不复制没效果
			*returnbool = true;
			memcpy(m_pBodyIndexBuffer, pBodyIndexBuffer, nBodyIndexBufferSize*sizeof(BYTE));
			//cout << "111111!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!======="<<nBodyIndexWidth << nBodyIndexHeight;
			//ProcessBodyIndex(pBodyIndexBuffer, nBodyIndexWidth, nBodyIndexHeight);
		}
		SafeRelease(pBodyIndexFrameDescription);
	}
	SafeRelease(pBodyIndexFrame);

	return *returnbool;
}

void	MyKinect::ProcessBodyIndex(const BYTE* pBodyIndexBuffer, int nBodyIndexWidth, int nBodyIndexHeight)
{
#ifdef DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif // DEBUG

	if (m_IBodyIndexBufferSize != nBodyIndexWidth *nBodyIndexHeight)
	{
		cout << " m_IBodyIndexBufferSize ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ " << m_IBodyIndexBufferSize << endl;
	}
	RGBQUAD *pBodyIndexRGB = new RGBQUAD[nBodyIndexWidth*nBodyIndexHeight];
	for (int w = 0; w < nBodyIndexWidth; ++w)
	{
		for (int h = 0; h < nBodyIndexHeight; ++h)
		{
			BYTE player = pBodyIndexBuffer[h*nBodyIndexWidth + w];
			if (player != 0xff)
			{
				pBodyIndexRGB[h*nBodyIndexWidth + w].rgbBlue = 0;
				pBodyIndexRGB[h*nBodyIndexWidth + w].rgbGreen = 220;
				pBodyIndexRGB[h*nBodyIndexWidth + w].rgbRed = 0;
				//	pBodyIndexRGB[h*nBodyIndexWidth + w].rgbReserved = 0xff;				
			}
			else  
			{
				pBodyIndexRGB[h*nBodyIndexWidth + w].rgbBlue = 0;
				pBodyIndexRGB[h*nBodyIndexWidth + w].rgbGreen = 0;
				pBodyIndexRGB[h*nBodyIndexWidth + w].rgbRed = 0;
				//	pBodyIndexRGB[h*nBodyIndexWidth + w].rgbReserved = 0xff;				
			}
		}
	}

	Mat  &mtrix_BodyIndex = m_bodyIndex;
	 mtrix_BodyIndex = Mat(nBodyIndexHeight, nBodyIndexWidth, CV_8UC4, pBodyIndexRGB);
	//m_bodyIndex = mtrix_BodyIndex.clone();
	//imshow("bodyindex", mtrix_BodyIndex);

#ifdef DEBUG 
	mt.End();
	cout << "bodyindex 处理运行时           ：：：：：：" << mt.costTime << endl;
#endif

}
bool	MyKinect::Process_BodyFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool)
{
	*returnbool = false;
	if (!pMultiSourceFrame)	{ return  *returnbool; }

	IBodyFrameReference * pBodyFramReference = NULL;
	IBodyFrame * pBodyFrame = NULL;
	HRESULT hr = pMultiSourceFrame->get_BodyFrameReference(&pBodyFramReference);
	if (SUCCEEDED(hr))
	{
		hr = pBodyFramReference->AcquireFrame(&pBodyFrame);
	}
	SafeRelease(pBodyFramReference);

	if (SUCCEEDED(hr))
	{
	//	INT64 nTime = 0;
		hr = pBodyFrame->get_RelativeTime(&m_nTime);//时间错用来计算fps
		//IBody * ppBodies[BODY_COUNT] = { 0 };

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrame->GetAndRefreshBodyData(_countof(m_ppBodies), m_ppBodies);
		}
		if (SUCCEEDED(hr))
		{
			*returnbool = true;
			//ProcessBody(nTime, BODY_COUNT, m_ppBodies);
		}
	
		/* 
		for (int i = 0; i < _countof(m_ppBodies); ++i)
		{
			
			SafeRelease(m_ppBodies[i]);

		}*/
		//SafeRelease(ppBodies);不可用，
		//这里是IBody * ppBodies[6]  先是一个6长度的数组，数组里面是指针，
		//所以不能释放ppbodies ，而自己建立的int ** 二维指针是不同的

	}
	SafeRelease(pBodyFrame);//frmae 是每次都要跟新，所以要释放
	//Reader是一次申请，以后每次要用Reader获取frame，所以不要释放了
	//source是最开始 用来获取reader的，马上就释放了

	return *returnbool;
}

void MyKinect::ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
{

#ifdef DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif // DEBUG

	//都绘制到这矩阵上
	//Mat BodyMat(cDepthWidth,cDepthHeight,CV_8UC3);
	Mat &BodyMat = m_bodyMat;
	BodyMat=Mat::zeros(cDepthHeight, cDepthWidth, CV_8UC3);
	//Mat ColorBodyMat = Mat::zeros(cColorHeight,cColorWidth,  CV_8UC3);
	for (int i = 0; i < nBodyCount; i++)
	{
		IBody *pBody = ppBodies[i];
		if (pBody)
		{

			BOOLEAN bTracked = false;//boolean是byte的重定义 BOOL是int的重定义
			HRESULT hr = NULL;
			//hr = pBody->get_IsRestricted(&bTracked);  有受限制的; 不是跟踪与否 这两个函数很像

			hr = pBody->get_IsTracked(&bTracked);
			//kinect的调用大多都是 用句柄的方式返回是否成功，而结果放回都用指针。
			if (SUCCEEDED(hr) && bTracked)
			{//提取每个body的点
				Joint joints[JointType_Count];
				HandState leftHandState = HandState_Unknown;
				HandState rightHandState = HandState_Unknown;
				pBody->get_HandLeftState(&leftHandState);
				pBody->get_HandRightState(&rightHandState);

				hr = pBody->GetJoints(_countof(joints), joints);

				if (SUCCEEDED(hr))
				{
					for (int j = 0; j < _countof(joints); ++j)
					{//获取每一个点 绘制
						/*ColorSpacePoint colorPoint = { 0 };
						m_pCoordinateMapper->MapCameraPointToColorSpace(
						joints[j].Position, &colorPoint);
						//将二维的点转到opencv上的点
						Point temp;
						temp.x = colorPoint.X;
						temp.y = colorPoint.Y;
						*/
						DepthSpacePoint depthPoint = { 0 };
						m_pCoordinateMapper->MapCameraPointToDepthSpace(joints[j].Position, &depthPoint);

						//将二维的点转到opencv上的点
						Point temp;
						temp.x = static_cast<int>(depthPoint.X);
						temp.y = static_cast<int>(depthPoint.Y);
						circle(BodyMat, temp, 1, Scalar(0, 200, 10), 8, 0);
					}
				}

			}
		}
	}
	 
	for (int i = 0; i < _countof(m_ppBodies); ++i)
	{
		SafeRelease(m_ppBodies[i]);
	} 
	
#ifdef DEBUG 
	mt.End();
	cout << "body处理运行时           ：：：：：：" << mt.costTime << endl;
#endif
}





bool  MyKinect::Process_ColorFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool)
{//先是multireader中获取lastframe，然后就是各自获取Frame的reference（就是引用的意思啊），
	if (!pMultiSourceFrame)	{ *returnbool = false;	return false; }
	*returnbool = false;

	IColorFrameReference* pColorFrameReference = NULL;
	IColorFrame* pColorFrame = NULL;
	//提取色彩帧的引用
	HRESULT hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
	//问题就在这里了， 没有读取到彩色帧啊！！！

	// hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
	if (SUCCEEDED(hr))
	{
		hr = pColorFrameReference->AcquireFrame(&pColorFrame);
	}

	/*if (!SUCCEEDED(hr))
	{
	cout << "     没有读到彩色帧 " << ++colorLostFrames;
	}*/
	SafeRelease(pColorFrameReference);//这个引用还要释放呢
	if (SUCCEEDED(hr))
	{
		IFrameDescription* pFrameDescription = NULL;
		//	int nWidth = 0;
		//	int nHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nBufferSize = 0;
		RGBQUAD *pBuffer = NULL;

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_FrameDescription(&pFrameDescription);
		}

		/*	if (SUCCEEDED(hr))
		{
		hr = pFrameDescription->get_Width(&nColorWidth);
		}

		if (SUCCEEDED(hr))
		{
		hr = pFrameDescription->get_Height(&nColorHeight);
		}
		*/

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}

		if (SUCCEEDED(hr))
		{//色彩不同之处在于，这里可以按格式获取到buffer了（RGBQUAD的格式）
			if (imageFormat == ColorImageFormat_Bgra)
			{
				pBuffer = m_pColorBuffer;
				nBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
				//memcpy(m_pColorBuffer, pBuffer, nBufferSize); 减少一次复制
			}
			else if (m_pColorBuffer)
			{
				pBuffer = m_pColorBuffer;
				nBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
			}
			else
			{
				hr = E_FAIL;
			}
		}

		if (SUCCEEDED(hr))
		{
			*returnbool = true;
			//colorFrames++;
			//cout << "colorFrames  第" << colorFrames << "帧   " << endl;
			//ProcessColor(pBuffer, cColorWidth, cColorHeight);
		}

		SafeRelease(pFrameDescription);
	}

	SafeRelease(pColorFrame);
	return *returnbool;
}
//接着写ProcessBody

Mat MyKinect::ProcessCoordinates(Mat &color, Mat& depth, DepthSpacePoint	* m_pColorCoordinates)
{/* 先对结果求并，得到最小区域。
 然后，求或， 加上深度梯度小的。 得到最大区域。
 用最小的膨胀，如果最大的点//// 看膨胀原理
 还是使用或矩阵，来腐蚀？？？

 得到目标区域后，计算在color的中心位置和大小，
 =》投影到深度，的中心，尺寸按照 colorsize和depthsize比得到。
 */

	/* 用color映射到depth：每个color坐标，对应深度的坐标。
	for color坐标：
	color前景==0 的，
	找对应深度坐标，得到前景值 。==0 则，写入并矩阵
	有一个==0 的写入 或矩阵。
	或矩阵中，深度变化小的周围点也赋值=0

	/或矩阵中，每个是255前景的点==》如果 10的范围 and矩阵 没有一个=255 ，就判断为错误点。
	*/
#ifdef  DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif //  TEST



	Mat nullMat;
	if (color.cols <= 0 || color.rows <= 0)return nullMat;
	if (depth.cols <= 0 || depth.rows <= 0)return nullMat;

	/*	Mat temp;
	morphologyEx(depth, temp, MORPH_OPEN, Mat(3, 3, CV_8U), Point(-1, -1), 2);
	cv::morphologyEx(temp, temp, MORPH_CLOSE, Mat(3, 3, CV_8U), Point(-1, -1), 5);
	imshow("close", temp);*/
	//	resize(color, color, color.size() * 3);   2015-5-9修改为在缩小图上比较前景，加速效果

	Mat orMatrix(color.rows, color.cols, CV_8U);
	Mat andMatrix(color.rows, color.cols, CV_8U);// = color.clone();

	m_pDepthMatInColorSize = Mat(color.rows, color.cols, CV_8UC3, Scalar(0));//初始值
	//	int xxxx = m_pDepthMat_Last.channels();  深度图像也是3通道了

	int depthNchannels = depth.channels();//
	int colorNchannels = color.channels();
	float infinityFloat = -std::numeric_limits<float>::infinity();

	//0是黑色， 255是白色
	int tempsig = -1;//0 都不是,1 彩色矩阵 , 2 深度矩阵 3都是
	//	for (int i = 0; i < cColorHeight_RESIZE; i++)//cColorHeight / RESIZE_FACTOR
	for (int i = 0; i < color.rows; i++)
	{
		uchar * orMatrixptr = orMatrix.ptr<uchar>(i);
		uchar * andMatrixptr = andMatrix.ptr<uchar>(i);
		uchar * colorptr = color.ptr<uchar>(i);
		//Vec3b * tempColorSize = tempColorSizeMat.ptr<Vec3b>(i);//2015-5-9增加   cv_8u3c的如何访问？？？

		uchar * tempColorSize = m_pDepthMatInColorSize.ptr<uchar>(i);
		//uchar * depthMat_Last = m_pDepthMat_Last.ptr<uchar>(i);
		//	for (int j = 0; j < cColorWidth_RESIZE  ; j++)//cColorWidth
		for (int j = 0; j < color.cols; j++)//
		{
			tempsig = 0;
			if (colorptr[j*colorNchannels] == 255)
			{
				tempsig = 1;
			}
			//DepthSpacePoint p = m_pColorCoordinates[i*cColorWidth_RESIZE*RESIZE_FACTOR + j*RESIZE_FACTOR];
			//由于原有的背景缩小了, 而官方的映射是按原尺寸映射的.

			//DepthSpacePoint p = m_pColorCoordinates[i*color.cols * 3 * 3 + j * 3];
			DepthSpacePoint p = m_pColorCoordinates[i*color.cols*COLOR_RESIZE_FACTOR*COLOR_RESIZE_FACTOR
				+ j * COLOR_RESIZE_FACTOR];
			if (p.X != infinityFloat && p.Y != infinityFloat)
			{//说明能映射到
				int colorInDepthX = static_cast<int>(p.X + 0.5f);
				int colorInDepthY = static_cast<int>(p.Y + 0.5f);

				if ((colorInDepthX >= 0 && colorInDepthY < cDepthWidth) &&
					(colorInDepthY >= 0 && colorInDepthY < cDepthHeight))//映射的结果在depth图里
				{

					/* 
					//2015-5-9增加
					//tempColorSize[j] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX);
					tempColorSize[3 * j] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX)[0];
					tempColorSize[3 * j + 1] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX)[1];
					tempColorSize[3 * j + 2] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX)[2];*/
					tempColorSize[3 * j] = m_pDepthMat_Last_Resize.at<Vec3b>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR)[0];
					tempColorSize[3 * j + 1] = m_pDepthMat_Last_Resize.at<Vec3b>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR)[1];
					tempColorSize[3 * j + 2] = m_pDepthMat_Last_Resize.at<Vec3b>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR)[2];
					//	cout << " " << i << "  "<<j;
				//	int depthvalue = depth.at<uchar>(colorInDepthY, colorInDepthX);//传过来的是几个通道
					int depthvalue = depth.at<uchar>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR);//传过来的是几个通道
					
					//注意了 at函数 是matlab类型的，先行后列，先第一维度
					//vec3b 是返回一个数组，直接当数组用就是了
					if (depthvalue == 255)// 0是前景 白色, 255是背景 黑色
					{
						tempsig = 2;
					}
				}
			}
			switch (tempsig)
			{
			case 0:
				orMatrixptr[j*colorNchannels] = 0;
				andMatrixptr[j*colorNchannels] = 0;
				break;
			case 1:
				orMatrixptr[j*colorNchannels] = 255;
				andMatrixptr[j*colorNchannels] = 0;
				break;
			case 2://只要深度有的就是前景，
				orMatrixptr[j*colorNchannels] = 255;
				andMatrixptr[j*colorNchannels] = 255;
				break;
			default:
				break;
			}
		}
	}
	//	resize(orMatrix, orMatrix, cv::Size(color.cols / 3, color.rows / 3));
	//	resize(andMatrix, andMatrix, cv::Size(color.cols / 3, color.rows / 3));

	//	cv::cvtColor(tempColorSizeMat, m_pDepthMatInColorSize, CV_);

	//	cv::GaussianBlur(m_pDepthMatInColorSize, m_pDepthMatInColorSize, Size(3, 3), 0, 0); //Size( w,h ):
	/* imshow("or", orMatrix);
	imshow("and", andMatrix);
	*/imshow("m_pDepthMatInColorSize", m_pDepthMatInColorSize); //得到的映射结果是个有空洞的 图
	imshow("m_pDepthMat_Last", m_pDepthMat_Last);
	cvWaitKey(1);
	
	//改为直接处理吧：如果or的和》》and的3倍，就说明是光照来了。使用and结果，否则使用or结果
	Mat resutl = BaseImageTools::ProduceAndOrMatrix_1(andMatrix, orMatrix);
	//或矩阵中，每个是255前景的点==》如果 10的范围 and矩阵 没有一个=255 ，就判断为错误点。

	//要腐蚀膨胀了
	Mat temp;
	cv::morphologyEx(resutl, temp, MORPH_OPEN, Mat(3, 3, CV_8U), Point(-1, -1), 2);
	cv::morphologyEx(temp, temp, MORPH_CLOSE, Mat(3, 3, CV_8U), Point(-1, -1), 5);


	/*
	测试分割*///修复效果差， 浪费时间
	//	ImageSegment iseg;
	//m_pTempshow = iseg.regiongrowthIndepthWithmean(m_pColorMat_Last_Resize, m_pDepthMatInColorSize, temp);
	//	m_pTempshow = iseg.regiongrowthIndepthWithmeanInList(m_pColorMat_Last_Resize, m_pDepthMatInColorSize, temp);


	//要返回结果
	imshow("TONGBU_MASK", temp);
#ifdef  DEBUG
	mt.End();
	cout << "=============cordinate处理运行时         ===========" << mt.costTime << endl;
#endif	 
	return temp;

	//	imshow("result", result);
	//Mat tem;
	//tempColor.resize(tempColor.cols / 2, );
	//cv::resize(tempColor, tem, cvSize(tempColor.cols / 2, tempColor.rows / 2));
	//	imshow("掩饰", tem);

}
void MyKinect::ProcessCoordinates_andor(Mat color, Mat depth, DepthSpacePoint	* m_pColorCoordinates)
{/* 先对结果求并，得到最小区域。
 然后，求或， 加上深度梯度小的。 得到最大区域。
 用最小的膨胀，如果最大的点//// 看膨胀原理
 还是使用或矩阵，来腐蚀？？？

 得到目标区域后，计算在color的中心位置和大小，
 =》投影到深度，的中心，尺寸按照 colorsize和depthsize比得到。
 */

	/* 用color映射到depth：每个color坐标，对应深度的坐标。
	for color坐标：
	color前景==0 的，
	找对应深度坐标，得到前景值 。==0 则，写入并矩阵
	有一个==0 的写入 或矩阵。
	或矩阵中，深度变化小的周围点也赋值=0

	或矩阵中，每个是255前景的点==》如果4的范围 and矩阵 没有一个=255 ，就判断为错误点。
	*/

	if (color.cols <= 0 || color.rows <= 0)return;
	if (depth.cols <= 0 || depth.rows <= 0)return;
	resize(color, color, color.size() * 3);

	Mat orMatrix(color.rows, color.cols, CV_8U);
	Mat andMatrix(color.rows, color.cols, CV_8U);// = color.clone();

	int depthNchannels = depth.channels();//
	int colorNchannels = color.channels();
	float infinityFloat = -std::numeric_limits<float>::infinity();

	//0是黑色， 255是白色
	int tempsig = -1;//0 都不是,1 或矩阵 , 2 and矩阵
	//	for (int i = 0; i < cColorHeight_RESIZE; i++)//cColorHeight / RESIZE_FACTOR
	for (int i = 0; i < color.rows; i++)
	{
		uchar * orMatrixptr = orMatrix.ptr<uchar>(i);
		uchar * andMatrixptr = andMatrix.ptr<uchar>(i);
		uchar * colorptr = color.ptr<uchar>(i);
		//	for (int j = 0; j < cColorWidth_RESIZE  ; j++)//cColorWidth
		for (int j = 0; j < color.cols; j++)//
		{
			tempsig = 0;
			if (colorptr[j*colorNchannels] == 255)
			{
				tempsig++;
			}
			//DepthSpacePoint p = m_pColorCoordinates[i*cColorWidth_RESIZE*RESIZE_FACTOR + j*RESIZE_FACTOR];
			//由于原有的背景缩小了, 而官方的映射是按原尺寸映射的.

			DepthSpacePoint p = m_pColorCoordinates[i*color.cols + j];
			if (p.X != infinityFloat && p.Y != infinityFloat)
			{//说明能映射到
				int colorInDepthX = static_cast<int>(p.X + 0.5f);
				int colorInDepthY = static_cast<int>(p.Y + 0.5f);

				if ((colorInDepthX >= 0 && colorInDepthY < cDepthWidth) &&
					(colorInDepthY >= 0 && colorInDepthY < cDepthHeight))//映射的结果在depth图里
				{
					//	cout << " " << i << "  "<<j;
					int depthvalue = depth.at<uchar>(colorInDepthY, colorInDepthX);//传过来的是几个通道
					//注意了 at函数 是matlab类型的，先行后列，先第一维度
					//vec3b 是返回一个数组，直接当数组用就是了
					if (depthvalue == 255)// 0是前景 白色, 255是背景 黑色
					{
						tempsig++;
					}
				}
			}
			switch (tempsig)
			{
			case 0:
				orMatrixptr[j*colorNchannels] = 0;
				andMatrixptr[j*colorNchannels] = 0;
				break;
			case 1:
				orMatrixptr[j*colorNchannels] = 255;
				andMatrixptr[j*colorNchannels] = 0;
				break;
			case 2:
				orMatrixptr[j*colorNchannels] = 255;
				andMatrixptr[j*colorNchannels] = 255;
				break;
			default:
				break;
			}
		}
	}
	resize(orMatrix, orMatrix, cv::Size(color.cols / 3, color.rows / 3));
	resize(andMatrix, andMatrix, cv::Size(color.cols / 3, color.rows / 3));


	//imshow("or", orMatrix);
	//imshow("and", andMatrix);
	//改为直接处理吧：如果or的和》》and的3倍，就说明是光照来了。使用and结果，否则使用or结果
	Mat resutl = BaseImageTools::ProduceAndOrMatrix(andMatrix, orMatrix);//效果不好，不如直接用or的图, 可能开光灯有效果
	//深度图是基础， 然后用或来填充。

	//	imshow("result", result);
	//Mat tem;
	//tempColor.resize(tempColor.cols / 2, );
	//cv::resize(tempColor, tem, cvSize(tempColor.cols / 2, tempColor.rows / 2));
	//	imshow("掩饰", tem);

}

void MyKinect::ProcessCoordinatesTest(Mat color, Mat depth, DepthSpacePoint	* m_pColorCoordinates)
{
	if (color.cols <= 0 || color.rows <= 0)return;
	if (depth.cols <= 0 || depth.rows <= 0)return;
	Mat tempColor = color.clone();
	int depthNchannels = depth.channels();
	int colorNchannels = color.channels();
	float infinityFloat = -std::numeric_limits<float>::infinity();
	for (int i = 0; i < cColorHeight; i++)
	{
		//uchar * ptrDepth = depth.ptr<uchar>(i);
		uchar * ptrColor = tempColor.ptr<uchar>(i);
		for (int j = 0; j < cColorWidth; j++)
		{
			/*	if ( j == 1448)
			{
			j = j;
			}
			*/
			DepthSpacePoint p = m_pColorCoordinates[i*cColorWidth + j];

			if (p.X != infinityFloat && p.Y != infinityFloat)
			{//说明能映射到
				int colorInDepthX = static_cast<int>(p.X + 0.5f);
				int colorInDepthY = static_cast<int>(p.Y + 0.5f);
				if ((colorInDepthX >= 0 && colorInDepthY < cDepthWidth) &&
					(colorInDepthY >= 0 && colorInDepthY < cDepthHeight))//映射的结果在depth图里
				{
					//	cout << " " << i << "  "<<j;
					int depthvalue = depth.at<Vec3b>(colorInDepthY, colorInDepthX)[0];
					//注意了 at函数 是matlab类型的，先行后列，先第一维度
					//vec3b 是返回一个数组，直接当数组用就是了
					if (depthvalue < 100)//深度的坐标范围不同
					{
						ptrColor[j*colorNchannels] = depthvalue;
					}
				}
			}
		}
	}
	Mat tem;
	//tempColor.resize(tempColor.cols / 2, );
	cv::resize(tempColor, tem, cvSize(tempColor.cols / 2, tempColor.rows / 2));
	//	imshow("掩饰", tem);

}
void MyKinect::ProcessColor(RGBQUAD* pBuffer, int nWidth, int nHeight)
{

#ifdef DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif // DEBUG

	// Make sure we've received valid data
	if (pBuffer && (nWidth == cColorWidth) && (nHeight == cColorHeight))
	{
		colorDepthFramesynchronization = true;//用于同步，获取了color新的帧，那么depth也要处理，即使没有也要用上一帧处理
		colorFrames++;
		cout << "colorFrames第" << colorFrames << "帧" << endl;

		//if (isFirstFrame)
		HRESULT hr = m_pCoordinateMapper->MapColorFrameToDepthSpace
			(cDepthWidth * cDepthHeight, (UINT16*)m_pDepthBuffer, cColorWidth * cColorHeight, m_pColorCoordinates);

		// Draw the data with OpenCV
		Mat ColorImage(nHeight, nWidth, CV_8UC4, pBuffer);
		//建立8位无符号数*4个通道  的矩阵
		m_pColorMat_Last = ColorImage.clone();//保存最新color结果
		//	
		cv::resize(m_pColorMat_Last, m_pColorMat_Last_Resize, cvSize(cColorWidth_RESIZE, cColorHeight_RESIZE));
		//cv::moveWindow("color", 500, 0);
		//imshow("color", m_pColorMat_Last_Resize);
		//cvWaitKey(1);
		//cout << "smallcolor :"<<smallColor.channels();

		//colorProcess = new MixtureOfGaussianV1BGS;要转到rgb上
		Mat tempColor = m_pColorMat_Last_Resize.clone();
		IplImage * tempRGB = &IplImage(tempColor);//
		IplImage * rawImage = cvCreateImage(cvGetSize(tempRGB), 8, 3);
		cvCvtColor(tempRGB, rawImage, CV_RGBA2BGR);
		Mat smallColor;
		smallColor = rawImage;
		/**/
		//	cout << "smallcolor :" << smallColor.channels();
		//Mat foreMat;
		//	m_pDepthBackProcess.testManyMethod(smallColor, foreMat);
		m_pDepthBackProcess.colorProcessBackGround(smallColor, m_pColorMat_LastMask_Resize);


		/*测试 分割
		//转灰度
		Mat tempgray;
		Mat result;
		IplImage * grayImage = cvCreateImage(cvGetSize(tempRGB), 8, 1);
		cvCvtColor(tempRGB, grayImage, CV_BGR2GRAY);
		tempgray = grayImage;

		ImageSegment isegment;
		result=isegment.regiongrowth(tempgray, m_pColorMat_LastMask_Resize);

		m_pTempshow = result.clone();
		*/



		//	BaseImageTools::SaveColorImageToAVI(ColorImage, nWidth,nHeight,rgbWriter);
		/*
		Mat showImage;
		resize(ColorImage, showImage, Size(nWidth / 2, nHeight / 2));
		imshow("ColorImage", showImage);////imshow("ColorImage", ColorImage);
		//这里都没有释放，============================================
		//这里要看看 要如何释放
		IplImage  pSrcImage(showImage);


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
		}
		*/
	}

#ifdef DEBUG 
	mt.End();
	cout << "color处理运行时           ：：：：：：" << mt.costTime << endl;
#endif

}

