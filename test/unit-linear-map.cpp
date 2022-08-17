#include <catch2/catch.hpp>
#include <iostream>

#include <gul/LinearMap.h>


SCENARIO("Test LinearMap")
{
    struct D
    {
        int x;
        int y;
    };

    gul::LinearMap<std::string, D> M;

    THEN("Inserting and removing")
    {
        auto index = M.insert("Gavin", D{1,2});
        REQUIRE(index == 0);

        REQUIRE(M.size() == 1);
        REQUIRE(M.capacity() == 1);

        THEN("We can access the data")
        {
            REQUIRE(M["Gavin"].x == 1);
            REQUIRE(M["Gavin"].y == 2);
        }

        M.erase("Gavin");

        REQUIRE(M.size() == 0);
        REQUIRE(M.capacity() == 1);
    }

    THEN("Key lookup")
    {
        M["Gavin"] = D{1,2};

        REQUIRE(M.size() == 1);
        REQUIRE(M.capacity() == 1);

        THEN("We can access the data")
        {
            REQUIRE(M["Gavin"].x == 1);
            REQUIRE(M["Gavin"].y == 2);
        }

        M.erase("Gavin");

        REQUIRE(M.size() == 0);
        REQUIRE(M.capacity() == 1);
    }

    THEN("Index lookup")
    {
        M["Batman"] = D{1,7};
        M["Superman"] = D{2,8};
        M["WonderWoman"] = D{3,8};
        M["GreenLantern"] = D{4,10};
        M["Flash"] = D{5,11};
        M["MartianManhunter"] = D{6,12};

        REQUIRE(M.size() == 6);
        REQUIRE(M.capacity() == 6);

        REQUIRE(M.array().size() == M.capacity());

        WHEN("We access a non-existant key using at() ")
        {
            REQUIRE_THROWS( M.at("Cyborg"));
        }


        WHEN("We erase a key")
        {
            auto sI = M.findIndex("Superman");
            M.erase("Superman");
            REQUIRE(M.size() == 5);
            REQUIRE(M.capacity() == 6);

            THEN("The index can be reused for the next value")
            {
                auto sC = M.insert("Cyborg", D{5,5});
                REQUIRE( sC == sI);

                REQUIRE(M.size() == 6);
                REQUIRE(M.capacity() == 6);
            }

        }
    }

    THEN("Defragment")
    {
        M["Batman"] = D{1,7};
        M["Superman"] = D{2,8};
        M["WonderWoman"] = D{3,8};
        M["GreenLantern"] = D{4,10};
        M["Flash"] = D{5,11};
        M["MartianManhunter"] = D{6,12};

        REQUIRE(M.size() == 6);
        REQUIRE(M.capacity() == 6);

        M.erase("GreenLantern");
        REQUIRE(M.size() == 5);
        REQUIRE(M.capacity() == 6);

        M.defragment();
        REQUIRE(M.size() == 5);
        REQUIRE(M.capacity() == 5);

    }
}
