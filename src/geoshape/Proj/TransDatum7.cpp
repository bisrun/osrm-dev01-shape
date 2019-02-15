#include "geoshape/Proj/Projection.h"
#include "geoshape/Proj/ProjDef.h"
#include <math.h>

//
// 7 Parameter Datum Converter
//

// WGS 지심좌표를 사용자 지심좌표로 변환함, Bursa법
// 입력
//    xw,yw,zw : WGS 지심좌표
//    dx, dy, dz, omega, Phi, kappa, ds : 7 Parameter
// 계산
//    xb,yb,zb : 사용자 지심좌표
void TransBursa( double xw, double yw, double zw, double &xb, double &yb, double &zb, 
				 double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XB = (1+ts){R}XW + tx
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    TS     = ds * 0.000001;

	xb = (1 + TS) * (xw + TKAPPA * yw - TPHI * zw)      + dx;
	yb = (1 + TS) * (-1*TKAPPA * xw + yw + TOMEGA * zw) + dy;
	zb = (1 + TS) * (TPHI * xw - TOMEGA * yw + zw)      + dz;
}

void TransBursas( int numPoints, const double *xw, const double *yw, const double *zw, double *xb, double *yb, double *zb, 
				 double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XB = (1+ts){R}XW + tx
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    TS     = ds * 0.000001;

	for( int n=0; n<numPoints; n++ ) {
		xb[n] = (1 + TS) * (xw[n] + TKAPPA * yw[n] - TPHI * zw[n])      + dx;
		yb[n] = (1 + TS) * (-1*TKAPPA * xw[n] + yw[n] + TOMEGA * zw[n]) + dy;
		zb[n] = (1 + TS) * (TPHI * xw[n] - TOMEGA * yw[n] + zw[n])      + dz;
	}
}

// 지심좌표를 사용자 WGS 지심좌표로 역 변환함, Bursa법
// 입력
//   xb,yb,zb : 사용자 지심좌표
// 계산
//   xw,yw,zw : WGS 지심좌표
void InverseBursa( double xb, double yb, double zb, double &xw, double &yw, double &zw,
				 double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XW = 1/(1+ts) {R}^-1 {XB - tx} 
	double xt, yt, zt;
	xt = xb - dx;
	yt = yb - dy;
	zt = zb - dz;

	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    TS     = ds * 0.000001;
   
	xw = 1 / (1 + TS) * (xt - TKAPPA * yt + TPHI * zt);
	yw = 1 / (1 + TS) * (TKAPPA   * xt + yt - TOMEGA * zt);
	zw = 1 / (1 + TS) * (-1 *TPHI * xt + TOMEGA * yt + zt);
}

void InverseBursas( int numPoints, const double *xb, const double *yb, const double *zb, double *xw, double *yw, double *zw,
				 double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XW = 1/(1+ts) {R}^-1 {XB - tx} 
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    TS     = ds * 0.000001;

	for( int n=0; n<numPoints; n++ ) {
		double xt, yt, zt;
		xt = xb[n] - dx;
		yt = yb[n] - dy;
		zt = zb[n] - dz;
   
		xw[n] = 1 / (1 + TS) * (xt - TKAPPA * yt + TPHI * zt);
		yw[n] = 1 / (1 + TS) * (TKAPPA   * xt + yt - TOMEGA * zt);
		zw[n] = 1 / (1 + TS) * (-1 *TPHI * xt + TOMEGA * yt + zt);
	}
}

inline double fnSPHSN( double a, double es, double sphi )
{
	return a / sqrt( 1 - es * sin( sphi )*sin( sphi ) );
}

// GRS80 지심좌표를 사용자 지심좌표로 변환함, Molodensky-Badekas
// 입력
//   xw,yw,zw : GRS 지심좌표
// 계산
//   xb,yb,zb : 사용자 지심좌표
void InverseMolodenskyBadekas( double xw, double yw, double zw, double &xb, double &yb, double &zb, double xa, double ya, double za, double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XW = XA + dX + 1/(1+ts){R}^-1{XB-XA}
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    
	//회전량( PPM : Parts Per Million )
	TS     = ds * 0.000001;

	double xt = xw - xa;
	double yt = yw - ya;
	double zt = zw - za;
	xb = xa - dx + ( 1. / (1. + TS) ) * ( xt - TKAPPA * yt + TPHI * zt );
	yb = ya - dy + ( 1. / (1. + TS) ) * ( TKAPPA   * xt + yt - TOMEGA * zt );
	zb = za - dz + ( 1. / (1. + TS) ) * ( -TPHI * xt + TOMEGA * yt + zt );
}

