#pragma once

#include <mbgl/util/geo.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/unitbezier.hpp>
#include <mbgl/util/optional.hpp>

#include <functional>

namespace mbgl {

/** Various options for describing the viewpoint of a map. All fields are
    optional. */
struct CameraOptions {
    /** Coordinate at the center of the map. */
    optional<LatLng> center;

    /** Padding around the interior of the view that affects the frame of
        reference for `center`. */
    EdgeInsets padding;

    /** Zero-based zoom level. Constrained to the minimum and maximum zoom
        levels. */
    optional<double> zoom;

    /** Bearing, measured in radians counterclockwise from true north. Wrapped
        to [−π rad, π rad). */
    optional<double> angle;

    /** Pitch toward the horizon measured in radians, with 0 rad resulting in a
        two-dimensional map. */
    optional<double> pitch;
};

constexpr bool operator==(const CameraOptions& a, const CameraOptions& b) {
    return a.center == b.center
        && a.padding == b.padding
        && a.zoom == b.zoom
        && a.angle == b.angle
        && a.pitch == b.pitch;
}

constexpr bool operator!=(const CameraOptions& a, const CameraOptions& b) {
    return !(a == b);
}

} // namespace mbgl
