#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "soci/soci.h"

namespace soci_wrapper {
namespace detail {

template<class Type>
auto cpp_to_soci_type(Type&&) -> std::decay_t<Type>;

template<class Type, size_t N>
auto cpp_to_soci_type(std::array<Type, N>&&) -> std::string;

} // namespace detail

template<class Type, class S = void>
struct cpp_to_db_type;

template<>
struct cpp_to_db_type<std::string>
{
    inline static const std::string_view db_type = "VARCHAR";
};

template<class Type, size_t N>
struct cpp_to_db_type<std::array<Type, N>>
{
    inline static const std::string_view db_type = "CHAR(" + std::to_string(N) + ")";
};

template<class CPPType>
struct cpp_to_db_type<CPPType, std::enable_if_t<std::is_integral_v<CPPType>>>
{
    inline static const std::string_view db_type = "INTEGER";
};

template<class CPPType>
struct cpp_to_db_type<CPPType, std::enable_if_t<std::is_floating_point_v<CPPType>>>
{
    inline static const std::string_view db_type = "REAL";
};

template<class Type>
struct cpp_to_soci_type
{
    using type = decltype(detail::cpp_to_soci_type(std::declval<Type>()));
};

template<class Type>
using cpp_to_soci_type_t = typename cpp_to_soci_type<Type>::type;

template<class V>
struct to_ind
{
    static soci::indicator get_ind(const V &val)
    {
        return soci::i_ok;
    }
};

template<>
struct to_ind<std::string>
{
    static soci::indicator get_ind(const std::string &val)
    {
        if (val.size()) {
            return soci::i_ok;
        }
        return soci::i_null;
    }
};

} // namespace soci_wrapper
