/*
This file contain simple implementation of BlobTrackerAuto virtual interface
This module just connected other low level 3 modules
(foreground estimator + BlobDetector + BlobTracker)
and some simple code to detect "lost tracking"
The track is lost when integral of foreground mask image by blob area has low value
*/
#include "precomp.hpp"
#include <time.h>
#include <highgui.h>

/* list of Blob Detection modules */
CvBlobDetector* cvCreateBlobDetectorSimple();

/* Get frequency for each module time working estimation: */
static double FREQ = 1000 * cvGetTickFrequency();

#if 1
#define COUNTNUM 100
#define TIME_BEGIN() \
{\
	static double   _TimeSum = 0; \
	static int      _Count = 0; \
	static int      _CountBlob = 0; \
	int64           _TickCount = cvGetTickCount(); \

#define TIME_END(_name_,_BlobNum_)    \
	_Count++; \
	_CountBlob += _BlobNum_; \
	_TimeSum += (cvGetTickCount() - _TickCount) / FREQ; \
if (m_TimesFile)if (_Count%COUNTNUM == 0)\
{ \
	FILE* out = fopen(m_TimesFile, "at"); \
if (out)\
{\
	fprintf(out, "ForFrame Frame: %d %s %f on %f blobs\n", _Count, _name_, _TimeSum / COUNTNUM, ((float)_CountBlob) / COUNTNUM); \
if (_CountBlob > 0)fprintf(out, "ForBlob  Frame: %d %s - %f\n", _Count, _name_, _TimeSum / _CountBlob); \
	fclose(out); \
}\
	_TimeSum = 0; \
	_CountBlob = 0; \
}\
}
#else
#define TIME_BEGIN()
#define TIME_END(_name_)
#endif


/* Auto Blob tracker creater (sole interface function for this file) */
CvBlobTrackerAuto* cvCreateBlobTrackerAuto1()
{
	return (CvBlobTrackerAuto*)new CvBlobTrackerAuto1();
}
/* Auto Blob tracker creater (sole interface function for this file) */
CvBlobTrackerAuto* cvCreateBlobTrackerAuto1(CvBlobTrackerAutoParam1* param)
{
	return (CvBlobTrackerAuto*)new CvBlobTrackerAuto1(param);
}

/* Constructor of auto blob tracker: */
CvBlobTrackerAuto1::CvBlobTrackerAuto1() :m_BlobList(sizeof(CvBlobTrackAuto))
{//��������ֵ������ģ��
	m_BlobList.AddFormat("i");
	// m_TimesFile = NULL;
	m_TimesFile = "test.txt";
	AddParam("TimesFile", &m_TimesFile);

	m_NextBlobID = 0;
	m_pFGMask = NULL;
	m_FrameCount = 0;

	m_FGTrainFrames =  0;
//	m_pFG = cvCreateFGDetectorBase(CV_BG_MODEL_FGD_SIMPLE, NULL);// cvCreateFGDetector0Simple;

	m_BDDel = 0;
//	m_pBD = cvCreateBlobDetectorSimple();
	m_pBD = cvCreateBlobDetectorSimpleOfMy();
	m_BTDel = 0;
	m_pBT = cvCreateBlobTrackerListColorTracker();//����color
	//cvCreateBlobTrackerMSPF();
//	m_pBT = cvCreateBlobTrackerMS();
	// m_pBT = cvCreateBlobTrackerCCMSPF();	
//	m_pBT=cvCreateBlobTrackerCC();
	m_pBT->m_Wnd = 1;

	m_BTReal = m_pBT ? m_pBT->IsModuleName("BlobTrackerReal") : 0;

	m_pBTGen = NULL;// cvCreateModuleBlobTrackGen1();

	m_pBTA = cvCreateModuleBlobTrackAnalysisIOR();// NULL;//

	m_pBTPostProc = cvCreateModuleBlobTrackPostProcKalman();// NULL;
	m_UsePPData = NULL;

	/* Create default submodules: */
	if (m_pBD == NULL)
	{
		m_pBD = cvCreateBlobDetectorSimple();
		m_BDDel = 1;
	}

	if (m_pBT == NULL)
	{
		m_pBT = cvCreateBlobTrackerMS();
		m_BTDel = 1;
	}
	//pBTAvi=
	SetModuleName("Auto1");

} /* CvBlobTrackerAuto1::CvBlobTrackerAuto1 */

/* Destructor for auto blob tracker: */
CvBlobTrackerAuto1::~CvBlobTrackerAuto1()
{
	if (m_BDDel)m_pBD->Release();
	if (m_BTDel)m_pBT->Release();
}

