#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dql

#include <boost/test/unit_test.hpp>
#include "soci-wrapper/orm.hpp"

namespace sw = soci_wrapper;
namespace utf = boost::unit_test;

struct person
{
    int id;
    std::string name;
    std::string surname;

    bool operator==(const person &rhs) const
    {
        return id == rhs.id &&
            name == rhs.name &&
            surname == rhs.surname;
    }
};

struct data_types
{
    double f_double;
    int f_int;
    std::string f_string;

    bool operator==(const data_types &rhs) const
    {
        return f_double == rhs.f_double &&
            f_int == rhs.f_int &&
            f_string == rhs.f_string;
    }
} dt {
    .f_double = std::numeric_limits<double>::max(),
    .f_int = std::numeric_limits<int>::max(),
    .f_string = "ABCDE"
};

DECLARE_PERSISTENT_OBJECT(person,
    id,
    name,
    surname
);

DECLARE_PERSISTENT_OBJECT(data_types,
    f_double,
    f_int,
    f_string
);

sw::session::session_ptr_type session;

BOOST_AUTO_TEST_CASE(tst_data, * utf::depends_on("tst_populate"))
{
    std::vector<person> data = sw::dql::query_from<person>().objects(*session);
    BOOST_TEST(data.size() == 25);

    for (int idx = 0; idx < 25; ++idx) {
        BOOST_TEST(data[idx].id == idx);
        BOOST_TEST(data[idx].name == "name " + std::to_string(idx));
        BOOST_TEST(data[idx].surname == "surname " + std::to_string(idx));
    }

    person prsn{
        .id = 20,
        .name = "name 20",
        .surname = "surname 20"
    };

    BOOST_TEST(
        (sw::dql::query_from<person>()
            .where(sw::fields_query<person>::id == 20)
            .objects(*session)[0] 
        == prsn)
    );

    BOOST_TEST(
        (sw::dql::query_from<person>()
            .where(sw::fields_query<person>::name == "name 20")
            .objects(*session)[0] 
        == prsn)
    );

    BOOST_TEST(
        (sw::dql::query_from<data_types>().objects(*session)[0] == dt)
    );
}

BOOST_AUTO_TEST_CASE(tst_populate, * utf::depends_on("tst_conn"))
{
    for (int idx = 0; idx < 25; ++idx) {
        person prsn{
            .id = idx,
            .name = "name " + std::to_string(idx),
            .surname = "surname " + std::to_string(idx)
        };
        sw::dml::persist(*session, prsn);
    }

    sw::dml::persist(*session, dt);
    BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE(tst_conn, * utf::enable_if<SOCI_WRAPPER_SQLITE>())
{
    session = sw::session::connect("tst_object.db", [](std::string_view){});
    BOOST_TEST(session->is_connected());

    sw::ddl<person>::drop_table(*session);
    sw::ddl<person>::create_table(*session);

    sw::ddl<data_types>::drop_table(*session);
    sw::ddl<data_types>::create_table(*session);
}

