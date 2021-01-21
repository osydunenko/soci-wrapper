#pragma once

#include <iostream>
#include <sstream>

#include <cassert>
#include <memory>
#include <type_traits>
#include <tuple>

#include <boost/preprocessor.hpp>

#include "soci/soci.h"
#include "types_convertor.hpp"

#ifdef SOCI_WRAPPER_SQLITE
#include "soci/sqlite3/soci-sqlite3.h"
#endif

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

template<class Type>
struct type_meta_data
{
    using self_type = type_meta_data<Type>;

    using is_declared = std::false_type;

    using tuple_type = std::tuple<void>;

    using tuple_type_pair = std::tuple<std::pair<void, void>>;

    using fields_number = std::integral_constant<size_t, 0>;

    static std::string_view class_name();

    static std::vector<std::string_view> member_names();

    static const tuple_type_pair &types_pair();
};

} // namespace detail

struct session
{
    using session_type = soci::session;
    using session_ptr_type = std::unique_ptr<session_type>;
    using on_error_type = std::function<void(const std::string_view)>;

    static session_ptr_type connect(const std::string &conn_string, on_error_type &&on_error = nullptr)
    {
        session_ptr_type session_ptr = std::make_unique<session_type>();
        try {
            session_ptr->open(
#ifdef SOCI_WRAPPER_SQLITE
                soci::sqlite3,
#endif
                conn_string);
        } catch(const std::exception &ex) {
            if (on_error) {
                on_error(ex.what());
            }
            return nullptr;
        }
        return session_ptr;
    }
};

template<class Type>
struct ddl
{
    static void create_table(session::session_type &session)
    {
        static_assert(detail::type_meta_data<Type>::is_declared::value);

        assert(detail::type_meta_data<Type>::fields_number::value == 
            detail::type_meta_data<Type>::member_names().size());

        std::stringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS \"" << detail::type_meta_data<Type>::class_name() << "\" (";

        std::vector<std::string> fields;
        detail::tuple_for_each(
            detail::type_meta_data<Type>::types_pair(),
            format_fields(fields)
        );

        for (size_t idx = 0; idx < fields.size(); ++idx) {
            if (idx != 0) {
                sql << ",";
            }
            sql << fields[idx];
        }

        sql << ")";

        std::cout << sql.str() << std::endl;
    }

private:
    struct format_fields
    {
        format_fields(std::vector<std::string> &vec)
            : m_vec(vec)
        {}

        template<class T>
        void operator()(const T &val)
        {
            using cpp_type = std::remove_pointer_t<decltype(val.first)>;

            std::stringstream str;
            str << val.second << " " << cpp_to_db_type<cpp_type>::db_type;
            m_vec.emplace_back(str.str());
        }

    private:
        std::vector<std::string> &m_vec;
    };
};

} // namespace soci_wrapper

#define EXPAND_MEMBERS_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(N, DATA))
#define EXPAND_MEMBERS(TUPLE) BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(TUPLE), EXPAND_MEMBERS_IDX, TUPLE)

#define EXPAND_MEMBERS_TYPE_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) std::decay_t<decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))>
#define EXPAND_MEMBERS_TYPE(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_TYPE_IDX, TUPLE)

#define EXPAND_MEMBERS_TYPE_PAIR_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) std::pair<std::add_pointer_t<std::decay_t<decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))>>, std::string>
#define EXPAND_MEMBERS_TYPE_PAIR(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_TYPE_PAIR_IDX, TUPLE)

#define EXPAND_MEMBERS_PAIR_ELEM_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) std::make_pair<std::add_pointer_t<std::decay_t<decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))>>, std::string>(nullptr, BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA)))
#define EXPAND_MEMBERS_PAIR_ELEM(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_PAIR_ELEM_IDX, TUPLE)

#define DECLARE_PERSISTENT_OBJECT(...) \
    namespace soci_wrapper { \
    namespace detail { \
        template<> \
        struct type_meta_data<BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))> \
        { \
            using self_type = type_meta_data<BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            using is_declared = std::true_type; \
            using tuple_type = std::tuple<EXPAND_MEMBERS_TYPE(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            using tuple_type_pair = std::tuple<EXPAND_MEMBERS_TYPE_PAIR(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            using fields_number = std::tuple_size<tuple_type>; \
            static std::string_view class_name() \
            { \
                static const std::string class_name = BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))); \
                return class_name; \
            } \
            static std::vector<std::string_view> member_names() \
            { \
                static const std::vector<std::string> names{EXPAND_MEMBERS(BOOST_PP_TUPLE_POP_FRONT(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)))}; \
                return std::vector<std::string_view>{names.begin(), names.end()}; \
            } \
            static const tuple_type_pair &types_pair() \
            { \
                static const tuple_type_pair pairs{EXPAND_MEMBERS_PAIR_ELEM(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))}; \
                return pairs; \
            } \
        }; \
    } \
    }

