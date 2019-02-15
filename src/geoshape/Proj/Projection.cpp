#include "geoshape/Proj/Projection.h"
#include "geoshape/Proj/ProjDef.h"
#include "geoshape/Proj/TransDatum7.h"
#include <math.h>
#include <string.h>


struct SMPTransferMeractorParam {
	double		fAxis;    // Major Axis
	double		fFlat;    // Flat
	double		fCentralMeridian;
	double		fLatitudeOfOrigin;
	double		fScaleFactor;
	double		fFalseEasting;
	double		fFalseNorthing;
} TMParams[] = {
	{ BESSEL_AXIS, BESSEL_FLAT, 127.00289027777777777777777777777778, 38.0, 1, 200000, 500000 }, // TM중부(Bessel)
	{ BESSEL_AXIS, BESSEL_FLAT, 125.00289027777777777777777777777778, 38.0, 1, 200000, 500000 }, // TM서부(Bessel)
	{ BESSEL_AXIS, BESSEL_FLAT, 129.00289027777777777777777777777778, 38.0, 1, 200000, 500000 }, // TM동부(Bessel)
	{ BESSEL_AXIS, BESSEL_FLAT, 129.00289027777777777777777777777778, 38.0, 1, 200000, 550000 }, // TM제주(Bessel)
	{ BESSEL_AXIS, BESSEL_FLAT, 131.00289027777777777777777777777778, 38.0, 1, 200000, 500000 }, // TM동해(Bessel) : 을릉도,독도
	{ BESSEL_AXIS, BESSEL_FLAT, 128.000000000000, 38.0, 0.9999, 400000, 600000 }, // KATECH
	
	{ GRS80_AXIS,  GRS80_FLAT,  127.000000000000, 38.0, 1, 200000, 500000 }, // TM중부(GRS80)
	{ GRS80_AXIS,  GRS80_FLAT,  125.000000000000, 38.0, 1, 200000, 500000 }, // TM서부(GRS80)
	{ GRS80_AXIS,  GRS80_FLAT,  129.000000000000, 38.0, 1, 200000, 500000 }, // TM동부(GRS80)
	{ GRS80_AXIS,  GRS80_FLAT,  129.000000000000, 38.0, 1, 200000, 550000 }, // TM제주(GRS80)
	{ GRS80_AXIS,  GRS80_FLAT,  131.000000000000, 38.0, 1, 200000, 500000 }, // TM동해(GRS80) : 을릉도,독도
	
	// 세계 측지계 변경
	// "측량.수로조사 및 지적에 관한 법률" 2009년 12월 10 시행
	{ GRS80_AXIS,  GRS80_FLAT,  127.000000000000, 38.0, 1, 200000, 600000 }, // TM중부(GRS80) 
	{ GRS80_AXIS,  GRS80_FLAT,  125.000000000000, 38.0, 1, 200000, 600000 }, // TM서부(GRS80)
	{ GRS80_AXIS,  GRS80_FLAT,  129.000000000000, 38.0, 1, 200000, 600000 }, // TM동부(GRS80)
	{ GRS80_AXIS,  GRS80_FLAT,  131.000000000000, 38.0, 1, 200000, 500000 }, // TM동해(GRS80) : 을릉도,독도

	{ BESSEL_AXIS, BESSEL_FLAT, 123.000000000000,  0.0, 0.9996, 500000, 0 }, // UTM 51 North(Bessel) 
	{ BESSEL_AXIS, BESSEL_FLAT, 129.000000000000,  0.0, 0.9996, 500000, 0 }, // UTM 52 North(Bessel) 
	{ WGS84_AXIS,  WGS84_FLAT,  123.000000000000,  0.0, 0.9996, 500000, 0 }, // UTM 51 North(WGS84) 
	{ WGS84_AXIS,  WGS84_FLAT,  129.000000000000,  0.0, 0.9996, 500000, 0 }, // UTM 52 North(WGS84) 
	{ GRS80_AXIS,  GRS80_FLAT,  123.000000000000,  0.0, 0.9996, 500000, 0 }, // UTM 51 North(GRS80) 
	{ GRS80_AXIS,  GRS80_FLAT,  129.000000000000,  0.0, 0.9996, 500000, 0 }, // UTM 52 North(GRS80) 
	{ GRS80_AXIS,  GRS80_FLAT,  127.500000000000, 38.0, 0.9996, 1000000, 2000000 },	//UTM-K GRS80
	{ BESSEL_AXIS, BESSEL_FLAT, 127.502890000000, 38.0, 0.9996, 1000000, 2000000 },	//UTM-K BESSEL
	
	{ WGS84_AXIS,  WGS84_FLAT,  127.000000000000, 38.0, 1, 200000, 500000 }, // WCONGNAMUL
};

