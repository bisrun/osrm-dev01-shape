// GPoint.h: interface for the GPoint class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TRIPTEST_HPP__INCLUDED_)
#define TRIPTEST_HPP__INCLUDED_

#pragma once

#include "engine/guidance/assemble_geometry.hpp"
#include "engine/guidance/route_leg.hpp"
#include "geoshape/ShpFiles.hpp"
#include "geoshape/GPoint.hpp"
#include "geoshape/ESRIShapeFile.hpp"
#include "geoshape/TripTest.hpp"

#include <string.h>
#include <math.h>

//using namespace osrm::engine;
	
namespace osrm
{
namespace engine
{
	class	 CTestTrip
	{
	public:
	
		CTestTrip();

		int		CreateTripGeometry(std::vector<guidance::LegGeometry> & leg_geometries
			, std::vector<guidance::RouteLeg>& legs);
		int		CreateViaInfo(const std::vector<PhantomNode> & phantomnode, const std::vector<std::vector<NodeID>>& trips);
	};
}
}
#endif // !defined(TRIPTEST_HPP__INCLUDED_)
