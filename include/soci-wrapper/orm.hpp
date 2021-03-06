#pragma once

#include <iostream>
#include <sstream>

#include <cassert>
#include <cstddef>
#include <memory>

#include <range/v3/all.hpp>

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

template<class Cont>
std::string join(const Cont &cont, const std::string &sep = ",")
{
    return cont | ranges::views::join(sep) | ranges::to<std::string>();
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

struct dml
{
    template<class ...Type>
    static void persist(session::session_type &session, Type &&...objects)
    {
        detail::tuple_for_each(
            std::make_tuple(std::forward<Type>(objects)...),
            handle(session)
        );
    }

private:
    struct handle
    {
        handle(session::session_type &session)
            : sql_session(session)
        {
        }

        template<class Type>
        void operator()(Type *object)
        {
            assert(object != nullptr);
            this->operator()(*object);
        }

        template<class Type>
        void operator()(Type &&object)
        {
            using decayed_type = std::decay_t<Type>;
            using type_meta_data = detail::type_meta_data<decayed_type>;

            static_assert(type_meta_data::is_declared::value,
                    "The object being persisted was not declared");

            auto fields = type_meta_data::member_names();
            std::vector<std::string> sql_placeholders;

            std::transform(std::begin(fields), std::end(fields),
                std::back_inserter(sql_placeholders), 
                [](auto field){
                    std::stringstream str;
                    str << ":" << field;
                    return str.str();
                });

            std::stringstream sql;
            sql << "INSERT INTO " << type_meta_data::class_name() << " ("
                << detail::join(fields)
                << ") VALUES ("
                << detail::join(sql_placeholders)
                << ")";

            sql_session << sql.str(), soci::use(std::forward<Type>(object));
        }

        session::session_type &sql_session;
    };
};

template<class Type>
struct ddl
{
    using self_type = ddl<Type>;

    using type_meta_data = detail::type_meta_data<Type>;

    using fields_type = typename type_meta_data::dsl_fields;

    using configuration_type = configuration<Type>;

    static_assert(type_meta_data::is_declared::value);

    static void drop_table(session::session_type &session)
    {
        std::stringstream sql;
        sql << "DROP TABLE IF EXISTS "
            << self_type::type_meta_data::class_name();
        session << sql.str();
    }

    static void create_table(session::session_type &session)
    {
        assert(self_type::type_meta_data::fields_number::value == 
            self_type::type_meta_data::member_names().size());
        
        std::stringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS " 
            << self_type::type_meta_data::class_name()
            << " (";

        std::vector<std::string> fields;
        detail::tuple_for_each(
            self_type::type_meta_data::types_pair(),
            format_fields(fields)
        );

        sql << detail::join(fields);

        // process pk & fk constrains
        for (const auto &v : self_type::configuration_type::primary_key()) {
            sql << ",CONSTRAINT pk_" << self_type::type_meta_data::class_name() << "_" << v
                << " PRIMARY KEY (" << v << ")";
        }

        for (const auto &v : self_type::configuration_type::foreign_key()) {
            sql << ",CONSTRAINT fk_" << self_type::type_meta_data::class_name() << "_" << v.first
                << " FOREIGN KEY (" << v.first << ")" 
                << " REFERENCES " << v.second.first << "(" << v.second.second << ")";
        }

        sql << ")";

        session << sql.str();
    }

    template<class ...Expr>
    static void create_table(session::session_type &session, const Expr &...expr)
    {
        [](...){}(
            (self_type::configuration_type::eval(expr), false)...
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

            m_vec.emplace_back(str.str());
        }

    private:
        std::vector<std::string> &m_vec;
    };
};

template<class Type>
using fields_query = typename ddl<Type>::fields_type;

} // namespace soci_wrapper

namespace soci {

template<class Type>
struct type_conversion<
    Type,
    std::enable_if_t<
        soci_wrapper::detail::type_meta_data<Type>::is_declared::value
    >
>
{
    using base_type = values;

    using object_type = Type;

    using type_meta_data = soci_wrapper::detail::type_meta_data<object_type>;

    static void from_base(const base_type &v, indicator ind, Type &type)
    {
        std::cout << "From Base" << std::endl;
    }

    static void to_base(const Type &type, base_type &v, indicator &ind)
    {
        soci_wrapper::detail::tuple_for_each(
            type_meta_data::types_pair(),
            to_base_obj(type, v)
        );

        ind = soci::i_ok;
    }
private:
    struct to_base_obj
    {
        to_base_obj(const Type &obj, base_type &v)
            : object(obj)
            , values(v)
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
            values.set(val.second, *value);
        }

        const Type &object;
        base_type &values;
    };
};

} // namespace soci

