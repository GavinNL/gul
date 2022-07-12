#include <catch2/catch.hpp>
#include <iostream>
#include <gul/CallEvery.h>
#include <thread>

using namespace gul;

SCENARIO("Execute without sleeping")
{
    CallEvery C1( std::chrono::milliseconds(20));
    size_t count = 0;
    size_t iterations = 0;

    while(count < 10)
    {
        auto execTime = C1([&](auto dt)
        {
            ++count;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt);

            REQUIRE(ms.count() == 20); // called at regular intervals
        });
        (void)execTime;
        iterations++;
    }

    // should have looped more than count times
    REQUIRE(iterations > count);

    // total number of loops should be quite large
    // since we are not sleeping at all
    REQUIRE(iterations > 1000);
}

SCENARIO("Execute without sleeping, no-lambda")
{
    CallEvery C1( std::chrono::milliseconds(20));
    size_t count = 0;
    size_t iterations = 0;

    auto t0 = CallEvery::_clock::now();
    while(count < 10)
    {
        if(C1())
        {
            auto t1 = CallEvery::_clock::now();
            auto dt = t1-t0;
            t0 = t1;
            ++count;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt);

            REQUIRE(ms.count() >= 19); // called at regular intervals
        };
        iterations++;
    }

    // should have looped more than count times
    REQUIRE(iterations > count);

    // total number of loops should be quite large
    // since we are not sleeping at all
    REQUIRE(iterations > 1000);
}


SCENARIO("Execute with sleep")
{
    CallEvery C1( std::chrono::milliseconds(20));
    size_t count = 0;
    size_t iterations = 0;
    while(count < 10)
    {
        auto execTime = C1([&](auto dt)
        {
            ++count;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt);

            REQUIRE(ms.count() == 20); // called at regular intervals
        });

        auto timeToSleep = C1.getInterval()-execTime;

        std::this_thread::sleep_for(timeToSleep);
        iterations++;
    }
    // should iterate at least count times
    REQUIRE(iterations >= count);

    // but shouldn't have 1000s of iterations
    // since we are sleeping
    REQUIRE(iterations <= 20);
}


SCENARIO("Execute with computation time longer than interval")
{
    CallEvery C1;
    size_t count = 0;
    bool isRun=false;
    while(count < 10)
    {
        auto execTime = C1([&](auto dt)
        {
            isRun = true;

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt);

            // make the exec time at least 2x as long as the interval
            std::this_thread::sleep_for(C1.getInterval()*2);

            if(count == 0)
            {
                CHECK( ms.count() == 20);
            }
            else
            {
                CHECK( ms.count() == 40 );
            }
            ++count;
        });

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(execTime);

        if(isRun) // during iterations when the funciton is run
        {
            REQUIRE( execTime > C1.getInterval());
            REQUIRE( ms.count() == 40 );
        }
        else // when the funciton isn't run, the exec time should be 0
        {
            REQUIRE( ms.count() == 0 );
        }
        isRun = false;
    }
}
