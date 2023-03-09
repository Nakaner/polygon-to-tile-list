/*
 * gdal_intersecting_tiles_finder.cpp
 *
 *  Created on:  2019-09-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */
#include "gdal_intersecting_tiles_finder.hpp"
#include "projection.hpp"
#include "utils.hpp"
#include <cmath>
#include <iostream>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/Polygon.h>

GDALIntersectingTilesFinder::GDALIntersectingTilesFinder(const bool verbose, uint32_t minzoom, uint32_t maxzoom) :
    m_features(0),
    m_minzoom(minzoom),
    m_verbose(verbose),
    m_maxzoom(maxzoom),
    m_tile_list(maxzoom),
    m_coord_sequence_factory(),
    m_wkb_reader(),
    m_web_merc_ref(),
#ifdef GEOS_36
    m_geos_factory(geos::geom::GeometryFactory::create(new geos::geom::PrecisionModel(), -1)) {
#else
    m_geos_factory(new geos::geom::GeometryFactory(pm, -1)) {
#endif
    m_tree.reset(new geos::index::quadtree::Quadtree());
    m_web_merc_ref.importFromProj4("+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs");
    OGRRegisterAll();
}

void GDALIntersectingTilesFinder::handle_geos_geometry(geos_geom_type geometry, const double buffer_size) {
    const geos::geom::Envelope* envelope = geometry->getEnvelopeInternal();
    // 2) get buffer size at that latitude
    geos_geom_type buffered;
    bool buffering_required = (buffer_size > 0);
    if (buffering_required) {
        double avg_lat = (envelope->getMaxY() - envelope->getMinY()) / 2 + envelope->getMinY();
        double scale = projection::mercator_scale(projection::y_to_lat(avg_lat));
        double buffer = buffer_size * scale;
        // 3) buffer
        buffered = geometry->buffer(buffer, 4);
        // 4) get bounding box of buffer
        envelope = buffered->getEnvelopeInternal();
    } else {
        buffered = geometry->clone();
    }
#ifdef GEOS_38
    geos_prep_type prep = geos::geom::prep::PreparedGeometryFactory::prepare(buffered.get());
#else
    geos_prep_type prep = geos::geom::prep::PreparedGeometryFactory::prepare(buffered);
#endif
    // 5) create tiles in bounding box
    ZoomRange tile_range = ZoomRange::from_bbox_webmerc(envelope->getMinX(),
            envelope->getMinY(), envelope->getMaxX(), envelope->getMaxY(), m_maxzoom);
    // 6) check which tiles intersect, add them to the tile list
    for (uint32_t x = tile_range.xmin; x <= tile_range.xmax; ++x) {
        for (uint32_t y = tile_range.ymin; y <= tile_range.ymax; ++y) {
            // shortcut: If zoom range is 1 tile wide or high, skip the intersection check
            if (tile_range.width() == 1 || tile_range.height() == 1) {
                m_tile_list.add_tile(x, y);
                continue;
            }
            // create tile
            // check if bounding box of the geometry in the shape file is fully covered by the tile
            std::unique_ptr<std::vector<geos::geom::Coordinate>> coords {new std::vector<geos::geom::Coordinate>};
            coords->emplace_back(projection::tile_x_to_merc(x, m_maxzoom), projection::tile_y_to_merc(y, m_maxzoom));
            coords->emplace_back(projection::tile_x_to_merc(x, m_maxzoom), projection::tile_y_to_merc(y + 1, m_maxzoom));
            coords->emplace_back(projection::tile_x_to_merc(x + 1, m_maxzoom), projection::tile_y_to_merc(y + 1, m_maxzoom));
            coords->emplace_back(projection::tile_x_to_merc(x + 1, m_maxzoom), projection::tile_y_to_merc(y, m_maxzoom));
            coords->emplace_back(projection::tile_x_to_merc(x, m_maxzoom), projection::tile_y_to_merc(y, m_maxzoom));
            std::unique_ptr<geos::geom::CoordinateSequence> coord_sequence {m_coord_sequence_factory.create(coords.release(), 2)};
            std::unique_ptr<geos::geom::LineString> ring {m_geos_factory->createLineString(coord_sequence.release())};
            if (prep->intersects(ring.get())) {
                m_tile_list.add_tile(x, y);
            }
        }
    }
#ifndef GEOS_38
    geos::geom::prep::PreparedGeometryFactory::destroy(prep);
    if (buffering_required) {
        delete buffered;
    }
#endif
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

void GDALIntersectingTilesFinder::output(FILE* output_file, const std::string& suffix) {
    m_tile_list.output(output_file, m_minzoom, suffix);
}

void GDALIntersectingTilesFinder::handle_geometry(OGRGeometry* geometry,
        OGRCoordinateTransformation* transformation, const double buffer_size) {
    // steps to do:
    // 1) transform to EPSG:3857
    if (geometry->transform(transformation) != OGRERR_NONE) {
        std::cerr << "Failed to transform geometry\n";
        exit(1);
    }
    geos_geom_type geos_geom = ogr2geos(geometry);
    handle_geos_geometry(std::move(geos_geom), buffer_size);
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

geos_geom_type GDALIntersectingTilesFinder::ogr2geos(const OGRGeometry* ogrgeom) {
    char static_buffer[1024 * 1024];
    char *buffer = static_buffer;
    size_t wkb_size = ogrgeom->WkbSize();
    if (wkb_size > sizeof(static_buffer)) {
        buffer = static_cast<char*>(malloc(wkb_size));
    }
    ogrgeom->exportToWkb(wkbXDR, reinterpret_cast<unsigned char*>(buffer));
    std::istringstream ss;
    ss.rdbuf()->pubsetbuf(buffer, wkb_size);
    geos_geom_type geosgeom = m_wkb_reader.read(ss);
    if (buffer != static_buffer) {
        free(buffer);
    }
    return geosgeom;
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
        std::cerr << "Processing " << layer->GetFeatureCount() << " features from layer " << layer->GetName() << " of " << path << '\n';
        handle_layer(layer, buffer_size);
    }
}


