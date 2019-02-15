#ifndef _TRANSDATUM7_H
#define _TRANSDATUM7_H

// WGS84 <=> BESSEL
void WGP2BGP( double xw, double yw, double zw, double &xb, double &yb, double &zb );
void BGP2WGP( double xb, double yb, double zb, double &xw, double &yw, double &zw );
void WGP2BGPs( int numPoints, const double *xw, const double *yw, const double *zw, double *xb, double *yb, double *zb );
void BGP2WGPs( int numPoints, const double *xb, const double *yb, const double *zb, double *xw, double *yw, double *zw );

// GRS80 <=> BESSEL
void GGP2BGP( double xw, double yw, double zw, double &xb, double &yb, double &zb );
void BGP2GGP( double xb, double yb, double zb, double &xw, double &yw, double &zw );

void GGP2BGPs( int numPoints, const double *xw, const double *yw, const double *zw, double *xb, double *yb, double *zb );
void BGP2GGPs( int numPoints, const double *xb, const double *yb, const double *zb, double *xw, double *yw, double *zw );

void GGP2BGP_( double x, double y, double z, double &tx, double &ty, double &tz );
void BGP2GGP_( double x, double y, double z, double &tx, double &ty, double &tz );

#endif

