    /*
 * polygon-to-tile-list.cpp
 *
 *  Created on:  2023-03-09
 *      Author: Michael Reichert <michael.reichert@geofabrik.de>
 */


#include <getopt.h>
#include <string.h>
#include <iostream>
#include <vector>

#include "gdal_intersecting_tiles_finder.hpp"
#include "utils.hpp"


void print_all_tiles_on_range(FILE* output_file, const uint32_t minzoom, const uint32_t maxzoom,
        const BoundingBox& bbox, const std::string& suffix, const char delimiter,
        const bool check_exists, const std::string& path, bool tirex) {
    for (uint32_t z = minzoom; z <= maxzoom; ++z) {
        ZoomRange range = ZoomRange::from_bbox_geographic(bbox, z);
        for (uint32_t x = range.xmin; x <= range.xmax; ++x) {
            for (uint32_t y = range.ymin; y <= range.ymax; ++y) {
                std::unique_ptr<char> tile_path = TileList::get_tile_path(path, z, x, y, suffix, tirex);
                if (check_exists && !TileList::check_file_exists(tile_path.get())) {
                    continue;
                }
                fprintf(output_file, "%s%c", tile_path.get(), delimiter);
            }
        }
    }
}

void print_usage(char* argv[]) {
    std::cerr << "Usage: " << argv[0] << " OPTIONS\n" \
    "Positional Arguments:\n" \
    "Options:\n" \
    "  -h, --help                  print help and exit\n" \
    "  -a STR, --append=STR        Print following string at the end of the output. The program will append newline character to the string\n" \
    "  -b BBOX, --bbox=BBOX        bounding box separated by comma: min_lon,min_lat,max_lon,max_lat\n" \
    "  -c, --check-exists          Check if the tiles exist as files on the disk.\n" \
    "  -d DIR, --directory=DIR     Tile directory for --check-exists.\n" \
    "  -g PATH, --geom=PATH        Print all tiles intersecting with the (multi)linestrings and (multi)polygons in the specified file\n" \
    "  -n, --null                  Use NULL character, not LF as file delimiter.\n" \
    "  -s SUFFIX, --suffix=SUFFIX  suffix to append (do not forget the leading dot)\n" \
    "  -t, --tirex                 tirex mode (different output style, only coords that are multiples of 8)\n" \
    "  -z ZOOM, --minzoom=ZOOM     minimum zoom level, defaults to 0\n" \
    "  -Z ZOOM, --maxzoom=ZOOM     maximum zoom level, defaults to 14\n" \
    "  --buffer-size=SIZE          buffer size in meter for lines and polygons (not bounding boxes)\n" \
    "  -o FILE, --output=FILE      write output to file instead of standard output\n" \
    "  -v, --verbose               be verbose" << std::endl;
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"append", required_argument, 0, 'a'},
        {"bbox", required_argument, 0, 'b'},
        {"buffer-size", required_argument, 0, 'B'},
        {"check.exists", required_argument, 0, 'c'},
        {"directory", required_argument, 0, 'd'},
        {"geom", required_argument, 0, 'g'},
        {"minzoom", required_argument, 0, 'z'},
        {"maxzoom", required_argument, 0, 'Z'},
        {"null", no_argument, 0, 'n'},
        {"output", required_argument, 0, 'o'},
        {"suffix", required_argument, 0, 's'},
        {"tirex", no_argument, 0, 't'},
        {"help",  no_argument, 0, 'h'},
        {"verbose",  no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    bool verbose = false;
    bool tirex = false;
    char delimiter = '\n';
    int minzoom = 0;
    int maxzoom = 14;
    double buffer_size = 0.0;
    bool bbox_enabled = false;
    BoundingBox bbox {-180, -83, 180, 83};
    std::string shapefile_path;
    bool check_exists = false;
    std::string check_dir;
    FILE* output_file = stdout;
    std::string suffix;
    std::string append_str;

    char* rest;
    while (true) {
        int c = getopt_long(argc, argv, "a:B:b:cd:g:nz:Z:o:s:vht", long_options, 0);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'a':
            append_str = optarg;
            break;
        case 'B':
            buffer_size = strtod(optarg, &rest);
            break;
        case 'b':
            bbox = BoundingBox::from_str(optarg);
            bbox_enabled = true;
            break;
        case 'c':
            check_exists = true;
            break;
        case 'd':
            check_dir = optarg;
            break;
        case 'g':
            shapefile_path = optarg;
            break;
        case 'n':
            delimiter = '\0';
            break;
        case 's':
            suffix = optarg;
            if (suffix.empty()) {
                std::cerr << "ERROR: File name suffix is empty.\n";
                exit(1);
            } else if (suffix.at(0) != '.') {
                std::cerr << "WARNING: Suffix does not start with a dot.\n";
            }
            break;
        case 'z':
            minzoom = atoi(optarg);
            break;
        case 'Z':
            maxzoom = atoi(optarg);
            break;
        case 'o':
            output_file = fopen(optarg, "w");
            if (!output_file) {
                std::cerr << "ERROR: Failed to open output file " << optarg << '\n';
                exit(1);
            }
            break;
        case 'h':
            print_usage(argv);
            exit(1);
            break;
        case 't':
            tirex = true;
            break;
        case 'v':
            verbose = true;
            break;
        default:
            std::cerr << "ERROR: Wrong usage\n";
            print_usage(argv);
            exit(1);
        }
    }

    int remaining_args = argc - optind;
    if (remaining_args != 0) {
        std::cerr << "ERROR: Positional arguments remaining.\n";
        print_usage(argv);
        exit(1);
    }

    if (!bbox_enabled && shapefile_path.empty()) {
        std::cerr << "ERROR: Neither a bounding box nor a polygon was provided.\n";
        print_usage(argv);
        exit(1);
    }

    if (check_exists && suffix.empty()) {
        std::cerr << "WARNING: suffix is empty but checking tiles for existance is enabled.\n";
    }

    if (tirex) {
        maxzoom = (maxzoom>3) ? maxzoom -3 : 0;
        minzoom = (minzoom>3) ? minzoom -3 : 0;
    }

    if (bbox_enabled) {
        print_all_tiles_on_range(output_file, minzoom, maxzoom, bbox, suffix, delimiter, check_exists, check_dir, tirex);
    }

    if (!shapefile_path.empty()) {
        GDALIntersectingTilesFinder finder {verbose, static_cast<uint32_t>(minzoom),
            static_cast<uint32_t>(maxzoom), check_exists, tirex};
        finder.find_intersections(shapefile_path, buffer_size);
        if (verbose) {
            std::cerr << "dumping tiles on medium zoom levels\n";
        }
        finder.output(output_file, suffix, delimiter, check_dir);
    } // close scope to ensure that destructor of IntersectingTilesFinder is called now to free memory.
    if (!append_str.empty()) {
        fprintf(output_file, "%s%c", append_str.c_str(), delimiter);
    }
    if (output_file != stdout) {
        if (fclose(output_file) != 0) {
            std::cerr << "ERROR: closing output file failed\n";
            exit(1);
        }
    }
}
