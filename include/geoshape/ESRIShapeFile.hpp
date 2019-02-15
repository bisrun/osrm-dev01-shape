// ESRIShapeFile.h: interface for the CESRIShapeFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ESRISHAPEFILE_H__CF8C8C0A_6E29_4E84_A171_091870A43C61__INCLUDED_)
#define AFX_ESRISHAPEFILE_H__CF8C8C0A_6E29_4E84_A171_091870A43C61__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "geoshape/GRect.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <mutex>          // std::mutex
#include <iostream>
using namespace std ;




const double		ESRI_MEC_PI = (3.14159265358979323846);
const double		ESRI_MEC_DEGREE = (3.14159265358979323846 / 180.0);
const double		ESRI_MEC_IDEGREE = (180.0 / 3.14159265358979323846);

typedef enum { stUNKNOWN, stPOINT, stPOLYLINE, stPOLYGON, stGRID } SHAPETYPE;

/************************************************************************/
/*                        Configuration options.                        */
/************************************************************************/

/* -------------------------------------------------------------------- */
/*      Should the DBFReadStringAttribute() strip leading and           */
/*      trailing white space?                                           */
/* -------------------------------------------------------------------- */
#define TRIM_DBF_WHITESPACE

/* -------------------------------------------------------------------- */
/*      Should we write measure values to the Multipatch object?        */
/*      Reportedly ArcView crashes if we do write it, so for now it     */
/*      is disabled.                                                    */
/* -------------------------------------------------------------------- */
#define DISABLE_MULTIPATCH_MEASURE

/* -------------------------------------------------------------------- */
/*      Shape types (nSHPType)                                          */
/* -------------------------------------------------------------------- */
#define SHPT_NULL	0
#define SHPT_POINT	1
#define SHPT_LINE	3
#define SHPT_POLYGON	5
#define SHPT_MULTIPOINT	8
#define SHPT_POINTZ	11
#define SHPT_ARCZ	13
#define SHPT_POLYGONZ	15
#define SHPT_MULTIPOINTZ 18
#define SHPT_POINTM	21
#define SHPT_ARCM	23
#define SHPT_POLYGONM	25
#define SHPT_MULTIPOINTM 28
#define SHPT_MULTIPATCH 31

/* -------------------------------------------------------------------- */
/*      Part types - everything but SHPT_MULTIPATCH just uses           */
/*      SHPP_RING.                                                      */
/* -------------------------------------------------------------------- */

#define SHPP_TRISTRIP	0
#define SHPP_TRIFAN	1
#define SHPP_OUTERRING	2
#define SHPP_INNERRING	3
#define SHPP_FIRSTRING	4
#define SHPP_RING	5


typedef enum {
	FTString,
	FTInteger,
	FTDouble,
	FTInvalid
} DBFFieldType;


/* -------------------------------------------------------------------- */
/*      CSHPObject - represents on shape (without attributes) read       */
/*      from the .shp file.                                             */
/* -------------------------------------------------------------------- */
class CSHPObject
{
public:
    CSHPObject()
    {
        nSHPType = 0;

        nShapeId = 0; /* -1 is unknown/unassigned */

        nParts = 0;
        panPartStart = 0 ;
        panPartType = 0;

        vertex_cnt = 0;
        pfX = 0;
        pfY = 0;
        pfZ = 0;
        pfM = 0;

        fxMin = 0;
        fyMin = 0;
        fzMin = 0;
        fmidMin = 0;

        fxMax = 0;
        fyMax = 0;
        fzMax = 0;
        fmidMax = 0;
    };
	~CSHPObject();

	void Copy(CSHPObject *pShp);


public:
	int		nSHPType;

	int		nShapeId; /* -1 is unknown/unassigned */

	int		nParts;
	int		*panPartStart;
	int		*panPartType;

	int		vertex_cnt;
	double	*pfX;
	double	*pfY;
	double	*pfZ;
	double	*pfM;

	double	fxMin;
	double	fyMin;
	double	fzMin;
	double	fmidMin;

	double	fxMax;
	double	fyMax;
	double	fzMax;
    double	fmidMax;

};

typedef std::vector<CSHPObject*> CArrSHPObject;
//typedef vector<Cunsigned intArray*> CArrShpIdxList;


