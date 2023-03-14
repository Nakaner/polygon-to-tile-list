/*
 * gdal_intersecting_tiles_finder.hpp
 *
 *  Created on:  2019-09-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_GDAL_INTERSECTING_TILES_FINDER_HPP_
#define SRC_GDAL_INTERSECTING_TILES_FINDER_HPP_

#include <memory>
#include <sstream>
#include <string>
#include <geos/index/quadtree/Quadtree.h>
#include <geos/geom/CoordinateArraySequenceFactory.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/PrecisionModel.h>
#include <geos/geom/prep/PreparedGeometryFactory.h>
#include <gdal/gdal_version.h>
#include <gdal/ogr_geometry.h>
#include <gdal/ogrsf_frmts.h>
#include <gdal/ogr_api.h>
#include <geos/io/WKBReader.h>
#include <geos/version.h>
#include "tile_list.hpp"

#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && (GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 6))
#define GEOS_36
#endif
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && (GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 8))
#define GEOS_38
#endif

#ifdef GEOS_38
using geos_geom_type = geos::geom::Geometry::Ptr;
using geos_prep_type = std::unique_ptr<const geos::geom::prep::PreparedGeometry>;
using geos_geometry_factory_type = geos::geom::GeometryFactory::Ptr;
#else
using geos_geom_type = geos::geom::Geometry*;
using geos_prep_type = const geos::geom::prep::PreparedGeometry*;
#ifdef GEOS_36
using geos_geometry_factory_type = geos::geom::GeometryFactory::unique_ptr;
#else
using geos_geometry_factory_type = geos::geom::GeometryFactory*;
#endif
#endif

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
    geos::io::WKBReader m_wkb_reader;
    OGRSpatialReference m_web_merc_ref;
    bool m_verbose;
    uint32_t m_maxzoom;
    TileList m_tile_list;
    geos::geom::CoordinateArraySequenceFactory m_coord_sequence_factory;
    geos_geometry_factory_type m_geos_factory;
    std::unique_ptr<geos::index::quadtree::Quadtree> m_tree;

    void handle_geos_geometry(geos_geom_type geometry, const double buffer_size);

    void end_progress();

    void progress();

    void reset_progress();

    void handle_geometry(OGRGeometry* geom, OGRCoordinateTransformation* transformation, const double buffer_size);

    void handle_layer(OGRLayer* layer, const double buffer_size);

    geos_geom_type ogr2geos(const OGRGeometry* ogrgeom);

public:
    GDALIntersectingTilesFinder() = delete;

    GDALIntersectingTilesFinder(const bool verbose, uint32_t minzoom, uint32_t maxzoom, const bool check_tiles);

    void find_intersections(const std::string& input_filepath, const double buffer_size);

    void output(FILE* output_file, const std::string& suffix, const char delimiter, const std::string& path);
};



#endif /* SRC_GDAL_INTERSECTING_TILES_FINDER_HPP_ */
