
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

/* List of foreground (FG) DETECTION modules: */
//前景检测的 模块， 返回一个CvFGDetector检测类：
/* 基类包含要实现的函数：
/* Process current image:  图片处理
virtual void    Process(IplImage* pImg) = 0;
/* Release foreground detector:   释放
virtual void    Release() = 0;
*/
static CvFGDetector* cvCreateFGDetector0      () { return cvCreateFGDetectorBase(CV_BG_MODEL_FGD,        NULL); }	
static CvFGDetector* cvCreateFGDetector0Simple() { return cvCreateFGDetectorBase(CV_BG_MODEL_FGD_SIMPLE, NULL); }		//是对FG的简单实现
static CvFGDetector* cvCreateFGDetector1      () { return cvCreateFGDetectorBase(CV_BG_MODEL_MOG,        NULL); }//高斯背景
/* 参数的相关文献
*  o CV_BG_MODEL_FGD: latest and greatest algorithm, described in*
*	 Foreground Object Detection from Videos Containing Complex Background.
*	 Liyuan Li, Weimin Huang, Irene Y.H. Gu, and Qi Tian.
*	 ACM MM2003 9p
*
*  o CV_BG_MODEL_FGD_SIMPLE:
*       A code comment describes this as a simplified version of the above,
*       but the code is in fact currently identical
*
*  o CV_BG_MODEL_MOG: "Mixture of Gaussians", older algorithm, described in
*
*       Moving target classification and tracking from real-time video.
*       A Lipton, H Fujijoshi, R Patil
*       Proceedings IEEE Workshop on Application of Computer Vision pp 8-14 1998
*
*       Learning patterns of activity using real-time tracking
*       C Stauffer and W Grimson  August 2000
*       IEEE Transactions on Pattern Analysis and Machine Intelligence 22(8):747-757
*/


/*这是c语言实现C++类的继承的一个很好的方法，*/
typedef struct DefModule_FGDetector
{
    CvFGDetector* (*create)();//指向函数的指针。使用这种写法，可以使c语言具有面向对象的特性。
    const char* nickname;//这个FGDETECTOR的别名。Nickname相当于小名。
	const char* description;//这个fgDetector的简单描述。

} DefModule_FGDetector;

DefModule_FGDetector FGDetector_Modules[] =//这是个数组。 ,每个元素对应一个背景检测的方法
{//这里就是对前景检测类 的实例化，（他用结构体模拟了类，用函数指针模拟要处理的具体方法）
    {cvCreateFGDetector0,"FG_0","Foreground Object Detection from Videos Containing Complex Background. ACM MM2003."},
    {cvCreateFGDetector0Simple,"FG_0S","Simplified version of FG_0"},
    {cvCreateFGDetector1,"FG_1","Adaptive background mixture models for real-time tracking. CVPR1999"},
    {NULL,NULL,NULL}
	//全部为空，这里有的null有很大的作用，来指示这是这个数组的最后一个，增加其他的算法。在main函数使用这个数组时会使用到这一点。
};





/* List of BLOB DETECTION modules: */
//块的检测
typedef struct DefModule_BlobDetector
{
    CvBlobDetector* (*create)();//这个BlobDetector的作用是检验新的blob
    const char* nickname;
    const char* description;
} DefModule_BlobDetector;

DefModule_BlobDetector BlobDetector_Modules[] =
{//这里定义了3个blobDetector 来检验新的blob
    {cvCreateBlobDetectorCC,"BD_CC","Detect new blob by tracking CC of FG mask"},
    {cvCreateBlobDetectorSimple,"BD_Simple","Detect new blob by uniform moving of connected components of FG mask"},
    {NULL,NULL,NULL}
};

/* List of BLOB TRACKING modules: */
//团块跟踪 虚类
typedef struct DefModule_BlobTracker
{
    CvBlobTracker* (*create)();//作用是找到blob的位置，大小，id
    const char* nickname;
    const char* description;
} DefModule_BlobTracker;

