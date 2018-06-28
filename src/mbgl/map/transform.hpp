#pragma once

#include <mbgl/map/camera.hpp>
#include <mbgl/map/mode.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/optional.hpp>

#include <cmath>
#include <cstdint>
#include <functional>

namespace mbgl {

class Transform : private util::noncopyable {
public:
    Transform(ConstrainMode = ConstrainMode::HeightOnly, ViewportMode = ViewportMode::Default);

    Transform(const TransformState& state_) : state(state_) {
    }

    // Map view
    void resize(Size size);

    // Camera
    /** Returns the current camera options. */
    CameraOptions getCameraOptions(const EdgeInsets&) const;

    /** Instantaneously, synchronously applies the given camera options. */
    void jumpTo(const CameraOptions&);

    // Position

    /** Pans the map by the given amount.
        @param offset The distance to pan the map by, measured in pixels from
            top to bottom and from left to right. */
    void moveBy(const ScreenCoordinate& offset);
    void setLatLng(const LatLng&);
    void setLatLng(const LatLng&, const EdgeInsets&);
    void setLatLng(const LatLng&, optional<ScreenCoordinate>);
    void setLatLngZoom(const LatLng&, double zoom);
    void setLatLngZoom(const LatLng&, double zoom, const EdgeInsets&);
    LatLng getLatLng(const EdgeInsets& = {}) const;
    ScreenCoordinate getScreenCoordinate(const EdgeInsets& = {}) const;

    // Bounds

    void setLatLngBounds(optional<LatLngBounds>);
    void setMinZoom(double);
    void setMaxZoom(double);
    void setMinPitch(double);
    void setMaxPitch(double);

    // Zoom

    /** Sets the zoom level, keeping the given point fixed within the view.
        @param zoom The new zoom level. */
    void setZoom(double zoom);
    /** Returns the zoom level. */
    double getZoom() const;

    // Angle

    void rotateBy(const ScreenCoordinate& first, const ScreenCoordinate& second);
    /** Sets the angle of rotation.
        @param angle The new angle of rotation, measured in radians
            counterclockwise from true north. */
    void setAngle(double angle);
    /** Returns the angle of rotation.
        @return The angle of rotation, measured in radians counterclockwise from
            true north. */
    double getAngle() const;

    // Pitch
    /** Sets the pitch angle.
        @param angle The new pitch angle, measured in radians toward the
            horizon. */
    void setPitch(double pitch);
    double getPitch() const;

    // North Orientation
    void setNorthOrientation(NorthOrientation);
    NorthOrientation getNorthOrientation() const;

    // Constrain mode
    void setConstrainMode(ConstrainMode);
    ConstrainMode getConstrainMode() const;

    // Viewport mode
    void setViewportMode(ViewportMode);
    ViewportMode getViewportMode() const;

    // Projection mode
    void setAxonometric(bool);
    bool getAxonometric() const;
    void setXSkew(double xSkew);
    double getXSkew() const;
    void setYSkew(double ySkew);
    double getYSkew() const;

    // Transform state
    const TransformState& getState() const {
        return state;
    }

    // Conversion and projection
    ScreenCoordinate latLngToScreenCoordinate(const LatLng&) const;
    LatLng screenCoordinateToLatLng(const ScreenCoordinate&) const;

private:
    TransformState state;
};

} // namespace mbgl
