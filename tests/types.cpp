#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE types

#include "soci-wrapper/types_convertor.hpp"
#include <array>
#include <boost/test/unit_test.hpp>
#include <tuple>

using namespace soci_wrapper;

BOOST_AUTO_TEST_CASE(tst_array_of_char_type)
{
    std::array<char, 256> arr;
    BOOST_TEST(cpp_to_db_type<decltype(arr)>::db_type == "CHAR(256)");
}

BOOST_AUTO_TEST_CASE(tst_real_type)
{
    std::tuple<
        float, double>
        types;

    base::tuple_for_each(types, [](auto t) {
        BOOST_TEST(cpp_to_db_type<decltype(t)>::db_type == "REAL");
    });
}

BOOST_AUTO_TEST_CASE(tst_integral_type)
{
    std::tuple<
        long, int, short, char, bool,
        unsigned long, unsigned int, unsigned short, unsigned char>
        types;

    base::tuple_for_each(types, [](auto t) {
        BOOST_TEST(cpp_to_db_type<decltype(t)>::db_type == "INTEGER");
    });
}

BOOST_AUTO_TEST_CASE(tst_strings_types)
{
    BOOST_TEST(cpp_to_db_type<std::string>::db_type == "VARCHAR");
}