double AdjustLongitude( double x )
{
	long count = 0;
	for( ; ; ) {
		if( fabs(x) <= _PIE)
			break;
		else if( ((long) fabs( x / _PIE) ) < 2 )
			x = x - ( sign(x) * _TWO_PIE );
		else if( ((long) fabs( x / _TWO_PIE)) < _MAXLONG ) {
			x = x - (((long)(x / _TWO_PIE))*_TWO_PIE);
		}else if( ((long) fabs( x / (_MAXLONG * _TWO_PIE))) < _MAXLONG ) {
			x = x - (((long)(x / (_MAXLONG * _TWO_PIE))) * (_TWO_PIE * _MAXLONG));
		}else if( ((long) fabs( x / (_DBLLONG * _TWO_PIE)) ) < _MAXLONG ) {
			x = x - (((long)(x / (_DBLLONG * _TWO_PIE))) * (_TWO_PIE * _DBLLONG));
		}else
			x = x - ( sign(x) * _TWO_PIE);
		count++;
		if (count > _MAX_VAL)
			break;
	}

	return( x );
}

void TM_Forword( SMPTransferMeractorParam &param, double lon_deg, double lat_deg, double &x, double &y )
{
	double a, b, f, recf, fe, nfn, ok;
	double es, ebs;
	double ap, bp, cp, dp, ep, fp, gp, hp, ip;
	double olam, ophi;  
	double fScaleFactor;
	double fFalseEasting;
	double fFalseNorthing;

	olam				= I_GPN2RAD( param.fCentralMeridian );
	ophi				= I_GPN2RAD( param.fLatitudeOfOrigin );
	fScaleFactor        = param.fScaleFactor;
	fFalseEasting       = param.fFalseEasting;
	fFalseNorthing      = param.fFalseNorthing;

	// Internal Params 
	a      = param.fAxis; 
	f      = param.fFlat;
	recf   = 1./f;
	b      = a*(recf-1.)/recf;
	fe     = fFalseEasting;
	nfn    = fFalseNorthing;
	ok     = fScaleFactor;

	es    = (a*a - b*b)/(a*a);
	ebs   = (a*a - b*b)/(b*b);

	ap = 1+3*es/4+45*pow(es,2)/64+175*pow(es,3)/256+11025*pow(es,4)/16384+43659*pow(es,5)/65536;
	bp = 3*es/4+15*pow(es,2)/16+525*pow(es,3)/512+2205*pow(es,4)/2048+72765*pow(es,5)/65536;
	cp = 15*pow(es,2)/64+105*pow(es,3)/256+2205*pow(es,4)/4096+10395*pow(es,5)/16384;
	dp = 35*pow(es,3)/512+315*pow(es,4)/2048+31185*pow(es,5)/131072;
	ep = 315*pow(es,4)/16384+3465*pow(es,5)/65536+99099*pow(es,6)/1048480+4099100*pow(es,7)/29360128+348423075*pow(es,8)/1879048192;
	fp = 693*pow(es,5)/131072+9009*pow(es,6)/524288+4099100*pow(es,7)/117440512+26801775*pow(es,8)/469762048;
	gp = 3003*pow(es,6)/2097150+315315*pow(es,7)/58720256+11486475*pow(es,8)/939524096;
	hp = 45045*pow(es,7)/117440512+765765*pow(es,8)/469762048;
	ip = 765765*pow(es,8)/7516192768.0;


	// 계산
	double slam = I_GPN2RAD( lon_deg );
	double sphi = I_GPN2RAD( lat_deg );

	double dlam  = AdjustLongitude( slam - olam );

	double tmd  = a*(1-es)*( 
		             ap*(sphi-ophi)                   - bp*(sin(2*sphi) - sin(2*ophi))/2 
				   + cp*(sin(4*sphi) -sin(4*ophi))/4  - dp*(sin(6*sphi) - sin(6*ophi))/6   
				   + ep*(sin(8*sphi) -sin(8*ophi))/8  - fp*(sin(10*sphi)- sin(10*ophi))/10 
				   + gp*(sin(12*sphi)-sin(12*ophi))/12- hp*(sin(14*sphi)- sin(14*ophi))/14 
				   + ip*(sin(16*sphi)-sin(16*ophi))/16 );

	double s  = sin(sphi);
	double c  = cos(sphi);
	double t  = tan(sphi); // sin/cos
	double eta = ebs*c*c;
    double sn  = a / sqrt(1-es*s*s);

	double t1, t2, t3, t4, t5, t6, t7, t8, t9;
	t1 = tmd*ok;
    t2 = sn*s*c*ok/2.;
	t3 = sn*s*c*c*c*ok*(5.-t*t+9.*eta+4.*eta*eta)/24.;
	t4 = sn*s*c*c*c*c*c*ok*(61.-58.*t*t+t*t*t*t+270.*eta-330.*t*t*eta
	     +445.*eta*eta+324.*eta*eta*eta-680*eta*eta*t*t+88.*eta*eta*eta*eta
	     -600.*t*t*eta*eta*eta-192.*t*t*eta*eta*eta*eta)/720.;
	t5 = sn*s*c*c*c*c*c*c*c*ok*(1385.-3111.*t*t+543.*t*t*t*t-t*t*t*t*t*t)/40320.;

	y = nfn+t1+dlam*dlam*t2+dlam*dlam*dlam*dlam*t3+dlam*dlam*dlam*dlam*dlam*dlam*t4
	     +dlam*dlam*dlam*dlam*dlam*dlam*dlam*dlam*t5;

	t6 = sn*c*ok;
	t7 = sn*c*c*c*ok*(1.-t*t+eta)/6.;
	t8 = sn*c*c*c*c*c*ok*(5.-18.*t*t+t*t*t*t+14.*eta-58.*t*t*eta
	   +13.*eta*eta+4.*eta*eta*eta-64.*eta*eta*t*t-24.*eta*eta*eta*eta)/120.;
	t9 = sn*c*c*c*c*c*c*c*ok*(61.-479.*t*t+179.*t*t*t*t-t*t*t*t*t*t)/5040.;

	x = fe+dlam*t6+dlam*dlam*dlam*t7+dlam*dlam*dlam*dlam*dlam*t8+dlam*dlam*dlam*dlam*dlam*dlam*dlam*t9;
}

