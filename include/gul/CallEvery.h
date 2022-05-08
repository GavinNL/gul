#ifndef GUL_INTERVAL_EXECUTOR_H
#define GUL_INTERVAL_EXECUTOR_H

#include <chrono>

namespace gul
{

/**
 * @brief The CallEvery class
 *
 * This class allows you to call a particular function at periodic
 * intervals.
 *
 * CallEvery C1(std::chrono::milliseconds(20));
 *
 * while(true)
 * {
 *      auto execTime = C1([&](auto dt)
 *      {
 *          // this lambda is executed every 20 milliseconds
 *
 *      });
 * }
 */
class CallEvery
{
public:
    using _clock     = std::chrono::system_clock;
    using time_point = _clock::time_point;
    using duration   = _clock::duration;

    CallEvery(duration dt = std::chrono::milliseconds(20)) : m_interval(dt)
    {
    }
    void setInterval(std::chrono::system_clock::duration dr)
    {
        m_interval = dr;
    }

    bool operator()()
    {
        auto _start = _clock::now();
        auto nextExec = m_lastExec+m_interval;

        if( _start >= nextExec)
        {
            m_lastExec = _start;
            return true;
        }
        return false;
    }

    template<typename callable_t>
    std::chrono::system_clock::duration operator()(callable_t && c)
    {
        auto _start = _clock::now();
        auto nextExec = m_lastExec+m_interval;

        if( _start >= nextExec)
        {
            auto dt = _start - m_lastExec;
            m_lastExec = _start;
            c(dt);
            auto _end = _clock::now();

            return _end-_start;
        }
        return duration(0);
    }

    duration getInterval() const
    {
        return m_interval;
    }
    time_point getLastExecTime() const
    {
        return m_lastExec;
    }
protected:
    duration   m_interval = std::chrono::milliseconds(20);
    time_point m_lastExec = _clock::now();
};

}

#endif
