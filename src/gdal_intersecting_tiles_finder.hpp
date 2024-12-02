/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2024 Geofabrik GmbH
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SRC_GDAL_INTERSECTING_TILES_FINDER_HPP_
#define SRC_GDAL_INTERSECTING_TILES_FINDER_HPP_

#include <memory>
#include <sstream>
#include <string>
#include <gdal_version.h>
#include <ogr_geometry.h>
#include <ogrsf_frmts.h>
#include <ogr_api.h>
#include "tile_list.hpp"
#include <variant>
#include <boost/geometry/geometries/adapted/c_array.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/multi_point.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/multi_linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/multi_polygon.hpp>
#include <boost/geometry/geometries/box.hpp>


namespace bgeom = boost::geometry;
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(bgeom::cs::cartesian)
using geometry_numeric_type = double;
using bpoint_t = bgeom::model::d2::point_xy<geometry_numeric_type>;
using bmulti_point_t = bgeom::model::multi_point<bpoint_t>;
using blinestring_t = bgeom::model::linestring<bpoint_t>;
using bmulti_linestring_t = bgeom::model::multi_linestring<blinestring_t>;
using bpolygon_t = bgeom::model::polygon<bpoint_t>;
using bmulti_polygon_t = bgeom::model::multi_polygon<bpolygon_t>;
using box_t = bgeom::model::box<bpoint_t>;
using bgeometry_t = std::variant<bpoint_t, bmulti_point_t, blinestring_t, bmulti_linestring_t, bpolygon_t, bmulti_polygon_t>;


class GDALIntersectingTilesFinder {

#if GDAL_VERSION_MAJOR >= 2
    using gdal_driver_type = GDALDriver;
    using gdal_dataset_type = GDALDataset;
#else
    using gdal_driver_type = OGRSFDriver;
    using gdal_dataset_type = OGRDataSource;
#endif

    size_t m_features;
    uint32_t m_minzoom;
    OGRSpatialReference m_web_merc_ref;
    bool m_verbose;
    uint32_t m_maxzoom;
    TileList m_tile_list;

    void handle_boost_geometry(bgeometry_t geometry, const double buffer_size);

    static box_t get_envelope_from_geom(const bgeometry_t& geom);

    static bmulti_polygon_t get_buffer_from_geom(bgeometry_t& geom, const double radius);

    static bool geoms_intersect(const bgeometry_t& geom, const box_t& box);

    void end_progress();

    void progress();

    void reset_progress();

    void handle_geometry(OGRGeometry* geom, OGRCoordinateTransformation* transformation, const double buffer_size);

    void handle_layer(OGRLayer* layer, const double buffer_size);

    bgeometry_t ogr2boost_geom(const OGRGeometry* ogr_geom);

    void add_points_to_linestring(blinestring_t& ls, const OGRSimpleCurve* c);

    bpolygon_t ogr_polygon2boost_geom(const OGRPolygon* ogr_polygon);

    bmulti_polygon_t ogr_multipolygon2boost_geom(const OGRMultiPolygon* ogr_multi_polygon);

    void add_polygon_to_multipolygon(bpolygon_t& multi_polygon_part, const OGRPolygon* ogr_polygon);

public:
    GDALIntersectingTilesFinder() = delete;

    GDALIntersectingTilesFinder(const bool verbose, uint32_t minzoom, uint32_t maxzoom, const bool check_tiles, const bool tirex);

    void find_intersections(const std::string& input_filepath, const double buffer_size);

    void output(FILE* output_file, const std::string& suffix, const char delimiter, const std::string& path);
};

#endif /* SRC_GDAL_INTERSECTING_TILES_FINDER_HPP_ */
