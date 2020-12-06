#pragma once

#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <string_view>
#include <variant>

namespace result_internal {
template<typename E>
struct PureError {
    E e;
};
} // namespace result_internal

template<typename E> inline result_internal::PureError<E> Err(E e) {
    return result_internal::PureError<E>{e};
}

using ResultVoid = std::monostate;

template<typename T, typename E>
class Result {
public:
    // No copy or assignment
    Result(const Result<T,E>&) = delete;
    Result& operator=(const Result<T,E>&) = delete;
    Result& operator=(Result<T,E>&&) = delete;
    // Allows move constructor
    Result(Result<T,E>&& another) noexcept : values_(std::move(another.values_)) {}

    // New result creation
    template<typename RawT>
    Result(RawT&& t) : values_(std::in_place_index<0>, std::forward<RawT>(t)) {}
    template<typename RawE>
    Result(result_internal::PureError<RawE>&& wrapped_e) :
        values_(std::in_place_index<1>, std::move(wrapped_e.e)) {}

    // Accessors
    [[nodiscard]] bool Ok() const {return values_.index() == 0;}
    [[nodiscard]] explicit operator bool() const {return Ok();}

    [[nodiscard]] T& Value() {return std::get<0>(values_);}
    [[nodiscard]] const T& Value() const {return std::get<0>(values_);}
    [[nodiscard]] T TakeValue() && {return std::get<0>(std::move(values_));}
    
    [[nodiscard]] E& Err() {return std::get<1>(values_);}
    [[nodiscard]] const E& Err() const {return std::get<1>(values_);}
    [[nodiscard]] result_internal::PureError<E> TakeErr() && {
        return {std::get<1>(std::move(values_))};
    };

    
private:
    template <typename AnotherT, typename AnotherE> friend class Result;
    std::variant<T,E> values_;
};


#define VALUE_OR_RAISE(result_expr)                \
    ({                                             \
        auto result_ = (result_expr);              \
        if (!result_.Ok()) {                       \
            return ::std::move(result_).TakeErr(); \
        }                                          \
        ::std::move(result_).TakeValue();          \
    })

#define ASSIGN_OR_RAISE(var, result_expr) \
    var = VALUE_OR_RAISE(result_expr)
