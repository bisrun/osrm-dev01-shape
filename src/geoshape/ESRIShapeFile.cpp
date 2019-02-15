// ESRIShapeFile.cpp: implementation of the CESRIShapeFile class.
//
//////////////////////////////////////////////////////////////////////

#include "geoshape/ESRIShapeFile.hpp"
#include <math.h>
#include <stdlib.h>
#include <iostream>  
#include <fstream>
#include <ostream>
#include <sys/stat.h>

#include <assert.h>  
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static int 	bBigEndian;
typedef int INT32;

#define ByteCopy( a, b, c )	memcpy( b, a, c )
#ifndef MAX
#  define MIN(a,b)      ((a<b) ? a : b)
#  define MAX(a,b)      ((a>b) ? a : b)
#endif

/* -------------------------------------------------------------------- */
/*      If the following is 0.5, nodes will be split in half.  If it    */
/*      is 0.6 then each subnode will contain 60% of the parent         */
/*      node, with 20% representing overlap.  This can be help to       */
/*      prevent small objects on a boundary from shifting too high      */
/*      up the tree.                                                    */
/* -------------------------------------------------------------------- */
#define SHP_SPLIT_RATIO	0.55

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSHPObject::~CSHPObject()
{
	if(panPartStart)
		delete[] panPartStart;
	if(panPartType)
		delete[] panPartType;
	
	delete[] pfX;
	delete[] pfY;
	delete[] pfZ;
	delete[] pfM;
}



void CSHPObject::Copy( CSHPObject *pShp )
{
	int loop1 = 0 ;
	nSHPType = pShp->nSHPType ;
	nShapeId = pShp->nShapeId ;
	nParts = pShp->nParts ;

	if ( panPartStart )	{ delete []panPartStart; panPartStart = 0;}
	if ( panPartType ){	delete []panPartType; panPartType = 0;}

	panPartStart = new int[nParts];
	panPartType = new int[nParts];

	for ( loop1  = 0; loop1 < nParts ;loop1 ++ )
	{
		panPartStart[loop1] = pShp->panPartStart[loop1];
		panPartType[loop1] = pShp->panPartType[loop1];
	}


	if ( pfX )	{	delete []pfX;	pfX = 0;}
	if ( pfY )	{	delete []pfY;	pfY = 0;}
	if ( pfZ )	{	delete []pfZ;	pfZ = 0;}
	if ( pfM )	{	delete []pfM;	pfM = 0;}


	vertex_cnt = pShp->vertex_cnt ;

	pfX = new double[vertex_cnt];
	pfY = new double[vertex_cnt];
	pfZ = new double[vertex_cnt];
	pfM = new double[vertex_cnt];

	for ( loop1  = 0; loop1 < vertex_cnt ;loop1 ++ )
	{
		pfX[loop1] = pShp->pfX[loop1];
		pfY[loop1] = pShp->pfY[loop1];
		pfZ[loop1] = pShp->pfZ[loop1];
		pfM[loop1] = pShp->pfM[loop1];
	}

	fxMin = pShp->fxMin;
	fyMin = pShp->fyMin;
	fzMin = pShp->fzMin;
	fmidMin = pShp->fmidMin;

	fxMax = pShp->fxMax;
	fyMax = pShp->fyMax;
	fzMax = pShp->fzMax;
	fmidMax = pShp->fmidMax;

}

CESRIShapeFile::CESRIShapeFile(int nMaxDepth)
{
	m_nRef = 1;
	m_nMaxDepth = nMaxDepth;
	m_nStringFieldLen =0;
	m_szStringField = NULL;
	m_nDBFReadTupleLen =0;
	m_pDBFReturnTuple = NULL ;
    m_hTree = NULL;
    m_hSHP = NULL;
    m_hDBF = NULL;

	//InitializeCriticalSection(&m_csQueueLock);
};

CESRIShapeFile::~CESRIShapeFile()
{
	if(m_hTree) SHPDestroyTree(m_hTree);
	if(m_hSHP) SHPClose();
	if(m_hDBF) DBFClose();
}

//DEL DBFHandle CESRIShapeFile::GetDBFHandle()
//DEL {
//DEL 	return m_hDBF;
//DEL }

CESRIShapeFile::SHPHandle	CESRIShapeFile::GetSHPHandle()
{
	return m_hSHP;
}

CESRIShapeFile::SHPTree*	CESRIShapeFile::GetSHPTree()
{
	return 	m_hTree;
}



//////////////////////////////////////////////////////////////////////////
// 


CESRIShapeFile* CESRIShapeFile::Open(const string& sShapeFileName, bool bCreateTree, const char * pszAccess)
{
	//if(sShapeFileName.IsEmpty()) return NULL;
	//if(sShapeFileName.Right(3) != "shp") return NULL;
	
	//if (m_bOpened) Close();
	
	string sDBF = sShapeFileName;
	string sIdx = GetQTreeFileName(sShapeFileName);


	m_hSHP= SHPOpen( sShapeFileName.c_str(), pszAccess);
	
    if (m_hSHP != 0)
	{
		SHPGetInfo( &m_nEntities, &pnShapeType,padfMinBound, padfMaxBound );
		
		m_grBoundary.SetRect(padfMinBound[0],padfMaxBound[1],padfMaxBound[0],padfMinBound[1]);
		
		//Build a quadtree structure for this file.
		//m_hTree = SHPCreateTree( m_hSHP, 2, m_nMaxDepth, padfMinBound, padfMaxBound );
		//Trim unused nodes from the tree.
		//SHPTreeTrimExtraNodes( m_hTree );
		
		//Load Q-Tree from Q-Tree index file(if not exist, Build Q-Tree)
		if(bCreateTree)
		{
			m_hTree = SHPCreateTree( 2, m_nMaxDepth, padfMinBound, padfMaxBound, sIdx.c_str() );
#if 0
			if(!FileExists(sIdx))
				CreateQTreeFile(sIdx); 
#endif

			FillOffset(m_hTree); // SHPTreeNode에 treenode의 offset정보 입력 comment by hsb
		}
		else
			m_hTree = NULL;
		
		std::size_t nExtPos = sDBF.rfind('.');
		if(nExtPos)
		{
			sDBF.erase(nExtPos+1, 3);
			sDBF+="DBF";
			
			m_hDBF = DBFOpen(sDBF.c_str(), pszAccess);
            if (m_hDBF != 0)
			{
				if(pnShapeType == SHPT_POINT)
				{
					m_Type = stPOINT; 
				}
				else if (pnShapeType == SHPT_LINE)
					m_Type = stPOLYLINE; 
				else if (pnShapeType == SHPT_POLYGON)
					m_Type = stPOLYGON; 
				else
					m_Type = stUNKNOWN; 
#if 1 // by hsb
				if(bCreateTree)
				{
					if(!FileExists(GetMBRFileName(sShapeFileName).c_str()))
						CreateMBRFile(GetMBRFileName(sShapeFileName).c_str());
					m_mbrFile.open( GetMBRFileName(sShapeFileName).c_str(),ios::in  | ios::binary );

				}
#endif
				//	DHLee
				
				return this;
			}
		}
	}
	
    return NULL;
}

int CESRIShapeFile::GetLabelFieldIndex()
{
	return -1; //m_nField;
}

int CESRIShapeFile::GetEntityCount() const
{
	return m_nEntities;
}

GRect CESRIShapeFile::GetBoundary()
{
	return m_grBoundary;
}

SHAPETYPE CESRIShapeFile::GetShapeType()
{
	return m_Type;
}

string CESRIShapeFile::GetShapeFilePath()
{
	return m_sShapeFilePath;
}

int CESRIShapeFile::AddRef()
{
	return ++m_nRef;
}

int CESRIShapeFile::Release()
{
	return --m_nRef;
}

ifstream * CESRIShapeFile::GetMBRFile()
{
	if(m_mbrFile.is_open())
		return NULL;
	else
		return &m_mbrFile;
}

CESRIShapeFile::SHPTree* CESRIShapeFile::SHPCreateTree( int nDimension, int nMaxDepth,
								double *padfBoundsMin, double *padfBoundsMax, const char * szQTreeFileName )
										   
{
	//idx 파일이 있는지 검사해서 있으면 로딩, 없으면 Shplib의 SHPCreateTree()호출
	ifstream theFile;
	theFile.open(szQTreeFileName, ios::in | ios::binary);
	if(!theFile.good())

	{
        //const char * errorStr = strerror(errno);
		SHPTree	*psTree = SHPCreateTree( 2, nMaxDepth, padfBoundsMin, padfBoundsMax );
		SHPTreeTrimExtraNodes( psTree );

		return psTree;
	}
	else	
	{
		if( padfBoundsMin == NULL && m_hSHP == NULL )
			return NULL;
		
		//CArchive archive(&theFile, CArchive::load);
		SHPTree	*psTree;
		/* -------------------------------------------------------------------- */
		/*      Allocate the tree object                                        */
		/* -------------------------------------------------------------------- */
		psTree = (SHPTree *) malloc(sizeof(SHPTree)); //new SHPTree; 
		
		psTree->hSHP = m_hSHP;
		psTree->nMaxDepth = nMaxDepth;
		psTree->nDimension = nDimension;
		
		/* -------------------------------------------------------------------- */
		/*      Allocate the root node.                                         */
		/* -------------------------------------------------------------------- */
		//psTree->psRoot = SHPTreeNodeCreate( padfBoundsMin, padfBoundsMax );
		psTree->psRoot = (SHPTreeNode*)malloc(sizeof(SHPTreeNode)); //new SHPTreeNode; 
		
		/* -------------------------------------------------------------------- */
		/*      Assign the bounds to the root node.  If none are passed in,     */
		/*      use the bounds of the provided file otherwise the create        */
		/*      function will have already set the bounds.                      */
		/* -------------------------------------------------------------------- */
		if( padfBoundsMin == NULL )
		{
			SHPGetInfo( NULL, NULL,
				psTree->psRoot->adfBoundsMin, 
				psTree->psRoot->adfBoundsMax );
		}
		/* -------------------------------------------------------------------- */
		/*      If we have a QTree index file, load the tree.        */
		/* -------------------------------------------------------------------- */
		ReadTreeNodeFromFile(psTree->psRoot, theFile);
		
		return psTree;
	}
	
    return NULL;
}

void CESRIShapeFile::FillOffset(SHPTree * psTree)
{
	//순회하면서 nShapeCount의 누적 값을 각 노드에 기록. .mbr파일 인덱싱용으로 사용
	int nSum=0;
	FillOffsetToNode(psTree->psRoot, &nSum);
}

void CESRIShapeFile::FillOffsetToNode(SHPTreeNode * psTreeNode, int* pnSum)
{
	psTreeNode->nOffset = *pnSum;
	*pnSum += psTreeNode->nShapeCount;
	
	int i;
	for(i=0;i<psTreeNode->nSubNodes;i++)
	{
		//SHPTreeNode* pNode = psTreeNode->apsSubNode[i];
		FillOffsetToNode(psTreeNode->apsSubNode[i], pnSum);
	}
}

void CESRIShapeFile::ReadTreeNodeFromFile(SHPTreeNode * psTreeNode, ifstream& ar)
{
	//Read Node From File
    /* region covered by this node */
    int i;
	for(i=0;i<4;i++)
	{
		ar.read(reinterpret_cast<char *>(&psTreeNode->adfBoundsMin[i]), sizeof(psTreeNode->adfBoundsMin[i]));
	}
    for(i=0;i<4;i++)
	{
		ar.read(reinterpret_cast<char *>(&psTreeNode->adfBoundsMax[i]), sizeof(psTreeNode->adfBoundsMax[i]));
		
	}
	
    /* list of shapes stored at this node.  The papsShapeObj pointers
	or the whole list can be NULL */
	ar.read(reinterpret_cast<char *>(&psTreeNode->nShapeCount), sizeof(psTreeNode->nShapeCount));

	if(psTreeNode->nShapeCount > 0)
		psTreeNode->panShapeIds = (int*)malloc(sizeof(int) * psTreeNode->nShapeCount); //new int[psTreeNode->nShapeCount];
			
	else
		psTreeNode->panShapeIds = NULL;
	
	for (i = 0; i < psTreeNode->nShapeCount; i++)
		ar.read(reinterpret_cast<char *>(&psTreeNode->panShapeIds[i]), sizeof(psTreeNode->panShapeIds[i]));
	psTreeNode->papsShapeObj = NULL;
	ar.read(reinterpret_cast<char *>(&psTreeNode->nSubNodes), sizeof(psTreeNode->nSubNodes));
	//Call WriteTreeNodeToFile(this function) recursively
	for(i=0;i<psTreeNode->nSubNodes;i++)
	{
		//Alloc memory for sub node
		psTreeNode->apsSubNode[i] = (SHPTreeNode*)malloc(sizeof(SHPTreeNode)); //new SHPTreeNode;
			
		//SHPTreeNode* pNode = psTreeNode->apsSubNode[i];
		ReadTreeNodeFromFile(psTreeNode->apsSubNode[i], ar);
	}
	
	//Return
	return;
}

string CESRIShapeFile::GetQTreeFileName(const string &sShpFilePath)
{
	string sQTreeFile;
	std::size_t nExtPos = sShpFilePath.rfind('.');
	if(nExtPos != std::string::npos)
	{
		sQTreeFile = sShpFilePath;
		sQTreeFile.erase(nExtPos+1, 3);
		sQTreeFile+=("idx");
	}
	return sQTreeFile;
}

string CESRIShapeFile::GetMBRFileName(const string &sShpFilePath)
{
	string sMBRFile;
	int nExtPos = sShpFilePath.rfind('.');
	if(nExtPos)
	{
		sMBRFile = sShpFilePath;
		sMBRFile.erase(nExtPos+1, 3);
		sMBRFile+=("mbr");
	}
	
	return sMBRFile;
}


int CESRIShapeFile::DBFAddField(const char * pszFieldName, 
						  DBFFieldType eType, int nWidth, int nDecimals )
						  
{
    char	*pszFInfo;
    int		i;
	
	/* -------------------------------------------------------------------- */
	/*      Do some checking to ensure we can add records to this file.     */
	/* -------------------------------------------------------------------- */
    if( m_hDBF->nRecords > 0 )
        return( -1 );
	
    if( !m_hDBF->bNoHeader )
        return( -1 );
	
    if( eType != FTDouble && nDecimals != 0 )
        return( -1 );
	
	/* -------------------------------------------------------------------- */
	/*      SfRealloc all the arrays larger to hold the additional field      */
	/*      information.                                                    */
	/* -------------------------------------------------------------------- */
    m_hDBF->nFields++;
	
    m_hDBF->panFieldOffset = (int *) SfRealloc( m_hDBF->panFieldOffset, 
		sizeof(int) * m_hDBF->nFields );
	
    m_hDBF->panFieldSize = (int *) SfRealloc( m_hDBF->panFieldSize, 
		sizeof(int) * m_hDBF->nFields );
	
    m_hDBF->panFieldDecimals = (int *) SfRealloc( m_hDBF->panFieldDecimals, 
		sizeof(int) * m_hDBF->nFields );
	
    m_hDBF->pachFieldType = (char *) SfRealloc( m_hDBF->pachFieldType, 
		sizeof(char) * m_hDBF->nFields );
	
	/* -------------------------------------------------------------------- */
	/*      Assign the new field information fields.                        */
	/* -------------------------------------------------------------------- */
    m_hDBF->panFieldOffset[m_hDBF->nFields-1] = m_hDBF->nRecordLength;
    m_hDBF->nRecordLength += nWidth;
    m_hDBF->panFieldSize[m_hDBF->nFields-1] = nWidth;
    m_hDBF->panFieldDecimals[m_hDBF->nFields-1] = nDecimals;
	
    if( eType == FTString )
        m_hDBF->pachFieldType[m_hDBF->nFields-1] = 'C';

    else
        m_hDBF->pachFieldType[m_hDBF->nFields-1] = 'N';
	
	
	/* -------------------------------------------------------------------- */
	/*      Extend the required header information.                         */
	/* -------------------------------------------------------------------- */
    m_hDBF->nHeaderLength += 32;
    m_hDBF->bUpdated = false;
	
    m_hDBF->pszHeader = (char *) SfRealloc(m_hDBF->pszHeader,m_hDBF->nFields*32);
	
    pszFInfo = m_hDBF->pszHeader + 32 * (m_hDBF->nFields-1);
	
    for( i = 0; i < 32; i++ )
        pszFInfo[i] = '\0';
	
    if( (int) strlen(pszFieldName) < 10 )
        strncpy( pszFInfo, pszFieldName, strlen(pszFieldName));
    else
        strncpy( pszFInfo, pszFieldName, 10);
	
    pszFInfo[11] = m_hDBF->pachFieldType[m_hDBF->nFields-1];
	
    if( eType == FTString )
    {
        pszFInfo[16] = nWidth % 256;
        pszFInfo[17] = nWidth / 256;
    }
    else
    {
        pszFInfo[16] = nWidth;
        pszFInfo[17] = nDecimals;
    }
    
	/* -------------------------------------------------------------------- */
	/*      Make the current record buffer appropriately larger.            */
	/* -------------------------------------------------------------------- */
    m_hDBF->pszCurrentRecord = (char *) SfRealloc(m_hDBF->pszCurrentRecord,
		m_hDBF->nRecordLength);
	
    return( m_hDBF->nFields-1 );
}

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/

void * CESRIShapeFile::SfRealloc( void * pMem, int nNewSize )

{
    if( pMem == NULL )
        return( (void *) malloc(nNewSize) );
    else
        return( (void *) realloc(pMem,nNewSize) );
}

/************************************************************************/
/*                           DBFWriteHeader()                           */
/*                                                                      */
/*      This is called to write out the file header, and field          */
/*      descriptions before writing any actual data records.  This      */
/*      also computes all the DBFDataSet field offset/size/decimals     */
/*      and so forth values.                                            */
/************************************************************************/

void CESRIShapeFile::DBFWriteHeader()
{
    unsigned char	abyHeader[XBASE_FLDHDR_SZ];
    int		i;
	
    if( !m_hDBF->bNoHeader )
        return;
	
    m_hDBF->bNoHeader = false;
	
	/* -------------------------------------------------------------------- */
	/*	Initialize the file header information.				*/
	/* -------------------------------------------------------------------- */
    for( i = 0; i < XBASE_FLDHDR_SZ; i++ )
        abyHeader[i] = 0;
	
    abyHeader[0] = 0x03;		/* memo field? - just copying 	*/
	
    /* date updated on close, record count preset at zero */
	
    abyHeader[8] = m_hDBF->nHeaderLength % 256;
    abyHeader[9] = m_hDBF->nHeaderLength / 256;
    
    abyHeader[10] = m_hDBF->nRecordLength % 256;
    abyHeader[11] = m_hDBF->nRecordLength / 256;
	
	/* -------------------------------------------------------------------- */
	/*      Write the initial 32 byte file header, and all the field        */
	/*      descriptions.                                     		*/
	/* -------------------------------------------------------------------- */
	m_csQueueLock.lock();
    fseek( m_hDBF->fp, 0, 0 );
    fwrite( abyHeader, XBASE_FLDHDR_SZ, 1, m_hDBF->fp );
    fwrite( m_hDBF->pszHeader, XBASE_FLDHDR_SZ, m_hDBF->nFields, m_hDBF->fp );
	
	/* -------------------------------------------------------------------- */
	/*      Write out the newline character if there is room for it.        */
	/* -------------------------------------------------------------------- */
    if( m_hDBF->nHeaderLength > 32*m_hDBF->nFields + 32 )
    {
        char	cNewline;
		
        cNewline = 0x0d;
        fwrite( &cNewline, 1, 1, m_hDBF->fp );
    }
	m_csQueueLock.unlock();
}

/************************************************************************/
/*                           DBFFlushRecord()                           */
/*                                                                      */
/*      Write out the current record if there is one.                   */
/************************************************************************/

void CESRIShapeFile::DBFFlushRecord()
{
    int		nRecordOffset;
	
    if( m_hDBF->bCurrentRecordModified && m_hDBF->nCurrentRecord > -1 )
    {
		m_hDBF->bCurrentRecordModified = false;
		
		nRecordOffset = m_hDBF->nRecordLength * m_hDBF->nCurrentRecord 
			+ m_hDBF->nHeaderLength;
		
		m_csQueueLock.lock();
		fseek( m_hDBF->fp, nRecordOffset, 0 );
		fwrite( m_hDBF->pszCurrentRecord, m_hDBF->nRecordLength, 1, m_hDBF->fp );
		m_csQueueLock.unlock();
    }
}

/************************************************************************/
/*                              DBFOpen()                               */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/

