#include "MyKinect.h"


//主函数
int _tmain()//realse必须要用这个tmain才行
{
	MyKinect kinect;
	kinect.InitKinect();
	while (1)
	{
		kinect.Update();
		if (waitKey(1) >= 0)//按下任意键退出
		{
			break;
		}
	}

	return 0;
}