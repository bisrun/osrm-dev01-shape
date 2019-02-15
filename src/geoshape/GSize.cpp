// GSize.cpp: implementation of the GSize class.
//
//////////////////////////////////////////////////////////////////////

#include "geoshape/GSize.hpp"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


// Constructors
// construct an uninitialized size
GSize::GSize() : cx(0), cy(0)
{}

// create from two integers
GSize::GSize(double initCX, double initCY) : cx(initCX), cy(initCY)
{}

// create from another size
GSize::GSize(const GSize& initSize)
{
	if(this == &initSize) return;
	cx = initSize.cx;
	cy = initSize.cy;
}
// create from a point
GSize::GSize(const GPoint& initPt) : cx(initPt.x), cy(initPt.y)
{}

// Operations
bool GSize::operator==(const GSize& size) const
{
	return (cx == size.cx && cy == size.cy);
}

bool GSize::operator!=(const GSize&  size) const
{
	return !(operator==(size));
}

void GSize::operator+=(const GSize&  size)
{
	cx += size.cx;
	cy += size.cy;
}

void GSize::operator-=(const GSize&  size)
{
	cx -= size.cx;
	cy -= size.cy;	
}

// Operators returning GSize values
GSize GSize::operator+(const GSize&  size) const
{
	return GSize(cx+size.cx, cy+size.cy);
}

GSize GSize::operator-(const GSize&  size) const
{
	return GSize(cx-size.cx, cy-size.cy);
}

GSize GSize::operator-() const
{
	return GSize(-cx, -cy);
}

// Operators returning CPoint values
GPoint GSize::operator+(const GPoint& point) const
{
	return GPoint(cx+point.x, cy+point.y);
}

GPoint GSize::operator-(const GPoint& point) const
{
	return GPoint(cx-point.x, cy-point.y);
}

// Operators returning CRect values
GRect GSize::operator+(const GRect& rect) const
{
	return GRect(rect.left-cx, rect.top-cy, rect.right+cx, rect.bottom+cy);
}

GRect GSize::operator-(const GRect& rect) const
{
	return GRect(rect.left+cx, rect.top+cy, rect.right-cx, rect.bottom-cy);
}