class CESRIShapeFile
{
public:
	CSHPObject * SHPCreateObject(int nSHPType, int nShapeId, int nParts, int * panPartStart, int * panPartType,
		int vertex_cnt, double * pfX, double * pfY, double * pfZ, double * pfM, GPOINT *pt = NULL);
	CSHPObject * SHPCreateSimpleObject(int nSHPType, int vertex_cnt, double * pfX, double * pfY, double * pfZ, GPOINT *pt = NULL);
	void SHPComputeExtents(CSHPObject * psObject);

	bool	HoleOuterPG(double *rgX, double *rgY, int ofs, int iVtxCnt);
	bool	GetOuterRingFlag_ShapePolyPart(CSHPObject *pShp, int *pOuter, int iOuterBufLen);

public:

	void SortByColumn(int nField, int* anSortedLink);

	void operator+=(CESRIShapeFile& o);
	CESRIShapeFile(int nMaxDepth = 7);
	virtual ~CESRIShapeFile();

	int GetNaviIndexFromFile(const string& sFileName);

    struct SHPInfo
	{
		FILE    *fpSHP;
		FILE	*fpSHX;

		int		nShapeType;				/* SHPT_* */

		int		nFileSize;				/* SHP file */

		int     nRecords;
		int		nMaxRecords;
		int		*panRecOffset;
		int		*panRecSize;

		double	adBoundsMin[4];
		double	adBoundsMax[4];

		int		bUpdated;
	};

	typedef SHPInfo * SHPHandle;

	/************************************************************************/
	/*                             DBF Support.                             */
	/************************************************************************/
    struct DBFInfo
	{
		FILE	*fp;

		int     nRecords;

		int		nRecordLength;
		int		nHeaderLength;
		int		nFields;
		int		*panFieldOffset;
		int		*panFieldSize;
		int		*panFieldDecimals;
		char	*pachFieldType;

		char	*pszHeader;

		int		nCurrentRecord;
		int		bCurrentRecordModified;
		char	*pszCurrentRecord;

		int		bNoHeader;
		int		bUpdated;
	};

	typedef DBFInfo * DBFHandle;

#define XBASE_FLDHDR_SZ       32

	/* -------------------------------------------------------------------- */
	/*      Shape quadtree indexing API.                                    */
	/* -------------------------------------------------------------------- */

	/* this can be two or four for binary or quad tree */
#define MAX_SUBNODE	4

	typedef struct shape_tree_node
	{
		/* region covered by this node */
		double	adfBoundsMin[4];
		double	adfBoundsMax[4];

		/* list of shapes stored at this node.  The papsShapeObj pointers
		or the whole list can be NULL */
		int		nOffset;//루트 노드로부터 순회할 때의 nShapeCount의 누적 값. .mbr파일 인덱싱용으로 사용
		int		nShapeCount;
		int		*panShapeIds;
		CSHPObject   **papsShapeObj;

		int		nSubNodes;
		struct shape_tree_node *apsSubNode[MAX_SUBNODE];

	} SHPTreeNode;

    struct SHPTree
	{
		SHPHandle   hSHP;

		int		nMaxDepth;
		int		nDimension;

		SHPTreeNode	*psRoot;
	};

protected:
	ifstream m_mbrFile;
	int m_nRef;
	string m_sShapeFilePath;
	DBFHandle	m_hDBF;
	SHPHandle	m_hSHP;
	SHPTree	*m_hTree; //공간 인덱싱용 트리
	SHAPETYPE	m_Type; //shape 타입

	int m_nEntities;	//A pointer to an integer into which the number of
						//entities/structures should be placed.  May be NULL.
	int pnShapeType;	//A pointer to an integer into which the shapetype
						//of this file should be placed.  Shapefiles may contain
						//either SHPT_POINT, SHPT_LINE, SHPT_POLYGON or 
						//SHPT_MULTIPOINT entities.  This may be NULL.
	double padfMinBound[4];	//The X, Y, Z and M minimum values will be placed into
							//this four entry array.  This may be NULL.
	double padfMaxBound[4];	//The X, Y, Z and M maximum values will be placed into
							//this four entry array.  This may be NULL.
	DBFFieldType m_FieldType;
	int m_nField;
	GRect	m_grBoundary;
	int	m_nMaxDepth;
	int	m_nStringFieldLen;
	char * m_szStringField;
	double	m_retvalDoubleField;
    int64_t m_retvalLongLongField;
	int		m_retvalIntField;

public:
	int       DBFGetFieldIndex(const char *pszFieldName);
	void Close();
	//NaviIndex를 리턴한다
	int GetNaviIndex();

