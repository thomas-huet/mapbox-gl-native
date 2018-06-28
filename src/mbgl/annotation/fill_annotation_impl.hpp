#pragma once

#include <mbgl/annotation/annotation.hpp>
#include <mbgl/annotation/shape_annotation_impl.hpp>

namespace mbgl {

class FillAnnotationImpl : public ShapeAnnotationImpl {
public:
    FillAnnotationImpl(AnnotationID, FillAnnotation);

    void updateStyle(style::Style::Impl&) const final;
    const ShapeAnnotationGeometry& geometry() const final;

private:
    const FillAnnotation annotation;
};

} // namespace mbgl