void InverseMolodenskyBadekas_s( int numPoints, const double *xw, const double *yw, const double *zw, double *xb, double *yb, double *zb, double xa, double ya, double za, double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XW = XA + dX + 1/(1+ts){R}^-1{XB-XA}
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    
	//회전량( PPM : Parts Per Million )
	TS     = ds * 0.000001;

	for( int n=0; n<numPoints; n++ ) {
		double xt = xw[n] - xa;
		double yt = yw[n] - ya;
		double zt = zw[n] - za;
		xb[n] = xa - dx + ( 1. / (1. + TS) ) * (xt - TKAPPA * yt + TPHI * zt);
		yb[n] = ya - dy + ( 1. / (1. + TS) ) * (TKAPPA   * xt + yt - TOMEGA * zt);
		zb[n] = za - dz + ( 1. / (1. + TS) ) * (-1 *TPHI * xt + TOMEGA * yt + zt);
	}
}

// 사용자 지심좌표를 GRS80 지심좌표로 변환함, Molodensky-Badekas
// 입력
//   xb,yb,zb : 사용자 지심좌표
// 계산
//   xw,yw,zw : GRS 지심좌표
void ForwordMolodenskyBadekas( double xb, double yb, double zb, double &xw, double &yw, double &zw, double xa, double ya, double za, double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XW = XA + dX + (1+ts){R}{XB-XA}
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    TS     = ds * 0.000001;

	double tx = xb - xa;
	double ty = yb - ya;
	double tz = zb - za;
	xw = xa + dx + (1 + TS) * (tx + TKAPPA * ty - TPHI * tz);
	yw = ya + dy + (1 + TS) * (-1*TKAPPA * tx + ty + TOMEGA * tz);
	zw = za + dz + (1 + TS) * (TPHI * tx - TOMEGA * ty + tz);
}

void ForwordMolodenskyBadekas_s( int numPoints, const double *xb, const double *yb, const double *zb, double *xw, double *yw, double *zw, double xa, double ya, double za, double dx, double dy, double dz, double omega, double Phi, double kappa, double ds )
{
	// XW = XA + dX + (1+ts){R}{XB-XA}
	double TKAPPA, TOMEGA, TPHI, TS;
	TOMEGA = I_GPN2RAD( omega / 3600 );
    TPHI   = I_GPN2RAD( Phi   / 3600 );
    TKAPPA = I_GPN2RAD( kappa / 3600 );
    TS     = ds * 0.000001;

	for( int n=0; n<numPoints; n++ ) {
		double tx = xb[n] - xa;
		double ty = yb[n] - ya;
		double tz = zb[n] - za;
		xw[n] = xa + dx + (1 + TS) * (tx + TKAPPA * ty - TPHI * tz);
		yw[n] = ya + dy + (1 + TS) * (-1*TKAPPA * tx + ty + TOMEGA * tz);
		zw[n] = za + dz + (1 + TS) * (TPHI * tx - TOMEGA * ty + tz);
	}
}

//
// 경위도를 지심좌표로 바꿈
// 입력 parameter
// phi  : 위도
// lam  : 경도
// a    : 장반경
// f    : 편평율 (1/299.....)
// 계산 결과
//  x, y, z : 지심좌표  x(north), y(east), z
//
//
void GP2CTR( double Phi, double Lam, double h, double a, double f, double &x, double &y, double &z )
{
	double sphi, slam, recf, b, es, n;
   
    //double degrad = atan(1.0) / 45.0;
	sphi   = I_GPN2RAD( Phi );
	slam   = I_GPN2RAD( Lam );

	//단반경 b#, 횡곡률 반경 N의 계산
	//** SEMI MAJOR AXIS - B **
	recf = 1 / f;
	b    = a * (recf - 1) / recf;
	es   = (a * a - b * b ) / ( a * a );
    
	//횡곡률 반경
	n    = fnSPHSN( a, es, sphi );
   
	//'x, y, z의 계산
	x = (n + h) * cos(sphi) * cos(slam);
	y = (n + h) * cos(sphi) * sin(slam);
	z = ( (b*b) / (a*a) * n + h) * sin(sphi);
}

