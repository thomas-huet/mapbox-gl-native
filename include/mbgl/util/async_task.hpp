#pragma once

#include <mbgl/util/noncopyable.hpp>

#include <functional>
#include <memory>

namespace mbgl {
namespace util {

class AsyncTask : private util::noncopyable {
public:
    AsyncTask(std::function<void()>&&);
    ~AsyncTask();

    void send();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace util
} // namespace mbgl
