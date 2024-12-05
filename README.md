# Polygon-To-Tile-List

Polygon-To-Tile-List converts a geometry or bounding box into a list of tile coordinates (zoom/x/y)
according to the [slippy map tile naming convention](https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames).

## Usage

```
polygon-to-tile-list OPTIONS 

Options:
  -h, --help                  print help and exit
  -a STR, --append=STR        Print following string at the end of the output. The program will append newline character to the string
  -b BBOX, --bbox=BBOX        bounding box separated by comma: min_lon,min_lat,max_lon,max_lat
  --buffer-size=SIZE          buffer size in meter for lines and polygons (not bounding boxes)
  -c, --check-exists          Check if the tiles exist as files on the disk.
  -d DIR, --directory=DIR     Tile directory for --check-exists.
  -g PATH, --geom=PATH        Print all tiles intersecting with the (multi)linestrings and (multi)polygons in the specified file
  -n, --null                  Use NULL character, not LF as file delimiter.
  -s SUFFIX, --suffix=SUFFIX  suffix to append (do not forget the leading dot)
  -t, --tirex                 tirex mode (different output style, only coords that are multiples of 8)
  -z ZOOM, --minzoom=ZOOM     minimum zoom level, defaults to 0
  -Z ZOOM, --maxzoom=ZOOM     maximum zoom level, defaults to 14
  -o FILE, --output=FILE      write output to file instead of standard output
  -v, --verbose               be verbose
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
