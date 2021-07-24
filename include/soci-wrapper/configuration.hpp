#pragma once

#include "base/config.hpp"

namespace soci_wrapper {

template<class Type>
struct configuration
{
public:
    using self_type = configuration<Type>;

    using object_type = Type;

    using context_type = config::context<Type>;

    using container_type = typename config::configuration_attributes<Type>::container_type;

    using foreign_key_container_type = typename config::configuration_attributes<Type>::foreign_key_container_type;

    static_assert(details::type_meta_data<Type>::is_declared::value,
        "The concerned Type is not declared as a persistent type");

    template<class Expr>
    static void eval(const Expr &expr)
    {
        static_assert(boost::proto::matches<Expr, config::configuration_grammar>::value, 
            "Invalid Configuration Grammar");
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
