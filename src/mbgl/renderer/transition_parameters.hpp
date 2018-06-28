#pragma once

#include <mbgl/style/transition_options.hpp>
#include <mbgl/util/chrono.hpp>

#include <vector>

namespace mbgl {

class TransitionParameters {
public:
    TimePoint now;
    style::TransitionOptions transition;
};

} // namespace mbgl
