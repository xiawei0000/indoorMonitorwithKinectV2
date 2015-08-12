#include "precomp.hpp"

/*使用 colortracker来跟踪


*/

/*============== BLOB TRACKERCC CLASS DECLARATION =============== */
typedef struct DefBlobTrackerColorTracker
{
	CvBlob                      blob;
	CvBlobTrackPredictor*       pPredictor;
	CvBlob                      BlobPredict;
	int                         Collision;
//	CvBlobSeq*                  pBlobHyp;
//	float                       AverFG;
} DefBlobTrackerColorTracker;


class CvBlobTrackerColorTracker : public CvBlobTracker
{
private:
	float           m_AlphaSize;
	float           m_AlphaPos;
	float           m_Alpha;
	int             m_Collision;
	int             m_ConfidenceType;
	const char*           m_ConfidenceTypeStr;
	CvBlobSeq       m_BlobList;//保存所有要跟踪的团块的链表
	CvBlobSeq       m_BlobListNew;
	//  int             m_LastID;
	CvMemStorage*   m_pMem;
	int             m_ClearHyp;
	IplImage*       m_pImg;
	IplImage*       m_pImgFG;
public:
	CvBlobTrackerColorTracker() :m_BlobList(sizeof(DefBlobTrackerColorTracker))
	{
		m_pMem = cvCreateMemStorage();
		m_Collision = 1; /* if 1 then collistion will be detected and processed */
		AddParam("Collision", &m_Collision);
		CommentParam("Collision", "If 1 then collision cases are processed in special way");
	};

	~CvBlobTrackerColorTracker()
	{
		if (m_pMem)cvReleaseMemStorage(&m_pMem);
	};

	/* Blob functions: */
	virtual int     GetBlobNum() { return m_BlobList.GetBlobNum(); };
	virtual CvBlob* GetBlob(int BlobIndex){ return m_BlobList.GetBlob(BlobIndex); };
	virtual void    SetBlob(int BlobIndex, CvBlob* pBlob)
	{
		CvBlob* pB = m_BlobList.GetBlob(BlobIndex);
		if (pB) pB[0] = pBlob[0];
	};

	virtual CvBlob* GetBlobByID(int BlobID){ return m_BlobList.GetBlobByID(BlobID); };
	virtual void    DelBlob(int BlobIndex)
	{//删除，先得到指针，删除预测类，再删除自己
		//问题 一直将 DefBlobTracker *和CvBlob* 混用？？？
		DefBlobTrackerColorTracker* pBT = (DefBlobTrackerColorTracker*)m_BlobList.GetBlob(BlobIndex);
		if (pBT == NULL) return;
		if (pBT->pPredictor)
		{
			pBT->pPredictor->Release();
		}
		else
		{
			printf("WARNING!!! Invalid Predictor in CC tracker");
		}
		delete pBT->pBlobHyp;
		m_BlobList.DelBlob(BlobIndex);
	};
#if 0
	virtual void    DelBlobByID(int BlobID)
	{
		DefBlobTracker* pBT = (DefBlobTracker*)m_BlobList.GetBlobByID(BlobID);
		pBT->pPredictor->Release();
		delete pBT->pBlobHyp;
		m_BlobList.DelBlobByID(BlobID);
	};
#endif
	virtual void    Release(){ delete this; };

	/* Add new blob to track it and assign to this blob personal ID */
	/* pBlob - pinter to structure with blob parameters (ID is ignored)*/
	/* pImg - current image */
	/* pImgFG - current foreground mask */
	/* return pointer to new added blob */
	virtual CvBlob* AddBlob(CvBlob* pB, IplImage* /*pImg*/, IplImage* pImgFG = NULL)
	{
		assert(pImgFG); /* This tracker uses only foreground mask. */
		DefBlobTrackerColorTracker NewB;
		NewB.blob = pB[0];
		//        CV_BLOB_ID(&NewB) = m_LastID;
		NewB.pBlobHyp = new CvBlobSeq;
		NewB.pPredictor = cvCreateModuleBlobTrackPredictKalman(); /* Module for position prediction. */
		NewB.pPredictor->Update(pB);
		NewB.AverFG = pImgFG ? CalcAverageMask(pB, pImgFG) : 0;
		//对团块的区域前景直接求和，求平均
		m_BlobList.AddBlob((CvBlob*)&NewB);
		return m_BlobList.GetBlob(m_BlobList.GetBlobNum() - 1);
	};

