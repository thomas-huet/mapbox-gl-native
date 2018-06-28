#pragma once

#include <mbgl/style/conversion.hpp>
#include <mbgl/style/filter.hpp>

namespace mbgl {
namespace style {
namespace conversion {

template <>
struct Converter<Filter> {
public:
    optional<Filter> operator()(const Convertible& value, Error& error) const;
};

} // namespace conversion
} // namespace style
} // namespace mbgl