// CESRIShapeFile::DBFHandle CESRIShapeFile::DBFOpenW(const  string & strFileName, const char * pszAccess )
// {
// 	char szFileName[MAX_PATH];
// 
// 	memset( szFileName, 0x00, sizeof(szFileName)); 
// 	WideCharToMultiByte(CP_ACP, 0, strFileName, -1, szFileName, sizeof(szFileName), NULL, NULL);
// 
// 	return DBFOpen( szFileName, pszAccess );
// }
CESRIShapeFile::DBFHandle CESRIShapeFile::DBFOpen( const char * pszFilename, const char * pszAccess )
{
    //DBFHandle		m_hDBF;
    unsigned char	*pabyBuf;
    int			nFields, nRecords, nHeadLen, nRecLen, iField, i;
    char		*pszBasename, *pszFullname;
	
	/* -------------------------------------------------------------------- */
	/*      We only allow the access strings "rb" and "r+".                  */
	/* -------------------------------------------------------------------- */
    if( strcmp(pszAccess,"r") != 0 && strcmp(pszAccess,"r+") != 0 
        && strcmp(pszAccess,"rb") != 0 && strcmp(pszAccess,"rb+") != 0
        && strcmp(pszAccess,"r+b") != 0 )
        return( NULL );
	
    if( strcmp(pszAccess,"r") == 0 )
        pszAccess = "rb";
	
    if( strcmp(pszAccess,"r+") == 0 )
        pszAccess = "rb+";
	
	/* -------------------------------------------------------------------- */
	/*	Compute the base (layer) name.  If there is any extension	*/
	/*	on the passed in filename we will strip it off.			*/
	/* -------------------------------------------------------------------- */
    //pszBasename = new char[strlen(pszFilename)+5];
	pszBasename = (char *) malloc(strlen(pszFilename)+5);
    strcpy( pszBasename, pszFilename );
    for( i = strlen(pszBasename)-1; 
	i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	       && pszBasename[i] != '\\';
	i-- ) {}
	
    if( pszBasename[i] == '.' )
        pszBasename[i] = '\0';
	
    //pszFullname = new char[strlen(pszBasename)+5];
	pszFullname = (char *) malloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.dbf", pszBasename );
	
    m_hDBF =  (DBFHandle) calloc( 1, sizeof(DBFInfo) ); //m_hDBF = new DBFInfo; 
    m_hDBF->fp = fopen( pszFullname, pszAccess );
	
    if( m_hDBF->fp == NULL )
    {
        sprintf( pszFullname, "%s.DBF", pszBasename );
        m_hDBF->fp = fopen(pszFullname, pszAccess );
    }
    
    free( pszBasename );
    free( pszFullname );
    
    if( m_hDBF->fp == NULL )
    {
		//delete m_hDBF;
        free( m_hDBF );
        return( NULL );
    }
	
    m_hDBF->bNoHeader = false;
    m_hDBF->nCurrentRecord = -1;
    m_hDBF->bCurrentRecordModified = false;
	
	/* -------------------------------------------------------------------- */
	/*  Read Table Header info                                              */
	/* -------------------------------------------------------------------- */
    pabyBuf = (unsigned char *) malloc(500); //new unsigned char[500]; 
    fread( pabyBuf, 32, 1, m_hDBF->fp );
	
    m_hDBF->nRecords = nRecords = 
		pabyBuf[4] + pabyBuf[5]*256 + pabyBuf[6]*256*256 + pabyBuf[7]*256*256*256;
	
    m_hDBF->nHeaderLength = nHeadLen = pabyBuf[8] + pabyBuf[9]*256;
    m_hDBF->nRecordLength = nRecLen = pabyBuf[10] + pabyBuf[11]*256;
    
    m_hDBF->nFields = nFields = (nHeadLen - 32) / 32;
	
    m_hDBF->pszCurrentRecord = (char *) malloc(nRecLen); //new char[nRecLen]; 
	
	/* -------------------------------------------------------------------- */
	/*  Read in Field Definitions                                           */
	/* -------------------------------------------------------------------- */
    
    pabyBuf = (unsigned char *) SfRealloc(pabyBuf,nHeadLen);
    m_hDBF->pszHeader = (char *) pabyBuf;

	m_csQueueLock.lock();	
    fseek( m_hDBF->fp, 32, 0 );
    fread( pabyBuf, nHeadLen, 1, m_hDBF->fp );
	m_csQueueLock.unlock();
	
    m_hDBF->panFieldOffset = (int *) malloc(sizeof(int) * nFields); //new int[nFields]; 
    m_hDBF->panFieldSize = (int *) malloc(sizeof(int) * nFields); //new int[nFields]; 
    m_hDBF->panFieldDecimals = (int *) malloc(sizeof(int) * nFields); //new int[nFields];  
    m_hDBF->pachFieldType = (char *) malloc(sizeof(char) * nFields); //new char[nFields]; 
	
    for( iField = 0; iField < nFields; iField++ )
    {
		unsigned char		*pabyFInfo;
		
		pabyFInfo = pabyBuf+iField*32;
		
		if( pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F' )
		{
			m_hDBF->panFieldSize[iField] = pabyFInfo[16];
			m_hDBF->panFieldDecimals[iField] = pabyFInfo[17];
		}
		else
		{
			m_hDBF->panFieldSize[iField] = pabyFInfo[16] + pabyFInfo[17]*256;
			m_hDBF->panFieldDecimals[iField] = 0;
		}
		
		m_hDBF->pachFieldType[iField] = (char) pabyFInfo[11];
		if( iField == 0 )
			m_hDBF->panFieldOffset[iField] = 1;
		else
			m_hDBF->panFieldOffset[iField] = 
			m_hDBF->panFieldOffset[iField-1] + m_hDBF->panFieldSize[iField-1];
    }
	
    return( m_hDBF );
}

/************************************************************************/
/*                              DBFClose()                              */
/************************************************************************/

void CESRIShapeFile::DBFClose()
{
	/* -------------------------------------------------------------------- */
	/*      Write out header if not already written.                        */
	/* -------------------------------------------------------------------- */
	if(!m_hDBF) return;
    if( m_hDBF->bNoHeader )
        DBFWriteHeader();
	
    DBFFlushRecord();

	if(m_pDBFReturnTuple) // by hsb
	{
		free(m_pDBFReturnTuple);
		m_pDBFReturnTuple = 0;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Update last access date, and number of records if we have	*/
	/*	write access.                					*/
	/* -------------------------------------------------------------------- */
    if( m_hDBF->bUpdated )
    {
		unsigned char		abyFileHeader[32];
		
		fseek( m_hDBF->fp, 0, 0 );
		fread( abyFileHeader, 32, 1, m_hDBF->fp );
		
		abyFileHeader[1] = 95;			/* YY */
		abyFileHeader[2] = 7;			/* MM */
		abyFileHeader[3] = 26;			/* DD */
		
		abyFileHeader[4] = m_hDBF->nRecords % 256;
		abyFileHeader[5] = (m_hDBF->nRecords/256) % 256;
		abyFileHeader[6] = (m_hDBF->nRecords/(256*256)) % 256;
		abyFileHeader[7] = (m_hDBF->nRecords/(256*256*256)) % 256;
		
		fseek( m_hDBF->fp, 0, 0 );
		fwrite( abyFileHeader, 32, 1, m_hDBF->fp );
    }
	
	/* -------------------------------------------------------------------- */
	/*      Close, and free resources.                                      */
	/* -------------------------------------------------------------------- */
    fclose( m_hDBF->fp );
	
    if( m_hDBF->panFieldOffset != NULL )
    {
        free( m_hDBF->panFieldOffset ); //delete[] m_hDBF->panFieldOffset; 
        free( m_hDBF->panFieldSize ); //delete[] m_hDBF->panFieldSize; 
        free( m_hDBF->panFieldDecimals ); //delete[] m_hDBF->panFieldDecimals; 
        free( m_hDBF->pachFieldType ); //delete[] m_hDBF->pachFieldType; 
    }
	
    free( m_hDBF->pszHeader ); //delete[] m_hDBF->pszHeader; 
    free( m_hDBF->pszCurrentRecord ); //delete[] m_hDBF->pszCurrentRecord; 
	
    free( m_hDBF ); //delete m_hDBF; 
	m_hDBF = NULL;
	
    if( m_szStringField != NULL )
    {
        free( m_szStringField ); //delete[] m_szStringField; 
        m_szStringField = NULL;
        m_nStringFieldLen = 0;
    }
}

/************************************************************************/
/*                             DBFCreate()                              */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/
// CESRIShapeFile::DBFHandle CESRIShapeFile::DBFCreateW( const string & strFilename )
// {
// 	char szFileName[MAX_PATH];
// 
// //	WideCharToMultiByte(CP_ACP, 0, strFilename, -1, szFileName, sizeof(szFileName), NULL, NULL);
// 
// 	return DBFCreate( szFileName);
// }

CESRIShapeFile::DBFHandle CESRIShapeFile::DBFCreate( const char * pszFilename )

{
    FILE	*fp;
    char	*pszFullname, *pszBasename;
    int		i;
	
	/* -------------------------------------------------------------------- */
	/*	Compute the base (layer) name.  If there is any extension	*/
	/*	on the passed in filename we will strip it off.			*/
	/* -------------------------------------------------------------------- */
    //pszBasename = new char[strlen(pszFilename)+5];
	pszBasename = (char *) malloc(strlen(pszFilename)+5);
    strcpy( pszBasename, pszFilename );
    for( i = strlen(pszBasename)-1; 
	i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	       && pszBasename[i] != '\\';
	i-- ) {}
	
    if( pszBasename[i] == '.' )
        pszBasename[i] = '\0';
	
    //pszFullname = new char[strlen(pszBasename)+5];
	pszFullname = (char *) malloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.dbf", pszBasename );
	//delete[] pszBasename;
    free( pszBasename );
	
	/* -------------------------------------------------------------------- */
	/*      Create the file.                                                */
	/* -------------------------------------------------------------------- */
    fp = fopen( pszFullname, "wb" );
    if( fp == NULL )
	{
		//DWORD err = GetLastError();
		//TRACE("error %d %s\n", errno, pszFullname );
        return( NULL );
	}
	
    fputc( 0, fp );
    fclose( fp );
	
    fp = fopen( pszFullname, "rb+" );
    if( fp == NULL )
        return( NULL );
	//delete[] pszFullname;
    free( pszFullname );
	
	/* -------------------------------------------------------------------- */
	/*	Create the info structure.					*/
	/* -------------------------------------------------------------------- */
    m_hDBF = (DBFHandle) malloc(sizeof(DBFInfo)); //new DBFInfo; 
	
    m_hDBF->fp = fp;
    m_hDBF->nRecords = 0;
    m_hDBF->nFields = 0;
    m_hDBF->nRecordLength = 1;
    m_hDBF->nHeaderLength = 33;
    
    m_hDBF->panFieldOffset = NULL;
    m_hDBF->panFieldSize = NULL;
    m_hDBF->panFieldDecimals = NULL;
    m_hDBF->pachFieldType = NULL;
    m_hDBF->pszHeader = NULL;
	
    m_hDBF->nCurrentRecord = -1;
    m_hDBF->bCurrentRecordModified = false;
    m_hDBF->pszCurrentRecord = NULL;
	
    m_hDBF->bNoHeader = true;
	
    return( m_hDBF );
}


/************************************************************************/
/*                          DBFReadAttribute()                          */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

void* CESRIShapeFile::DBFReadAttribute(int hEntity, int iField,
								 char chReqType  )
								 
{
    int	       	nRecordOffset;
    unsigned char	*pabyRec;
    void	*pReturnField = NULL;
	
 //   static double	dDoubleField;
	//static __int64_t i8Field;
	//static int		i4Field;
	
	/* -------------------------------------------------------------------- */
	/*      Verify selection.                                               */
	/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity >= m_hDBF->nRecords )
        return( NULL );
	
    if( iField < 0 || iField >= m_hDBF->nFields )
        return( NULL );
	
	/* -------------------------------------------------------------------- */
	/*	Have we read the record?					*/
	/* -------------------------------------------------------------------- */
    if( m_hDBF->nCurrentRecord != hEntity )
    {
		DBFFlushRecord();
		
		nRecordOffset = m_hDBF->nRecordLength * hEntity + m_hDBF->nHeaderLength;
		
		m_csQueueLock.lock();
		if( fseek( m_hDBF->fp, nRecordOffset, 0 ) != 0 )
        {
            fprintf( stderr, "fseek(%d) failed on DBF file.\n",
				nRecordOffset );
            return NULL;
        }
		
		if( fread( m_hDBF->pszCurrentRecord, m_hDBF->nRecordLength, 
			1, m_hDBF->fp ) != 1 )
        {
            fprintf( stderr, "fread(%d) failed on DBF file.\n",
				m_hDBF->nRecordLength );
            return NULL;
        }
		m_csQueueLock.unlock();
		
		m_hDBF->nCurrentRecord = hEntity;
    }
	
    pabyRec = (unsigned char *) m_hDBF->pszCurrentRecord;
	
	/* -------------------------------------------------------------------- */
	/*	Ensure our field buffer is large enough to hold this buffer.	*/
	/* -------------------------------------------------------------------- */
    if( m_hDBF->panFieldSize[iField]+1 > m_nStringFieldLen )
    {
		m_nStringFieldLen = m_hDBF->panFieldSize[iField]*2 + 10;
		m_szStringField = (char *) SfRealloc(m_szStringField,m_nStringFieldLen);
    }
	
	/* -------------------------------------------------------------------- */
	/*	Extract the requested field.					*/
	/* -------------------------------------------------------------------- */
    strncpy( m_szStringField, 
		((const char *) pabyRec) + m_hDBF->panFieldOffset[iField],
		m_hDBF->panFieldSize[iField] );
    m_szStringField[m_hDBF->panFieldSize[iField]] = '\0';
	
    pReturnField = m_szStringField;
	
	/* -------------------------------------------------------------------- */
	/*      Decode the field.                                               */
	/* -------------------------------------------------------------------- */
    if( chReqType == 'N' )
    {
        m_retvalIntField = atoi(m_szStringField);
		pReturnField = &m_retvalIntField;

    }
	else if( chReqType == 'F' )
	{
		m_retvalDoubleField = atof(m_szStringField);
		pReturnField = &m_retvalDoubleField;
	}
	else if( chReqType == 'E' )
	{
        m_retvalLongLongField = atol (m_szStringField);
		pReturnField = &m_retvalLongLongField;
	}
	/* -------------------------------------------------------------------- */
	/*      Should we trim white space off the string attribute value?      */
	/* -------------------------------------------------------------------- */
#ifdef TRIM_DBF_WHITESPACE
    else
    {
        char	*pchSrc, *pchDst;
		
        pchDst = pchSrc = m_szStringField;
        while( *pchSrc == ' ' )
            pchSrc++;
		
        while( *pchSrc != '\0' )
            *(pchDst++) = *(pchSrc++);
        *pchDst = '\0';
		
        while( pchDst != m_szStringField && *(--pchDst) == ' ' )
            *pchDst = '\0';
    }
#endif
    
    return( pReturnField );
}

/************************************************************************/
/*                        DBFReadIntAttribute()                         */
/*                                                                      */
/*      Read an integer attribute.                                      */
/************************************************************************/

int CESRIShapeFile::DBFReadIntegerAttribute(int iRecord, int iField )
{
    int	*pdValue;
	//int value;
	
    pdValue = (int *) DBFReadAttribute( iRecord, iField, 'N' );
	
    if( pdValue == NULL )
        return 0;
    else
        return( (int) *pdValue );
}

long long CESRIShapeFile::DBFReadInt64Attribute(int iRecord, int iField )
{
	long long	*pdValue;

	pdValue = (long long *) DBFReadAttribute( iRecord, iField, 'E' );

	if( pdValue == NULL )
		return 0;
	else
		return( (long long) *pdValue );
}


/************************************************************************/
/*                        DBFReadDoubleAttribute()                      */
/*                                                                      */
/*      Read a double attribute.                                        */
/************************************************************************/

double CESRIShapeFile::DBFReadDoubleAttribute( int iRecord, int iField )

{
    double	*pdValue;
	
    pdValue = (double *) DBFReadAttribute( iRecord, iField, 'F' );
	
	
    if( pdValue == NULL )
        return 0.0;
    else
        return( *pdValue );
}

/************************************************************************/
/*                        DBFReadStringAttribute()                      */
/*                                                                      */
/*      Read a string attribute.                                        */
/************************************************************************/

const char * CESRIShapeFile::DBFReadStringAttribute( int iRecord, int iField )
{
    return( (const char *) DBFReadAttribute( iRecord, iField, 'C' ) );
}

/************************************************************************/
/*                         DBFIsAttributeNULL()                         */
/*                                                                      */
/*      Return true if value for field is NULL.                         */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/************************************************************************/

int CESRIShapeFile::DBFIsAttributeNULL(int iRecord, int iField )
{
    const char	*pszValue;
	
    pszValue = DBFReadStringAttribute(iRecord, iField );
	
    switch(m_hDBF->pachFieldType[iField])
    {
	case 'N':
	case 'F':
        /* NULL numeric fields have value "****************" */
        return pszValue[0] == '*';
		
	case 'D':
        /* NULL date fields have value "00000000" */
        return strncmp(pszValue,"00000000",8) == 0;
		
	case 'L':
        /* NULL boolean fields have value "?" */ 
        return pszValue[0] == '?';
		
	default:
        /* empty string fields are considered NULL */
        return strlen(pszValue) == 0;
    }
    return false;
}

/************************************************************************/
/*                          DBFGetFieldCount()                          */
/*                                                                      */
/*      Return the number of fields in this table.                      */
/************************************************************************/

int CESRIShapeFile::DBFGetFieldCount()

{
    return( m_hDBF->nFields );
}

/************************************************************************/
/*                         DBFGetRecordCount()                          */
/*                                                                      */
/*      Return the number of records in this table.                     */
/************************************************************************/

int CESRIShapeFile::DBFGetRecordCount()

{
    return( m_hDBF->nRecords );
}

/************************************************************************/
/*                          DBFGetFieldInfo()                           */
/*                                                                      */
/*      Return any requested information about the field.               */
/************************************************************************/

DBFFieldType CESRIShapeFile::DBFGetFieldInfo(int iField, char * pszFieldName,
									   int * pnWidth, int * pnDecimals )
									   
{
    if( iField < 0 || iField >= m_hDBF->nFields )
        return( FTInvalid );
	
    if( pnWidth != NULL )
        *pnWidth = m_hDBF->panFieldSize[iField];
	
    if( pnDecimals != NULL )
        *pnDecimals = m_hDBF->panFieldDecimals[iField];
	
    if( pszFieldName != NULL )
    {
		int	i;
		
		strncpy( pszFieldName, (char *) m_hDBF->pszHeader+iField*32, 11 );
		pszFieldName[11] = '\0';
		for( i = 10; i > 0 && pszFieldName[i] == ' '; i-- )
			pszFieldName[i] = '\0';
    }
	
    if( m_hDBF->pachFieldType[iField] == 'N' 
        || m_hDBF->pachFieldType[iField] == 'F'
        || m_hDBF->pachFieldType[iField] == 'D'
		)
    {
		if( m_hDBF->panFieldDecimals[iField] > 0 )
			return( FTDouble );
		else
			return( FTInteger );
    }

    else
    {
		return( FTString );
    }
}

/************************************************************************/
/*                         DBFWriteAttribute()                          */
/*									*/
/*	Write an attribute record to the file.				*/
/************************************************************************/

int CESRIShapeFile::DBFWriteAttribute(int hEntity, int iField, void * pValue )
{
    int	       	nRecordOffset, i, j;
    unsigned char	*pabyRec;
    char	szSField[400], szFormat[20];
	
	/* -------------------------------------------------------------------- */
	/*	Is this a valid record?						*/
	/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity > m_hDBF->nRecords )
        return( false );
	
    if( m_hDBF->bNoHeader )
        DBFWriteHeader();
	
	/* -------------------------------------------------------------------- */
	/*      Is this a brand new record?                                     */
	/* -------------------------------------------------------------------- */
    if( hEntity == m_hDBF->nRecords )
    {
		DBFFlushRecord();
		
		m_hDBF->nRecords++;
		for( i = 0; i < m_hDBF->nRecordLength; i++ )
			m_hDBF->pszCurrentRecord[i] = ' ';
		
		m_hDBF->nCurrentRecord = hEntity;
    }
	
	/* -------------------------------------------------------------------- */
	/*      Is this an existing record, but different than the last one     */
	/*      we accessed?                                                    */
	/* -------------------------------------------------------------------- */
    if( m_hDBF->nCurrentRecord != hEntity )
    {
		DBFFlushRecord();
		
		nRecordOffset = m_hDBF->nRecordLength * hEntity + m_hDBF->nHeaderLength;
		
		fseek( m_hDBF->fp, nRecordOffset, 0 );
		fread( m_hDBF->pszCurrentRecord, m_hDBF->nRecordLength, 1, m_hDBF->fp );
		
		m_hDBF->nCurrentRecord = hEntity;
    }
	
    pabyRec = (unsigned char *) m_hDBF->pszCurrentRecord;
	
    m_hDBF->bCurrentRecordModified = true;
    m_hDBF->bUpdated = true;
	
	/* -------------------------------------------------------------------- */
	/*      Translate NULL value to valid DBF file representation.          */
	/*                                                                      */
	/*      Contributed by Jim Matthews.                                    */
	/* -------------------------------------------------------------------- */
    if( pValue == NULL )
    {
        switch(m_hDBF->pachFieldType[iField])
        {
		case 'N':
		case 'F':
			/* NULL numeric fields have value "****************" */
            memset( (char *) (pabyRec+m_hDBF->panFieldOffset[iField]), '*', 
				m_hDBF->panFieldSize[iField] );
            break;
			
		case 'D':
			/* NULL date fields have value "00000000" */
            memset( (char *) (pabyRec+m_hDBF->panFieldOffset[iField]), '0', 
				m_hDBF->panFieldSize[iField] );
            break;
			
		case 'L':
			/* NULL boolean fields have value "?" */ 
            memset( (char *) (pabyRec+m_hDBF->panFieldOffset[iField]), '?', 
				m_hDBF->panFieldSize[iField] );
            break;
			
		default:
            /* empty string fields are considered NULL */
            memset( (char *) (pabyRec+m_hDBF->panFieldOffset[iField]), '\0', 
				m_hDBF->panFieldSize[iField] );
            break;
        }
        return true;
    }
	
	/* -------------------------------------------------------------------- */
	/*      Assign all the record fields.                                   */
	/* -------------------------------------------------------------------- */
    switch( m_hDBF->pachFieldType[iField] )
    {
	case 'D':
	case 'N':
	case 'F':
		if( m_hDBF->pachFieldType[iField] == 'N'
			&& m_hDBF->panFieldDecimals[iField] == 0 
			&& m_hDBF->panFieldSize[iField] >= 10 )
		{
            int		nWidth = m_hDBF->panFieldSize[iField];
			
            if( (int)(sizeof(szSField))-2 < nWidth )
                nWidth = sizeof(szSField)-2;
			
			sprintf( szFormat, "%%%d.0lf", nWidth );
			sprintf(szSField, szFormat, *((double *) pValue) );
			if( (int) strlen(szSField) > m_hDBF->panFieldSize[iField] )
				szSField[m_hDBF->panFieldSize[iField]] = '\0';
			strncpy((char *) (pabyRec+m_hDBF->panFieldOffset[iField]),
				szSField, strlen(szSField) );
		}
		else if( m_hDBF->panFieldDecimals[iField] == 0 )
		{
            int		nWidth = m_hDBF->panFieldSize[iField];
			
            if( (int)(sizeof(szSField))-2 < nWidth )
                nWidth = sizeof(szSField)-2;
			
			sprintf( szFormat, "%%%dd", nWidth );
			sprintf(szSField, szFormat, (int) *((double *) pValue) );
			if( (int)strlen(szSField) > m_hDBF->panFieldSize[iField] )
				szSField[m_hDBF->panFieldSize[iField]] = '\0';
			
			strncpy((char *) (pabyRec+m_hDBF->panFieldOffset[iField]),
				szSField, strlen(szSField) );
		}
		else
		{
            int		nWidth = m_hDBF->panFieldSize[iField];
			
            if( (int)(sizeof(szSField))-2 < nWidth )
                nWidth = sizeof(szSField)-2;
			
			sprintf( szFormat, "%%%d.%df", 
				nWidth, m_hDBF->panFieldDecimals[iField] );
			sprintf(szSField, szFormat, *((double *) pValue) );
			if( (int) strlen(szSField) > m_hDBF->panFieldSize[iField] )
				szSField[m_hDBF->panFieldSize[iField]] = '\0';
			strncpy((char *) (pabyRec+m_hDBF->panFieldOffset[iField]),
				szSField, strlen(szSField) );
		}
		break;
	default:
		if( (int) strlen((char *) pValue) > m_hDBF->panFieldSize[iField] )
			j = m_hDBF->panFieldSize[iField];
		else
        {
            memset( pabyRec+m_hDBF->panFieldOffset[iField], ' ',
				m_hDBF->panFieldSize[iField] );
			j = strlen((char *) pValue);
        }
		
		strncpy((char *) (pabyRec+m_hDBF->panFieldOffset[iField]),
			(char *) pValue, j );
		break;
    }
	
    return( true );
}

