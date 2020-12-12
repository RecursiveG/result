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
#include <sstream>

namespace result_internal {
template<typename E>
struct PureError {
    E e;
};
} // namespace result_internal

template<typename E> inline result_internal::PureError<E> Err(E&& e) {
    return result_internal::PureError<E>{std::forward<E>(e)};
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
    template<typename AnotherT, typename AnotherE>
    Result(Result<AnotherT,AnotherE>&& another) noexcept : values_(
        another.Ok() ?
            std::variant<T,E>{std::in_place_index<0>, std::move(another).TakeValue()} :
            std::variant<T,E>{std::in_place_index<1>, std::move(another).TakeError()}) {}
    Result(Result<T,E>&& another) noexcept : values_(std::move(another.values_)) {}

    // New result creation
    template<typename... Args>
    Result(Args&&... init_list) : values_(std::in_place_index<0>, std::forward<Args>(init_list)...) {}
    template<typename RawE>
    Result(result_internal::PureError<RawE>&& wrapped_e) :
        values_(std::in_place_index<1>, std::move(wrapped_e.e)) {}

    // Accessors
    [[nodiscard]] bool Ok() const {return values_.index() == 0;}
    [[nodiscard]] explicit operator bool() const {return Ok();}

    [[nodiscard]] T& Value() {return std::get<0>(values_);}
    [[nodiscard]] const T& Value() const {return std::get<0>(values_);}
    [[nodiscard]] T TakeValue() && {return std::get<0>(std::move(values_));}
    [[nodiscard]] T TakeValueOr(T alternative) && {
        if (Ok()) {
            return std::move(*this).TakeValue();
        } else {
            return alternative;
        }
    }
    T Expect(const std::string& msg) && {
        if (Ok()) {
            return std::move(*this).TakeValue();
        } else {
            if constexpr (std::is_same<E, std::string>::value) {
                throw std::runtime_error(msg + " (" + Error() + ")");
            } else {
                throw std::runtime_error(msg);
            }
        }
    }
    
    [[nodiscard]] E& Error() {return std::get<1>(values_);}
    [[nodiscard]] const E& Error() const {return std::get<1>(values_);}
    [[nodiscard]] E TakeError() && {
        return std::get<1>(std::move(values_));
    };

    // Equality Result-Result
    [[nodiscard]] friend bool operator==(const Result<T,E>& lhs, const Result<T,E>& rhs) {
        return lhs.values_ == rhs.values_;
    }
    [[nodiscard]] friend bool operator!=(const Result<T,E>& lhs, const Result<T,E>& rhs) {
        return lhs.values_ != rhs.values_;
    }
    // Equality Result<T,E>-AnotherT
    template<typename AnotherT>
    [[nodiscard]] friend bool operator==(const Result<T,E>& lhs, const AnotherT& rhs) {
        return lhs.Ok() && lhs.Value() == rhs;
    }
    template<typename AnotherT>
    [[nodiscard]] friend bool operator!=(const Result<T,E>& lhs, const AnotherT& rhs) {
        return !(lhs==rhs);
    }
    template<typename AnotherT>
    [[nodiscard]] friend bool operator==(const AnotherT& lhs, const Result<T,E>& rhs) {
        return rhs == lhs;
    }
    template<typename AnotherT>
    [[nodiscard]] friend bool operator!=(const AnotherT& lhs, const Result<T,E>& rhs) {
        return !(rhs == lhs);
    }
    // Equality Result<T,E>-AnotherE
    template<typename AnotherE>
    [[nodiscard]] friend bool operator==(const Result<T,E>& lhs, const result_internal::PureError<AnotherE>& rhs) {
        return !lhs.Ok() && lhs.Error() == rhs.e;
    }
    template<typename AnotherE>
    [[nodiscard]] friend bool operator!=(const Result<T,E>& lhs, const result_internal::PureError<AnotherE>& rhs) {
        return !(lhs==rhs);
    }
    template<typename AnotherE>
    [[nodiscard]] friend bool operator==(const result_internal::PureError<AnotherE>& lhs, const Result<T,E>& rhs) {
        return rhs == lhs;
    }
    template<typename AnotherE>
    [[nodiscard]] friend bool operator!=(const result_internal::PureError<AnotherE>& lhs, const Result<T,E>& rhs) {
        return !(rhs == lhs);
    }

    // functional stuff
    template <typename F, typename R = std::result_of_t<F(T)>>
    [[nodiscard]] Result<R, E> Fmap(const F& f) && {
        if (Ok()) {
            return f(std::move(*this).TakeValue());
        } else {
            return Err(std::move(*this).TakeError());
        }
    }

    template <typename F, typename R = std::result_of_t<F(T)>>
    // F must also return a Result<..., E>
    [[nodiscard]] R Bind(const F& f) && {
        if (Ok()) {
            return f(std::move(*this).TakeValue());
        } else {
            return Err(std::move(*this).TakeError());
        }
    }

private:
    template <typename AnotherT, typename AnotherE> friend class Result;
    std::variant<T,E> values_;
};



#define VALUE_OR_RAISE(result_expr)                     \
    ({                                                  \
        auto result_ = (result_expr);                   \
        if (!result_.Ok()) {                            \
            return Err(std::move(result_).TakeError()); \
        }                                               \
        std::move(result_).TakeValue();                 \
    })

#define NOT_NULL_OR_RAISE(ptr_expr, err_expr)   \
    ({                                          \
        auto maybe_nullptr_ = (ptr_expr);       \
        if (maybe_nullptr_ == nullptr) {        \
            return Err(err_expr);               \
        }                                       \
        maybe_nullptr_;                         \
    })

#define OPTIONAL_OR_RAISE(optional_expr, err_expr)  \
    ({                                              \
        auto maybe_ = (optional_expr);              \
        if (!maybe_) {                              \
            return Err(err_expr);                   \
        }                                           \
        ::std::move(*maybe_);                       \
    })

#define RAISE_ERRNO(msg)                        \
    do {                                        \
        int e = errno;                          \
        std::stringstream ss;                   \
        ss << msg << " (errno=" << e << ", "    \
           << std::strerror(e) << ")";          \
        return Err(ss.str());                   \
    } while(false)

#define ASSIGN_OR_RAISE(var, result_expr) \
    var = VALUE_OR_RAISE(result_expr)
