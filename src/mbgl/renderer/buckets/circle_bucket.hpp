#pragma once

#include <mbgl/gl/index_buffer.hpp>
#include <mbgl/gl/vertex_buffer.hpp>
#include <mbgl/programs/circle_program.hpp>
#include <mbgl/programs/segment.hpp>
#include <mbgl/renderer/bucket.hpp>
#include <mbgl/style/layers/circle_layer_properties.hpp>
#include <mbgl/tile/geometry_tile_data.hpp>

namespace mbgl {

class BucketParameters;

class CircleBucket : public Bucket {
public:
    CircleBucket(const BucketParameters&, const std::vector<const RenderLayer*>&);

    void addFeature(const GeometryTileFeature&, const GeometryCollection&) override;
    bool hasData() const override;

    void upload(gl::Context&) override;

    float getQueryRadius(const RenderLayer&) const override;

    gl::VertexVector<CircleLayoutVertex> vertices;
    gl::IndexVector<gl::Triangles> triangles;
    SegmentVector<CircleAttributes> segments;

    optional<gl::VertexBuffer<CircleLayoutVertex>> vertexBuffer;
    optional<gl::IndexBuffer<gl::Triangles>> indexBuffer;

    std::map<std::string, CircleProgram::PaintPropertyBinders> paintPropertyBinders;
};

} // namespace mbgl