DefModule_BlobTracker BlobTracker_Modules[] =
{//5种跟踪的方法
    {cvCreateBlobTrackerCCMSPF,"CCMSPF","connected component tracking and MSPF resolver for collision"},
    {cvCreateBlobTrackerCC,"CC","Simple connected component tracking"},
    {cvCreateBlobTrackerMS,"MS","Mean shift algorithm "},
    {cvCreateBlobTrackerMSFG,"MSFG","Mean shift algorithm with FG mask using"},
    {cvCreateBlobTrackerMSPF,"MSPF","Particle filtering based on MS weight"},
	{cvCreateBlobTrackerMSFGS, "MSFGS", "Particle filtering based on MS weight" },

	{NULL,NULL,NULL}
};

/* List of BLOB TRAJECTORY GENERATION modules: */
typedef struct DefModule_BlobTrackGen
{//轨迹产生器(Trajectory Generator)主要作用是用于把轨迹存到一个文件中。
    CvBlobTrackGen* (*create)();
    const char* nickname;
    const char* description;
} DefModule_BlobTrackGen;

DefModule_BlobTrackGen BlobTrackGen_Modules[] =
{
    {cvCreateModuleBlobTrackGenYML,"YML","Generate track record in YML format as synthetic video data"},
    {cvCreateModuleBlobTrackGen1,"RawTracks","Generate raw track record (x,y,sx,sy),()... in each line"},
    {NULL,NULL,NULL}
};


/* List of BLOB TRAJECTORY POST PROCESSING modules: */
typedef struct DefModule_BlobTrackPostProc
{//轨迹的后续处理程序
    CvBlobTrackPostProc* (*create)();
    const char* nickname;
    const char* description;
} DefModule_BlobTrackPostProc;

DefModule_BlobTrackPostProc BlobTrackPostProc_Modules[] =
{
    {cvCreateModuleBlobTrackPostProcKalman,"Kalman","Kalman filtering of blob position and size"},
    {NULL,"None","No post processing filter"},
//    {cvCreateModuleBlobTrackPostProcTimeAverRect,"TimeAverRect","Average by time using rectangle window"},
//    {cvCreateModuleBlobTrackPostProcTimeAverExp,"TimeAverExp","Average by time using exponential window"},
    {NULL,NULL,NULL}
};

/* List of BLOB TRAJECTORY ANALYSIS modules: */
CvBlobTrackAnalysis* cvCreateModuleBlobTrackAnalysisDetector();

//团块跟踪结果分析
typedef struct DefModule_BlobTrackAnalysis
{
    CvBlobTrackAnalysis* (*create)();
    const char* nickname;
    const char* description;
} DefModule_BlobTrackAnalysis;

DefModule_BlobTrackAnalysis BlobTrackAnalysis_Modules[] =
{
    {cvCreateModuleBlobTrackAnalysisHistPVS,"HistPVS","Histogram of 5D feature vector analysis (x,y,vx,vy,state)"},
    {NULL,"None","No trajectory analiser"},
    {cvCreateModuleBlobTrackAnalysisHistP,"HistP","Histogram of 2D feature vector analysis (x,y)"},
    {cvCreateModuleBlobTrackAnalysisHistPV,"HistPV","Histogram of 4D feature vector analysis (x,y,vx,vy)"},
    {cvCreateModuleBlobTrackAnalysisHistSS,"HistSS","Histogram of 4D feature vector analysis (startpos,endpos)"},
    {cvCreateModuleBlobTrackAnalysisTrackDist,"TrackDist","Compare tracks directly"},
    {cvCreateModuleBlobTrackAnalysisIOR,"IOR","Integrator (by OR operation) of several analysers "},
    {NULL,NULL,NULL}
};

//上面全是模块的定义，各模块都是一个结构体（虚类）作为接口， 然后通过结构体数组，来得到各种不同的模块实现。
/* List of Blob Trajectory ANALYSIS modules: */
/*================= END MODULES DECRIPTION ===================================*/

