#pragma once

#include <unordered_map>
#include <unordered_set>

#include "meta_data.hpp"
#include "terminals.hpp"

namespace soci_wrapper {
namespace config {

    template <class Type>
    struct configuration_attributes {
        using container_type = std::unordered_set<std::string>;
        using foreign_key_value_type = std::pair<std::string_view, std::string_view>;
        using foreign_key_container_type = std::unordered_map<std::string, foreign_key_value_type>;

        static container_type& not_null()
        {
            static container_type values;
            return values;
        }

        static container_type& unique()
        {
            static container_type values;
            return values;
        }

        static container_type& primary_key()
        {
            static container_type values;
            return values;
        }

        static foreign_key_container_type& foreign_key()
        {
            static foreign_key_container_type values;
            return values;
        }
    };

    struct not_null_constraint {
    };

    struct unique_constraint {
    };

    struct primary_key_constraint {
    };

    template <class Type>
    struct foreign_key_constraint {
        static_assert(details::type_meta_data<Type>::is_declared::value,
            "The concerned Type is not declared as a persistent type");

        const placeholder::index_type index;

        template <class Placeholder>
        foreign_key_constraint(const Placeholder&)
            : index(Placeholder::proto_args::child0::value)
        {
        }

        std::string_view reference_table() const
        {
            return details::type_meta_data<Type>::class_name();
        }

        std::string_view reference_column() const
        {
            return details::type_meta_data<Type>::member_names()[index];
        }
    };

    struct configuration_terminals : boost::proto::or_<
                                         boost::proto::terminal<not_null_constraint>,
                                         boost::proto::terminal<unique_constraint>,
                                         boost::proto::terminal<primary_key_constraint>,
                                         boost::proto::terminal<foreign_key_constraint<boost::proto::_>>> {
    };

    struct configuration_grammar : boost::proto::or_<
                                       boost::proto::assign<
                                           placeholder::terminals,
                                           configuration_terminals>,
                                       configuration_terminals> {
    };

    template <class Type>
    struct context : boost::proto::callable_context<const context<Type>> {
        using self_type = context<Type>;

        using result_type = bool;

        template <placeholder::index_type Idx>
        result_type operator()(boost::proto::tag::terminal, placeholder::query_placeholder<Idx>) const
        {
            field_name = details::type_meta_data<Type>::member_names()[Idx];
            return true;
        }

        result_type operator()(boost::proto::tag::terminal, const not_null_constraint&) const
        {
            configuration_attributes<Type>::not_null().emplace(field_name);
            return true;
        }

        result_type operator()(boost::proto::tag::terminal, const unique_constraint&) const
        {
            configuration_attributes<Type>::unique().emplace(field_name);
            return true;
        }

        result_type operator()(boost::proto::tag::terminal, const primary_key_constraint&) const
        {
            configuration_attributes<Type>::primary_key().emplace(field_name);
            return true;
        }

        template <class X>
        result_type operator()(boost::proto::tag::terminal, const foreign_key_constraint<X>& fk) const
        {
            configuration_attributes<Type>::foreign_key().emplace(
                std::make_pair(field_name, std::make_pair(fk.reference_table(), fk.reference_column())));
            return true;
        }

        template <class Left, class Right>
        result_type operator()(boost::proto::tag::assign, const Left& left, const Right& right) const
        {
            bool l = boost::proto::eval(left, *this);
            bool r = boost::proto::eval(right, *this);
            return l & r;
        }

        mutable std::string_view field_name {};
    };

} // namespace config
} // namespace soci_wrapper
