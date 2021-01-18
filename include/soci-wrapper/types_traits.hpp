#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace soci_wrapper {

enum class db_types:std::uint32_t
{
    UNKNOWN = 0,
    INTEGER,
    REAL,
    NUMERIC,
    TEXT
};

template<class Type>
inline constexpr db_types cpp_to_db_type()
{
    return db_types::UNKNOWN;
}

template<>
inline constexpr db_types cpp_to_db_type<std::string>()
{
    return db_types::TEXT;
}

template<db_types Type = db_types::UNKNOWN>
inline std::string_view cpp_to_db_string()
{
    static const std::string db_string = "UNKNOWN";
    return {db_string};
}

template<>
inline std::string_view cpp_to_db_string<db_types::INTEGER>()
{
    static const std::string db_string = "INTEGER";
    return {db_string};
}

template<>
inline std::string_view cpp_to_db_string<db_types::REAL>()
{
    static const std::string db_string = "REAL";
    return {db_string};
}

template<>
inline std::string_view cpp_to_db_string<db_types::NUMERIC>()
{
    static const std::string db_string = "NUMERIC";
    return {db_string};
}

template<>
inline std::string_view cpp_to_db_string<db_types::TEXT>()
{
    static const std::string db_string = "VARCHAR";
    return {db_string};
}

} // namespace soci_wrapper