/************************************************************************/
/*                      DBFWriteDoubleAttribute()                       */
/*                                                                      */
/*      Write a double attribute.                                       */
/************************************************************************/

int CESRIShapeFile::DBFWriteAttribute( int iRecord, int iField,double dValue )
{
    return( DBFWriteAttribute( iRecord, iField, (void *) &dValue ) );
}

/************************************************************************/
/*                      DBFWriteIntegerAttribute()                      */
/*                                                                      */
/*      Write a integer attribute.                                      */
/************************************************************************/

int CESRIShapeFile::DBFWriteAttribute( int iRecord, int iField,int nValue )
{
    double	dValue = nValue;
	
    return( DBFWriteAttribute(iRecord, iField, (void *) &dValue ) );
}

int CESRIShapeFile::DBFWriteAttribute( int iRecord, int iField, int64_t nValue )
{
	double	dValue = (double)nValue;

	return( DBFWriteAttribute(iRecord, iField, (void *) &dValue ) );

	//return( DBFWriteAttribute(iRecord, iField, (void *) &nValue ) );
}

/************************************************************************/
/*                      DBFWriteStringAttribute()                       */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

int CESRIShapeFile::DBFWriteAttribute( int iRecord, int iField,
								const char * pszValue )
								
{
    return( DBFWriteAttribute(iRecord, iField, (void *) pszValue ) );
}

/************************************************************************/
/*                      DBFWriteNULLAttribute()                         */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/

int CESRIShapeFile::DBFWriteAttribute( int iRecord, int iField )

{
    return( DBFWriteAttribute(iRecord, iField, NULL ) );
}

/************************************************************************/
/*                         DBFWriteTuple()                              */
/*									*/
/*	Write an attribute record to the file.			// 	*/
/************************************************************************/

int CESRIShapeFile::DBFWriteTuple(int hEntity, void * pRawTuple )

{
    int	       	nRecordOffset, i;
    unsigned char	*pabyRec;
	
	/* -------------------------------------------------------------------- */
	/*	Is this a valid record?						*/
	/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity > m_hDBF->nRecords )
        return( false );
	
    if( m_hDBF->bNoHeader )
        DBFWriteHeader();
	
	/* -------------------------------------------------------------------- */
	/*      Is this a brand new record?                                     */
	/* -------------------------------------------------------------------- */
    if( hEntity == m_hDBF->nRecords )
    {
		DBFFlushRecord();
		
		m_hDBF->nRecords++;
		for( i = 0; i < m_hDBF->nRecordLength; i++ )
			m_hDBF->pszCurrentRecord[i] = ' ';
		
		m_hDBF->nCurrentRecord = hEntity;
    }
	
	/* -------------------------------------------------------------------- */
	/*      Is this an existing record, but different than the last one     */
	/*      we accessed?                                                    */
	/* -------------------------------------------------------------------- */
    if( m_hDBF->nCurrentRecord != hEntity )
    {
		DBFFlushRecord();
		
		nRecordOffset = m_hDBF->nRecordLength * hEntity + m_hDBF->nHeaderLength;
		
		fseek( m_hDBF->fp, nRecordOffset, 0 );
		fread( m_hDBF->pszCurrentRecord, m_hDBF->nRecordLength, 1, m_hDBF->fp );
		
		m_hDBF->nCurrentRecord = hEntity;
    }
	
    pabyRec = (unsigned char *) m_hDBF->pszCurrentRecord;
	
    memcpy ( pabyRec, pRawTuple,  m_hDBF->nRecordLength );
	
    m_hDBF->bCurrentRecordModified = true;
    m_hDBF->bUpdated = true;
	
    return( true );
}

/************************************************************************/
/*                          DBFReadTuple()                              */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

const char* CESRIShapeFile::DBFReadTuple(int hEntity )

{
    int	       	nRecordOffset;
    unsigned char	*pabyRec;
    //static char	*pReturnTuple = NULL;
	
    //static int	nReadTupleLen = 0;
	
	/* -------------------------------------------------------------------- */
	/*	Have we read the record?					*/
	/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity >= m_hDBF->nRecords )
        return( NULL );
	
    if( m_hDBF->nCurrentRecord != hEntity )
    {
		DBFFlushRecord();
		
		nRecordOffset = m_hDBF->nRecordLength * hEntity + m_hDBF->nHeaderLength;
		
		m_csQueueLock.lock();
		fseek( m_hDBF->fp, nRecordOffset, 0 );
		fread( m_hDBF->pszCurrentRecord, m_hDBF->nRecordLength, 1, m_hDBF->fp );
		m_csQueueLock.unlock();
		
		m_hDBF->nCurrentRecord = hEntity;
    }
	
    pabyRec = (unsigned char *) m_hDBF->pszCurrentRecord;
	
    if ( m_nDBFReadTupleLen < m_hDBF->nRecordLength) {
		m_nDBFReadTupleLen = m_hDBF->nRecordLength;
		m_pDBFReturnTuple = (char *) SfRealloc(m_pDBFReturnTuple, m_hDBF->nRecordLength);
    }
    
    memcpy ( m_pDBFReturnTuple, pabyRec, m_hDBF->nRecordLength );
	
    return( m_pDBFReturnTuple );
}

/************************************************************************/
/*                          DBFCloneEmpty()                              */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/

CESRIShapeFile::DBFHandle CESRIShapeFile::DBFCloneEmpty(const char * pszFilename ) 
{
	//복사생성자로 교체해야 함.
    DBFHandle	newDBF;
	
	newDBF = DBFCreate ( pszFilename );
	if ( newDBF == NULL ) return ( NULL ); 
	
	newDBF->pszHeader = (char *) malloc ( 32 * m_hDBF->nFields ); //new char[ 32 * m_hDBF->nFields ]; 
	memcpy ( newDBF->pszHeader, m_hDBF->pszHeader, 32 * m_hDBF->nFields );
	
	newDBF->nFields = m_hDBF->nFields;
	newDBF->nRecordLength = m_hDBF->nRecordLength;
	newDBF->nHeaderLength = m_hDBF->nHeaderLength;
    
	newDBF->panFieldOffset = (int *) malloc ( sizeof(int) * m_hDBF->nFields );   //new int[ m_hDBF->nFields ];
	memcpy ( newDBF->panFieldOffset, m_hDBF->panFieldOffset, sizeof(int) * m_hDBF->nFields );
	newDBF->panFieldSize = (int *) malloc ( sizeof(int) * m_hDBF->nFields );    //new int[ m_hDBF->nFields ];
	memcpy ( newDBF->panFieldSize, m_hDBF->panFieldSize, sizeof(int) * m_hDBF->nFields );
	newDBF->panFieldDecimals = (int *) malloc ( sizeof(int) * m_hDBF->nFields ); //new int[ m_hDBF->nFields ];
	memcpy ( newDBF->panFieldDecimals, m_hDBF->panFieldDecimals, sizeof(int) * m_hDBF->nFields );
	newDBF->pachFieldType = (char *) malloc ( sizeof(int) * m_hDBF->nFields );  //new char[ m_hDBF->nFields ];
	memcpy ( newDBF->pachFieldType, m_hDBF->pachFieldType, sizeof(int) * m_hDBF->nFields );
	
	newDBF->bNoHeader = true;
	newDBF->bUpdated = true;
	
	DBFWriteHeader ();
	DBFClose ();
	
	newDBF = DBFOpen ( pszFilename, "rb+" );
	
	return ( newDBF );
}

/************************************************************************/
/*                       DBFGetNativeFieldType()                        */
/*                                                                      */
/*      Return the DBase field type for the specified field.            */
/*                                                                      */
/*      Value can be one of: 'C' (String), 'D' (Date), 'F' (Float),     */
/*                           'N' (Numeric, with or without decimal),    */
/*                           'L' (Logical),                             */
/*                           'M' (Memo: 10 digits .DBT block ptr)       */
/************************************************************************/

char CESRIShapeFile::DBFGetNativeFieldType( int iField )
{
    if( iField >=0 && iField < m_hDBF->nFields )
        return m_hDBF->pachFieldType[iField];
	
    return  ' ';
}

/************************************************************************/
/*                            str_to_upper()                            */
/************************************************************************/

static void str_to_upper (char *string)
{
    int len;
    short i = -1;
	
    len = strlen (string);
	
    while (++i < len)
        if (isalpha(string[i]) && islower(string[i]))
            string[i] = toupper ((int)string[i]);
}

/************************************************************************/
/*                          DBFGetFieldIndex()                          */
/*                                                                      */
/*      Get the index number for a field in a .dbf file.                */
/*                                                                      */
/*      Contributed by Jim Matthews.                                    */
/************************************************************************/

int CESRIShapeFile::DBFGetFieldIndex(const char *pszFieldName)
{
    char          name[12], name1[12], name2[12];
    int           i;
	memset( name, 0x00, sizeof(name));
	memset( name1, 0x00, sizeof(name1));
	memset( name2, 0x00, sizeof(name2));
	
    strncpy(name1, pszFieldName,11);
    str_to_upper(name1);
	
    for( i = 0; i < DBFGetFieldCount(); i++ )
    {
        DBFGetFieldInfo(i, name, NULL, NULL );
        strncpy(name2,name,11);
        str_to_upper(name2);
		
        if(!strncmp(name1,name2,10))
            return(i);
    }
    return(-1);
}

int* CESRIShapeFile::SHPMBRShapes(double * padfBoundsMin, double * padfBoundsMax,
							int * pnShapeCount, ifstream * pMBR, bool bSortByBottom )
{
    int	*panShapeList=NULL, nMaxShapes = 0, nMaxNodes = 0;
	int *panOffsetList = NULL;
	int *panCountList = NULL;
	int nNodeCount=0;
	int* pnIds=NULL;
	double* pdBottoms=NULL;
	/* -------------------------------------------------------------------- */
	/*      Perform the search by recursive descent.                        */
	/* -------------------------------------------------------------------- */
    *pnShapeCount = 0;
	
    SHPTreeCollectShapeIds( m_hTree->psRoot,
		padfBoundsMin, padfBoundsMax,
		pnShapeCount, &nMaxShapes, &nMaxNodes,
		&panShapeList, &panOffsetList, &panCountList, &nNodeCount );
	if(*pnShapeCount>0)
	{
		pnIds = new int[*pnShapeCount];
		if(bSortByBottom)
			pdBottoms = new double[*pnShapeCount];
	}
	else
	{
		free(panShapeList);
		free(panOffsetList);
		free(panCountList);
		return NULL;
	}
	
	int nPos = 0;
	int n = 0;
	
	if(pMBR)
	{
		//char szShpMBR[16];
		unsigned int anShpMBR[4];
        //unsigned int nBytesRead;
		
		for(register int i=0;i<nNodeCount;i++)
		{
			pMBR->seekg(panOffsetList[i]*16, std::ios::beg);

			for(register int j=0;j<panCountList[i];j++)
			{
				pMBR->read(reinterpret_cast<char *>(anShpMBR),sizeof(anShpMBR));
				//nBytesRead = pMBR->read(&anShpMBR[0], 16 );
				//nBytesRead = ar.read(reinterpret_cast<char *>(&psTreeNode->adfBoundsMin[i]), sizeof(psTreeNode->adfBoundsMin[i]));
				const int MBR_LEFT = 0;
				const int MBR_BOTTOM = 1;
				const int MBR_RIGHT = 2;
				const int MBR_TOP = 3;

				if ( (anShpMBR[MBR_RIGHT] > padfBoundsMin[0])
					&& (anShpMBR[MBR_TOP] > padfBoundsMin[1])
					&& (padfBoundsMax[0] > anShpMBR[MBR_LEFT])
					&& (padfBoundsMax[1] > anShpMBR[MBR_BOTTOM]) )
				{
					pnIds[n] = panShapeList[nPos];
					if(pdBottoms)
						pdBottoms[n] = anShpMBR[MBR_BOTTOM];
					n++;
				}
				nPos++;
			}
		}
	}
	else
	{
		CSHPObject* pShape=NULL;
		
		for(register int i=0;i<*pnShapeCount;i++)
		{
			pShape = SHPReadObject(panShapeList[i]);
			if(!pShape) continue;
			
			if(SHPCheckBoundsOverlap(padfBoundsMin,padfBoundsMax,
				&(pShape->fxMin),&(pShape->fxMax),m_hTree->nDimension) )
			{
				pnIds[n] = pShape->nShapeId;
				if(pdBottoms)
					pdBottoms[n] = pShape->fyMin;
				n++;
			}
			
			delete pShape;
		}
	}
	*pnShapeCount = n;
	
// 	delete[] panShapeList;
// 	delete[] panOffsetList;
// 	delete[] panCountList;
	free(panShapeList);
	free(panOffsetList);
	free(panCountList);

	//Sort
	if(bSortByBottom)
	{
		BottomSort(pnIds, pdBottoms, 0, n-1);
	}
	else
		QuickSort(pnIds, 0, n-1);
	delete[] pdBottoms;
	return pnIds;
}


int* CESRIShapeFile::SHPMBRShapes2(double * padfBoundsMin, double * padfBoundsMax,  int * pnShapeCount, CArrSHPObject *pArrShpObj )
{
	int	*panShapeList=NULL, nMaxShapes = 0, nMaxNodes = 0;
	int *panOffsetList = NULL;
	int *panCountList = NULL;
	int nNodeCount=0;
	int* pnIds=NULL;

	/* -------------------------------------------------------------------- */
	/*      Perform the search by recursive descent.                        */
	/* -------------------------------------------------------------------- */
	*pnShapeCount = 0;

	SHPTreeCollectShapeIds( m_hTree->psRoot,
		padfBoundsMin, padfBoundsMax,
		pnShapeCount, &nMaxShapes, &nMaxNodes,
		&panShapeList, &panOffsetList, &panCountList, &nNodeCount );

	if(*pnShapeCount>0)
	{
		pnIds = new int[*pnShapeCount];
	}
	else
	{
		free (panShapeList);
		free (panOffsetList);
		free (panCountList);
		return NULL;
	}

    //int nPos = 0;
	int n = 0;

	{
		CSHPObject* pShape=NULL;

		for(register int i=0;i<*pnShapeCount;i++)
		{
			pShape = pArrShpObj->at(panShapeList[i]);
			if(!pShape) 
				continue;

			if(SHPCheckBoundsOverlap(padfBoundsMin,padfBoundsMax,
				&(pShape->fxMin),&(pShape->fxMax),m_hTree->nDimension) )
			{
				pnIds[n] = panShapeList[i];
				n++;
			}

			//delete pShape;
		}
	}
	*pnShapeCount = n;

//	delete[] panShapeList;
//	delete[] panOffsetList;
//	delete[] panCountList;
	free (panShapeList);
	free (panOffsetList);
	free (panCountList);
	

	return pnIds;
}



void CESRIShapeFile::SHPTreeCollectShapeIds(SHPTreeNode * psTreeNode,
									  double * padfBoundsMin, double * padfBoundsMax,
									  int * pnShapeCount, int * pnMaxShapes, int* pnMaxNodes,
									  int ** ppanShapeList, int ** ppanOffsetList, int ** ppanCountList, int* pnNodeCount )
									  
{
    int		i;
    
	/* -------------------------------------------------------------------- */
	/*      Does this node overlap the area of interest at all?  If not,    */
	/*      return without adding to the list at all.                       */
	/* -------------------------------------------------------------------- */
    if( !SHPCheckBoundsOverlap( psTreeNode->adfBoundsMin,
		psTreeNode->adfBoundsMax,
		padfBoundsMin,
		padfBoundsMax,
		m_hTree->nDimension ) )
        return;
	
	/* -------------------------------------------------------------------- */
	/*      Grow the list to hold the shapes on this node.                  */
	/* -------------------------------------------------------------------- */
    if( *pnShapeCount + psTreeNode->nShapeCount > *pnMaxShapes )
    {
        *pnMaxShapes = (*pnShapeCount + psTreeNode->nShapeCount) * 2 + 20;
        *ppanShapeList = (int *) SfRealloc(*ppanShapeList,sizeof(int) * *pnMaxShapes);
    }
	
	if( *pnNodeCount + 1 > *pnMaxNodes )
    {
		*pnMaxNodes = (*pnMaxNodes + 1) * 2 + 20;
        *ppanOffsetList = (int *) SfRealloc(*ppanOffsetList,sizeof(int) * *pnMaxNodes);
        *ppanCountList = (int *) SfRealloc(*ppanCountList,sizeof(int) * *pnMaxNodes);
	}
	
	/* -------------------------------------------------------------------- */
	/*      Add the local nodes shapeids to the list.                       */
	/* -------------------------------------------------------------------- */
    for( i = 0; i < psTreeNode->nShapeCount; i++ )
    {
        (*ppanShapeList)[(*pnShapeCount)++] = psTreeNode->panShapeIds[i];
    }
	if(psTreeNode->nShapeCount>0)
	{
		(*pnNodeCount)++;
		(*ppanOffsetList)[*pnNodeCount-1] = psTreeNode->nOffset;
		(*ppanCountList)[*pnNodeCount-1] = psTreeNode->nShapeCount;
	}
	/* -------------------------------------------------------------------- */
	/*      Recurse to subnodes if they exist.                              */
	/* -------------------------------------------------------------------- */
    for( i = 0; i < psTreeNode->nSubNodes; i++ )
    {
        if( psTreeNode->apsSubNode[i] != NULL )
            SHPTreeCollectShapeIds( psTreeNode->apsSubNode[i],
			padfBoundsMin, padfBoundsMax,
			pnShapeCount, pnMaxShapes, pnMaxNodes,
			ppanShapeList, ppanOffsetList, ppanCountList, pnNodeCount );
    }
}

void CESRIShapeFile::BottomSort(int* anShapeList, double* pdBottoms, long l, long r)
{ 
	long i, last; 
	
	if(l>=r) 
		return; 
	SwapShapeID(&anShapeList[l], &anShapeList[(l+r)/2]); 
	SwapBottom(&pdBottoms[l], &pdBottoms[(l+r)/2]); 

	last = l; 
	for(i=(long)l+1;i<=(long)r;i++) 
    {
		if(pdBottoms[i] > pdBottoms[l]) 
		{
			SwapShapeID(&anShapeList[++last], &anShapeList[i]); 
			SwapBottom(&pdBottoms[last], &pdBottoms[i]);
		}
		SwapShapeID(&anShapeList[l], &anShapeList[last]); 
		SwapBottom(&pdBottoms[l], &pdBottoms[last]);
		BottomSort(anShapeList, pdBottoms, l, last-1); 
		BottomSort(anShapeList, pdBottoms, last+1, r); 
    }
} 

