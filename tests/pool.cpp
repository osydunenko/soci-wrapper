#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE pool

#include <list>

#include <boost/test/unit_test.hpp>
#include "soci-wrapper/orm.hpp"

namespace sw = soci_wrapper;
namespace utf = boost::unit_test;
using sessions_pool = sw::sessions_pool<sw::session>;

static sessions_pool::ptr_type pool;
static const size_t conn_size = 8;
static std::list<sessions_pool::session_proxy> proxies;

BOOST_AUTO_TEST_CASE(tst_session_proxy, * utf::depends_on("tst_release_sessions"))
{
    // Create empty session
    auto emp = sessions_pool::get_empty_session();
    BOOST_TEST(!emp.is_connected());

    // and compare to a valid session
    auto valid = pool->get_session();
    BOOST_TEST(emp.is_connected() != valid.is_connected());
    BOOST_TEST(pool->size() == conn_size - 1);

    // Move assignment operator and relaeasing of the valid session
    valid = std::move(emp);

    BOOST_TEST(emp.is_connected() == valid.is_connected());
    BOOST_TEST(pool->size() == conn_size);
}

BOOST_AUTO_TEST_CASE(tst_release_sessions, * utf::depends_on("tst_empty_pool"))
{
    // Release all the previously acquired connections
    // from the "proxies" list
    proxies.clear();
    BOOST_TEST(pool->size() == conn_size);
    BOOST_TEST(pool->get_session().is_connected());
    BOOST_TEST(pool->size() == conn_size);
}

BOOST_AUTO_TEST_CASE(tst_empty_pool, *  utf::depends_on("tst_init_pool"))
{
    BOOST_TEST(pool->size() == 0);

    // All the previously acquired connections are valid
    for (const auto &session : proxies)
        BOOST_TEST(session.is_connected());

    // The pool is empty. The next session acquisitions should return an empty
    BOOST_TEST(!pool->get_session().is_connected());
}

BOOST_AUTO_TEST_CASE(tst_init_pool)
{
    pool = sessions_pool::create(conn_size, "tst_object.db");
    
    // Check the number of available connections
    // and those are connected properly in a way by acquiring
    // a connection on each iteration
    BOOST_TEST(pool->size() == conn_size);

    for (size_t i = 0; i < conn_size; ++i) {
        BOOST_TEST(pool->size() == conn_size - i);
        proxies.emplace_back(pool->get_session());
    }
}
