// GPoint.h: interface for the GPoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TESTEXTRACT_HPP___INCLUDED_)
#define TESTEXTRACT_HPP___INCLUDED_

#pragma once
#include "../third_party/mappers/ShpFiles.hpp"
#include "../third_party/mappers/GPoint.hpp"
#include "../third_party/mappers/ESRIShapeFile.hpp"
#include <math.h>
#include <string.h>

//using namespace osrm::engine;
	
namespace osrm
{
namespace extractor
{
	class   CTestExtract
	{
	public:
		CTestExtract()
		{

		}
		int		CreatePbfToShape()
		{
			return 0;
		}

	};

	class CNodePBF: CESRIShapeFile
	{
	public:
		CNodePBF()
		{

		}
	
		bool  CreateShpFile(char *pFilePath)
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
	};
}
}
#endif // !defined(AFX_GPOINT_H__EE7622A7_5950_4F5D_B24C_4E7D7EDC22A9__INCLUDED_)
