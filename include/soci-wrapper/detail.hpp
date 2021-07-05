#pragma once

#include <cassert>
#include <cstddef>
#include <memory>

#include <boost/algorithm/string/join.hpp>

namespace soci_wrapper {
namespace detail {

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

template<class Cont>
std::string join(const Cont &cont, const std::string &sep = ",")
{
    return boost::algorithm::join(cont, sep);
}

} // namespace detail
} // namespace soci_wrapper
