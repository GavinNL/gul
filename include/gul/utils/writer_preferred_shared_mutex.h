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
        while( writersWaiting.load() > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        m_mutex.lock_shared();
    }
    void unlock_shared()
    {
        m_mutex.unlock_shared();
    }

    void lock()
    {
        writersWaiting.fetch_add(1);
        m_mutex.lock();
    }
    void unlock()
    {
        writersWaiting.fetch_sub(1);
        m_mutex.unlock();
    }

protected:
    std::atomic<int>            writersWaiting = 0;
    std::condition_variable_any cv;
    std::shared_mutex           m_mutex;
};

}

#endif
