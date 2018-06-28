#include <mbgl/util/dtoa.hpp>
#include <mbgl/util/string.hpp>

namespace mbgl {
namespace util {

std::string toString(float num) {
    return dtoa(num);
}

std::string toString(double num) {
    return dtoa(num);
}

std::string toString(long double num) {
    return dtoa(num);
}

} // namespace util
} // namespace mbgl
