#include "BackGroundProcess.h"


BackGroundProcess::BackGroundProcess()  //:codeBook()
{
	numberOfFrame = 0;
	// setMyMethod();
	depthProcess = new VuMeter;//要过滤小点11
	colorProcess = new MixtureOfGaussianV1BGS;//8  COLOR BEST虽然消去过多
	//2015-5-9 修改了学习率
	//colorProcess = new LBSimpleGaussian;  抗光照太差，
	//cv::namedWindow("depthback");
//	cv::moveWindow("depthback", 0, 500);

// 

}


BackGroundProcess::~BackGroundProcess()
{
}
Mat BackGroundProcess::process_Mog(Mat inputMat, Mat &outputMat)//用高斯方法1
{
	//double learnRate = 0.02;
	//Mat outputMat;
	mog(inputMat, outputMat, -1);
	numberOfFrame++;
	//IplImage * foreImage = &(IplImage(foreMaskMat));

	//cvErode(foreImage, foreImage, 0, 2);这是C的写法，只对iplimage有效，所以上面要mat转iplimag
	//cvDilate(foreImage, foreImage, 0, 1);

	cv::erode(outputMat, outputMat, Mat(), cv::Point(-1, -1), 2);
	cv::dilate(outputMat, outputMat, Mat());

	imshow("Mog前景",outputMat);
	return outputMat;
}

Mat BackGroundProcess::process_Mog2(Mat inputMat, Mat &outputMat)//用高斯方法1
{
	numberOfFrame++;
/*	if (numberOfFrame >15)
	{//只用前20帧做背景
		mog2(inputMat, outputMat, 0.002);
	}
	else
	{
		mog2(inputMat, outputMat, -1);
	}*/

	mog2(inputMat, outputMat, -1);
	//double learnRate = 0.02;
	//Mat outputMat;
	//

	//cout << "帧数  "<<numberOfFrame << endl;

	//IplImage * foreImage = &(IplImage(foreMaskMat));

	//cvErode(foreImage, foreImage, 0, 2);这是C的写法，只对iplimage有效，所以上面要mat转iplimag
	//cvDilate(foreImage, foreImage, 0, 1);

	cv::erode(outputMat, outputMat, Mat(), cv::Point(-1, -1));
	cv::dilate(outputMat, outputMat, Mat());

	imshow("Mog2前景", outputMat);
	return outputMat;
}

Mat  BackGroundProcess::colorProcessBackGround(Mat inputMat, Mat &outputMat)
{
	cv::Mat img_bkgmodel;
	colorProcess->process(inputMat, outputMat, img_bkgmodel);
	return outputMat;
}
Mat BackGroundProcess::process_codeBook(Mat inputMat, Mat &outputMat)
{
	codeBook.process(inputMat, outputMat);
	//imshow("codebook", outputMat);
	//cvMoveWindow("codebook",500,500);
	return outputMat;
}
Mat  BackGroundProcess::depthProcessBackGround(Mat inputMat, Mat &outputMat)
{
	numberOfFrame++;

	cv::Mat img_bkgmodel;
	cv::Mat  foreMat;
	if (inputMat.empty())
	{
		return outputMat;
	}
	depthProcess->process(inputMat, foreMat, img_bkgmodel);
	if (foreMat.rows != 0 && foreMat.cols != 0)
	{
		outputMat = foreMat.clone();
	}
	if (!outputMat.empty())
	{
		//cv::imshow("Foreground", outputMat);
	//	cv::imshow("depthback", outputMat);
	}
	return outputMat;

}