void GP2CTRs( int numPoints, const double *Phi, const double *Lam, const double *h, double a, double f, double *x, double *y, double *z )
{
	double sphi, slam, recf, b, es, nn;
   
    //double degrad = atan(1.0) / 45.0;

	//단반경 b#, 횡곡률 반경 N의 계산
	//** SEMI MAJOR AXIS - B **
	recf = 1 / f;
	b    = a * (recf - 1) / recf;
	es   = (a * a - b * b ) / ( a * a );

	for( int n=0; n<numPoints; n++ ) {
		sphi   = I_GPN2RAD( Phi[n] );
		slam   = I_GPN2RAD( Lam[n] );
    
		//횡곡률 반경
		nn    = fnSPHSN( a, es, sphi );
   
		//'x, y, z의 계산
		x[n] = (nn + h[n]) * cos(sphi) * cos(slam);
		y[n] = (nn + h[n]) * cos(sphi) * sin(slam);
		z[n] = ( (b*b) / (a*a) * nn + h[n]) * sin(sphi);
	}
}


// 지심좌표를 경위도 좌표로 바꿈
// 입력 parameter
// x, y, z : 지심좌표  x(north), y(east), z
// a    : 장반경
// f    : 편평율 (1/299.....)
// 계산 결과
//   phi#  : 위도
//   lam#  : 경도
void CTR2GP( double x, double y, double z, double a, double f, double &Phi, double &Lam, double &h )
{
	double sphiold, sphinew, slam, recf, b, es, n, p;

    // 단반경 b#, 횡곡률 반경 N의 계산
    //     ** SEMI MAJOR AXIS - B **
    recf = 1 / f;
	b    = a * (recf - 1) / recf;
    es   = (a * a - b * b ) / ( a * a );

    //경도 계산
    slam = atan( y / x );
   
    //위도 및 타원체 높이 계산
    p = sqrt( x * x + y * y);
   
	// 1차 추정값
    h		= 0;
	sphiold = atan( z / p );
	n = fnSPHSN( a, es, sphiold );

	// 오차가 10^-12이내 일때까지 반복계산
    for( int i=0; i<30; i++ ) {
		sphinew= atan( z/(p-es*n*cos(sphiold)) );
		if( i>0 && fabs( sphinew - sphiold ) < 1E-18 )
			break;
		// 새 횡곡률 반경 및 h
		n = fnSPHSN( a, es, sphinew );
		h = p / cos( sphinew ) - n;
		sphiold = sphinew;
	}

/*
	double theta      = atan( (z*a) / (p*b) );
	double sin_theta3 = sin(theta) * sin(theta) * sin(theta);
	double cos_theta3 = cos(theta) * cos(theta) * cos(theta);
 	sphinew = atan( (z+es*b*sin_theta3) / (p-es*a*cos_theta3) );
*/
	Phi = I_RAD2GPN( sphinew );
	Lam = I_RAD2GPN( slam );

	double degrad = atan(1.0) / 45.0;
	Phi = sphinew  / degrad;
	Lam = slam     / degrad;

	if( x < 0 ) 
		Lam = 180 + Lam; // 90도 - 270 범위
	if( Lam < 0 ) 
	    Lam = 360 + Lam; // 270도 - 360 범위
}

