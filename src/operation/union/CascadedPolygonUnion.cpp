/**********************************************************************
 *
 * GEOS - Geometry Engine Open Source
 * http://geos.refractions.net
 *
 * Copyright (C) 2011 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Public Licence as published
 * by the Free Software Foundation. 
 * See the COPYING file for more information.
 *
 **********************************************************************
 *
 * Last port: operation/union/CascadedPolygonUnion.java r320 (JTS-1.12)
 *
 **********************************************************************/

#include <geos/operation/union/CascadedPolygonUnion.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/MultiPolygon.h>
#include <geos/geom/util/GeometryCombiner.h>
#include <geos/index/strtree/STRtree.h>
// std
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

namespace geos {
namespace operation { // geos.operation
namespace geounion {  // geos.operation.geounion

///////////////////////////////////////////////////////////////////////////////
void GeometryListHolder::deleteItem(geom::Geometry* item)
{
    delete item;
}

///////////////////////////////////////////////////////////////////////////////
geom::Geometry* CascadedPolygonUnion::Union(std::vector<geom::Polygon*>* polys)
{
    CascadedPolygonUnion op (polys);
    return op.Union();
}

geom::Geometry* CascadedPolygonUnion::Union(const geom::MultiPolygon* multipoly)
{
    std::vector<geom::Polygon*> polys;
    
    typedef geom::MultiPolygon::const_iterator iterator;
    iterator end = multipoly->end();
    for (iterator i = multipoly->begin(); i != end; ++i)
        polys.push_back(dynamic_cast<geom::Polygon*>(*i));

    CascadedPolygonUnion op (&polys);
    return op.Union();
}

geom::Geometry* CascadedPolygonUnion::Union()
{
    if (inputPolys->empty())
        return NULL;

    geomFactory = inputPolys->front()->getFactory();

    /**
     * A spatial index to organize the collection
     * into groups of close geometries.
     * This makes unioning more efficient, since vertices are more likely 
     * to be eliminated on each round.
     */
    index::strtree::STRtree index(STRTREE_NODE_CAPACITY);

    typedef std::vector<geom::Polygon*>::iterator iterator_type;
    iterator_type end = inputPolys->end();
    for (iterator_type i = inputPolys->begin(); i != end; ++i) {
        geom::Geometry* g = dynamic_cast<geom::Geometry*>(*i);
        index.insert(g->getEnvelopeInternal(), g);
    }

    std::auto_ptr<index::strtree::ItemsList> itemTree (index.itemsTree());

    return unionTree(itemTree.get());
}

geom::Geometry* CascadedPolygonUnion::unionTree(
    index::strtree::ItemsList* geomTree)
{
    /**
     * Recursively unions all subtrees in the list into single geometries.
     * The result is a list of Geometry's only
     */
    std::auto_ptr<GeometryListHolder> geoms(reduceToGeometries(geomTree));
    return binaryUnion(geoms.get());
}

geom::Geometry* CascadedPolygonUnion::binaryUnion(GeometryListHolder* geoms)
{
    return binaryUnion(geoms, 0, geoms->size());
}

geom::Geometry* CascadedPolygonUnion::binaryUnion(GeometryListHolder* geoms, 
    std::size_t start, std::size_t end)
{
    if (end - start <= 1) {
        return unionSafe(geoms->getGeometry(start), NULL);
    }
    else if (end - start == 2) {
        return unionSafe(geoms->getGeometry(start), geoms->getGeometry(start + 1));
    }
    else {
        // recurse on both halves of the list
        std::size_t mid = (end + start) / 2;
        std::auto_ptr<geom::Geometry> g0 (binaryUnion(geoms, start, mid));
        std::auto_ptr<geom::Geometry> g1 (binaryUnion(geoms, mid, end));
        return unionSafe(g0.get(), g1.get());
    }
}

GeometryListHolder* 
CascadedPolygonUnion::reduceToGeometries(index::strtree::ItemsList* geomTree)
{
    std::auto_ptr<GeometryListHolder> geoms (new GeometryListHolder());

    typedef index::strtree::ItemsList::iterator iterator_type;
    iterator_type end = geomTree->end();
    for (iterator_type i = geomTree->begin(); i != end; ++i) {
        if ((*i).get_type() == index::strtree::ItemsListItem::item_is_list) {
            std::auto_ptr<geom::Geometry> geom (unionTree((*i).get_itemslist()));
            geoms->push_back_owned(geom.get());
            geom.release();
        }
        else if ((*i).get_type() == index::strtree::ItemsListItem::item_is_geometry) {
            geoms->push_back(reinterpret_cast<geom::Geometry*>((*i).get_geometry()));
        }
        else {
            assert(!"should never be reached");
        }
    }

    return geoms.release();
}

geom::Geometry* 
CascadedPolygonUnion::unionSafe(geom::Geometry* g0, geom::Geometry* g1)
{
    if (g0 == NULL && g1 == NULL)
        return NULL;

    if (g0 == NULL)
        return g1->clone();
    if (g1 == NULL)
        return g0->clone();

    return unionOptimized(g0, g1);
}

geom::Geometry* 
CascadedPolygonUnion::unionOptimized(geom::Geometry* g0, geom::Geometry* g1)
{
    geom::Envelope const* g0Env = g0->getEnvelopeInternal();
    geom::Envelope const* g1Env = g1->getEnvelopeInternal();

    if (!g0Env->intersects(g1Env))
        return geom::util::GeometryCombiner::combine(g0, g1);

    if (g0->getNumGeometries() <= 1 && g1->getNumGeometries() <= 1)
        return unionActual(g0, g1);

    geom::Envelope commonEnv; 
    g0Env->intersection(*g1Env, commonEnv);
    return unionUsingEnvelopeIntersection(g0, g1, commonEnv);
}

geom::Geometry* 
CascadedPolygonUnion::unionUsingEnvelopeIntersection(geom::Geometry* g0, 
    geom::Geometry* g1, geom::Envelope const& common)
{
    std::vector<geom::Geometry*> disjointPolys;

    std::auto_ptr<geom::Geometry> g0Int(extractByEnvelope(common, g0, disjointPolys));
    std::auto_ptr<geom::Geometry> g1Int(extractByEnvelope(common, g1, disjointPolys));

    std::auto_ptr<geom::Geometry> u(unionActual(g0Int.get(), g1Int.get()));
    disjointPolys.push_back(u.get());

    return geom::util::GeometryCombiner::combine(disjointPolys);
}

geom::Geometry* 
CascadedPolygonUnion::extractByEnvelope(geom::Envelope const& env, 
    geom::Geometry* geom, std::vector<geom::Geometry*>& disjointGeoms)
{
    std::vector<geom::Geometry*> intersectingGeoms;

    for (std::size_t i = 0; i < geom->getNumGeometries(); i++) { 
        geom::Geometry* elem = const_cast<geom::Geometry*>(geom->getGeometryN(i));
        if (elem->getEnvelopeInternal()->intersects(env))
            intersectingGeoms.push_back(elem);
        else
            disjointGeoms.push_back(elem);
    }

    return geomFactory->buildGeometry(intersectingGeoms);
}

geom::Geometry* 
CascadedPolygonUnion::unionActual(geom::Geometry* g0, geom::Geometry* g1)
{
    return g0->Union(g1);
}

} // namespace geos.operation.union
} // namespace geos.operation
} // namespace geos