	ifstream * GetMBRFile();
	int Release();
	int AddRef();
	string GetShapeFilePath();
	SHAPETYPE GetShapeType();
	GRect GetBoundary();
	int GetEntityCount() const;
	int GetLabelFieldIndex();

	SHPHandle		SHPOpen(const char* pszShapeFile, const char* pszAccess);
	SHPHandle		SHPOpenW(const string & strLayer, const char* pszAccess);
	CESRIShapeFile*	Open(const string & sShapeFileName, bool bCreateTree = true, const char* pszAccess = "r");

	CSHPObject*		SHPReadObject(int iShape, bool bSetOuterRing = false);
	int				SHPWriteObject(int iShape, CSHPObject* psObject);

	SHPHandle		GetSHPHandle();
	SHPTree*		GetSHPTree();
	int* SHPMBRShapes(double * padfBoundsMin, double * padfBoundsMax, int * pnShapeCount, ifstream* pMBR, bool bSortByBottom = false);

	int* SHPMBRShapes2(double * padfBoundsMin, double * padfBoundsMax, int * pnShapeCount, CArrSHPObject *pArrShpObj);

	int 	DBFReadIntegerAttribute(int iShape, int iField);
	long long  	DBFReadInt64Attribute(int iShape, int iField);
	double 	DBFReadDoubleAttribute(int iShape, int iField);
	const char * DBFReadStringAttribute(int iShape, int iField);
	int           DBFIsAttributeNULL(int iShape, int iField);
	int	      DBFGetFieldCount();
	DBFFieldType DBFGetFieldInfo(int iField, char * pszFieldName, int * pnWidth, int * pnDecimals);
	int	      DBFGetRecordCount();
	string GetQTreeFileName(const string& sShpFilePath);
	bool CreateQTreeFile(const char* szFileName);
	string GetMBRFileName(const string &sShpFilePath);
	bool CreateMBRFile(const char *szFileName);
	void WriteMBRFile(SHPTreeNode * psTreeNode, ofstream & ar);
	bool SHPCreate(const char * pszShapeFile, int nShapeType);
	bool SHPCreateW(const string & strLayer, int nShapeType);
	void SHPClose();
	int	      DBFAddField(const char * pszFieldName, DBFFieldType eType, int nWidth, int nDecimals);

	int       DBFWriteAttribute(int iShape, int iField, int64_t nFieldValue);

	int       DBFWriteAttribute(int iShape, int iField, int nFieldValue);
	int       DBFWriteAttribute(int iShape, int iField, double dFieldValue);
	int       DBFWriteAttribute(int iShape, int iField, const char * pszFieldValue);
	int      DBFWriteAttribute(int iShape, int iField);

public:
	int m_nDBFReadTupleLen;
	char * m_pDBFReturnTuple;
	inline bool FileExists(const char* FileName);		/// 파일 존재여부 확인

	std::mutex m_csQueueLock;
	/* -------------------------------------------------------------------- */
	/*      SHP API Prototypes                                              */
	/* -------------------------------------------------------------------- */
	void       SHPGetInfo(int * pnEntities, int * pnShapeType,
		double * padfMinBound, double * padfMaxBound);

	//void SHPDestroyObject( CSHPObject * psObject );

	const char * SHPTypeName(int nSHPType);
	const char * SHPPartTypeName(int nPartType);

	SHPTree* SHPCreateTree(int nDimension, int nMaxDepth,
		double *padfBoundsMin, double *padfBoundsMax, const char * szQTreeFileName);

	SHPTree * SHPCreateTree(int nDimension, int nMaxDepth,
		double *padfBoundsMin, double *padfBoundsMax);


	void FillOffset(SHPTree * psTree);
	void FillOffsetToNode(SHPTreeNode * psTreeNode, int* pnSum);

	void    SHPDestroyTree(SHPTree * hTree);

	int		SHPWriteTree(SHPTree *hTree, const char * pszFilename);
	SHPTree SHPReadTree(const char * pszFilename);

	int	      SHPTreeAddObject(SHPTree * hTree, CSHPObject * psObject);
	int	      SHPTreeAddShapeId(SHPTree * hTree, CSHPObject * psObject);
	int	      SHPTreeRemoveShapeId(SHPTree * hTree, int nShapeId);

	void 	SHPTreeTrimExtraNodes(SHPTree * hTree);