void TM_Forwords( SMPTransferMeractorParam &param, int numPoints, const double *lon_deg, const double *lat_deg, double *d_x, double *d_y )
{
	double a, b, f, recf, fe, nfn, ok;
	double es, ebs;
	double ap, bp, cp, dp, ep, fp, gp, hp, ip;
	double olam, ophi;  
	double fScaleFactor;
	double fFalseEasting;
	double fFalseNorthing;

	olam				= I_GPN2RAD( param.fCentralMeridian );
	ophi				= I_GPN2RAD( param.fLatitudeOfOrigin );
	fScaleFactor        = param.fScaleFactor;
	fFalseEasting       = param.fFalseEasting;
	fFalseNorthing      = param.fFalseNorthing;

	// Internal Params 
	a      = param.fAxis; 
	f      = param.fFlat;
	recf   = 1./f;
	b      = a*(recf-1.)/recf;
	fe     = fFalseEasting;
	nfn    = fFalseNorthing;
	ok     = fScaleFactor;

	es    = (a*a - b*b)/(a*a);
	ebs   = (a*a - b*b)/(b*b);

	ap = 1+3*es/4+45*pow(es,2)/64+175*pow(es,3)/256+11025*pow(es,4)/16384+43659*pow(es,5)/65536;
	bp = 3*es/4+15*pow(es,2)/16+525*pow(es,3)/512+2205*pow(es,4)/2048+72765*pow(es,5)/65536;
	cp = 15*pow(es,2)/64+105*pow(es,3)/256+2205*pow(es,4)/4096+10395*pow(es,5)/16384;
	dp = 35*pow(es,3)/512+315*pow(es,4)/2048+31185*pow(es,5)/131072;
	ep = 315*pow(es,4)/16384+3465*pow(es,5)/65536+99099*pow(es,6)/1048480+4099100*pow(es,7)/29360128+348423075*pow(es,8)/1879048192;
	fp = 693*pow(es,5)/131072+9009*pow(es,6)/524288+4099100*pow(es,7)/117440512+26801775*pow(es,8)/469762048;
	gp = 3003*pow(es,6)/2097150+315315*pow(es,7)/58720256+11486475*pow(es,8)/939524096;
	hp = 45045*pow(es,7)/117440512+765765*pow(es,8)/469762048;
	ip = 765765*pow(es,8)/7516192768.0;


	// 계산
	for( int n=0; n<numPoints; n++ ) {
		double slam = I_GPN2RAD( lon_deg[ n ] );
		double sphi = I_GPN2RAD( lat_deg[ n ] );

		double dlam  = AdjustLongitude( slam - olam );

		double tmd  = a*(1-es)*( 
						 ap*(sphi-ophi)                   - bp*(sin(2*sphi) - sin(2*ophi))/2 
					   + cp*(sin(4*sphi) -sin(4*ophi))/4  - dp*(sin(6*sphi) - sin(6*ophi))/6   
					   + ep*(sin(8*sphi) -sin(8*ophi))/8  - fp*(sin(10*sphi)- sin(10*ophi))/10 
					   + gp*(sin(12*sphi)-sin(12*ophi))/12- hp*(sin(14*sphi)- sin(14*ophi))/14 
					   + ip*(sin(16*sphi)-sin(16*ophi))/16 );

		double s  = sin(sphi);
		double c  = cos(sphi);
		double t  = tan(sphi); // sin/cos
		double eta = ebs*c*c;
		double sn  = a / sqrt(1-es*s*s);

		double t1, t2, t3, t4, t5, t6, t7, t8, t9;
		t1 = tmd*ok;
		t2 = sn*s*c*ok/2.;
		t3 = sn*s*c*c*c*ok*(5.-t*t+9.*eta+4.*eta*eta)/24.;
		t4 = sn*s*c*c*c*c*c*ok*(61.-58.*t*t+t*t*t*t+270.*eta-330.*t*t*eta
			 +445.*eta*eta+324.*eta*eta*eta-680*eta*eta*t*t+88.*eta*eta*eta*eta
			 -600.*t*t*eta*eta*eta-192.*t*t*eta*eta*eta*eta)/720.;
		t5 = sn*s*c*c*c*c*c*c*c*ok*(1385.-3111.*t*t+543.*t*t*t*t-t*t*t*t*t*t)/40320.;

		d_y[n] = nfn+t1+dlam*dlam*t2+dlam*dlam*dlam*dlam*t3+dlam*dlam*dlam*dlam*dlam*dlam*t4
			 +dlam*dlam*dlam*dlam*dlam*dlam*dlam*dlam*t5;

		t6 = sn*c*ok;
		t7 = sn*c*c*c*ok*(1.-t*t+eta)/6.;
		t8 = sn*c*c*c*c*c*ok*(5.-18.*t*t+t*t*t*t+14.*eta-58.*t*t*eta
		   +13.*eta*eta+4.*eta*eta*eta-64.*eta*eta*t*t-24.*eta*eta*eta*eta)/120.;
		t9 = sn*c*c*c*c*c*c*c*ok*(61.-479.*t*t+179.*t*t*t*t-t*t*t*t*t*t)/5040.;
		d_x[n] = fe+dlam*t6+dlam*dlam*dlam*t7+dlam*dlam*dlam*dlam*dlam*t8+dlam*dlam*dlam*dlam*dlam*dlam*dlam*t9;
	}
}

