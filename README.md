# Polygon-To-Tile-List

Polygon-To-Tile-List converts a geometry or bounding box into a list of tile coordinates (zoom/x/y)
according to the [slippy map tile naming convention](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames).

## Usage

```
polygon-to-tile-list OPTIONS 

Options:
  -h, --help                  print help and exit
  -b BBOX, --bbox=BBOX        bounding box separated by comma: min_lon,min_lat,max_lon,max_lat
  -g FILE, --geom=FILE        path to shape file with (multi)linestrings or (multi)polygons
  --buffer-size=METER         buffer size in meter (not for bounding boxes)
  -z ZOOM, --minzoom=ZOOM     maximal zoom level for the low zoom range
  -Z ZOOM, --maxzoom=ZOOM     maximal zoom level for the med zoom range
  -s SUFFIX, --suffix=SUFFIX  suffix to append (do not forget the leading dot)
  -o FILE, --output=FILE      output file (instead of standard output)
  -v, --verbose               verbose output
```

This program generates a list of tile coordinates, in the format zoom/x/y with an optional file name suffix, on stdout.

You can specify a bounding box or a shape file of line or polygon geometries (to be precise: any file GDAL can read).
The program will print only those tiles intersecting with the bounding box/geometry.

If you specify both a bounding box and a geometry, tiles intersecting any of the two will be printed.

## Dependencies

* Boost Geometry
* GDAL

## Building

```sh
mkdir build
cd build
cmake ..
make
```

## License

This project is licensed under the terms of General Public License version 2 or newer.
