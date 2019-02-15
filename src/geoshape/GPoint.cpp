// GPoint.cpp: implementation of the GPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "geoshape/GPoint.hpp"
#include "geoshape/GSize.hpp"
#include <math.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GPoint::GPoint()
{
	x = 0;
	y = 0;
}

GPoint::GPoint(double _x, double _y)
{
	x = _x;
	y = _y;
}

void GPoint::operator +=(const GSize &s)
{
	x += s.cx;
	y += s.cy;
}


double GPoint::Distance(const GPoint &point)
{
	return sqrt(pow(x-point.x, 2.0) + pow(y-point.y, 2.0) );
}