void TM_Inverse( SMPTransferMeractorParam &param, double x, double y, double &lon_deg, double &lat_deg )
{
	double olam, ophi;  
	double a, b, f, recf, fe, nfn, ok;
	double es, ebs;
	double ap, bp, cp, dp, ep, fp, gp, hp, ip;
	double fScaleFactor;
	double fFalseEasting;
	double fFalseNorthing;

	olam				= I_GPN2RAD( param.fCentralMeridian );
	ophi				= I_GPN2RAD( param.fLatitudeOfOrigin );
	fScaleFactor        = param.fScaleFactor;
	fFalseEasting       = param.fFalseEasting;
	fFalseNorthing      = param.fFalseNorthing;

	// Internal Params
	a      = param.fAxis; 
	f      = param.fFlat;
	recf   = 1./f;
	b      = a*(recf-1.)/recf;
	fe     = fFalseEasting;
	nfn    = fFalseNorthing;
	ok     = fScaleFactor;

	es    = (a*a - b*b)/(a*a);
	ebs   = (a*a - b*b)/(b*b);

	ap = 1+3*es/4+45*pow(es,2)/64+175*pow(es,3)/256+11025*pow(es,4)/16384+43659*pow(es,5)/65536;
	bp = 3*es/4+15*pow(es,2)/16+525*pow(es,3)/512+2205*pow(es,4)/2048+72765*pow(es,5)/65536;
	cp = 15*pow(es,2)/64+105*pow(es,3)/256+2205*pow(es,4)/4096+10395*pow(es,5)/16384;
	dp = 35*pow(es,3)/512+315*pow(es,4)/2048+31185*pow(es,5)/131072;
	ep = 315*pow(es,4)/16384+3465*pow(es,5)/65536+99099*pow(es,6)/1048480+4099100*pow(es,7)/29360128+348423075*pow(es,8)/1879048192;
	fp = 693*pow(es,5)/131072+9009*pow(es,6)/524288+4099100*pow(es,7)/117440512+26801775*pow(es,8)/469762048;
	gp = 3003*pow(es,6)/2097150+315315*pow(es,7)/58720256+11486475*pow(es,8)/939524096;
	hp = 45045*pow(es,7)/117440512+765765*pow(es,8)/469762048;
	ip = 765765*pow(es,8)/7516192768.0;


	double t10, t11, t12, t13, t14, t15, t16, t17;

	double tmd   = (y-nfn)/ok;
	double sr    = a*(1-es)*ap; 
	double ftphi = ophi + tmd/sr;

	for( int i=0; i<20; i++ ) {
		t10 =    a*(1.-es) * ( ap*( ftphi - ophi ) 
			   - bp*( sin(2*ftphi) - sin(2*ophi) )/2 
			   + cp*( sin(4*ftphi) - sin(4*ophi) )/4 
			   - dp*( sin(6*ftphi) - sin(6*ophi) )/6   
			   + ep*( sin(8*ftphi) - sin(8*ophi) )/8
			   - fp*( sin(10*ftphi)- sin(10*ophi))/10
			   + gp*( sin(12*ftphi)- sin(12*ophi))/12
			   - hp*( sin(14*ftphi)- sin(14*ophi))/14 
			   + ip*( sin(16*ftphi)- sin(16*ophi))/16 );
//		sr     = a*(1.-es) * ap;
		ftphi += ( tmd - t10 ) / sr;
	}

	double c  = cos(ftphi);
	double s  = sin(ftphi);
	double t  = tan(ftphi);     // sin/cos
	double eta = ebs*c*c;
    double sn  = a/sqrt(1-es*s*s);											 
	sr  = a*(1.-es)/( sqrt(1-es*s*s)* sqrt(1-es*s*s)* sqrt(1-es*s*s) );

	double de = x - fe;
	
	t10 = t/( 2.*sr*sn*ok*ok );
	t11 = t*( 5.+3.*t*t +eta -4.*eta*eta-9.*t*t*eta )/( 24.*sr*sn*sn*sn*ok*ok*ok*ok );
	t12 = t*( 61. +90.*t*t +46.*eta +45.*t*t*t*t -252.*t*t*eta -3.*eta*eta +100.*eta*eta*eta -66.*t*t*eta*eta -90.*t*t*t*t*eta
		  +88.*eta*eta*eta*eta +225.*t*t*t*t*eta*eta +84.*t*t*eta*eta*eta -192.*t*t*eta*eta*eta*eta )/( 720.*sr*sn*sn*sn*sn*sn*ok*ok*ok*ok*ok*ok );
	t13 = t*( 1385. +3633.*t*t +4095.*t*t*t*t +1575.*t*t*t*t*t*t )/( 40320.*sr*sn*sn*sn*sn*sn*sn*sn*ok*ok*ok*ok*ok*ok*ok*ok );
    double sphi = ftphi -de*de*t10 +de*de*de*de*t11 -de*de*de*de*de*de*t12 +de*de*de*de*de*de*de*de*t13;

	t14 = 1./( sn*c*ok );
	t15 = ( 1. +2.*t*t +eta )/( 6.*sn*sn*sn*c*ok*ok*ok );
	t16 = ( 5. +6.*eta +28.*t*t -3.*eta*eta +8.*t*t*eta +24.*t*t*t*t -4.*eta*eta*eta +4.*t*t*eta*eta +24.*t*t*eta*eta*eta )/( 120.*sn*sn*sn*sn*sn*c*ok*ok*ok*ok*ok );
	t17 = ( 61. +662.*t*t +1320.*t*t*t*t +720.*t*t*t*t*t*t )/( 5040.*sn*sn*sn*sn*sn*sn*sn*c*ok*ok*ok*ok*ok*ok*ok );

	double dlam = de*t14 - de*de*de*t15 + de*de*de*de*de*t16 - de*de*de*de*de*de*de*t17;
	
	double slam = olam + dlam;

	lon_deg = I_RAD2GPN( slam );
	lat_deg = I_RAD2GPN( sphi );
}

