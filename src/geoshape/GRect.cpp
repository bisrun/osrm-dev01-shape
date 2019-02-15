// GRect.cpp: implementation of the GRect class.
//
//////////////////////////////////////////////////////////////////////

#include "geoshape/GRect.hpp"
#include "geoshape/GSize.hpp"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

GRect::GRect()
{
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
}

// from left, top, right, and bottom
GRect::GRect(double l, double t, double r, double b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

GRect::~GRect()
{

}

// copy constructor
GRect::GRect(const GRect& srcRect)
{
	left = srcRect.left;
	top = srcRect.top;
	right = srcRect.right;
	bottom = srcRect.bottom;
}

// from a pointer to another rect
GRect::GRect(GRect* lpSrcRect)
{
	left = (lpSrcRect->left);
	top = (lpSrcRect->top);
	right = (lpSrcRect->right);
	bottom = (lpSrcRect->bottom);
}
	
double GRect::Width() const
{
	return right - left;	
}

double GRect::Height() const
{
	double h = bottom - top;
	if (h<0)
		h *= -1;
	return h;
}
	
GPoint GRect::CenterPoint(void) const 
{
	GPoint p((right+left)/2, (bottom+top)/2);
	return p;
}


void GRect::SetRect(double x1, double y1, double x2, double y2)
{
	left = x1;
	right = x2;
	if(y1>y2)
	{
		top = y1;
		bottom = y2;
	}
	else
	{
		top = y2;
		bottom = y1;
	}
}

void GRect::SetRect(GPoint TL, GPoint BR)
{
	left = TL.x;
	//top = BR.y;
	top = TL.y;
	right = BR.x;
	//bottom = TL.y;
	bottom = BR.y;
}

void GRect::SetRect(GRect& r)
{
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
}

GRect GRect::operator + (GPoint p) const
{
	GRect r((GRect*)this);
	r.left += p.x;
	r.top  += p.y;
	r.right += p.x;
	r.bottom += p.y;
	return r;
}

void GRect::operator +=(GPoint p)
{
	left += p.x;
	top  += p.y;
	right += p.x;
	bottom += p.y;
}

void GRect::operator +=(const GSize& s)
{
	left += s.cx;
	top  += s.cy;
	right += s.cx;
	bottom += s.cy;
}

void GRect::operator -=(GPoint p)
{
	left -= p.x;
	top  -= p.y;
	right -= p.x;
	bottom -= p.y;
}

void GRect::operator =(const GRect &gr)
{
	left	= gr.left;
	top		= gr.top;
	right	= gr.right;
	bottom	= gr.bottom;
}



void GRect::UnionRect(GRect *gr1, const GRect &gr2)
{
	gr1->left	= (gr1->left	< gr2.left)		? gr1->left		: gr2.left;
	gr1->right	= (gr1->right	> gr2.right)	? gr1->right	: gr2.right;
	gr1->top	= (gr1->top		> gr2.top)		? gr1->top		: gr2.top;
	gr1->bottom	= (gr1->bottom	< gr2.bottom)	? gr1->bottom	: gr2.bottom;
}

void GRect::IntersectRect(GRect* gr1, const GRect& gr2)
{
	gr1->left	= (gr1->left	< gr2.left)		? gr2.left		: gr1->left;
	gr1->right	= (gr1->right	> gr2.right)	? gr2.right		: gr1->right;
	gr1->top	= (gr1->top		> gr2.top)		? gr2.top		: gr1->top;
	gr1->bottom	= (gr1->bottom	< gr2.bottom)	? gr2.bottom	: gr1->bottom;
}

bool GRect::PTInRect (GPoint pt)
{
	if (left <= pt.x && right >= pt.x &&
		bottom <= pt.y && top >= pt.y)
	{
		return true;
	}

	return false;
}

GPoint GRect::TopLeft()
{
	return GPoint(left, top);
}

GPoint GRect::BottomRight()
{
	return GPoint(right, bottom);
}

void GRect::NormalizeRect()
{
	//double t;
	if (left > right)
		std::swap(left, right);
	
    if (top > bottom)
		std::swap(top, bottom);
}

bool GRect::IsIntersect(const GRect& gr)
{
	return (left <= gr.right
		&& right >= gr.left
		&& top >= gr.bottom
		&& bottom <= gr.top
		);
}



bool	GRECT2::IsTouch( GRECT2 &kRect )
{
	bool mx =false , my =false ;

	if( (left <= kRect.left &&  kRect.left <= right) 
		|| (left <= kRect.right &&  kRect.right <= right) 
		||(kRect.left <= left &&  left <= kRect.right) 
		|| (kRect.left <= right &&  right <= kRect.right) 
		) 
	{
		mx =true ;

	}

	if( (bottom <= kRect.bottom &&  kRect.bottom <= top) 
		|| (bottom <= kRect.top &&  kRect.top <= top) 
		||(kRect.bottom <= bottom &&  bottom <= kRect.top) 
		|| (kRect.bottom <= top &&  top <= kRect.top) 
		) 
	{
		my = true ;
	}

	if( mx && my )
	{
		return true ;
	}
	return false ;
}

bool GRECT2::IsTouch ( GPOINT & pt)
{
	if (left <= pt.x && right >= pt.x &&
		bottom <= pt.y && top >= pt.y)
	{
		return true;
	}

	return false;
}

bool GRECT2::IsTouch (double x, double y)
{
	if (left <= x && right >= x &&
		bottom <= y && top >= y)
	{
		return true;
	}

	return false;
}

bool	GRECT2::RectInclude( GRECT2 &kRectChild )
{
	if( (left <= kRectChild.left &&  kRectChild.left <= right) 
		&& (left <= kRectChild.right &&  kRectChild.right <= right) 
		&& (bottom <= kRectChild.bottom &&  kRectChild.bottom <= top) 
		&& (bottom <= kRectChild.top &&  kRectChild.top <= top) 
		) 
	{
		return true ;

	}
	return false ;
}

void GRECT2::UnionRect( const GRECT2 &gr2)
{
	left	= (left		< gr2.left)		? left		: gr2.left;
	right	= (right	> gr2.right)	? right		: gr2.right;
	top		= (top		> gr2.top)		? top		: gr2.top;
	bottom	= (bottom	< gr2.bottom)	? bottom	: gr2.bottom;
}

void GRECT2::AddBuffer( double dfBufferLen )
{
	left = left - dfBufferLen;
	right = right + dfBufferLen;
	bottom = bottom - dfBufferLen;
	top = top + dfBufferLen;
}
