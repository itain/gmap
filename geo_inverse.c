#include "gmap.h"

#define WGS84_A		6378137.0
#define WGS84_f		(1.0/298.257223563)

/* lat, lon are in radians! */
double
_Distance(double lat1, double lon1, double lat2, double lon2) {
	double a = WGS84_A;
	double f = WGS84_f;

	double eps = 1.0e-23;
	double max_loop_count = 10;

	double r = 1.0 - f;

	double tu1 = r * sin(lat1) / cos(lat1);
	double tu2 = r * sin(lat2) / cos(lat2);
	double cu1 = 1.0 / hypot(tu1, 1.0);
	double su1 = cu1 * tu1;
	double cu2 = 1.0 / hypot(tu2, 1.0);
	double s = cu1 * cu2;
	double baz = s * tu2;
	double faz = baz * tu1;
	double dlon = lon2 - lon1;

	double x = dlon;
	int cnt = 0;
	double c2a, c, cx, cy, cz, d, del, e, sx, sy, y;
	do {
		double sa;
		sx = sin(x);
		cx = cos(x);
		tu1 = cu2*sx;
		tu2 = baz - (su1*cu2*cx);
		sy = hypot(tu1, tu2);
		cy = s*cx + faz;
		y = atan2(sy,cy);
		if( sy == 0.0 ) {
			sa = 1.0;
		}else{
			sa = (s*sx) / sy;
		}
		c2a = 1.0 - (sa*sa);
		cz = faz + faz;
		if( c2a > 0.0 ) {
			cz = ((-cz)/c2a) + cy;
		}
		e = ( 2.0 * cz * cz ) - 1.0;
		c = ( ((( (-3.0 * c2a) + 4.0)*f) + 4.0) * c2a * f )/16.0;
		d = x;
		x = ( (e * cy * c + cz) * sy * c + y) * sa;
		x = ( 1.0 - c ) * x * f + dlon;
		del = d - x;

	} while( (abs(del) > eps) && ( ++cnt <= max_loop_count ) );

	faz = atan2(tu1,tu2);
	baz = atan2(cu1*sx,(baz*cx - su1*cu2)) + M_PI;
	x = sqrt( ((1.0/(r*r)) -1.0 ) * c2a+1.0 ) + 1.0;
	x = (x-2.0)/x;
	c = 1.0 - x;
	c = ((x*x)/4.0 + 1.0)/c;
	d = ((0.375*x*x) - 1.0)*x;
	x = e*cy;

	s = 1.0 - e - e;
	s = (((((((( sy * sy * 4.0 ) - 3.0) * s * cz * d/6.0) - x) * 
		d /4.0) + cz) * sy * d) + y ) * c * a * r;

	/*  adjust azimuth to (0,360) */
	if(faz < 0)
		faz += 2 * M_PI;

	/* return(faz, baz, s); */
	return s;
}

#define RAD(x) ((x)*M_PI/180.)
double
Distance(double lat1, double lon1, double lat2, double lon2) {
	return _Distance(RAD(lat1), RAD(lon1), RAD(lat2), RAD(lon2));
}