void TM_Inverses( SMPTransferMeractorParam &param, int numPoints, const double *s_x, const double *s_y, double *lon_deg, double *lat_deg )
{
	double olam, ophi;  
	double a, b, f, recf, fe, nfn, ok;
	double es, ebs;
	double ap, bp, cp, dp, ep, fp, gp, hp, ip;
	double fScaleFactor;
	double fFalseEasting;
	double fFalseNorthing;

	olam				= I_GPN2RAD( param.fCentralMeridian );
	ophi				= I_GPN2RAD( param.fLatitudeOfOrigin );
	fScaleFactor        = param.fScaleFactor;
	fFalseEasting       = param.fFalseEasting;
	fFalseNorthing      = param.fFalseNorthing;

	// Internal Params
	a      = param.fAxis; 
	f      = param.fFlat;
	recf   = 1./f;
	b      = a*(recf-1.)/recf;
	fe     = fFalseEasting;
	nfn    = fFalseNorthing;
	ok     = fScaleFactor;

	es    = (a*a - b*b)/(a*a);
	ebs   = (a*a - b*b)/(b*b);

	ap = 1+3*es/4+45*pow(es,2)/64+175*pow(es,3)/256+11025*pow(es,4)/16384+43659*pow(es,5)/65536;
	bp = 3*es/4+15*pow(es,2)/16+525*pow(es,3)/512+2205*pow(es,4)/2048+72765*pow(es,5)/65536;
	cp = 15*pow(es,2)/64+105*pow(es,3)/256+2205*pow(es,4)/4096+10395*pow(es,5)/16384;
	dp = 35*pow(es,3)/512+315*pow(es,4)/2048+31185*pow(es,5)/131072;
	ep = 315*pow(es,4)/16384+3465*pow(es,5)/65536+99099*pow(es,6)/1048480+4099100*pow(es,7)/29360128+348423075*pow(es,8)/1879048192;
	fp = 693*pow(es,5)/131072+9009*pow(es,6)/524288+4099100*pow(es,7)/117440512+26801775*pow(es,8)/469762048;
	gp = 3003*pow(es,6)/2097150+315315*pow(es,7)/58720256+11486475*pow(es,8)/939524096;
	hp = 45045*pow(es,7)/117440512+765765*pow(es,8)/469762048;
	ip = 765765*pow(es,8)/7516192768.0;


	double t10, t11, t12, t13, t14, t15, t16, t17;

	for( int n=0; n<numPoints; n++ ) {
		double tmd   = (s_y[n]-nfn)/ok;
		double sr    = a*(1-es)*ap; 
		double ftphi = ophi + tmd/sr;

		for( int i=0; i<20; i++ ) {
			t10 =    a*(1.-es) * ( ap*( ftphi - ophi ) 
				   - bp*( sin(2*ftphi) - sin(2*ophi) )/2 
				   + cp*( sin(4*ftphi) - sin(4*ophi) )/4 
				   - dp*( sin(6*ftphi) - sin(6*ophi) )/6   
				   + ep*( sin(8*ftphi) - sin(8*ophi) )/8
				   - fp*( sin(10*ftphi)- sin(10*ophi))/10
				   + gp*( sin(12*ftphi)- sin(12*ophi))/12
				   - hp*( sin(14*ftphi)- sin(14*ophi))/14 
				   + ip*( sin(16*ftphi)- sin(16*ophi))/16 );
	//		sr     = a*(1.-es) * ap;
			ftphi += ( tmd - t10 ) / sr;
		}

		double c  = cos(ftphi);
		double s  = sin(ftphi);
		double t  = tan(ftphi);     // sin/cos
		double eta = ebs*c*c;
		double sn  = a/sqrt(1-es*s*s);											 
		sr  = a*(1.-es)/( sqrt(1-es*s*s)* sqrt(1-es*s*s)* sqrt(1-es*s*s) );

		double de = s_x[n] - fe;
		
		t10 = t/( 2.*sr*sn*ok*ok );
		t11 = t*( 5.+3.*t*t +eta -4.*eta*eta-9.*t*t*eta )/( 24.*sr*sn*sn*sn*ok*ok*ok*ok );
		t12 = t*( 61. +90.*t*t +46.*eta +45.*t*t*t*t -252.*t*t*eta -3.*eta*eta +100.*eta*eta*eta -66.*t*t*eta*eta -90.*t*t*t*t*eta
			  +88.*eta*eta*eta*eta +225.*t*t*t*t*eta*eta +84.*t*t*eta*eta*eta -192.*t*t*eta*eta*eta*eta )/( 720.*sr*sn*sn*sn*sn*sn*ok*ok*ok*ok*ok*ok );
		t13 = t*( 1385. +3633.*t*t +4095.*t*t*t*t +1575.*t*t*t*t*t*t )/( 40320.*sr*sn*sn*sn*sn*sn*sn*sn*ok*ok*ok*ok*ok*ok*ok*ok );
		double sphi = ftphi -de*de*t10 +de*de*de*de*t11 -de*de*de*de*de*de*t12 +de*de*de*de*de*de*de*de*t13;

		t14 = 1./( sn*c*ok );
		t15 = ( 1. +2.*t*t +eta )/( 6.*sn*sn*sn*c*ok*ok*ok );
		t16 = ( 5. +6.*eta +28.*t*t -3.*eta*eta +8.*t*t*eta +24.*t*t*t*t -4.*eta*eta*eta +4.*t*t*eta*eta +24.*t*t*eta*eta*eta )/( 120.*sn*sn*sn*sn*sn*c*ok*ok*ok*ok*ok );
		t17 = ( 61. +662.*t*t +1320.*t*t*t*t +720.*t*t*t*t*t*t )/( 5040.*sn*sn*sn*sn*sn*sn*sn*c*ok*ok*ok*ok*ok*ok*ok );

		double dlam = de*t14 - de*de*de*t15 + de*de*de*de*de*t16 - de*de*de*de*de*de*de*t17;
		
		double slam = olam + dlam;

		lon_deg[n] = I_RAD2GPN( slam );
		lat_deg[n] = I_RAD2GPN( sphi );
	}
}

