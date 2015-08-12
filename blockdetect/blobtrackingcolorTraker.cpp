#include "precomp.hpp"
#include <opencv/highgui.h>
#include "colorTracker/color_tracker.hpp"
using namespace cv::colortracker;
using namespace std;

class CvBlobTrackerOneColorTracker :public CvBlobTrackerOne
{
public:
	CvSize          m_ObjSize;
	CvBlob          m_Blob; //一个团块，就是跟踪的目标了，一个目标对应自己的一个跟踪器
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
		//设置初始位置，大小，计算中心
		params.imageSize.height= pImg->height;
		params.imageSize.width = pImg->width;
		
		tracker.setParam(params);//设置参数， 读入w2c矩阵
		tracker.init_tracking();
		cv::Mat currentframe = pImg;

		tracker.track_frame(currentframe);//第一帧处理
		//初始化的前期， 
	};

	//是一种在直方图基础上，不断meanshift 移动团块位置，得到最佳匹配目标位置的方法。
	virtual CvBlob* Process(CvBlob* pBlobPrev, IplImage* pImg, IplImage* pImgFG = NULL)
	{//计算当前pBlobPrev 区域
		if (pBlobPrev)
		{
			m_Blob = pBlobPrev[0];
		}

		cv::Mat currentframe = pImg;
		cv::Rect temprect=tracker.track_frame(currentframe);//第一帧处理
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
		return 1.0;//最后返回 匹配度，模型的匹配度
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