void CESRIShapeFile::QuickSort(int* anShapeList, long l, long r) 
{ 
	long i, last; 
	
	if(l>=r) 
		return; 
	SwapShapeID(&anShapeList[l], &anShapeList[(l+r)/2]); 
	last = l; 
	for(i=(long)l+1;i<=(long)r;i++) 
    {
		if(anShapeList[i] < anShapeList[l]) 
			SwapShapeID(&anShapeList[++last], &anShapeList[i]); 
		SwapShapeID(&anShapeList[l], &anShapeList[last]); 
		QuickSort(anShapeList, l, last-1); 
		QuickSort(anShapeList, last+1, r); 
    }
} 

void CESRIShapeFile::SwapBottom(double *i, double *j) 
{ 
	double temp = *i; 
	*i = *j;
	*j = temp;
}

void CESRIShapeFile::SwapShapeID(int *i, int *j) 
{ 
	int temp = *i; 
	*i = *j;
	*j = temp;
}

void CESRIShapeFile::QuickSort2(string* asValueList, int* anShapeList, long l, long r) 
{ 
	long i, last; 
	
	if(l>=r) 
		return; 
	SwapShapeID(&anShapeList[l], &anShapeList[(l+r)/2]); 
	SwapString(&asValueList[l], &asValueList[(l+r)/2]); 
	last = l; 
	for(i=(long)l+1;i<=(long)r;i++) 
		if(anShapeList[i] < anShapeList[l]) 
		{
			SwapString(&asValueList[++last], &asValueList[i]);  
			SwapShapeID(&anShapeList[last], &anShapeList[i]); 		
		}
	SwapString(&asValueList[l], &asValueList[last]);  
	SwapShapeID(&anShapeList[l], &anShapeList[last]); 
	QuickSort2(asValueList, anShapeList, l, last-1); 
	QuickSort2(asValueList, anShapeList, last+1, r); 
} 

void CESRIShapeFile::SwapString(string *i, string *j) 
{ 
	string temp = *i; 
	*i = *j;
	*j = temp;
}

bool CESRIShapeFile::CreateQTreeFile( const char * szFileName)
{
	//File Open
	//파일이 있으면 걍 지움
	ofstream theFile;
	theFile.open(szFileName, ios::out | ios::trunc | ios::binary);
	if(!theFile.good())
	{
		//DWORD dwRet = GetLastError();
        //const char * errorStr = strerror(errno);
		//CArchive archive(&theFile, CArchive::store);
		
		//트리를 순회하면서 각 노드값을 저장
		WriteTreeNodeToFile(m_hTree->psRoot, theFile);
		theFile.close();
		return true;
	}
	else
		return false;
}


bool CESRIShapeFile::CreateMBRFile(const char *szFileName)
{
	//File Open
	//파일이 있으면 걍 지움
	ofstream theFile;
	theFile.open(szFileName, ios::trunc | ios::out | ios::binary);
	if(!theFile.good())
	{
        //const char * errorStr = strerror(errno);
		
		//CArchive archive(&theFile, CArchive::store);
		/* 트리 순회 버전으로 수정 20040617 taeho ////////////////////////////////////
		SHPGetInfo( psTree->hSHP, &nEntities, &pnShapeType,padfMinBound, padfMaxBound );
		
		  SHPObject* pShape=NULL;
		  for(int i=0;i<nEntities;i++)
		  {
		  pShape = SHPReadObject(psTree->hSHP,i);
		  archive << (unsigned int)pShape->fxMin;
		  archive << (unsigned int)pShape->fyMin;
		  archive << (unsigned int)pShape->fxMax;
		  archive << (unsigned int)pShape->fyMax;
		  
			SHPDestroyObject(pShape);
			}
		///////////////////////////////////////////////////////////////////////*/
		WriteMBRFile(m_hTree->psRoot, theFile); // QTree를 따라서 각 shapeobject의 mbr파일 저장
		theFile.close();
		return true;
	}
	else
		return false;
}

void CESRIShapeFile::WriteTreeNodeToFile(SHPTreeNode * psTreeNode, ofstream& ar)
{
	//	if(psTreeNode == NULL)
	//		return;
	
	//Write Node to File
    /* region covered by this node */
    int i;
	for(i=0;i<4;i++)
	{
		ar << psTreeNode->adfBoundsMin[i];
	}
    for(i=0;i<4;i++)
	{
		ar << psTreeNode->adfBoundsMax[i];
	}
	
    /* list of shapes stored at this node.  The papsShapeObj pointers
	or the whole list can be NULL */
	ar << psTreeNode->nShapeCount;
	for(i=0;i<psTreeNode->nShapeCount;i++)
		ar << psTreeNode->panShapeIds[i];
	ar << psTreeNode->nSubNodes;
	
	//Call WriteTreeNodeToFile(this function) recursively
	for(i=0;i<psTreeNode->nSubNodes;i++)
	{
		//SHPTreeNode* pNode = psTreeNode->apsSubNode[i];
		WriteTreeNodeToFile(psTreeNode->apsSubNode[i], ar);
	}
	
	//Return
	return;
}


void CESRIShapeFile::WriteMBRFile(SHPTreeNode * psTreeNode, ofstream & ar)
{
	int i;
	if(psTreeNode->nShapeCount > 0)
	{
		CSHPObject* pShape=NULL;
		for(i=0;i<psTreeNode->nShapeCount;i++)
		{
			pShape = SHPReadObject(psTreeNode->panShapeIds[i]);
			ar << (unsigned int)pShape->fxMin;
			ar << (unsigned int)pShape->fyMin;
			ar << (unsigned int)pShape->fxMax;
			ar << (unsigned int)pShape->fyMax;
			
			delete pShape;
		}
	}
	//Call WriteMBRFile(this function) recursively
	for(i=0;i<psTreeNode->nSubNodes;i++)
	{
		WriteMBRFile(psTreeNode->apsSubNode[i], ar);
	}
}

/************************************************************************/
/*                             SHPCreate()                              */
/*                                                                      */
/*      Create a new shape file and return a handle to the open         */
/*      shape file with read/write access.                              */
/************************************************************************/
// bool CESRIShapeFile::SHPCreateW( const string & strLayer, int nShapeType )
// {
// 	char szLayer[MAX_PATH];
// 
// //	WideCharToMultiByte(CP_ACP, 0, strLayer, -1, szLayer, sizeof(szLayer), NULL, NULL);
// 
// 	return SHPCreate( szLayer, nShapeType );
// }

bool CESRIShapeFile::SHPCreate( const char * pszLayer, int nShapeType )
{
    char	*pszBasename, *pszFullname;
    int		i;
    FILE	*fpSHP = 0, *fpSHX = 0;
    unsigned char abyHeader[100];
    INT32	i32;
    double	dValue;
    
	/* -------------------------------------------------------------------- */
	/*      Establish the byte order on this system.                        */
	/* -------------------------------------------------------------------- */
    i = 1;
    if( *((unsigned char *) &i) == 1 )
        bBigEndian = false;
    else
        bBigEndian = true;
	
	/* -------------------------------------------------------------------- */
	/*	Compute the base (layer) name.  If there is any extension	*/
	/*	on the passed in filename we will strip it off.			*/
	/* -------------------------------------------------------------------- */
    //pszBasename = new char[ strlen(pszLayer) + 5 ];
	pszBasename = (char *) malloc(strlen(pszLayer)+5);
    strcpy( pszBasename, pszLayer );
    for( i = strlen(pszBasename)-1; 
	i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'
	       && pszBasename[i] != '\\';
	i-- ) {}
	
    if( pszBasename[i] == '.' )
        pszBasename[i] = '\0';
	
	/* -------------------------------------------------------------------- */
	/*      Open the two files so we can write their headers.               */
	/* -------------------------------------------------------------------- */
    //pszFullname = new char[ strlen(pszBasename) +5 ];
	pszFullname = (char *) malloc(strlen(pszBasename) + 5);
    sprintf( pszFullname, "%s.shp", pszBasename );
    fpSHP = fopen(pszFullname, "wb" );
    if( fpSHP == NULL )
	{
		//DWORD err = GetLastError();
		//TRACE("error %d %s\n", errno, pszFullname );

        return( NULL );
	}
	
    sprintf( pszFullname, "%s.shx", pszBasename );
    fpSHX = fopen(pszFullname, "wb" );
    if( fpSHX == NULL )
        return( NULL );
	
	//delete[] pszFullname;
	//delete[] pszBasename;
    free( pszFullname );
    free( pszBasename );
	
	/* -------------------------------------------------------------------- */
	/*      Prepare header block for .shp file.                             */
	/* -------------------------------------------------------------------- */
    for( i = 0; i < 100; i++ )
		abyHeader[i] = 0;
	
    abyHeader[2] = 0x27;			/* magic cookie */
    abyHeader[3] = 0x0a;
	
    i32 = 50;						/* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
    
    i32 = 1000;						/* version */
    ByteCopy( &i32, abyHeader+28, 4 );
    if( bBigEndian ) SwapWord( 4, abyHeader+28 );
    
    i32 = nShapeType;				/* shape type */
    ByteCopy( &i32, abyHeader+32, 4 );
    if( bBigEndian ) SwapWord( 4, abyHeader+32 );
	
    dValue = 0.0;					/* set bounds */
    ByteCopy( &dValue, abyHeader+36, 8 );
    ByteCopy( &dValue, abyHeader+44, 8 );
    ByteCopy( &dValue, abyHeader+52, 8 );
    ByteCopy( &dValue, abyHeader+60, 8 );
	
	/* -------------------------------------------------------------------- */
	/*      Write .shp file header.                                         */
	/* -------------------------------------------------------------------- */
    fwrite( abyHeader, 100, 1, fpSHP );
	
	/* -------------------------------------------------------------------- */
	/*      Prepare, and write .shx file header.                            */
	/* -------------------------------------------------------------------- */
    i32 = 50;						/* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
    
    fwrite( abyHeader, 100, 1, fpSHX );
	
	/* -------------------------------------------------------------------- */
	/*      Close the files, and then open them as regular existing files.  */
	/* -------------------------------------------------------------------- */
    fclose( fpSHP );
    fclose( fpSHX );
// 	
// 	string sDBF = pszLayer;
// 	int nExtPos = sDBF.ReverseFind('.');
// 	if(nExtPos >= 0 )
// 	{
// 		sDBF.Delete(nExtPos+1, 3);
// 		sDBF+="DBF";
// 	
// 		if(!DBFCreate(sDBF))
// 		{
// 			DBFClose();
//     		return false;
// 		}
// 	}

    return( (m_hSHP = SHPOpen( pszLayer, "r+b" )) != 0 );
}

/************************************************************************/
/*                              SwapWord()                              */
/*                                                                      */
/*      Swap a 2, 4 or 8 byte word.                                     */
/************************************************************************/
void	CESRIShapeFile::SwapWord( int length, void * wordP )
{
    int		i;
    unsigned char	temp;
	
    for( i=0; i < length/2; i++ )
    {
		temp = ((unsigned char*) wordP)[i];
		((unsigned char*)wordP)[i] = ((unsigned char*) wordP)[length-i-1];
		((unsigned char*) wordP)[length-i-1] = temp;
    }
}


CSHPObject* CESRIShapeFile::SHPReadObject(int hEntity, bool bSetOuterRing  )
{
    CSHPObject		*psShape;
	unsigned char *pabyRec = NULL;
	int	nBufSize = 0;
	
	/* -------------------------------------------------------------------- */
	/*      Validate the record/entity number.                              */
	/* -------------------------------------------------------------------- */
    if( hEntity < 0 || hEntity >= m_hSHP->nRecords )
        return( NULL );
	
	/* -------------------------------------------------------------------- */
	/*      Ensure our record buffer is large enough.                       */
	/* -------------------------------------------------------------------- */
    if( m_hSHP->panRecSize[hEntity]+8 > nBufSize )
    {
		nBufSize = m_hSHP->panRecSize[hEntity]+8;
		pabyRec = (unsigned char *) SfRealloc(pabyRec,nBufSize);
    }
	
	/* -------------------------------------------------------------------- */
	/*      Read the record.                                                */
	/* -------------------------------------------------------------------- */
	m_csQueueLock.lock();
    fseek( m_hSHP->fpSHP, m_hSHP->panRecOffset[hEntity], 0 );
    fread( pabyRec, m_hSHP->panRecSize[hEntity]+8, 1, m_hSHP->fpSHP );
	m_csQueueLock.unlock();
	
	/* -------------------------------------------------------------------- */
	/*	Allocate and minimally initialize the object.			*/
	/* -------------------------------------------------------------------- */
    psShape = new CSHPObject(); //(CSHPObject *) calloc(1,sizeof(CSHPObject));
    psShape->nShapeId = hEntity;
	
    memcpy( &psShape->nSHPType, pabyRec + 8, 4 );
    if( bBigEndian ) SwapWord( 4, &(psShape->nSHPType) );
	
	/* ==================================================================== */
	/*  Extract vertices for a Polygon or Arc.				*/
	/* ==================================================================== */
    if( psShape->nSHPType == SHPT_POLYGON || psShape->nSHPType == SHPT_LINE
        || psShape->nSHPType == SHPT_POLYGONZ
        || psShape->nSHPType == SHPT_POLYGONM
        || psShape->nSHPType == SHPT_ARCZ
        || psShape->nSHPType == SHPT_ARCM
        || psShape->nSHPType == SHPT_MULTIPATCH )
    {
		INT32		nPoints, nParts;
		int    		i, nOffset;
		
		/* -------------------------------------------------------------------- */
		/*	Get the X/Y bounds.						*/
		/* -------------------------------------------------------------------- */
        memcpy( &(psShape->fxMin), pabyRec + 8 +  4, 8 );
        memcpy( &(psShape->fyMin), pabyRec + 8 + 12, 8 );
        memcpy( &(psShape->fxMax), pabyRec + 8 + 20, 8 );
        memcpy( &(psShape->fyMax), pabyRec + 8 + 28, 8 );
		
		if( bBigEndian ) SwapWord( 8, &(psShape->fxMin) );
		if( bBigEndian ) SwapWord( 8, &(psShape->fyMin) );
		if( bBigEndian ) SwapWord( 8, &(psShape->fxMax) );
		if( bBigEndian ) SwapWord( 8, &(psShape->fyMax) );
		
		/* -------------------------------------------------------------------- */
		/*      Extract part/point count, and build vertex and part arrays      */
		/*      to proper size.                                                 */
		/* -------------------------------------------------------------------- */
		memcpy( &nPoints, pabyRec + 40 + 8, 4 );
		memcpy( &nParts, pabyRec + 36 + 8, 4 );
		
		if( bBigEndian ) SwapWord( 4, &nPoints );
		if( bBigEndian ) SwapWord( 4, &nParts );
		
		psShape->vertex_cnt = nPoints;
        psShape->pfX = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
        psShape->pfY = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
        psShape->pfZ = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
        psShape->pfM = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
		
		psShape->nParts = nParts;
        psShape->panPartStart = new int[nParts]; //(int *) calloc(nParts,sizeof(int));
        psShape->panPartType = new int[nParts]; //(int *) calloc(nParts,sizeof(int));
		
        for( i = 0; i < nParts; i++ )
            psShape->panPartType[i] = SHPP_RING;
		
		/* -------------------------------------------------------------------- */
		/*      Copy out the part array from the record.                        */
		/* -------------------------------------------------------------------- */
		memcpy( psShape->panPartStart, pabyRec + 44 + 8, 4 * nParts );
		for( i = 0; i < nParts; i++ )
		{
			if( bBigEndian ) SwapWord( 4, psShape->panPartStart+i );
		}
		
		nOffset = 44 + 8 + 4*nParts;
		
		/* -------------------------------------------------------------------- */
		/*      If this is a multipatch, we will also have parts types.         */
		/* -------------------------------------------------------------------- */
        if( psShape->nSHPType == SHPT_MULTIPATCH )
        {
            memcpy( psShape->panPartType, pabyRec + nOffset, 4*nParts );
            for( i = 0; i < nParts; i++ )
            {
                if( bBigEndian ) SwapWord( 4, psShape->panPartType+i );
            }
			
            nOffset += 4*nParts;
        }
        
		/* -------------------------------------------------------------------- */
		/*      Copy out the vertices from the record.                          */
		/* -------------------------------------------------------------------- */
		for( i = 0; i < nPoints; i++ )
		{
			memcpy(psShape->pfX + i,
				pabyRec + nOffset + i * 16,
				8 );
			
			memcpy(psShape->pfY + i,
				pabyRec + nOffset + i * 16 + 8,
				8 );
			
			if( bBigEndian ) SwapWord( 8, psShape->pfX + i );
			if( bBigEndian ) SwapWord( 8, psShape->pfY + i );
		}
		
        nOffset += 16*nPoints;
        
		/* -------------------------------------------------------------------- */
		/*      If we have a Z coordinate, collect that now.                    */
		/* -------------------------------------------------------------------- */
        if( psShape->nSHPType == SHPT_POLYGONZ
            || psShape->nSHPType == SHPT_ARCZ
            || psShape->nSHPType == SHPT_MULTIPATCH )
        {
            memcpy( &(psShape->fzMin), pabyRec + nOffset, 8 );
            memcpy( &(psShape->fzMax), pabyRec + nOffset + 8, 8 );
            
            if( bBigEndian ) SwapWord( 8, &(psShape->fzMin) );
            if( bBigEndian ) SwapWord( 8, &(psShape->fzMax) );
            
            for( i = 0; i < nPoints; i++ )
            {
                memcpy( psShape->pfZ + i,
					pabyRec + nOffset + 16 + i*8, 8 );
                if( bBigEndian ) SwapWord( 8, psShape->pfZ + i );
            }
			
            nOffset += 16 + 8*nPoints;
        }
		
		/* -------------------------------------------------------------------- */
		/*      If we have a M measure value, then read it now.  We assume      */
		/*      that the measure can be present for any shape if the size is    */
		/*      big enough, but really it will only occur for the Z shapes      */
		/*      (options), and the M shapes.                                    */
		/* -------------------------------------------------------------------- */
        if( m_hSHP->panRecSize[hEntity]+8 >= nOffset + 16 + 8*nPoints )
        {
            memcpy( &(psShape->fmidMin), pabyRec + nOffset, 8 );
            memcpy( &(psShape->fmidMax), pabyRec + nOffset + 8, 8 );
            
            if( bBigEndian ) SwapWord( 8, &(psShape->fmidMin) );
            if( bBigEndian ) SwapWord( 8, &(psShape->fmidMax) );
            
            for( i = 0; i < nPoints; i++ )
            {
                memcpy( psShape->pfM + i,
					pabyRec + nOffset + 16 + i*8, 8 );
                if( bBigEndian ) SwapWord( 8, psShape->pfM + i );
            }
        }

		// by hsb

		if( bSetOuterRing )
			GetOuterRingFlag_ShapePolyPart( psShape , psShape->panPartType, nParts );
        
    }
	
	/* ==================================================================== */
	/*  Extract vertices for a MultiPoint.					*/
	/* ==================================================================== */
    else if( psShape->nSHPType == SHPT_MULTIPOINT
		|| psShape->nSHPType == SHPT_MULTIPOINTM
		|| psShape->nSHPType == SHPT_MULTIPOINTZ )
    {
		INT32		nPoints;
		int    		i, nOffset;
		
		memcpy( &nPoints, pabyRec + 44, 4 );
		if( bBigEndian ) SwapWord( 4, &nPoints );
		
		psShape->vertex_cnt = nPoints;
        psShape->pfX = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
        psShape->pfY = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
        psShape->pfZ = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
        psShape->pfM = new double[nPoints]; //(double *) calloc(nPoints,sizeof(double));
		
		for( i = 0; i < nPoints; i++ )
		{
			memcpy(psShape->pfX+i, pabyRec + 48 + 16 * i, 8 );
			memcpy(psShape->pfY+i, pabyRec + 48 + 16 * i + 8, 8 );
			
			if( bBigEndian ) SwapWord( 8, psShape->pfX + i );
			if( bBigEndian ) SwapWord( 8, psShape->pfY + i );
		}
		
        nOffset = 48 + 16*nPoints;
        
		/* -------------------------------------------------------------------- */
		/*	Get the X/Y bounds.						*/
		/* -------------------------------------------------------------------- */
        memcpy( &(psShape->fxMin), pabyRec + 8 +  4, 8 );
        memcpy( &(psShape->fyMin), pabyRec + 8 + 12, 8 );
        memcpy( &(psShape->fxMax), pabyRec + 8 + 20, 8 );
        memcpy( &(psShape->fyMax), pabyRec + 8 + 28, 8 );
		
		if( bBigEndian ) SwapWord( 8, &(psShape->fxMin) );
		if( bBigEndian ) SwapWord( 8, &(psShape->fyMin) );
		if( bBigEndian ) SwapWord( 8, &(psShape->fxMax) );
		if( bBigEndian ) SwapWord( 8, &(psShape->fyMax) );
		
		/* -------------------------------------------------------------------- */
		/*      If we have a Z coordinate, collect that now.                    */
		/* -------------------------------------------------------------------- */
        if( psShape->nSHPType == SHPT_MULTIPOINTZ )
        {
            memcpy( &(psShape->fzMin), pabyRec + nOffset, 8 );
            memcpy( &(psShape->fzMax), pabyRec + nOffset + 8, 8 );
            
            if( bBigEndian ) SwapWord( 8, &(psShape->fzMin) );
            if( bBigEndian ) SwapWord( 8, &(psShape->fzMax) );
            
            for( i = 0; i < nPoints; i++ )
            {
                memcpy( psShape->pfZ + i,
					pabyRec + nOffset + 16 + i*8, 8 );
                if( bBigEndian ) SwapWord( 8, psShape->pfZ + i );
            }
			
            nOffset += 16 + 8*nPoints;
        }
		
		/* -------------------------------------------------------------------- */
		/*      If we have a M measure value, then read it now.  We assume      */
		/*      that the measure can be present for any shape if the size is    */
		/*      big enough, but really it will only occur for the Z shapes      */
		/*      (options), and the M shapes.                                    */
		/* -------------------------------------------------------------------- */
        if( m_hSHP->panRecSize[hEntity]+8 >= nOffset + 16 + 8*nPoints )
        {
            memcpy( &(psShape->fmidMin), pabyRec + nOffset, 8 );
            memcpy( &(psShape->fmidMax), pabyRec + nOffset + 8, 8 );
            
            if( bBigEndian ) SwapWord( 8, &(psShape->fmidMin) );
            if( bBigEndian ) SwapWord( 8, &(psShape->fmidMax) );
            
            for( i = 0; i < nPoints; i++ )
            {
                memcpy( psShape->pfM + i,
					pabyRec + nOffset + 16 + i*8, 8 );
                if( bBigEndian ) SwapWord( 8, psShape->pfM + i );
            }
        }
    }
	
	/* ==================================================================== */
	/*      Extract vertices for a point.                                   */
	/* ==================================================================== */
    else if( psShape->nSHPType == SHPT_POINT
		|| psShape->nSHPType == SHPT_POINTM
		|| psShape->nSHPType == SHPT_POINTZ )
    {
        int	nOffset;
        
		psShape->vertex_cnt = 1;
        psShape->pfX = new double[1]; //(double *) calloc(1,sizeof(double));
        psShape->pfY = new double[1]; //(double *) calloc(1,sizeof(double));
        psShape->pfZ = new double[1]; //(double *) calloc(1,sizeof(double));
        psShape->pfM = new double[1]; //(double *) calloc(1,sizeof(double));
		
		memcpy( psShape->pfX, pabyRec + 12, 8 );
		memcpy( psShape->pfY, pabyRec + 20, 8 );
		
		if( bBigEndian ) SwapWord( 8, psShape->pfX );
		if( bBigEndian ) SwapWord( 8, psShape->pfY );
		
        nOffset = 20 + 8;
        
		/* -------------------------------------------------------------------- */
		/*      If we have a Z coordinate, collect that now.                    */
		/* -------------------------------------------------------------------- */
        if( psShape->nSHPType == SHPT_POINTZ )
        {
            memcpy( psShape->pfZ, pabyRec + nOffset, 8 );
			
            if( bBigEndian ) SwapWord( 8, psShape->pfZ );
            
            nOffset += 8;
        }
		
		/* -------------------------------------------------------------------- */
		/*      If we have a M measure value, then read it now.  We assume      */
		/*      that the measure can be present for any shape if the size is    */
		/*      big enough, but really it will only occur for the Z shapes      */
		/*      (options), and the M shapes.                                    */
		/* -------------------------------------------------------------------- */
        if( m_hSHP->panRecSize[hEntity]+8 >= nOffset + 8 )
        {
            memcpy( psShape->pfM, pabyRec + nOffset, 8 );
			
            if( bBigEndian ) SwapWord( 8, psShape->pfM );
        }
		
		/* -------------------------------------------------------------------- */
		/*      Since no extents are supplied in the record, we will apply      */
		/*      them from the single vertex.                                    */
		/* -------------------------------------------------------------------- */
        psShape->fxMin = psShape->fxMax = psShape->pfX[0];
        psShape->fyMin = psShape->fyMax = psShape->pfY[0];
        psShape->fzMin = psShape->fzMax = psShape->pfZ[0];
        psShape->fmidMin = psShape->fmidMax = psShape->pfM[0];
    }
	free(pabyRec); //delete[] pabyRec;
    return( psShape );
}