void BackGroundProcess::setMyMethod()
{
	//在这当中选深度的最好的,做一个搬凳子的sp
	//bestbgs=new VuMeter;//要过滤小点11
	
//	bestbgs = new LBSimpleGaussian;


	cv::namedWindow("best");
	cv::moveWindow("best", 0, 500);
	
	//接着调试后的几个
	bgs1 = new LBMixtureOfGaussians;//10
	bgs2= new MixtureOfGaussianV1BGS;// = new LBFuzzyGaussian; //8 color 消去太多， 光照免疫
	
	//bgs3 = new LBSimpleGaussian; 
	//new StaticFrameDifferenceBGS;;//1//都行，有点光照问题
	//这个最佳？？，挑一挑其他的阈值看看
 



/*	快速
bgs1 = new DPZivkovicAGMMBGS;//11

bgs1 = new DPWrenGABGS;//10
bgs1 = new LBSimpleGaussian;//11
bgs1 = new MixtureOfGaussianV1BGS;//8

bgs1 = new VuMeter;//要过滤小点11
bgs1 = new LBMixtureOfGaussians;//15
bgs1 = new LBFuzzyGaussian;//17

*/

/*中速
bgs1 = new T2FGMM_UV;//35
bgs4 = new DPGrimsonGMMBGS;//34
bgs1 = new LbpMrf;
*/


	cv::namedWindow("1");
	cv::moveWindow("1", 500, 0);

	cv::namedWindow("2");
	cv::moveWindow("2", 1000, 0);

//	cv::namedWindow("3");
//	cv::moveWindow("3", 500, 500);

	/*cv::namedWindow("4");
	cv::moveWindow("4", 1000, 500);

	
	cv::namedWindow("3");
	cv::moveWindow("3", 1500, 0);

	cv::namedWindow("4");
	cv::moveWindow("4", 0, 500);
	
	cv::namedWindow("5");
	cv::moveWindow("5", 500, 500);

	cv::namedWindow("6");
	cv::moveWindow("6", 1000, 500);

	cv::namedWindow("7");
	cv::moveWindow("7", 1500, 500);
	*/

}
Mat BackGroundProcess::testManyMethod(Mat inputMat, Mat &outputMat)//测试各种方法
{
	/* Background Subtraction Methods */


	/*** Default Package ***/
	//bgs = new FrameDifferenceBGS;
	//bgs = new StaticFrameDifferenceBGS;
	//bgs = new WeightedMovingMeanBGS;
	//bgs = new WeightedMovingVarianceBGS;
	//bgs = new MixtureOfGaussianV1BGS;
	//bgs = new MixtureOfGaussianV2BGS;
	//bgs = new AdaptiveBackgroundLearning;
	//bgs = new AdaptiveSelectiveBackgroundLearning;

	//bgs = new GMG;

	/*** DP Package (thanks to Donovan Parks) ***/
	//bgs = new DPAdaptiveMedianBGS;
	//bgs = new DPGrimsonGMMBGS;
	//bgs = new DPZivkovicAGMMBGS;
	//bgs = new DPMeanBGS;
	//bgs = new DPWrenGABGS;
	//bgs = new DPPratiMediodBGS;
	//bgs = new DPEigenbackgroundBGS;
	//bgs = new DPTextureBGS;

	/*** TB Package (thanks to Thierry Bouwmans, Fida EL BAF and Zhenjie Zhao) ***/
	//bgs = new T2FGMM_UM;
	//bgs = new T2FGMM_UV;
	//bgs = new T2FMRF_UM;
	//bgs = new T2FMRF_UV;
	//bgs = new FuzzySugenoIntegral;
	//bgs = new FuzzyChoquetIntegral;

	/*** JMO Package (thanks to Jean-Marc Odobez) ***/
	//bgs = new MultiLayerBGS;

	/*** PT Package (thanks to Martin Hofmann, Philipp Tiefenbacher and Gerhard Rigoll) ***/
	//bgs = new PixelBasedAdaptiveSegmenter;

	/*** LB Package (thanks to Laurence Bender) ***/
	//bgs = new LBSimpleGaussian;
	//bgs = new LBFuzzyGaussian;
	//bgs = new LBMixtureOfGaussians;
	//bgs = new LBAdaptiveSOM;
	//bgs = new LBFuzzyAdaptiveSOM;

	/*** LBP-MRF Package (thanks to Csaba Kertész) ***/
	//bgs = new LbpMrf;

	/*** AV Package (thanks to Lionel Robinault and Antoine Vacavant) ***/
	//bgs = new VuMeter;

	/*** EG Package (thanks to Ahmed Elgammal) ***/
	//bgs = new KDE;

	/*** DB Package (thanks to Domenico Daniele Bloisi) ***/
	//bgs = new IndependentMultimodalBGS;

	/*** SJN Package (thanks to SeungJong Noh) ***/
	//bgs = new SJN_MultiCueBGS;

	/*** BL Package (thanks to Benjamin Laugraud) ***/
	//bgs = new SigmaDeltaBGS;

	/*** PL Package (thanks to Pierre-Luc) ***/
	//bgs = new SuBSENSEBGS();
	//bgs = new LOBSTERBGS();

		cv::Mat img_bkgmodel;
		cv::Mat  foreMat;
		
		//暂时codebook最好了
		codeBook.process(inputMat, outputMat);
		cv::imshow("best", outputMat);

		/*
		bestbgs->process(inputMat, foreMat, img_bkgmodel);
		if (foreMat.rows != 0 && foreMat.cols != 0)
		{
			outputMat = foreMat.clone();
		}
		if (!outputMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("best", outputMat);
		}
		*/

		bgs1->process(inputMat, foreMat, img_bkgmodel); 
		if (!outputMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("1", outputMat);
		}
	//	cout << foreMat.channels();

		foreMat = Mat::zeros(0, 0, CV_8UC1);
		bgs2->process(inputMat, foreMat, img_bkgmodel);
		if (!foreMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("2", foreMat);
		}

	/*	foreMat = Mat::zeros(0, 0, CV_8UC1);
		bgs3->process(inputMat, foreMat, img_bkgmodel);
		if (!foreMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("3", foreMat);
		}


			foreMat = Mat::zeros(0, 0, CV_8UC1);
		bgs4->process(inputMat, foreMat, img_bkgmodel);
		if (!foreMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("4", foreMat);
		}


	
		foreMat = Mat::zeros(0, 0, CV_8UC1);
		bgs5->process(inputMat, foreMat, img_bkgmodel);
		if (!foreMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("5", foreMat);
		}

		foreMat = Mat::zeros(0, 0, CV_8UC1);
		bgs6->process(inputMat, foreMat, img_bkgmodel);
		if (!foreMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("6", foreMat);
		}

		foreMat = Mat::zeros(0, 0, CV_8UC1);
		bgs7->process(inputMat, foreMat, img_bkgmodel);
		if (!foreMat.empty())
		{
			//cv::imshow("Foreground", outputMat);
			cv::imshow("7", foreMat);
		}


		if (!img_bkgmodel.empty())
		{
		//	cv::imshow("backgounp", img_bkgmodel);
		}
		*/




		return outputMat;	//delete bgs;
}