PROJ_COORD_TYPE GetGPType( PROJ_COORD_TYPE src )
{
    switch( src )
    {
        case PROJ_COORD_TYPE::TM_KOREA_MIDDLE :
        case PROJ_COORD_TYPE::TM_KOREA_EAST	 :
        case PROJ_COORD_TYPE::TM_KOREA_EAST_SEA	 :
        case PROJ_COORD_TYPE::TM_KOREA_WEST   :
        case PROJ_COORD_TYPE::TM_KOREA_JEIJU	 :
        case PROJ_COORD_TYPE::TM_KATECH:
        case PROJ_COORD_TYPE::UTM_51N_BESSEL  :
        case PROJ_COORD_TYPE::UTM_52N_BESSEL  :
        case PROJ_COORD_TYPE::LL_KOREA_BESSEL :
        case PROJ_COORD_TYPE::UTMK_BESSEL	 :
            return PROJ_COORD_TYPE::LL_KOREA_BESSEL;
        case PROJ_COORD_TYPE::TM_GRS80_MIDDLE :
        case PROJ_COORD_TYPE::TM_GRS80_WEST   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST   :
        case PROJ_COORD_TYPE::TM_GRS80_JEIJU  :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_SEA :
		
        case PROJ_COORD_TYPE::TM_GRS80_MIDDLE_60 :
        case PROJ_COORD_TYPE::TM_GRS80_WEST_60   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_60   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_SEA_60 :

        case PROJ_COORD_TYPE::UTM_51N_GRS80 :
        case PROJ_COORD_TYPE::UTM_52N_GRS80 :
        case PROJ_COORD_TYPE::UTMK_GRS80	   :
        case PROJ_COORD_TYPE::LL_GRS80      :
            return PROJ_COORD_TYPE::LL_GRS80;
        case PROJ_COORD_TYPE::UTM_51N_WGS84 :
        case PROJ_COORD_TYPE::UTM_52N_WGS84 :
        case PROJ_COORD_TYPE::LL_WGS84      :
        case PROJ_COORD_TYPE::MPS_WORLD	   :
            return PROJ_COORD_TYPE::LL_WGS84;
        case PROJ_COORD_TYPE::WCONGNAMUL	   :
            return PROJ_COORD_TYPE::WCONGNAMUL;

	}
    return PROJ_COORD_TYPE::LL_WGS84;
}

