#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "base/meta_data.hpp"
#include "base/utility.hpp"
#include "soci/soci.h"

namespace soci_wrapper {
namespace details {

    template <unsigned... digits>
    struct to_chars {
        static constexpr char value[] = { ('0' + digits)..., 0 };
    };

    template <unsigned rem, unsigned... digits>
    struct comp_to_string : comp_to_string<rem / 10, rem % 10, digits...> {
    };

    template <unsigned... digits>
    struct comp_to_string<0, digits...> : to_chars<digits...> {
    };

    template <const std::string_view&... Strs>
    struct join_s {
        static constexpr auto impl() noexcept -> std::array<char, (Strs.size() + ...)>
        {
            constexpr size_t len = (Strs.size() + ...);
            std::array<char, len> arr {};

            auto append = [i = 0, &arr](const std::string_view& v) mutable {
                for (auto c : v) {
                    arr[i++] = c;
                }
            };

            (append(Strs), ...);

            return arr;
        }

        static constexpr decltype(impl()) arr = impl();

        static constexpr std::string_view value { arr.data(), arr.size() };
    };

    template <class Type>
    auto cpp_to_soci_type(Type&&) -> std::decay_t<Type>;

    template <class Type, size_t N>
    auto cpp_to_soci_type(std::array<Type, N>&&) -> std::string;

    enum class ARRAYS_TYPE {
        CHAR
    };

    template <ARRAYS_TYPE T, size_t Size>
    struct array_to_db_type;

    template <size_t Size>
    struct array_to_db_type<ARRAYS_TYPE::CHAR, Size> {
        static constexpr std::string_view type = "CHAR(";
        static constexpr std::string_view tail = ")";
        static constexpr std::string_view size = comp_to_string<Size>::value;
        static constexpr std::string_view value = join_s<type, size, tail>::value;
    };

    template <class>
    struct treat_as_array : std::false_type {
    };

    template <size_t N>
    struct treat_as_array<std::array<char, N>> : std::true_type {
    };

    template <size_t N>
    struct treat_as_array<char[N]> : std::true_type {
    };

    template <class T>
    constexpr bool treat_as_array_v = treat_as_array<T>::value;

    template <class T>
    concept integral = std::is_integral_v<T>;

    template <class T>
    concept floating_point = std::is_floating_point_v<T>;

} // namespace details

template <class...>
struct cpp_to_db_type;

template <>
struct cpp_to_db_type<std::string> {
    static constexpr std::string_view db_type = "VARCHAR";
};

template <class Type, size_t N>
requires std::same_as<Type, char>
struct cpp_to_db_type<Type[N]> {
    static constexpr std::string_view db_type = details::array_to_db_type<details::ARRAYS_TYPE::CHAR, N>::value;
};

template <class Type, size_t N>
requires std::same_as<Type, char>
struct cpp_to_db_type<std::array<Type, N>> {
    static constexpr std::string_view db_type = details::array_to_db_type<details::ARRAYS_TYPE::CHAR, N>::value;
};

template <class CPPType>
requires details::integral<CPPType>
struct cpp_to_db_type<CPPType> {
    static constexpr std::string_view db_type = "INTEGER";
};

template <class CPPType>
requires details::floating_point<CPPType>
struct cpp_to_db_type<CPPType> {
    static constexpr std::string_view db_type = "REAL";
};

template <class Type>
struct cpp_to_soci_type {
    using type = decltype(details::cpp_to_soci_type(std::declval<Type>()));
};

template <class Type>
using cpp_to_soci_type_t = typename cpp_to_soci_type<Type>::type;

template <class V>
struct to_ind {
    static soci::indicator get_ind(const V& val)
    {
        return soci::i_ok;
    }
};

template <>
struct to_ind<std::string> {
    static soci::indicator get_ind(const std::string& val)
    {
        if (val.size()) {
            return soci::i_ok;
        }
        return soci::i_null;
    }
};

} // namespace soci_wrapper

namespace soci {

template <class Type>
requires soci_wrapper::details::type_meta_data<Type>::is_declared::value struct type_conversion<Type> {
    using base_type = values;

    using object_type = Type;

    using type_meta_data = soci_wrapper::details::type_meta_data<object_type>;

    static void from_base(const base_type& v, indicator ind, Type& object)
    {
        soci_wrapper::base::tuple_for_each(
            type_meta_data::types_pair(),
            from_base_obj(v, ind, object));
    }

    static void to_base(const Type& object, base_type& v, indicator& ind)
    {
        soci_wrapper::base::tuple_for_each(
            type_meta_data::types_pair(),
            to_base_obj(object, v, ind));
    }

private:
    struct from_base_obj {
        from_base_obj(const base_type& v, indicator& i, Type& obj)
            : values(v)
            , ind(i)
            , object(obj)
        {
        }

        template <class T>
        void operator()(const T& val)
        {
            using cpp_type = std::remove_pointer_t<decltype(val.first)>;
            using soci_type = soci_wrapper::cpp_to_soci_type_t<cpp_type>;

            const size_t field_offset = type_meta_data::field_offset(val.second);

            cpp_type* value = reinterpret_cast<cpp_type*>(
                reinterpret_cast<size_t>(&object) + field_offset);

            auto&& src = values.get<soci_type>(val.second, soci_type {});
            if constexpr (!::soci_wrapper::details::treat_as_array_v<cpp_type>)
                *value = std::move(src);
            else
                std::copy(std::begin(src), std::end(src), std::begin(*value));
        }

        const base_type& values;
        indicator& ind;
        Type& object;
    };

    struct to_base_obj {
        to_base_obj(const Type& obj, base_type& v, indicator& i)
            : object(obj)
            , values(v)
            , ind(i)
        {
        }

        template <class T>
        soci::indicator operator()(const T& val)
        {
            using cpp_type = std::remove_pointer_t<decltype(val.first)>;
            const size_t field_offset = type_meta_data::field_offset(val.second);

            cpp_type* value = reinterpret_cast<cpp_type*>(
                reinterpret_cast<size_t>(&object) + field_offset);

            soci::indicator ind = soci_wrapper::to_ind<cpp_type>::get_ind(*value);
            if constexpr (!::soci_wrapper::details::treat_as_array_v<cpp_type>)
                values.set(val.second, *value, ind);
            else
                // Convert data to std::string, the most suitable dt what offers by SOCI
                // http://soci.sourceforge.net/doc/master/types/
                values.set(val.second, std::string(value->data()), ind);

            return ind;
        }

        const Type& object;
        base_type& values;
        indicator& ind;
    };
};

} // namespace soci