/*
bgs = new FrameDifferenceBGS;
//bgs = new StaticFrameDifferenceBGS;
//bgs = new WeightedMovingMeanBGS;
//bgs = new WeightedMovingVarianceBGS;
//bgs = new MixtureOfGaussianV1BGS;
//bgs = new MixtureOfGaussianV2BGS;
//bgs = new AdaptiveBackgroundLearning;
//bgs = new AdaptiveSelectiveBackgroundLearning;
//bgs = new GMG;

/*** DP Package (thanks to Donovan Parks) ***/
//bgs = new DPAdaptiveMedianBGS;
//bgs = new DPGrimsonGMMBGS;
//bgs = new DPZivkovicAGMMBGS;
//bgs = new DPMeanBGS;
//bgs = new DPWrenGABGS;
//bgs = new DPPratiMediodBGS;
//bgs = new DPEigenbackgroundBGS;
//bgs = new DPTextureBGS;

/*** TB Package (thanks to Thierry Bouwmans, Fida EL BAF and Zhenjie Zhao) ***/
//bgs = new T2FGMM_UM;
//bgs = new T2FGMM_UV;
//bgs = new T2FMRF_UM;
//bgs = new T2FMRF_UV;
//bgs = new FuzzySugenoIntegral;
//bgs = new FuzzyChoquetIntegral;

/*** JMO Package (thanks to Jean-Marc Odobez) ***/
//bgs = new MultiLayerBGS;

/*** PT Package (thanks to Martin Hofmann, Philipp Tiefenbacher and Gerhard Rigoll) ***/
//bgs = new PixelBasedAdaptiveSegmenter;

/*** LB Package (thanks to Laurence Bender) ***/
//bgs = new LBSimpleGaussian;
//bgs = new LBFuzzyGaussian;
//bgs = new LBMixtureOfGaussians;
//bgs = new LBAdaptiveSOM;
//bgs = new LBFuzzyAdaptiveSOM;

/*** LBP-MRF Package (thanks to Csaba Kertész) ***/
//bgs = new LbpMrf;

/*** AV Package (thanks to Lionel Robinault and Antoine Vacavant) ***/
//bgs = new VuMeter;

/*** EG Package (thanks to Ahmed Elgammal) ***/
//bgs = new KDE;

/*** DB Package (thanks to Domenico Daniele Bloisi) ***/
//bgs = new IndependentMultimodalBGS;

/*** SJN Package (thanks to SeungJong Noh) ***/
//bgs = new SJN_MultiCueBGS;

/*** BL Package (thanks to Benjamin Laugraud) ***/
//bgs = new SigmaDeltaBGS;

/*** PL Package (thanks to Pierre-Luc) ***/
//bgs = new SuBSENSEBGS();
//bgs = new LOBSTERBGS();*/