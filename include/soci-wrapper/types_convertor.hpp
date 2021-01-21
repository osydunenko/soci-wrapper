#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace soci_wrapper {

template<class CPPType>
std::enable_if_t<
    std::is_integral_v<CPPType>,
    std::string_view
> cpp_to_db_type()
{
    static const std::string type = "INTEGER";
    return {type};
}

template<class CPPType>
std::enable_if_t<
    std::is_same_v<CPPType, std::string>,
    std::string_view
> cpp_to_db_type()
{
    static const std::string type = "VARCHAR";
    return {type};
}

} // namespace soci_wrapper