/*
void CvBlobTrackerAuto1::showAndSaveResult(Mat pImg)
{
	IplImage iplimage = pImg;
	showAndSaveResult(&iplimage);
}*/
void CvBlobTrackerAuto1::showAndSaveResult(IplImage* pImg)
{
	char* btavi_name = "trackingresult.avi";
	int                     FrameNum = 0;
	
	//��ԭͼ�ϣ��ҵ���blob����д��id
	/* Draw debug info: */
	if (pImg)//ԭʼ��ÿ֡ͼ��
	{   /* Draw all information about test sequence: */
		char        str[1024];
		int         line_type = CV_AA;   // Change it to 8 to see non-antialiased graphics.
		CvFont      font;
		int         i;	
		IplImage*   pI;

		if (pImg->nChannels == 4)
		{//rgba =��ת rgb
			pI = cvCreateImage(cvGetSize(pImg), 8, 3);
			cvCvtColor(pImg, pI, CV_BGRA2BGR);
		}
		else
		{		
			 pI = cvCloneImage(pImg);
		}

		cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 0.7, 0.7, 0, 1, line_type);

		for (i = this->GetBlobNum(); i > 0; i--)
		{
			CvSize  TextSize;
			CvBlob* pB = this->GetBlob(i - 1);
			CvPoint p = cvPoint(cvRound(pB->x * 256), cvRound(pB->y * 256));
			CvSize  s = cvSize(MAX(1, cvRound(CV_BLOB_RX(pB) * 256)), MAX(1, cvRound(CV_BLOB_RY(pB) * 256)));
			int c = cvRound(255 *  this->GetState(CV_BLOB_ID(pB)));

			//���ſ鵽ԭʼͼ����
			cvEllipse(pI,
				p,
				s,
				0, 0, 360,
				CV_RGB(c, 255 - c, 0), cvRound(1 + (3 * 0) / 255), CV_AA, 8);


			//�������Ĵ����˼�������ҵ���blob����д��id
			p.x >>= 8;
			p.y >>= 8;
			s.width >>= 8;
	 		s.height >>= 8;
			sprintf(str, "%03d", CV_BLOB_ID(pB));
			cvGetTextSize(str, &font, &TextSize, NULL);
			p.y -= s.height;
			cvPutText(pI, str, p, &font, CV_RGB(0, 255, 255));
			{
				const char* pS = this->GetStateDesc(CV_BLOB_ID(pB));

				if (pS)
				{
					char* pStr = _strdup(pS);
					char* pStrFree = pStr;

					while (pStr && strlen(pStr) > 0)
					{
						char* str_next = strchr(pStr, '\n');

						if (str_next)
						{
							str_next[0] = 0;
							str_next++;
						}

						p.y += TextSize.height + 1;
						cvPutText(pI, pStr, p, &font, CV_RGB(0, 255, 255));
						pStr = str_next;
					}
					free(pStrFree);
				}
			}

		}   /* Next blob. */;

		cvNamedWindow("Tracking");
		cvShowImage("Tracking", pI);
		cvWaitKey(1);

		if (btavi_name && pI)//�����һ֡�����ң������ͼ������������Ǵ������Ĳ�����Ϊ������  btavi_name=��1.avi"   ���ܴ������ˡ�
		{   /* Save to avi file: */
			CvSize      S = cvSize(pI->width, pI->height);
			if (pBTAvi == NULL)
			{
				pBTAvi = cvCreateVideoWriter(
					btavi_name,
					CV_FOURCC('x', 'v', 'i', 'd'),
					25,
					S);
			}
			cvWriteFrame(pBTAvi, pI);
		}

		cvReleaseImage(&pI);
	}   /* Draw all information about test sequence. */


}
void CvBlobTrackerAuto1::ProcessNewBlob(IplImage* pImg, IplImage* pFG)
{
	if (m_TimesFile)
	{
		static int64  TickCount = cvGetTickCount();
		static double TimeSum = 0;
		static int Count = 0;
		Count++;

		if (Count % 50 == 0)
		{
#ifndef WINCE
			time_t ltime;
			time(&ltime);
			char* stime = ctime(&ltime);
#else
			/* WINCE does not have above POSIX functions (time,ctime) */
			const char* stime = " wince ";
#endif
			FILE* out = fopen(m_TimesFile, "at");
			double Time;
			TickCount = cvGetTickCount() - TickCount;
			Time = TickCount / FREQ;
			TimeSum += Time;
			if (out){ fprintf(out, "- %sFrame: %d ALL_TIME - %f\n", stime, Count, TimeSum / 1000); fclose(out); }

			TimeSum = 0;
			TickCount = cvGetTickCount();
		}
	}
	/* Detect new blob: */
	//����µ��ſ顣
	//��һ�ο�����ȫ����
	TIME_BEGIN()
	if (!m_BTReal && m_pBD && pFG && (m_FrameCount > m_FGTrainFrames))
	{   /* Detect new blob: */
		static CvBlobSeq    NewBlobList;
		CvBlobTrackAuto     NewB;

		NewBlobList.Clear();

		//m_BlobList ����һ֡ �������ٺ����Ľ����
		//NewBlobList��Ҫ��ģ�����һ֡�²�����û�и��ٵ����� 
		if (m_pBD->DetectNewBlob(pImg, pFG, &NewBlobList, &m_BlobList))
		{ //ͨ�� enteringblobdetection��cpp�����
			//ԭ����� �ҵ���ǰ��ͼ��ͨ��findcontours�ҵ�ÿ������
			//Ȼ��ʹ�þ�ֵƯ�Ƶķ������ҵ����ġ�
			//	2 ɾ��С�飬�ڲ���
			//������������

			/* Add new blob to tracker and blob list: */
			int i;
			IplImage* pmask = pFG;


			//����⵽���ſ� ������ΪNewBlobList������ӵ�m_pBT���С�
			for (i = 0; i < NewBlobList.GetBlobNum(); ++i)
			{
				CvBlob* pBN = NewBlobList.GetBlob(i);

				if (pBN && pBN->w >= CV_BLOB_MINW && pBN->h >= CV_BLOB_MINH)
				{
					pBN->ID = m_NextBlobID;//���id ÿ�ζ���++

					CvBlob* pB = m_pBT->AddBlob(pBN, pImg, pmask);//������ģ�����Ŀ��
					//���ǵ��ø���ģ����
					if (pB)
					{//���ǽ���⵽���ſ� ��ӵ����ٵ��ſ��У�
						NewB.blob = pB[0];
						NewB.BadFrames = 0;
						m_BlobList.AddBlob((CvBlob*)&NewB);//m_BlobList�Ǻ��ĵĸ�������
						//NewB �и�badFrame�� ����cvBlobֻ��һ��blob�飬ǿ��ת��ȥ��
						m_NextBlobID++;//���id ÿ�ζ���++ ��ֻ����ӵ��˸��������ż�һ��
					}
				}
			}   /* Add next blob from list of detected blob. */

			if (pmask != pFG) cvReleaseImage(&pmask);

		}   /* Create and add new blobs and trackers. */

	}   /*  Detect new blob. */

	TIME_END("BlobDetector", -1)
}
void CvBlobTrackerAuto1::ProcessTracker(IplImage* pImg, IplImage* pMask)
{// ��Ҫ�������� ��blobtrack��sample��cpp ���ù���
	int         CurBlobNum = 0;
	IplImage*   pFG = pMask;

	/* Bump frame counter: */
	m_FrameCount++;

	if (m_TimesFile)
	{
		static int64  TickCount = cvGetTickCount();
		static double TimeSum = 0;
		static int Count = 0;
		Count++;

		if (Count % 50 == 0)
		{
#ifndef WINCE
			time_t ltime;
			time(&ltime);
			char* stime = ctime(&ltime);
#else
			/* WINCE does not have above POSIX functions (time,ctime) */
			const char* stime = " wince ";
#endif
			FILE* out = fopen(m_TimesFile, "at");
			double Time;
			TickCount = cvGetTickCount() - TickCount;
			Time = TickCount / FREQ;
			TimeSum += Time;
			if (out){ fprintf(out, "- %sFrame: %d ALL_TIME - %f\n", stime, Count, TimeSum / 1000); fclose(out); }

			TimeSum = 0;
			TickCount = cvGetTickCount();
		}
	}

//	/* Update BG model: */
//	TIME_BEGIN()
//
//		//����ģ�飬 ================��������acmmm2003�ļ�
//		//�������ݱ��浽��pFG =�����pMask��
//	if (m_pFG)
//	{   /* If FG detector is needed: */
//		m_pFG->Process(pImg);
//		pFG = m_pFG->GetMask();
//	}   /* If FG detector is needed. */
//	TIME_END("FGDetector", -1)

		m_pFGMask = pFG; /* For external use. */
	//���ڷ��� ��������


/*	if (m_pFG)
		//if(m_pFG && m_pFG->GetParam("DebugWnd") == 1)
	{// debug foreground result
		IplImage *pFG = m_pFG->GetMask();
		if (pFG)
		{
			cvNamedWindow("FG", 0);
			cvShowImage("FG", pFG);
		}
	}*/

	/* Track blobs: */
	TIME_BEGIN()
	if (m_pBT)
	{//�ſ���٣��ص�
		m_pBT->Process(pImg, pFG);
		//@@@@@@@@@@@@�ص㿴���

		//��һ��ûʲôЧ������
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			int     BlobID = CV_BLOB_ID(pB);
			int     idx = m_pBT->GetBlobIndexByID(BlobID);
			m_pBT->ProcessBlob(idx, pB, pImg, pFG);
			//�ӽ����������ȡ���˸������������Ӧindex��Ԫ�أ���id��ֵ0������ 
			pB->ID = BlobID;//��������ָ�ֵ��Դid��

			//��һ��ʵ���Ǽ̳еģ���ת��blobtrackinglist��
			//Ȼ��ͨ�����þ���Ĵ����ࣺ�����˲�����������ÿһ���ſ�
		}
		CurBlobNum = m_pBT->GetBlobNum();
	}
	TIME_END("BlobTracker", CurBlobNum)

	/* This part should be removed: */
	//��һ�ξ��Ǳ��� �������е�blob����� ȫ��m_BlobList ͬ����
	if (m_BTReal && m_pBT)
	{   /* Update blob list (detect new blob for real blob tracker): */
		for (int i = m_pBT->GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_pBT->GetBlob(i - 1);//���������ſ���
			if (pB && m_BlobList.GetBlobByID(CV_BLOB_ID(pB)) == NULL)
			{//��� ���������У���ģ��m_BlobList��ȴû�����blob��
				//˵�������ˣ� Ҫ��ӵ�m_BlobList
				CvBlobTrackAuto     NewB;
				NewB.blob = pB[0];
				NewB.BadFrames = 0;
				m_BlobList.AddBlob((CvBlob*)&NewB);
			}
		}   /* Next blob. */

		/* Delete blobs: */
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			if (pB && m_pBT->GetBlobByID(CV_BLOB_ID(pB)) == NULL)
			{//����ģ��m_BlobList�еģ� ���ſ������û�У���Ҫɾ����
				m_BlobList.DelBlob(i - 1);
			}
		}   /* Next blob. */
	}   /* Update bloblist. */


	//��һ�ο����ã� ��������ģ��Ҫʵ�� m_pBT->SetBlobByID(BlobID, pBN);
	//color ͨ����depthͨ��==������list===����Ӧ����m_pBTPostProc  m_pBTPostProc_Depth
	TIME_BEGIN()
	//Ԥ����ģ�飬���ǹ��ˣ�kalman�ķ���Ԥ�⡣
	if (m_pBTPostProc)
	{   /* Post-processing module: */
		//��������������ӵ� m_pBTPostProc �
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			m_pBTPostProc->AddBlob(pB);
		}
		m_pBTPostProc->Process();//���������ÿ���ſ鴦��Ԥ��

		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			int     BlobID = CV_BLOB_ID(pB);
			CvBlob* pBN = m_pBTPostProc->GetBlobByID(BlobID);
			//��Ȼ��ͬһ��id�������ǲ�ͬ�������Ԫ��

			if (pBN && m_UsePPData && pBN->w >= CV_BLOB_MINW && pBN->h >= CV_BLOB_MINH)
			{   /* Set new data for tracker: */
				m_pBT->SetBlobByID(BlobID, pBN);//��Ԥ��Ľ�� �����ſ� Ŀ����ٵ��ſ�
			}

			if (pBN)
			{   /* Update blob list with results from postprocessing: */
				pB[0] = pBN[0];//���� m_BlobList������ſ�
			}
		}
	}   /* Post-processing module. */

	TIME_END("PostProcessing", CurBlobNum)

	/* Blob deleter (experimental and simple): */
	TIME_BEGIN()
	//���Ƕ��ſ������� ǰ������Ƚϡ� //����˴����ſ�
	//��������˶�������Ч�ˣ� ���Լ�һ��ǰ��֡���ٽ�������Ǹ������岻���ˣ���˵�������ˡ�������󣡣���
	//@@@@@@@@@@@@@
	if (pFG)
	{   /* Blob deleter: */
		int i;
		if (!m_BTReal)
		for (i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Check all blobs on list: */
			CvBlobTrackAuto* pB = (CvBlobTrackAuto*)(m_BlobList.GetBlob(i - 1));
			int     Good = 0;
			int     w = pFG->width;
			int     h = pFG->height;
			CvRect  r = CV_BLOB_RECT(pB);
			CvMat   mat;
			double  aver = 0;
			double  area = CV_BLOB_WX(pB)*CV_BLOB_WY(pB);
			if (r.x < 0){ r.width += r.x; r.x = 0; }
			if (r.y < 0){ r.height += r.y; r.y = 0; }
			if (r.x + r.width >= w){ r.width = w - r.x - 1; }
			if (r.y + r.height >= h){ r.height = h - r.y - 1; }

			if (r.width > 4 && r.height > 4 && r.x < w && r.y < h &&
				r.x >= 0 && r.y >= 0 &&
				r.x + r.width < w && r.y + r.height < h && area > 0)
			{//���������� ��ǰ���еĵ� /�ܵ��� ����ֵ �����Ǻõĸ����ſ���
				aver = cvSum(cvGetSubRect(pFG, &mat, r)).val[0] / area;
				/* if mask in blob area exists then its blob OK*/
				if (aver > 0.05 * 255)Good = 1;
			}
			else
			{
				pB->BadFrames += 2;//���� ˵��������١�
			}

			if (Good)
			{
				pB->BadFrames = 0;
			}
			else
			{
				pB->BadFrames++;
			}
		}   /* Next blob: */

		/* Check error count: */
		for (i = 0; i<m_BlobList.GetBlobNum(); ++i)
		{
			CvBlobTrackAuto* pB = (CvBlobTrackAuto*)m_BlobList.GetBlob(i);

			if (pB->BadFrames> 7)//����˴����ſ�
			{   /* Delete such objects */
				/* from tracker...     */
				m_pBT->DelBlobByID(CV_BLOB_ID(pB));

				/* ... and from local list: */
				m_BlobList.DelBlob(i);
				i--;
			}
		}   /* Check error count for next blob. */
	}   /*  Blob deleter. */

	TIME_END("BlobDeleter", m_BlobList.GetBlobNum())

	/* Update blobs: */
	//����ĸ����� Ŀ��ģ�͵ĸ��� �����������˲��� ����Ҷ������ƻ���ģ�����ˣ�Ҫ��ϵ�����process�
	TIME_BEGIN()
	if (m_pBT)
		m_pBT->Update(pImg, pFG);//����ĸ����� Ŀ��ģ�͵ĸ���
	TIME_END("BlobTrackerUpdate", CurBlobNum)
} /* CvBlobTrackerAuto1::Process */





