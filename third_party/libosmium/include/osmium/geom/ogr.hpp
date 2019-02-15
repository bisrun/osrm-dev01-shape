#ifndef OSMIUM_GEOM_OGR_HPP
#define OSMIUM_GEOM_OGR_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2018 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

/**
 * @file
 *
 * This file contains code for conversion of OSM geometries into OGR
 * geometries.
 *
 * @attention If you include this file, you'll need to link with `libgdal`.
 */

#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/factory.hpp>

#include <ogr_geometry.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace osmium {

    namespace geom {

        namespace detail {

            class OGRFactoryImpl {

            public:

                using point_type        = std::unique_ptr<OGRPoint>;
                using linestring_type   = std::unique_ptr<OGRLineString>;
                using polygon_type      = std::unique_ptr<OGRPolygon>;
                using multipolygon_type = std::unique_ptr<OGRMultiPolygon>;
                using ring_type         = std::unique_ptr<OGRLinearRing>;

            private:

                linestring_type   m_linestring{nullptr};
                multipolygon_type m_multipolygon{nullptr};
                polygon_type      m_polygon{nullptr};
                ring_type         m_ring{nullptr};

            public:

                explicit OGRFactoryImpl(int /* srid */) {
                }

                /* Point */

                point_type make_point(const osmium::geom::Coordinates& xy) const {
                    return point_type{new OGRPoint{xy.x, xy.y}};
                }

                /* LineString */

                void linestring_start() {
                    m_linestring.reset(new OGRLineString{});
                }

                void linestring_add_location(const osmium::geom::Coordinates& xy) {
                    assert(!!m_linestring);
                    m_linestring->addPoint(xy.x, xy.y);
                }

                linestring_type linestring_finish(size_t /* num_points */) {
                    assert(!!m_linestring);
                    return std::move(m_linestring);
                }

                /* Polygon */

                void polygon_start() {
                    m_ring.reset(new OGRLinearRing{});
                }

                void polygon_add_location(const osmium::geom::Coordinates& xy) {
                    assert(!!m_ring);
                    m_ring->addPoint(xy.x, xy.y);
                }

                polygon_type polygon_finish(size_t /* num_points */) {
                    auto polygon = std::unique_ptr<OGRPolygon>{new OGRPolygon{}};
                    polygon->addRingDirectly(m_ring.release());
                    return polygon;
                }

                /* MultiPolygon */

                void multipolygon_start() {
                    m_multipolygon.reset(new OGRMultiPolygon{});
                }

                void multipolygon_polygon_start() {
                    m_polygon.reset(new OGRPolygon{});
                }

                void multipolygon_polygon_finish() {
                    assert(!!m_multipolygon);
                    assert(!!m_polygon);
                    m_multipolygon->addGeometryDirectly(m_polygon.release());
                }

                void multipolygon_outer_ring_start() {
                    m_ring.reset(new OGRLinearRing{});
                }

                void multipolygon_outer_ring_finish() {
                    assert(!!m_polygon);
                    assert(!!m_ring);
                    m_polygon->addRingDirectly(m_ring.release());
                }

                void multipolygon_inner_ring_start() {
                    m_ring.reset(new OGRLinearRing{});
                }

                void multipolygon_inner_ring_finish() {
                    assert(!!m_polygon);
                    assert(!!m_ring);
                    m_polygon->addRingDirectly(m_ring.release());
                }

                void multipolygon_add_location(const osmium::geom::Coordinates& xy) {
                    assert(!!m_polygon);
                    assert(!!m_ring);
                    m_ring->addPoint(xy.x, xy.y);
                }

                multipolygon_type multipolygon_finish() {
                    assert(!!m_multipolygon);
                    return std::move(m_multipolygon);
                }

            }; // class OGRFactoryImpl

        } // namespace detail

        template <typename TProjection = IdentityProjection>
        using OGRFactory = GeometryFactory<osmium::geom::detail::OGRFactoryImpl, TProjection>;

    } // namespace geom

} // namespace osmium

#endif // OSMIUM_GEOM_OGR_HPP
