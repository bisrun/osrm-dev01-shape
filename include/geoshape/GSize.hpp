// GSize.h: interface for the GSize class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GSIZE_H__4A8EFC38_D421_482A_B96B_D9F93DE3271E__INCLUDED_)
#define AFX_GSIZE_H__4A8EFC38_D421_482A_B96B_D9F93DE3271E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "geoshape/GPoint.hpp"
#include "geoshape/GRect.hpp"

class GSize  
{
public:

// Constructors
	// construct an uninitialized size
	GSize();
	// create from two integers
	GSize(double initCX, double initCY);
	// create from another size
	GSize(const GSize& initSize);
	// create from a point
	GSize(const GPoint& initPt);
	// create from a DWORD: cx = LOWORD(dw) cy = HIWORD(dw)
	//GSize(DWORD dwSize);

// Operations
	bool operator==(const GSize& size) const;
	bool operator!=(const GSize&  size) const;
	void operator+=(const GSize&  size);
	void operator-=(const GSize&  size);

// Operators returning GSize values
	GSize operator+(const GSize&  size) const;
	GSize operator-(const GSize&  size) const;
	GSize operator-() const;

// Operators returning CPoint values
	GPoint operator+(const GPoint& point) const;
	GPoint operator-(const GPoint& point) const;

// Operators returning CRect values
	GRect operator+(const GRect& rect) const;
	GRect operator-(const GRect& rect) const;

public:
	double cx;
	double cy;
};

#endif // !defined(AFX_GSIZE_H__4A8EFC38_D421_482A_B96B_D9F93DE3271E__INCLUDED_)
