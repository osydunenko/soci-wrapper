#pragma once

#include <iostream>
#include <sstream>

#include <cassert>
#include <memory>

#include <boost/preprocessor.hpp>

#include "soci/soci.h"
#include "types_convertor.hpp"
#include "configuration.hpp"
#include "meta_data.hpp"

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
    using self_type = ddl<Type>;

    using type_meta_data = detail::type_meta_data<Type>;

    using fields_type = typename type_meta_data::dsl_fields;

    using configuration_type = configuration<Type>;

    static_assert(type_meta_data::is_declared::value);

    static void create_table(session::session_type &session)
    {
        assert(type_meta_data::fields_number::value == 
            type_meta_data::member_names().size());
        
        std::stringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS \"" << type_meta_data::class_name() << "\" (";

        std::vector<std::string> fields;
        detail::tuple_for_each(
            type_meta_data::types_pair(),
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

    template<class ...Expr>
    static void create_table(session::session_type &session, const Expr &...expr)
    {
        [](...){}(
            (configuration_type::eval(expr), false)...
        );
        self_type::create_table(session);
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

            // process NOT NULL
            if (self_type::configuration_type::not_null().find(val.second) != 
                self_type::configuration_type::not_null().end()) {
                str << " NOT NULL";
            }

            // process UNIQUE
            if (self_type::configuration_type::unique().find(val.second) != 
                self_type::configuration_type::unique().end()) {
                str << " UNIQUE";
            }

            // process PRIMARY KEY
            if (self_type::configuration_type::primary_key().find(val.second) != 
                self_type::configuration_type::primary_key().end()) {
                str << " PRIMARY KEY";
            }

            // foreign key
            auto fk = self_type::configuration_type::foreign_key().find(val.second);
            if (fk != self_type::configuration_type::foreign_key().end()) {
                str << " FOREIGN KEY REFERENCES " << fk->second.first << "(" 
                    << fk->second.second << ")";
            }

            m_vec.emplace_back(str.str());
        }

    private:
        std::vector<std::string> &m_vec;
    };
};

template<class Type>
using fields_query = typename ddl<Type>::fields_type;

} // namespace soci_wrapper

