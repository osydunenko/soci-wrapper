#pragma once

#include <sstream>
#include <typeinfo>

#include "session.hpp"
#include "types_convertor.hpp"
#include "configuration.hpp"
#include "meta_data.hpp"
#include "query.hpp"
#include "detail.hpp"

namespace soci_wrapper {

struct dql
{
    template<class Type>
    static query::from<Type> query_from()
    {
        static_assert(detail::type_meta_data<Type>::is_declared::value,
            "The concerned Type is not declared as a persistent type");
        return query::from<Type>();
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

    static_assert(type_meta_data::is_declared::value,
        "The concerned Type is not declared as a persistent type");

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
        if (self_type::configuration_type::primary_key().size()) {
            sql << ", PRIMARY KEY (" << detail::join(self_type::configuration_type::primary_key()) << ")";
        }

        for (const auto &v : self_type::configuration_type::foreign_key()) {
            sql /*<< ",CONSTRAINT fk_" << self_type::type_meta_data::class_name() << "_" << v.first*/
                << ", FOREIGN KEY (" << v.first << ")" 
                << " REFERENCES " << v.second.first << "(" << v.second.second << ")";
        }

        sql << ")";

        session << sql.str();
    }

    template<class ...Expr>
    static void create_table(session::session_type &session, const Expr &...expr)
    {
        // Processing the configuration grammar
        [](...){}(
            (self_type::configuration_type::eval(expr), true)...
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

    static void from_base(const base_type &v, indicator ind, Type &object)
    {
        soci_wrapper::detail::tuple_for_each(
            type_meta_data::types_pair(),
            from_base_obj(v, ind, object)
        );
    }

    static void to_base(const Type &object, base_type &v, indicator &ind)
    {
        soci_wrapper::detail::tuple_for_each(
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

