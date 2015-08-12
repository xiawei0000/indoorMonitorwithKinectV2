#include "precomp.hpp"

#define  PIX_HIST_BIN_NUM_1  3 //number of bins for classification (not used now)
#define  PIX_HIST_BIN_NUM_2  5 //number of bins for statistic collection
#define  PIX_HIST_ALPHA      0.01f //alpha-coefficient for running avarage procedure
#define  PIX_HIST_DELTA      2 //maximal difference between descriptors(RGB)
#define  PIX_HIST_COL_QUANTS 64 //quantization level in rgb-space
#define  PIX_HIST_DELTA_IN_PIX_VAL  (PIX_HIST_DELTA * 256 / PIX_HIST_COL_QUANTS) //allowed difference in rgb-space


//背景直方图的统计
// Structures for background statistics estimation:
typedef struct CvPixHistBin{//每个bin三通道
    float          bin_val;
    uchar          cols[3];
} CvPixHistBin;

typedef struct CvPixHist{//每个Hist 5个bin
    CvPixHistBin   bins[PIX_HIST_BIN_NUM_2];
} CvPixHist;

// Class for background statistics estimation:
class CvBGEstimPixHist
{
private:
    CvPixHist*  m_PixHists;
    int         m_width;
    int         m_height;

    // Function for update color histogram for one pixel:
    void update_hist_elem(int x, int y, uchar* cols )
    {//每个坐标xy处， 都有5个bin来统计， 每个bin三个通道。
		//将传入的rgb 和bin匹配，得到最匹配的一个，增加权值
        // Find closest bin:
        int    dist = 0, min_dist = 2147483647, indx = -1;
        for( int k = 0; k < PIX_HIST_BIN_NUM_2; k++ ){

            uchar* hist_cols = m_PixHists[y*m_width+x].bins[k].cols;//获取对应位置直方图每个bin

            m_PixHists[y*m_width+x].bins[k].bin_val *= (1-PIX_HIST_ALPHA);//权值下降

            int  l;
            for( l = 0; l < 3; l++ ){//计算三通道的差
                int val = abs( hist_cols[l] - cols[l] );
                if( val > PIX_HIST_DELTA_IN_PIX_VAL ) break;
                dist += val;
            }

            if( l == 3 && dist < min_dist ){
                min_dist = dist;
                indx = k;
            }
        }//对5个bin中，计算和传入的cols 距离，得到最小的一个index

		//没有找到匹配的，建立一个新的。给很小的权值
        if( indx < 0 ){   // N2th elem in the table is replaced by a new feature.
            indx = PIX_HIST_BIN_NUM_2 - 1;
            m_PixHists[y*m_width+x].bins[indx].bin_val = PIX_HIST_ALPHA;
            for(int l = 0; l < 3; l++ ){
                m_PixHists[y*m_width+x].bins[indx].cols[l] = cols[l];
            }
        }
        else {//得到最匹配的bin了，权值++
            //add vote!
            m_PixHists[y*m_width+x].bins[indx].bin_val += PIX_HIST_ALPHA;
        }
        // Re-sort bins by BIN_VAL:
        {//根据权值排序
            int k;
            for(k = 0; k < indx; k++ ){
                if( m_PixHists[y*m_width+x].bins[k].bin_val <= m_PixHists[y*m_width+x].bins[indx].bin_val ){
                    CvPixHistBin tmp1, tmp2 = m_PixHists[y*m_width+x].bins[indx];
                    // Shift elements:
                    for(int l = k; l <= indx; l++ ){
                        tmp1 = m_PixHists[y*m_width+x].bins[l];
                        m_PixHists[y*m_width+x].bins[l] = tmp2;
                        tmp2 = tmp1;
                    }
                    break;
                }
            }
        }
    }   // void update_hist(...)

    // Function for calculation difference between histograms:
    float get_hist_diff(int x1, int y1, int x2, int y2)
    {//计算直方图间的距离
        float  dist = 0;
        for( int i = 0; i < 3; i++ ){
            dist += labs(m_PixHists[y1*m_width+x1].bins[0].cols[i] -
                                m_PixHists[y2*m_width+x2].bins[0].cols[i]);
        }
        return dist;
    }


public:
    IplImage*   bg_image;

