#pragma once
#include <stdio.h>
#include <stdlib.h>
#include<iostream>
#include <thread>
#include <utility>
#include <chrono>
#include <atomic>
#include <functional>
#include <mutex>

#include <stdio.h>
#include <tchar.h>
//#include <windows.h>
#include <Kinect.h>// Kinect Header files
#include <opencv2\opencv.hpp>//用这个 +opencv文件夹 和opencv2就可以了
#include "BaseImageTools.h"
#include<iomanip>
#include "BackGroundProcess.h"
#include "MyTime.h" 

#include "ImageSegment.h"
#include "blockdetect/blobtrack.hpp"
using namespace std;
using namespace cv;
// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}


class MyKinect
{

public:
	static const int		COLOR_RESIZE_FACTOR = 3 * 5;//5 4 ， 2对于轮廓还行，傅里叶太慢
	static const int		DEPTH_RESIZE_FACTOR = 5;
	static const int        cColorWidth = 1920;
	static const int        cColorHeight = 1080;
	static const int        cDepthWidth = 512;
	static const int        cDepthHeight = 424;

	static const  int		cBodyIndexWidth = cDepthWidth;
	static const int		cBodyIndexHeight = cDepthHeight;

	static const int        cColorWidth_RESIZE = cColorWidth / COLOR_RESIZE_FACTOR;// 640;//缩小的尺寸190/3
	static const int        cColorHeight_RESIZE = cColorHeight / COLOR_RESIZE_FACTOR;// 360;// 1080/3

	static const int		cDepthWidth_RESIZE = cDepthWidth / DEPTH_RESIZE_FACTOR;
	static const int		cDepthHeight_RESIZE = cDepthHeight / DEPTH_RESIZE_FACTOR;
	/*static const int        cColorWidth_RESIZE = 640;//缩小的尺寸190/3
	static const int        cColorHeight_RESIZE = 360;// 1080/3
	const int				RESIZE_FACTOR=3;*/


	USHORT cDepthMinReliableDistance ;
	USHORT cDepthMaxDistance ;

	MyKinect();
	~MyKinect();
	HRESULT					InitKinect();//初始化Kinect
	
	void					Update();//更新数据
	//bool					Produce_ColorFrame(IMultiSourceFrame * pMultiSourceFrame);
	bool					Process_ColorFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool);
	bool					Process_DepthFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool);
	//bool					Process_DepthFrame(IMultiSourceFrame * pMultiSourceFrame);
	bool					Process_BodyFrame(IMultiSourceFrame * pMultiSourceFrame,bool *returnbool);
	bool					Process_BodyIndexFrame(IMultiSourceFrame * pMultiSourceFrame,bool *returnbool);
	Mat						m_bodyIndex;

	void					ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies);
	INT64					m_nTime;
//	int		m_BodyCount;==BODY_COUNT==6
	IBody *					m_ppBodies[BODY_COUNT];
	Mat						m_bodyMat;
	void					ProcessDepth(const UINT16* pBuffer, int nWidth, int nHeight, USHORT nMinDepth, USHORT nMaxDepth);
	void					ProcessColor(RGBQUAD* pBuffer, int nWidth, int nHeight);//处理得到的数据
	void					ProcessBodyIndex(const BYTE* pBodyIndexBuffer, int nBodyIndexWidth, int nBodyIndexHeight);
	BYTE					*m_pBodyIndexBuffer;
	UINT					m_IBodyIndexBufferSize;


	bool					Process_AudioFrame(IMultiSourceFrame * pMultiSourceFrame, bool *returnbool);

	void					ProcessCoordinates_andor(Mat color, Mat depth, DepthSpacePoint	* m_pColorCoordinates);
	void					ProcessCoordinatesTest(Mat color, Mat depth, DepthSpacePoint	* m_pColorCoordinates);
	Mat						ProcessCoordinates( Mat &color,  Mat& depth, DepthSpacePoint	* m_pColorCoordinates);
	//	Mat						ProcessCoordinates(Mat color, Mat depth, DepthSpacePoint	* m_pColorCoordinates);
private:

	IKinectSensor*          m_pKinectSensor;// Current Kinect

	IMultiSourceFrameReader*m_pMultiSourceFrameReader;
	
	IColorFrameReader*      m_pColorFrameReader;// Color reader
	RGBQUAD*                m_pColorRGBX;
	Mat						m_pColorMat_Last;
	Mat						m_pColorMat_LastMask;//彩色图像的前景

	Mat						m_pColorMat_Last_Resize;//缩小后的图 
	Mat						m_pColorMat_LastMask_Resize;//彩色图像的前景

	Mat						m_pSyncMat_MASK;//同步帧的 color depth结合后的结果。

	Mat						m_pTempshow;//彩色图像的前景

	IDepthFrameReader*      m_pDepthFrameReader;// Color reader
	RGBQUAD*                m_pDepthRGBX;
	RGBQUAD*                m_pDepthRGBX_Last;
	Mat						m_pDepthMat_Last;
	Mat						m_pDepthMat_LastMask;//深度图像的前景
	
	Mat						m_pDepthMat_Last_Resize;
	Mat						m_pDepthMat_LastMask_Resize;//深度图像的前景


	Mat						m_pDepthMatInColorSize;//深度图像映射到color的图

	BackGroundProcess		m_pDepthBackProcess;

	IBodyFrameReader*		m_pBodyFrameReader;
	//RGBQUAD*				m_pBodyRGBX;

	ICoordinateMapper*		m_pCoordinateMapper;
	DepthSpacePoint	*		m_pColorCoordinates;
	UINT16*					m_pDepthBuffer;
	RGBQUAD*				m_pColorBuffer;
	bool					isFirstFrame;

	int						nFrames;
	int						colorFrames;
	int						depthFrames;
	int						colorLostFrames;
	int						depthLostFrames;

	int						prcossedFrames;//记录处理好的同步帧

	bool					colorDepthFramesynchronization;
	/*int						nDepthWidth;
	int						nDepthHeight;
	int						nColorWidth;
	int						nColorHeight;
	*/

	IAudioSource*			m_pAudioSource ;//音频处理
	BYTE*                   m_pAudioBuffer;
	// 音频帧读取器
	IAudioBeamFrameReader*  m_pAudioBeamFrameReader = nullptr;



	//跟踪模块的成员
	CvBlobTrackerAuto1*          pTracker = NULL;


	VideoWriter rgbWriter ;
	VideoWriter depthWriter;
	VideoWriter depthWriter2;
};

