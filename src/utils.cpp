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

#include "utils.hpp"
#include <string.h>
#include <algorithm>
#include <array>
#include <iostream>


/*static*/ BoundingBox BoundingBox::from_str(const char* bbox_str) {
    std::array<double, 4> coords;
    size_t element = 0;
    char* ptr;
    coords[0] = strtod(bbox_str, &ptr);
    ++element;
    while (element < coords.size() && *ptr == ',') {
        ++ptr; // move one character forward
        char* next;
        coords[element] = strtod(ptr, &next);
        ptr = next;
        ++element;
    }
    if (element < 4) {
        throw std::runtime_error{"Bounding box contains less than four elements."};
    }
    return BoundingBox{coords[0], coords[1], coords[2], coords[3]};
}

BoundingBox::BoundingBox(double x1, double y1, double x2, double y2) :
    min_lon(x1),
    min_lat(y1),
    max_lon(x2),
    max_lat(y2) {
}

BoundingBox::BoundingBox() :
    BoundingBox(-180, -83, 180, 83) {
}

ZoomRange::ZoomRange(uint32_t zoom) :
    xmin(0),
    ymin(0),
    xmax(get_max_xy_index(zoom)),
    ymax(get_max_xy_index(zoom)) {
}

ZoomRange::ZoomRange(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2) :
    xmin(x1),
    ymin(y1),
    xmax(x2),
    ymax(y2) {
}

uint32_t ZoomRange::width() const {
    return xmax - xmin;
}

uint32_t ZoomRange::height() const {
    return ymax - ymin;
}

/*static*/ uint32_t ZoomRange::get_max_xy_index(const uint32_t zoom) {
    return (1u << zoom);
}

/*static*/ ZoomRange ZoomRange::from_bbox_geographic(const BoundingBox& b, const uint32_t zoom) {
    ZoomRange range {zoom};
    range.xmin = static_cast<uint32_t>(projection::merc_x_to_tile(projection::lon_to_x(b.min_lon), zoom));
    range.ymin = static_cast<uint32_t>(projection::merc_y_to_tile(projection::lat_to_y(b.max_lat), zoom));
    range.xmax = static_cast<uint32_t>(projection::merc_x_to_tile(projection::lon_to_x(b.max_lon), zoom));
    range.ymax = static_cast<uint32_t>(projection::merc_y_to_tile(projection::lat_to_y(b.min_lat), zoom));
    return range;
}

/*static*/ ZoomRange ZoomRange::from_bbox_webmerc(const double x1, const double y1, const double x2, const double y2, const uint32_t zoom) {
    ZoomRange range {zoom};
    range.xmin = static_cast<uint32_t>(projection::merc_x_to_tile(x1, zoom));
    range.ymin = static_cast<uint32_t>(projection::merc_y_to_tile(y2, zoom));
    range.xmax = static_cast<uint32_t>(projection::merc_x_to_tile(x2, zoom));
    range.ymax = static_cast<uint32_t>(projection::merc_y_to_tile(y1, zoom));
    return range;
}
