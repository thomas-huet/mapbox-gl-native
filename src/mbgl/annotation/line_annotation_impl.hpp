#pragma once

#include <mbgl/annotation/annotation.hpp>
#include <mbgl/annotation/shape_annotation_impl.hpp>

namespace mbgl {

class LineAnnotationImpl : public ShapeAnnotationImpl {
public:
    LineAnnotationImpl(AnnotationID, LineAnnotation);

    void updateStyle(style::Style::Impl&) const final;
    const ShapeAnnotationGeometry& geometry() const final;

private:
    const LineAnnotation annotation;
};

} // namespace mbgl
