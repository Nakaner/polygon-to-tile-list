/*
 * utils.hpp
 *
 *  Created on:  2019-09-05
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#ifndef SRC_UTILS_HPP_
#define SRC_UTILS_HPP_

#include "projection.hpp"

struct BoundingBox {
    double min_lon;
    double min_lat;
    double max_lon;
    double max_lat;

    static BoundingBox from_str(const char* bbox_str);

    BoundingBox(double x1, double y1, double x2, double y2);

    BoundingBox();
};

struct ZoomRange {
    uint32_t xmin;
    uint32_t ymin;
    uint32_t xmax;
    uint32_t ymax;

    ZoomRange(uint32_t zoom);

    ZoomRange(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2);

    uint32_t width() const;

    uint32_t height() const;

    static uint32_t get_max_xy_index(const uint32_t zoom);

    /**
     * Build a zoom range from a bounding box in geographic coordinates
     */
    static ZoomRange from_bbox_geographic(const BoundingBox& b, const uint32_t zoom);

    /**
     * Build a zoom range from a bounding box in Web Mercator coordinates
     */
    static ZoomRange from_bbox_webmerc(const double x1, const double y1, const double x2, const double y2, const uint32_t zoom);
};



#endif /* SRC_UTILS_HPP_ */
