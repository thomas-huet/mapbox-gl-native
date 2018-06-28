#include <mbgl/map/camera.hpp>
#include <mbgl/map/transform.hpp>
#include <mbgl/math/clamp.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/interpolate.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/util/mat4.hpp>
#include <mbgl/util/math.hpp>
#include <mbgl/util/platform.hpp>
#include <mbgl/util/projection.hpp>
#include <mbgl/util/unitbezier.hpp>

#include <cmath>
#include <cstdio>

namespace mbgl {

/** Converts the given angle (in radians) to be numerically close to the anchor angle, allowing it
 * to be interpolated properly without sudden jumps. */
static double _normalizeAngle(double angle, double anchorAngle) {
    if (std::isnan(angle) || std::isnan(anchorAngle)) {
        return 0;
    }

    angle = util::wrap(angle, -M_PI, M_PI);
    if (angle == -M_PI)
        angle = M_PI;
    double diff = std::abs(angle - anchorAngle);
    if (std::abs(angle - util::M2PI - anchorAngle) < diff) {
        angle -= util::M2PI;
    }
    if (std::abs(angle + util::M2PI - anchorAngle) < diff) {
        angle += util::M2PI;
    }

    return angle;
}

Transform::Transform(ConstrainMode constrainMode, ViewportMode viewportMode)
    : state(constrainMode, viewportMode) {
}

#pragma mark - Map View

void Transform::resize(const Size size) {
    if (size.isEmpty()) {
        throw std::runtime_error("failed to resize: size is empty");
    }

    if (state.size == size) {
        return;
    }

    state.size = size;
    state.constrain(state.scale, state.x, state.y);
}

#pragma mark - Camera

CameraOptions Transform::getCameraOptions(const EdgeInsets& padding) const {
    CameraOptions camera;
    camera.center = getLatLng(padding);
    camera.padding = padding;
    camera.zoom = getZoom();
    camera.angle = getAngle();
    camera.pitch = getPitch();
    return camera;
}

/**
 * Change any combination of center, zoom, bearing, and pitch, without
 * a transition. The map will retain the current values for any options
 * not included in `options`.
 */
void Transform::jumpTo(const CameraOptions& camera) {
    const LatLng unwrappedLatLng = camera.center.value_or(getLatLng());
    const LatLng latLng = unwrappedLatLng.wrapped();
    double zoom = camera.zoom.value_or(getZoom());
    double angle = camera.angle.value_or(getAngle());
    double pitch = camera.pitch.value_or(getPitch());

    if (std::isnan(zoom)) {
        return;
    }

    EdgeInsets padding = camera.padding;

    ScreenCoordinate center = getScreenCoordinate(padding);
    center.y = state.size.height - center.y;

    // Constrain camera options.
    zoom = util::clamp(zoom, state.getMinZoom(), state.getMaxZoom());
    const double scale = state.zoomScale(zoom);
    pitch = util::clamp(pitch, state.min_pitch, state.max_pitch);

    // Minimize rotation by taking the shorter path around the circle.
    angle = _normalizeAngle(angle, state.angle);
    state.angle = _normalizeAngle(state.angle, angle);

    state.setLatLngZoom(latLng, state.scaleZoom(scale));
    state.angle = util::wrap(angle, -M_PI, M_PI);
    state.pitch = pitch;
    if (!padding.isFlush()) {
        state.moveLatLng(latLng, center);
    }
}

#pragma mark - Position

void Transform::moveBy(const ScreenCoordinate& offset) {
    ScreenCoordinate centerOffset = {
        offset.x, -offset.y,
    };
    ScreenCoordinate centerPoint = getScreenCoordinate() - centerOffset;

    CameraOptions camera;
    camera.center = state.screenCoordinateToLatLng(centerPoint);
    jumpTo(camera);
}

void Transform::setLatLng(const LatLng& latLng) {
    setLatLng(latLng, optional<ScreenCoordinate>{});
}

void Transform::setLatLng(const LatLng& latLng, const EdgeInsets& padding) {
    CameraOptions camera;
    camera.center = latLng;
    camera.padding = padding;
    jumpTo(camera);
}

void Transform::setLatLng(const LatLng& latLng, optional<ScreenCoordinate> anchor) {
    CameraOptions camera;
    camera.center = latLng;
    if (anchor) {
        camera.padding = EdgeInsets(anchor->y, anchor->x, state.size.height - anchor->y,
                                    state.size.width - anchor->x);
    }
    jumpTo(camera);
}

void Transform::setLatLngZoom(const LatLng& latLng, double zoom) {
    setLatLngZoom(latLng, zoom, EdgeInsets());
}

void Transform::setLatLngZoom(const LatLng& latLng, double zoom, const EdgeInsets& padding) {
    if (std::isnan(zoom))
        return;

    CameraOptions camera;
    camera.center = latLng;
    camera.padding = padding;
    camera.zoom = zoom;
    jumpTo(camera);
}

LatLng Transform::getLatLng(const EdgeInsets& padding) const {
    if (padding.isFlush()) {
        return state.getLatLng();
    } else {
        return screenCoordinateToLatLng(padding.getCenter(state.size.width, state.size.height));
    }
}

ScreenCoordinate Transform::getScreenCoordinate(const EdgeInsets& padding) const {
    if (padding.isFlush()) {
        return { state.size.width / 2., state.size.height / 2. };
    } else {
        return padding.getCenter(state.size.width, state.size.height);
    }
}

#pragma mark - Zoom

void Transform::setZoom(double zoom) {
    CameraOptions camera;
    camera.zoom = zoom;
    jumpTo(camera);
}

double Transform::getZoom() const {
    return state.getZoom();
}

#pragma mark - Bounds

void Transform::setLatLngBounds(optional<LatLngBounds> bounds) {
    if (bounds && !bounds->valid()) {
        throw std::runtime_error("failed to set bounds: bounds are invalid");
    }
    state.setLatLngBounds(bounds);
}

void Transform::setMinZoom(const double minZoom) {
    if (std::isnan(minZoom))
        return;
    state.setMinZoom(minZoom);
}

void Transform::setMaxZoom(const double maxZoom) {
    if (std::isnan(maxZoom))
        return;
    state.setMaxZoom(maxZoom);
}

void Transform::setMinPitch(double minPitch) {
    if (std::isnan(minPitch))
        return;
    state.setMinPitch(minPitch);
}

void Transform::setMaxPitch(double maxPitch) {
    if (std::isnan(maxPitch))
        return;
    state.setMaxPitch(maxPitch);
}

#pragma mark - Angle

void Transform::rotateBy(const ScreenCoordinate& first, const ScreenCoordinate& second) {
    ScreenCoordinate center = getScreenCoordinate();
    const ScreenCoordinate offset = first - center;
    const double distance = std::sqrt(std::pow(2, offset.x) + std::pow(2, offset.y));

    // If the first click was too close to the center, move the center of rotation by 200 pixels
    // in the direction of the click.
    if (distance < 200) {
        const double heightOffset = -200;
        const double rotateAngle = std::atan2(offset.y, offset.x);
        center.x = first.x + std::cos(rotateAngle) * heightOffset;
        center.y = first.y + std::sin(rotateAngle) * heightOffset;
    }

    CameraOptions camera;
    camera.angle = state.angle + util::angle_between(first - center, second - center);
    jumpTo(camera);
}

void Transform::setAngle(double angle) {
    if (std::isnan(angle))
        return;
    CameraOptions camera;
    camera.angle = angle;
    jumpTo(camera);
}

double Transform::getAngle() const {
    return state.angle;
}

#pragma mark - Pitch

void Transform::setPitch(double pitch) {
    if (std::isnan(pitch))
        return;
    CameraOptions camera;
    camera.pitch = pitch;
    jumpTo(camera);
}

double Transform::getPitch() const {
    return state.pitch;
}

#pragma mark - North Orientation

void Transform::setNorthOrientation(NorthOrientation orientation) {
    state.orientation = orientation;
    state.constrain(state.scale, state.x, state.y);
}

NorthOrientation Transform::getNorthOrientation() const {
    return state.getNorthOrientation();
}

#pragma mark - Constrain mode

void Transform::setConstrainMode(mbgl::ConstrainMode mode) {
    state.constrainMode = mode;
    state.constrain(state.scale, state.x, state.y);
}

ConstrainMode Transform::getConstrainMode() const {
    return state.getConstrainMode();
}

#pragma mark - Viewport mode

void Transform::setViewportMode(mbgl::ViewportMode mode) {
    state.viewportMode = mode;
}

ViewportMode Transform::getViewportMode() const {
    return state.getViewportMode();
}

#pragma mark - Projection mode

void Transform::setAxonometric(bool axonometric) {
    state.axonometric = axonometric;
}

bool Transform::getAxonometric() const {
    return state.axonometric;
}

void Transform::setXSkew(double xSkew) {
    state.xSkew = xSkew;
}

double Transform::getXSkew() const {
    return state.xSkew;
}

void Transform::setYSkew(double ySkew) {
    state.ySkew = ySkew;
}

double Transform::getYSkew() const {
    return state.ySkew;
}

#pragma mark Conversion and projection

ScreenCoordinate Transform::latLngToScreenCoordinate(const LatLng& latLng) const {
    ScreenCoordinate point = state.latLngToScreenCoordinate(latLng);
    point.y = state.size.height - point.y;
    return point;
}

LatLng Transform::screenCoordinateToLatLng(const ScreenCoordinate& point) const {
    ScreenCoordinate flippedPoint = point;
    flippedPoint.y = state.size.height - flippedPoint.y;
    return state.screenCoordinateToLatLng(flippedPoint).wrapped();
}

} // namespace mbgl
