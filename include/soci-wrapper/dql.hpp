#pragma once

#include <concepts>
#include <sstream>

#include "base/terminals.hpp"
#include "base/utility.hpp"
#include "session.hpp"
#include "types_convertor.hpp"

namespace soci_wrapper {
namespace query {

    struct query_grammar : boost::proto::or_<
                               boost::proto::equal_to<
                                   placeholder::terminals,
                                   literal_terminals>> {
    };

    template <class Type>
    struct context : boost::proto::callable_context<const context<Type>> {
        using self_type = context<Type>;

        using result_type = std::string;

        using type_meta_data = details::type_meta_data<Type>;

        template <placeholder::index_type Idx>
        result_type operator()(boost::proto::tag::terminal, placeholder::query_placeholder<Idx>) const
        {
            return std::string { type_meta_data::member_names()[Idx] };
        }

        result_type operator()(boost::proto::tag::terminal, int i) const
        {
            return std::to_string(i);
        }

        result_type operator()(boost::proto::tag::terminal, const std::string& val) const
        {
            std::stringstream str;
            str << "'" << val << "'";
            return str.str();
        }

        template <class Left, class Right>
        result_type operator()(boost::proto::tag::equal_to, const Left& left, const Right& right) const
        {
            std::stringstream str;
            str << boost::proto::eval(left, *this)
                << " = "
                << boost::proto::eval(right, *this);
            return str.str();
        }
    };

    template <class Type>
    class from {
    public:
        using self_type = from<Type>;

        using object_type = Type;

        using context_type = query::context<Type>;

        using query_result_type = typename context_type::result_type;

        using type_meta_data = details::type_meta_data<Type>;

        static_assert(type_meta_data::is_declared::value,
            "The concerned Type is not declared as a persistent type");
        static_assert(std::is_default_constructible_v<Type>,
            "The concerned Type is not default constructible");

        from()
            : m_sql_stream()
        {
            m_sql_stream << "SELECT "
                         << base::join(type_meta_data::member_names())
                         << " FROM "
                         << type_meta_data::table_name();
        }

        template <class Expr>
        self_type& where(const Expr& expr)
        {
            m_sql_stream << " WHERE " << eval(expr);
            return *this;
        }

        std::string sql() const
        {
            return m_sql_stream.str();
        }

        template <template <class...> class Cont = std::vector, class... Args>
        Cont<Type, Args...> objects(session::session_type& session)
        {
            return exec<Cont, Args...>(session);
        }

    private:
        template <class Expr>
        query_result_type eval(const Expr& expr)
        {
            static_assert(boost::proto::matches<Expr, query::query_grammar>::value,
                "Invalid query grammar");
            return boost::proto::eval(expr, context_type {});
        }

        template <template <class...> class Cont, class... Args>
        Cont<Type, Args...> exec(session::session_type& session)
        {
            soci::rowset<Type> rs = (session.prepare << sql());
            return Cont<Type, Args...> { rs.begin(), rs.end() };
        }

        std::stringstream m_sql_stream;
    };

} // namespace query

/*! \brief DQL -- Data Query Language
 */
struct dql {
    template <class Type>
    static query::from<Type> query_from()
    {
        static_assert(details::type_meta_data<Type>::is_declared::value,
            "The concerned Type is not declared as a persistent type");
        return query::from<Type>();
    }
};

} // namespace soci_wrapper