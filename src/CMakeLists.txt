#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(polygon-to-tile-list polygon-to-tile-list.cpp gdal_intersecting_tiles_finder.cpp tile_list.cpp utils.cpp)
target_link_libraries(polygon-to-tile-list ${GDAL_LIBRARIES} ${GEOS_LIBRARY} ${FAST_CPP_CSV_PARSER_LINK_FLAGS})
install(TARGETS polygon-to-tile-list DESTINATION bin)

