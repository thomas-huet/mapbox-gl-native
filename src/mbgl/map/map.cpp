#include <mbgl/map/map.hpp>
#include <mbgl/map/camera.hpp>
#include <mbgl/map/transform.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/annotation/annotation_manager.hpp>
#include <mbgl/style/style_impl.hpp>
#include <mbgl/style/observer.hpp>
#include <mbgl/renderer/update_parameters.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/resource.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/math.hpp>
#include <mbgl/util/exception.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/util/tile_coordinate.hpp>
#include <mbgl/actor/scheduler.hpp>
#include <mbgl/util/logging.hpp>
#include <mbgl/math/log2.hpp>
#include <utility>

namespace mbgl {

using namespace style;

struct StillImageRequest {
    StillImageRequest(Map::StillImageCallback&& callback_)
        : callback(std::move(callback_)) {
    }

    Map::StillImageCallback callback;
};

class Map::Impl : public style::Observer,
                  public RendererObserver {
public:
    Impl(Map&,
         RendererFrontend&,
         float pixelRatio,
         FileSource&,
         Scheduler&,
         ConstrainMode,
         ViewportMode);

    ~Impl();

    // StyleObserver
    void onUpdate() override;
    void onStyleLoading() override;
    void onStyleLoaded() override;

    // RendererObserver
    void onInvalidate() override;
    void onResourceError(std::exception_ptr) override;
    void onDidFinishRenderingFrame(RenderMode) override;

    Map& map;
    RendererFrontend& rendererFrontend;
    FileSource& fileSource;
    Scheduler& scheduler;

    Transform transform;

    const float pixelRatio;

    MapDebugOptions debugOptions { MapDebugOptions::NoDebug };

    std::unique_ptr<Style> style;
    AnnotationManager annotationManager;

    bool cameraMutated = false;

    uint8_t prefetchZoomDelta = util::DEFAULT_PREFETCH_ZOOM_DELTA;

    bool loading = false;
    bool rendererFullyLoaded;
    std::unique_ptr<StillImageRequest> stillImageRequest;
};

Map::Map(RendererFrontend& rendererFrontend,
         const Size size,
         const float pixelRatio,
         FileSource& fileSource,
         Scheduler& scheduler,
         ConstrainMode constrainMode,
         ViewportMode viewportMode)
    : impl(std::make_unique<Impl>(*this,
                                  rendererFrontend,
                                  pixelRatio,
                                  fileSource,
                                  scheduler,
                                  constrainMode,
                                  viewportMode)) {
    impl->transform.resize(size);
}

Map::Impl::Impl(Map& map_,
                RendererFrontend& frontend,
                float pixelRatio_,
                FileSource& fileSource_,
                Scheduler& scheduler_,
                ConstrainMode constrainMode_,
                ViewportMode viewportMode_)
    : map(map_),
      rendererFrontend(frontend),
      fileSource(fileSource_),
      scheduler(scheduler_),
      transform(constrainMode_,
                viewportMode_),
      pixelRatio(pixelRatio_),
      style(std::make_unique<Style>(scheduler, fileSource, pixelRatio)),
      annotationManager(*style) {

    style->impl->setObserver(this);
    rendererFrontend.setObserver(*this);
}

Map::Impl::~Impl() {
    // Explicitly reset the RendererFrontend first to ensure it releases
    // All shared resources (AnnotationManager)
    rendererFrontend.reset();
};

Map::~Map() = default;

void Map::renderStill(StillImageCallback callback) {
    if (!callback) {
        Log::Error(Event::General, "StillImageCallback not set");
        return;
    }

    if (impl->stillImageRequest) {
        callback(std::make_exception_ptr(util::MisuseException("Map is currently rendering an image")));
        return;
    }

    if (impl->style->impl->getLastError()) {
        callback(impl->style->impl->getLastError());
        return;
    }

    impl->stillImageRequest = std::make_unique<StillImageRequest>(std::move(callback));

    impl->onUpdate();
}

void Map::renderStill(const CameraOptions& camera, MapDebugOptions debugOptions, StillImageCallback callback) {
    impl->cameraMutated = true;
    impl->debugOptions = debugOptions;
    impl->transform.jumpTo(camera);
    renderStill(std::move(callback));
}

void Map::triggerRepaint() {
    impl->onUpdate();
}

#pragma mark - Map::Impl RendererObserver

void Map::Impl::onDidFinishRenderingFrame(RenderMode renderMode) {
    rendererFullyLoaded = renderMode == RenderMode::Full;

    if (stillImageRequest && rendererFullyLoaded) {
        auto request = std::move(stillImageRequest);
        request->callback(nullptr);
    }
}

#pragma mark - Style

style::Style& Map::getStyle() {
    return *impl->style;
}

const style::Style& Map::getStyle() const {
    return *impl->style;
}

void Map::setStyle(std::unique_ptr<Style> style) {
    assert(style);
    impl->onStyleLoading();
    impl->style = std::move(style);
    impl->annotationManager.setStyle(*impl->style);
}

#pragma mark -

CameraOptions Map::getCameraOptions(const EdgeInsets& padding) const {
    return impl->transform.getCameraOptions(padding);
}

void Map::jumpTo(const CameraOptions& camera) {
    impl->cameraMutated = true;
    impl->transform.jumpTo(camera);
    impl->onUpdate();
}

#pragma mark - Position

void Map::moveBy(const ScreenCoordinate& point) {
    impl->cameraMutated = true;
    impl->transform.moveBy(point);
    impl->onUpdate();
}

void Map::setLatLng(const LatLng& latLng) {
    impl->cameraMutated = true;
    setLatLng(latLng);
}

LatLng Map::getLatLng(const EdgeInsets& padding) const {
    return impl->transform.getLatLng(padding);
}

void Map::resetPosition(const EdgeInsets& padding) {
    impl->cameraMutated = true;
    CameraOptions camera;
    camera.angle = 0;
    camera.pitch = 0;
    camera.center = LatLng(0, 0);
    camera.padding = padding;
    camera.zoom = 0;
    impl->transform.jumpTo(camera);
    impl->onUpdate();
}


#pragma mark - Zoom

void Map::setZoom(double zoom) {
    impl->cameraMutated = true;
    impl->transform.setZoom(zoom);
    impl->onUpdate();
}

double Map::getZoom() const {
    return impl->transform.getZoom();
}

void Map::setLatLngZoom(const LatLng& latLng, double zoom) {
    impl->cameraMutated = true;
    impl->transform.setLatLngZoom(latLng, zoom);
    impl->onUpdate();
}

CameraOptions Map::cameraForLatLngBounds(const LatLngBounds& bounds, const EdgeInsets& padding, optional<double> bearing) const {
    return cameraForLatLngs({
        bounds.northwest(),
        bounds.southwest(),
        bounds.southeast(),
        bounds.northeast(),
    }, padding, bearing);
}

CameraOptions cameraForLatLngs(const std::vector<LatLng>& latLngs, const Transform& transform, const EdgeInsets& padding) {
    CameraOptions options;
    if (latLngs.empty()) {
        return options;
    }
    Size size = transform.getState().getSize();
    // Calculate the bounds of the possibly rotated shape with respect to the viewport.
    ScreenCoordinate nePixel = {-INFINITY, -INFINITY};
    ScreenCoordinate swPixel = {INFINITY, INFINITY};
    double viewportHeight = size.height;
    for (LatLng latLng : latLngs) {
        ScreenCoordinate pixel = transform.latLngToScreenCoordinate(latLng);
        swPixel.x = std::min(swPixel.x, pixel.x);
        nePixel.x = std::max(nePixel.x, pixel.x);
        swPixel.y = std::min(swPixel.y, viewportHeight - pixel.y);
        nePixel.y = std::max(nePixel.y, viewportHeight - pixel.y);
    }
    double width = nePixel.x - swPixel.x;
    double height = nePixel.y - swPixel.y;

    // Calculate the zoom level.
    double minScale = INFINITY;
    if (width > 0 || height > 0) {
        double scaleX = double(size.width) / width;
        double scaleY = double(size.height) / height;
        scaleX -= (padding.left() + padding.right()) / width;
        scaleY -= (padding.top() + padding.bottom()) / height;
        minScale = util::min(scaleX, scaleY);
    }
    double zoom = transform.getZoom() + ::log2(minScale);
    zoom = util::clamp(zoom, transform.getState().getMinZoom(), transform.getState().getMaxZoom());

    // Calculate the center point of a virtual bounds that is extended in all directions by padding.
    ScreenCoordinate centerPixel = nePixel + swPixel;
    ScreenCoordinate paddedNEPixel = {
        padding.right() / minScale,
        padding.top() / minScale,
    };
    ScreenCoordinate paddedSWPixel = {
        padding.left() / minScale,
        padding.bottom() / minScale,
    };
    centerPixel = centerPixel + paddedNEPixel - paddedSWPixel;
    centerPixel /= 2.0;

    // CameraOptions origin is at the top-left corner.
    centerPixel.y = viewportHeight - centerPixel.y;

    options.center = transform.screenCoordinateToLatLng(centerPixel);
    options.zoom = zoom;
    return options;
}

CameraOptions Map::cameraForLatLngs(const std::vector<LatLng>& latLngs, const EdgeInsets& padding, optional<double> bearing) const {
    if(bearing) {
        double angle = -*bearing * util::DEG2RAD;  // Convert to radians
        Transform transform(impl->transform.getState());
        transform.setAngle(angle);
        CameraOptions options = mbgl::cameraForLatLngs(latLngs, transform, padding);
        options.angle = angle;
        return options;
    } else {
        return mbgl::cameraForLatLngs(latLngs, impl->transform, padding);
    }
}

CameraOptions Map::cameraForGeometry(const Geometry<double>& geometry, const EdgeInsets& padding, optional<double> bearing) const {

    std::vector<LatLng> latLngs;
    forEachPoint(geometry, [&](const Point<double>& pt) {
        latLngs.push_back({ pt.y, pt.x });
    });
    return cameraForLatLngs(latLngs, padding, bearing);
}

LatLngBounds Map::latLngBoundsForCamera(const CameraOptions& camera) const {
    Transform shallow { impl->transform.getState() };
    Size size = shallow.getState().getSize();

    shallow.jumpTo(camera);
    return LatLngBounds::hull(
        shallow.screenCoordinateToLatLng({}),
        shallow.screenCoordinateToLatLng({ double(size.width), double(size.height) })
    );
}

void Map::resetZoom() {
    impl->cameraMutated = true;
    setZoom(0);
}

#pragma mark - Bounds

optional<LatLngBounds> Map::getLatLngBounds() const {
    return impl->transform.getState().getLatLngBounds();
}

void Map::setLatLngBounds(optional<LatLngBounds> bounds) {
    impl->cameraMutated = true;
    impl->transform.setLatLngBounds(bounds);
    impl->onUpdate();
}

void Map::setMinZoom(const double minZoom) {
    impl->transform.setMinZoom(minZoom);
    if (getZoom() < minZoom) {
        setZoom(minZoom);
    }
}

double Map::getMinZoom() const {
    return impl->transform.getState().getMinZoom();
}

void Map::setMaxZoom(const double maxZoom) {
    impl->transform.setMaxZoom(maxZoom);
    if (getZoom() > maxZoom) {
        setZoom(maxZoom);
    }
}

double Map::getMaxZoom() const {
    return impl->transform.getState().getMaxZoom();
}

void Map::setMinPitch(double minPitch) {
    impl->transform.setMinPitch(minPitch * util::DEG2RAD);
    if (getPitch() < minPitch) {
        setPitch(minPitch);
    }
}

double Map::getMinPitch() const {
    return impl->transform.getState().getMinPitch() * util::RAD2DEG;
}

void Map::setMaxPitch(double maxPitch) {
    impl->transform.setMaxPitch(maxPitch * util::DEG2RAD);
    if (getPitch() > maxPitch) {
        setPitch(maxPitch);
    }
}

double Map::getMaxPitch() const {
    return impl->transform.getState().getMaxPitch() * util::RAD2DEG;
}

#pragma mark - Size

void Map::setSize(const Size size) {
    impl->transform.resize(size);
    impl->onUpdate();
}

Size Map::getSize() const {
    return impl->transform.getState().getSize();
}

#pragma mark - Rotation

void Map::rotateBy(const ScreenCoordinate& first, const ScreenCoordinate& second) {
    impl->cameraMutated = true;
    impl->transform.rotateBy(first, second);
    impl->onUpdate();
}

void Map::setBearing(double degrees) {
    impl->cameraMutated = true;
    impl->transform.setAngle(-degrees * util::DEG2RAD);
    impl->onUpdate();
}

double Map::getBearing() const {
    return -impl->transform.getAngle() * util::RAD2DEG;
}

void Map::resetNorth() {
    impl->cameraMutated = true;
    impl->transform.setAngle(0);
    impl->onUpdate();
}

#pragma mark - Pitch

void Map::setPitch(double pitch) {
    impl->cameraMutated = true;
    impl->transform.setPitch(pitch * util::DEG2RAD);
    impl->onUpdate();
}

double Map::getPitch() const {
    return impl->transform.getPitch() * util::RAD2DEG;
}

#pragma mark - North Orientation

void Map::setNorthOrientation(NorthOrientation orientation) {
    impl->transform.setNorthOrientation(orientation);
    impl->onUpdate();
}

NorthOrientation Map::getNorthOrientation() const {
    return impl->transform.getNorthOrientation();
}

#pragma mark - Constrain mode

void Map::setConstrainMode(mbgl::ConstrainMode mode) {
    impl->transform.setConstrainMode(mode);
    impl->onUpdate();
}

ConstrainMode Map::getConstrainMode() const {
    return impl->transform.getConstrainMode();
}

#pragma mark - Viewport mode

void Map::setViewportMode(mbgl::ViewportMode mode) {
    impl->transform.setViewportMode(mode);
    impl->onUpdate();
}

ViewportMode Map::getViewportMode() const {
    return impl->transform.getViewportMode();
}

#pragma mark - Projection mode

void Map::setAxonometric(bool axonometric) {
    impl->transform.setAxonometric(axonometric);
    impl->onUpdate();
}

bool Map::getAxonometric() const {
    return impl->transform.getAxonometric();
}

void Map::setXSkew(double xSkew) {
    impl->transform.setXSkew(xSkew);
    impl->onUpdate();
}

double Map::getXSkew() const {
    return impl->transform.getXSkew();
}

void Map::setYSkew(double ySkew) {
    impl->transform.setYSkew(ySkew);
    impl->onUpdate();
}

double Map::getYSkew() const {
    return impl->transform.getYSkew();
}

#pragma mark - Projection

ScreenCoordinate Map::pixelForLatLng(const LatLng& latLng) const {
    // If the center and point longitudes are not in the same side of the
    // antimeridian, we unwrap the point longitude so it would be seen if
    // e.g. the next antimeridian side is visible.
    LatLng unwrappedLatLng = latLng.wrapped();
    unwrappedLatLng.unwrapForShortestPath(getLatLng());
    return impl->transform.latLngToScreenCoordinate(unwrappedLatLng);
}

LatLng Map::latLngForPixel(const ScreenCoordinate& pixel) const {
    return impl->transform.screenCoordinateToLatLng(pixel);
}

#pragma mark - Annotations

void Map::addAnnotationImage(std::unique_ptr<style::Image> image) {
    impl->annotationManager.addImage(std::move(image));
}

void Map::removeAnnotationImage(const std::string& id) {
    impl->annotationManager.removeImage(id);
}

double Map::getTopOffsetPixelsForAnnotationImage(const std::string& id) {
    return impl->annotationManager.getTopOffsetPixelsForImage(id);
}

AnnotationID Map::addAnnotation(const Annotation& annotation) {
    auto result = impl->annotationManager.addAnnotation(annotation);
    impl->onUpdate();
    return result;
}

void Map::updateAnnotation(AnnotationID id, const Annotation& annotation) {
    if (impl->annotationManager.updateAnnotation(id, annotation)) {
        impl->onUpdate();
    }
}

void Map::removeAnnotation(AnnotationID annotation) {
    impl->annotationManager.removeAnnotation(annotation);
    impl->onUpdate();
}

#pragma mark - Toggles

void Map::setDebug(MapDebugOptions debugOptions) {
    impl->debugOptions = debugOptions;
    impl->onUpdate();
}

void Map::cycleDebugOptions() {
#if not MBGL_USE_GLES2
    if (impl->debugOptions & MapDebugOptions::StencilClip)
        impl->debugOptions = MapDebugOptions::NoDebug;
    else if (impl->debugOptions & MapDebugOptions::Overdraw)
        impl->debugOptions = MapDebugOptions::StencilClip;
#else
    if (impl->debugOptions & MapDebugOptions::Overdraw)
        impl->debugOptions = MapDebugOptions::NoDebug;
#endif // MBGL_USE_GLES2
    else if (impl->debugOptions & MapDebugOptions::Collision)
        impl->debugOptions = MapDebugOptions::Overdraw;
    else if (impl->debugOptions & MapDebugOptions::Timestamps)
        impl->debugOptions = impl->debugOptions | MapDebugOptions::Collision;
    else if (impl->debugOptions & MapDebugOptions::ParseStatus)
        impl->debugOptions = impl->debugOptions | MapDebugOptions::Timestamps;
    else if (impl->debugOptions & MapDebugOptions::TileBorders)
        impl->debugOptions = impl->debugOptions | MapDebugOptions::ParseStatus;
    else
        impl->debugOptions = MapDebugOptions::TileBorders;

    impl->onUpdate();
}

MapDebugOptions Map::getDebug() const {
    return impl->debugOptions;
}

void Map::setPrefetchZoomDelta(uint8_t delta) {
    impl->prefetchZoomDelta = delta;
}

uint8_t Map::getPrefetchZoomDelta() const {
    return impl->prefetchZoomDelta;
}

bool Map::isFullyLoaded() const {
    return impl->style->impl->isLoaded() && impl->rendererFullyLoaded;
}

void Map::Impl::onInvalidate() {
    onUpdate();
}

void Map::Impl::onUpdate() {
    // Don't load/render anything in still mode until explicitly requested.
    if (!stillImageRequest) {
        return;
    }
    
    TimePoint timePoint = Clock::time_point::max();

    UpdateParameters params = {
        style->impl->isLoaded(),
        pixelRatio,
        debugOptions,
        timePoint,
        transform.getState(),
        style->impl->getGlyphURL(),
        style->impl->spriteLoaded,
        style->impl->getTransitionOptions(),
        style->impl->getLight()->impl,
        style->impl->getImageImpls(),
        style->impl->getSourceImpls(),
        style->impl->getLayerImpls(),
        annotationManager,
        prefetchZoomDelta,
        bool(stillImageRequest)
    };

    rendererFrontend.update(std::make_shared<UpdateParameters>(std::move(params)));
}

void Map::Impl::onStyleLoading() {
    loading = true;
    rendererFullyLoaded = false;
}

void Map::Impl::onStyleLoaded() {
    if (!cameraMutated) {
        map.jumpTo(style->getDefaultCamera());
    }

    annotationManager.onStyleLoaded();
}

void Map::Impl::onResourceError(std::exception_ptr error) {
    if (stillImageRequest) {
        auto request = std::move(stillImageRequest);
        request->callback(error);
    }
}

void Map::dumpDebugLogs() const {
    Log::Info(Event::General, "--------------------------------------------------------------------------------");
    impl->style->impl->dumpDebugLogs();
    Log::Info(Event::General, "--------------------------------------------------------------------------------");
}

} // namespace mbgl
