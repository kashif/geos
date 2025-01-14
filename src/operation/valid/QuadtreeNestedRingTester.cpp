/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2005-2006 Refractions Research Inc.
 * Copyright (C) 2001-2002 Vivid Solutions Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: operation/valid/QuadtreeNestedRingTester.java rev. 1.12 (JTS-1.10)
 *
 **********************************************************************/


#include <geos/operation/valid/QuadtreeNestedRingTester.h>
#include <geos/operation/valid/IsValidOp.h>
#include <geos/algorithm/CGAlgorithms.h>
#include <geos/geom/Envelope.h>
#include <geos/geom/LinearRing.h>
#include <geos/index/quadtree/Quadtree.h> 

#include <vector>
#include <cassert>

using namespace std;
using namespace geos::geomgraph;
using namespace geos::geom;
using namespace geos::algorithm;
using namespace geos::index::quadtree;

namespace geos {
namespace operation { // geos.operation
namespace valid { // geos.operation.valid

QuadtreeNestedRingTester::QuadtreeNestedRingTester(GeometryGraph *newGraph):
	graph(newGraph),
	rings(),
	totalEnv(),
	qt(NULL),
	nestedPt(NULL)
{
}

QuadtreeNestedRingTester::~QuadtreeNestedRingTester()
{
	//delete rings;
	//delete totalEnv;
	delete qt;
}

Coordinate *
QuadtreeNestedRingTester::getNestedPoint()
{
	return nestedPt;
}

void
QuadtreeNestedRingTester::add(const LinearRing *ring)
{
	rings.push_back(ring);
	const Envelope *envi=ring->getEnvelopeInternal();
	totalEnv.expandToInclude(envi);
}

bool
QuadtreeNestedRingTester::isNonNested()
{
	buildQuadtree();
	for(size_t i=0, ni=rings.size(); i<ni; ++i)
	{
		const LinearRing *innerRing=rings[i];
		const CoordinateSequence *innerRingPts=innerRing->getCoordinatesRO();
		const Envelope *envi=innerRing->getEnvelopeInternal();

		vector<void*> results;
		qt->query(envi, results);
		for(size_t j=0, nj=results.size(); j<nj; ++j)
		{
			LinearRing *searchRing=(LinearRing*)results[j];
			const CoordinateSequence *searchRingPts=searchRing->getCoordinatesRO();

			if (innerRing==searchRing) continue;

			const Envelope *e1=innerRing->getEnvelopeInternal();
			const Envelope *e2=searchRing->getEnvelopeInternal();
			if (!e1->intersects(e2)) continue;

			const Coordinate *innerRingPt=IsValidOp::findPtNotNode(innerRingPts,
					searchRing, graph);

			// Unable to find a ring point not a node of the search ring
			assert(innerRingPt!=NULL);

			bool isInside=CGAlgorithms::isPointInRing(*innerRingPt,searchRingPts);
			if (isInside) {
				/*
				 * innerRingPt is const just because the input
				 * CoordinateSequence is const. If the input
				 * Polygon survives lifetime of this object
				 * we are safe.
				 */
				nestedPt=const_cast<Coordinate *>(innerRingPt);
				return false;
			}
		}
	}
	return true;
}

void
QuadtreeNestedRingTester::buildQuadtree()
{
	qt=new Quadtree();
	for(size_t i=0, n=rings.size(); i<n; ++i)
	{
		const LinearRing *ring=rings[i];
		const Envelope *env=ring->getEnvelopeInternal();

		qt->insert(env, (void*)ring);
	}
}

} // namespace geos.operation.valid
} // namespace geos.operation
} // namespace geos

/**********************************************************************
 * $Log$
 * Revision 1.25  2006/06/12 11:29:24  strk
 * unsigned int => size_t
 *
 * Revision 1.24  2006/03/29 11:48:53  strk
 * Removed useless heap allocations in construction, enforced const correctness
 *
 * Revision 1.23  2006/03/22 12:22:50  strk
 * indexQuadtree.h split
 *
 * Revision 1.22  2006/03/20 16:57:44  strk
 * spatialindex.h and opValid.h headers split
 *
 * Revision 1.21  2006/03/17 16:48:55  strk
 * LineIntersector and PointLocator made complete components of RelateComputer
 * (were statics const pointers before). Reduced inclusions from opRelate.h
 * and opValid.h, updated .cpp files to allow build.
 *
 **********************************************************************/