    CvBGEstimPixHist(CvSize img_size)
    {//是整个Image大小的直方图
        m_PixHists = (CvPixHist*)cvAlloc(img_size.width*img_size.height*sizeof(CvPixHist));
        memset( m_PixHists, 0, img_size.width*img_size.height*sizeof(CvPixHist) );
        m_width = img_size.width;
        m_height = img_size.height;

        bg_image = cvCreateImage(img_size, IPL_DEPTH_8U, 3 );
    }   /* Constructor. */

    ~CvBGEstimPixHist()
    {
        cvReleaseImage(&bg_image);
        cvFree(&m_PixHists);
    }   /* Destructor. */

    // Function to update histograms and bg_image:
    void update_hists( IplImage* pImg )
    {//更新 背景图bg_image 和直方图
        for( int i = 0; i < pImg->height; i++ ){
            for( int j = 0; j < pImg->width; j++ ){
                update_hist_elem( j, i, ((uchar*)(pImg->imageData))+i*pImg->widthStep+3*j );
				//将传入的rgb 和bin匹配，得到最匹配的一个，增加权值

                ((uchar*)(bg_image->imageData))[i*bg_image->widthStep+3*j] = m_PixHists[i*m_width+j].bins[0].cols[0];
                ((uchar*)(bg_image->imageData))[i*bg_image->widthStep+3*j+1] = m_PixHists[i*m_width+j].bins[0].cols[1];
                ((uchar*)(bg_image->imageData))[i*bg_image->widthStep+3*j+2] = m_PixHists[i*m_width+j].bins[0].cols[2];
            }
        }
        // cvNamedWindow("RoadMap2",0);
        // cvShowImage("RoadMap2", bg_image);
    }
};  /* CvBGEstimPixHist */



/*======================= TRACKER LIST SHELL =====================*/
typedef struct DefBlobTrackerL
{//这是包括跟踪链和 跟踪器的结构体，使得一个目标一个跟踪器，多目标跟踪
    CvBlob                  blob;//团块位置
    CvBlobTrackerOne*       pTracker;//团块的跟踪器，每个目标团块一个
    int                     Frame;
    int                     Collision;
    CvBlobTrackPredictor*   pPredictor;
    CvBlob                  BlobPredict;//预测位置
    CvBlobSeq*              pBlobHyp;
} DefBlobTrackerL;

class CvBlobTrackerList : public CvBlobTracker
{
private:
    CvBlobTrackerOne*       (*m_Create)();//这就是构建函数了 表示返回 CvBlobTrackerOne*的函数指针
    CvBlobSeq               m_BlobTrackerList;
//    int                     m_LastID;
    int                     m_Collision;
    int                     m_ClearHyp;
    float                   m_BGImageUsing;
    CvBGEstimPixHist*       m_pBGImage;
    IplImage*               m_pImgFG;
    IplImage*               m_pImgReg; /* mask for multiblob confidence calculation */

public:
    CvBlobTrackerList(CvBlobTrackerOne* (*create)()):m_BlobTrackerList(sizeof(DefBlobTrackerL))
    {
        //int i;
        CvBlobTrackerOne* pM = create();
//        m_LastID = 0;
        m_Create = create;
        m_ClearHyp = 0;
        m_pImgFG = 0;
        m_pImgReg = NULL;

        TransferParamsFromChild(pM,NULL);

        pM->Release();

        m_Collision = 1; /* if 1 then collistion will be detected and processed */
        AddParam("Collision",&m_Collision);
        CommentParam("Collision", "if 1 then collision cases are processed in special way");

        m_pBGImage = NULL;
        m_BGImageUsing = 50;
        AddParam("BGImageUsing", &m_BGImageUsing);
        CommentParam("BGImageUsing","Weight of using BG image in update hist model (0 - BG dies not use 1 - use)");

        SetModuleName("List");
    }

    ~CvBlobTrackerList()
    {
        int i;
        if(m_pBGImage) delete m_pBGImage;
        if(m_pImgFG) cvReleaseImage(&m_pImgFG);
        if(m_pImgReg) cvReleaseImage(&m_pImgReg);
        for(i=m_BlobTrackerList.GetBlobNum();i>0;--i)
        {
            m_BlobTrackerList.DelBlob(i-1);
        }
    };

