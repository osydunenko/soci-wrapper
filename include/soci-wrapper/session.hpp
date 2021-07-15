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

    using on_error_type = std::function<void(std::string_view)>;

    static session_ptr_type connect(const std::string &conn_string, on_error_type &&on_error = nullptr)
    {
        session_ptr_type session_ptr = get_empty_session();
        try {
            session_ptr->open(
#ifdef SOCI_WRAPPER_SQLITE
                soci::sqlite3,
#endif
                conn_string);
        } catch(const std::exception &ex) {
            if (on_error) {
                on_error(ex.what());
                return nullptr;
            }
            throw;
        }

        return session_ptr;
    }

    static session_ptr_type get_empty_session()
    {
        return std::make_unique<session_type>();
    }
};

} // namespace soci_wrapper