	virtual void    Process(IplImage* pImg, IplImage* pImgFG = NULL)
	{//第一步：从前景里提取团块：位置+大小 ==》保存到m_BlobListNew链表里
		//第二步：预测团块位置 更新到各自的BlobPredict里
		//第三步：标记每个目标的碰撞
		//已经跟踪的团块链表里， 相互匹配预测位置 和当前位置。如果重合了，就将Collision=1 
		//第四步： 对于每个目标， 用他可能的位置 和新的团块链表比较，
		//求最近的一个 作为他新的可能位置。


		CvSeq*      cnts;
		CvSeq*      cnt;
		int i;

		m_pImg = pImg;
		m_pImgFG = pImgFG;

		if (m_BlobList.GetBlobNum() <= 0) return;

		/* Clear bloblist for new blobs: */
		m_BlobListNew.Clear();

		assert(m_pMem);
		cvClearMemStorage(m_pMem);
		assert(pImgFG);

		//第一步：从前景里提取团块：位置+大小 ==》保存到m_BlobListNew链表里



		//第二步：预测团块位置 更新到各自的BlobPredict里
		for (i = m_BlobList.GetBlobNum(); i>0; --i)
		{   /* Predict new blob position: */

			CvBlob*         pB = NULL;
			DefBlobTrackerColorTracker* pBT = (DefBlobTrackerColorTracker*)m_BlobList.GetBlob(i - 1);

			/* Update predictor by previous value of blob: */
			//更新上一帧的位置
			pBT->pPredictor->Update(&(pBT->blob));

			//预测下一帧位置
			/* Predict current position: */
			pB = pBT->pPredictor->Predict();

			if (pB)
			{
				pBT->BlobPredict = pB[0];
			}
			else
			{
				pBT->BlobPredict = pBT->blob;
			}
		}   /* Predict new blob position. */

		//第三步：标记每个目标的碰撞
		//已经跟踪的团块链表里， 相互匹配预测位置 和当前位置。如果重合了，就将Collision=1 
		if (m_Collision)//如果需要考虑碰撞
		for (i = m_BlobList.GetBlobNum(); i>0; --i)
		{   /* Predict collision. */
			int             Collision = 0;
			int             j;
			DefBlobTrackerColorTracker* pF = (DefBlobTrackerColorTracker*)m_BlobList.GetBlob(i - 1);

			for (j = m_BlobList.GetBlobNum(); j>0; --j)
			{   /* Predict collision: */
				CvBlob* pB1;
				CvBlob* pB2;
				DefBlobTrackerColorTracker* pF2 = (DefBlobTrackerColorTracker*)m_BlobList.GetBlob(j - 1);
				if (i == j) continue;
				pB1 = &pF->BlobPredict;
				pB2 = &pF2->BlobPredict;

				//两个团块预测位置是否碰撞了
				if (fabs(pB1->x - pB2->x)<0.6*(pB1->w + pB2->w) &&
					fabs(pB1->y - pB2->y)<0.6*(pB1->h + pB2->h)) Collision = 1;

				pB1 = &pF->blob;
				pB2 = &pF2->blob;

				//当前位置是否碰撞了
				if (fabs(pB1->x - pB2->x)<0.6*(pB1->w + pB2->w) &&
					fabs(pB1->y - pB2->y)<0.6*(pB1->h + pB2->h)) Collision = 1;

				if (Collision) break;

			}   /* Check next blob to cross current. */

			pF->Collision = Collision;

		}   /* Predict collision. */


		//第四步： 对于每个目标， 用他可能的位置 和新的团块链表比较，
		//求最近的一个 作为他新的可能位置。
		for (i = m_BlobList.GetBlobNum(); i>0; --i)
		{   /* Find a neighbour on current frame
			* for each blob from previous frame:
			*/
			CvBlob*         pBl = m_BlobList.GetBlob(i - 1);
			DefBlobTrackerColorTracker* pBT = (DefBlobTrackerColorTracker*)pBl;
			//int             BlobID = CV_BLOB_ID(pB);
			//CvBlob*         pBBest = NULL;
			//double          DistBest = -1;
			//int j;

			if (pBT->pBlobHyp->GetBlobNum()>0)//pBlobHyp难道是每个目标可能的下一个位置？？？
			{   /* Track all hypotheses: */

				int h, hN = pBT->pBlobHyp->GetBlobNum();
				for (h = 0; h<hN; ++h)
				{//提取每一个可能的位置 pBlobHyp，
					//和当前的前景团块链表m_BlobListNew 比较，求得最近团块，
					//将最近团块值==》赋值给pBlobHyp
					//如果最近团块也很远就删除 这个可能
					int         j, jN = m_BlobListNew.GetBlobNum();
					CvBlob*     pB = pBT->pBlobHyp->GetBlob(h);//可能的位置
					int         BlobID = CV_BLOB_ID(pB);
					CvBlob*     pBBest = NULL;
					double      DistBest = -1;
					for (j = 0; j<jN; j++)
					{   /* Find best CC: */
						double  Dist = -1;
						CvBlob* pBNew = m_BlobListNew.GetBlob(j);
						double  dx = fabs(CV_BLOB_X(pB) - CV_BLOB_X(pBNew));
						double  dy = fabs(CV_BLOB_Y(pB) - CV_BLOB_Y(pBNew));
						if (dx > 2 * CV_BLOB_WX(pB) || dy > 2 * CV_BLOB_WY(pB)) continue;

						Dist = sqrt(dx*dx + dy*dy);
						if (Dist < DistBest || pBBest == NULL)
						{
							DistBest = Dist;
							pBBest = pBNew;
						}
					}   /* Find best CC. */

					if (pBBest)
					{
						pB[0] = pBBest[0];
						CV_BLOB_ID(pB) = BlobID;
					}
					else
					{   /* Delete this hypothesis. */
						pBT->pBlobHyp->DelBlob(h);
						h--;
						hN--;
					}
				}   /* Next hypothysis. */
			}   /*  Track all hypotheses. */
		}   /*  Track next blob. */

		m_ClearHyp = 1;

	} /* Process. */