/* Run pipeline on all frames: */
//将所有模块连接使用的函数
//根据这个来修改自己的
static int RunBlobTrackingAuto( CvCapture* pCap, CvBlobTrackerAuto* pTracker,char* fgavi_name = NULL, char* btavi_name = NULL )
{
    int                     OneFrameProcess = 0;
    int                     key;
    int                     FrameNum = 0;
    CvVideoWriter*          pFGAvi = NULL;
    CvVideoWriter*          pBTAvi = NULL;

    //cvNamedWindow( "FG", 0 );

    /* Main loop: */
	/*OneFrameProcess =0 时，为waitkey（0） 不等待了，返回-1，waitkey（1）表示等1ms，如果按键了返回按键，超时返回-1*/
    for( FrameNum=0; pCap && (key=cvWaitKey(OneFrameProcess?0:1))!=27;//按下esc键整个程序结束。 
         FrameNum++)
	{   /* Main loop: */// 整个程序的主循环。这个循环终止，意味着这个程序结束。
        IplImage*   pImg  = NULL;
        IplImage*   pMask = NULL;

        if(key!=-1)
        {
            OneFrameProcess = 1;
            if(key=='r')OneFrameProcess = 0;
        }

        pImg = cvQueryFrame(pCap);//读取视频
        if(pImg == NULL) break;


        /* Process: */
        pTracker->Process(pImg, pMask);//处理图像。这个函数应该执行完了所有的处理过程。

        if(fgavi_name)//参数设置了fg前景要保存的文件名
        if(pTracker->GetFGMask())//前景的图像的mask存在的话，保存前景。画出团块 
        {   /* Debug FG: */
            IplImage*           pFG = pTracker->GetFGMask();//得到前景的mask
            CvSize              S = cvSize(pFG->width,pFG->height);
            static IplImage*    pI = NULL;

            if(pI==NULL)pI = cvCreateImage(S,pFG->depth,3);
            cvCvtColor( pFG, pI, CV_GRAY2BGR );

            if(fgavi_name)//保存前景到视频
            {   /* Save fg to avi file: */
                if(pFGAvi==NULL)
                {
                    pFGAvi=cvCreateVideoWriter(
                        fgavi_name,
                        CV_FOURCC('x','v','i','d'),
                        25,
                        S );
                }
                cvWriteFrame( pFGAvi, pI );//写入一张图
            }


			//画出团块的椭圆
			if (pTracker->GetBlobNum()>0) //pTracker找到了blob
            {   /* Draw detected blobs: */
                int i;
                for(i=pTracker->GetBlobNum();i>0;i--)
                {
                    CvBlob* pB = pTracker->GetBlob(i-1);//得到第i-1个blob
                    CvPoint p = cvPointFrom32f(CV_BLOB_CENTER(pB));//团块中心
					//这个宏竟然是个强制转换得来的。见下行。
					//#define CV_BLOB_CENTER(pB) cvPoint2D32f(((CvBlob*)(pB))->x,((CvBlob*)(pB))->y)
                    CvSize  s = cvSize(MAX(1,cvRound(CV_BLOB_RX(pB))), MAX(1,cvRound(CV_BLOB_RY(pB))));
					//通过宏 获得团块的w 和h 的size
                    int c = cvRound(255*pTracker->GetState(CV_BLOB_ID(pB)));
                    cvEllipse( pI,//在图中，对团块画圆
                        p,
                        s,
                        0, 0, 360,
                        CV_RGB(c,255-c,0), cvRound(1+(3*c)/255) );
                }   /* Next blob: */;
            }

            cvNamedWindow( "FG",0);
            cvShowImage( "FG",pI);
        }   /* Debug FG. *///如果要保存结果，对前景保存，画出团块


		//在原图上：找到的blob附近写下id
        /* Draw debug info: */
        if(pImg)//原始的每帧图像。
        {   /* Draw all information about test sequence: */
            char        str[1024];
            int         line_type = CV_AA;   // Change it to 8 to see non-antialiased graphics.
            CvFont      font;
            int         i;
            IplImage*   pI = cvCloneImage(pImg);

            cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 0.7, 0.7, 0, 1, line_type );

            for(i=pTracker->GetBlobNum(); i>0; i--)
            {
                CvSize  TextSize;
                CvBlob* pB = pTracker->GetBlob(i-1);
                CvPoint p = cvPoint(cvRound(pB->x*256),cvRound(pB->y*256));
                CvSize  s = cvSize(MAX(1,cvRound(CV_BLOB_RX(pB)*256)), MAX(1,cvRound(CV_BLOB_RY(pB)*256)));
                int c = cvRound(255*pTracker->GetState(CV_BLOB_ID(pB)));


				//画团块到原始图像上
                cvEllipse( pI,
                    p,
                    s,
                    0, 0, 360,
                    CV_RGB(c,255-c,0), cvRound(1+(3*0)/255), CV_AA, 8 );


				//下面代码的大概意思就是在找到的blob附近写下id
                p.x >>= 8;
                p.y >>= 8;
                s.width >>= 8;
                s.height >>= 8;
                sprintf(str,"%03d",CV_BLOB_ID(pB));
                cvGetTextSize( str, &font, &TextSize, NULL );
                p.y -= s.height;
                cvPutText( pI, str, p, &font, CV_RGB(0,255,255));
                {
                    const char* pS = pTracker->GetStateDesc(CV_BLOB_ID(pB));

                    if(pS)
                    {
                        char* pStr = MY_STRDUP(pS);
                        char* pStrFree = pStr;

                        while (pStr && strlen(pStr) > 0)
                        {
                            char* str_next = strchr(pStr,'\n');

                            if(str_next)
                            {
                                str_next[0] = 0;
                                str_next++;
                            }

                            p.y += TextSize.height+1;
                            cvPutText( pI, pStr, p, &font, CV_RGB(0,255,255));
                            pStr = str_next;
                        }
                        free(pStrFree);
                    }
                }

            }   /* Next blob. */;

            cvNamedWindow( "Tracking", 0);
            cvShowImage( "Tracking",pI );

            if(btavi_name && pI)//如果这一帧存在且，你想把图像存起来，就是传过来的参数不为空例如  btavi_name=“1.avi"   就能存起来了。
            {   /* Save to avi file: */
                CvSize      S = cvSize(pI->width,pI->height);
                if(pBTAvi==NULL)
                {
                    pBTAvi=cvCreateVideoWriter(
                        btavi_name,
                        CV_FOURCC('x','v','i','d'),
                        25,
                        S );
                }
                cvWriteFrame( pBTAvi, pI );
            }

            cvReleaseImage(&pI);
        }   /* Draw all information about test sequence. */
    }   /*  Main loop. */

    if(pFGAvi)cvReleaseVideoWriter( &pFGAvi );
    if(pBTAvi)cvReleaseVideoWriter( &pBTAvi );
    return 0;
}   /* RunBlobTrackingAuto */

