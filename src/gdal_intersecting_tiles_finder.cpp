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

#include "gdal_intersecting_tiles_finder.hpp"
#include "projection.hpp"
#include "utils.hpp"
#include <cmath>
#include <iostream>
#include <boost/geometry.hpp>


GDALIntersectingTilesFinder::GDALIntersectingTilesFinder(const bool verbose, uint32_t minzoom,
        uint32_t maxzoom, bool check_tiles, bool tirex) :
    m_features(0),
    m_minzoom(minzoom),
    m_verbose(verbose),
    m_maxzoom(maxzoom),
    m_tile_list(maxzoom, check_tiles, tirex),
    m_web_merc_ref() {
    m_web_merc_ref.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs");
    OGRRegisterAll();
}

void GDALIntersectingTilesFinder::handle_boost_geometry(bgeometry_t geometry, const double buffer_size) {
    box_t box = get_envelope_from_geom(geometry);
    // 2) get buffer size at that latitude
    bool buffering_required = (buffer_size > 0);
    bgeometry_t buffered = geometry;
    if (buffering_required) {
        double avg_lat = (box.max_corner().get<1>() - box.min_corner().get<1>()) / 2 + box.min_corner().get<1>();
        double scale = projection::mercator_scale(projection::y_to_lat(avg_lat));
        double buffer = buffer_size * scale;
        // 3) buffer
        buffered = std::move(get_buffer_from_geom(geometry, buffer));
        // 4) get bounding box of buffer
        box = get_envelope_from_geom(buffered);
    }
    // 5) create tiles in bounding box
    ZoomRange tile_range = ZoomRange::from_bbox_webmerc(box.min_corner().get<0>(),
            box.min_corner().get<1>(), box.max_corner().get<0>(), box.max_corner().get<1>(), m_maxzoom);
    // 6) check which tiles intersect, add them to the tile list
    for (uint32_t x = tile_range.xmin; x <= tile_range.xmax; ++x) {
        for (uint32_t y = tile_range.ymin; y <= tile_range.ymax; ++y) {
            // Shortcut: If zoom range is 1 tile wide or high (i.e. difference between min and max
            // is 0), skip the intersection check.
            if (tile_range.width() == 0 || tile_range.height() == 0) {
                m_tile_list.add_tile(x, y);
                continue;
            }
            // create tile
            // check if bounding box of the geometry in the shape file is fully covered by the tile
            box_t tile_box {
                {projection::tile_x_to_merc(x, m_maxzoom), projection::tile_y_to_merc(y + 1, m_maxzoom)},
                {projection::tile_x_to_merc(x + 1, m_maxzoom), projection::tile_y_to_merc(y, m_maxzoom)}
            };
            if (geoms_intersect(buffered, box)) {
                m_tile_list.add_tile(x, y);
            }
        }
    }
}

box_t GDALIntersectingTilesFinder::get_envelope_from_geom(const bgeometry_t& geom) {
    box_t box;
    if (std::holds_alternative<bpoint_t>(geom)) {
        bpoint_t p = std::get<bpoint_t>(geom);
        bgeom::envelope(p, box);
    } else if (std::holds_alternative<bmulti_point_t>(geom)) {
        bmulti_point_t p = std::get<bmulti_point_t>(geom);
        bgeom::envelope(p, box);
    } else if (std::holds_alternative<blinestring_t>(geom)) {
        blinestring_t p = std::get<blinestring_t>(geom);
        bgeom::envelope(p, box);
    } else if (std::holds_alternative<bmulti_linestring_t>(geom)) {
        bmulti_linestring_t p = std::get<bmulti_linestring_t>(geom);
        bgeom::envelope(p, box);
    } else if (std::holds_alternative<bpolygon_t>(geom)) {
        bpolygon_t p = std::get<bpolygon_t>(geom);
        bgeom::envelope(p, box);
    } else if (std::holds_alternative<bmulti_polygon_t>(geom)) {
        bmulti_polygon_t p = std::get<bmulti_polygon_t>(geom);
        bgeom::envelope(p, box);
    }
    return box;
}

template<typename TGeometry>
inline bmulti_polygon_t call_buffer(TGeometry& geom, const double radius) {
    boost::geometry::strategy::buffer::distance_symmetric<geometry_numeric_type> distance_strategy(radius);
    boost::geometry::strategy::buffer::join_round join_strategy(10);
    boost::geometry::strategy::buffer::end_round end_strategy(10);
    boost::geometry::strategy::buffer::point_circle circle_strategy(10);
    boost::geometry::strategy::buffer::side_straight side_strategy;
    bmulti_polygon_t buffer;
    bgeom::buffer(geom, buffer, distance_strategy, side_strategy, join_strategy, end_strategy, circle_strategy);
    return buffer;
}