bool IsTM( PROJ_COORD_TYPE src )
{
    switch( src )
	{
        case PROJ_COORD_TYPE::TM_KOREA_MIDDLE :
        case PROJ_COORD_TYPE::TM_KOREA_EAST	:
        case PROJ_COORD_TYPE::TM_KOREA_EAST_SEA	 :
        case PROJ_COORD_TYPE::TM_KOREA_WEST   :
        case PROJ_COORD_TYPE::TM_KOREA_JEIJU	 :
        case PROJ_COORD_TYPE::TM_KATECH		 :
        case PROJ_COORD_TYPE::UTM_51N_BESSEL  :
        case PROJ_COORD_TYPE::UTM_52N_BESSEL  :
        case PROJ_COORD_TYPE::TM_GRS80_MIDDLE :
        case PROJ_COORD_TYPE::TM_GRS80_WEST   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST   :
        case PROJ_COORD_TYPE::TM_GRS80_JEIJU  :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_SEA :
		
        case PROJ_COORD_TYPE::TM_GRS80_MIDDLE_60 :
        case PROJ_COORD_TYPE::TM_GRS80_WEST_60   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_60   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_SEA_60 :

        case PROJ_COORD_TYPE::UTM_51N_GRS80 :
        case PROJ_COORD_TYPE::UTM_52N_GRS80 :
        case PROJ_COORD_TYPE::UTM_51N_WGS84 :
        case PROJ_COORD_TYPE::UTM_52N_WGS84 :
        case PROJ_COORD_TYPE::UTMK_GRS80	   :
        case PROJ_COORD_TYPE::UTMK_BESSEL   :

			return true;
        default :
            return false;
	}
	return false;
}

bool IsWRD( PROJ_COORD_TYPE src )
{
    if ( src == PROJ_COORD_TYPE::MPS_WORLD )
	{
		return true;
	}

	return false;
}

bool IsWCongnamul( PROJ_COORD_TYPE src )
{
    if ( src == PROJ_COORD_TYPE::WCONGNAMUL )
		return true;

	return false;
}

bool PROJ_IsTM( PROJ_COORD_TYPE src )
{
	switch( src ) 
	{
        case PROJ_COORD_TYPE::TM_KOREA_MIDDLE :
        case PROJ_COORD_TYPE::TM_KOREA_EAST	 :
        case PROJ_COORD_TYPE::TM_KOREA_EAST_SEA	 :
        case PROJ_COORD_TYPE::TM_KOREA_WEST   :
        case PROJ_COORD_TYPE::TM_KOREA_JEIJU	 :
        case PROJ_COORD_TYPE::TM_KATECH		 :
        case PROJ_COORD_TYPE::UTM_51N_BESSEL  :
        case PROJ_COORD_TYPE::UTM_52N_BESSEL  :
        case PROJ_COORD_TYPE::TM_GRS80_MIDDLE :
        case PROJ_COORD_TYPE::TM_GRS80_WEST   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST   :
        case PROJ_COORD_TYPE::TM_GRS80_JEIJU  :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_SEA :
		
        case PROJ_COORD_TYPE::TM_GRS80_MIDDLE_60 :
        case PROJ_COORD_TYPE::TM_GRS80_WEST_60   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_60   :
        case PROJ_COORD_TYPE::TM_GRS80_EAST_SEA_60 :

        case PROJ_COORD_TYPE::UTM_51N_GRS80 :
        case PROJ_COORD_TYPE::UTM_52N_GRS80 :
        case PROJ_COORD_TYPE::UTM_51N_WGS84 :
        case PROJ_COORD_TYPE::UTM_52N_WGS84 :
        case PROJ_COORD_TYPE::UTMK_GRS80	   :
        case PROJ_COORD_TYPE::UTMK_BESSEL   :
			return true;
        default :
            return false;
	}
	return false;
}

bool PROJ_Convert( PROJ_COORD_TYPE src, double src_x, double src_y, PROJ_COORD_TYPE dst, double *dst_x, double *dst_y )
{
	double x, y;
    PROJ_COORD_TYPE src_gp = GetGPType( src );
    PROJ_COORD_TYPE dst_gp = GetGPType( dst );

	if( IsTM( src ) ) 
	{
        TM_Inverse( TMParams[  static_cast<int>(src) ], src_x, src_y, x, y );
		src = src_gp;
	}
	else if ( IsWRD( src ) )
	{
		x = src_x / 524288; //( 256 * 2048 )
		y = src_y / 524288; //( 256 * 2048 )
	}
	else if ( IsWCongnamul( src ))
	{
		src_x /= 2.5;
		src_y /= 2.5;
        TM_Inverse( TMParams[ static_cast<int>(src) ], src_x, src_y, x, y );
        src_gp = PROJ_COORD_TYPE::LL_WGS84;
	}
	else 
	{
		x = src_x;
		y = src_y;
	}

	if( src==dst ) {
		*dst_x = x; 
		*dst_y = y;
		return true;
	}

	double z = 0.0;

	// Convert Datum
	double tx, ty, tz;
	if( src_gp != dst_gp ) 
	{
		switch( src_gp ) 
		{
            case PROJ_COORD_TYPE::LL_KOREA_BESSEL :
                if( dst_gp==PROJ_COORD_TYPE::LL_WGS84 ) BGP2WGP( y, x, z, ty, tx, tz );
				else				   BGP2GGP( y, x, z, ty, tx, tz );
				break;
            case PROJ_COORD_TYPE::LL_WGS84        :
                if( dst_gp==PROJ_COORD_TYPE::LL_KOREA_BESSEL ) WGP2BGP( y, x, z, ty, tx, tz );
				else					      { tx = x; ty = y; }
				break;
            case PROJ_COORD_TYPE::LL_GRS80       :
                if( dst_gp==PROJ_COORD_TYPE::LL_KOREA_BESSEL ) GGP2BGP( y, x, z, ty, tx, tz );
				else					      { tx = x; ty = y; }
				break;
			default :
				{ tx = x; ty = y; }
		}
	}
	else 
	{
		tx = x;
		ty = y;
	}

	// convert LonLat to local
	if( IsTM( dst ) ) 
        TM_Forword( TMParams[ static_cast<int>(dst) ], tx, ty, *dst_x, *dst_y );
	else if ( IsWRD( dst )  )
	{
		*dst_x = (int)((tx * 524288) + 0.5);
		*dst_y = (int)((ty * 524288) + 0.5);
	}
	else if ( IsWCongnamul( dst ) )
	{
        TM_Forword( TMParams[ static_cast<int>(dst) ], tx, ty, *dst_x, *dst_y );
		*dst_x *= 2.5;
		*dst_y *= 2.5;
	}
	else {
		*dst_x = tx;
		*dst_y = ty;
	}


	return true;
}