/************************************************************************/
/*                           SHPWriteObject()                           */
/*                                                                      */
/*      Write out the vertices of a new structure.  Note that it is     */
/*      only possible to write vertices at the end of the file.         */
/************************************************************************/

int CESRIShapeFile::SHPWriteObject(int nShapeId, CSHPObject * psObject )

{
    int	       	nRecordOffset, i, nRecordSize;
    unsigned char	*pabyRec;
    INT32	i32;
	
    m_hSHP->bUpdated = true;
	
	/* -------------------------------------------------------------------- */
	/*      Ensure that shape object matches the type of the file it is     */
	/*      being written to.                                               */
	/* -------------------------------------------------------------------- */
    assert( psObject->nSHPType == m_hSHP->nShapeType 
		|| psObject->nSHPType == SHPT_NULL );
	
	/* -------------------------------------------------------------------- */
	/*      Ensure that -1 is used for appends.  Either blow an             */
	/*      assertion, or if they are disabled, set the shapeid to -1       */
	/*      for appends.                                                    */
	/* -------------------------------------------------------------------- */
    assert( nShapeId == -1 
		|| (nShapeId >= 0 && nShapeId < m_hSHP->nRecords) );
	
    if( nShapeId != -1 && nShapeId >= m_hSHP->nRecords )
        nShapeId = -1;
	
	/* -------------------------------------------------------------------- */
	/*      Add the new entity to the in memory index.                      */
	/* -------------------------------------------------------------------- */
    if( nShapeId == -1 && m_hSHP->nRecords+1 > m_hSHP->nMaxRecords )
    {
		m_hSHP->nMaxRecords =(int) ( m_hSHP->nMaxRecords * 1.3 + 100);

		m_hSHP->panRecOffset = (int *) SfRealloc(m_hSHP->panRecOffset,
			sizeof(int) * m_hSHP->nMaxRecords );
		m_hSHP->panRecSize = (int *) SfRealloc(m_hSHP->panRecSize,
			sizeof(int) * m_hSHP->nMaxRecords );
    }
	
	/* -------------------------------------------------------------------- */
	/*      Initialize record.                                              */
	/* -------------------------------------------------------------------- */
    //pabyRec = new unsigned char[ psObject->vertex_cnt * 4 * sizeof(double) + psObject->nParts * 8 + 128 ];
	pabyRec =(unsigned char *) malloc(psObject->vertex_cnt * 4 * sizeof(double) + psObject->nParts * 8 + 128);
    
	/* -------------------------------------------------------------------- */
	/*  Extract vertices for a Polygon or Arc.				*/
	/* -------------------------------------------------------------------- */
    if( psObject->nSHPType == SHPT_POLYGON
        || psObject->nSHPType == SHPT_POLYGONZ
        || psObject->nSHPType == SHPT_POLYGONM
        || psObject->nSHPType == SHPT_LINE 
        || psObject->nSHPType == SHPT_ARCZ
        || psObject->nSHPType == SHPT_ARCM
        || psObject->nSHPType == SHPT_MULTIPATCH )
    {
		INT32		nPoints, nParts;
		int    		i;
		
		nPoints = psObject->vertex_cnt;
		nParts = psObject->nParts;
		
		_SHPSetBounds( pabyRec + 12, psObject );
		
		if( bBigEndian ) SwapWord( 4, &nPoints );
		if( bBigEndian ) SwapWord( 4, &nParts );
		
		ByteCopy( &nPoints, pabyRec + 40 + 8, 4 );
		ByteCopy( &nParts, pabyRec + 36 + 8, 4 );
		
        nRecordSize = 52;
		
        /*
		* Write part start positions.
		*/
		ByteCopy( psObject->panPartStart, pabyRec + 44 + 8,
			4 * psObject->nParts );
		for( i = 0; i < psObject->nParts; i++ )
		{
			if( bBigEndian ) SwapWord( 4, pabyRec + 44 + 8 + 4*i );
            nRecordSize += 4;
		}
		
        /*
		* Write multipatch part types if needed.
		*/
        if( psObject->nSHPType == SHPT_MULTIPATCH )
        {
            memcpy( pabyRec + nRecordSize, psObject->panPartType,
				4*psObject->nParts );
            for( i = 0; i < psObject->nParts; i++ )
            {
                if( bBigEndian ) SwapWord( 4, pabyRec + nRecordSize );
                nRecordSize += 4;
            }
        }
		
        /*
		* Write the (x,y) vertex values.
		*/
		for( i = 0; i < psObject->vertex_cnt; i++ )
		{
			ByteCopy( psObject->pfX + i, pabyRec + nRecordSize, 8 );
			ByteCopy( psObject->pfY + i, pabyRec + nRecordSize + 8, 8 );
			
			if( bBigEndian )
                SwapWord( 8, pabyRec + nRecordSize );
            
			if( bBigEndian )
                SwapWord( 8, pabyRec + nRecordSize + 8 );
			
            nRecordSize += 2 * 8;
		}
		
        /*
		* Write the Z coordinates (if any).
		*/
        if( psObject->nSHPType == SHPT_POLYGONZ
            || psObject->nSHPType == SHPT_ARCZ
            || psObject->nSHPType == SHPT_MULTIPATCH )
        {
            ByteCopy( &(psObject->fzMin), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
            
            ByteCopy( &(psObject->fzMax), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
			
            for( i = 0; i < psObject->vertex_cnt; i++ )
            {
                ByteCopy( psObject->pfZ + i, pabyRec + nRecordSize, 8 );
                if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
                nRecordSize += 8;
            }
        }
		
        /*
		* Write the M values, if any.
		*/
        if( psObject->nSHPType == SHPT_POLYGONM
            || psObject->nSHPType == SHPT_ARCM
#ifndef DISABLE_MULTIPATCH_MEASURE            
            || psObject->nSHPType == SHPT_MULTIPATCH
#endif            
            || psObject->nSHPType == SHPT_POLYGONZ
            || psObject->nSHPType == SHPT_ARCZ )
        {
            ByteCopy( &(psObject->fmidMin), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
            
            ByteCopy( &(psObject->fmidMax), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
			
            for( i = 0; i < psObject->vertex_cnt; i++ )
            {
                ByteCopy( psObject->pfM + i, pabyRec + nRecordSize, 8 );
                if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
                nRecordSize += 8;
            }
        }
    }
	
	/* -------------------------------------------------------------------- */
	/*  Extract vertices for a MultiPoint.					*/
	/* -------------------------------------------------------------------- */
    else if( psObject->nSHPType == SHPT_MULTIPOINT
		|| psObject->nSHPType == SHPT_MULTIPOINTZ
		|| psObject->nSHPType == SHPT_MULTIPOINTM )
    {
		INT32		nPoints;
		int    		i;
		
		nPoints = psObject->vertex_cnt;
		
        _SHPSetBounds( pabyRec + 12, psObject );
		
		if( bBigEndian ) SwapWord( 4, &nPoints );
		ByteCopy( &nPoints, pabyRec + 44, 4 );
		
		for( i = 0; i < psObject->vertex_cnt; i++ )
		{
			ByteCopy( psObject->pfX + i, pabyRec + 48 + i*16, 8 );
			ByteCopy( psObject->pfY + i, pabyRec + 48 + i*16 + 8, 8 );
			
			if( bBigEndian ) SwapWord( 8, pabyRec + 48 + i*16 );
			if( bBigEndian ) SwapWord( 8, pabyRec + 48 + i*16 + 8 );
		}
		
		nRecordSize = 48 + 16 * psObject->vertex_cnt;
		
        if( psObject->nSHPType == SHPT_MULTIPOINTZ )
        {
            ByteCopy( &(psObject->fzMin), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
			
            ByteCopy( &(psObject->fzMax), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
            
            for( i = 0; i < psObject->vertex_cnt; i++ )
            {
                ByteCopy( psObject->pfZ + i, pabyRec + nRecordSize, 8 );
                if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
                nRecordSize += 8;
            }
        }
		
        if( psObject->nSHPType == SHPT_MULTIPOINTZ
            || psObject->nSHPType == SHPT_MULTIPOINTM )
        {
            ByteCopy( &(psObject->fmidMin), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
			
            ByteCopy( &(psObject->fmidMax), pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
            
            for( i = 0; i < psObject->vertex_cnt; i++ )
            {
                ByteCopy( psObject->pfM + i, pabyRec + nRecordSize, 8 );
                if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
                nRecordSize += 8;
            }
        }
    }
	
	/* -------------------------------------------------------------------- */
	/*      Write point.							*/
	/* -------------------------------------------------------------------- */
    else if( psObject->nSHPType == SHPT_POINT
		|| psObject->nSHPType == SHPT_POINTZ
		|| psObject->nSHPType == SHPT_POINTM )
    {
		ByteCopy( psObject->pfX, pabyRec + 12, 8 );
		ByteCopy( psObject->pfY, pabyRec + 20, 8 );
		
		if( bBigEndian ) SwapWord( 8, pabyRec + 12 );
		if( bBigEndian ) SwapWord( 8, pabyRec + 20 );
		
        nRecordSize = 28;
        
        if( psObject->nSHPType == SHPT_POINTZ )
        {
            ByteCopy( psObject->pfZ, pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
        }
        
        if( psObject->nSHPType == SHPT_POINTZ
            || psObject->nSHPType == SHPT_POINTM )
        {
            ByteCopy( psObject->pfM, pabyRec + nRecordSize, 8 );
            if( bBigEndian ) SwapWord( 8, pabyRec + nRecordSize );
            nRecordSize += 8;
        }
    }
	
	/* -------------------------------------------------------------------- */
	/*      Not much to do for null geometries.                             */
	/* -------------------------------------------------------------------- */
    else if( psObject->nSHPType == SHPT_NULL )
    {
        nRecordSize = 12;
    }
	
    else
    {
        /* unknown type */
        assert( false );
    }
	
	/* -------------------------------------------------------------------- */
	/*      Establish where we are going to put this record. If we are      */
	/*      rewriting and existing record, and it will fit, then put it     */
	/*      back where the original came from.  Otherwise write at the end. */
	/* -------------------------------------------------------------------- */
    if( nShapeId == -1 || m_hSHP->panRecSize[nShapeId] < nRecordSize-8 )
    {
        if( nShapeId == -1 )
            nShapeId = m_hSHP->nRecords++;
		
        m_hSHP->panRecOffset[nShapeId] = nRecordOffset = m_hSHP->nFileSize;
        m_hSHP->panRecSize[nShapeId] = nRecordSize-8;
        m_hSHP->nFileSize += nRecordSize;
    }
    else
    {
        nRecordOffset = m_hSHP->panRecOffset[nShapeId];
    }
    
	/* -------------------------------------------------------------------- */
	/*      Set the shape type, record number, and record size.             */
	/* -------------------------------------------------------------------- */
    i32 = nShapeId+1;					/* record # */
    if( !bBigEndian ) SwapWord( 4, &i32 );
    ByteCopy( &i32, pabyRec, 4 );
	
    i32 = (nRecordSize-8)/2;				/* record size */
    if( !bBigEndian ) SwapWord( 4, &i32 );
    ByteCopy( &i32, pabyRec + 4, 4 );
	
    i32 = psObject->nSHPType;				/* shape type */
    if( bBigEndian ) SwapWord( 4, &i32 );
    ByteCopy( &i32, pabyRec + 8, 4 );
	
	/* -------------------------------------------------------------------- */
	/*      Write out record.                                               */
	/* -------------------------------------------------------------------- */
    if( fseek( m_hSHP->fpSHP, nRecordOffset, 0 ) != 0
        || fwrite( pabyRec, nRecordSize, 1, m_hSHP->fpSHP ) < 1 )
    {
        printf( "Error in fseek() or fwrite().\n" );
		//delete[] pabyRec;
        free( pabyRec );
        return -1;
    }
    //delete[] pabyRec;
    free( pabyRec );
	
	/* -------------------------------------------------------------------- */
	/*	Expand file wide bounds based on this shape.			*/
	/* -------------------------------------------------------------------- */
    if( m_hSHP->adBoundsMin[0] == 0.0
        && m_hSHP->adBoundsMax[0] == 0.0
        && m_hSHP->adBoundsMin[1] == 0.0
        && m_hSHP->adBoundsMax[1] == 0.0 
        && psObject->nSHPType != SHPT_NULL )
    {
        m_hSHP->adBoundsMin[0] = m_hSHP->adBoundsMax[0] = psObject->pfX[0];
        m_hSHP->adBoundsMin[1] = m_hSHP->adBoundsMax[1] = psObject->pfY[0];
        m_hSHP->adBoundsMin[2] = m_hSHP->adBoundsMax[2] = psObject->pfZ[0];
        m_hSHP->adBoundsMin[3] = m_hSHP->adBoundsMax[3] = psObject->pfM[0];
    }
	
    for( i = 0; i < psObject->vertex_cnt; i++ )
    {
		m_hSHP->adBoundsMin[0] = MIN(m_hSHP->adBoundsMin[0],psObject->pfX[i]);
		m_hSHP->adBoundsMin[1] = MIN(m_hSHP->adBoundsMin[1],psObject->pfY[i]);
		m_hSHP->adBoundsMin[2] = MIN(m_hSHP->adBoundsMin[2],psObject->pfZ[i]);
		m_hSHP->adBoundsMin[3] = MIN(m_hSHP->adBoundsMin[3],psObject->pfM[i]);
		m_hSHP->adBoundsMax[0] = MAX(m_hSHP->adBoundsMax[0],psObject->pfX[i]);
		m_hSHP->adBoundsMax[1] = MAX(m_hSHP->adBoundsMax[1],psObject->pfY[i]);
		m_hSHP->adBoundsMax[2] = MAX(m_hSHP->adBoundsMax[2],psObject->pfZ[i]);
		m_hSHP->adBoundsMax[3] = MAX(m_hSHP->adBoundsMax[3],psObject->pfM[i]);
    }
	
    return( nShapeId  );
}

/************************************************************************/
/*                           _SHPSetBounds()                            */
/*                                                                      */
/*      Compute a bounds rectangle for a shape, and set it into the     */
/*      indicated location in the record.                               */
/************************************************************************/

void	CESRIShapeFile::_SHPSetBounds( unsigned char * pabyRec, CSHPObject * psShape )

{
    ByteCopy( &(psShape->fxMin), pabyRec +  0, 8 );
    ByteCopy( &(psShape->fyMin), pabyRec +  8, 8 );
    ByteCopy( &(psShape->fxMax), pabyRec + 16, 8 );
    ByteCopy( &(psShape->fyMax), pabyRec + 24, 8 );
	
    if( bBigEndian )
    {
        SwapWord( 8, pabyRec + 0 );
        SwapWord( 8, pabyRec + 8 );
        SwapWord( 8, pabyRec + 16 );
        SwapWord( 8, pabyRec + 24 );
    }
}

/************************************************************************/
/*                         SHPComputeExtents()                          */
/*                                                                      */
/*      Recompute the extents of a shape.  Automatically done by        */
/*      SHPCreateObject().                                              */
/************************************************************************/

void CESRIShapeFile::SHPComputeExtents( CSHPObject * psObject )

{
    int		i;
    
	/* -------------------------------------------------------------------- */
	/*      Build extents for this object.                                  */
	/* -------------------------------------------------------------------- */
    if( psObject->vertex_cnt > 0 )
    {
        psObject->fxMin = psObject->fxMax = psObject->pfX[0];
        psObject->fyMin = psObject->fyMax = psObject->pfY[0];
        psObject->fzMin = psObject->fzMax = psObject->pfZ[0];
        psObject->fmidMin = psObject->fmidMax = psObject->pfM[0];
    }
    
    for( i = 0; i < psObject->vertex_cnt; i++ )
    {
        psObject->fxMin = MIN(psObject->fxMin, psObject->pfX[i]);
        psObject->fyMin = MIN(psObject->fyMin, psObject->pfY[i]);
        psObject->fzMin = MIN(psObject->fzMin, psObject->pfZ[i]);
        psObject->fmidMin = MIN(psObject->fmidMin, psObject->pfM[i]);
		
        psObject->fxMax = MAX(psObject->fxMax, psObject->pfX[i]);
        psObject->fyMax = MAX(psObject->fyMax, psObject->pfY[i]);
        psObject->fzMax = MAX(psObject->fzMax, psObject->pfZ[i]);
        psObject->fmidMax = MAX(psObject->fmidMax, psObject->pfM[i]);
    }
}

/************************************************************************/
/*                          SHPCreateObject()                           */
/*                                                                      */
/*      Create a shape object.  It should be freed with                 */
/*      SHPDestroyObject().                                             */
/************************************************************************/
CSHPObject * CESRIShapeFile::SHPCreateObject( int nSHPType, int nShapeId, int nParts,
							 int * panPartStart, int * panPartType,
							 int vertex_cnt, double * pfX, double * pfY,
							 double * pfZ, double * pfM ,GPOINT *pt)
{
    CSHPObject	*psObject;
    int		i, bHasM, bHasZ;
	
    psObject = new CSHPObject();
		//(CSHPObject *) calloc(1,sizeof(CSHPObject));
    psObject->nSHPType = nSHPType;
    psObject->nShapeId = nShapeId;
	
	/* -------------------------------------------------------------------- */
	/*	Establish whether this shape type has M, and Z values.		*/
	/* -------------------------------------------------------------------- */
    if( nSHPType == SHPT_ARCM
        || nSHPType == SHPT_POINTM
        || nSHPType == SHPT_POLYGONM
        || nSHPType == SHPT_MULTIPOINTM )
    {
        bHasM = true;
        bHasZ = false;
    }
    else if( nSHPType == SHPT_ARCZ
		|| nSHPType == SHPT_POINTZ
		|| nSHPType == SHPT_POLYGONZ
		|| nSHPType == SHPT_MULTIPOINTZ
		|| nSHPType == SHPT_MULTIPATCH )
    {
        bHasM = true;
        bHasZ = true;
    }
    else
    {
        bHasM = false;
        bHasZ = false;
    }
	
	/* -------------------------------------------------------------------- */
	/*      Capture parts.  Note that part type is optional, and            */
	/*      defaults to ring.                                               */
	/* -------------------------------------------------------------------- */
    if( nSHPType == SHPT_LINE || nSHPType == SHPT_POLYGON
        || nSHPType == SHPT_ARCM || nSHPType == SHPT_POLYGONM
        || nSHPType == SHPT_ARCZ || nSHPType == SHPT_POLYGONZ
        || nSHPType == SHPT_MULTIPATCH )
    {
        psObject->nParts = MAX(1,nParts);
		
        psObject->panPartStart = (int *) malloc(sizeof(int) * psObject->nParts); //new int[psObject->nParts];
	    psObject->panPartType = (int *) malloc(sizeof(int) * psObject->nParts); //new int[psObject->nParts];
				
        psObject->panPartStart[0] = 0;
        psObject->panPartType[0] = SHPP_RING;
        
        for( i = 0; i < nParts; i++ )
        {
            psObject->panPartStart[i] = panPartStart[i];
            if( panPartType != NULL )
                psObject->panPartType[i] = panPartType[i];
            else
                psObject->panPartType[i] = SHPP_RING;
        }
    }
	
	/* -------------------------------------------------------------------- */
	/*      Capture vertices.  Note that Z and M are optional, but X and    */
	/*      Y are not.                                                      */
	/* -------------------------------------------------------------------- */
    if( vertex_cnt > 0 )
    {
        psObject->pfX = new double[vertex_cnt]; //(double *) calloc(sizeof(double),vertex_cnt);
        psObject->pfY = new double[vertex_cnt]; //(double *) calloc(sizeof(double),vertex_cnt);
        psObject->pfZ = new double[vertex_cnt]; //(double *) calloc(sizeof(double),vertex_cnt);
        psObject->pfM = new double[vertex_cnt]; //(double *) calloc(sizeof(double),vertex_cnt);
		
		if(pt == NULL )
		{
			assert( pfX != NULL );
			assert( pfY != NULL );
		}
		if( pt ) // by hsb
		{
			for( i = 0; i < vertex_cnt; i++ )
			{
				psObject->pfX[i] = pt[i].x;
				psObject->pfY[i] = pt[i].y;


				memset (psObject->pfZ, 0x00, sizeof (double) * vertex_cnt);
				memset (psObject->pfM, 0x00, sizeof (double) * vertex_cnt);
			}		
		}
		else
		{
			for( i = 0; i < vertex_cnt; i++ )
			{

				psObject->pfX[i] = pfX[i];
				psObject->pfY[i] = pfY[i];
				if( pfZ )
					psObject->pfZ[i] = pfZ[i];
				
				memset (psObject->pfM, 0x00, sizeof (double) * vertex_cnt);

				if( pfZ != NULL && bHasZ )
					psObject->pfZ[i] = pfZ[i];
				if( pfM != NULL && bHasM )
					psObject->pfM[i] = pfM[i];
			}
		}


    }
	
	/* -------------------------------------------------------------------- */
	/*      Compute the extents.                                            */
	/* -------------------------------------------------------------------- */
    psObject->vertex_cnt = vertex_cnt;
    SHPComputeExtents( psObject );
	
    return( psObject );
}
							 
void CESRIShapeFile::SHPClose()
{
	if(!m_hSHP) return;
	/* -------------------------------------------------------------------- */
	/*	Update the header if we have modified anything.			*/
	/* -------------------------------------------------------------------- */
	if( m_hSHP->bUpdated )
	{
		SHPWriteHeader();
	}
	
	/* -------------------------------------------------------------------- */
	/*      Free all resources, and close files.                            */
	/* -------------------------------------------------------------------- */
	free( m_hSHP->panRecOffset ); //delete[] m_hSHP->panRecOffset; 
	free( m_hSHP->panRecSize );   //delete[] m_hSHP->panRecSize; 
	
	fclose( m_hSHP->fpSHX );
	fclose( m_hSHP->fpSHP );
	
	free( m_hSHP );
	//delete m_hSHP;

	m_hSHP = NULL;
	
	/*
	if( pabyRec != NULL )
	{
		free( pabyRec );
		pabyRec = NULL;
		nBufSize = 0;
	}
	*/
}

/************************************************************************/
/*                              SHPOpen()                               */
/*                                                                      */
/*      Open the .shp and .shx files based on the basename of the       */
/*      files or either file name.                                      */
/************************************************************************/
// CESRIShapeFile::SHPHandle CESRIShapeFile::SHPOpenW( const string & strLayer , const char * pAccess )
// {
// 	char szLayer[MAX_PATH];
// 
// 	WideCharToMultiByte(CP_ACP, 0, strLayer, -1, szLayer, sizeof(szLayer), NULL, NULL);
// 
// 	return SHPOpen( szLayer, pAccess );
// }
CESRIShapeFile::SHPHandle CESRIShapeFile::SHPOpen( const char * pszLayer, const char * pszAccess )
{
	char		*pszFullname, *pszBasename;
	
	unsigned char		*pabyBuf;
	int			i;
	double		dValue;
	
	/* -------------------------------------------------------------------- */
	/*      Ensure the access string is one of the legal ones.  We          */
	/*      ensure the result string indicates binary to avoid common       */
	/*      problems on Windows.                                            */
	/* -------------------------------------------------------------------- */
	if( strcmp(pszAccess,"rb+") == 0 || strcmp(pszAccess,"r+b") == 0
		|| strcmp(pszAccess,"r+") == 0 )
		pszAccess = "r+b";
	else
		pszAccess = "rb";

	/* -------------------------------------------------------------------- */
	/*	Establish the byte order on this machine.			*/
	/* -------------------------------------------------------------------- */
	i = 1;
	if( *((unsigned char *) &i) == 1 )
		bBigEndian = false;
	else
		bBigEndian = true;
	
	/* -------------------------------------------------------------------- */
	/*	Initialize the info structure.					*/
	/* -------------------------------------------------------------------- */
	m_hSHP = (SHPHandle) malloc(sizeof(SHPInfo)); //new SHPInfo; 
	
	m_hSHP->bUpdated = false;
	
	/* -------------------------------------------------------------------- */
	/*	Compute the base (layer) name.  If there is any extension	*/
	/*	on the passed in filename we will strip it off.			*/
	/* -------------------------------------------------------------------- */
	pszBasename = (char *) malloc(strlen(pszLayer)+5); //new char[strlen(pszLayer)+5]; 
	strcpy( pszBasename, pszLayer );
	for( i = strlen(pszBasename)-1; 
			i > 0 && pszBasename[i] != '.' && pszBasename[i] != '/'	&& pszBasename[i] != '\\';
			i-- ) {}
	
	if( pszBasename[i] == '.' )
		pszBasename[i] = '\0';
	
	/* -------------------------------------------------------------------- */
	/*	Open the .shp and .shx files.  Note that files pulled from	*/
	/*	a PC to Unix with upper case filenames won't work!		*/
	/* -------------------------------------------------------------------- */
	pszFullname = (char *) malloc(strlen(pszBasename) + 5); //new char[strlen(pszBasename)+5]; 
	sprintf( pszFullname, "%s.shp", pszBasename );
	m_hSHP->fpSHP = fopen(pszFullname, pszAccess );
	if( m_hSHP->fpSHP == NULL )
	{
		sprintf( pszFullname, "%s.SHP", pszBasename );
		m_hSHP->fpSHP = fopen(pszFullname, pszAccess );
	}
	
	if( m_hSHP->fpSHP == NULL )
	{
		free( m_hSHP ); //delete m_hSHP; 
		free( pszBasename ); //delete[] pszBasename; 
		free( pszFullname ); //delete[] pszFullname; 
		return( NULL );
	}
	
	sprintf( pszFullname, "%s.shx", pszBasename );
	m_hSHP->fpSHX = fopen(pszFullname, pszAccess );
	if( m_hSHP->fpSHX == NULL )
	{
		sprintf( pszFullname, "%s.SHX", pszBasename );
		m_hSHP->fpSHX = fopen(pszFullname, pszAccess );
	}
	
	if( m_hSHP->fpSHX == NULL )
	{
		fclose( m_hSHP->fpSHX );
		free( m_hSHP ); //delete m_hSHP; 
		free( pszBasename ); //delete[] pszBasename; 
		free( pszFullname ); //delete[] pszFullname; 
		return( NULL );
	}
	
	free( pszFullname ); //delete[] pszFullname;
	free( pszBasename ); //delete[] pszBasename;
	
	/* -------------------------------------------------------------------- */
	/*  Read the file size from the SHP file.				*/
	/* -------------------------------------------------------------------- */
	pabyBuf = (unsigned char *) malloc(100); //new unsigned char[100];
	fread( pabyBuf, 100, 1, m_hSHP->fpSHP );
	
	m_hSHP->nFileSize = (pabyBuf[24] * 256 * 256 * 256
		+ pabyBuf[25] * 256 * 256
		+ pabyBuf[26] * 256
		+ pabyBuf[27]) * 2;
	
	/* -------------------------------------------------------------------- */
	/*  Read SHX file Header info                                           */
	/* -------------------------------------------------------------------- */
	fread( pabyBuf, 100, 1, m_hSHP->fpSHX );
	
	if( pabyBuf[0] != 0 
		|| pabyBuf[1] != 0 
		|| pabyBuf[2] != 0x27 
		|| (pabyBuf[3] != 0x0a && pabyBuf[3] != 0x0d) )
	{
		fclose( m_hSHP->fpSHP );
		fclose( m_hSHP->fpSHX );
		free( m_hSHP ); //delete m_hSHP; 
		
		return( NULL );
	}

	m_hSHP->nRecords =	pabyBuf[27] + 
						pabyBuf[26] * 256 + 
						pabyBuf[25] * 256 * 256 + 
						pabyBuf[24] * 256 * 256 * 256;

	m_hSHP->nRecords = (m_hSHP->nRecords*2 - 100) / 8;
	
	m_hSHP->nShapeType = pabyBuf[32];
	
	if( m_hSHP->nRecords < 0 || m_hSHP->nRecords > 256000000 )
	{
		/* this header appears to be corrupt.  Give up. */
		fclose( m_hSHP->fpSHP );
		fclose( m_hSHP->fpSHX );
		free( m_hSHP ); //delete m_hSHP;
		
		return( NULL );
	}
	
	/* -------------------------------------------------------------------- */
	/*      Read the bounds.                                                */
	/* -------------------------------------------------------------------- */
	if( bBigEndian ) SwapWord( 8, pabyBuf+36 );
	memcpy( &dValue, pabyBuf+36, 8 );
	m_hSHP->adBoundsMin[0] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+44 );
	memcpy( &dValue, pabyBuf+44, 8 );
	m_hSHP->adBoundsMin[1] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+52 );
	memcpy( &dValue, pabyBuf+52, 8 );
	m_hSHP->adBoundsMax[0] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+60 );
	memcpy( &dValue, pabyBuf+60, 8 );
	m_hSHP->adBoundsMax[1] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+68 );		/* z */
	memcpy( &dValue, pabyBuf+68, 8 );
	m_hSHP->adBoundsMin[2] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+76 );
	memcpy( &dValue, pabyBuf+76, 8 );
	m_hSHP->adBoundsMax[2] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+84 );		/* z */
	memcpy( &dValue, pabyBuf+84, 8 );
	m_hSHP->adBoundsMin[3] = dValue;
	
	if( bBigEndian ) SwapWord( 8, pabyBuf+92 );
	memcpy( &dValue, pabyBuf+92, 8 );
	m_hSHP->adBoundsMax[3] = dValue;
	
	free( pabyBuf ); //delete[] pabyBuf; 
	
	/* -------------------------------------------------------------------- */
	/*	Read the .shx file to get the offsets to each record in 	*/
	/*	the .shp file.							*/
	/* -------------------------------------------------------------------- */
	m_hSHP->nMaxRecords = m_hSHP->nRecords;
	
	//m_hSHP->panRecOffset = new int[MAX(1, m_hSHP->nMaxRecords)];
	m_hSHP->panRecOffset = (int *) malloc(sizeof(int) * MAX(1,m_hSHP->nMaxRecords) );
	//m_hSHP->panRecSize = new int[MAX(1,m_hSHP->nMaxRecords)];
	m_hSHP->panRecSize = (int *) malloc(sizeof(int) * MAX(1,m_hSHP->nMaxRecords) );
	
	pabyBuf = (unsigned char *) malloc(8 * MAX(1,m_hSHP->nRecords) ); //new unsigned char[8*MAX(1,m_hSHP->nRecords)];
	fread( pabyBuf, 8, m_hSHP->nRecords, m_hSHP->fpSHX );
	
	for( i = 0; i < m_hSHP->nRecords; i++ )
	{
		INT32		nOffset, nLength;
		
		memcpy( &nOffset, pabyBuf + i * 8, 4 );
		if( !bBigEndian ) SwapWord( 4, &nOffset );
		
		memcpy( &nLength, pabyBuf + i * 8 + 4, 4 );
		if( !bBigEndian ) SwapWord( 4, &nLength );
		
		m_hSHP->panRecOffset[i] = nOffset*2;
		m_hSHP->panRecSize[i] = nLength*2;
	}
	free( pabyBuf ); //delete[] pabyBuf;

	return( m_hSHP );
}

/************************************************************************/
/*                           SHPDestroyTree()                           */
/************************************************************************/

void CESRIShapeFile::SHPDestroyTree( SHPTree * psTree )
{
    SHPDestroyTreeNode( psTree->psRoot );
    free( psTree ); //delete psTree; 
	psTree = NULL;
}

/************************************************************************/
/*                             SHPGetInfo()                             */
/*                                                                      */
/*      Fetch general information about the shape file.                 */
/************************************************************************/

void CESRIShapeFile::SHPGetInfo( int * pnEntities, int * pnShapeType,
						  double * padfMinBound, double * padfMaxBound )
{
    int		i;
    
    if( pnEntities != NULL )
        *pnEntities = m_hSHP->nRecords;
	
    if( pnShapeType != NULL )
        *pnShapeType = m_hSHP->nShapeType;
	
    for( i = 0; i < 4; i++ )
    {
        if( padfMinBound != NULL )
            padfMinBound[i] = m_hSHP->adBoundsMin[i];
        if( padfMaxBound != NULL )
            padfMaxBound[i] = m_hSHP->adBoundsMax[i];
    }
}

/************************************************************************/
/*                       SHPTreeTrimExtraNodes()                        */
/*                                                                      */
/*      Trim empty nodes from the tree.  Note that we never trim an     */
/*      empty root node.                                                */
/************************************************************************/
void CESRIShapeFile::SHPTreeTrimExtraNodes( SHPTree * hTree )

{
    SHPTreeNodeTrim( hTree->psRoot );
}


/************************************************************************/
/*                          SHPTreeNodeTrim()                           */
/*                                                                      */
/*      This is the recurve version of SHPTreeTrimExtraNodes() that     */
/*      walks the tree cleaning it up.                                  */
/************************************************************************/

int CESRIShapeFile::SHPTreeNodeTrim( SHPTreeNode * psTreeNode )
{
    int		i;
	
	/* -------------------------------------------------------------------- */
	/*      Trim subtrees, and free subnodes that come back empty.          */
	/* -------------------------------------------------------------------- */
    for( i = 0; i < psTreeNode->nSubNodes; i++ )
    {
        if( SHPTreeNodeTrim( psTreeNode->apsSubNode[i] ) )
        {
            SHPDestroyTreeNode( psTreeNode->apsSubNode[i] );
			
            psTreeNode->apsSubNode[i] =
                psTreeNode->apsSubNode[psTreeNode->nSubNodes-1];
			
            psTreeNode->nSubNodes--;
			
            i--; /* process the new occupant of this subnode entry */
        }
    }
	
	/* -------------------------------------------------------------------- */
	/*      We should be trimmed if we have no subnodes, and no shapes.     */
	/* -------------------------------------------------------------------- */
    return( psTreeNode->nSubNodes == 0 && psTreeNode->nShapeCount == 0 );
}

/************************************************************************/
/*                           SHPCreateTree()                            */
/************************************************************************/

CESRIShapeFile::SHPTree * CESRIShapeFile::SHPCreateTree( int nDimension, int nMaxDepth,
			  double *padfBoundsMin, double *padfBoundsMax )
{
    SHPTree	*psTree;
	
    if( padfBoundsMin == NULL && m_hSHP == NULL )
        return NULL;
	
	/* -------------------------------------------------------------------- */
	/*      Allocate the tree object                                        */
	/* -------------------------------------------------------------------- */
    psTree = (SHPTree *) malloc(sizeof(SHPTree)); //new SHPTree; 
	
    psTree->hSHP = m_hSHP;
    psTree->nMaxDepth = nMaxDepth;
    psTree->nDimension = nDimension;
	
	/* -------------------------------------------------------------------- */
	/*      If no max depth was defined, try to select a reasonable one     */
	/*      that implies approximately 8 shapes per node.                   */
	/* -------------------------------------------------------------------- */
    if( psTree->nMaxDepth == 0 && m_hSHP != NULL )
    {
        int	nMaxNodeCount = 1;
        int	nShapeCount;
		
        SHPGetInfo( &nShapeCount, NULL, NULL, NULL );
        while( nMaxNodeCount*4 < nShapeCount )
        {
            psTree->nMaxDepth += 1;
            nMaxNodeCount = nMaxNodeCount * 2;
        }
    }
	
	/* -------------------------------------------------------------------- */
	/*      Allocate the root node.                                         */
	/* -------------------------------------------------------------------- */
    psTree->psRoot = SHPTreeNodeCreate( padfBoundsMin, padfBoundsMax );
	
	/* -------------------------------------------------------------------- */
	/*      Assign the bounds to the root node.  If none are passed in,     */
	/*      use the bounds of the provided file otherwise the create        */
	/*      function will have already set the bounds.                      */
	/* -------------------------------------------------------------------- */
    if( padfBoundsMin == NULL )
    {
        SHPGetInfo( NULL, NULL,
			psTree->psRoot->adfBoundsMin, 
			psTree->psRoot->adfBoundsMax );
    }
	
	/* -------------------------------------------------------------------- */
	/*      If we have a file, insert all it's shapes into the tree.        */
	/* -------------------------------------------------------------------- */
    if( m_hSHP != NULL )
    {
        int	iShape, nShapeCount;
        
        SHPGetInfo( &nShapeCount, NULL, NULL, NULL );
		
        for( iShape = 0; iShape < nShapeCount; iShape++ )
        {
            CSHPObject	*psShape;
            
            psShape = SHPReadObject( iShape );
            SHPTreeAddShapeId( psTree, psShape ); // psShape이 있는 QTree sector를 만들고 그안에 psShape를 포함시킨다. comment by hsb 
            delete psShape ;
        }
    }        
	
    return psTree;
}

/************************************************************************/
/*                       SHPCheckBoundsOverlap()                        */
/*                                                                      */
/*      Do the given boxes overlap at all?                              */
/************************************************************************/
int CESRIShapeFile::SHPCheckBoundsOverlap( double * padfBox1Min, double * padfBox1Max,
					  double * padfBox2Min, double * padfBox2Max,
					  int nDimension )					  
{
    int		iDim;
	
    for( iDim = 0; iDim < nDimension; iDim++ )
    {
        if( padfBox2Max[iDim] < padfBox1Min[iDim] )
            return false;
        
        if( padfBox1Max[iDim] < padfBox2Min[iDim] )
            return false;
    }
	
    return true;
}
/************************************************************************/
/*                          SHPWriteHeader()                            */
/*                                                                      */
/*      Write out a header for the .shp and .shx files as well as the	*/
/*	contents of the index (.shx) file.				*/
/************************************************************************/

void CESRIShapeFile::SHPWriteHeader()
{
    unsigned char     	abyHeader[100];
    int		i;
    INT32	i32;
    double	dValue;
    INT32	*panSHX;

/* -------------------------------------------------------------------- */
/*      Prepare header block for .shp file.                             */
/* -------------------------------------------------------------------- */
    for( i = 0; i < 100; i++ )
      abyHeader[i] = 0;

    abyHeader[2] = 0x27;				/* magic cookie */
    abyHeader[3] = 0x0a;

    i32 = m_hSHP->nFileSize/2;				/* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
    
    i32 = 1000;						/* version */
    ByteCopy( &i32, abyHeader+28, 4 );
    if( bBigEndian ) SwapWord( 4, abyHeader+28 );
    
    i32 = m_hSHP->nShapeType;				/* shape type */
    ByteCopy( &i32, abyHeader+32, 4 );
    if( bBigEndian ) SwapWord( 4, abyHeader+32 );

    dValue = m_hSHP->adBoundsMin[0];			/* set bounds */
    ByteCopy( &dValue, abyHeader+36, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+36 );

    dValue = m_hSHP->adBoundsMin[1];
    ByteCopy( &dValue, abyHeader+44, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+44 );

    dValue = m_hSHP->adBoundsMax[0];
    ByteCopy( &dValue, abyHeader+52, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+52 );

    dValue = m_hSHP->adBoundsMax[1];
    ByteCopy( &dValue, abyHeader+60, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+60 );

    dValue = m_hSHP->adBoundsMin[2];			/* z */
    ByteCopy( &dValue, abyHeader+68, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+68 );

    dValue = m_hSHP->adBoundsMax[2];
    ByteCopy( &dValue, abyHeader+76, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+76 );

    dValue = m_hSHP->adBoundsMin[3];			/* m */
    ByteCopy( &dValue, abyHeader+84, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+84 );

    dValue = m_hSHP->adBoundsMax[3];
    ByteCopy( &dValue, abyHeader+92, 8 );
    if( bBigEndian ) SwapWord( 8, abyHeader+92 );

/* -------------------------------------------------------------------- */
/*      Write .shp file header.                                         */
/* -------------------------------------------------------------------- */
    fseek( m_hSHP->fpSHP, 0, 0 );
    fwrite( abyHeader, 100, 1, m_hSHP->fpSHP );

/* -------------------------------------------------------------------- */
/*      Prepare, and write .shx file header.                            */
/* -------------------------------------------------------------------- */
    i32 = (m_hSHP->nRecords * 2 * sizeof(INT32) + 100)/2;   /* file size */
    ByteCopy( &i32, abyHeader+24, 4 );
    if( !bBigEndian ) SwapWord( 4, abyHeader+24 );
    
    fseek( m_hSHP->fpSHX, 0, 0 );
    fwrite( abyHeader, 100, 1, m_hSHP->fpSHX );

/* -------------------------------------------------------------------- */
/*      Write out the .shx contents.                                    */
/* -------------------------------------------------------------------- */
    //panSHX = new INT32[ 2 * m_hSHP->nRecords ]; 
	panSHX = (INT32 *) malloc(sizeof(INT32) * 2 * m_hSHP->nRecords);

    for( i = 0; i < m_hSHP->nRecords; i++ )
    {
	panSHX[i*2  ] = m_hSHP->panRecOffset[i]/2;
	panSHX[i*2+1] = m_hSHP->panRecSize[i]/2;
	if( !bBigEndian ) SwapWord( 4, panSHX+i*2 );
	if( !bBigEndian ) SwapWord( 4, panSHX+i*2+1 );
    }

    fwrite( panSHX, sizeof(INT32) * 2, m_hSHP->nRecords, m_hSHP->fpSHX );

	//delete[] panSHX;
    free( panSHX );
}

/************************************************************************/
/*                         SHPDestroyTreeNode()                         */
/************************************************************************/
void CESRIShapeFile::SHPDestroyTreeNode( SHPTreeNode * psTreeNode )
{
    int		i;
    
    for( i = 0; i < psTreeNode->nSubNodes; i++ )
    {
        if( psTreeNode->apsSubNode[i] != NULL )
            SHPDestroyTreeNode( psTreeNode->apsSubNode[i] );
    }
    
    if( psTreeNode->panShapeIds != NULL )
		//delete[] psTreeNode->panShapeIds;
        free( psTreeNode->panShapeIds );

    if( psTreeNode->papsShapeObj != NULL )
    {
        for( i = 0; i < psTreeNode->nShapeCount; i++ )
        {
            if( psTreeNode->papsShapeObj[i] != NULL )
                free(psTreeNode->papsShapeObj[i]) ; //delete psTreeNode->papsShapeObj[i] ;
        }
		//delete[] psTreeNode->papsShapeObj;
        free( psTreeNode->papsShapeObj );
    }

//	delete psTreeNode;
    free( psTreeNode );
}

/************************************************************************/
/*                          SHPTreeNodeInit()                           */
/*                                                                      */
/*      Initialize a tree node.                                         */
/************************************************************************/
CESRIShapeFile::SHPTreeNode *CESRIShapeFile::SHPTreeNodeCreate( double * padfBoundsMin,
                                       double * padfBoundsMax )
{
    SHPTreeNode	*psTreeNode;

    psTreeNode = (SHPTreeNode *) malloc(sizeof(SHPTreeNode)); //new SHPTreeNode; 

    psTreeNode->nShapeCount = 0;
    psTreeNode->panShapeIds = NULL;
    psTreeNode->papsShapeObj = NULL;

    psTreeNode->nSubNodes = 0;

    if( padfBoundsMin != NULL )
        memcpy( psTreeNode->adfBoundsMin, padfBoundsMin, sizeof(double) * 4 );

    if( padfBoundsMax != NULL )
        memcpy( psTreeNode->adfBoundsMax, padfBoundsMax, sizeof(double) * 4 );

    return psTreeNode;
}

/************************************************************************/
/*                         SHPTreeAddShapeId()                          */
/*                                                                      */
/*      Add a shape to the tree, but don't keep a pointer to the        */
/*      object data, just keep the shapeid.                             */
/************************************************************************/
int CESRIShapeFile::SHPTreeAddShapeId( SHPTree * psTree, CSHPObject * psObject )
{
    return( SHPTreeNodeAddShapeId( psTree->psRoot, psObject,
                                   psTree->nMaxDepth, psTree->nDimension ) );
}

/************************************************************************/
/*                       SHPTreeNodeAddShapeId()                        */
/************************************************************************/

int CESRIShapeFile::SHPTreeNodeAddShapeId( SHPTreeNode * psTreeNode, CSHPObject * psObject,
                       int nMaxDepth, int nDimension )
{
    int		i;
    
/* -------------------------------------------------------------------- */
/*      If there are subnodes, then consider wiether this object        */
/*      will fit in them.                                               */
/* -------------------------------------------------------------------- */
    if( nMaxDepth > 1 && psTreeNode->nSubNodes > 0 )
    {
        for( i = 0; i < psTreeNode->nSubNodes; i++ )
        {
            if( SHPCheckObjectContained(psObject, nDimension,
                                      psTreeNode->apsSubNode[i]->adfBoundsMin,
                                      psTreeNode->apsSubNode[i]->adfBoundsMax))
            {
                return SHPTreeNodeAddShapeId( psTreeNode->apsSubNode[i],
                                              psObject, nMaxDepth-1,
                                              nDimension );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Otherwise, consider creating four subnodes if could fit into    */
/*      them, and adding to the appropriate subnode.                    */
/* -------------------------------------------------------------------- */
#if MAX_SUBNODE == 4
    else if( nMaxDepth > 1 && psTreeNode->nSubNodes == 0 )
    {
        double	adfBoundsMinH1[4], adfBoundsMaxH1[4];
        double	adfBoundsMinH2[4], adfBoundsMaxH2[4];
        double	adfBoundsMin1[4], adfBoundsMax1[4];
        double	adfBoundsMin2[4], adfBoundsMax2[4];
        double	adfBoundsMin3[4], adfBoundsMax3[4];
        double	adfBoundsMin4[4], adfBoundsMax4[4];

        SHPTreeSplitBounds( psTreeNode->adfBoundsMin,
                            psTreeNode->adfBoundsMax,
                            adfBoundsMinH1, adfBoundsMaxH1,
                            adfBoundsMinH2, adfBoundsMaxH2 );

        SHPTreeSplitBounds( adfBoundsMinH1, adfBoundsMaxH1,
                            adfBoundsMin1, adfBoundsMax1,
                            adfBoundsMin2, adfBoundsMax2 );

        SHPTreeSplitBounds( adfBoundsMinH2, adfBoundsMaxH2,
                            adfBoundsMin3, adfBoundsMax3,
                            adfBoundsMin4, adfBoundsMax4 );

        if( SHPCheckObjectContained(psObject, nDimension,
                                    adfBoundsMin1, adfBoundsMax1)
            || SHPCheckObjectContained(psObject, nDimension,
                                    adfBoundsMin2, adfBoundsMax2)
            || SHPCheckObjectContained(psObject, nDimension,
                                    adfBoundsMin3, adfBoundsMax3)
            || SHPCheckObjectContained(psObject, nDimension,
                                    adfBoundsMin4, adfBoundsMax4) )
        {
            psTreeNode->nSubNodes = 4;
            psTreeNode->apsSubNode[0] = SHPTreeNodeCreate( adfBoundsMin1,
                                                           adfBoundsMax1 );
            psTreeNode->apsSubNode[1] = SHPTreeNodeCreate( adfBoundsMin2,
                                                           adfBoundsMax2 );
            psTreeNode->apsSubNode[2] = SHPTreeNodeCreate( adfBoundsMin3,
                                                           adfBoundsMax3 );
            psTreeNode->apsSubNode[3] = SHPTreeNodeCreate( adfBoundsMin4,
                                                           adfBoundsMax4 );

            /* recurse back on this node now that it has subnodes */
            return( SHPTreeNodeAddShapeId( psTreeNode, psObject,
                                           nMaxDepth, nDimension ) );
        }
    }
#endif /* MAX_SUBNODE == 4 */

/* -------------------------------------------------------------------- */
/*      Otherwise, consider creating two subnodes if could fit into     */
/*      them, and adding to the appropriate subnode.                    */
/* -------------------------------------------------------------------- */
#if MAX_SUBNODE == 2
    else if( nMaxDepth > 1 && psTreeNode->nSubNodes == 0 )
    {
        double	adfBoundsMin1[4], adfBoundsMax1[4];
        double	adfBoundsMin2[4], adfBoundsMax2[4];

        SHPTreeSplitBounds( psTreeNode->adfBoundsMin, psTreeNode->adfBoundsMax,
                            adfBoundsMin1, adfBoundsMax1,
                            adfBoundsMin2, adfBoundsMax2 );

        if( SHPCheckObjectContained(psObject, nDimension,
                                 adfBoundsMin1, adfBoundsMax1))
        {
            psTreeNode->nSubNodes = 2;
            psTreeNode->apsSubNode[0] = SHPTreeNodeCreate( adfBoundsMin1,
                                                           adfBoundsMax1 );
            psTreeNode->apsSubNode[1] = SHPTreeNodeCreate( adfBoundsMin2,
                                                           adfBoundsMax2 );

            return( SHPTreeNodeAddShapeId( psTreeNode->apsSubNode[0], psObject,
                                           nMaxDepth - 1, nDimension ) );
        }
        else if( SHPCheckObjectContained(psObject, nDimension,
                                         adfBoundsMin2, adfBoundsMax2) )
        {
            psTreeNode->nSubNodes = 2;
            psTreeNode->apsSubNode[0] = SHPTreeNodeCreate( adfBoundsMin1,
                                                           adfBoundsMax1 );
            psTreeNode->apsSubNode[1] = SHPTreeNodeCreate( adfBoundsMin2,
                                                           adfBoundsMax2 );

            return( SHPTreeNodeAddShapeId( psTreeNode->apsSubNode[1], psObject,
                                           nMaxDepth - 1, nDimension ) );
        }
    }
#endif /* MAX_SUBNODE == 2 */

/* -------------------------------------------------------------------- */
/*      If none of that worked, just add it to this nodes list.         */
/* -------------------------------------------------------------------- */
    psTreeNode->nShapeCount++;

    psTreeNode->panShapeIds = (int*)SfRealloc( psTreeNode->panShapeIds,
		sizeof(int) * psTreeNode->nShapeCount );
    psTreeNode->panShapeIds[psTreeNode->nShapeCount-1] = psObject->nShapeId;

    if( psTreeNode->papsShapeObj != NULL )
    {
        psTreeNode->papsShapeObj = (CSHPObject**)SfRealloc( psTreeNode->papsShapeObj,
			sizeof(void *) * psTreeNode->nShapeCount );
        psTreeNode->papsShapeObj[psTreeNode->nShapeCount-1] = NULL;
    }

    return true;
}

/************************************************************************/
/*                      SHPCheckObjectContained()                       */
/*                                                                      */
/*      Does the given shape fit within the indicated extents?          */
/************************************************************************/
int CESRIShapeFile::SHPCheckObjectContained( CSHPObject * psObject, int nDimension,
                           double * padfBoundsMin, double * padfBoundsMax )
{
    if( psObject->fxMin < padfBoundsMin[0]
        || psObject->fxMax > padfBoundsMax[0] )
        return false;
    
    if( psObject->fyMin < padfBoundsMin[1]
        || psObject->fyMax > padfBoundsMax[1] )
        return false;

    if( nDimension == 2 )
        return true;
    
    if( psObject->fzMin < padfBoundsMin[2]
        || psObject->fzMax < padfBoundsMax[2] )
        return false;
        
    if( nDimension == 3 )
        return true;

    if( psObject->fmidMin < padfBoundsMin[3]
        || psObject->fmidMax < padfBoundsMax[3] )
        return false;

    return true;
}

/************************************************************************/
/*                         SHPTreeSplitBounds()                         */
/*                                                                      */
/*      Split a region into two subregion evenly, cutting along the     */
/*      longest dimension.                                              */
/************************************************************************/
void CESRIShapeFile::SHPTreeSplitBounds( double *padfBoundsMinIn, double *padfBoundsMaxIn,
                    double *padfBoundsMin1, double * padfBoundsMax1,
                    double *padfBoundsMin2, double * padfBoundsMax2 )
{
/* -------------------------------------------------------------------- */
/*      The output bounds will be very similar to the input bounds,     */
/*      so just copy over to start.                                     */
/* -------------------------------------------------------------------- */
    memcpy( padfBoundsMin1, padfBoundsMinIn, sizeof(double) * 4 );
    memcpy( padfBoundsMax1, padfBoundsMaxIn, sizeof(double) * 4 );
    memcpy( padfBoundsMin2, padfBoundsMinIn, sizeof(double) * 4 );
    memcpy( padfBoundsMax2, padfBoundsMaxIn, sizeof(double) * 4 );
    
/* -------------------------------------------------------------------- */
/*      Split in X direction.                                           */
/* -------------------------------------------------------------------- */
    if( (padfBoundsMaxIn[0] - padfBoundsMinIn[0])
        			> (padfBoundsMaxIn[1] - padfBoundsMinIn[1]) ) // cx > cy
    {
        double	dfRange = padfBoundsMaxIn[0] - padfBoundsMinIn[0];

        padfBoundsMax1[0] = padfBoundsMinIn[0] + dfRange * SHP_SPLIT_RATIO;
        padfBoundsMin2[0] = padfBoundsMaxIn[0] - dfRange * SHP_SPLIT_RATIO;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise split in Y direction.                                 */
/* -------------------------------------------------------------------- */
    else
    {
        double	dfRange = padfBoundsMaxIn[1] - padfBoundsMinIn[1];

        padfBoundsMax1[1] = padfBoundsMinIn[1] + dfRange * SHP_SPLIT_RATIO;
        padfBoundsMin2[1] = padfBoundsMaxIn[1] - dfRange * SHP_SPLIT_RATIO;
    }
}

// 경위도를 입력받아 NaviIndex를 리턴한다
int CESRIShapeFile::GetNaviIndex()
{
	double lonx = GetBoundary().CenterPoint().x;
	double laty = GetBoundary().CenterPoint().y;

    // lonx = 127.06, laty = 37.54
    int i1, i2, i3;
    double dx,dy;

    double xm = lonx * 60;  // 7623.60
    double ym = laty * 60;  // 2252.4

    i1 = (int)floor(ym / 40);    // 56
    i2 = (int)floor(xm / 60);    // 127

    dy = ym - (i1 * 40);    // 12.4...f
    dx = xm - (i2 * 60);    // 3.6...f

    // 2차MESH Index
    i3 = (int)(floor(dy / 5) * 10 + floor(dx / 7.5f)); // 20
    return (i1 * 10000) + ((i2 - 100) * 100) + i3;    
}

int CESRIShapeFile::GetNaviIndexFromFile(const string& sFileName)
{
	//sFileName 은 .shp이다.
	CESRIShapeFile tmpObject;
	tmpObject.Open(sFileName, false);
	return tmpObject.GetNaviIndex();
}

void CESRIShapeFile::operator+=(CESRIShapeFile& o)
{
	if(this == &o) return;

	int i, n;
	int nRecordCount = DBFGetRecordCount();
	CSHPObject* pShp;
	for(i=0;i<o.GetEntityCount();i++)
	{
		//Shape복사
		pShp = o.SHPReadObject(i);
		SHPWriteObject(-1, pShp);//-1 for Append
		delete pShp;
		
		//DBF 복사
		for(n=0;n<DBFGetFieldCount();n++)
		{
			//필드명 읽어서 인덱스 얻기
			char szFldName[20];
			int nW, nDecimal;
			DBFFieldType FldType = o.DBFGetFieldInfo(n, szFldName, &nW, &nDecimal);
			
			int nFldidx = DBFGetFieldIndex(szFldName);
			if(nFldidx<0) continue;

			DBFWriteAttribute (nRecordCount + i, nFldidx, o.DBFReadAttribute(i, n, FldType));
		}
	}
}

void CESRIShapeFile::Close()
{
	SHPClose();
	DBFClose();
}


void CESRIShapeFile::SortByColumn(int nField, int* anSortedLink)
{
	string* asValueList = new string[GetEntityCount()];
	int i;
	for(i=0;i<GetEntityCount();i++)
	{
		asValueList[i] = DBFReadStringAttribute(i, nField);
		anSortedLink[i] = i;
	}

	QuickSort2(asValueList, anSortedLink, 0, GetEntityCount()-1);
	delete[] asValueList;
}

inline bool CESRIShapeFile::FileExists(const char* sFileName)		/// 파일 존재여부 확인
{
	std::ifstream file(sFileName);
	if (!file)
	{
		return false;
	}
	return true;

}

bool CESRIShapeFile::ReverseVtxOrder(CSHPObject *pShp)
{
//	int part_idx = 0;
	if( pShp->nParts > 1 ) assert( false );
	int swap_cnt = pShp->vertex_cnt/2;
	double swap =0;
	for( int loop1 = 0, loop2 = pShp->vertex_cnt-1 ; loop1 < swap_cnt ; loop1 ++ , loop2 -- )
	{
		swap = pShp->pfX[loop1] ;
		pShp->pfX[loop1] = pShp->pfX[loop2];
		pShp->pfX[loop2] = swap;

		swap = pShp->pfY[loop1] ;
		pShp->pfY[loop1] = pShp->pfY[loop2];
		pShp->pfY[loop2] = swap;
	}
	return true;
}

bool CESRIShapeFile::GetOuterRingFlag_ShapePolyPart( CSHPObject *pShp , int * pPolyType, int iOuterBufLen  )
{
    int loop1 = 0  ;

    bool	bRet2 = false  ;
	int		part_vtx_cnt = 0;


	if ( pShp->nSHPType != SHPT_POLYGON ) 
	{
		return false ;
	}


	if( pShp->nParts > 1 )
	{
		for( loop1 = 0 ; loop1 < pShp->nParts && loop1 < iOuterBufLen; loop1 ++ )
		{
			if( loop1 == pShp->nParts - 1)
			{
				part_vtx_cnt =  pShp->vertex_cnt - pShp->panPartStart[loop1];
			}
			else
			{
				part_vtx_cnt =  pShp->panPartStart[loop1+1] - pShp->panPartStart[loop1];

			}
			bRet2 = IsPolygonClockWise( pShp->pfX, pShp->pfY, pShp->panPartStart[loop1], part_vtx_cnt ) ;

			if( bRet2 )
			{
				pPolyType[loop1] = SHPP_OUTERRING ;
			}
			else
			{
				pPolyType[loop1] = SHPP_INNERRING ;
			}
		}
	}
	else
	{
		part_vtx_cnt = pShp->vertex_cnt ;

		bRet2 = IsPolygonClockWise( pShp->pfX, pShp->pfY, pShp->panPartStart[loop1], part_vtx_cnt ) ;

		if( bRet2)
		{
			pPolyType[loop1] = SHPP_OUTERRING ;
		}
		else
		{
			pPolyType[loop1] = SHPP_INNERRING ;
		}
	}
	return true ;
}
bool	CESRIShapeFile::HoleOuterPG( double *rgX, double *rgY, int ofs, int iVtxCnt )
{
	double sx, sy, dx, dy;
	double m1, m2;
	double slope , check = 0;

	int loop1 = 0 ,cross_cnt = 0, last_vtx_cnt;
	double a, b, c;
	double a1, b1, c1;
	double cx, cy, px, py, rx, ry;
	double d;
	double vx, vy, vx1, vy1;

	const static double mcos60	= cos(-ESRI_MEC_PI/3);
	const static double msin60	= sin(-ESRI_MEC_PI/3);
	double inner_product =  0;

	sx = (rgX[ofs+1] - rgX[ofs])/2 + rgX[ofs];
	sy = (rgY[ofs+1] - rgY[ofs])/2 + rgY[ofs];

	// 직선의 방정식 구하기

	dx = rgX[ofs+1] - sx;
	dy = rgY[ofs+1] - sy;

	slope = - dx/dy;
	//double py	=   sy - (slope*sx); // y절편 

	rx = rgX[ofs+1] - rgX[ofs];
	ry = rgY[ofs+1] - rgY[ofs];

	px	=	mcos60* rx  - msin60*ry + rgX[ofs];
	py	=	msin60* rx  + mcos60*ry + rgY[ofs];

	a	= sy - py;
	b	= px - sx;
	c	= a * sx + b * sy;

	vx	= px - sx;
	vy  = py - sy;


	loop1 = ofs+1;
	if( dy == 0 )
	{
		if( sx > rgX[loop1] ) m1 = 1;
		else m1 = -1; 
	}
	else
	{
		m1 =   (slope * ( rgX[loop1] - sx)) - rgY[loop1] + sy;
	}

	last_vtx_cnt = ofs + iVtxCnt ;

	for(  ; loop1 < last_vtx_cnt - 1 ; loop1 ++  )
	{
		if( dy == 0 )
		{
			if( sx > rgX[loop1+1] ) m2 = 1;
			else m2 = -1;
		}
		else
		{
			m2 = (slope * ( rgX[loop1+1] - sx)) - rgY[loop1+1] + sy;
		}
		check = m1 * m2 ;
		if( check < 0 ) 
		{ // 교차한다.

			// 교점 구하기
			a1	= rgY[loop1] - rgY[loop1+1];
			b1	= rgX[loop1+1] - rgX[loop1];
			c1	= a1* rgX[loop1] + b1*rgY[loop1];

			d = a*b1 - b*a1;
			cx = (b1*c - b*c1 ) / d;
			cy = (- a1*c + a*c1 ) / d;

			vx1 = cx - sx ;
			vy1 = cy - sy ;

			//////////////////////////////////////////////////////////////////////////
			inner_product = vx * vx1 + vy * vy1 ;
			if( inner_product > 0 ) // 같은방향
			{
				cross_cnt ++ ;
			}
			else // 반대방향
			{

			}
		}

		m1 = m2;
	}

	if( cross_cnt % 2 == 1 )
	{
		return  true ;
	}
	return false ;
}


CSHPObject * CESRIShapeFile::SHPCreateSimpleObject( int nSHPType, int nVertices,
					  double * padfX, double * padfY,
					  double * padfZ , GPOINT *pt)

{
	return( SHPCreateObject( nSHPType, -1, 0, NULL, NULL,
		nVertices, padfX, padfY, padfZ, NULL , pt ) ); 
}


/************************************************************************
    Polygon 의 방향이 clockwise 인지 여부 반환
    -	2010.11.05
************************************************************************/
bool CESRIShapeFile::IsPolygonClockWise (double *rgX, double *rgY, int nStartVertex, int nPartVertexCount)
{
	//	볼록 다각형의 경우 외적이 Negative 인 경우 Clockwise (Positive 인 경우 CountClockwise)
	if (IsPolygonConvex(rgX, rgY, nStartVertex, nPartVertexCount) == true)
	{
		double		lCrossProduct;
		GPOINT	sPoint1;
		GPOINT	sPoint2;
		GPOINT	sPoint3;
		for (int nVertexIndex = nStartVertex ; nVertexIndex < nStartVertex + nPartVertexCount ; nVertexIndex++)
		{
			if (nVertexIndex == nStartVertex)
			{
				sPoint1.x	= rgX[nVertexIndex + nPartVertexCount - 1];
				sPoint1.y	= rgY[nVertexIndex + nPartVertexCount - 1];
				sPoint2.x	= rgX[nVertexIndex];
				sPoint2.y	= rgY[nVertexIndex];
				sPoint3.x	= rgX[nVertexIndex + 1];
				sPoint3.y	= rgY[nVertexIndex + 1];
			}
			else if (nVertexIndex == nStartVertex + nPartVertexCount - 1)
			{
				sPoint1.x	= rgX[nVertexIndex - 1];
				sPoint1.y	= rgY[nVertexIndex - 1];
				sPoint2.x	= rgX[nVertexIndex];
				sPoint2.y	= rgY[nVertexIndex];
				sPoint3.x	= rgX[nStartVertex];
				sPoint3.y	= rgY[nStartVertex];
			}
			else
			{
				sPoint1.x	= rgX[nVertexIndex - 1];
				sPoint1.y	= rgY[nVertexIndex - 1];
				sPoint2.x	= rgX[nVertexIndex];
				sPoint2.y	= rgY[nVertexIndex];
				sPoint3.x	= rgX[nVertexIndex + 1];
				sPoint3.y	= rgY[nVertexIndex + 1];
			}

			lCrossProduct	= GetCrossProduct(sPoint1, sPoint2, sPoint3);
			if (lCrossProduct == 0)
			{
				continue;
			}

			return lCrossProduct < 0;
		}
	}
	//	오목 다각형의 경우 면적이 음수인 경우 Clockwise
	else
	{
		double lArea	= 0;

		//////////////////////////////////////////////////////////////////////////
		//	면적, 무게 중심 구하기
		int		nVertexIndex1	= 0;
		int		nVertexIndex2	= 0;
		double	lX1		= 0;
		double	lY1		= 0;
		double	lX2		= 0;
		double	lY2		= 0;

		for (nVertexIndex1 =  nStartVertex ; nVertexIndex1 < nStartVertex + nPartVertexCount ; nVertexIndex1++)
		{
			nVertexIndex2 = (nVertexIndex1 == nStartVertex + nPartVertexCount - 1) ? nStartVertex : nVertexIndex1 + 1;

			lX1 = rgX[nVertexIndex1];
			lY1 = rgY[nVertexIndex1];
			lX2 = rgX[nVertexIndex2];
			lY2 = rgY[nVertexIndex2];

			lArea	+= lX1 * lY2;
			lArea	-= lY1 * lX2;
		}

		return lArea < 0;
	}

	assert(false);
	return true;
}

/*
*	Polygon 이 볼록 다각형인지 여부
*	(외적이 방향성이 전부 동일하면 볼록)
*	-	2010.11.05
*/
bool CESRIShapeFile::IsPolygonConvex ( double *rgX, double *rgY, int nStartVertex, int nPartVertexCount)
{
	bool		bPositive;
	bool		bInit	= false;
	double		lCrossProduct;
	GPOINT	sPoint1;
	GPOINT	sPoint2;
	GPOINT	sPoint3;
	for (int nVertexIndex = nStartVertex ; nVertexIndex < nStartVertex + nPartVertexCount ; nVertexIndex++)
	{
		if (nVertexIndex == nStartVertex)
		{
			sPoint1.x	= rgX[nVertexIndex + nPartVertexCount - 1];
			sPoint1.y	= rgY[nVertexIndex + nPartVertexCount - 1];
			sPoint2.x	= rgX[nVertexIndex];
			sPoint2.y	= rgY[nVertexIndex];
			sPoint3.x	= rgX[nVertexIndex + 1];
			sPoint3.y	= rgY[nVertexIndex + 1];
		}
		else if (nVertexIndex == nStartVertex + nPartVertexCount - 1)
		{
			sPoint1.x	= rgX[nVertexIndex - 1];
			sPoint1.y	= rgY[nVertexIndex - 1];
			sPoint2.x	= rgX[nVertexIndex];
			sPoint2.y	= rgY[nVertexIndex];
			sPoint3.x	= rgX[nStartVertex];
			sPoint3.y	= rgY[nStartVertex];
		}
		else
		{
			sPoint1.x	= rgX[nVertexIndex - 1];
			sPoint1.y	= rgY[nVertexIndex - 1];
			sPoint2.x	= rgX[nVertexIndex];
			sPoint2.y	= rgY[nVertexIndex];
			sPoint3.x	= rgX[nVertexIndex + 1];
			sPoint3.y	= rgY[nVertexIndex + 1];
		}

		lCrossProduct	= GetCrossProduct(sPoint1, sPoint2, sPoint3);
		if (lCrossProduct == 0)
		{
			continue;
		}
		if (bInit == false)
		{
			bPositive	= (lCrossProduct > 0) ? true : false;
			bInit		= true;
		}
		else
		{
			if (bPositive != (lCrossProduct > 0))
			{
				return false;
			}
		}
	}

	return true;
}

/************************************************************************
*	벡터 외적
*	-	2010.11.05
************************************************************************/
double CESRIShapeFile::GetCrossProduct (GPOINT &sPoint1, GPOINT &sPoint2, GPOINT &sPoint3)
{
	return	(sPoint2.x - sPoint1.x) * (sPoint3.y - sPoint2.y) - (sPoint2.y - sPoint1.y) * (sPoint3.x - sPoint2.x);
}

double CESRIShapeFile::GetLength(CSHPObject *pShpObj)
{
	if ( pShpObj->vertex_cnt  < 2)
	{
		return 0;
	}

	double	lLegnth	= 0;
    for (int nIndex = 0 ; nIndex < (int)(pShpObj->vertex_cnt) - 1; nIndex++)
	{
		lLegnth	+= GPOINT::distance(pShpObj->pfX[nIndex], pShpObj->pfY[nIndex], pShpObj->pfX[nIndex + 1], pShpObj->pfY[nIndex + 1]);
	}

	return lLegnth;
}


CSHPObject * CESRIShapeFile::MergeLineShape( CSHPObject * pShpObj, CSHPObject * pShpDelObj, unsigned char cMergeFlag )
{
	int iVtxCnt = pShpDelObj->vertex_cnt + pShpObj->vertex_cnt -1;
	int ori_loop , del_loop ,new_loop = 0 ;
	GPOINT * pMergeLine = new GPOINT[iVtxCnt];

	if(cMergeFlag == 2 || cMergeFlag == 4 ) //  버텍스순서 역방
	{
		ReverseVtxOrder(pShpDelObj);
	}

	switch( cMergeFlag  )  
	{
	case 1 : //// 뒤에붙는다 / 버텍스순서 정방
	case 2 : //// 뒤에붙는다 / 버텍스순서 역방
		for( ori_loop = 0 ; ori_loop < pShpObj->vertex_cnt ; ori_loop ++, new_loop ++ )
		{
			pMergeLine[new_loop].x = pShpObj->pfX[ori_loop];
			pMergeLine[new_loop].y = pShpObj->pfY[ori_loop];
		}
		for( del_loop = 1 ; del_loop < pShpDelObj->vertex_cnt ; del_loop ++ , new_loop ++)
		{
			pMergeLine[new_loop].x = pShpDelObj->pfX[del_loop];
			pMergeLine[new_loop].y = pShpDelObj->pfY[del_loop];
		}

		break;
	case 3 : //// 앞에붙는다 / 버텍스순서 정방
	case 4 : //// 앞에붙는다 / 버텍스순서 역방
		for( del_loop = 0 ; del_loop < pShpDelObj->vertex_cnt ; del_loop ++ , new_loop ++)
		{
			pMergeLine[new_loop].x = pShpDelObj->pfX[del_loop];
			pMergeLine[new_loop].y = pShpDelObj->pfY[del_loop];
		}
		for( ori_loop = 1 ; ori_loop < pShpObj->vertex_cnt ; ori_loop ++, new_loop ++ )
		{
			pMergeLine[new_loop].x = pShpObj->pfX[ori_loop];
			pMergeLine[new_loop].y = pShpObj->pfY[ori_loop];
		}
		break;
	}

	CSHPObject * pMergeLinkShp = SHPCreateSimpleObject(SHPT_LINE, iVtxCnt, 0,0,0, pMergeLine );

	if( pMergeLine )
	{
		delete [] pMergeLine ;
		pMergeLine = 0 ;
	}
	

	return pMergeLinkShp;

}


int			CESRIShapeFile::GetPtOnLine(CSHPObject *pLinkShp, STPtOnLine &kPtOnLine , double dfLimitDist)
{
	GVECTOR vVtxToPtOver, vVtxToVtx;
	GVECTOR vVtxToPtOver2, vVtxToVtx2;
	double dfLinkDist = 0;
	double	dfClosestDist = 0 ;
	double dfMinDist = 1000000;
	GPOINT ptClosestOnLink;
	int ofsClosest = -1;
	double dfClsVtxMatchLen = 0;
	double orthogonal_proj_dist = 0, opd_x = 0, opd_y = 0;
	double vtx_match_length = 0;
	GPOINT ptOnLine;
	unsigned char  cClosestMatchDir = 99;
	double dfVtxDist = 0;
	double dfInnerProduct = 0;
	double 		dfPtToLinkDist =  0,dfPtToOnLinkDist = 0;
	int vtx_ofs = 0;

	GPOINT ptOverLine = kPtOnLine.ptOverLine;
	for( vtx_ofs = 0 ; vtx_ofs < pLinkShp->vertex_cnt -1 ; vtx_ofs ++ )
	{
		// 정점과 라인위의버텍스 거리구한다.
		vVtxToPtOver.SetVector( ptOverLine.x - pLinkShp->pfX[vtx_ofs], ptOverLine.y - pLinkShp->pfY[vtx_ofs] );
		vVtxToVtx.SetVector( pLinkShp->pfX[vtx_ofs+1] - pLinkShp->pfX[vtx_ofs], pLinkShp->pfY[vtx_ofs+1] - pLinkShp->pfY[vtx_ofs] );
		dfLinkDist = vVtxToPtOver.length();

		if( dfLinkDist < dfLimitDist /*dfClosestDist*/ )
		{
			ptOnLine.x			= pLinkShp->pfX[vtx_ofs];
			ptOnLine.y			= pLinkShp->pfY[vtx_ofs];

			if( dfMinDist > dfLinkDist )
			{
				dfMinDist = dfLinkDist;
				dfClosestDist = dfLinkDist;
				ptClosestOnLink = ptOnLine;
				ofsClosest = vtx_ofs ;
				dfClsVtxMatchLen = vtx_match_length;
				cClosestMatchDir =( vVtxToVtx.OuterProduct(vVtxToPtOver) <= 0 ) ? 1:0;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// 라인위의 버텍스 역방향 벡터를 구한다. 위와 동일하다.
		vVtxToVtx2.SetVector(pLinkShp->pfX[vtx_ofs] -pLinkShp->pfX[vtx_ofs+1], pLinkShp->pfY[vtx_ofs] -pLinkShp->pfY[vtx_ofs+1]);
		dfVtxDist = vVtxToVtx2.length();

		// 버텍스시점과 정점의 벡터를 구한다.		
		vVtxToPtOver2.SetVector( ptOverLine.x -pLinkShp->pfX[vtx_ofs+1],ptOverLine.y -pLinkShp->pfY[vtx_ofs+1]);

		// 두벡터의 내적을 구한다.
		dfInnerProduct = vVtxToVtx2.InnerProduct(vVtxToPtOver2) ;
		if ( dfInnerProduct < 0 )
		{
			vtx_match_length += dfVtxDist;
			continue ;// 라인위의 점이 아니므로 버린다.
		}

		//////////////////////////////////////////////////////////////////////////
		// 라인위의 버텍스 정방향 벡터를 구한다.

		dfInnerProduct = vVtxToVtx.InnerProduct(vVtxToPtOver) ;
		if ( dfInnerProduct < 0 ) 
		{
			vtx_match_length += dfVtxDist;
			continue ; 
		}

		//////////////////////////////////////////////////////////////////////////
		// valid 영역

		// 라인의 길이를 구한다.
		dfLinkDist = vVtxToVtx.length();

		// 라인위에서 점까지의 거리를 구한다.
		//dfPtToLinkDist = abs((vVtxToVtx.vx * vVtxToPtOver.vy - vVtxToVtx.vy * vVtxToPtOver.vx) / dfLinkDist);
		dfPtToLinkDist = fabs(vVtxToVtx.OuterProduct(vVtxToPtOver) / dfLinkDist);
		dfPtToOnLinkDist = fabs(dfInnerProduct / dfLinkDist);

		// 제한거리안에 들어오지 않으면 다음으로 ...
		if( dfPtToLinkDist > dfLimitDist )
		{
			vtx_match_length += dfVtxDist;
			continue ;
		}
		else if( dfPtToLinkDist < dfLimitDist )
		{
			
			// 라인위의 점을 구한다.
			orthogonal_proj_dist = dfInnerProduct/dfLinkDist;		
			opd_x	= orthogonal_proj_dist*vVtxToVtx.vx/dfLinkDist;
			opd_y	= orthogonal_proj_dist*vVtxToVtx.vy/dfLinkDist;	

			ptOnLine.x	=pLinkShp->pfX[vtx_ofs]+opd_x;
			ptOnLine.y	=pLinkShp->pfY[vtx_ofs]+opd_y;

			if( dfMinDist > dfPtToLinkDist )
			{
				//	if( kLink.link_id == moniter_link) 
				//	{
				//		bMoniterTrigger = true;
				//		TRACE("step 03--- %d %.2lf,%.2lf,%lf\n", score, oth_link_len , dfClosestDist ,linelink_inner_product);
				//	}
				dfMinDist = dfPtToLinkDist;
				cClosestMatchDir =( vVtxToVtx.OuterProduct(vVtxToPtOver) <= 0 ) ? 1:0;
				dfClosestDist = dfPtToLinkDist;
				dfClsVtxMatchLen = vtx_match_length + dfPtToOnLinkDist;
				ptClosestOnLink = ptOnLine;
				//if(match_link_len > 500)
				//	TRACE("link=%lld, len=%.3lf\n", kLink.link_id, oth_link_len);
				ofsClosest = vtx_ofs ;

			}
		}
		vtx_match_length += dfVtxDist;

	}// end of loop  링크내 버텍스 
	
	// 정점과 라인위의 마지막버텍스 거리구한다.
	vVtxToPtOver.SetVector( ptOverLine.x - pLinkShp->pfX[vtx_ofs], ptOverLine.y - pLinkShp->pfY[vtx_ofs] );
	dfLinkDist = vVtxToPtOver.length();

	if( dfLinkDist < dfLimitDist /*dfClosestDist*/ )
	{
		ptOnLine.x			= pLinkShp->pfX[vtx_ofs];
		ptOnLine.y			= pLinkShp->pfY[vtx_ofs];

		vVtxToPtOver.SetVector( ptOverLine.x - pLinkShp->pfX[vtx_ofs-1], ptOverLine.y - pLinkShp->pfY[vtx_ofs-1] );
		vVtxToVtx.SetVector( pLinkShp->pfX[vtx_ofs] - pLinkShp->pfX[vtx_ofs-1], pLinkShp->pfY[vtx_ofs] - pLinkShp->pfY[vtx_ofs-1] );

		if( dfMinDist > dfLinkDist )
		{
			dfMinDist = dfLinkDist;
			dfClosestDist = dfLinkDist;
			ptClosestOnLink = ptOnLine;
			ofsClosest = vtx_ofs ;
			dfClsVtxMatchLen = vtx_match_length;
			cClosestMatchDir =( vVtxToVtx.OuterProduct(vVtxToPtOver) <= 0 ) ? 1:0;
		}
	}
	kPtOnLine.ptOnLine = ptClosestOnLink;
	kPtOnLine.dfLen_PtOverLine2PtOnLine =  dfClosestDist ;
	kPtOnLine.dfLenEtc = dfClsVtxMatchLen ;
	kPtOnLine.cDirPtOverLine = cClosestMatchDir;
	{
		GVECTOR kVector = GVECTOR(pLinkShp->pfX[ofsClosest+1] - pLinkShp->pfX[ofsClosest], pLinkShp->pfY[ofsClosest+1]  - pLinkShp->pfY[ofsClosest] );
		kPtOnLine.dfMatchLinkAngle =  kVector.NorthBasedAngle(&kVector);
	}

	if( ofsClosest < 0 ) 
	{
		return -1;
	}
	return 0;
}

int		CESRIShapeFile::LoadShpObj(CArrSHPObject &arShp )
{
	int iCount = GetEntityCount();

	arShp.clear();
	arShp.reserve(iCount);
	CSHPObject *pShpObj = 0;

	for( int row_ofs = 0 ; row_ofs < iCount ; row_ofs++)
	{
		pShpObj = SHPReadObject(row_ofs);

		arShp.at(row_ofs) = pShpObj ;
	}
	return 0;
}



int		CESRIShapeFile::GetCenterPtOfLine( CSHPObject *pShpObj , GPOINT & pt, double *  pLNAngle )
{
	double dfHalfLength = GetLength(pShpObj)/2;
	if ( pShpObj->vertex_cnt  < 2)
	{
		return -1;
	}

	double	dfLegnthSum	= 0, dfLastLength = 0, dfLength = 0;
	GVECTOR vec;
    int nIndex = 0;
    for ( nIndex = 0 ; nIndex < (int)(pShpObj->vertex_cnt) - 1; nIndex++)
	{
		dfLength = GPOINT::distance(pShpObj->pfX[nIndex], pShpObj->pfY[nIndex], pShpObj->pfX[nIndex + 1], pShpObj->pfY[nIndex + 1]);
		dfLegnthSum	+= dfLength;
		if( dfHalfLength < dfLegnthSum )
		{
			double dfLengthDiff = dfHalfLength - dfLastLength ;
			vec.SetVector(pShpObj->pfX[nIndex + 1] - pShpObj->pfX[nIndex], pShpObj->pfY[nIndex + 1] - pShpObj->pfY[nIndex]);
			double xdiff = (vec.vx) * dfLengthDiff/dfLength;
			double ydiff = (vec.vy) * dfLengthDiff/dfLength;

			pt.x = pShpObj->pfX[nIndex] + xdiff ;
			pt.y = pShpObj->pfY[nIndex] + ydiff ;
			
			if( pLNAngle ) 
			{
				*pLNAngle = GVECTOR::NorthBasedAngle(&vec);
			}
			

			break;
		}
		dfLastLength = dfLegnthSum ;
	}

	return nIndex;
}

void CESRIShapeFile::ReleaseShp( CESRIShapeFile **ppSF, CArrSHPObject * pArShp )
{
	if(*ppSF){(*ppSF)->Close(); delete (*ppSF) ; (*ppSF) = 0;}
	if( pArShp )
	{
		int iCnt = pArShp->size();
		for( int loop1 = 0 ; loop1 < iCnt ; loop1 ++ ) delete pArShp->at(loop1);
		pArShp->clear();

	}
	
}