bmulti_polygon_t GDALIntersectingTilesFinder::get_buffer_from_geom(bgeometry_t& geom, const double radius) {
    if (std::holds_alternative<bpoint_t>(geom)) {
        bpoint_t& p = std::get<bpoint_t>(geom);
        return call_buffer(p, radius);
    } else if (std::holds_alternative<bmulti_point_t>(geom)) {
        bmulti_point_t p = std::get<bmulti_point_t>(geom);
        return call_buffer(p, radius);
    } else if (std::holds_alternative<blinestring_t>(geom)) {
        blinestring_t p = std::get<blinestring_t>(geom);
        return call_buffer(p, radius);
    } else if (std::holds_alternative<bmulti_linestring_t>(geom)) {
        bmulti_linestring_t p = std::get<bmulti_linestring_t>(geom);
        return call_buffer(p, radius);
    } else if (std::holds_alternative<bpolygon_t>(geom)) {
        bpolygon_t p = std::get<bpolygon_t>(geom);
        return call_buffer(p, radius);
    } else if (std::holds_alternative<bmulti_polygon_t>(geom)) {
        bmulti_polygon_t p = std::get<bmulti_polygon_t>(geom);
        return call_buffer(p, radius);
    }
    return bmulti_polygon_t();
}

bool GDALIntersectingTilesFinder::geoms_intersect(const bgeometry_t& geom, const box_t& box) {
    if (std::holds_alternative<bpoint_t>(geom)) {
        bpoint_t p = std::get<bpoint_t>(geom);
        return bgeom::intersects(p, box);
    } else if (std::holds_alternative<bmulti_point_t>(geom)) {
        bmulti_point_t p = std::get<bmulti_point_t>(geom);
        return bgeom::intersects(p, box);
    } else if (std::holds_alternative<blinestring_t>(geom)) {
        blinestring_t p = std::get<blinestring_t>(geom);
        return bgeom::intersects(p, box);
    } else if (std::holds_alternative<bmulti_linestring_t>(geom)) {
        bmulti_linestring_t p = std::get<bmulti_linestring_t>(geom);
        return bgeom::intersects(p, box);
    } else if (std::holds_alternative<bpolygon_t>(geom)) {
        bpolygon_t p = std::get<bpolygon_t>(geom);
        return bgeom::intersects(p, box);
    } else if (std::holds_alternative<bmulti_polygon_t>(geom)) {
        bmulti_polygon_t p = std::get<bmulti_polygon_t>(geom);
        return bgeom::intersects(p, box);
    }
    return false;
}

void GDALIntersectingTilesFinder::end_progress() {
    if (m_verbose) {
        fprintf(stderr, "\n");
    }
}

void GDALIntersectingTilesFinder::progress() {
    if (!m_verbose) {
        return;
    }
    ++m_features;
    if (m_features % 10 == 0) {
        fprintf(stderr, "\r%ld features processed", m_features);
    }
}

void GDALIntersectingTilesFinder::reset_progress() {
    if (m_verbose) {
        m_features = 0;
    }
}

void GDALIntersectingTilesFinder::output(FILE* output_file, const std::string& suffix,
        const char delimiter, const std::string& path) {
    m_tile_list.output(output_file, m_minzoom, suffix, delimiter, path);
}

void GDALIntersectingTilesFinder::handle_geometry(OGRGeometry* geometry,
        OGRCoordinateTransformation* transformation, const double buffer_size) {
    // steps to do:
    // 1) transform to EPSG:3857
    if (geometry->transform(transformation) != OGRERR_NONE) {
        std::cerr << "Failed to transform geometry\n";
        exit(1);
    }
    bgeometry_t geom = ogr2boost_geom(geometry);
    handle_boost_geometry(std::move(geom), buffer_size);
}

void GDALIntersectingTilesFinder::handle_layer(OGRLayer* layer, const double buffer_size) {
    OGRFeature* feature;
    layer->ResetReading();
    std::unique_ptr<OGRCoordinateTransformation> tranformation {OGRCreateCoordinateTransformation(layer->GetSpatialRef(), &m_web_merc_ref)};

    reset_progress();
    while ((feature = layer->GetNextFeature()) != NULL) {
        OGRGeometry* geom = feature->GetGeometryRef();
        handle_geometry(geom, tranformation.get(), buffer_size);
        OGRFeature::DestroyFeature(feature);
        progress();
    }
    end_progress();
}

