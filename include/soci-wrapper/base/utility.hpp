#pragma once

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptors.hpp>
#include <concepts>

namespace soci_wrapper {
namespace base {

    template <typename T, typename... A>
    concept type_in = (std::same_as<std::remove_cvref_t<T>, A> or ...);

    template <class Tuple, class Func, std::size_t... Idxs>
    void tuple_at(std::size_t idx, const Tuple& tuple, Func&& func, std::index_sequence<Idxs...>)
    {
        [](...) {}(
            (idx == Idxs && (static_cast<void>(std::forward<Func>(func)(std::get<Idxs>(tuple))), false))...);
    }

    template <class Tuple, class Func>
    void tuple_for_each(const Tuple& tuple, Func&& func)
    {
        constexpr size_t size = std::tuple_size_v<Tuple>;
        for (size_t idx = 0; idx < size; ++idx) {
            tuple_at(idx, tuple, std::forward<Func>(func), std::make_index_sequence<size> {});
        }
    }

    template <template <class...> class Cont, class T, class... Args>
    requires type_in<T, std::string_view>
        std::string join(const Cont<T, Args...>& cont, const std::string& sep = ",")
    {
        return boost::algorithm::join(
            cont | boost::adaptors::transformed([](auto v) {
                return std::string { v };
            }),
            sep);
    }

    template <template <class...> class Cont, class T, class... Args>
    requires(not type_in<T, std::string_view>)
        std::string join(const Cont<T, Args...>& cont, const std::string& sep = ",")
    {
        return boost::algorithm::join(cont, sep);
    }

} // namespace base
} // namespace soci_wrapper
