#pragma once

#include <list>
#include <string>
#include <memory>
#include <cassert>

#include "lockable.hpp"

namespace soci_wrapper {

template<class Session>
class sessions_pool: public std::enable_shared_from_this<sessions_pool<Session>>
{
public:
    using self_type = sessions_pool<Session>;
    
    using self_ptr_type = std::shared_ptr<self_type>;

    using session_type = Session;

    using session_raw_type = typename Session::session_type;

    using session_ptr_type = typename session_type::session_ptr_type;

    using ptr_type = std::shared_ptr<self_type>;

    using weak_ptr_type = std::weak_ptr<self_type>;

    using mutex_type = lockable::mutex_type;

    using session_cont_type = std::list<session_ptr_type>;

    struct session_proxy;

    static ptr_type create(size_t size, const std::string &conn_string)
    {
        return ptr_type(new self_type(size, conn_string));
    }

    static session_proxy get_empty_session()
    {
        return session_proxy({}, session_type::get_empty_session());
    }

    session_proxy get_session()
    {
        LOCKABLE_ENTER_TO_WRITE(m_mutex);
        if (m_idle.empty())
            return get_empty_session();

        auto session = std::move(m_idle.front());
        m_idle.pop_front();

        return session_proxy(this->weak_from_this(), std::move(session));
    }

    void release_session(session_ptr_type session)
    {
        LOCKABLE_ENTER_TO_WRITE(m_mutex);
        if (session && session->is_connected())
            m_idle.emplace_back(std::move(session));
    }

    size_t size() const
    {
        LOCKABLE_ENTER_TO_READ(m_mutex);
        return m_idle.size();
    }

    struct session_proxy
    {
        operator session_raw_type &()
        {
            assert(m_session);
            return *m_session;
        }

        operator const session_raw_type &() const
        {
            assert(m_session);
            return *m_session;
        }

        bool is_connected() const
        {
            assert(m_session);
            return m_session->is_connected();
        }

        session_proxy() = delete;

        session_proxy(const session_proxy &) = delete;

        session_proxy &operator=(const session_proxy &) = delete;

        session_proxy(session_proxy &&other)
            : m_pool(other.m_pool)
            , m_session(std::move(other.m_session))
        {
            other.release_session();
        }

        session_proxy &operator=(session_proxy &&other)
        {
            if (this != &other) {
                release_session();
                m_pool = other.m_pool;
                m_session = std::move(other.m_session);
                other.release_session();
            }

            return *this;
        }

        ~session_proxy()
        {
            release_session();
        }

    private:
        friend self_type;

        session_proxy(weak_ptr_type pool, session_ptr_type session)
            : m_pool(pool)
            , m_session(std::move(session))
        {
        }

        void release_session()
        {
            if (!m_pool.expired())
                m_pool.lock()->release_session(std::move(m_session));

            m_pool.reset();
            m_session = session_type::get_empty_session();
        }

        weak_ptr_type m_pool;
        session_ptr_type m_session;
    };

private:
    sessions_pool(size_t size, const std::string &conn_string)
        : m_idle()
        , m_mutex()
    {
        for (size_t idx = 0; idx < size; ++idx) {
            auto session = session_type::connect(conn_string);
            m_idle.emplace_back(std::move(session));
        }
    }

    session_cont_type m_idle;
    mutable mutex_type m_mutex;
};

} // namespace soci_wrapper