    CvBlob* AddBlob(CvBlob* pBlob, IplImage* pImg, IplImage* pImgFG )
    {	//	通过一个团块，向m_BlobTrackerList 表里添加 跟踪器+团块 的结构体
		/* Create new tracker: */
		//DefBlobTrackerL其实代表了
        DefBlobTrackerL F;//封装了blob链表， 跟踪器的结构体，
        F.blob = pBlob[0];
//        F.blob.ID = m_LastID++;
        F.pTracker = m_Create();//在这里创建？？
        F.pPredictor = cvCreateModuleBlobTrackPredictKalman();//使用kalman预测结果，那么预测模块就多余了。
        F.pBlobHyp = new CvBlobSeq;
        F.Frame = 0;
        TransferParamsToChild(F.pTracker,NULL);

        F.pTracker->Init(pBlob,pImg, pImgFG);
        m_BlobTrackerList.AddBlob((CvBlob*)&F);//同一个问题，可以强制转码？？？
        return m_BlobTrackerList.GetBlob(m_BlobTrackerList.GetBlobNum()-1);
    };

    void DelBlob(int BlobIndex)
    {
        DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIndex);
        if(pF == NULL) return;
        pF->pTracker->Release();
        pF->pPredictor->Release();
        delete pF->pBlobHyp;
        m_BlobTrackerList.DelBlob(BlobIndex);
    }

    void DelBlobByID(int BlobID)
    {
        DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlobByID(BlobID);
        if(pF == NULL) return;
        pF->pTracker->Release();
        pF->pPredictor->Release();
        delete pF->pBlobHyp;
        m_BlobTrackerList.DelBlobByID(BlobID);
    }

    virtual void Process(IplImage* pImg, IplImage* pImgFG = NULL)
	{		
		//第一步：这一段每个点，计算图片和背景差，获取前景值
		//第二步：预测好位置
		//第三步：标记每个目标的碰撞
		//已经跟踪的团块链表里， 相互匹配预测位置 和当前位置。如果重合了，就将Collision=1 
		//对于每一个已经跟踪的链表目标， 用他的可能位置去计算下一个位置。

        int i;
        if(pImgFG)
        {
            if(m_pImgFG) cvCopy(pImgFG,m_pImgFG);
            else         m_pImgFG = cvCloneImage(pImgFG);
        }

        if(m_pBGImage==NULL && m_BGImageUsing>0)
        {//估计背景的初始化
            m_pBGImage = new CvBGEstimPixHist(cvSize(pImg->width,pImg->height));
        }

		//预测了一下
        if(m_Collision)
        for(i=m_BlobTrackerList.GetBlobNum(); i>0; --i)
        {   /* Update predictor: */
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(i-1);
            pF->pPredictor->Update((CvBlob*)pF);
        }   /* Update predictor. */


		//第一步：这一段每个点，计算图片和背景差，获取前景值
        if(m_pBGImage && m_pImgFG)
        {   /* Weighting mask mask: */
            int x,y,yN=pImg->height,xN=pImg->width;
            IplImage* pImgBG = NULL;
            m_pBGImage->update_hists(pImg);//更新背景
            pImgBG = m_pBGImage->bg_image;

            for(y=0; y<yN; ++y)
            {
                unsigned char* pI = (unsigned char*)pImg->imageData + y*pImg->widthStep;
                unsigned char* pBG = (unsigned char*)pImgBG->imageData + y*pImgBG->widthStep;
                unsigned char* pFG = (unsigned char*)m_pImgFG->imageData +y*m_pImgFG->widthStep;

                for(x=0; x<xN; ++x)
                {
                    if(pFG[x])
                    {
                        int D1 = (int)(pI[3*x+0])-(int)(pBG[3*x+0]);
                        int D2 = (int)(pI[3*x+1])-(int)(pBG[3*x+1]);
                        int D3 = (int)(pI[3*x+2])-(int)(pBG[3*x+2]);
                        int DD = D1*D1+D2*D2+D3*D3;//图片和三通道的背景差距离
                        double  D = sqrt((float)DD);
                        double  DW = 25;
                        double  W = 1/(exp(-4*(D-m_BGImageUsing)/DW)+1);//距离大权值大， 
						//距离大就是图和背景差别大，自然可能是前景了
                        pFG[x] = (uchar)cvRound(W*255);//这里扩大到255了，
                    }
                }   /* Next mask pixel. */
            }   /*  Next mask line. */
        }   /* Weighting mask mask. */

		//第二步：预测好位置
        for(i=m_BlobTrackerList.GetBlobNum(); i>0; --i)
        {   /* Predict position. */
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(i-1);
            CvBlob*         pB = pF->pPredictor->Predict();
            if(pB)
            {
                pF->BlobPredict = pB[0];
                pF->BlobPredict.w = pF->blob.w;
                pF->BlobPredict.h = pF->blob.h;
            }
        }   /* Predict position. */


		//第三步：标记每个目标的碰撞
		//已经跟踪的团块链表里， 相互匹配预测位置 和当前位置。如果重合了，就将Collision=1 
        if(m_Collision)
        for(i=m_BlobTrackerList.GetBlobNum(); i>0; --i)
        {   /* Predict collision. */
            int             Collision = 0;
            int             j;
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(i-1);

            for(j=m_BlobTrackerList.GetBlobNum(); j>0; --j)
            {   /* Predict collision. */
                CvBlob* pB1;
                CvBlob* pB2;
                DefBlobTrackerL* pF2 = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(j-1);
                if(i==j) continue;
                pB1 = &pF->BlobPredict;
                pB2 = &pF2->BlobPredict;
                if( fabs(pB1->x-pB2->x)<0.5*(pB1->w+pB2->w) &&
                    fabs(pB1->y-pB2->y)<0.5*(pB1->h+pB2->h) ) Collision = 1;
                pB1 = &pF->blob;
                pB2 = &pF2->blob;
                if( fabs(pB1->x-pB2->x)<0.5*(pB1->w+pB2->w) &&
                    fabs(pB1->y-pB2->y)<0.5*(pB1->h+pB2->h) ) Collision = 1;
                if(Collision) break;
            }   /* Check next blob to cross current. */

            pF->Collision = Collision;
            pF->pTracker->SetCollision(Collision);

        }   /* Predict collision. */


		//对于每一个已经跟踪的链表目标， 用他的可能位置去计算下一个位置。
        for(i=m_BlobTrackerList.GetBlobNum(); i>0; --i)
        {   /* Track each blob. */
            DefBlobTrackerL*    pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(i-1);
            if(pF->pBlobHyp->GetBlobNum()>0)
            {   /* Track all hypothesis. */
                int h,hN = pF->pBlobHyp->GetBlobNum();
                for(h=0;h<hN;++h)
                {
                    CvBlob*     pB = pF->pBlobHyp->GetBlob(h);
                    CvBlob*     pNewBlob = pF->pTracker->Process(pB,pImg,m_pImgFG);
                    int         BlobID = CV_BLOB_ID(pB);
                    if(pNewBlob)
                    {
                        pB[0] = pNewBlob[0];
                        pB->w = MAX(CV_BLOB_MINW,pNewBlob->w);
                        pB->h = MAX(CV_BLOB_MINH,pNewBlob->h);
                        CV_BLOB_ID(pB) = BlobID;
                    }
                }   /* Next hypothesis. */

            }   /* Track all hypotheses. */

            pF->Frame++;

        }   /* Next blob. */

#if 0
        for(i=m_BlobTrackerList.GetBlobNum(); i>0; --i)
        {   /* Update predictor: */
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(i-1);
            if((m_Collision && !pF->Collision) || !m_Collision)
            {
                pF->pPredictor->Update((CvBlob*)pF);
            }
            else
            {   /* pravilnyp putem idete tovarischy!!! */
                pF->pPredictor->Update(&(pF->BlobPredict));
            }
        }   /* Update predictor. */
#endif
        m_ClearHyp = 1;
    };


    /* Process on blob (for multi hypothesis tracing) */
    virtual void ProcessBlob(int BlobIndex, CvBlob* pBlob, IplImage* pImg, IplImage* /*pImgFG*/ = NULL)
    {//在trackinglist。cpp中。 团块被包含到一个DefBlobTrackerL*    pF 里。
		//pF包含了团块 跟踪器，预处理。
        int                 ID = pBlob->ID;
        DefBlobTrackerL*    pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIndex);
        CvBlob*             pNewBlob = pF->pTracker->Process(pBlob?pBlob:&(pF->blob),pImg,m_pImgFG);
        if(pNewBlob)
        {
            pF->blob = pNewBlob[0];
            pF->blob.w = MAX(CV_BLOB_MINW,pNewBlob->w);
            pF->blob.h = MAX(CV_BLOB_MINH,pNewBlob->h);
            pBlob[0] = pF->blob;
        }
        pBlob->ID = ID;
    };

    virtual double  GetConfidence(int BlobIndex, CvBlob* pBlob, IplImage* pImg, IplImage* pImgFG = NULL)
    {
        DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIndex);
        if(pF==NULL) return 0;
        if(pF->pTracker==NULL) return 0;
        return pF->pTracker->GetConfidence(pBlob?pBlob:(&pF->blob), pImg, pImgFG, NULL);
    };

    virtual double GetConfidenceList(CvBlobSeq* pBlobList, IplImage* pImg, IplImage* pImgFG = NULL)
    {
        double  W = 1;
        int     b,bN = pBlobList->GetBlobNum();

        if(m_pImgReg == NULL)
        {
            m_pImgReg = cvCreateImage(cvSize(pImg->width,pImg->height),IPL_DEPTH_8U,1);
        }
        assert(pImg);

        cvSet(m_pImgReg,cvScalar(255));

        for(b=0; b<bN; ++b)
        {
            CvBlob* pB = pBlobList->GetBlob(b);
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlobByID(pB->ID);
            if(pF==NULL || pF->pTracker==NULL) continue;
            W *= pF->pTracker->GetConfidence(pB, pImg, pImgFG, m_pImgReg );
            cvEllipse(
                m_pImgReg,
                cvPoint(cvRound(pB->x*256),cvRound(pB->y*256)), cvSize(cvRound(pB->w*128),cvRound(pB->h*128)),
                0, 0, 360,
                cvScalar(0), CV_FILLED, 8, 8 );
//            cvNamedWindow("REG",0);
//            cvShowImage("REG",m_pImgReg);
//            cvWaitKey(0);
        }
        return W;
    };

    virtual void UpdateBlob(int BlobIndex, CvBlob* pBlob, IplImage* pImg, IplImage* /*pImgFG*/ = NULL)
    {
        DefBlobTrackerL*    pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIndex);
        if(pF)
        {
            pF->pTracker->Update(pBlob?pBlob:&(pF->blob),pImg,m_pImgFG);
        }
    };

    int     GetBlobNum(){return m_BlobTrackerList.GetBlobNum();};
    CvBlob* GetBlob(int index){return m_BlobTrackerList.GetBlob(index);};

    void  SetBlob(int BlobIndex, CvBlob* pBlob)
    {
        CvBlob* pB = m_BlobTrackerList.GetBlob(BlobIndex);
        if(pB)
        {
            pB[0] = pBlob[0];
            pB->w = MAX(CV_BLOB_MINW, pBlob->w);
            pB->h = MAX(CV_BLOB_MINH, pBlob->h);
        }
    }

    void    Release(){delete this;};

    /* Additional functionality: */
    CvBlob* GetBlobByID(int BlobID){return m_BlobTrackerList.GetBlobByID(BlobID);}

    /*  ===============  MULTI HYPOTHESIS INTERFACE ==================  */
    /* Return number of position hypotheses of currently tracked blob: */
    virtual int     GetBlobHypNum(int BlobIdx)
    {
        DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIdx);
        assert(pF->pBlobHyp);
        return pF->pBlobHyp->GetBlobNum();
    };  /* CvBlobtrackerList::GetBlobHypNum() */

    /* Return pointer to specified blob hypothesis by index blob: */
    virtual CvBlob* GetBlobHyp(int BlobIndex, int hypothesis)
    {
        DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIndex);
        assert(pF->pBlobHyp);
        return pF->pBlobHyp->GetBlob(hypothesis);
    };  /* CvBlobtrackerList::GetBlobHyp() */

    /* Set new parameters for specified (by index) blob hyp (can be called several times for each hyp )*/
    virtual void    SetBlobHyp(int BlobIndex, CvBlob* pBlob)
    {
        if(m_ClearHyp)
        {   /* Clear all hypotheses: */
            int b, bN = m_BlobTrackerList.GetBlobNum();
            for(b=0; b<bN; ++b)
            {
                DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(b);
                assert(pF->pBlobHyp);
                pF->pBlobHyp->Clear();
            }
            m_ClearHyp = 0;
        }
        {   /* Add hypothesis: */
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(BlobIndex);
            assert(pF->pBlobHyp);
            pF->pBlobHyp->AddBlob(pBlob);
        }
    };  /* CvBlobtrackerList::SetBlobHyp */

private:
public:
    void ParamUpdate()
    {
        int i;
        for(i=m_BlobTrackerList.GetBlobNum(); i>0; --i)
        {
            DefBlobTrackerL* pF = (DefBlobTrackerL*)m_BlobTrackerList.GetBlob(i-1);
            TransferParamsToChild(pF->pTracker);
            pF->pTracker->ParamUpdate();
        }
    }
};  /* CvBlobTrackerList */

CvBlobTracker* cvCreateBlobTrackerList(CvBlobTrackerOne* (*create)())
{
    return (CvBlobTracker*) new CvBlobTrackerList(create);
}