void CTR2GPs( int numPoints, const double *x, const double *y, const double *z, double a, double f, double *Phi, double *Lam, double *h )
{
	double sphiold, sphinew, slam, recf, b, es, nn, p;

    // 단반경 b#, 횡곡률 반경 N의 계산
    //     ** SEMI MAJOR AXIS - B **
    recf = 1 / f;
	b    = a * (recf - 1) / recf;
    es   = (a * a - b * b ) / ( a * a );
	double degrad = atan(1.0) / 45.0;

	for( int n=0; n<numPoints; n++ ) {
		//경도 계산
		slam = atan( y[n] / x[n] );
   
		//위도 및 타원체 높이 계산
		p = sqrt( x[n] * x[n] + y[n] * y[n]);
   
		// 1차 추정값
		h[n]		= 0;
		sphiold = atan( z[n] / p );
		nn = fnSPHSN( a, es, sphiold );

		// 오차가 10^-12이내 일때까지 반복계산
		for( int i=0; i<30; i++ ) {
			sphinew= atan( z[n]/(p-es*nn*cos(sphiold)) );
			if( i>0 && fabs( sphinew - sphiold ) < 1E-18 )
				break;
			// 새 횡곡률 반경 및 h
			nn = fnSPHSN( a, es, sphinew );
			h[n] = p / cos( sphinew ) - nn;
			sphiold = sphinew;
		}

	/*
		double theta      = atan( (z*a) / (p*b) );
		double sin_theta3 = sin(theta) * sin(theta) * sin(theta);
		double cos_theta3 = cos(theta) * cos(theta) * cos(theta);
 		sphinew = atan( (z+es*b*sin_theta3) / (p-es*a*cos_theta3) );
	*/
		Phi[n] = I_RAD2GPN( sphinew );
		Lam[n] = I_RAD2GPN( slam );

		Phi[n] = sphinew  / degrad;
		Lam[n] = slam     / degrad;

		if( x[n] < 0 ) 
			Lam[n] = 180 + Lam[n]; // 90도 - 270 범위
        if( Lam != 0 )
			Lam[n] = 360 + Lam[n]; // 270도 - 360 범위
	}
}


// Parameters:
//     장반경 ( a ) , 단반경 ( b ), 편평률 ( f )
//     dfromLat, dfromLon, dfromH:  The geodetic position to be translated( 단위 radian )
//     from_a:   The semi-major axis of the "from" ellipsoid.( a )
//     from_f:   Flattening of the "from" ellipsoid.( f )
//     from_esq: Eccentricity-squared of the "from" ellipsoid. E^2 = 1 - b^2/a^2 = ( 2f - f^2 ) 
//     da:       Change in semi-major axis length (meters); "to" minus "from"    
//     df:       Change in flattening; "to" minus "from"
//     dx:       Change in x between "from" and "to" datum.
//     dy:       Change in y between "from" and "to" datum.
//     dz:       Change in z between "from" and "to" datum.
//	   dtoLat, dtoLon, dtoH : results
int TransGeocentric_Method( double dfromLon, double dfromLat,double dfromH,
                                   double from_a, double from_f,
                                   double from_esq, double da, double df,
                                   double dx, double dy, double dz,
								   double *dtoLon, double *dtoLat,double *dtoH )
{

	double slat = sin(dfromLat);
    double clat = cos(dfromLat);
    double slon = sin(dfromLon);
    double clon = cos(dfromLon);
    double ssqlat = slat * slat;
    double adb = 1.0 / (1.0 - from_f);  // "a divided by b"
    double dlat, dlon, dh;

    double rn = from_a / sqrt(1.0 - from_esq * ssqlat);
    double rm = from_a * (1. - from_esq) / pow((1.0 - from_esq * ssqlat), 1.5);

    dlat = (((((-dx * slat * clon - dy * slat * slon) + dz * clat)
                + (da * ((rn * from_esq * slat * clat) / from_a)))
            + (df * (rm * adb + rn / adb) * slat * clat)))
        / (rm + dfromH );

    dlon = (-dx * slon + dy * clon) / ((rn + dfromH) * clat);

    dh = (dx * clat * clon) + (dy * clat * slon) + (dz * slat)
         - (da * (from_a / rn)) + ((df * rn * ssqlat) / adb);
	
	//radian to degree
	*dtoLon = (dfromLon + dlon);
	*dtoLat = (dfromLat + dlat);
	*dtoH   = dfromH + dh;

	return 1;
}


/* Bessel <==> WGS84
	a) 128.535, -482.401, -664.745, 2.2004, 0.2038, -3.483, -0.3281 : 권대원논문 Moldensky 
	b) 199.538, -467.589, -607.207, 2.2004, 0.2038, -3.483, -0.3281 : 권대원논문 Bursa
	c) 127.046, -478.916, -665.615, -2.156, -2.341, 1.714, -5.626   : 임영빈논문
	d) 146.44, -507.89, -681.46, 0, 0, 0, 0 :  측량 작업규정 
	d') 146.44, 507.89, 608.46
	e) 147, -506, -687, 0, 0, 0, 0 : 미국 프로그램 , Tokyo datum, south Korea
	f) 144.778, -503.702, -684.67, 0, 0, 0, 0 :  임영빈 논문 
	g) 105.627, -462.37, -643.258, 0, 0, 0, 0 : GPS Korea 
*/
void WGP2BGP( double xw, double yw, double zw, double &xb, double &yb, double &zb )
{
	double x, y, z, tx, ty, tz;
	GP2CTR( xw, yw, zw, WGS84_AXIS, WGS84_FLAT, x, y, z );

	double dx, dy, dz;
	dx = 146; dy = -507; dz = -687;
	
	TransBursa( x, y, z, tx, ty, tz, dx, dy, dz, 0, 0, 0, 0 );
	
	CTR2GP( tx, ty, tz, BESSEL_AXIS, BESSEL_FLAT, xb, yb, zb );
}

