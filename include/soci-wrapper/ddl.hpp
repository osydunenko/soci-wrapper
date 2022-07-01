#pragma once

#include "base/utility.hpp"
#include "configuration.hpp"
#include "session.hpp"
#include "types_convertor.hpp"

namespace soci_wrapper {

/*! \brief DDL -- Data Definition Language
 */
template <class Type>
struct ddl {
    using self_type = ddl<Type>;

    using type_meta_data = details::type_meta_data<Type>;

    using fields_type = typename type_meta_data::dsl_fields;

    using configuration_type = configuration<Type>;

    static_assert(type_meta_data::is_declared::value,
        "The concerned Type is not declared as a persistent type");

    static void drop_table(session::session_type& session)
    {
        std::stringstream sql;
        sql << "DROP TABLE IF EXISTS "
            << self_type::type_meta_data::table_name();
        session << sql.str();
    }

    static void create_table(session::session_type& session)
    {
        assert(self_type::type_meta_data::fields_number::value == self_type::type_meta_data::member_names().size());

        std::stringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS "
            << self_type::type_meta_data::table_name()
            << " (";

        std::vector<std::string> fields;
        base::tuple_for_each(
            self_type::type_meta_data::types_pair(),
            format_fields(fields));

        sql << base::join(fields);

        for (const auto& v : self_type::configuration_type::foreign_key()) {
            sql << ", FOREIGN KEY (" << v.first << ")"
                << " REFERENCES " << v.second.first << "(" << v.second.second << ")";
        }

        sql << ")";

        session << sql.str();
    }

    template <class... Expr>
    static void create_table(session::session_type& session, const Expr&... expr)
    {
        config(expr...);
        self_type::create_table(session);
    }

    template <class... Expr>
    static void config(const Expr&... expr)
    {
        // Processing the configuration grammar
        [](...) {}(
            (self_type::configuration_type::eval(expr), true)...);
    }

private:
    struct format_fields {
        format_fields(std::vector<std::string>& vec)
            : m_vec(vec)
        {
        }

        template <class T>
        void operator()(const T& val)
        {
            using cpp_type = std::remove_reference_t<std::remove_pointer_t<decltype(val.first)>>;

            std::stringstream str;
            str << val.second << " " << cpp_to_db_type<cpp_type>::db_type;

            // process NOT NULL
            if (self_type::configuration_type::not_null().contains(val.second)) {
                str << " NOT NULL";
            }

            // process UNIQUE
            if (self_type::configuration_type::unique().contains(val.second)) {
                str << " UNIQUE";
            }

            // process AUTOINCREMENT
            if (self_type::configuration_type::auto_increment().contains(val.second)) {
                str << " PRIMARY KEY AUTOINCREMENT";
            } else
                // process PRIMARY KEY
                if (self_type::configuration_type::primary_key().contains(val.second)) {
                    str << " PRIMARY KEY";
                }

            m_vec.emplace_back(str.str());
        }

    private:
        std::vector<std::string>& m_vec;
    };
};

template <class Type>
using fields_query = typename ddl<Type>::fields_type;

} // namespace soci_wrapper
