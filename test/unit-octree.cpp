#include <catch2/catch.hpp>
#include <iostream>

#include <thread>
#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <condition_variable>
#include <gul/math/Octree.h>
#include <gul/math/frustum.h>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

SCENARIO("TEST")
{
    gul::Octree<uint32_t> Node( {0,0,0}, std::pow(2.f,20.f));

    Node.insert( 3, {{10,10,10},{20,20,20}});

    size_t count=0;
    gul::bb3f BBB{{0,0,0}, {1,1,1}};
    Node.query<gul::bb3f>(BBB, [&](auto & node)
    {
        count++;
        (void)node;
    });

    REQUIRE( Node.size() == 1);
    REQUIRE(count==0);

    count=0;
    Node.query<gul::bb3f>(gul::bb3f{{0,0,0}, {50,50,50}}, [&](auto & node)
    {
        count++;
        (void)node;
    });

    REQUIRE(count==1);


    Node.erase(3);
    REQUIRE(Node.size()==0);
    count=0;
    Node.query<gul::bb3f>(gul::bb3f{{0,0,0}, {50,50,50}}, [&](auto & node)
    {
        count++;
        (void)node;
    });
    REQUIRE(count==0);
}

SCENARIO("TEST2")
{
    gul::Octree<uint32_t> Node( {0,0,0}, std::pow(2.f,20.f));

    gul::bb3f objSize( glm::vec3{-0.5f}, glm::vec3{0.5f});

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-500, 500);


    for(uint32_t i=0;i<10000;i++)
    {
        auto pos = objSize;
        pos.translate( {dist(gen), dist(gen),dist(gen)});
        Node.insert( i, pos);
    }

    REQUIRE(Node.size() == 10000);

    size_t count=0;
    Node.query<gul::bb3f>(gul::bb3f{{-600,-600,-600}, {600,600,600}}, [&](auto & node)
    {
        count++;
        (void)node;
    });

    REQUIRE( count == 10000);


    auto P = glm::perspective( glm::radians(90.0f), 4.0f/3.0f, 0.1f, 1000.f);
    gul::frustum F(P);

    count=0;
    Node.query<gul::frustum>(F, [&](auto & node)
    {
        count++;
        (void)node;
    });
    REQUIRE( count > 0);
    REQUIRE( count < 10000);

}


SCENARIO("TEST3")
{
    gul::Octree<uint32_t> Node( {0,0,0}, std::pow(2.f,20.f));

    gul::bb3f objSize( glm::vec3{-0.5f}, glm::vec3{0.5f});

   {
        auto pos = objSize;
        pos.translate( {0,0,-10});
        Node.insert( 0, pos);
    }

    auto P = glm::perspective( glm::radians(90.0f), 4.0f/3.0f, 0.1f, 100.f);
    gul::frustum F(P);
    auto T = glm::translate(glm::mat4(1.0f), glm::vec3(100,0,0));
    F.transform(T);
    size_t count=0;
    Node.query<gul::frustum>(F, [&](auto & node)
    {
        count++;
        (void)node;
    });
    REQUIRE( count == 1);

}
