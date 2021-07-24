#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "soci/soci.h"
#include "base/meta_data.hpp"
#include "base/utility.hpp"

namespace soci_wrapper {
namespace details {

template<class Type>
auto cpp_to_soci_type(Type&&) -> std::decay_t<Type>;

template<class Type, size_t N>
auto cpp_to_soci_type(std::array<Type, N>&&) -> std::string;

} // namespace details

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
    using type = decltype(details::cpp_to_soci_type(std::declval<Type>()));
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

namespace soci {

template<class Type>
struct type_conversion<
    Type,
    std::enable_if_t<
        soci_wrapper::details::type_meta_data<Type>::is_declared::value
    >
>
{
    using base_type = values;

    using object_type = Type;

    using type_meta_data = soci_wrapper::details::type_meta_data<object_type>;

    static void from_base(const base_type &v, indicator ind, Type &object)
    {
        soci_wrapper::base::tuple_for_each(
            type_meta_data::types_pair(),
            from_base_obj(v, ind, object)
        );
    }

    static void to_base(const Type &object, base_type &v, indicator &ind)
    {
        soci_wrapper::base::tuple_for_each(
            type_meta_data::types_pair(),
            to_base_obj(object, v, ind)
        );
    }
private:
    struct from_base_obj
    {
        from_base_obj(const base_type &v, indicator &i, Type &obj)
            : values(v)
            , ind(i)
            , object(obj)
        {
        }

        template<class T>
        void operator()(const T &val)
        {
            using cpp_type = std::remove_pointer_t<decltype(val.first)>;
            const size_t field_offset = type_meta_data::field_offset(val.second);

            cpp_type *value = reinterpret_cast<cpp_type *>(
                reinterpret_cast<size_t>(&object) + field_offset
            );

            using soci_type = soci_wrapper::cpp_to_soci_type_t<cpp_type>;
            *value = values.get<soci_type>(val.second, soci_type{});
        }

        const base_type &values;
        indicator &ind;
        Type &object;
    };

    struct to_base_obj
    {
        to_base_obj(const Type &obj, base_type &v, indicator &i)
            : object(obj)
            , values(v)
            , ind(i)
        {
        }

        template<class T>
        soci::indicator operator()(const T &val)
        {
            using cpp_type = std::remove_pointer_t<decltype(val.first)>;
            const size_t field_offset = type_meta_data::field_offset(val.second);

            cpp_type *value = reinterpret_cast<cpp_type *>(
                reinterpret_cast<size_t>(&object) + field_offset
            );

            const soci::indicator ind = soci_wrapper::to_ind<cpp_type>::get_ind(*value);
            values.set(val.second, *value, ind);

            return ind;
        }

        const Type &object;
        base_type &values;
        indicator &ind;
    };
};

} // namespace soci

