#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include <boost/algorithm/string/join.hpp>

namespace soci_wrapper {
namespace base {

template<class Tuple, class Func, std::size_t ...Idxs>
void tuple_at(std::size_t idx, const Tuple &tuple, Func &&func, std::index_sequence<Idxs...>)
{
    [](...){}(
        (idx == Idxs && (static_cast<void>(std::forward<Func>(func)(std::get<Idxs>(tuple))), false))...
    );
}

template<class Tuple, class Func>
void tuple_for_each(const Tuple &tuple, Func &&func)
{
    constexpr size_t size = std::tuple_size_v<Tuple>;
    for (size_t idx = 0; idx < size; ++idx) {
        tuple_at(idx, tuple, std::forward<Func>(func), std::make_index_sequence<size>{});
    }
}

template<template<class ...> class Cont, class T, class ...Args>
std::enable_if_t<std::is_same_v<std::string_view, T>, std::string>
join(const Cont<T, Args...> &cont, const std::string &sep = ",")
{
    std::vector<std::string> tmp_vec(cont.begin(), cont.end());
    return boost::algorithm::join(tmp_vec, sep);
}

template<template<class ...> class Cont, class T, class ...Args>
std::enable_if_t<std::is_same_v<std::string, T>, std::string>
join(const Cont<T, Args...> &cont, const std::string &sep = ",")
{
    return boost::algorithm::join(cont, sep);
}

} // namespace base
} // namespace soci_wrapper