void BGP2WGP( double xb, double yb, double zb, double &xw, double &yw, double &zw )
{
	double x, y, z, tx, ty, tz;
	GP2CTR( xb, yb, zb, BESSEL_AXIS, BESSEL_FLAT, x, y, z );

	double dx, dy, dz;
	
	dx = 146; dy = -507; dz = -687;
	
	InverseBursa( x,  y,  z, tx, ty, tz,
				  dx, dy, dz, 0, 0, 0, 0 );
	CTR2GP( tx, ty, tz, WGS84_AXIS, WGS84_FLAT, xw, yw, zw );
}

void WGP2BGPs( int numPoints, const double *xw, const double *yw, const double *zw, double *xb, double *yb, double *zb )
{
	double *x, *y, *z, *tx, *ty, *tz;
	x = new double [ numPoints ];
	y = new double [ numPoints ];
	z = new double [ numPoints ];
	tx = new double [ numPoints ];
	ty = new double [ numPoints ];
	tz = new double [ numPoints ];

	GP2CTRs( numPoints, xw, yw, zw, WGS84_AXIS, WGS84_FLAT, x, y, z );

	double dx, dy, dz;
	dx = 146; dy = -507; dz = -687;
	TransBursas( numPoints, x, y, z, tx, ty, tz,
				 dx, dy, dz, 0, 0, 0, 0 );
	CTR2GPs( numPoints, tx, ty, tz, BESSEL_AXIS, BESSEL_FLAT, xb, yb, zb );

	delete []x;
	delete []y;
	delete []z;
	delete []tx;
	delete []ty;
	delete []tz;
}

void BGP2WGPs( int numPoints, const double *xb, const double *yb, const double *zb, double *xw, double *yw, double *zw )
{
	double *x, *y, *z, *tx, *ty, *tz;
	x = new double [ numPoints ];
	y = new double [ numPoints ];
	z = new double [ numPoints ];
	tx = new double [ numPoints ];
	ty = new double [ numPoints ];
	tz = new double [ numPoints ];

	GP2CTRs( numPoints, xb, yb, zb, BESSEL_AXIS, BESSEL_FLAT, x, y, z );

	double dx, dy, dz;
	dx = 146; dy = -507; dz = -687;
	InverseBursas( numPoints, x,  y,  z, tx, ty, tz,
				  dx, dy, dz, 0, 0, 0, 0 );
	CTR2GPs( numPoints, tx, ty, tz, WGS84_AXIS, WGS84_FLAT, xw, yw, zw );

	delete []x;
	delete []y;
	delete []z;
	delete []tx;
	delete []ty;
	delete []tz;
}

void BGP2GGP_( double x, double y, double z, double &tx, double &ty, double &tz )
{
	double xa, ya, za;
	xa = -3159521.31;
	ya = 4068151.32;
	za = 3748113.85;

	double dx, dy, dz, omega, Phi, kappa, ds;
	dx = -145.907;  dy = 505.034; dz = 685.756;
	omega = -1.162; Phi = 2.347; kappa = 1.592;
	ds    = 6.342;
	ForwordMolodenskyBadekas( x,  y,  z, tx, ty, tz, xa, ya, za, dx, dy, dz, omega, Phi, kappa, ds );
}

void BGP2GGP_s( int numPoints, const double *x, const double *y, const double *z, double *tx, double *ty, double *tz )
{
	double xa, ya, za;
	xa = -3159521.31;
	ya = 4068151.32;
	za = 3748113.85;

	double dx, dy, dz, omega, Phi, kappa, ds;
	dx = -145.907;  dy = 505.034; dz = 685.756;
	omega = -1.162; Phi = 2.347; kappa = 1.592;
	ds    = 6.342;
	ForwordMolodenskyBadekas_s( numPoints, x,  y,  z, tx, ty, tz, xa, ya, za, dx, dy, dz, omega, Phi, kappa, ds );
}

