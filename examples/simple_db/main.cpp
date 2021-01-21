#include "soci-wrapper/orm.hpp"

struct person
{
    std::string string_field;
    int int_field;
    char char_field;
};

DECLARE_PERSISTENT_OBJECT(person, 
    string_field, 
    int_field, 
    char_field
);

int main(int argc, char *argv[])
{
    soci_wrapper::session::session_ptr_type session = soci_wrapper::session::connect("object.db");
    soci_wrapper::ddl<person>::create_table(*session);

    return 0;
}
