#pragma once

#include "soci/soci.h"

#ifdef SOCI_WRAPPER_SQLITE
#include "soci/sqlite3/soci-sqlite3.h"
#endif

namespace soci_wrapper {

struct session
{
    using session_type = soci::session;

    using session_ptr_type = std::unique_ptr<session_type>;

    using raw_ptr_type = session_type *;

    static session_ptr_type connect(const std::string &conn_string)
    {
        session_ptr_type session_ptr = connect();
        session_ptr->open(
#ifdef SOCI_WRAPPER_SQLITE
            soci::sqlite3,
#endif
            conn_string);

        return session_ptr;
    }

    static session_ptr_type connect(const std::string &conn_string, std::string &err)
    {
        try {
            auto conn = connect(conn_string);
            return conn;
        } catch (const std::exception &ex) {
            err = ex.what();
        }
        return connect();
    }

    static session_ptr_type connect()
    {
        return std::make_unique<session_type>();
    }
};

} // namespace soci_wrapper
