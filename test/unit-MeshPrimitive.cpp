#include <catch2/catch.hpp>
#include <iostream>
#include <gul/MeshPrimitive.h>

SCENARIO("Stride Copy")
{
    GIVEN("A vertex attribute with 3 items")
    {
        gul::VertexAttribute_v V = std::vector<uint32_t>( {1,2,3});

        WHEN("We do a strideCopy with a stride size of 2*sizeof(uint32_t)")
        {
            std::vector<uint32_t> D = {0,0,0,0,0,0};

            gul::VertexAttributeStrideCopy(D.data(), V, 2*sizeof(uint32_t));

            THEN("Every other value is copied")
            {
                REQUIRE( D[0] == 1);
                REQUIRE( D[1] == 0);
                REQUIRE( D[2] == 2);
                REQUIRE( D[3] == 0);
                REQUIRE( D[4] == 3);
                REQUIRE( D[5] == 0);
            }
        }
    }
}

SCENARIO("Copy Interleaved")
{
    GIVEN("Two vertex attributes")
    {
        gul::VertexAttribute_v V1  = std::vector<glm::uvec2>( {{1,2}, {3,4}});
        gul::VertexAttribute_v V2  = std::vector<glm::uvec3>( {{5,6,7}, {8,9,10}});

        WHEN("We copyInterleaved with the two attributes")
        {
            std::vector<uint32_t> D(100);

            gul::VertexAttributeInterleaved(D.data(), {&V1,&V2});

            THEN("Every other value is copied")
            {
                REQUIRE( D[0] == 1); //V1[0].x
                REQUIRE( D[1] == 2); //V1[0].y
                REQUIRE( D[2] == 5); //V2[0].x
                REQUIRE( D[3] == 6); //V2[0].y
                REQUIRE( D[4] == 7); //V2[0].z

                REQUIRE( D[5] == 3); //V1[1].x
                REQUIRE( D[6] == 4); //V1[1].y
                REQUIRE( D[7] == 8); //V2[1].x
                REQUIRE( D[8] == 9); //V2[1].y
                REQUIRE( D[9] == 10);//V2[1].z
            }
        }
    }
}

SCENARIO("Copy Sequential")
{
    GIVEN("Two vertex attributes")
    {
        gul::VertexAttribute_v V1  = std::vector<glm::uvec2>( {{1,2}, {3,4}});
        gul::VertexAttribute_v V2  = std::vector<glm::uvec3>( {{5,6,7}, {8,9,10}});

        WHEN("We copySequential with the two attributes")
        {
            std::vector<uint32_t> D(100);

            auto offsets = gul::VertexAttributeCopySequential(D.data(), {&V1,&V2});

            REQUIRE( offsets[0] == 0);
            REQUIRE( offsets[1] == sizeof(glm::uvec2)*2);

            THEN("Attribute vectors are copied as if they were appended")
            {
                REQUIRE( D[0] == 1); //V1[0].x
                REQUIRE( D[1] == 2); //V1[0].y
                REQUIRE( D[2] == 3); //V1[1].x
                REQUIRE( D[3] == 4); //V1[1].y

                REQUIRE( D[4] == 5); //V2[0].z
                REQUIRE( D[5] == 6); //V2[0].x
                REQUIRE( D[6] == 7); //V2[0].y
                REQUIRE( D[7] == 8); //V2[1].x
                REQUIRE( D[8] == 9); //V2[1].y
                REQUIRE( D[9] == 10);//V2[1].z
            }
        }
    }
}

SCENARIO("Copy Sequential with nullptr")
{
    GIVEN("Two vertex attributes")
    {
        gul::VertexAttribute_v V1  = std::vector<glm::uvec2>( {{1,2}, {3,4}});
        gul::VertexAttribute_v V2  = std::vector<glm::uvec3>();
        gul::VertexAttribute_v V3  = std::vector<glm::uvec3>( {{5,6,7}, {8,9,10}});

        WHEN("We copySequential with the three attributes, and one of them has zero attributes")
        {
            std::vector<uint32_t> D(100);

            auto offsets = gul::VertexAttributeCopySequential(D.data(), {&V1, &V2, &V3});

            THEN("We get an offset vector of size 3")
            {
                REQUIRE( offsets.size() == 3);
            }

            REQUIRE( offsets[0] == 0);

            THEN("The vertex with zero attributes has an offset of zero")
            {
                REQUIRE( offsets[1] == 0);
            }

            REQUIRE( offsets[2] == sizeof(glm::uvec2)*2);

            THEN("Attribute vectors are copied as if they were appended")
            {
                REQUIRE( D[0] == 1); //V1[0].x
                REQUIRE( D[1] == 2); //V1[0].y
                REQUIRE( D[2] == 3); //V1[1].x
                REQUIRE( D[3] == 4); //V1[1].y

                REQUIRE( D[4] == 5); //V2[0].z
                REQUIRE( D[5] == 6); //V2[0].x
                REQUIRE( D[6] == 7); //V2[0].y
                REQUIRE( D[7] == 8); //V2[1].x
                REQUIRE( D[8] == 9); //V2[1].y
                REQUIRE( D[9] == 10);//V2[1].z
            }
        }
    }
}

SCENARIO("Load obj")
{
    std::ifstream in(CMAKE_SOURCE_DIR "/test/data/test.obj");
    auto M = gul::ReadOBJ(in);

    REQUIRE(M.indexCount() == 3);
}
