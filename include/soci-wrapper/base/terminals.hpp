#pragma once

#include <cstdint>
#include <type_traits>

#include <boost/proto/context.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/matches.hpp>

namespace soci_wrapper {
namespace placeholder {

    using index_type = std::uint32_t;

    template <index_type Idx>
    struct query_placeholder : std::integral_constant<index_type, Idx> {
    };

    struct terminals : boost::proto::or_<
                           boost::proto::terminal<query_placeholder<0>>,
                           boost::proto::terminal<query_placeholder<1>>,
                           boost::proto::terminal<query_placeholder<2>>,
                           boost::proto::terminal<query_placeholder<3>>,
                           boost::proto::terminal<query_placeholder<4>>,
                           boost::proto::terminal<query_placeholder<5>>,
                           boost::proto::terminal<query_placeholder<6>>,
                           boost::proto::terminal<query_placeholder<7>>,
                           boost::proto::terminal<query_placeholder<8>>> {
    };

} // namespace placeholder

struct literal_terminals : boost::proto::or_<
                               boost::proto::terminal<boost::proto::convertible_to<int>>,
                               boost::proto::terminal<boost::proto::convertible_to<std::string>>,
                               boost::proto::terminal<boost::proto::convertible_to<std::string_view>>> {
};

} // namespace soci_wrapper
