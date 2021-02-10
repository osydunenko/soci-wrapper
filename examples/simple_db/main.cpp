#include "soci-wrapper/orm.hpp"

struct person
{
    int id;
    std::string name;
    std::string surname;
    char rate;
};

struct account
{
    int id;
    std::string account_id;
};

DECLARE_PERSISTENT_OBJECT(person, 
    id,
    name,
    surname,
    rate
);

DECLARE_PERSISTENT_OBJECT(account,
    id,
    account_id
);

int main(int argc, char *argv[])
{
    soci_wrapper::session::session_ptr_type session = soci_wrapper::session::connect("object.db");

    soci_wrapper::ddl<person>::create_table(*session,
        soci_wrapper::fields_query<person>::id = soci_wrapper::primary_key_constraint,
        soci_wrapper::fields_query<person>::name = soci_wrapper::not_null_constraint,
        soci_wrapper::fields_query<person>::rate = soci_wrapper::unique_constraint
    );

    soci_wrapper::ddl<account>::create_table(*session,
        soci_wrapper::fields_query<account>::id = soci_wrapper::foreign_key_constraint<person>(
            soci_wrapper::fields_query<person>::id),
        soci_wrapper::fields_query<account>::account_id = soci_wrapper::unique_constraint
    );

    person p;
    p.id = 12;
    p.name = "name1";
    p.surname = "surname1";
    p.rate = 'B';

    const person c_person = p;

    soci_wrapper::dml::persist(*session, c_person);

    return 0;
}
