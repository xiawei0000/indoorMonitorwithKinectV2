
#include "blobtrack.hpp"
#include "legacy.hpp"
//#include "blobtrack_sample.cpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/imgproc/imgproc_c.h>
#include "opencv2/video/background_segm.hpp"
#include <stdio.h>

/* Select appropriate case insensitive string comparison function: */
#if defined WIN32 || defined _MSC_VER
# define MY_STRNICMP _strnicmp
# define MY_STRICMP _stricmp
# define MY_STRDUP _strdup
#else
# define MY_STRNICMP strncasecmp
# define MY_STRICMP strcasecmp
# define MY_STRDUP strdup
#endif


//int RunBlobTrackingAuto2(CvCapture* pCap, CvBlobTrackerAuto* pTracker,
//	char* fgavi_name = NULL, char* btavi_name = NULL);


/* Run pipeline on all frames: */
//������ģ������ʹ�õĺ���
//����������޸��Լ���
 int RunBlobTrackingAuto2323(CvCapture* pCap, CvBlobTrackerAuto* pTracker, char* fgavi_name , char* btavi_name )
{
	int                     OneFrameProcess = 0;
	int                     key;
	int                     FrameNum = 0;
	CvVideoWriter*          pFGAvi = NULL;
	CvVideoWriter*          pBTAvi = NULL;

	/* Main loop: */
	/*OneFrameProcess =0 ʱ��Ϊwaitkey��0�� ���ȴ��ˣ�����-1��waitkey��1����ʾ��1ms����������˷��ذ�������ʱ����-1*/
	for (FrameNum = 0; pCap && (key = cvWaitKey(OneFrameProcess ? 0 : 1)) != 27;//����esc��������������� 
		FrameNum++)
	{   /* Main loop: */// �����������ѭ�������ѭ����ֹ����ζ��������������
		IplImage*   pImg = NULL;
		IplImage*   pMask = NULL;

		if (key != -1)
		{
			OneFrameProcess = 1;
			if (key == 'r')OneFrameProcess = 0;
		}

		pImg = cvQueryFrame(pCap);//��ȡ��Ƶ
		if (pImg == NULL) break;


		/* Process: */
		pTracker->Process(pImg, pMask);//����ͼ���������Ӧ��ִ���������еĴ�����̡�

		if (fgavi_name)//����������fgǰ��Ҫ������ļ���
		if (pTracker->GetFGMask())//ǰ����ͼ���mask���ڵĻ�������ǰ���������ſ� 
		{   /* Debug FG: */
			IplImage*           pFG = pTracker->GetFGMask();//�õ�ǰ����mask
			CvSize              S = cvSize(pFG->width, pFG->height);
			static IplImage*    pI = NULL;

			if (pI == NULL)pI = cvCreateImage(S, pFG->depth, 3);
			cvCvtColor(pFG, pI, CV_GRAY2BGR);

			if (fgavi_name)//����ǰ������Ƶ
			{   /* Save fg to avi file: */
				if (pFGAvi == NULL)
				{
					pFGAvi = cvCreateVideoWriter(
						fgavi_name,
						CV_FOURCC('x', 'v', 'i', 'd'),
						25,
						S);
				}
				cvWriteFrame(pFGAvi, pI);//д��һ��ͼ
			}

			//�����ſ����Բ
			if (pTracker->GetBlobNum() > 0) //pTracker�ҵ���blob
			{   /* Draw detected blobs: */
				int i;
				for (i = pTracker->GetBlobNum(); i > 0; i--)
				{
					CvBlob* pB = pTracker->GetBlob(i - 1);//�õ���i-1��blob
					CvPoint p = cvPointFrom32f(CV_BLOB_CENTER(pB));//�ſ�����
					//����꾹Ȼ�Ǹ�ǿ��ת�������ġ������С�
					//#define CV_BLOB_CENTER(pB) cvPoint2D32f(((CvBlob*)(pB))->x,((CvBlob*)(pB))->y)
					CvSize  s = cvSize(MAX(1, cvRound(CV_BLOB_RX(pB))), MAX(1, cvRound(CV_BLOB_RY(pB))));
					//ͨ���� ����ſ��w ��h ��size
					int c = cvRound(255 * pTracker->GetState(CV_BLOB_ID(pB)));
					cvEllipse(pI,//��ͼ�У����ſ黭Բ
						p,
						s,
						0, 0, 360,
						CV_RGB(c, 255 - c, 0), cvRound(1 + (3 * c) / 255));
				}   /* Next blob: */;
			}
			cvNamedWindow("FG", 0);
			cvShowImage("FG", pI);
		}   /* Debug FG. *///���Ҫ����������ǰ�����棬�����ſ�


		//��ԭͼ�ϣ��ҵ���blob����д��id
		/* Draw debug info: */
		if (pImg)//ԭʼ��ÿ֡ͼ��
		{   /* Draw all information about test sequence: */
			char        str[1024];
			int         line_type = CV_AA;   // Change it to 8 to see non-antialiased graphics.
			CvFont      font;
			int         i;
			IplImage*   pI = cvCloneImage(pImg);

			cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 0.7, 0.7, 0, 1, line_type);

			for (i = pTracker->GetBlobNum(); i > 0; i--)
			{
				CvSize  TextSize;
				CvBlob* pB = pTracker->GetBlob(i - 1);
				CvPoint p = cvPoint(cvRound(pB->x * 256), cvRound(pB->y * 256));
				CvSize  s = cvSize(MAX(1, cvRound(CV_BLOB_RX(pB) * 256)), MAX(1, cvRound(CV_BLOB_RY(pB) * 256)));
				int c = cvRound(255 * pTracker->GetState(CV_BLOB_ID(pB)));


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
					const char* pS = pTracker->GetStateDesc(CV_BLOB_ID(pB));

					if (pS)
					{
						char* pStr = MY_STRDUP(pS);
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

			cvNamedWindow("Tracking", 0);
			cvShowImage("Tracking", pI);

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
	}   /*  Main loop. */

	if (pFGAvi)cvReleaseVideoWriter(&pFGAvi);
	if (pBTAvi)cvReleaseVideoWriter(&pBTAvi);
	return 0;
}   /* RunBlobTrackingAuto */


int main223(int argc, char* argv[])
{   /* Main function: */
	CvCapture*                  pCap = NULL;
	CvBlobTrackerAutoParam1     param = { 0, 0, 0, 0, 0, 0, 0, 0 };
	CvBlobTrackerAuto*          pTracker = NULL;

	//float       scale = 1;
	const char* scale_name = NULL;
	char*       yml_name = NULL;
	char**      yml_video_names = NULL;
	int         yml_video_num = 0;
	char*       avi_name = NULL;
	const char* fg_name = NULL;
	char*       fgavi_name = NULL;
	char*       btavi_name = NULL;
	const char* bd_name = NULL;
	const char* bt_name = NULL;
	const char* btgen_name = NULL;
	const char* btpp_name = NULL;
	const char* bta_name = NULL;
	char*       bta_data_name = NULL;
	char*       track_name = NULL;
	//char*       comment_name = NULL;
	char*       FGTrainFrames = NULL;
	char*       log_name = NULL;
	char*       savestate_name = NULL;
	char*       loadstate_name = NULL;
	const char* bt_corr = NULL;




	avi_name = "RGB.avi";
	btavi_name = "";
	/* Create source video: */
	if (avi_name)
		pCap = cvCaptureFromFile(avi_name);//����Ƶ
	//pCap = cvCreateCameraCapture(-1);//������ͷ�ж�ȡ���ݡ�
	if (pCap == NULL)
	{
		printf("Can't open %s file\n", avi_name);
		return -1;
	}

	//��ʼ��pTracker

	/* Run pipeline: //��ʼ��ˮ�ߡ���ˮ���˳�����ζ�ų��������
	���������*/
	//RunBlobTrackingAuto(pCap, pTracker, fgavi_name, btavi_name);

	pTracker = cvCreateBlobTrackerAuto1();
	//pTracker->m_Wnd = 1;
	RunBlobTrackingAuto2323(pCap, pTracker, fgavi_name, btavi_name);


	if (pCap)	cvReleaseCapture(&pCap);
	return 0;
}   /* main() */