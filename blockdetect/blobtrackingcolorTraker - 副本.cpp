#include "precomp.hpp"

/*ʹ�� colortracker������


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
	CvBlobSeq       m_BlobList;//��������Ҫ���ٵ��ſ������
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
	{//ɾ�����ȵõ�ָ�룬ɾ��Ԥ���࣬��ɾ���Լ�
		//���� һֱ�� DefBlobTracker *��CvBlob* ���ã�����
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
		//���ſ������ǰ��ֱ����ͣ���ƽ��
		m_BlobList.AddBlob((CvBlob*)&NewB);
		return m_BlobList.GetBlob(m_BlobList.GetBlobNum() - 1);
	};

	virtual void    Process(IplImage* pImg, IplImage* pImgFG = NULL)
	{//��һ������ǰ������ȡ�ſ飺λ��+��С ==�����浽m_BlobListNew������
		//�ڶ�����Ԥ���ſ�λ�� ���µ����Ե�BlobPredict��
		//�����������ÿ��Ŀ�����ײ
		//�Ѿ����ٵ��ſ������ �໥ƥ��Ԥ��λ�� �͵�ǰλ�á�����غ��ˣ��ͽ�Collision=1 
		//���Ĳ��� ����ÿ��Ŀ�꣬ �������ܵ�λ�� ���µ��ſ�����Ƚϣ�
		//�������һ�� ��Ϊ���µĿ���λ�á�


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

		//��һ������ǰ������ȡ�ſ飺λ��+��С ==�����浽m_BlobListNew������



		//�ڶ�����Ԥ���ſ�λ�� ���µ����Ե�BlobPredict��
		for (i = m_BlobList.GetBlobNum(); i>0; --i)
		{   /* Predict new blob position: */

			CvBlob*         pB = NULL;
			DefBlobTrackerColorTracker* pBT = (DefBlobTrackerColorTracker*)m_BlobList.GetBlob(i - 1);

			/* Update predictor by previous value of blob: */
			//������һ֡��λ��
			pBT->pPredictor->Update(&(pBT->blob));

			//Ԥ����һ֡λ��
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

		//�����������ÿ��Ŀ�����ײ
		//�Ѿ����ٵ��ſ������ �໥ƥ��Ԥ��λ�� �͵�ǰλ�á�����غ��ˣ��ͽ�Collision=1 
		if (m_Collision)//�����Ҫ������ײ
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

				//�����ſ�Ԥ��λ���Ƿ���ײ��
				if (fabs(pB1->x - pB2->x)<0.6*(pB1->w + pB2->w) &&
					fabs(pB1->y - pB2->y)<0.6*(pB1->h + pB2->h)) Collision = 1;

				pB1 = &pF->blob;
				pB2 = &pF2->blob;

				//��ǰλ���Ƿ���ײ��
				if (fabs(pB1->x - pB2->x)<0.6*(pB1->w + pB2->w) &&
					fabs(pB1->y - pB2->y)<0.6*(pB1->h + pB2->h)) Collision = 1;

				if (Collision) break;

			}   /* Check next blob to cross current. */

			pF->Collision = Collision;

		}   /* Predict collision. */


		//���Ĳ��� ����ÿ��Ŀ�꣬ �������ܵ�λ�� ���µ��ſ�����Ƚϣ�
		//�������һ�� ��Ϊ���µĿ���λ�á�
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

			if (pBT->pBlobHyp->GetBlobNum()>0)//pBlobHyp�ѵ���ÿ��Ŀ����ܵ���һ��λ�ã�����
			{   /* Track all hypotheses: */

				int h, hN = pBT->pBlobHyp->GetBlobNum();
				for (h = 0; h<hN; ++h)
				{//��ȡÿһ�����ܵ�λ�� pBlobHyp��
					//�͵�ǰ��ǰ���ſ�����m_BlobListNew �Ƚϣ��������ſ飬
					//������ſ�ֵ==����ֵ��pBlobHyp
					//�������ſ�Ҳ��Զ��ɾ�� �������
					int         j, jN = m_BlobListNew.GetBlobNum();
					CvBlob*     pB = pBT->pBlobHyp->GetBlob(h);//���ܵ�λ��
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
	{/*������ �Ѹ���Ŀ���index ��Ŀ����ſ�
	 �����������ײ�� ����ǰ���ſ������� ������ģ�����
	 ������ײ�� �ſ�λ��= Ԥ��ֵ
	 */
		//pBlob �� ���Ŀ��Id��Ӧ���ſ�  BlobIndex������INdex

		//pB���ڲ�cc�� �Լ���Ŀ���ſ�
		//pBT�� �ڲ��ſ�������ʾ
		int             ID = pBlob->ID;
		CvBlob*         pB = m_BlobList.GetBlob(BlobIndex);
		DefBlobTrackerColorTracker* pBT = (DefBlobTrackerColorTracker*)pB;
		//CvBlob*         pBBest = NULL;
		//double          DistBest = -1;
		int             BlobID;

		if (pB == NULL) return;

		BlobID = pB->ID;//����ID ͬ��ͬ��

		//���ڿ�����ײ�ģ������ſ鷢������ײ �� �ſ�λ��=�ſ��Ԥ��λ��
		if (m_Collision && pBT->Collision)
		{   /* Tracking in collision: */
			pB[0] = pBT->BlobPredict;//��Ԥ���ֵ��ֵ���ſ��λ��
			CV_BLOB_ID(pB) = BlobID;
		}   /* Tracking in collision. */
		else
		{   /* Non-collision tracking: */
			CvBlob* pBBest = GetNearestBlob(pB);//��PB�����ſ�Ƚ� �����������ſ�

			if (pBBest)
			{
				float   w = pBlob->w*(1 - m_AlphaSize) + m_AlphaSize*pBBest->w;
				float   h = pBlob->h*(1 - m_AlphaSize) + m_AlphaSize*pBBest->h;
				float   x = pBlob->x*(1 - m_AlphaPos) + m_AlphaPos*pBBest->x;
				float   y = pBlob->y*(1 - m_AlphaPos) + m_AlphaPos*pBBest->y;
				//���������� Ŀ���ſ�Ĵ�С��λ��
				pB->w = w;
				pB->h = h;
				pB->x = x;
				pB->y = y;
				CV_BLOB_ID(pB) = BlobID;
			}//���������ƥ��ֻ�ܴ���û����ײ��Ŀ�����
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
{//���ش������һ��ָ��
	return (CvBlobTracker*) new CvBlobTrackerColorTracker;
}
/*============== BLOB TRACKERCC CLASS DECLARATION =============== */
