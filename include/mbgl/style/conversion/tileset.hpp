#pragma once

#include <mbgl/style/conversion.hpp>
#include <mbgl/util/tileset.hpp>

namespace mbgl {
namespace style {
namespace conversion {

template <>
struct Converter<Tileset> {
public:
    optional<Tileset> operator()(const Convertible& value, Error& error) const;
};

} // namespace conversion
} // namespace style
} // namespace mbgl
