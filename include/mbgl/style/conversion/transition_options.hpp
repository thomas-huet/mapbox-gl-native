#pragma once

#include <mbgl/style/conversion.hpp>
#include <mbgl/style/transition_options.hpp>

namespace mbgl {
namespace style {
namespace conversion {

template <>
struct Converter<TransitionOptions> {
public:
    optional<TransitionOptions> operator()(const Convertible& value, Error& error) const;
};

} // namespace conversion
} // namespace style
} // namespace mbgl
