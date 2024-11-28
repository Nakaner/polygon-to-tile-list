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

#include "tile_list.hpp"
#include <linux/limits.h>
#include <unistd.h>
#include <algorithm>
#include <memory>
#include <vector>

TileList::TileList(uint32_t maxzoom, bool check_tiles, bool tirex) :
    maxzoom(maxzoom),
    check_tiles(check_tiles), 
    tirex(tirex) {
    last_tile_x = static_cast<uint32_t>(1u << maxzoom) + 1;
    last_tile_y = static_cast<uint32_t>(1u << maxzoom) + 1;
}

bool TileList::check_file_exists(const char* path) {
    return (access(path, F_OK) == 0);
}

std::unique_ptr<char> TileList::get_tile_path(const std::string& path, const uint32_t zoom, const uint32_t x, const uint32_t y, const std::string& suffix, bool tirex_mode) {
    std::unique_ptr<char> str {new char[PATH_MAX]};
    if (tirex_mode) {
        snprintf(str.get(), PATH_MAX, "x=%u y=%u z=%u %s", 8*x, 8*y, zoom+3, suffix.c_str());
    } else if (path.empty()) {
        snprintf(str.get(), PATH_MAX, "%u/%u/%u%s", zoom, x, y, suffix.c_str());
    } else {
        snprintf(str.get(), PATH_MAX, "%s/%u/%u/%u%s", path.c_str(), zoom, x, y, suffix.c_str());
    }
    return str;
}

void TileList::add_tile(uint32_t x, uint32_t y)
{
    // Only try to insert to tile into the set if the last inserted tile
    // is different from this tile.
    if (last_tile_x != x || last_tile_y != y) {
        m_dirty_tiles.insert(xy_to_quadkey(x, y, maxzoom));
        last_tile_x = x;
        last_tile_y = y;
    }
}

void TileList::output(FILE* output_file, uint32_t minzoom, const std::string& suffix,
        const char delimiter, const std::string& path) {
    // build a sorted vector of all expired tiles
    std::vector<uint64_t> tiles_maxzoom(m_dirty_tiles.begin(),
                                        m_dirty_tiles.end());
    std::sort(tiles_maxzoom.begin(), tiles_maxzoom.end());
    /* Loop over all requested zoom levels (from maximum down to the minimum zoom level).
     * Tile IDs of the tiles enclosing this tile at lower zoom levels are calculated using
     * bit shifts.
     *
     * last_quadkey is initialized with a value which is not expected to exist
     * (larger than largest possible quadkey). */
    uint64_t last_quadkey = 1ULL << (2 * maxzoom);
    for (std::vector<uint64_t>::const_iterator it = tiles_maxzoom.cbegin();
         it != tiles_maxzoom.cend(); ++it) {
        for (uint32_t dz = 0; dz <= maxzoom - minzoom; dz++) {
            // scale down to the current zoom level
            uint64_t qt_current = *it >> (dz * 2);
            /* If dz > 0, there are propably multiple elements whose quadkey
             * is equal because they are all sub-tiles of the same tile at the current
             * zoom level. We skip all of them after we have written the first sibling.
             */
            if (qt_current == last_quadkey >> (dz * 2)) {
                continue;
            }
            xy_coord_t xy = quadkey_to_xy(qt_current, maxzoom - dz);
            std::unique_ptr<char> tile_path = get_tile_path(path, maxzoom - dz, xy.x, xy.y, suffix, tirex);
            if (check_tiles && !check_file_exists(tile_path.get())) {
                continue;
            }
            fprintf(output_file, "%s%c", tile_path.get(), delimiter);
        }
        last_quadkey = *it;
    }
}

uint64_t TileList::xy_to_quadkey(uint32_t x, uint32_t y, uint32_t zoom)
{
    uint64_t quadkey = 0;
    // the two highest bits are the bits of zoom level 1, the third and fourth bit are level 2, …
    for (uint32_t z = 0; z < zoom; z++) {
        quadkey |= ((x & (1ULL << z)) << z);
        quadkey |= ((y & (1ULL << z)) << (z + 1));
    }
    return quadkey;
}

xy_coord_t TileList::quadkey_to_xy(uint64_t quadkey_coord, uint32_t zoom)
{
    xy_coord_t result;
    for (uint32_t z = zoom; z > 0; --z) {
        /* The quadkey contains Y and X bits interleaved in following order: YXYX...
         * We have to pick out the bit representing the y/x bit of the current zoom
         * level and then shift it back to the right on its position in a y-/x-only
         * coordinate.*/
        result.y = result.y + static_cast<uint32_t>(
                                  (quadkey_coord & (1ULL << (2 * z - 1))) >> z);
        result.x = result.x +
                   static_cast<uint32_t>(
                       (quadkey_coord & (1ULL << (2 * (z - 1)))) >> (z - 1));
    }
    return result;
}
