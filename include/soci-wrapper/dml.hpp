#pragma once

#include "session.hpp"
#include "types_convertor.hpp"
#include "base/utility.hpp"

namespace soci_wrapper {

/*! \brief DML -- Data Modification Language
 */
struct dml
{
    template<class ...Type>
    static void persist(session::session_type &session, Type &&...objects)
    {
        base::tuple_for_each(
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
            using type_meta_data = details::type_meta_data<decayed_type>;

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
                <<base::join(fields)
                << ") VALUES ("
                <<base::join(sql_placeholders)
                << ")";

            sql_session << sql.str(), soci::use(std::forward<Type>(object));
        }

        session::session_type &sql_session;
    };
};


} // namespace soci_wrapper
