#pragma once

#include <unordered_set>
#include <unordered_map>

#include <boost/proto/context.hpp>
#include <boost/proto/matches.hpp>

#include "placeholders.hpp"
#include "meta_data.hpp"

namespace soci_wrapper {
namespace config {

template<class Type>
struct configuration_attributes
{
    using container_type = std::unordered_set<std::string>;
    using foreign_key_value_type = std::pair<std::string_view, std::string_view>;
    using foreign_key_container_type = std::unordered_map<std::string, foreign_key_value_type>;

    static container_type &not_null()
    {
        static container_type values;
        return values;
    }

    static container_type &unique()
    {
        static container_type values;
        return values;
    }

    static container_type &primary_key()
    {
        static container_type values;
        return values;
    }

    static foreign_key_container_type &foreign_key()
    {
        static foreign_key_container_type values;
        return values;
    }
};

struct not_null_constraint
{
};

struct unique_constraint
{
};

struct primary_key_constraint
{
};

struct foreign_key_constraint_base
{
    virtual std::string_view reference_table() const = 0;
    virtual std::string_view reference_column() const = 0;
};

template<class Type>
struct foreign_key_constraint: foreign_key_constraint_base
{
    static_assert(detail::type_meta_data<Type>::is_declared::value,
            "The concerned Type is not declared as a persistent type");

    const placeholder::index_type index;

    template<class Placeholder>
    foreign_key_constraint(const Placeholder &)
        : index(Placeholder::proto_args::child0::value)
    {
    }
    
    std::string_view reference_table() const override
    {
        return detail::type_meta_data<Type>::class_name();
    }

    std::string_view reference_column() const override
    {
        return detail::type_meta_data<Type>::member_names()[index];
    }
};

struct configuration_terminals: boost::proto::or_<
    boost::proto::terminal<not_null_constraint>,
    boost::proto::terminal<unique_constraint>,
    boost::proto::terminal<foreign_key_constraint_base>
>
{
};

struct configuration_grammar: 
    boost::proto::assign<
        placeholder::terminals,
        configuration_terminals
    >
{
};

template<class Type>
struct context: boost::proto::callable_context<const context<Type>>
{
    using self_type = context<Type>;

    using result_type = bool;

    template<placeholder::index_type Idx>
    result_type operator()(boost::proto::tag::terminal, placeholder::query_placeholder<Idx>) const
    {
        field_name = detail::type_meta_data<Type>::member_names()[Idx];
        return true;
    }

    result_type operator()(boost::proto::tag::terminal, const not_null_constraint &) const
    {
        configuration_attributes<Type>::not_null().emplace(field_name);
        return true;
    }

    result_type operator()(boost::proto::tag::terminal, const unique_constraint &) const
    {
        configuration_attributes<Type>::unique().emplace(field_name);
        return true;
    }

    result_type operator()(boost::proto::tag::terminal, const primary_key_constraint &) const
    {
        configuration_attributes<Type>::primary_key().emplace(field_name);
        return true;
    }

    result_type operator()(boost::proto::tag::terminal, const foreign_key_constraint_base &fk) const
    {
        configuration_attributes<Type>::foreign_key().emplace(
            std::make_pair(field_name, std::make_pair(fk.reference_table(), fk.reference_column()))
        );
        return true;
    }

    template<class Left, class Right>
    result_type operator()(boost::proto::tag::assign, const Left &left, const Right &right) const
    {
        bool l = boost::proto::eval(left, *this);
        bool r = boost::proto::eval(right, *this);
        return l & r;
    }

    mutable std::string_view field_name{};
};

} // namespace config

template<class Type>
struct configuration
{
public:
    using self_type = configuration<Type>;

    using object_type = Type;

    using context_type = config::context<Type>;

    using container_type = typename config::configuration_attributes<Type>::container_type;

    using foreign_key_container_type = typename config::configuration_attributes<Type>::foreign_key_container_type;

    template<class Expr>
    static void eval(const Expr &expr)
    {
        // TODO: to check why the follows assert triggers
        //static_assert(boost::proto::matches<Expr, config::configuration_grammar>::value, "Invalid Grammar");
        boost::proto::eval(expr, context_type{});
    }

    static const container_type &not_null()
    {
        return config::configuration_attributes<Type>::not_null();
    }

    static const container_type &unique()
    {
        return config::configuration_attributes<Type>::unique();
    }

    static const container_type &primary_key()
    {
        return config::configuration_attributes<Type>::primary_key();
    }

    static const foreign_key_container_type &foreign_key()
    {
        return config::configuration_attributes<Type>::foreign_key();
    }
};

static config::not_null_constraint not_null_constraint;
static config::unique_constraint unique_constraint;
static config::primary_key_constraint primary_key_constraint;

template<class Type>
using foreign_key_constraint = config::foreign_key_constraint<Type>;

} // namespace soci_wrapper

