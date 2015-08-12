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
	m_pDepthRGBX = new RGBQUAD[cDepthWidth * cColorHeight];//RGBQUAD�Ƕ�ά�ȵ�
	m_pDepthRGBX_Last = new RGBQUAD[cDepthWidth*cColorHeight];//��һ֡�ı���

	//Mat ��ʵ����namespaceʵ�ֵģ���������inline������������ ����new��new�����Ķ���ָ���ˡ�
	//m_pDepthMat_Last= new cv::Mat(cv::Size(cDepthWidth, cColorHeight), CV_8UC4,cvScalar(0));
	//ֱ��д��mat�Ϳ�����
	//m_pDepthMat_Last = Mat(cv::Size(cDepthWidth, cColorHeight), CV_8UC4, cvScalar(0));
	 //�������create����Ҳ����
	//m_pDepthMat_Last.create(cv::Size(cDepthWidth, cColorHeight), CV_8UC4);
	m_pDepthMat_Last = cv::Mat::zeros(cDepthHeight,cDepthWidth,  CV_8UC3);
	m_pDepthMat_LastMask = cv::Mat::zeros(cDepthHeight, cDepthWidth, CV_8UC1);
	//m_pDepthBackProcess =  BackGroundProcess();//���Ǹ�ֵ����ĳ�ʼ��ֻ����������

	memset(m_pDepthRGBX_Last, 0, sizeof(RGBQUAD));//��ʼΪ0

//	rgbWriter = BaseImageTools::NewWrite(cColorWidth/2,cColorHeight/2,"Color.avi");
	depthWriter = BaseImageTools::NewWrite(cDepthWidth, cDepthHeight, "Depth.avi");
	//depthWriter2 = BaseImageTools::NewWrite(cDepthWidth, cDepthHeight, "Depth2.avi");

	//opencv����Щ������֧�ֶ��̰߳�
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
	pTracker = (CvBlobTrackerAuto1*)new CvBlobTrackerAuto1(); //cvCreateBlobTrackerAuto1();//��ʼ������ģ�顣
	//��ֱ�ӵ�ָ���ã�������
}

