#pragma once

#include <vector>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <functional>

#include <boost/preprocessor.hpp>

namespace soci_wrapper {
namespace detail {

template<class Type>
struct type_meta_data
{
    using self_type = type_meta_data<Type>;

    using is_declared = std::false_type;

    using tuple_type = std::tuple<void>;

    using tuple_type_pair = std::tuple<std::pair<void, void>>;

    using field_offsets = std::unordered_map<std::string, size_t>;

    using fields_number = std::integral_constant<size_t, 0>;

    static std::string_view class_name();

    static std::vector<std::string_view> member_names();

    static const tuple_type_pair &types_pair();

    static size_t field_offset(std::string_view field);

    struct dsl_fields {};
};

} // namespace detail
} // namespace soci_wrapper

#define EXPAND_MEMBERS_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(N, DATA))
#define EXPAND_MEMBERS(TUPLE) BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(TUPLE), EXPAND_MEMBERS_IDX, TUPLE)

#define EXPAND_MEMBERS_TYPE_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))
#define EXPAND_MEMBERS_TYPE(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_TYPE_IDX, TUPLE)

#define EXPAND_MEMBERS_TYPE_PAIR_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) std::pair<std::add_pointer_t<decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))>, std::string>
#define EXPAND_MEMBERS_TYPE_PAIR(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_TYPE_PAIR_IDX, TUPLE)

#define EXPAND_MEMBERS_PAIR_ELEM_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) std::make_pair<std::add_pointer_t<decltype(BOOST_PP_TUPLE_ELEM(0, DATA)::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))>, std::string>(nullptr, BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA)))
#define EXPAND_MEMBERS_PAIR_ELEM(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_PAIR_ELEM_IDX, TUPLE)

#define EXPAND_MEMBERS_PAIR_OFFSET_IDX(Z, N, DATA) BOOST_PP_COMMA_IF(N) {BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA)), offsetof(BOOST_PP_TUPLE_ELEM(0, DATA), BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA))}
#define EXPAND_MEMBERS_PAIR_OFFSET(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_MEMBERS_PAIR_OFFSET_IDX, TUPLE) 

#define EXPAND_DSL_FIELDS_DECL_IDX(Z, N, DATA) static const boost::proto::terminal<soci_wrapper::placeholder::query_placeholder<N>>::type BOOST_PP_TUPLE_ELEM(N, DATA);
#define EXPAND_DSL_FIELDS_DECL(TUPLE) BOOST_PP_REPEAT(BOOST_PP_TUPLE_SIZE(TUPLE), EXPAND_DSL_FIELDS_DECL_IDX, TUPLE)

#define EXPAND_DSL_FIELDS_DEF_IDX(Z, N, DATA) const boost::proto::terminal<soci_wrapper::placeholder::query_placeholder<N>>::type type_meta_data<BOOST_PP_TUPLE_ELEM(0, DATA)>::dsl_fields::BOOST_PP_TUPLE_ELEM(BOOST_PP_ADD(N, 1), DATA){{}};
#define EXPAND_DSL_FIELDS_DEF(TUPLE) BOOST_PP_REPEAT(BOOST_PP_SUB(BOOST_PP_TUPLE_SIZE(TUPLE), 1), EXPAND_DSL_FIELDS_DEF_IDX, TUPLE)

#define DECLARE_PERSISTENT_OBJECT(...) \
    namespace soci_wrapper { \
    namespace detail { \
        template<> \
        struct type_meta_data<BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))> \
        { \
            using self_type = type_meta_data<BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            using is_declared = std::true_type; \
            using tuple_type = std::tuple<EXPAND_MEMBERS_TYPE(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            using tuple_type_pair = std::tuple<EXPAND_MEMBERS_TYPE_PAIR(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))>; \
            using fields_offset = std::unordered_map<std::string, size_t>; \
            using fields_number = std::tuple_size<tuple_type>; \
            static std::string_view class_name() \
            { \
                static const std::string class_name = BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(0, BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))); \
                return class_name; \
            } \
            static std::vector<std::string_view> member_names() \
            { \
                static const std::vector<std::string> names{EXPAND_MEMBERS(BOOST_PP_TUPLE_POP_FRONT(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)))}; \
                return std::vector<std::string_view>{names.begin(), names.end()}; \
            } \
            static const tuple_type_pair &types_pair() \
            { \
                static const tuple_type_pair pairs{EXPAND_MEMBERS_PAIR_ELEM(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))}; \
                return pairs; \
            } \
            static size_t field_offset(std::string_view field) \
            { \
                static const fields_offset offsets = { \
                    EXPAND_MEMBERS_PAIR_OFFSET(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) \
                }; \
                return offsets.at(std::string{field}); \
            } \
            struct dsl_fields \
            { \
                EXPAND_DSL_FIELDS_DECL(BOOST_PP_TUPLE_POP_FRONT(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__))) \
            }; \
        }; \
        EXPAND_DSL_FIELDS_DEF(BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) \
    } \
    }

