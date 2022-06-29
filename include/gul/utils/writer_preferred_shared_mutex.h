#ifndef GUL_WRITER_PREFERRED_SHARED_MUTEX
#define GUL_WRITER_PREFERRED_SHARED_MUTEX

#include <atomic>
#include <shared_mutex>
#include <thread>

namespace gul
{

struct writer_preferred_shared_mutex
{
    void lock_shared()
    {
        while( m_writersWaiting.load() > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        m_mutex.lock_shared();
        m_readersCount.fetch_add(1);
    }

    void unlock_shared()
    {
        m_mutex.unlock_shared();
        m_readersCount.fetch_sub(1);
    }

    bool try_lock_shared()
    {
        if(m_writersWaiting.load() > 0)
        {
            return false;
        }
        else
        {
            return m_mutex.try_lock_shared();
        }
    }

    bool try_lock()
    {
        m_writersWaiting.fetch_add(1);
        auto ret = m_mutex.try_lock();
        if( ret )
        {
            return true;
        }
        else
        {
            m_writersWaiting.fetch_sub(1);
            return false;
        }
    }

    void lock()
    {
        m_writersWaiting.fetch_add(1);
        m_mutex.lock();
    }
    void unlock()
    {
        m_writersWaiting.fetch_sub(1);
        m_mutex.unlock();
    }

    auto get_reader_count() const
    {
        return m_readersCount.load();
    }
    auto get_writers_waiting() const
    {
        return m_writersWaiting.load();
    }
protected:
    std::atomic<int>  m_writersWaiting = 0;
    std::atomic<int>  m_readersCount = 0;
    std::shared_mutex m_mutex;
};

}

#endif
