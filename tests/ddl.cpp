#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ddl

#include "soci-wrapper.hpp"
#include <array>
#include <boost/test/unit_test.hpp>

#if defined(SW_SQLITE)
constexpr bool sw_sqlite = true;
#else
constexpr bool sw_sqlite = false;
#endif

namespace sw = soci_wrapper;
namespace utf = boost::unit_test;
using namespace std::literals;

sw::session::session_ptr_type session;

struct ddl_pk_tbl {
    int f_pk_id;
    std::string f_not_null;
};

struct ddl_fk_tbl {
    int f_fk_id;
    std::string f_unique;
};

struct ddl_ai_tbl {
    int f_ai_id;
    int f_v_id;
};

DECLARE_PERSISTENT_OBJECT(ddl_pk_tbl,
    f_pk_id,
    f_not_null);

DECLARE_PERSISTENT_OBJECT(ddl_fk_tbl,
    f_fk_id,
    f_unique);

DECLARE_PERSISTENT_OBJECT(ddl_ai_tbl,
    f_ai_id,
    f_v_id);

namespace N1 {
struct ddl_namespace {
    int id;
};

namespace N2 {
    struct ddl_namespace {
        int id;
    };
} // namespace N2
} // namespace N1

struct ddl_namespace {
    int id;
};

DECLARE_PERSISTENT_OBJECT(N1::ddl_namespace, id);
DECLARE_PERSISTENT_OBJECT(N1::N2::ddl_namespace, id);
DECLARE_PERSISTENT_OBJECT(::ddl_namespace, id);

BOOST_AUTO_TEST_CASE(tst_check_namespace)
{
    BOOST_TEST(sw::details::type_meta_data<N1::ddl_namespace>::table_name() == "ddl_namespace"sv);
    BOOST_TEST(sw::details::type_meta_data<N1::N2::ddl_namespace>::table_name() == "ddl_namespace"sv);
    BOOST_TEST(sw::details::type_meta_data<::ddl_namespace>::table_name() == "ddl_namespace"sv);
}

BOOST_AUTO_TEST_CASE(tst_check_autoincrement, *utf::depends_on("tst_smooth_data"))
{
    std::array<ddl_ai_tbl, 4> data { { { 18, 1 }, { 0, 2 }, { 3, 3 }, { -1, 4 } } };
    sw::base::tuple_for_each(data, [](const ddl_ai_tbl& tbl) {
        sw::dml::persist(*session, tbl);
    });

    auto vals = sw::dql::query_from<ddl_ai_tbl>().objects(*session);
    BOOST_TEST(vals.size() == data.size());

    sw::base::tuple_for_each(data, [&vals](const ddl_ai_tbl& tbl) {
        BOOST_TEST(vals[tbl.f_v_id - 1].f_ai_id = tbl.f_v_id);
    });
}

BOOST_AUTO_TEST_CASE(tst_check_constraints, *utf::depends_on("tst_smooth_data"))
{
    ddl_pk_tbl pk {
        .f_pk_id = 24,
        .f_not_null = ""
    };

    BOOST_CHECK_THROW(sw::dml::persist(*session, pk), std::exception);

    ddl_fk_tbl fk {
        .f_fk_id = 24,
        .f_unique = "unique 24"
    };

    BOOST_CHECK_THROW(sw::dml::persist(*session, fk), std::exception);
}

BOOST_AUTO_TEST_CASE(tst_query_data, *utf::depends_on("tst_smooth_data"))
{
    ddl_pk_tbl obj = sw::dql::query_from<ddl_pk_tbl>()
                         .where(sw::fields_query<ddl_pk_tbl>::f_pk_id == 12)
                         .objects(*session)[0];
    BOOST_TEST(obj.f_not_null == "not null");
}

BOOST_AUTO_TEST_CASE(tst_smooth_data, *utf::depends_on("tst_conn"))
{
    ddl_pk_tbl pk {
        .f_pk_id = 12,
        .f_not_null = "not null"
    };

    ddl_fk_tbl fk {
        .f_fk_id = 12,
        .f_unique = "unique"
    };

    sw::dml::persist(*session, pk, fk);
    BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(tst_conn, *utf::enable_if<sw_sqlite>())
{
    session = sw::session::connect("tst_object.db");
    BOOST_TEST(session->is_connected());

    (*session) << "PRAGMA foreign_keys = ON;";

    sw::ddl<ddl_fk_tbl>::drop_table(*session);
    sw::ddl<ddl_pk_tbl>::drop_table(*session);
    sw::ddl<ddl_ai_tbl>::drop_table(*session);

    sw::ddl<ddl_pk_tbl>::create_table(*session,
        sw::fields_query<ddl_pk_tbl>::f_pk_id = sw::primary_key_constraint,
        sw::fields_query<ddl_pk_tbl>::f_not_null = sw::not_null_constraint);

    sw::ddl<ddl_fk_tbl>::create_table(*session,
        sw::fields_query<ddl_fk_tbl>::f_fk_id = sw::foreign_key_constraint<ddl_pk_tbl>(
            sw::fields_query<ddl_pk_tbl>::f_pk_id),
        sw::fields_query<ddl_fk_tbl>::f_unique = sw::unique_constraint);

    sw::ddl<ddl_ai_tbl>::create_table(*session,
        sw::fields_query<ddl_ai_tbl>::f_ai_id = sw::auto_increment_constraint);
}
