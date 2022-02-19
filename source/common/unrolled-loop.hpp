#pragma once

#include <type_traits>
#include <utility>

namespace detail
{
    template<class T, T... inds, class F>
    constexpr void unrolledLoop(std::integer_sequence<T, inds...>, F&& f)
    {
        (f(std::integral_constant<T, inds>{}), ...);
    }
}

template<class T, T count, class F>
constexpr void unrolledLoop(F&& f)
{
    detail::unrolledLoop(std::make_integer_sequence<T, count>{}, std::forward<F>(f));
}