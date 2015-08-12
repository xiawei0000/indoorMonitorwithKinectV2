#ifndef __OPENCV_PRECOMP_H__
#define __OPENCV_PRECOMP_H__


#include "legacy.hpp"

#include "blobtrack.hpp"
#include "compat.hpp"

#include "_matrix.h"
#include "opencv2/core/internal.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/video/background_segm.hpp"


typedef unsigned short ushort;

//包含头文件， 声明和实现了 两个cvsize 等于 不等于的操作符重载函数
CV_INLINE bool operator == (CvSize size1, CvSize size2 );
CV_INLINE bool operator == (CvSize size1, CvSize size2 )
{
    return size1.width == size2.width && size1.height == size2.height;
}

CV_INLINE bool operator != (CvSize size1, CvSize size2 );
CV_INLINE bool operator != (CvSize size1, CvSize size2 )
{
    return size1.width != size2.width || size1.height != size2.height;
}

#endif /* __CVAUX_H__ */
