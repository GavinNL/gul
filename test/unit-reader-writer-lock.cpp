#include <catch2/catch.hpp>
#include <iostream>

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>

#include <gul/utils/writer_preferred_shared_mutex.h>

using RWLock = gul::writer_preferred_shared_mutex;


SCENARIO("TEST")
{
    RWLock lock;

    auto _reader= [&lock]()
    {
        for(int i=0;i<50;i++)
        {
            {
                std::shared_lock<RWLock> L(lock);

                std::this_thread::sleep_for( std::chrono::milliseconds(100+rand()%25));
            }
        }
    };

    auto _writer = [&lock]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(rand()%500));
        for(int i=0;i<5;i++)
        {

            {
                std::cout << "Readers Waiting: " << lock.get_reader_count() << std::endl;
                std::unique_lock<RWLock> L(lock);

                std::this_thread::sleep_for(std::chrono::milliseconds(100+rand()%25));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000+rand()%500));

        }
    };

    std::thread R1(_reader);
    std::thread R2(_reader);
    std::thread R3(_reader);
    std::thread R4(_reader);
    std::thread R5(_writer);
    std::thread R6(_writer);

    R1.join();
    R2.join();
    R3.join();
    R4.join();
    R5.join();
    R6.join();

}