/* Read parameters from command line
 * and transfer to specified module:
 */
static void set_params(int argc, char* argv[], CvVSModule* pM, const char* prefix, const char* module)
{//分析参数， 分隔符 ：  类型是 a=b
    int prefix_len = (int)strlen(prefix);
    int i;
    for(i=0; i<argc; ++i)
    {
        int j;
        char* ptr_eq = NULL;
        int   cmd_param_len=0;
        char* cmd = argv[i];
        if(MY_STRNICMP(prefix,cmd,prefix_len)!=0) continue;
        cmd += prefix_len;
        if(cmd[0]!=':')continue;
        cmd++;

        ptr_eq = strchr(cmd,'=');
        if(ptr_eq)
            cmd_param_len = (int)(ptr_eq-cmd);

        for(j=0; ; ++j)
        {
            int     param_len;
            const char*   param = pM->GetParamName(j);
            if(param==NULL) break;
            param_len = (int)strlen(param);
            if(cmd_param_len!=param_len) continue;
            if(MY_STRNICMP(param,cmd,param_len)!=0) continue;
            cmd+=param_len;
            if(cmd[0]!='=')continue;
            cmd++;
            pM->SetParamStr(param,cmd);
            printf("%s:%s param set to %g\n",module,param,pM->GetParam(param));
        }
    }

    pM->ParamUpdate();

}   /* set_params */

/* Print all parameter values for given module: */
static void print_params(CvVSModule* pM, const char* module, const char* log_name)
{
    FILE* log = log_name?fopen(log_name,"at"):NULL;
    int i;
    if(pM->GetParamName(0) == NULL ) return;


    printf("%s(%s) module parameters:\n",module,pM->GetNickName());
    if(log)
        fprintf(log,"%s(%s) module parameters:\n",module,pM->GetNickName());

    for (i=0; ; ++i)
    {
        const char*   param = pM->GetParamName(i);
        const char*   str = param?pM->GetParamStr(param):NULL;
        if(param == NULL)break;
        if(str)
        {
            printf("  %s: %s\n",param,str);
            if(log)
                fprintf(log,"  %s: %s\n",param,str);
        }
        else
        {
            printf("  %s: %g\n",param,pM->GetParam(param));
            if(log)
                fprintf(log,"  %s: %g\n",param,pM->GetParam(param));
        }
    }

    if(log) fclose(log);

}   /* print_params */

