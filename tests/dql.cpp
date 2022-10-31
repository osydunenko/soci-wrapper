#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dql

#include <array>
#include <boost/test/unit_test.hpp>

#include "soci-wrapper.hpp"

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
    std::array<char, 6> cpp_arr;

    bool operator==(const data_types &rhs) const
    {
        return f_double == rhs.f_double &&
            f_int == rhs.f_int &&
            f_string == rhs.f_string &&
            cpp_arr == rhs.cpp_arr;
    }
} dt {
    .f_double = std::numeric_limits<double>::max(),
    .f_int = std::numeric_limits<int>::max(),
    .f_string = "ABCDE",
    .cpp_arr{"ABCDE"}
};

DECLARE_PERSISTENT_OBJECT(person,
    id,
    name,
    surname
);

DECLARE_PERSISTENT_OBJECT(data_types,
    f_double,
    f_int,
    f_string,
    cpp_arr
);

sw::session::session_ptr_type session;

BOOST_AUTO_TEST_CASE(tst_order, * utf::depends_on("tst_populate"))
{
    std::vector<person> data = sw::dql::query_from<person>()
        .orderByDesc(sw::fields_query<person>::id)
        .objects(*session);
    BOOST_TEST(data.size() == 25);

    for (int idx = 0; idx < 25; ++idx) {
        BOOST_TEST(data[idx].id == data.size() - 1 - idx);
    }

    data = sw::dql::query_from<person>()
        .orderByDesc(sw::fields_query<person>::name)
        .orderByAsc(sw::fields_query<person>::id)
        .objects(*session);
    BOOST_TEST(data.size() == 25);
    const std::vector<int> vals {9,8,7,6,5,4,3,24,23,22,21,20,2,19,18,17,16,15,14,13,12,11,10,1,0};

    for (int idx = 0; idx < 25; ++idx) {
        BOOST_TEST(data[idx].id == vals[idx]);
    }
}

BOOST_AUTO_TEST_CASE(tst_data, * utf::depends_on("tst_populate"))
{
    std::vector<person> data = sw::dql::query_from<person>().objects(*session);
    BOOST_TEST(data.size() == 25);
    BOOST_TEST(sw::dql::query_from<person>().count(*session) == 25);

    for (int idx = 0; idx < 25; ++idx) {
        BOOST_TEST(data[idx].id == idx);
        BOOST_TEST(data[idx].name == "name " + std::to_string(idx));
        BOOST_TEST(data[idx].surname == "surname " + std::to_string(idx));
    }

    BOOST_TEST(
        (sw::dql::query_from<person>().object(*session) == data[0])
    );

    person prsn{
        .id = 20,
        .name = "name 20",
        .surname = "surname 20"
    };

    BOOST_TEST(
        (sw::dql::query_from<person>()
            .where(sw::fields_query<person>::id == 20 && 
                sw::fields_query<person>::name == std::string_view{ "name 20" } )
            .object(*session) 
        == prsn)
    );

    BOOST_TEST(
        (sw::dql::query_from<person>()
            .where(sw::fields_query<person>::name == std::string_view { "name 20" })
            .objects(*session)[0]
        == prsn)
    );

    BOOST_TEST(
        (sw::dql::query_from<person>()
            .where(sw::fields_query<person>::name == std::string{ "name 20" })
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
    session = sw::session::connect("tst_object.db");
    BOOST_TEST(session->is_connected());

    sw::ddl<person>::drop_table(*session);
    sw::ddl<person>::create_table(*session);

    sw::ddl<data_types>::drop_table(*session);
    sw::ddl<data_types>::create_table(*session);
}

