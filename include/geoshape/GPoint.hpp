// GPoint.h: interface for the GPoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GPOINT_H__EE7622A7_5950_4F5D_B24C_4E7D7EDC22B9__INCLUDED_)
#define AFX_GPOINT_H__EE7622A7_5950_4F5D_B24C_4E7D7EDC22B9__INCLUDED_

#pragma once
#include <string.h>
#include <math.h>


class GSize;
#ifndef PI
#define PI            3.14159265358979323846
#endif

struct GPOINT
{
	double x;
	double y;
	double distance(GPOINT pt){return sqrt((x-pt.x)*(x-pt.x)+(y-pt.y)*(y-pt.y));}
	static double distance(double px, double py, double px2, double py2){return sqrt((px2-px)*(px2-px)+(py2-py)*(py2-py));}

	void	set( double x1, double y1){x =x1 ; y = y1; }
	GPOINT(){clear();}
	GPOINT(double x, double y) { this->x = x; this->y = y; }
	void clear(){x=0;y=0;}
};
class GPoint  
{
public:
	double Distance(const GPoint& point);
	GPoint();
	GPoint(double _x, double _y);

	double x;
	double y;

	
	void operator +=(const GSize& s);
	void operator -=(GPoint p){x-=p.x; y-=p.y;};
	bool operator ==(const GPoint& p){return (x==p.x && y==p.y);};
	bool operator !=(const GPoint& p){return (x!=p.x || y!=p.y);;};
	//MEM_MGMT(GPoint);  //use macro LEDA_MEMORY

	friend GPoint operator-(const GPoint& p1, const GPoint& p2)
	{return GPoint(p1.x - p2.x, p1.y - p2.y);};

	//void Serialize(CArchive& ar);
};

struct GVECTOR
{
	double vx;
	double vy;	
	GVECTOR(){};
	GVECTOR(GPOINT s_pt, GPOINT e_pt){ vx = e_pt.x - s_pt.x ; vy = e_pt.y - s_pt.y; }
	GVECTOR(double x, double y ){ vx =x ;vy = y; }
	void	SetVector( GPOINT s_pt, GPOINT e_pt ){ vx = e_pt.x - s_pt.x ; vy = e_pt.y - s_pt.y; }
	void	SetVector( double x, double y){vx = x ; vy = y;}
	GVECTOR	GetNormal() { GVECTOR norm ; double len = length();  norm.SetVector( vx/len, vy/len); return norm;}
	double	InnerProduct(GVECTOR vec){ return vx*vec.vx + vy*vec.vy ;}
	double	OuterProduct(GVECTOR vec){ return vx*vec.vy - vy*vec.vx ;}
	//double	distance(GPOINT pt){return sqrt((x-pt.x)*(x-pt.x)+(y-pt.y)*(y-pt.y))};
	double	length(){return sqrt( vx*vx + vy*vy);}
	double	Cos(GVECTOR vec){ return InnerProduct(vec) / (length()*vec.length());}
	double	Sin(GVECTOR vec){ return OuterProduct(vec) / (length()*vec.length());}
	static double  NorthBasedAngle( GVECTOR * vector )
	{
		double ang = atan( vector->vy/vector->vx )*(180/PI);
		double ang2 = 90 - ang + 360;
		if( vector->vx < 0 )
		{
			ang2 += 180.0;
		}

		double ang3 =  (int)ang2 % 360;
		return ang3;
	}
	static void	SetUnitLine( double dfUnitLen , double x, double y , double &x2, double &y2)
	{
		double len = GPOINT::distance(x,y, x2, y2);
		x2=  x + (x2-x)*(dfUnitLen/len);
		y2=  y + (y2-y)*(dfUnitLen/len);
	}
};

struct STPtOnLine
{
	GPOINT ptVtxStart;
	GPOINT ptVtxEnd;
	GPOINT ptOverLine;
	GPOINT ptOnLine;

	double dfLenLine;
	double dfLen_PtOverLine2PtOnLine;
	double dfLen_PtVtxStart2PtOnLine;
	double dfLenEtc;
	double dfMatchLinkAngle; // 
	unsigned char	cDirPtOverLine; //
	unsigned char  dummy[3];

	STPtOnLine(){clear();}
	void clear(){memset(this, 0x00, sizeof(STPtOnLine));}
};
#endif // !defined(AFX_GPOINT_H__EE7622A7_5950_4F5D_B24C_4E7D7EDC22A9__INCLUDED_)
