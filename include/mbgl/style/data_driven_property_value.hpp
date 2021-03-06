#pragma once

#include <mbgl/style/function/camera_function.hpp>
#include <mbgl/style/function/composite_function.hpp>
#include <mbgl/style/function/source_function.hpp>
#include <mbgl/style/undefined.hpp>
#include <mbgl/util/variant.hpp>

namespace mbgl {
namespace style {

template <class T>
class DataDrivenPropertyValue {
private:
    using Value = variant<Undefined, T, CameraFunction<T>, SourceFunction<T>, CompositeFunction<T>>;

    Value value;

    friend bool operator==(const DataDrivenPropertyValue& lhs, const DataDrivenPropertyValue& rhs) {
        return lhs.value == rhs.value;
    }

    friend bool operator!=(const DataDrivenPropertyValue& lhs, const DataDrivenPropertyValue& rhs) {
        return !(lhs == rhs);
    }

public:
    DataDrivenPropertyValue() = default;
    DataDrivenPropertyValue(T v) : value(std::move(v)) {
    }
    DataDrivenPropertyValue(CameraFunction<T> v) : value(std::move(v)) {
    }
    DataDrivenPropertyValue(SourceFunction<T> v) : value(std::move(v)) {
    }
    DataDrivenPropertyValue(CompositeFunction<T> v) : value(std::move(v)) {
    }

    bool isUndefined() const {
        return value.template is<Undefined>();
    }

    bool isDataDriven() const {
        return value.template is<SourceFunction<T>>() || value.template is<CompositeFunction<T>>();
    }

    bool isZoomConstant() const {
        return !value.template is<CameraFunction<T>>() &&
               !value.template is<CompositeFunction<T>>();
    }

    bool isExpression() const {
        return value.match([](const Undefined&) { return false; }, [](const T&) { return false; },
                           [](const CameraFunction<T>& fn) { return fn.isExpression; },
                           [](const SourceFunction<T>& fn) { return fn.isExpression; },
                           [](const CompositeFunction<T>& fn) { return fn.isExpression; });
    }

    template <class... Ts>
    auto match(Ts&&... ts) const {
        return value.match(std::forward<Ts>(ts)...);
    }

    template <typename Evaluator>
    auto evaluate(const Evaluator& evaluator, TimePoint = {}) const {
        return Value::visit(value, evaluator);
    }

    bool hasDataDrivenPropertyDifference(const DataDrivenPropertyValue<T>& other) const {
        return *this != other && (isDataDriven() || other.isDataDriven());
    }
};

} // namespace style
} // namespace mbgl
