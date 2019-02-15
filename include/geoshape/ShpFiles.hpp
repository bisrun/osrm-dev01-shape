#pragma once
#include "geoshape/ESRIShapeFile.hpp"
#include <vector>
#include <map>
#include <list>
class CShpFiles : public CESRIShapeFile
{
public:

	CShpFiles(int nMaxDepth=7);
	virtual ~CShpFiles(void);
    string m_strFilePath;

	int				AddRecord();
	CESRIShapeFile * CreateShpFile(char *pFilePath);
};
//////////////////////////////////////////////////////////////////////////

struct STRouteUnit
{
	int	iShpIdx;
	int	iRouteIdx;
	double dfDistance;
	double dfDuration;
	double dfWeight;

	STRouteUnit() { clear(); }
	void clear() { memset(this, 0x00, sizeof(STRouteUnit)); }
};
typedef map<int, STRouteUnit > TMapRouteTrace;
typedef vector<STRouteUnit> TVecRouteUnit;
class CSFRouteUnit:public CESRIShapeFile
{
public:
	string m_strFilePath;
	CSFRouteUnit(int nMaxDepth = 7);
	virtual ~CSFRouteUnit(void);

	int				AddRecord(CSHPObject *pShpObj, STRouteUnit *pDbf);
	int				ReadRecord(int row_idx, CSHPObject **ppShpObj, STRouteUnit * pDbf);
	bool			CreateShpFile(char *pFilePath);
	int				LoadData(TMapRouteTrace * pmapLink, TVecRouteUnit * pvecRoute, CArrSHPObject * pvecShpObj);
};


//////////////////////////////////////////////////////////////////////////

struct STViaUnit
{
	int	iShpIdx;
	int	iViaIdx;
	int iPermutation;

	STViaUnit() { clear(); }
	void clear() { memset(this, 0x00, sizeof(STViaUnit)); }
};

class CSFViaUnit :public CESRIShapeFile
{
public:
	string m_strFilePath;
	CSFViaUnit(int nMaxDepth = 7);
	virtual ~CSFViaUnit(void);

	int				AddRecord(CSHPObject *pShpObj, STViaUnit *pDbf);
	int				ReadRecord(int row_idx, CSHPObject **ppShpObj, STViaUnit * pDbf);
	bool			CreateShpFile(char *pFilePath);
};