void CvBlobTrackerAuto1::Process(IplImage* pImg, IplImage* pMask)
{// ��Ҫ�������� ��blobtrack��sample��cpp ���ù���
	int         CurBlobNum = 0;
	IplImage*   pFG = pMask;

	/* Bump frame counter: */
	m_FrameCount++;

	if (m_TimesFile)
	{
		static int64  TickCount = cvGetTickCount();
		static double TimeSum = 0;
		static int Count = 0;
		Count++;

		if (Count % 50 == 0)
		{
#ifndef WINCE
			time_t ltime;
			time(&ltime);
			char* stime = ctime(&ltime);
#else
			/* WINCE does not have above POSIX functions (time,ctime) */
			const char* stime = " wince ";
#endif
			FILE* out = fopen(m_TimesFile, "at");
			double Time;
			TickCount = cvGetTickCount() - TickCount;
			Time = TickCount / FREQ;
			TimeSum += Time;
			if (out){ fprintf(out, "- %sFrame: %d ALL_TIME - %f\n", stime, Count, TimeSum / 1000); fclose(out); }

			TimeSum = 0;
			TickCount = cvGetTickCount();
		}
	}

	/* Update BG model: */
	TIME_BEGIN()

	//����ģ�飬 ================��������acmmm2003�ļ�
	//�������ݱ��浽��pFG =�����pMask��
	if (m_pFG)
	{   /* If FG detector is needed: */
		m_pFG->Process(pImg);
		pFG = m_pFG->GetMask();
	}   /* If FG detector is needed. */

	TIME_END("FGDetector", -1)

	m_pFGMask = pFG; /* For external use. */
	//���ڷ��� ��������


	if (m_pFG)
	//if(m_pFG && m_pFG->GetParam("DebugWnd") == 1)
	{// debug foreground result
		IplImage *pFG = m_pFG->GetMask();
		if (pFG)
		{
			cvNamedWindow("FG", 0);
			cvShowImage("FG", pFG);
		}
	}

	/* Track blobs: */
	TIME_BEGIN()
	if (m_pBT)
	{//�ſ���٣��ص�
		m_pBT->Process(pImg, pFG);
		//@@@@@@@@@@@@�ص㿴���

		//��һ��ûʲôЧ������
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			int     BlobID = CV_BLOB_ID(pB);
			int     idx = m_pBT->GetBlobIndexByID(BlobID);
			m_pBT->ProcessBlob(idx, pB, pImg, pFG);
			//�ӽ����������ȡ���˸������������Ӧindex��Ԫ�أ���id��ֵ0������ 
			pB->ID = BlobID;//��������ָ�ֵ��Դid��

			//��һ��ʵ���Ǽ̳еģ���ת��blobtrackinglist��
			//Ȼ��ͨ�����þ���Ĵ����ࣺ�����˲�����������ÿһ���ſ�
		}
		CurBlobNum = m_pBT->GetBlobNum();
	}
	TIME_END("BlobTracker", CurBlobNum)

	/* This part should be removed: */
	//��һ�ξ��Ǳ��� �������е�blob����� ȫ��m_BlobList ͬ����
	if (m_BTReal && m_pBT)
	{   /* Update blob list (detect new blob for real blob tracker): */
		for (int i = m_pBT->GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_pBT->GetBlob(i - 1);//���������ſ���
			if (pB && m_BlobList.GetBlobByID(CV_BLOB_ID(pB)) == NULL)
			{//��� ���������У���ģ��m_BlobList��ȴû�����blob��
				//˵�������ˣ� Ҫ��ӵ�m_BlobList
				CvBlobTrackAuto     NewB;
				NewB.blob = pB[0];
				NewB.BadFrames = 0;
				m_BlobList.AddBlob((CvBlob*)&NewB);
			}
		}   /* Next blob. */

		/* Delete blobs: */
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			if (pB && m_pBT->GetBlobByID(CV_BLOB_ID(pB)) == NULL)
			{//����ģ��m_BlobList�еģ� ���ſ������û�У���Ҫɾ����
				m_BlobList.DelBlob(i - 1);
			}
		}   /* Next blob. */
	}   /* Update bloblist. */



	//��һ�ο����ã� ��������ģ��Ҫʵ�� m_pBT->SetBlobByID(BlobID, pBN);
	//color ͨ����depthͨ��==������list===����Ӧ����m_pBTPostProc  m_pBTPostProc_Depth
	TIME_BEGIN()
	//Ԥ����ģ�飬���ǹ��ˣ�kalman�ķ���Ԥ�⡣
	if (m_pBTPostProc)
	{   /* Post-processing module: */
		//��������������ӵ� m_pBTPostProc �
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			m_pBTPostProc->AddBlob(pB);
		}
		m_pBTPostProc->Process();//���������ÿ���ſ鴦��Ԥ��

		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			int     BlobID = CV_BLOB_ID(pB);
			CvBlob* pBN = m_pBTPostProc->GetBlobByID(BlobID);
			//��Ȼ��ͬһ��id�������ǲ�ͬ�������Ԫ��

			if (pBN && m_UsePPData && pBN->w >= CV_BLOB_MINW && pBN->h >= CV_BLOB_MINH)
			{   /* Set new data for tracker: */
				m_pBT->SetBlobByID(BlobID, pBN);//��Ԥ��Ľ�� �����ſ� Ŀ����ٵ��ſ�
			}

			if (pBN)
			{   /* Update blob list with results from postprocessing: */
				pB[0] = pBN[0];//���� m_BlobList������ſ�
			}
		}
	}   /* Post-processing module. */

	TIME_END("PostProcessing", CurBlobNum)

	/* Blob deleter (experimental and simple): */
	TIME_BEGIN()
	//���Ƕ��ſ������� ǰ������Ƚϡ� //����˴����ſ�
	//��������˶�������Ч�ˣ� ���Լ�һ��ǰ��֡���ٽ�������Ǹ������岻���ˣ���˵�������ˡ�������󣡣���
	//@@@@@@@@@@@@@
	if (pFG)
	{   /* Blob deleter: */
		int i;
		if (!m_BTReal)
		for (i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Check all blobs on list: */
			CvBlobTrackAuto* pB = (CvBlobTrackAuto*)(m_BlobList.GetBlob(i - 1));
			int     Good = 0;
			int     w = pFG->width;
			int     h = pFG->height;
			CvRect  r = CV_BLOB_RECT(pB);
			CvMat   mat;
			double  aver = 0;
			double  area = CV_BLOB_WX(pB)*CV_BLOB_WY(pB);
			if (r.x < 0){ r.width += r.x; r.x = 0; }
			if (r.y < 0){ r.height += r.y; r.y = 0; }
			if (r.x + r.width >= w){ r.width = w - r.x - 1; }
			if (r.y + r.height >= h){ r.height = h - r.y - 1; }

			if (r.width > 4 && r.height > 4 && r.x < w && r.y < h &&
				r.x >= 0 && r.y >= 0 &&
				r.x + r.width < w && r.y + r.height < h && area > 0)
			{//���������� ��ǰ���еĵ� /�ܵ��� ����ֵ �����Ǻõĸ����ſ���
				aver = cvSum(cvGetSubRect(pFG, &mat, r)).val[0] / area;
				/* if mask in blob area exists then its blob OK*/
				if (aver > 0.1 * 255)Good = 1;
			}
			else
			{
				pB->BadFrames += 2;//���� ˵��������١�
			}

			if (Good)
			{
				pB->BadFrames = 0;
			}
			else
			{
				pB->BadFrames++;
			}
		}   /* Next blob: */

		/* Check error count: */
		for (i = 0; i<m_BlobList.GetBlobNum(); ++i)
		{
			CvBlobTrackAuto* pB = (CvBlobTrackAuto*)m_BlobList.GetBlob(i);

			if (pB->BadFrames>3)//����˴����ſ�
			{   /* Delete such objects */
				/* from tracker...     */
				m_pBT->DelBlobByID(CV_BLOB_ID(pB));

				/* ... and from local list: */
				m_BlobList.DelBlob(i);
				i--;
			}
		}   /* Check error count for next blob. */
	}   /*  Blob deleter. */

	TIME_END("BlobDeleter", m_BlobList.GetBlobNum())

	/* Update blobs: */
	//����ĸ����� Ŀ��ģ�͵ĸ��� �����������˲��� ����Ҷ������ƻ���ģ�����ˣ�Ҫ��ϵ�����process�
	TIME_BEGIN()
	if (m_pBT)
		m_pBT->Update(pImg, pFG);//����ĸ����� Ŀ��ģ�͵ĸ���
	TIME_END("BlobTrackerUpdate", CurBlobNum)

	/* Detect new blob: */
	//����µ��ſ顣
	//��һ�ο�����ȫ����
	TIME_BEGIN()
	if (!m_BTReal && m_pBD && pFG && (m_FrameCount > m_FGTrainFrames))
	{   /* Detect new blob: */
		static CvBlobSeq    NewBlobList;
		CvBlobTrackAuto     NewB;

		NewBlobList.Clear();

		//m_BlobList ����һ֡ �������ٺ����Ľ����
		//NewBlobList��Ҫ��ģ�����һ֡�²�����û�и��ٵ����� 
		if (m_pBD->DetectNewBlob(pImg, pFG, &NewBlobList, &m_BlobList))
		{ //ͨ�� enteringblobdetection��cpp�����
			//ԭ����� �ҵ���ǰ��ͼ��ͨ��findcontours�ҵ�ÿ������
			//Ȼ��ʹ�þ�ֵƯ�Ƶķ������ҵ����ġ�
			//	2 ɾ��С�飬�ڲ���
			//������������

			/* Add new blob to tracker and blob list: */
			int i;
			IplImage* pmask = pFG;


			//����⵽���ſ� ������ΪNewBlobList������ӵ�m_pBT���С�
			for (i = 0; i < NewBlobList.GetBlobNum(); ++i)
			{
				CvBlob* pBN = NewBlobList.GetBlob(i);

				if (pBN && pBN->w >= CV_BLOB_MINW && pBN->h >= CV_BLOB_MINH)
				{
					pBN->ID = m_NextBlobID;//���id ÿ�ζ���++

					CvBlob* pB = m_pBT->AddBlob(pBN, pImg, pmask);//������ģ�����Ŀ��
					//���ǵ��ø���ģ����
					if (pB)
					{//���ǽ���⵽���ſ� ��ӵ����ٵ��ſ��У�
						NewB.blob = pB[0];
						NewB.BadFrames = 0;
						m_BlobList.AddBlob((CvBlob*)&NewB);//m_BlobList�Ǻ��ĵĸ�������
						//NewB �и�badFrame�� ����cvBlobֻ��һ��blob�飬ǿ��ת��ȥ��
						m_NextBlobID++;//���id ÿ�ζ���++ ��ֻ����ӵ��˸��������ż�һ��
					}
				}
			}   /* Add next blob from list of detected blob. */

			if (pmask != pFG) cvReleaseImage(&pmask);

		}   /* Create and add new blobs and trackers. */

	}   /*  Detect new blob. */

	TIME_END("BlobDetector", -1)

} /* CvBlobTrackerAuto1::Process */












