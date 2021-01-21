#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace soci_wrapper {

template<class Type, class S = void>
struct cpp_to_db_type;

template<class CPPType>
struct cpp_to_db_type<CPPType, std::enable_if_t<std::is_integral_v<CPPType>>>
{
    inline static const std::string db_type = "INTEGER";
};

template<class CPPType>
struct cpp_to_db_type<CPPType, std::enable_if_t<std::is_same_v<CPPType, std::string>>>
{
    inline static const std::string db_type = "VARCHAR";
};

template<class Type, size_t N>
struct cpp_to_db_type<std::array<Type, N>>
{
    inline static const std::string db_type = "CHAR(" + std::to_string(N) + ")";
};

} // namespace soci_wrapper
