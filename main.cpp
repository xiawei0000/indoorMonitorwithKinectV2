#include "MyKinect.h"


//������
int _tmain()//realse����Ҫ�����tmain����
{
	MyKinect kinect;
	kinect.InitKinect();
	while (1)
	{
		kinect.Update();
		if (waitKey(1) >= 0)//����������˳�
		{
			break;
		}
	}

	return 0;
}