int main2234(int argc, char* argv[])
{   /* Main function: */
    CvCapture*                  pCap = NULL;
    CvBlobTrackerAutoParam1     param = {0,0,0,0,0,0,0,0};
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
    DefModule_FGDetector*           pFGModule = NULL;
    DefModule_BlobDetector*         pBDModule = NULL;
    DefModule_BlobTracker*          pBTModule = NULL;
    DefModule_BlobTrackPostProc*    pBTPostProcModule = NULL;
    DefModule_BlobTrackGen*         pBTGenModule = NULL;
    DefModule_BlobTrackAnalysis*    pBTAnalysisModule = NULL;

    cvInitSystem(argc, argv);

    if(argc < 2)
    {   /* Print help: */
        int i;
        printf("blobtrack [fg=<fg_name>] [bd=<bd_name>]\n"//前景算法  团块算法的名字
            "          [bt=<bt_name>] [btpp=<btpp_name>]\n"
            "          [bta=<bta_name>\n"
            "          [bta_data=<bta_data_name>\n"
            "          [bt_corr=<bt_corr_way>]\n"
            "          [btgen=<btgen_name>]\n"
            "          [track=<track_file_name>]\n"
            "          [scale=<scale val>] [noise=<noise_name>] [IVar=<IVar_name>]\n"
            "          [FGTrainFrames=<FGTrainFrames>]\n"
            "          [btavi=<avi output>] [fgavi=<avi output on FG>]\n"//要保存的名字
            "          <avi_file>\n");

        printf("  <bt_corr_way> is the method of blob position correction for the \"Blob Tracking\" module\n"
            "     <bt_corr_way>=none,PostProcRes\n"
            "  <FGTrainFrames> is number of frames for FG training\n"
            "  <track_file_name> is file name for save tracked trajectories\n"
            "  <bta_data> is file name for data base of trajectory analysis module\n"
            "  <avi_file> is file name of avi to process by BlobTrackerAuto\n");

        puts("\nModules:");
#define PR(_name,_m,_mt)\
        printf("<%s> is \"%s\" module name and can be:\n",_name,_mt);\
        for(i=0; _m[i].nickname; ++i)\
        {\
            printf("  %d. %s",i+1,_m[i].nickname);\
            if(_m[i].description)printf(" - %s",_m[i].description);\
            printf("\n");\
        }

        PR("fg_name",FGDetector_Modules,"FG/BG Detection");
        PR("bd_name",BlobDetector_Modules,"Blob Entrance Detection");
        PR("bt_name",BlobTracker_Modules,"Blob Tracking");
        PR("btpp_name",BlobTrackPostProc_Modules, "Blob Trajectory Post Processing");
        PR("btgen_name",BlobTrackGen_Modules, "Blob Trajectory Generation");
        PR("bta_name",BlobTrackAnalysis_Modules, "Blob Trajectory Analysis");
#undef PR
        return 0;
    }   /* Print help. */

    {   /* Parse arguments: */
        int i;
        for(i=1; i<argc; ++i)
        {
            int bParsed = 0;
            size_t len = strlen(argv[i]);
#define RO(_n1,_n2) if(strncmp(argv[i],_n1,strlen(_n1))==0) {_n2 = argv[i]+strlen(_n1);bParsed=1;};
			//临时定义了一个宏，只是为了简写。 比较 输入的参数，是对应的参数，就赋值到对应name里
            RO("fg=",fg_name);
            RO("fgavi=",fgavi_name);
            RO("btavi=",btavi_name);
            RO("bd=",bd_name);
            RO("bt=",bt_name);
            RO("bt_corr=",bt_corr);
            RO("btpp=",btpp_name);
            RO("bta=",bta_name);
            RO("bta_data=",bta_data_name);
            RO("btgen=",btgen_name);
            RO("track=",track_name);
            //RO("comment=",comment_name);
            RO("FGTrainFrames=",FGTrainFrames);
            RO("log=",log_name);
            RO("savestate=",savestate_name);
            RO("loadstate=",loadstate_name);
#undef RO
            {
                char* ext = argv[i] + len-4;
                if( strrchr(argv[i],'=') == NULL &&
                    !bParsed &&
                    (len>3  ))//&&(MY_STRICMP(ext,".avi") == 0 )))
                {
                    avi_name = argv[i];//要处理的视频文件名
                    break;
                }
            }   /* Next argument. */
        }
    }   /* Parse arguments. */

    if(track_name)//如果定义了轨迹存储的文件，
    {   /* Set Trajectory Generator module: */
        int i;
        if(!btgen_name)btgen_name=BlobTrackGen_Modules[0].nickname;

		//判断是用哪一种轨迹产生方法
        for(i=0; BlobTrackGen_Modules[i].nickname; ++i)
        {//从最初这个数组的定义可以看到，最后的一个元素是null，可以根据这个条件来判断是否结束。
            if(MY_STRICMP(BlobTrackGen_Modules[i].nickname,btgen_name)==0)
                pBTGenModule = BlobTrackGen_Modules + i;
        }
    }   /* Set Trajectory Generato module. */

    /* Initialize postprocessing module if tracker
     * correction by postprocessing is required.
     */
    if(bt_corr && MY_STRICMP(bt_corr,"PostProcRes")!=0 && !btpp_name)
    {
        btpp_name = bt_corr;
        if(MY_STRICMP(btpp_name,"none")!=0)bt_corr = "PostProcRes";
    }

	//没有给参数的 就默认了
    {   /* Set default parameters for one processing: */
        if(!bt_corr) bt_corr = "none";
        if(!fg_name) fg_name = FGDetector_Modules[0].nickname;
        if(!bd_name) bd_name = BlobDetector_Modules[0].nickname;
        if(!bt_name) bt_name = BlobTracker_Modules[0].nickname;
        if(!btpp_name) btpp_name = BlobTrackPostProc_Modules[0].nickname;
        if(!bta_name) bta_name = BlobTrackAnalysis_Modules[0].nickname;
        if(!scale_name) scale_name = "1";
    }

//    if(scale_name)
  //      scale = (float)atof(scale_name);
	//将字符串的名字 转化到各种model的对应数组上。（就是model在数组的位置）
    for(pFGModule=FGDetector_Modules; pFGModule->nickname; ++pFGModule)
        if( fg_name && MY_STRICMP(fg_name,pFGModule->nickname)==0 ) break;

    for(pBDModule=BlobDetector_Modules; pBDModule->nickname; ++pBDModule)
        if( bd_name && MY_STRICMP(bd_name,pBDModule->nickname)==0 ) break;

    for(pBTModule=BlobTracker_Modules; pBTModule->nickname; ++pBTModule)
        if( bt_name && MY_STRICMP(bt_name,pBTModule->nickname)==0 ) break;

    for(pBTPostProcModule=BlobTrackPostProc_Modules; pBTPostProcModule->nickname; ++pBTPostProcModule)
        if( btpp_name && MY_STRICMP(btpp_name,pBTPostProcModule->nickname)==0 ) break;

    for(pBTAnalysisModule=BlobTrackAnalysis_Modules; pBTAnalysisModule->nickname; ++pBTAnalysisModule)
        if( bta_name && MY_STRICMP(bta_name,pBTAnalysisModule->nickname)==0 ) break;

    /* Create source video: */
    if(avi_name)
        pCap = cvCaptureFromFile(avi_name);//打开视频
	//pCap = cvCreateCameraCapture(-1);//从摄像头中读取数据。

    if(pCap==NULL)
    {
        printf("Can't open %s file\n",avi_name);
        return -1;
    }


    {   /* Display parameters: */
        int i;
        FILE* log = log_name?fopen(log_name,"at"):NULL;
        if(log)//rug如果定义了log的name那么就给你输出到一个log文件中。
        {   /* Print to log file: */
            fprintf(log,"\n=== Blob Tracking pipline in processing mode===\n");
            if(avi_name)
            {
                fprintf(log,"AVIFile: %s\n",avi_name);
            }
            fprintf(log,"FGDetector:   %s\n", pFGModule->nickname);
            fprintf(log,"BlobDetector: %s\n", pBDModule->nickname);
            fprintf(log,"BlobTracker:  %s\n", pBTModule->nickname);
            fprintf(log,"BlobTrackPostProc:  %s\n", pBTPostProcModule->nickname);
            fprintf(log,"BlobCorrection:  %s\n", bt_corr);

            fprintf(log,"Blob Trajectory Generator:  %s (%s)\n",
                pBTGenModule?pBTGenModule->nickname:"None",
                track_name?track_name:"none");

            fprintf(log,"BlobTrackAnalysis:  %s\n", pBTAnalysisModule->nickname);
            fclose(log);
        }

        printf("\n=== Blob Tracking pipline in %s mode===\n","processing");
        if(yml_name)//如果定义了yml_name 那么就读取配置文件。
        {
            printf("ConfigFile: %s\n",yml_name);
            printf("BG: %s\n",yml_video_names[0]);
            printf("FG: ");
            for(i=1;i<(yml_video_num);++i){printf("%s",yml_video_names[i]);if((i+1)<yml_video_num)printf("|");};
            printf("\n");
        }
        if(avi_name)
        {
            printf("AVIFile: %s\n",avi_name);
        }
        printf("FGDetector:   %s\n", pFGModule->nickname);
        printf("BlobDetector: %s\n", pBDModule->nickname);
        printf("BlobTracker:  %s\n", pBTModule->nickname);
        printf("BlobTrackPostProc:  %s\n", pBTPostProcModule->nickname);
        printf("BlobCorrection:  %s\n", bt_corr);

        printf("Blob Trajectory Generator:  %s (%s)\n",
            pBTGenModule?pBTGenModule->nickname:"None",
            track_name?track_name:"none");

        printf("BlobTrackAnalysis:  %s\n", pBTAnalysisModule->nickname);

    }   /* Display parameters. */


	/*这一段是通过调用结构体的函数指针，*/
    {   /* Create autotracker module and its components: */
        param.FGTrainFrames = FGTrainFrames?atoi(FGTrainFrames):0;//上面的FgTrainFrames 是int类型，，指示当前的训练的帧数。
		//param的主要作用是param内部的指向各个模块的指针
        /* Create FG Detection module: */
        param.pFG = pFGModule->create();//pFg的作用是指向CvFGDetector模块的地址。
		//pFGModule原本是一个数组，通过比较名字，++ 得到了参数要的方法。

        if(!param.pFG)
            puts("Can not create FGDetector module");
        param.pFG->SetNickName(pFGModule->nickname);
        set_params(argc, argv, param.pFG, "fg", pFGModule->nickname);

        /* Create Blob Entrance Detection module: */
        param.pBD = pBDModule->create();
        if(!param.pBD)
            puts("Can not create BlobDetector module");
        param.pBD->SetNickName(pBDModule->nickname);
        set_params(argc, argv, param.pBD, "bd", pBDModule->nickname);

        /* Create blob tracker module: */
        param.pBT = pBTModule->create();
        if(!param.pBT)
            puts("Can not create BlobTracker module");
        param.pBT->SetNickName(pBTModule->nickname);
        set_params(argc, argv, param.pBT, "bt", pBTModule->nickname);

        /* Create blob trajectory generation module: */
        param.pBTGen = NULL;
        if(pBTGenModule && track_name && pBTGenModule->create)
        {
            param.pBTGen = pBTGenModule->create();
            param.pBTGen->SetFileName(track_name);
        }
        if(param.pBTGen)
        {
            param.pBTGen->SetNickName(pBTGenModule->nickname);
            set_params(argc, argv, param.pBTGen, "btgen", pBTGenModule->nickname);
        }

        /* Create blob trajectory post processing module: */
        param.pBTPP = NULL;
        if(pBTPostProcModule && pBTPostProcModule->create)
        {
            param.pBTPP = pBTPostProcModule->create();
        }
        if(param.pBTPP)
        {
            param.pBTPP->SetNickName(pBTPostProcModule->nickname);
            set_params(argc, argv, param.pBTPP, "btpp", pBTPostProcModule->nickname);
        }

        param.UsePPData = (bt_corr && MY_STRICMP(bt_corr,"PostProcRes")==0);

        /* Create blob trajectory analysis module: */
        param.pBTA = NULL;
        if(pBTAnalysisModule && pBTAnalysisModule->create)
        {
            param.pBTA = pBTAnalysisModule->create();
            param.pBTA->SetFileName(bta_data_name);
        }
        if(param.pBTA)
        {
            param.pBTA->SetNickName(pBTAnalysisModule->nickname);
            set_params(argc, argv, param.pBTA, "bta", pBTAnalysisModule->nickname);
        }

        /* Create whole pipline: */
		// 原来上面全是param的初始化。通过param来初始化pTracker
        pTracker = cvCreateBlobTrackerAuto1(&param);
        if(!pTracker)
            puts("Can not create BlobTrackerAuto");
    }

    {   /* Load states of each module from state file: */
        CvFileStorage* fs = NULL;
        if(loadstate_name)//如果你想从已有的配置文件中读取数据就定义loadstate_name中读取。
            fs=cvOpenFileStorage(loadstate_name,NULL,CV_STORAGE_READ);
        if(fs)
        {
            printf("Load states for modules...\n");
            if(param.pBT)
            {
                CvFileNode* fn = cvGetFileNodeByName(fs,NULL,"BlobTracker");
                param.pBT->LoadState(fs,fn);
            }

            if(param.pBTA)
            {
                CvFileNode* fn = cvGetFileNodeByName(fs,NULL,"BlobTrackAnalyser");
                param.pBTA->LoadState(fs,fn);
            }

            if(pTracker)
            {
                CvFileNode* fn = cvGetFileNodeByName(fs,NULL,"BlobTrackerAuto");
                pTracker->LoadState(fs,fn);
            }

            cvReleaseFileStorage(&fs);
            printf("... Modules states loaded\n");
        }
    }   /* Load states of each module. */

    {   /* Print module parameters: */
        struct DefMMM
        {
            CvVSModule* pM;
            const char* name;
        } Modules[] = {
            {(CvVSModule*)param.pFG,"FGdetector"},
            {(CvVSModule*)param.pBD,"BlobDetector"},
            {(CvVSModule*)param.pBT,"BlobTracker"},
            {(CvVSModule*)param.pBTGen,"TrackGen"},
            {(CvVSModule*)param.pBTPP,"PostProcessing"},
            {(CvVSModule*)param.pBTA,"TrackAnalysis"},
            {NULL,NULL}
        };
        int     i;
        for(i=0; Modules[i].name; ++i)//打各个模块的信息
        {
            if(Modules[i].pM)
                print_params(Modules[i].pM,Modules[i].name,log_name);
        }
    }   /* Print module parameters. */

    /* Run pipeline: //开始流水线。流水线退出，意味着程序结束，
	这里出错了*/
    RunBlobTrackingAuto( pCap, pTracker, fgavi_name, btavi_name );

    {   /* Save state and release modules: *///释放内存，摄像头句柄，保存文件等
        CvFileStorage* fs = NULL;
        if(savestate_name)
        {
            fs=cvOpenFileStorage(savestate_name,NULL,CV_STORAGE_WRITE);
        }
        if(fs)
        {
            cvStartWriteStruct(fs,"BlobTracker",CV_NODE_MAP);
            if(param.pBT)param.pBT->SaveState(fs);
            cvEndWriteStruct(fs);
            cvStartWriteStruct(fs,"BlobTrackerAuto",CV_NODE_MAP);
            if(pTracker)pTracker->SaveState(fs);
            cvEndWriteStruct(fs);
            cvStartWriteStruct(fs,"BlobTrackAnalyser",CV_NODE_MAP);
            if(param.pBTA)param.pBTA->SaveState(fs);
            cvEndWriteStruct(fs);
            cvReleaseFileStorage(&fs);
        }
        if(param.pBT)cvReleaseBlobTracker(&param.pBT);
        if(param.pBD)cvReleaseBlobDetector(&param.pBD);
        if(param.pBTGen)cvReleaseBlobTrackGen(&param.pBTGen);
        if(param.pBTA)cvReleaseBlobTrackAnalysis(&param.pBTA);
        if(param.pFG)cvReleaseFGDetector(&param.pFG);
        if(pTracker)cvReleaseBlobTrackerAuto(&pTracker);

    }   /* Save state and release modules. */

    if(pCap)
        cvReleaseCapture(&pCap);

    return 0;

}   /* main() */