void CopyXYs( int numPoints, double *dst_x, double *dst_y, const double *src_x, const double *src_y )
{
	for( int n=0; n<numPoints; n++ ) {
		dst_x[ n ] = src_x[ n ];
		dst_y[ n ] = src_y[ n ];
	}
}

bool PROJ_Converts( int numPoints, PROJ_COORD_TYPE src, const double *src_x, const double *src_y, PROJ_COORD_TYPE dst, double *dst_x, double *dst_y )
{
	double *x, *y, *z;
	x = new double[ numPoints ];
	y = new double[ numPoints ];
	z = new double[ numPoints ];
	memset( z, 0x00, sizeof(double)*numPoints );


	PROJ_COORD_TYPE src_gp = GetGPType( src );
	PROJ_COORD_TYPE dst_gp = GetGPType( dst );

	if( IsTM( src ) ) 
	{
        TM_Inverses( TMParams[ static_cast<int>(src) ], numPoints, src_x, src_y, x, y );
		src = src_gp;
	}
	else if ( IsWRD( src ) )
	{	
		for ( int i = 0; i < numPoints; i++)
		{
			x[i] = src_x[i] / 524288; //( 256 * 2048 )
			y[i] = src_y[i] / 524288; //( 256 * 2048 );
		}
		
	}
	else if ( IsWCongnamul( src ))
	{
		for ( int i = 0; i < numPoints; i++)
		{
			x[i] = src_x[i] / 2.5;
			y[i] = src_y[i] / 2.5;
		}
        TM_Inverses( TMParams[ static_cast<int>(src) ], numPoints, x, y, x, y );
        src_gp = PROJ_COORD_TYPE::LL_WGS84;
	}
	else
	{
		CopyXYs( numPoints, x, y, src_x, src_y );
	}

	if( src==dst ) 
	{
		CopyXYs( numPoints, dst_x, dst_y, x, y );
		delete []x;
		delete []y;
		delete []z;
		return true;
	}

	// Convert Datum
	double *tx, *ty, *tz;
	tx = new double [ numPoints ];
	ty = new double [ numPoints ];
	tz = new double [ numPoints ];
	memset( z, 0x00, sizeof(double)*numPoints );
	
	if( src_gp!=dst_gp ) 
	{
		switch( src_gp ) 
		{
            case PROJ_COORD_TYPE::LL_KOREA_BESSEL :
                if( dst_gp==PROJ_COORD_TYPE::LL_WGS84 ) BGP2WGPs( numPoints, y, x, z, ty, tx, tz );
				else				   BGP2GGPs( numPoints, y, x, z, ty, tx, tz );
				break;
            case PROJ_COORD_TYPE::LL_WGS84        :
                if( dst_gp==PROJ_COORD_TYPE::LL_KOREA_BESSEL ) WGP2BGPs( numPoints, y, x, z, ty, tx, tz );
				else					      { CopyXYs( numPoints, tx, ty, x, y ); }
				break;
            case PROJ_COORD_TYPE::LL_GRS80       :
                if( dst_gp==PROJ_COORD_TYPE::LL_KOREA_BESSEL ) GGP2BGPs( numPoints, y, x, z, ty, tx, tz );
				else					      { CopyXYs( numPoints, tx, ty, x, y ); }
				break;
			default :
				CopyXYs( numPoints, tx, ty, x, y );
				break;
		}
	}
	else 
	{
		CopyXYs( numPoints, tx, ty, x, y );
	}

	// convert LonLat to local
	if( IsTM( dst ) ) 
        TM_Forwords( TMParams[ static_cast<int>(dst) ], numPoints, tx, ty, dst_x, dst_y );
	else if ( IsWRD( dst ) )
	{
		for ( int i = 0; i < numPoints; i++)
		{
			dst_x[i] = (int) (tx[i] * 524288 + 0.5); //( 256 * 2048 )
			dst_y[i] = (int) (ty[i] * 524288 + 0.5); //( 256 * 2048 );
		}
		
	}
	else if ( IsWCongnamul( dst ) )
	{
        TM_Forwords( TMParams[ static_cast<int>(dst) ], numPoints, tx, ty, dst_x, dst_y );
		for ( int i = 0; i < numPoints; i++)
		{
			dst_x[i] *= 2.5;
			dst_y[i] *= 2.5;
		}

	}
	else
	{
		CopyXYs( numPoints, dst_x, dst_y, tx, ty );
	}

	delete []tx;
	delete []ty;
	delete []tz;
	delete []x;
	delete []y;
	delete []z;

	return true;
}