MyKinect::~MyKinect()
{
	if (m_pColorRGBX)
	{//�ͷ� RGB��������
		delete[] m_pColorRGBX;
		m_pColorRGBX = NULL;
	}
	//�ͷ�reader��
	SafeRelease(m_pColorFrameReader);// done with color frame reader

	if (m_pKinectSensor)
	{//�ر�kinect
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
		{//��ȡӳ���ϵ��  ���Է���openǰ��
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
		}

		if (SUCCEEDED(hr))
		{//ע�� ���һ��Դû�п�������ô������ɹ�
			hr = m_pKinectSensor->OpenMultiSourceFrameReader(//FrameSourceTypes::FrameSourceTypes_Audio|
				FrameSourceTypes::FrameSourceTypes_Body|
				FrameSourceTypes::FrameSourceTypes_BodyIndex|FrameSourceTypes::FrameSourceTypes_Color|
				FrameSourceTypes::FrameSourceTypes_Depth|FrameSourceTypes::FrameSourceTypes_Infrared,
				&m_pMultiSourceFrameReader);
		}
		//MUltiû��source������mutilreader��Ȼ������reader-���ֱ��ȡframe
		//����û��source�ͷ� 

		/* ��Ƶ���Ӻ�����Ĳ�һ��
		//Ҳ��source -��render-����ע���¼�
		// ��ȡ��ƵԴ(AudioSource)
		if (SUCCEEDED(hr)){
			hr = m_pKinectSensor->get_AudioSource(&m_pAudioSource);
		}
		// �ٻ�ȡ��Ƶ֡��ȡ��
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

	//��Դ�����Ȼ�ȡһ���ܵ�Դframe��Ȼ��ͨ����ȡ���Ե�reference����ȡ���Ե�frame

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

		//��øĳ�4���̲߳���
		//��ɫ֡��ǰ�棬�Ͷ��������֡�ˣ���
		bool *colorFrameReturn ; bool tcolor = false; colorFrameReturn = &tcolor;
		bool *depthFrameReturn ; bool tdepth = false; depthFrameReturn = &tdepth;
		bool *bodyIndexFrameReturn; bool tbodyIndex = false; bodyIndexFrameReturn = &tbodyIndex;
		bool * bodyFrameReturn; bool tbody = false; bodyFrameReturn = &tbody;

	//	bool *audioFrameReturn; bool taudio = false; audioFrameReturn = &taudio;


	//	bool * te = false;
		std::thread thread_Color_Copy(&MyKinect::Process_ColorFrame, this, pMultiSourceFrame, colorFrameReturn);
	//	bool colorFrameReturn = Produce_ColorFrame(pMultiSourceFrame);
		std::thread thread_Depth_Copy(&MyKinect::Process_DepthFrame, this, pMultiSourceFrame, depthFrameReturn);
		//���������޷���ý����ֻ����ָ�룬��ȡֵ

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
		cout << "����color buffer ����ʱ" << mt.costTime << endl;
		mt.Reset();
		mt.Start();
		bool depthFrameReturn=Process_DepthFrame(pMultiSourceFrame);
		//���������浽ȫ�ֱ����С�
		mt.End();
		cout << "����depth buffer ����ʱ" << mt.costTime << endl;
		*/
		
		thread_Color_Copy.join();//�ȴ����߳̽���
		thread_Depth_Copy.join();
		thread_Body_Copy.join();
		thread_BodyIndex_Copy.join();
	
	//	Process_BodyFrame(pMultiSourceFrame);
	//	Process_BodyIndexFrame(pMultiSourceFrame);
		if (*colorFrameReturn)
		{
			//��һ����ʹ��ͼƬ��С��3
			thread_Color_Process=std::thread(&MyKinect::ProcessColor, this, m_pColorBuffer, cColorWidth, cColorHeight);
		//	MyTime mt;
		//	mt.Reset();
		//	mt.Start();
		//	ProcessColor(m_pColorBuffer, cColorWidth, cColorHeight);			
		//	mt.End();
		//	cout << "color���� ����ʱ" << mt.costTime << endl;
		}
		
		if (*depthFrameReturn)
		{
			thread_Depth_Process=std::thread(&MyKinect::ProcessDepth, this, m_pDepthBuffer, cDepthWidth, cDepthHeight, cDepthMinReliableDistance, cDepthMaxDistance);
		/*	MyTime mt;
			mt.Reset();
			mt.Start(); 
		 	ProcessDepth(m_pDepthBuffer, cDepthWidth, cDepthHeight, cDepthMinReliableDistance, cDepthMaxDistance);
		//	mt.End();
		//	cout << "depth��������ʱ" << mt.costTime << endl;*/
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

		// 1  ���߳� color ��depth �ֿ����١�


		
		//�ȸ��ٴ���
		if (prcossedFrames > 10 && m_pColorMat_Last_Resize.rows > 0 && m_pColorMat_LastMask_Resize.rows > 0)
		{
			IplImage ipl_pColorMat_Last = m_pColorMat_Last_Resize;
			IplImage ipl_pColorMat_LastMask = m_pColorMat_LastMask_Resize;
			pTracker->ProcessTracker(&ipl_pColorMat_Last, &ipl_pColorMat_LastMask);
			//	pTracker->ProcessNewBlob(&ipl_pColorMat_Last, &ipl_pColorMat_LastMask);
			pTracker->showAndSaveResult(&ipl_pColorMat_Last);
		}
/**/

		/**ͬ��֡ ��
		1:��ȡ��ϱ����� 
		2:�����ڵ� �� ������һ֡���ſ飬 ���µ���ӵ� ����������
		3��.
		*/

		
		//����colorlast��ԭ����С��,����color������С��,
		if ((*depthFrameReturn) && (*colorFrameReturn))//����ͬ������
		{
			prcossedFrames++;//ǰ��֡������
			
		//	m_pSyncMat_MASK=ProcessCoordinates(m_pColorMat_LastMask_Resize, m_pDepthMat_LastMask, m_pColorCoordinates);
			m_pSyncMat_MASK = ProcessCoordinates(m_pColorMat_LastMask_Resize, m_pDepthMat_LastMask_Resize, m_pColorCoordinates);
			//Ҫ����ǰ����
			if (prcossedFrames > 5 && m_pSyncMat_MASK.rows > 0 && m_pSyncMat_MASK.rows > 0)
			{
			
				IplImage ipl_syn= m_pSyncMat_MASK;
				IplImage ipl_pColorMat_Last = m_pColorMat_Last_Resize;
				pTracker->ProcessNewBlob(&ipl_pColorMat_Last, &ipl_syn);
			}	
		}
		
	/*	*/		
		//cvWaitKey(500); ���û�ȡģʽ����500Ҳû����
	}

	++nFrames;
//	cout << "update  ��" << nFrames << "֡   " << endl;
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
		{//�����ǽ�ָ��buffer��ȡ�����ˣ�û�п���
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);//�����size��ushort���Եģ�memcopy��uchar���ġ�
		}

		if (SUCCEEDED(hr))
		{
			//int tempsize = cDepthHeight*cDepthWidth *sizeof(USHORT);
			memcpy(m_pDepthBuffer, pBuffer, nBufferSize*sizeof(USHORT));

			*returnbool = true;
			//ProcessDepth(pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
		}

		SafeRelease(pFrameDescription);//Description ��Frame ��Ҫ�ͷŵ�
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
		cout << "    ���֡��ʧ" << ++depthLostFrames << endl;
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
		{//�����ǽ�ָ��buffer��ȡ�����ˣ�û�п���
			hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);//�����size��ushort���Եģ�memcopy��uchar���ġ�
		}

		if (SUCCEEDED(hr))
		{
			//int tempsize = cDepthHeight*cDepthWidth *sizeof(USHORT);
			memcpy(m_pDepthBuffer, pBuffer, nBufferSize*sizeof(USHORT));

			*returnbool = true;
			//ProcessDepth(pBuffer, nWidth, nHeight, nDepthMinReliableDistance, nDepthMaxDistance);
		}

		SafeRelease(pFrameDescription);//Description ��Frame ��Ҫ�ͷŵ�
	}