void GGP2BGP_( double x, double y, double z, double &tx, double &ty, double &tz )
{
	double xa, ya, za;
	xa = -3159666.86;
	ya = 4068655.70;
	za = 3748799.65;

	double dx, dy, dz, omega, Phi, kappa, ds;
	dx = -145.907;  dy = 505.034; dz = 685.756;
	omega = -1.162; Phi = 2.347; kappa = 1.592;
	ds    = 6.342;

	InverseMolodenskyBadekas( x, y, z, tx, ty, tz, xa, ya, za, dx, dy, dz, omega, Phi, kappa, ds );
}

void GGP2BGP_s( int numPoints, const double *x, const double *y, const double *z, double *tx, double *ty, double *tz )
{
	double xa, ya, za;
	xa = -3159666.86;
	ya = 4068655.70;
	za = 3748799.65;

	double dx, dy, dz, omega, Phi, kappa, ds;
	dx = -145.907;  dy = 505.034; dz = 685.756;
	omega = -1.162; Phi = 2.347; kappa = 1.592;
	ds    = 6.342;

	InverseMolodenskyBadekas_s( numPoints, x, y, z, tx, ty, tz, xa, ya, za, dx, dy, dz, omega, Phi, kappa, ds );
}


//
// /* Bessel <==> GRS84
//    a) 145.907, -505.034, -685.756, 1.162, -2.347, -1.592, -6.342   : 2003년 정부고시(GRS80)
//
void GGP2BGP( double xw, double yw, double zw, double &xb, double &yb, double &zb )
{
	double x, y, z, tx, ty, tz;
	GP2CTR( xw, yw, zw, GRS80_AXIS, GRS80_FLAT, x, y, z );

	GGP2BGP_( x, y, z, tx, ty, tz );

	CTR2GP( tx, ty, tz, BESSEL_AXIS, BESSEL_FLAT, xb, yb, zb );
}

void BGP2GGP( double xb, double yb, double zb, double &xw, double &yw, double &zw )
{
	double x, y, z, tx, ty, tz;
	GP2CTR( xb, yb, zb, BESSEL_AXIS, BESSEL_FLAT, x, y, z );

	BGP2GGP_( x, y, z, tx, ty, tz );

	CTR2GP( tx, ty, tz, GRS80_AXIS, GRS80_FLAT, xw, yw, zw );
}

void GGP2BGPs( int numPoints, const double *xw, const double *yw, const double *zw, double *xb, double *yb, double *zb )
{
	double *x, *y, *z, *tx, *ty, *tz;
	x = new double [ numPoints ];
	y = new double [ numPoints ];
	z = new double [ numPoints ];
	tx = new double [ numPoints ];
	ty = new double [ numPoints ];
	tz = new double [ numPoints ];

	GP2CTRs( numPoints, xw, yw, zw, GRS80_AXIS, GRS80_FLAT, x, y, z );

	GGP2BGP_s( numPoints, x, y, z, tx, ty, tz );

	CTR2GPs( numPoints, tx, ty, tz, BESSEL_AXIS, BESSEL_FLAT, xb, yb, zb );

	delete []x;
	delete []y;
	delete []z;
	delete []tx;
	delete []ty;
	delete []tz;
}

void BGP2GGPs( int numPoints, const double *xb, const double *yb, const double *zb, double *xw, double *yw, double *zw )
{
	double *x, *y, *z, *tx, *ty, *tz;
	x = new double [ numPoints ];
	y = new double [ numPoints ];
	z = new double [ numPoints ];
	tx = new double [ numPoints ];
	ty = new double [ numPoints ];
	tz = new double [ numPoints ];

	GP2CTRs( numPoints, xb, yb, zb, BESSEL_AXIS, BESSEL_FLAT, x, y, z );

	BGP2GGP_s( numPoints, x, y, z, tx, ty, tz );

	CTR2GPs( numPoints, tx, ty, tz, GRS80_AXIS, GRS80_FLAT, xw, yw, zw );

	delete []x;
	delete []y;
	delete []z;
	delete []tx;
	delete []ty;
	delete []tz;
}