	virtual void ProcessBlob(int BlobIndex, CvBlob* pBlob, IplImage* /*pImg*/, IplImage* /*pImgFG*/ = NULL)
	{/*输入是 已跟踪目标的index 和目标的团块
	 如果不考虑碰撞， 在新前景团块链表总 找最近的，更新
	 考虑碰撞： 团块位置= 预测值
	 */
		//pBlob 是 外界目标Id对应的团块  BlobIndex是他的INdex

		//pB是内部cc类 自己的目标团块
		//pBT是 内部团块的特殊表示
		int             ID = pBlob->ID;
		CvBlob*         pB = m_BlobList.GetBlob(BlobIndex);
		DefBlobTrackerColorTracker* pBT = (DefBlobTrackerColorTracker*)pB;
		//CvBlob*         pBBest = NULL;
		//double          DistBest = -1;
		int             BlobID;

		if (pB == NULL) return;

		BlobID = pB->ID;//看看ID 同不同？

		//对于考虑碰撞的，并且团块发生了碰撞 。 团块位置=团块的预测位置
		if (m_Collision && pBT->Collision)
		{   /* Tracking in collision: */
			pB[0] = pBT->BlobPredict;//将预测的值赋值到团块的位置
			CV_BLOB_ID(pB) = BlobID;
		}   /* Tracking in collision. */
		else
		{   /* Non-collision tracking: */
			CvBlob* pBBest = GetNearestBlob(pB);//用PB和新团块比较 ，获得最近的团块

			if (pBBest)
			{
				float   w = pBlob->w*(1 - m_AlphaSize) + m_AlphaSize*pBBest->w;
				float   h = pBlob->h*(1 - m_AlphaSize) + m_AlphaSize*pBBest->h;
				float   x = pBlob->x*(1 - m_AlphaPos) + m_AlphaPos*pBBest->x;
				float   y = pBlob->y*(1 - m_AlphaPos) + m_AlphaPos*pBBest->y;
				//按比例更新 目标团块的大小和位置
				pB->w = w;
				pB->h = h;
				pB->x = x;
				pB->y = y;
				CV_BLOB_ID(pB) = BlobID;
			}//这种最近邻匹配只能处理没有碰撞的目标跟踪
		}   /* Non-collision tracking. */

		pBlob[0] = pB[0];
		pBlob->ID = ID;
	};

	virtual double  GetConfidence(int BlobIndex, CvBlob* pBlob, IplImage* /*pImg*/, IplImage* pImgFG = NULL)
	{
		return 1.0;
	};

	virtual void UpdateBlob(int BlobIndex, CvBlob* /*pBlob*/, IplImage* /*pImg*/, IplImage* pImgFG = NULL)
	{
		
	};



};

CvBlobTracker* cvCreateBlobTrackerColorTracker()
{//返回创建后的一个指针
	return (CvBlobTracker*) new CvBlobTrackerColorTracker;
}
/*============== BLOB TRACKERCC CLASS DECLARATION =============== */
