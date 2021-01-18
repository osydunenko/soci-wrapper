#include "soci-wrapper/orm.hpp"

struct person
{
    std::string name;
    std::string email;
};

DECLARE_PERSISTENT_OBJECT(person, name, email);

int main(int argc, char *argv[])
{
    soci_wrapper::session::session_ptr_type session = soci_wrapper::session::connect("object.db");
    soci_wrapper::ddl<person>::create_table(*session);

    return 0;
}