/*	else if (colorDepthFramesynchronization)//���֡û�ˣ����ǲ�ɫ֡�����ˣ���Ҫ����һ֡��
	{//û�õģ� �Ѿ�����һ֡��coordinates�����ˣ���Ͷ�����
	}
	*/
	SafeRelease(pDepthFrame);
	
	return *returnbool;

}

void MyKinect::ProcessDepth(const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth)
{//������ȣ����޸�,��������
	
#ifdef DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif


	// Make sure we've received valid data
	if (m_pDepthRGBX && pBuffer && (nWidth == cDepthWidth) && (nHeight == cDepthHeight))
	{

		depthFrames++;
	 	cout << "depthFrames��" << depthFrames << "֡" << endl;
		RGBQUAD* pRGBX = m_pDepthRGBX;
		const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);
		int k1 = 0, k2 = 0; int k3 = 0, k4 = 0;

		Mat inpaintResult;//��ͼ���
		Mat inpaintMask;//����		
		Mat badMask;//����������

		badMask = Mat::zeros(Size(nWidth, nHeight), CV_8U);
		inpaintMask = Mat::zeros(Size(nWidth, nHeight), CV_8U);
	
		/*uchar * inpaintDataMask = inpaintMask.data;
		uchar * badMaskData = badMask.data;//��ǵ�ǰ֡ ���������
		uchar * depthMat_LastData= m_pDepthMat_Last.data;
		uchar * depthMat_LastDataMask = m_pDepthMat_LastMask.data;//ǰ��������
		*/
	/*	int a = m_pDepthMat_Last.channels();
		int b = m_pDepthMat_LastMask.channels();
		int c = badMask.channels();
		int d = inpaintMask.channels();
		cout << a << "   " << b << "   " << c << "   " << d << endl;
		*/
		//���ǵ�mat�ֽڶ��룬��Ϊfor
		for (int i = 0; i < nHeight;i++)
		{
			uchar * inpaintDataMask = inpaintMask.ptr<uchar>(i);
			uchar * badMaskData = badMask.ptr<uchar>(i);//��ǵ�ǰ֡ ���������
			uchar * depthMat_LastData = m_pDepthMat_Last.ptr<uchar>(i);
			uchar * depthMat_LastDataMask = m_pDepthMat_LastMask.ptr<uchar>(i);//ǰ��������

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
				depth = depth >> 4;//�����õĽ� 450 -- 5500ӳ�䵽 0 -255����
				//�д���=0�ĵ㣬2W�� ,

				if (depth >= 255) {//���븳ֵ
					depth = 255;
				}
				BYTE intensity = static_cast<BYTE>(depth);

				//����һ֡����
				if (maskOfPixel)
				{//�������
					k3++;
					
					//��һ֡���޸�ֻ�ܶԱ����ã����ܶ��˶�����
					//if ((depthMat_LastDataMask[j]) == 0)//�Ǳ���
					//��Ҫ�ĵط����� ���˶��ı�Ե�����ⶼ��ǰ�������ˡ�		
					
					{//��������8u3c��
						intensity = (depthMat_LastData[j*3]);//������һ֡
					}

					if (intensity == 0)// || intensity == 255))//������� ����ͷ�̶����еĵ��һֱ����
					{//��һ֡Ҳ�Ǵ���㣬��û�޸�,�����޸�					
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

	// std::cout << setw(5)<< k1 << setw(5) << k2  <<"Ϊ0�㣺"<<setw(5)<<k3<<"  ������"<<setw(5)<<k4<<"  "<<" "<<std::endl ;
		//������һ֡
	//	memcpy( m_pDepthRGBX_Last,m_pDepthRGBX, sizeof(RGBQUAD)*cDepthHeight*cDepthWidth);

		// Draw the data with OpenCV
		Mat DepthImage(nHeight, nWidth, CV_8UC4, m_pDepthRGBX);
		//imshow("orignal", DepthImage);
		//cvMoveWindow("orignal",0,0);
		Mat depthImageRGB;
		cvtColor(DepthImage,depthImageRGB,CV_RGBA2RGB);
		cv::inpaint(depthImageRGB, inpaintMask, inpaintResult, 3, CV_INPAINT_TELEA);// CV_INPAINT_NS);//�ն�̫����Ч������
		//ʹ����inpaint�󣬾��ǵ�һ֡���޸��������Ķ������޸��ˡ�
	//	imshow("inpaint", inpaintResult);
	//	cvMoveWindow("inpaint", 500, 0);

		//cvSmooth(&inpaintResult, &m_pDepthMat_Last);����c��ʽ��֧��cvmat�ģ�����Ĳ���c++��
	//	Mat  blurMat;
	//	blur(inpaintResult, blurMat, Size(3, 3), Point(-1, -1)); //Size( w,h ):
	//	imshow("blur", blurMat);
		//cvMoveWindow("blur",500,500);
	
		//2015-5-10 �� ���޸�Ϊ��С��
		cv::resize(m_pDepthMat_Last, m_pDepthMat_Last_Resize, cv::Size(cDepthWidth_RESIZE, cDepthHeight_RESIZE));

		//�� fuliye�汾 ����λ��
		cv::GaussianBlur(inpaintResult, m_pDepthMat_Last, Size(3, 3), 0,0); //Size( w,h ):
	
	


	//	m_pDepthMat_Last = inpaintResult.clone();//������һ֡ 

	//	imshow("gaussianBlur", m_pDepthMat_Last);
		//cvMoveWindow("gaussianBlur",0,500);
	//	cvMoveWindow("gaussianBlur", 0, 0);
	//	cvWaitKey(1);

	//	cout << "m_pDepthMat_Last :" << m_pDepthMat_Last.channels();
	 	 Mat foreMat;//��ȡǰ��
	 	//m_pDepthBackProcess.process_Mog2(m_pDepthMat_Last, foreMat);
		// m_pDepthBackProcess.depthProcessBackGround(m_pDepthMat_Last, m_pDepthMat_LastMask);
	// 	 m_pDepthBackProcess.testManyMethod(m_pDepthMat_Last, foreMat);

		 m_pDepthBackProcess.depthProcessBackGround(m_pDepthMat_Last_Resize, m_pDepthMat_LastMask_Resize);


		 /*
		 ���Էָ� 	 
		 ImageSegment iseg;
		 m_pTempshow=iseg.regiongrowthIndepth3(m_pDepthMat_Last, m_pDepthMat_LastMask);
*/
		/* if( (foreMat.rows !=0 )&& (foreMat.cols !=0))
		 {	
			 m_pDepthMat_LastMask = foreMat.clone();//����ǰ��������
		 }*/
	 


	//	cout <<"ͨ����Ŀ"<< inpaintResult.channels();
	//	BaseImageTools::SaveDepthImageToAVI(inpaintResult, nWidth, nHeight, depthWriter);

	//	BaseImageTools::SaveDepthImageToAVI(inpaintResult, nWidth, nHeight, depthWriter2);

		//BaseImageTools::SaveDepthImageToAVI(DepthImage, nWidth, nHeight, depthWriter);
/*		Mat show = DepthImage.clone();
		imshow("DepthImage2", show);
		*/
	}

#ifdef DEBUG
	mt.End();
	cout << "depth��������ʱ           ��" << mt.costTime << endl;
#endif // DEBUG


}

/*��ɲ��� ͼƬ ������һ֡���� inpaint��䣬��blurƽ����
inpaint��ʹ���д���ߡ�
�Ϸ����Ŀն���δ�����*/


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
		{	//����һ��Ҫ��ֵ��ȥ���У� ������ûЧ��
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
	cout << "bodyindex ��������ʱ           ������������" << mt.costTime << endl;
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
		hr = pBodyFrame->get_RelativeTime(&m_nTime);//ʱ�����������fps
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
		//SafeRelease(ppBodies);�����ã�
		//������IBody * ppBodies[6]  ����һ��6���ȵ����飬����������ָ�룬
		//���Բ����ͷ�ppbodies �����Լ�������int ** ��άָ���ǲ�ͬ��

	}
	SafeRelease(pBodyFrame);//frmae ��ÿ�ζ�Ҫ���£�����Ҫ�ͷ�
	//Reader��һ�����룬�Ժ�ÿ��Ҫ��Reader��ȡframe�����Բ�Ҫ�ͷ���
	//source���ʼ ������ȡreader�ģ����Ͼ��ͷ���

	return *returnbool;
}

void MyKinect::ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
{

#ifdef DEBUG
	MyTime mt;
	mt.Reset();
	mt.Start();
#endif // DEBUG

	//�����Ƶ��������
	//Mat BodyMat(cDepthWidth,cDepthHeight,CV_8UC3);
	Mat &BodyMat = m_bodyMat;
	BodyMat=Mat::zeros(cDepthHeight, cDepthWidth, CV_8UC3);
	//Mat ColorBodyMat = Mat::zeros(cColorHeight,cColorWidth,  CV_8UC3);
	for (int i = 0; i < nBodyCount; i++)
	{
		IBody *pBody = ppBodies[i];
		if (pBody)
		{

			BOOLEAN bTracked = false;//boolean��byte���ض��� BOOL��int���ض���
			HRESULT hr = NULL;
			//hr = pBody->get_IsRestricted(&bTracked);  �������Ƶ�; ���Ǹ������ ��������������

			hr = pBody->get_IsTracked(&bTracked);
			//kinect�ĵ��ô�඼�� �þ���ķ�ʽ�����Ƿ�ɹ���������Żض���ָ�롣
			if (SUCCEEDED(hr) && bTracked)
			{//��ȡÿ��body�ĵ�
				Joint joints[JointType_Count];
				HandState leftHandState = HandState_Unknown;
				HandState rightHandState = HandState_Unknown;
				pBody->get_HandLeftState(&leftHandState);
				pBody->get_HandRightState(&rightHandState);

				hr = pBody->GetJoints(_countof(joints), joints);

				if (SUCCEEDED(hr))
				{
					for (int j = 0; j < _countof(joints); ++j)
					{//��ȡÿһ���� ����
						/*ColorSpacePoint colorPoint = { 0 };
						m_pCoordinateMapper->MapCameraPointToColorSpace(
						joints[j].Position, &colorPoint);
						//����ά�ĵ�ת��opencv�ϵĵ�
						Point temp;
						temp.x = colorPoint.X;
						temp.y = colorPoint.Y;
						*/
						DepthSpacePoint depthPoint = { 0 };
						m_pCoordinateMapper->MapCameraPointToDepthSpace(joints[j].Position, &depthPoint);

						//����ά�ĵ�ת��opencv�ϵĵ�
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
	cout << "body��������ʱ           ������������" << mt.costTime << endl;
#endif
}





bool  MyKinect::Process_ColorFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool)
{//����multireader�л�ȡlastframe��Ȼ����Ǹ��Ի�ȡFrame��reference���������õ���˼������
	if (!pMultiSourceFrame)	{ *returnbool = false;	return false; }
	*returnbool = false;

	IColorFrameReference* pColorFrameReference = NULL;
	IColorFrame* pColorFrame = NULL;
	//��ȡɫ��֡������
	HRESULT hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
	//������������ˣ� û�ж�ȡ����ɫ֡��������

	// hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
	if (SUCCEEDED(hr))
	{
		hr = pColorFrameReference->AcquireFrame(&pColorFrame);
	}

	/*if (!SUCCEEDED(hr))
	{
	cout << "     û�ж�����ɫ֡ " << ++colorLostFrames;
	}*/
	SafeRelease(pColorFrameReference);//������û�Ҫ�ͷ���
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
		{//ɫ�ʲ�֮ͬ�����ڣ�������԰���ʽ��ȡ��buffer�ˣ�RGBQUAD�ĸ�ʽ��
			if (imageFormat == ColorImageFormat_Bgra)
			{
				pBuffer = m_pColorBuffer;
				nBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
				//memcpy(m_pColorBuffer, pBuffer, nBufferSize); ����һ�θ���
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
			//cout << "colorFrames  ��" << colorFrames << "֡   " << endl;
			//ProcessColor(pBuffer, cColorWidth, cColorHeight);
		}

		SafeRelease(pFrameDescription);
	}

	SafeRelease(pColorFrame);
	return *returnbool;
}
//����дProcessBody

Mat MyKinect::ProcessCoordinates(Mat &color, Mat& depth, DepthSpacePoint	* m_pColorCoordinates)
{/* �ȶԽ���󲢣��õ���С����
 Ȼ����� ��������ݶ�С�ġ� �õ��������
 ����С�����ͣ�������ĵ�//// ������ԭ��
 ����ʹ�û��������ʴ������

 �õ�Ŀ������󣬼�����color������λ�úʹ�С��
 =��ͶӰ����ȣ������ģ��ߴ簴�� colorsize��depthsize�ȵõ���
 */

	/* ��colorӳ�䵽depth��ÿ��color���꣬��Ӧ��ȵ����ꡣ
	for color���꣺
	colorǰ��==0 �ģ�
	�Ҷ�Ӧ������꣬�õ�ǰ��ֵ ��==0 ��д�벢����
	��һ��==0 ��д�� �����
	������У���ȱ仯С����Χ��Ҳ��ֵ=0

	/������У�ÿ����255ǰ���ĵ�==����� 10�ķ�Χ and���� û��һ��=255 �����ж�Ϊ����㡣
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
	//	resize(color, color, color.size() * 3);   2015-5-9�޸�Ϊ����Сͼ�ϱȽ�ǰ��������Ч��

	Mat orMatrix(color.rows, color.cols, CV_8U);
	Mat andMatrix(color.rows, color.cols, CV_8U);// = color.clone();

	m_pDepthMatInColorSize = Mat(color.rows, color.cols, CV_8UC3, Scalar(0));//��ʼֵ
	//	int xxxx = m_pDepthMat_Last.channels();  ���ͼ��Ҳ��3ͨ����

	int depthNchannels = depth.channels();//
	int colorNchannels = color.channels();
	float infinityFloat = -std::numeric_limits<float>::infinity();

	//0�Ǻ�ɫ�� 255�ǰ�ɫ
	int tempsig = -1;//0 ������,1 ��ɫ���� , 2 ��Ⱦ��� 3����
	//	for (int i = 0; i < cColorHeight_RESIZE; i++)//cColorHeight / RESIZE_FACTOR
	for (int i = 0; i < color.rows; i++)
	{
		uchar * orMatrixptr = orMatrix.ptr<uchar>(i);
		uchar * andMatrixptr = andMatrix.ptr<uchar>(i);
		uchar * colorptr = color.ptr<uchar>(i);
		//Vec3b * tempColorSize = tempColorSizeMat.ptr<Vec3b>(i);//2015-5-9����   cv_8u3c����η��ʣ�����

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
			//����ԭ�еı�����С��, ���ٷ���ӳ���ǰ�ԭ�ߴ�ӳ���.

			//DepthSpacePoint p = m_pColorCoordinates[i*color.cols * 3 * 3 + j * 3];
			DepthSpacePoint p = m_pColorCoordinates[i*color.cols*COLOR_RESIZE_FACTOR*COLOR_RESIZE_FACTOR
				+ j * COLOR_RESIZE_FACTOR];
			if (p.X != infinityFloat && p.Y != infinityFloat)
			{//˵����ӳ�䵽
				int colorInDepthX = static_cast<int>(p.X + 0.5f);
				int colorInDepthY = static_cast<int>(p.Y + 0.5f);

				if ((colorInDepthX >= 0 && colorInDepthY < cDepthWidth) &&
					(colorInDepthY >= 0 && colorInDepthY < cDepthHeight))//ӳ��Ľ����depthͼ��
				{

					/* 
					//2015-5-9����
					//tempColorSize[j] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX);
					tempColorSize[3 * j] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX)[0];
					tempColorSize[3 * j + 1] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX)[1];
					tempColorSize[3 * j + 2] = m_pDepthMat_Last.at<Vec3b>(colorInDepthY, colorInDepthX)[2];*/
					tempColorSize[3 * j] = m_pDepthMat_Last_Resize.at<Vec3b>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR)[0];
					tempColorSize[3 * j + 1] = m_pDepthMat_Last_Resize.at<Vec3b>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR)[1];
					tempColorSize[3 * j + 2] = m_pDepthMat_Last_Resize.at<Vec3b>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR)[2];
					//	cout << " " << i << "  "<<j;
				//	int depthvalue = depth.at<uchar>(colorInDepthY, colorInDepthX);//���������Ǽ���ͨ��
					int depthvalue = depth.at<uchar>(colorInDepthY / DEPTH_RESIZE_FACTOR, colorInDepthX / DEPTH_RESIZE_FACTOR);//���������Ǽ���ͨ��
					
					//ע���� at���� ��matlab���͵ģ����к��У��ȵ�һά��
					//vec3b �Ƿ���һ�����飬ֱ�ӵ������þ�����
					if (depthvalue == 255)// 0��ǰ�� ��ɫ, 255�Ǳ��� ��ɫ
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
			case 2://ֻҪ����еľ���ǰ����
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
	*/imshow("m_pDepthMatInColorSize", m_pDepthMatInColorSize); //�õ���ӳ�����Ǹ��пն��� ͼ
	imshow("m_pDepthMat_Last", m_pDepthMat_Last);
	cvWaitKey(1);
	
	//��Ϊֱ�Ӵ���ɣ����or�ĺ͡���and��3������˵���ǹ������ˡ�ʹ��and���������ʹ��or���
	Mat resutl = BaseImageTools::ProduceAndOrMatrix_1(andMatrix, orMatrix);
	//������У�ÿ����255ǰ���ĵ�==����� 10�ķ�Χ and���� û��һ��=255 �����ж�Ϊ����㡣

	//Ҫ��ʴ������
	Mat temp;
	cv::morphologyEx(resutl, temp, MORPH_OPEN, Mat(3, 3, CV_8U), Point(-1, -1), 2);
	cv::morphologyEx(temp, temp, MORPH_CLOSE, Mat(3, 3, CV_8U), Point(-1, -1), 5);


	/*
	���Էָ�*///�޸�Ч��� �˷�ʱ��
	//	ImageSegment iseg;
	//m_pTempshow = iseg.regiongrowthIndepthWithmean(m_pColorMat_Last_Resize, m_pDepthMatInColorSize, temp);
	//	m_pTempshow = iseg.regiongrowthIndepthWithmeanInList(m_pColorMat_Last_Resize, m_pDepthMatInColorSize, temp);


	//Ҫ���ؽ��
	imshow("TONGBU_MASK", temp);
#ifdef  DEBUG
	mt.End();
	cout << "=============cordinate��������ʱ         ===========" << mt.costTime << endl;
#endif	 
	return temp;

	//	imshow("result", result);
	//Mat tem;
	//tempColor.resize(tempColor.cols / 2, );
	//cv::resize(tempColor, tem, cvSize(tempColor.cols / 2, tempColor.rows / 2));
	//	imshow("����", tem);

}
void MyKinect::ProcessCoordinates_andor(Mat color, Mat depth, DepthSpacePoint	* m_pColorCoordinates)
{/* �ȶԽ���󲢣��õ���С����
 Ȼ����� ��������ݶ�С�ġ� �õ��������
 ����С�����ͣ�������ĵ�//// ������ԭ��
 ����ʹ�û��������ʴ������

 �õ�Ŀ������󣬼�����color������λ�úʹ�С��
 =��ͶӰ����ȣ������ģ��ߴ簴�� colorsize��depthsize�ȵõ���
 */

	/* ��colorӳ�䵽depth��ÿ��color���꣬��Ӧ��ȵ����ꡣ
	for color���꣺
	colorǰ��==0 �ģ�
	�Ҷ�Ӧ������꣬�õ�ǰ��ֵ ��==0 ��д�벢����
	��һ��==0 ��д�� �����
	������У���ȱ仯С����Χ��Ҳ��ֵ=0

	������У�ÿ����255ǰ���ĵ�==�����4�ķ�Χ and���� û��һ��=255 �����ж�Ϊ����㡣
	*/

	if (color.cols <= 0 || color.rows <= 0)return;
	if (depth.cols <= 0 || depth.rows <= 0)return;
	resize(color, color, color.size() * 3);

	Mat orMatrix(color.rows, color.cols, CV_8U);
	Mat andMatrix(color.rows, color.cols, CV_8U);// = color.clone();

	int depthNchannels = depth.channels();//
	int colorNchannels = color.channels();
	float infinityFloat = -std::numeric_limits<float>::infinity();

	//0�Ǻ�ɫ�� 255�ǰ�ɫ
	int tempsig = -1;//0 ������,1 ����� , 2 and����
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
			//����ԭ�еı�����С��, ���ٷ���ӳ���ǰ�ԭ�ߴ�ӳ���.

			DepthSpacePoint p = m_pColorCoordinates[i*color.cols + j];
			if (p.X != infinityFloat && p.Y != infinityFloat)
			{//˵����ӳ�䵽
				int colorInDepthX = static_cast<int>(p.X + 0.5f);
				int colorInDepthY = static_cast<int>(p.Y + 0.5f);

				if ((colorInDepthX >= 0 && colorInDepthY < cDepthWidth) &&
					(colorInDepthY >= 0 && colorInDepthY < cDepthHeight))//ӳ��Ľ����depthͼ��
				{
					//	cout << " " << i << "  "<<j;
					int depthvalue = depth.at<uchar>(colorInDepthY, colorInDepthX);//���������Ǽ���ͨ��
					//ע���� at���� ��matlab���͵ģ����к��У��ȵ�һά��
					//vec3b �Ƿ���һ�����飬ֱ�ӵ������þ�����
					if (depthvalue == 255)// 0��ǰ�� ��ɫ, 255�Ǳ��� ��ɫ
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
	//��Ϊֱ�Ӵ���ɣ����or�ĺ͡���and��3������˵���ǹ������ˡ�ʹ��and���������ʹ��or���
	Mat resutl = BaseImageTools::ProduceAndOrMatrix(andMatrix, orMatrix);//Ч�����ã�����ֱ����or��ͼ, ���ܿ������Ч��
	//���ͼ�ǻ����� Ȼ���û�����䡣

	//	imshow("result", result);
	//Mat tem;
	//tempColor.resize(tempColor.cols / 2, );
	//cv::resize(tempColor, tem, cvSize(tempColor.cols / 2, tempColor.rows / 2));
	//	imshow("����", tem);

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
			{//˵����ӳ�䵽
				int colorInDepthX = static_cast<int>(p.X + 0.5f);
				int colorInDepthY = static_cast<int>(p.Y + 0.5f);
				if ((colorInDepthX >= 0 && colorInDepthY < cDepthWidth) &&
					(colorInDepthY >= 0 && colorInDepthY < cDepthHeight))//ӳ��Ľ����depthͼ��
				{
					//	cout << " " << i << "  "<<j;
					int depthvalue = depth.at<Vec3b>(colorInDepthY, colorInDepthX)[0];
					//ע���� at���� ��matlab���͵ģ����к��У��ȵ�һά��
					//vec3b �Ƿ���һ�����飬ֱ�ӵ������þ�����
					if (depthvalue < 100)//��ȵ����귶Χ��ͬ
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
	//	imshow("����", tem);

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
		colorDepthFramesynchronization = true;//����ͬ������ȡ��color�µ�֡����ôdepthҲҪ������ʹû��ҲҪ����һ֡����
		colorFrames++;
		cout << "colorFrames��" << colorFrames << "֡" << endl;

		//if (isFirstFrame)
		HRESULT hr = m_pCoordinateMapper->MapColorFrameToDepthSpace
			(cDepthWidth * cDepthHeight, (UINT16*)m_pDepthBuffer, cColorWidth * cColorHeight, m_pColorCoordinates);

		// Draw the data with OpenCV
		Mat ColorImage(nHeight, nWidth, CV_8UC4, pBuffer);
		//����8λ�޷�����*4��ͨ��  �ľ���
		m_pColorMat_Last = ColorImage.clone();//��������color���
		//	
		cv::resize(m_pColorMat_Last, m_pColorMat_Last_Resize, cvSize(cColorWidth_RESIZE, cColorHeight_RESIZE));
		//cv::moveWindow("color", 500, 0);
		//imshow("color", m_pColorMat_Last_Resize);
		//cvWaitKey(1);
		//cout << "smallcolor :"<<smallColor.channels();

		//colorProcess = new MixtureOfGaussianV1BGS;Ҫת��rgb��
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


		/*���� �ָ�
		//ת�Ҷ�
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
		//���ﶼû���ͷţ�============================================
		//����Ҫ���� Ҫ����ͷ�
		IplImage  pSrcImage(showImage);


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
		}
		*/
	}

#ifdef DEBUG 
	mt.End();
	cout << "color��������ʱ           ������������" << mt.costTime << endl;
#endif

}

