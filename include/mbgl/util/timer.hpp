#pragma once

#include <mbgl/util/chrono.hpp>
#include <mbgl/util/noncopyable.hpp>

#include <functional>
#include <memory>

namespace mbgl {
namespace util {

class Timer : private util::noncopyable {
public:
    Timer();
    ~Timer();

    void start(Duration timeout, Duration repeat, std::function<void()>&&);
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace util
} // namespace mbgl
