#include "precomp.hpp"
#include <opencv/highgui.h>
#include "colorTracker/color_tracker.hpp"
using namespace cv::colortracker;
using namespace std;

class CvBlobTrackerOneColorTracker :public CvBlobTrackerOne
{
public:
	CvSize          m_ObjSize;
	CvBlob          m_Blob; //һ���ſ飬���Ǹ��ٵ�Ŀ���ˣ�һ��Ŀ���Ӧ�Լ���һ��������
	int             m_Collision;
	ColorTracker tracker;
public:
	CvBlobTrackerOneColorTracker()
	{
		/* Initialize internal data: */
		m_Collision = 0;
		//        m_BinBit=0;
		SetModuleName("ColorTracker");
	}

	~CvBlobTrackerOneColorTracker()
	{	}
	virtual void ParamUpdate()
	{	}

	/* Interface: */
	virtual void Init(CvBlob* pBlobInit, IplImage* pImg, IplImage* pImgFG = NULL)
	{

		m_Blob = pBlobInit[0];
		ColorTrackerParameters params;

		params.visualization = 1;
		cv::Point pos = cv::Point(pBlobInit->x, pBlobInit->y);
		cv::Size target_sz = cv::Size(pBlobInit->w, pBlobInit->h);
		params.init_pos.x = (int)(floor(pos.x) + floor(target_sz.width / 2));
		params.init_pos.y = (int)(floor(pos.y) + floor(target_sz.height / 2));
		params.wsize = cv::Size((int)floor(target_sz.width), (int)floor(target_sz.height));
		//���ó�ʼλ�ã���С����������
		params.imageSize.height= pImg->height;
		params.imageSize.width = pImg->width;
		
		tracker.setParam(params);//���ò����� ����w2c����
		tracker.init_tracking();
		cv::Mat currentframe = pImg;

		tracker.track_frame(currentframe);//��һ֡����
		//��ʼ����ǰ�ڣ� 
	};

	//��һ����ֱ��ͼ�����ϣ�����meanshift �ƶ��ſ�λ�ã��õ����ƥ��Ŀ��λ�õķ�����
	virtual CvBlob* Process(CvBlob* pBlobPrev, IplImage* pImg, IplImage* pImgFG = NULL)
	{//���㵱ǰpBlobPrev ����
		if (pBlobPrev)
		{
			m_Blob = pBlobPrev[0];
		}

		cv::Mat currentframe = pImg;
		cv::Rect temprect=tracker.track_frame(currentframe);//��һ֡����
		m_Blob = rectToBlob(temprect);
		return &m_Blob;

	};  /* CvBlobTrackerOneMSFG::Process */

	CvBlob rectToBlob(cv::Rect rect)
	{
		CvBlob blob;
		blob.x = rect.x;
		blob.y = rect.y;
		blob.w = rect.width;
		blob.h = rect.height;
		return blob;
	}
	virtual double GetConfidence(CvBlob* pBlob, IplImage* pImg, IplImage* /*pImgFG*/ = NULL, IplImage* pImgUnusedReg = NULL)
	{//  
		return 1.0;//��󷵻� ƥ��ȣ�ģ�͵�ƥ���
	};  /*CvBlobTrackerOneMSFG::*/

	virtual void Update(CvBlob* pBlob, IplImage* pImg, IplImage* pImgFG = NULL)
	{    }    

	virtual void Release(){ delete this; };
	virtual void SetCollision(int CollisionFlag)
	{
		m_Collision = CollisionFlag;
	}
	

};  /*cvCreateBlobTrackerOneColorTracker*/


static CvBlobTrackerOne* cvCreateBlobTrackerOneColorTracker()
{
	return (CvBlobTrackerOne*) new CvBlobTrackerOneColorTracker;
}

CvBlobTracker* cvCreateBlobTrackerListColorTracker()
{
	return cvCreateBlobTrackerList(cvCreateBlobTrackerOneColorTracker);
}
