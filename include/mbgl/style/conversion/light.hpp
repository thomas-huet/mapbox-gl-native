#pragma once

#include <mbgl/style/conversion.hpp>
#include <mbgl/style/light.hpp>

namespace mbgl {
namespace style {
namespace conversion {

template <>
struct Converter<Light> {
public:
    optional<Light> operator()(const Convertible& value, Error& error) const;
};

} // namespace conversion
} // namespace style
} // namespace mbgl