void CvBlobTrackerAuto1::Process__(IplImage* pImg, IplImage* pMask)
{// ��Ҫ�������� ��blobtrack��sample��cpp ���ù���
	int         CurBlobNum = 0;
	IplImage*   pFG = pMask;

	/* Bump frame counter: */
	m_FrameCount++;

	if (m_TimesFile)
	{
		static int64  TickCount = cvGetTickCount();
		static double TimeSum = 0;
		static int Count = 0;
		Count++;

		if(Count%100==0)
		{
#ifndef WINCE
			time_t ltime;
			time(&ltime);
			char* stime = ctime(&ltime);
#else
			/* WINCE does not have above POSIX functions (time,ctime) */
			const char* stime = " wince ";
#endif
			FILE* out = fopen(m_TimesFile, "at");
			double Time;
			TickCount = cvGetTickCount() - TickCount;
			Time = TickCount / FREQ;
			TimeSum += Time;
			if (out){ fprintf(out, "- %sFrame: %d ALL_TIME - %f\n", stime, Count, TimeSum / 1000); fclose(out); }

			TimeSum = 0;
			TickCount = cvGetTickCount();
		}
	}

	/* Update BG model: */
	TIME_BEGIN()

		//����ģ�飬 ================��������acmmm2003�ļ�
		//�������ݱ��浽��pFG =�����pMask��
	if (m_pFG)
	{   /* If FG detector is needed: */
		m_pFG->Process(pImg);
		pFG = m_pFG->GetMask();
	}   /* If FG detector is needed. */

	TIME_END("FGDetector", -1)

		m_pFGMask = pFG; /* For external use. */

	/*if(m_pFG && m_pFG->GetParam("DebugWnd") == 1)
	{// debug foreground result
	IplImage *pFG = m_pFG->GetMask();
	if(pFG)
	{
	cvNamedWindow("FG",0);
	cvShowImage("FG", pFG);
	}
	}*/

	/* Track blobs: */
	TIME_BEGIN()
	if (m_pBT)
	{//�ſ���٣��ص�
		m_pBT->Process(pImg, pFG);

		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			int     BlobID = CV_BLOB_ID(pB);
			int     idx = m_pBT->GetBlobIndexByID(BlobID);
			m_pBT->ProcessBlob(idx, pB, pImg, pFG);
			pB->ID = BlobID;
		}
		CurBlobNum = m_pBT->GetBlobNum();
	}
	TIME_END("BlobTracker", CurBlobNum)

	/* This part should be removed: */
	if (m_BTReal && m_pBT)
	{   /* Update blob list (detect new blob for real blob tracker): */
		for (int i = m_pBT->GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_pBT->GetBlob(i - 1);
			if (pB && m_BlobList.GetBlobByID(CV_BLOB_ID(pB)) == NULL)
			{
				CvBlobTrackAuto     NewB;
				NewB.blob = pB[0];
				NewB.BadFrames = 0;
				m_BlobList.AddBlob((CvBlob*)&NewB);
			}
		}   /* Next blob. */

		/* Delete blobs: */
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			if (pB && m_pBT->GetBlobByID(CV_BLOB_ID(pB)) == NULL)
			{
				m_BlobList.DelBlob(i - 1);
			}
		}   /* Next blob. */
	}   /* Update bloblist. */


	TIME_BEGIN()
	//Ԥ����ģ�飬���ǹ��ˣ��������ķ���Ԥ�⡣
	if (m_pBTPostProc)
	{   /* Post-processing module: */
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			m_pBTPostProc->AddBlob(pB);
		}
		m_pBTPostProc->Process();

		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update tracked-blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			int     BlobID = CV_BLOB_ID(pB);
			CvBlob* pBN = m_pBTPostProc->GetBlobByID(BlobID);

			if (pBN && m_UsePPData && pBN->w >= CV_BLOB_MINW && pBN->h >= CV_BLOB_MINH)
			{   /* Set new data for tracker: */
				m_pBT->SetBlobByID(BlobID, pBN);
			}

			if (pBN)
			{   /* Update blob list with results from postprocessing: */
				pB[0] = pBN[0];
			}
		}
	}   /* Post-processing module. */

	TIME_END("PostProcessing", CurBlobNum)

	/* Blob deleter (experimental and simple): */
	TIME_BEGIN()
	//���Ƕ��ſ������� ǰ������Ƚϡ� //����˴����ſ�
	if (pFG)
	{   /* Blob deleter: */
		int i;
		if (!m_BTReal)for (i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Check all blobs on list: */
			CvBlobTrackAuto* pB = (CvBlobTrackAuto*)(m_BlobList.GetBlob(i - 1));
			int     Good = 0;
			int     w = pFG->width;
			int     h = pFG->height;
			CvRect  r = CV_BLOB_RECT(pB);
			CvMat   mat;
			double  aver = 0;
			double  area = CV_BLOB_WX(pB)*CV_BLOB_WY(pB);
			if (r.x < 0){ r.width += r.x; r.x = 0; }
			if (r.y < 0){ r.height += r.y; r.y = 0; }
			if (r.x + r.width >= w){ r.width = w - r.x - 1; }
			if (r.y + r.height >= h){ r.height = h - r.y - 1; }

			if (r.width > 4 && r.height > 4 && r.x < w && r.y < h &&
				r.x >= 0 && r.y >= 0 &&
				r.x + r.width < w && r.y + r.height < h && area > 0)
			{//���������� ��ǰ���еĵ� /�ܵ��� ����ֵ �����Ǻõĸ����ſ���
				aver = cvSum(cvGetSubRect(pFG, &mat, r)).val[0] / area;
				/* if mask in blob area exists then its blob OK*/
				if (aver > 0.1 * 255)Good = 1;
			}
			else
			{
				pB->BadFrames += 2;//���� ˵��������١�
			}

			if (Good)
			{
				pB->BadFrames = 0;
			}
			else
			{
				pB->BadFrames++;
			}
		}   /* Next blob: */

		/* Check error count: */
		for (i = 0; i<m_BlobList.GetBlobNum(); ++i)
		{
			CvBlobTrackAuto* pB = (CvBlobTrackAuto*)m_BlobList.GetBlob(i);

			if (pB->BadFrames>3)//����˴����ſ�
			{   /* Delete such objects */
				/* from tracker...     */
				m_pBT->DelBlobByID(CV_BLOB_ID(pB));

				/* ... and from local list: */
				m_BlobList.DelBlob(i);
				i--;
			}
		}   /* Check error count for next blob. */
	}   /*  Blob deleter. */

	TIME_END("BlobDeleter", m_BlobList.GetBlobNum())

	/* Update blobs: */
	TIME_BEGIN()
	if (m_pBT)
		m_pBT->Update(pImg, pFG);//����ĸ����� Ŀ��ģ�͵ĸ���
	TIME_END("BlobTrackerUpdate", CurBlobNum)

	/* Detect new blob: */
	//����µ��ſ顣
	TIME_BEGIN()
	if (!m_BTReal && m_pBD && pFG && (m_FrameCount > m_FGTrainFrames))
	{   /* Detect new blob: */
		static CvBlobSeq    NewBlobList;
		CvBlobTrackAuto     NewB;

		NewBlobList.Clear();

		if (m_pBD->DetectNewBlob(pImg, pFG, &NewBlobList, &m_BlobList))
		{ //ͨ�� enteringblobdetection��cpp�����
			//ԭ����� �ҵ���ǰ��ͼ��ͨ��findcontours�ҵ�ÿ������
			//Ȼ��ʹ�þ�ֵƯ�Ƶķ������ҵ����ġ�
			//	2 ɾ��С�飬�ڲ���
			//������������

			/* Add new blob to tracker and blob list: */
			int i;
			IplImage* pmask = pFG;


			//����⵽���ſ� ������ΪNewBlobList������ӵ�m_pBT���С�
			for (i = 0; i < NewBlobList.GetBlobNum(); ++i)
			{
				CvBlob* pBN = NewBlobList.GetBlob(i);

				if (pBN && pBN->w >= CV_BLOB_MINW && pBN->h >= CV_BLOB_MINH)
				{
					pBN->ID = m_NextBlobID;

					CvBlob* pB = m_pBT->AddBlob(pBN, pImg, pmask);
					//���ǵ��ø���ģ����
					if (pB)
					{//���ǽ���⵽���ſ� ��ӵ����ٵ��ſ��У�
						NewB.blob = pB[0];
						NewB.BadFrames = 0;
						m_BlobList.AddBlob((CvBlob*)&NewB);//m_BlobList�Ǻ��ĵĸ�������
						m_NextBlobID++;
					}
				}
			}   /* Add next blob from list of detected blob. */

			if (pmask != pFG) cvReleaseImage(&pmask);

		}   /* Create and add new blobs and trackers. */

	}   /*  Detect new blob. */

	TIME_END("BlobDetector", -1)

		TIME_BEGIN()
	if (m_pBTGen)
	{   /* Run track generator: */
		for (int i = m_BlobList.GetBlobNum(); i > 0; --i)
		{   /* Update data of tracked blob list: */
			CvBlob* pB = m_BlobList.GetBlob(i - 1);
			m_pBTGen->AddBlob(pB);
		}
		m_pBTGen->Process(pImg, pFG);
	}   /* Run track generator: */
	TIME_END("TrajectoryGeneration", -1)

	TIME_BEGIN()
	if (m_pBTA)
	{   /* Trajectory analysis module: */
		int i;
		for (i = m_BlobList.GetBlobNum(); i > 0; i--)
			m_pBTA->AddBlob(m_BlobList.GetBlob(i - 1));

		m_pBTA->Process(pImg, pFG);

	}   /* Trajectory analysis module. */

	TIME_END("TrackAnalysis", m_BlobList.GetBlobNum())

} /* CvBlobTrackerAuto1::Process */

