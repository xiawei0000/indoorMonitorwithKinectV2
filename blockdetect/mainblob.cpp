
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
//将所有模块连接使用的函数
//根据这个来修改自己的
 int RunBlobTrackingAuto2323(CvCapture* pCap, CvBlobTrackerAuto* pTracker, char* fgavi_name , char* btavi_name )
{
	int                     OneFrameProcess = 0;
	int                     key;
	int                     FrameNum = 0;
	CvVideoWriter*          pFGAvi = NULL;
	CvVideoWriter*          pBTAvi = NULL;

	/* Main loop: */
	/*OneFrameProcess =0 时，为waitkey（0） 不等待了，返回-1，waitkey（1）表示等1ms，如果按键了返回按键，超时返回-1*/
	for (FrameNum = 0; pCap && (key = cvWaitKey(OneFrameProcess ? 0 : 1)) != 27;//按下esc键整个程序结束。 
		FrameNum++)
	{   /* Main loop: */// 整个程序的主循环。这个循环终止，意味着这个程序结束。
		IplImage*   pImg = NULL;
		IplImage*   pMask = NULL;

		if (key != -1)
		{
			OneFrameProcess = 1;
			if (key == 'r')OneFrameProcess = 0;
		}

		pImg = cvQueryFrame(pCap);//读取视频
		if (pImg == NULL) break;


		/* Process: */
		pTracker->Process(pImg, pMask);//处理图像。这个函数应该执行完了所有的处理过程。

		if (fgavi_name)//参数设置了fg前景要保存的文件名
		if (pTracker->GetFGMask())//前景的图像的mask存在的话，保存前景。画出团块 
		{   /* Debug FG: */
			IplImage*           pFG = pTracker->GetFGMask();//得到前景的mask
			CvSize              S = cvSize(pFG->width, pFG->height);
			static IplImage*    pI = NULL;

			if (pI == NULL)pI = cvCreateImage(S, pFG->depth, 3);
			cvCvtColor(pFG, pI, CV_GRAY2BGR);

			if (fgavi_name)//保存前景到视频
			{   /* Save fg to avi file: */
				if (pFGAvi == NULL)
				{
					pFGAvi = cvCreateVideoWriter(
						fgavi_name,
						CV_FOURCC('x', 'v', 'i', 'd'),
						25,
						S);
				}
				cvWriteFrame(pFGAvi, pI);//写入一张图
			}

			//画出团块的椭圆
			if (pTracker->GetBlobNum() > 0) //pTracker找到了blob
			{   /* Draw detected blobs: */
				int i;
				for (i = pTracker->GetBlobNum(); i > 0; i--)
				{
					CvBlob* pB = pTracker->GetBlob(i - 1);//得到第i-1个blob
					CvPoint p = cvPointFrom32f(CV_BLOB_CENTER(pB));//团块中心
					//这个宏竟然是个强制转换得来的。见下行。
					//#define CV_BLOB_CENTER(pB) cvPoint2D32f(((CvBlob*)(pB))->x,((CvBlob*)(pB))->y)
					CvSize  s = cvSize(MAX(1, cvRound(CV_BLOB_RX(pB))), MAX(1, cvRound(CV_BLOB_RY(pB))));
					//通过宏 获得团块的w 和h 的size
					int c = cvRound(255 * pTracker->GetState(CV_BLOB_ID(pB)));
					cvEllipse(pI,//在图中，对团块画圆
						p,
						s,
						0, 0, 360,
						CV_RGB(c, 255 - c, 0), cvRound(1 + (3 * c) / 255));
				}   /* Next blob: */;
			}
			cvNamedWindow("FG", 0);
			cvShowImage("FG", pI);
		}   /* Debug FG. *///如果要保存结果，对前景保存，画出团块


		//在原图上：找到的blob附近写下id
		/* Draw debug info: */
		if (pImg)//原始的每帧图像。
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


				//画团块到原始图像上
				cvEllipse(pI,
					p,
					s,
					0, 0, 360,
					CV_RGB(c, 255 - c, 0), cvRound(1 + (3 * 0) / 255), CV_AA, 8);


				//下面代码的大概意思就是在找到的blob附近写下id
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

			if (btavi_name && pI)//如果这一帧存在且，你想把图像存起来，就是传过来的参数不为空例如  btavi_name=“1.avi"   就能存起来了。
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
		pCap = cvCaptureFromFile(avi_name);//打开视频
	//pCap = cvCreateCameraCapture(-1);//从摄像头中读取数据。
	if (pCap == NULL)
	{
		printf("Can't open %s file\n", avi_name);
		return -1;
	}

	//初始化pTracker

	/* Run pipeline: //开始流水线。流水线退出，意味着程序结束，
	这里出错了*/
	//RunBlobTrackingAuto(pCap, pTracker, fgavi_name, btavi_name);

	pTracker = cvCreateBlobTrackerAuto1();
	//pTracker->m_Wnd = 1;
	RunBlobTrackingAuto2323(pCap, pTracker, fgavi_name, btavi_name);


	if (pCap)	cvReleaseCapture(&pCap);
	return 0;
}   /* main() */