bgeometry_t GDALIntersectingTilesFinder::ogr2boost_geom(const OGRGeometry* ogr_geom) {
    if (ogr_geom->IsEmpty()) {
        return bpoint_t();
    }
    switch (ogr_geom->getGeometryType()) {
    case wkbPoint: {
        const OGRPoint* point = ogr_geom->toPoint();
        return bpoint_t(point->getX(), point->getY());
        break;
    }
    case wkbMultiPoint: {
        bmulti_point_t mp;
        const OGRMultiPoint* multi_point = ogr_geom->toMultiPoint();
        for (auto it = multi_point->begin(); it != multi_point->end(); ++it) {
            const OGRPoint* p = *it;
            bgeom::append(mp, bpoint_t(p->getX(), p->getY()));
        }
        return mp;
        break;
    }
    case wkbLineString: {
        const OGRLineString* ogr_linestring = ogr_geom->toLineString();
        blinestring_t ls;
        add_points_to_linestring(ls, ogr_linestring);
        return ls;
        break;
    }
    case wkbPolygon: {
        return ogr_polygon2boost_geom(ogr_geom->toPolygon());
        break;
    }
    case wkbMultiPolygon: {
        return ogr_multipolygon2boost_geom(ogr_geom->toMultiPolygon());
        break;
    }
    default:
        std::cerr << "ERROR: Got geometry type " << ogr_geom->getGeometryName() << " for conversion to Boost Geometry.\n";
        exit(1);
        break;
    }
}

void GDALIntersectingTilesFinder::add_points_to_linestring(blinestring_t& ls, const OGRSimpleCurve* c) {
    for (auto it = c->begin(); it != c->end(); ++it) {
        const OGRPoint& p = *it;
        bgeom::append(ls, bpoint_t(p.getX(), p.getY()));
    }
}

bpolygon_t GDALIntersectingTilesFinder::ogr_polygon2boost_geom(const OGRPolygon* ogr_polygon) {
    bpolygon_t p;
    add_polygon_to_multipolygon(p, ogr_polygon);
    return p;
}

bmulti_polygon_t GDALIntersectingTilesFinder::ogr_multipolygon2boost_geom(const OGRMultiPolygon* ogr_multi_polygon) {
    bmulti_polygon_t mp;
    auto poly_it = ogr_multi_polygon->begin();
    size_t outer_count = 1;
    for (; poly_it != ogr_multi_polygon->end(); ++poly_it, ++outer_count) {
        mp.resize(outer_count);
        const OGRPolygon* poly = *poly_it;
        add_polygon_to_multipolygon(mp.at(outer_count - 1), poly);
    }
    return mp;
}

void GDALIntersectingTilesFinder::add_polygon_to_multipolygon(bpolygon_t& multi_polygon_part, const OGRPolygon* ogr_polygon) {
    const OGRLinearRing* const* ring_it = ogr_polygon->begin();
    for (auto p_it = (*ring_it)->begin(); p_it != (*ring_it)->end() && ring_it != ogr_polygon->end(); ++p_it) {
        const OGRPoint& p = *p_it;
        bgeom::append(multi_polygon_part.outer(), bpoint_t(p.getX(), p.getY()));
    }
    ++ring_it;
    size_t rings = 1;
    for (; ring_it != ogr_polygon->end(); ++ring_it, ++rings) {
        multi_polygon_part.inners().resize(rings);
        for (const auto& p : *ring_it) {
            bgeom::append(multi_polygon_part.inners()[rings - 1], bpoint_t(p.getX(), p.getY()));
        }
    }
}

void GDALIntersectingTilesFinder::find_intersections(const std::string& path, const double buffer_size) {
    #if GDAL_VERSION_MAJOR >= 2
    std::unique_ptr<gdal_dataset_type> dataset {static_cast<gdal_dataset_type*>(GDALOpenEx(path.c_str(),
            GDAL_OF_VECTOR | GDAL_OF_READONLY | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL))};
    #else
    std::unique_ptr<gdal_dataset_type> dataset {OGRSFDriverRegistrar::Open(argv[optind + 1], FALSE)};
    #endif
    if (dataset == NULL) {
        std::cerr << "Opening " << path << " failed.\n";
        exit(1);
    }
    int layer_count = dataset->GetLayerCount();
    for (int i = 0; i < layer_count; ++i) {
        OGRLayer* layer = dataset->GetLayer(i);
        if (layer == NULL) {
            std::cerr << "WARNING: Skipping broken data layer " << i << " in " << path << '\n';
            continue;
        }
        if (layer->GetSpatialRef() == NULL) {
            std::cerr << "WARNING: Data layer " << i << " in " << path << " has no spatial reference. Skipping it.\n";
            continue;
        }
        if (layer->GetFeatureCount() == 0) {
            std::cerr << "WARNING: Skipping empty layer " << layer->GetName() << " of " << path << '\n';
            continue;
        }
        if (m_verbose) {
            std::cerr << "Processing " << layer->GetFeatureCount() << " features from layer " << layer->GetName() << " of " << path << '\n';
        }
        handle_layer(layer, buffer_size);
    }
}