	int    * SHPTreeFindLikelyShapes(SHPTree * hTree,
		double * padfBoundsMin,
		double * padfBoundsMax,
		int *);
	int     SHPCheckBoundsOverlap(double *, double *, double *, double *, int);
	bool	ReverseVtxOrder(CSHPObject *pShp);
	CSHPObject * MergeLineShape(CSHPObject * pShpObj, CSHPObject * pShpDelObj, unsigned char cMergeFlag);
	int			LoadShpObj(CArrSHPObject &arShp);
	int			GetCenterPtOfLine(CSHPObject *pShpObj, GPOINT & pt, double * pLNAngle = NULL);
	static void	ReleaseShp(CESRIShapeFile **ppSF, CArrSHPObject * pArShp);
#define XBASE_FLDHDR_SZ       32
	DBFHandle DBFOpen(const char * pszDBFFile, const char * pszAccess);
	DBFHandle DBFOpenW(const string & strFileName, const char * pszAccess);

	DBFHandle DBFCreate(const char * pszDBFFile);
	DBFHandle DBFCreateW(const string & strFilename);


	const char * DBFReadTuple(int hEntity);
	int       DBFWriteTuple(int hEntity, void * pRawTuple);

	DBFHandle       DBFCloneEmpty(const char * pszFilename);

	void	      DBFClose();
	char          DBFGetNativeFieldType(int iField);

	//void ReadTreeNodeFromFile(SHPTreeNode * psTreeNode, CArchive& ar);
	void ReadTreeNodeFromFile(SHPTreeNode * psTreeNode, ifstream& ar);

	void * SfRealloc(void * pMem, int nNewSize);
	void DBFWriteHeader();
	void DBFFlushRecord();
	void* DBFReadAttribute(int hEntity, int iField, char chReqType);
	int DBFWriteAttribute(int hEntity, int iField, void * pValue);
	double GetLength(CSHPObject *pShpObj);
	int	GetPtOnLine(CSHPObject *pShpObj, STPtOnLine &kPtOnLine, double dfLimitDist);
	static inline double GetCrossProduct(GPOINT &sPoint1, GPOINT &sPoint2, GPOINT &sPoint3);

	static bool IsPolygonConvex(double *rgX, double *rgY, int nStartVertex, int nPartVertexCount);
	static bool IsPolygonClockWise(double *rgX, double *rgY, int nStartVertex, int nPartVertexCount);

public:
	void SHPTreeCollectShapeIds(SHPTreeNode * psTreeNode,
		double * padfBoundsMin, double * padfBoundsMax,
		int * pnShapeCount, int * pnMaxShapes, int* pnMaxNodes,
		int ** ppanShapeList, int ** ppanOffsetList, int ** ppanCountList, int* pnNodeCount);
private:
	void QuickSort(int* anShapeList, long l, long r);
	void QuickSort2(string* asValueList, int* anShapeList, long l, long r);
	void BottomSort(int* anShapeList, double* pdBottoms, long l, long r);
	void SwapShapeID(int *i, int *j);
	void SwapString(string *i, string *j);
	void SwapBottom(double *i, double *j);
	void WriteTreeNodeToFile(SHPTreeNode * psTreeNode, ofstream& ar);
	void SwapWord(int length, void * wordP);

	void	_SHPSetBounds(unsigned char * pabyRec, CSHPObject * psShape);
	void	SHPWriteHeader();
	void SHPDestroyTreeNode(SHPTreeNode * psTreeNode);
	int SHPTreeNodeTrim(SHPTreeNode * psTreeNode);
	CESRIShapeFile::SHPTreeNode *SHPTreeNodeCreate(double * padfBoundsMin, double * padfBoundsMax);
	int SHPTreeNodeAddShapeId(SHPTreeNode * psTreeNode, CSHPObject * psObject,
		int nMaxDepth, int nDimension);
	int SHPCheckObjectContained(CSHPObject * psObject, int nDimension,
		double * padfBoundsMin, double * padfBoundsMax);
	void SHPTreeSplitBounds(double *padfBoundsMinIn, double *padfBoundsMaxIn,
		double *padfBoundsMin1, double * padfBoundsMax1,
		double *padfBoundsMin2, double * padfBoundsMax2);
};

#endif // !defined(AFX_ESRISHAPEFILE_H__CF8C8C0A_6E29_4E84_A171_091870A43C61__INCLUDED_)


