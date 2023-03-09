/*
 * tile_list.cpp
 *
 *  Created on:  2019-09-05
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */

#include "tile_list.hpp"
#include <algorithm>
#include <vector>

TileList::TileList(uint32_t maxzoom) :
    maxzoom(maxzoom) {
    last_tile_x = static_cast<uint32_t>(1u << maxzoom) + 1;
    last_tile_y = static_cast<uint32_t>(1u << maxzoom) + 1;
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

void TileList::output(FILE* output_file, uint32_t minzoom, const std::string& suffix)
{
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
            fprintf(output_file, "%u/%u/%u%s\n", maxzoom - dz, xy.x, xy.y, suffix.c_str());
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
