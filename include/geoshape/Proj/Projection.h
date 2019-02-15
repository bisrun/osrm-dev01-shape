#ifndef _PROJ_H
#define _PROJ_H

enum class PROJ_COORD_TYPE {
	TM_KOREA_MIDDLE   = 0,
	TM_KOREA_WEST,
	TM_KOREA_EAST,
	TM_KOREA_JEIJU,
	TM_KOREA_EAST_SEA,
	TM_KATECH,
	
	//세계측지계 ( 200000, 500000 )
	TM_GRS80_MIDDLE,
	TM_GRS80_WEST,
	TM_GRS80_EAST,
	TM_GRS80_JEIJU,
	TM_GRS80_EAST_SEA,

	//세계측지계 변경( 200000, 600000 )
	TM_GRS80_MIDDLE_60,
	TM_GRS80_WEST_60,
	TM_GRS80_EAST_60,
	TM_GRS80_EAST_SEA_60,

	UTM_51N_BESSEL,
	UTM_52N_BESSEL,
	UTM_51N_WGS84,
	UTM_52N_WGS84,
	UTM_51N_GRS80,
	UTM_52N_GRS80,
	UTMK_GRS80,
	UTMK_BESSEL,
	
	WCONGNAMUL,			//WCONGNAMUL Daum 지도 좌표

	LL_KOREA_BESSEL,
	LL_WGS84,
	LL_GRS80,
	
	MPS_WORLD,			//맵퍼스 월드 좌표 ( LL_WGS84 * 256 * 2048 )
};

bool PROJ_Convert( PROJ_COORD_TYPE src, double src_x, double src_y, PROJ_COORD_TYPE dst, double *dst_x, double *dst_y );
bool PROJ_Converts( int numPoints, PROJ_COORD_TYPE src, const double *src_x, const double *src_y, PROJ_COORD_TYPE dst, double *dst_x, double *dst_y );
bool PROJ_IsTM( PROJ_COORD_TYPE src );


#endif

