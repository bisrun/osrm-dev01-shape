// GRect.h: interface for the GRect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRECT_H__4722E726_6C98_4080_AB97_DBE42FB08980__INCLUDED_)
#define AFX_GRECT_H__4722E726_6C98_4080_AB97_DBE42FB08980__INCLUDED_


#include "geoshape/GPoint.hpp"

class GSize;
class GPoint;
struct GPOINT;


class GRect  
{
public:
	GRect();
	// from left, top, right, and bottom
	GRect(double l, double t, double r, double b);
	// copy constructor
	GRect(const GRect& srcRect);
	// from a pointer to another rect
	GRect(GRect* lpSrcRect);
	virtual ~GRect();

	double Height( ) const;
	double Width( ) const;
	
	GPoint CenterPoint(void) const ;
	void SetRect(double x1, double y1, double x2, double y2);
	void SetRect(GPoint TR, GPoint BL);
	void SetRect(GRect& r);
	GRect operator +(GPoint p) const;
	void operator +=(GPoint p);
	void operator +=(const GSize& s);
	void operator -=(GPoint p);
	void operator =(const GRect &gr);


	bool IsIntersect(const GRect& gr);
public:
	void NormalizeRect();
	GPoint BottomRight();
	GPoint TopLeft();
	void UnionRect(GRect* gr1, const GRect& gr2);
	void IntersectRect(GRect* gr1, const GRect& gr2);
	bool PTInRect (GPoint pt);


	double    left;
    double    top;
    double    right;
    double    bottom;
//public:
//	MEM_MGMT(GRect);
};
struct GRECT
{
	double left;
	double bottom;
	double	width;
	double	height;
	double	top(){return bottom+height;}
	double	right(){return left+width;}
};


struct GRECT2
{
	double  left;
	double  bottom;
	double	right;
	double	top;


	bool IsTouch( GRECT2 &kRect );
	bool IsTouch ( GPOINT & pt);
	bool IsTouch ( double x, double y);
	bool RectInclude(  GRECT2 &kRectChild );
	void set(double x1, double y1,double  x2, double y2){left = x1; bottom = y1; right=x2 ; top = y2;}
	void operator =(const GRect &gr){left = gr.left; bottom =gr.bottom ; right=gr.right ; top = gr.top ;}
    void UnionRect( const GRECT2 &gr2);
	void AddBuffer( double dfBufferLen );

};

#endif // !defined(AFX_GRECT_H__4722E726_6C98_4080_AB97_DBE42FB08980__INCLUDED_)
