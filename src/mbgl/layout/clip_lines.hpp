#pragma once

#include <map>
#include <mbgl/tile/geometry_tile_data.hpp>
#include <string>
#include <vector>

namespace mbgl {
namespace util {

GeometryCollection clipLines(const GeometryCollection& lines,
                             const int16_t x1,
                             const int16_t y1,
                             const int16_t x2,
                             const int16_t y2);

} // end namespace util
} // end namespace mbgl
