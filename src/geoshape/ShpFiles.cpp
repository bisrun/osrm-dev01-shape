#include "geoshape/ShpFiles.hpp"
#include "geoshape/ESRIShapeFile.hpp"
#include "geoshape/Proj/Projection.h"
CShpFiles::CShpFiles(int nMaxDepth):CESRIShapeFile(nMaxDepth)
{

}

CShpFiles::~CShpFiles(void)
{
}

CESRIShapeFile * CShpFiles::CreateShpFile(char *pFilePath)
{
    m_strFilePath = pFilePath;
	return NULL;
}

int CShpFiles::AddRecord()
{
	return 0;
}
//////////////////////////////////////////////////////////////////////////


CSFRouteUnit::CSFRouteUnit(int nMaxDepth) :CESRIShapeFile(nMaxDepth)
{

}

CSFRouteUnit::~CSFRouteUnit()
{

}
bool  CSFRouteUnit::CreateShpFile(char *pFilePath)
{
    m_strFilePath = pFilePath;
	try
	{
		if (SHPCreate(pFilePath, SHPT_LINE) == NULL)
		{
			throw - 1;
		}

		if (DBFCreate(pFilePath) == NULL)
		{
			throw - 12;
		}

		DBFAddField("RPIDX", FTInteger, 3, 0);
		DBFAddField("DIST", FTDouble, 7, 1);
		DBFAddField("DURA", FTDouble, 9, 3);
		DBFAddField("WEIGHT", FTDouble, 9, 3);

	}
	catch (int e)
	{
		if (e < 0)
		{
			return false;
		}
	}

	return true;
}


int		 CSFRouteUnit::AddRecord(CSHPObject *pShpObj, STRouteUnit *pDbf)
{
#if 1
	int		fld_idx = 0;

	int row_ofs = SHPWriteObject(-1, pShpObj);
	pDbf->iShpIdx = row_ofs;
	//GetCenterPtOfLine(pShpObj, pDbf->kCenterPt, &dfLNAngle);
	//pDbf->iLNAngle = (int)dfLNAngle;
	//PROJ_Convert(TM_KATECH, pDbf->kCenterPt.x, pDbf->kCenterPt.y, LL_WGS84, &pDbf->kCenterPtWgs.x, &pDbf->kCenterPtWgs.y);

	DBFWriteAttribute(row_ofs, fld_idx++, pDbf->iRouteIdx);
	DBFWriteAttribute(row_ofs, fld_idx++, pDbf->dfDistance);
	DBFWriteAttribute(row_ofs, fld_idx++, pDbf->dfDuration);
	DBFWriteAttribute(row_ofs, fld_idx++, pDbf->dfWeight);


#endif
	return row_ofs;

}

int		 CSFRouteUnit::ReadRecord(int row_ofs, CSHPObject **ppShpObj, STRouteUnit * pDbf)
{
	int		fld_idx = 0;

	if (row_ofs < 0 || row_ofs >= GetEntityCount())
		return -1;

	if (ppShpObj != NULL)
	{
		*ppShpObj = SHPReadObject(row_ofs);
	}

	pDbf->iShpIdx = row_ofs;
	pDbf->iRouteIdx = DBFReadIntegerAttribute(row_ofs, fld_idx++);
	pDbf->dfDistance = DBFReadDoubleAttribute(row_ofs, fld_idx++);
	pDbf->dfDuration = DBFReadDoubleAttribute(row_ofs, fld_idx++);
	pDbf->dfWeight = DBFReadDoubleAttribute(row_ofs, fld_idx++);


	return row_ofs;
}


int			CSFRouteUnit::LoadData(TMapRouteTrace * pmapRoute, TVecRouteUnit * pvecRoute, CArrSHPObject * pvecShpObj)
{
	int iCount = GetEntityCount();
	STRouteUnit kRoute;
	CSHPObject *pShpObj = 0;
	
	if (pvecRoute)
	{
		pvecRoute->reserve(iCount);
	}
	if (pvecShpObj)
	{
		pvecShpObj->reserve(iCount);

	}


	for (int loop1 = 0; loop1 < iCount; loop1++)
	{
		if (pvecShpObj)
		{
			if (ReadRecord(loop1, &pShpObj, &kRoute) < 0)
				return -1;
		}
		else
		{
			if (ReadRecord(loop1, NULL, &kRoute) < 0)
				return -1;
		}

		if (pvecShpObj)
		{
			pvecShpObj->at(loop1) = pShpObj;
		}

		if (pmapRoute)
		{
			pmapRoute->insert ( make_pair(kRoute.iRouteIdx, kRoute));
		}
		if (pvecShpObj)
		{
			(*pvecRoute)[loop1] = kRoute;
		}

	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////



CSFViaUnit::CSFViaUnit(int nMaxDepth) :CESRIShapeFile(nMaxDepth)
{

}

CSFViaUnit::~CSFViaUnit()
{

}
bool  CSFViaUnit::CreateShpFile(char *pFilePath)
{
	string m_strFilePath = pFilePath;
	try
	{
		if (SHPCreate(pFilePath, SHPT_POINT) == NULL)
		{
			throw - 1;
		}

		if (DBFCreate(pFilePath) == NULL)
		{
			throw - 12;
		}

		DBFAddField("VIAIDX", FTInteger, 3, 0);
		DBFAddField("ORDER", FTInteger, 3, 0);
		
	}
	catch (int e)
	{
		if (e < 0)
		{
			return false;
		}
	}

	return true;
}


int		 CSFViaUnit::AddRecord(CSHPObject *pShpObj, STViaUnit *pDbf)
{
#if 1
	int		fld_idx = 0;

	int row_ofs = SHPWriteObject(-1, pShpObj);
	pDbf->iShpIdx = row_ofs;
	//GetCenterPtOfLine(pShpObj, pDbf->kCenterPt, &dfLNAngle);
	//pDbf->iLNAngle = (int)dfLNAngle;
	//PROJ_Convert(TM_KATECH, pDbf->kCenterPt.x, pDbf->kCenterPt.y, LL_WGS84, &pDbf->kCenterPtWgs.x, &pDbf->kCenterPtWgs.y);

	DBFWriteAttribute(row_ofs, fld_idx++, pDbf->iViaIdx);
	DBFWriteAttribute(row_ofs, fld_idx++, pDbf->iPermutation);
	
#endif
	return row_ofs;

}

int		 CSFViaUnit::ReadRecord(int row_ofs, CSHPObject **ppShpObj, STViaUnit * pDbf)
{
	int		fld_idx = 0;

	if (row_ofs < 0 || row_ofs >= GetEntityCount())
		return -1;

	if (ppShpObj != NULL)
	{
		*ppShpObj = SHPReadObject(row_ofs);
	}

	pDbf->iShpIdx = row_ofs;
	pDbf->iShpIdx = DBFReadIntegerAttribute(row_ofs, fld_idx++);
	pDbf->iPermutation = DBFReadDoubleAttribute(row_ofs, fld_idx++);
	return row_ofs;
}