/* Constructor of auto blob tracker: */
CvBlobTrackerAuto1::CvBlobTrackerAuto1(CvBlobTrackerAutoParam1* param) :m_BlobList(sizeof(CvBlobTrackAuto))
{//��������ֵ������ģ��
	m_BlobList.AddFormat("i");
	// m_TimesFile = NULL;
	m_TimesFile = "test.txt";
	AddParam("TimesFile", &m_TimesFile);

	m_NextBlobID = 0;
	m_pFGMask = NULL;
	m_FrameCount = 0;

	m_FGTrainFrames = param ? param->FGTrainFrames : 0;
	m_pFG = param ? param->pFG : 0;

	m_BDDel = 0;
	m_pBD = param ? param->pBD : NULL;
	m_BTDel = 0;
	m_pBT = param ? param->pBT : NULL;;
	m_BTReal = m_pBT ? m_pBT->IsModuleName("BlobTrackerReal") : 0;

	m_pBTGen = param ? param->pBTGen : NULL;

	m_pBTA = param ? param->pBTA : NULL;

	m_pBTPostProc = param ? param->pBTPP : NULL;
	m_UsePPData = param ? param->UsePPData : 0;

	/* Create default submodules: */
	if (m_pBD == NULL)
	{
		m_pBD = cvCreateBlobDetectorSimple();
		m_BDDel = 1;
	}

	if (m_pBT == NULL)
	{
		m_pBT = cvCreateBlobTrackerMS();
		m_BTDel = 1;
	}

	SetModuleName("Auto1");

} /* CvBlobTrackerAuto1::CvBlobTrackerAuto1 */