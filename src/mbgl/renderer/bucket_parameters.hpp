#pragma once

#include <mbgl/tile/tile_id.hpp>

namespace mbgl {

class BucketParameters {
public:
    const OverscaledTileID tileID;
    const float pixelRatio;
};

} // namespace mbgl
