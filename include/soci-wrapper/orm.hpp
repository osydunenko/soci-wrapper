#pragma once

#include <iostream>

#include <memory>
#include <tuple>
#include <type_traits>
#include <boost/preprocessor.hpp>

#include "soci/soci.h"
#include "types_traits.hpp"

#ifdef SOCI_WRAPPER_SQLITE
#include "soci/sqlite3/soci-sqlite3.h"
#endif

namespace soci_wrapper {
namespace detail {

template<class Type>
struct type_meta_data
{
    using tuple_type = std::tuple<void>;
    static std::string_view class_name();
    static std::vector<std::string_view> member_names();
};

} // namespace detail

struct session
{
    using session_type = soci::session;
    using session_ptr_type = std::unique_ptr<session_type>;
    using on_error_type = std::function<void(const std::string_view)>;

    inline static session_ptr_type connect(const std::string &conn_string, on_error_type &&on_error = nullptr)
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
    static_assert(!std::is_same_v<typename detail::type_meta_data<Type>::tuple_type, std::tuple<void>>);

    inline static void create_table(session::session_type &session)
    {
        soci::transaction tx{session};
        std::cout << cpp_to_db_string<db_types::INTEGER>() << std::endl;
        tx.commit();
    }
};

} // namespace soci_wrapper

#define EXPAND_MEMBERS_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(N, DATA))
#define EXPAND_MEMBERS(TUPLE) BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(TUPLE), EXPAND_MEMBERS_IDX, TUPLE)

#define EXPAND_MEMBERS_TYPE_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) std::decay_t<decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))>
#define EXPAND_MEMBERS_TYPE(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_TYPE_IDX, TUPLE)

#define DECLARE_PERSISTENT_OBJECT(...) \
    namespace soci_wrapper { \
    namespace detail { \
        template<> \
        struct type_meta_data<BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))> \
        { \
            using type = BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)); \
            using tuple_type = std::tuple<EXPAND_MEMBERS_TYPE(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            inline static std::string_view class_name() \
            { \
                static std::string class_name = BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))); \
                return class_name; \
            } \
            inline static std::vector<std::string_view> member_names() \
            { \
                static const std::vector<std::string> names{EXPAND_MEMBERS(BOOST_PP_TUPLE_POP_FRONT(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)))}; \
                return std::vector<std::string_view>{names.begin(), names.end()}; \
            } \
        }; \
    } \
    } 

