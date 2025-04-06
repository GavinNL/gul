#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch_all.hpp>
#include <iostream>
#include <gul/math/Transform.h>
#include <glm/gtx/io.hpp>
#include <glm/gtc/random.hpp>

SCENARIO("test")
{
    gul::Transform T;

    auto M = T.getMatrix();


    REQUIRE( M[0][0] == Catch::Approx(1.0f));
    REQUIRE( M[1][1] == Catch::Approx(1.0f));
    REQUIRE( M[2][2] == Catch::Approx(1.0f));
    REQUIRE( M[3][3] == Catch::Approx(1.0f));
}

glm::mat4 matrix_multiply(gul::Transform const &T)
{
    return glm::translate(  glm::mat4(1.0f), T.position) * glm::mat4_cast(T.rotation) * glm::scale( glm::mat4(1.0), T.scale);
}

glm::mat4 sneaky(gul::Transform const &T)
{
    auto M = glm::mat4_cast(T.rotation);
    M[0] *= T.scale[0];
    M[1] *= T.scale[1];
    M[2] *= T.scale[2];
    M[3] = glm::vec4(T.position,1.0f);
    return M;
}

SCENARIO("Testing Transform::getMatrix()")
{
    gul::Transform T;

    T.lookat( {5,5,5}, {0,1,0});
    T.position = {3,3,3};
    T.scale = {2,3,4};

    BENCHMARK("Matrix Multiply")
    {
        return matrix_multiply(T);
    };

    BENCHMARK("Sneaky")
    {
        return sneaky(T);
    };

    BENCHMARK("T.getMatrix()")
    {
        return T.getMatrix();
    };


    auto V = glm::linearRand( glm::vec3(-10), glm::vec3(10));

    BENCHMARK("T * v")
    {
        return T*V;
    };

    auto N = T.getMatrix();
    BENCHMARK("M * v")
    {
        return glm::vec3(N*glm::vec4(V,1.0f));
    };

    BENCHMARK("T.getMatrix() * v")
    {
        return glm::vec3(T.getMatrix()*glm::vec4(V,1.0f));
    };

    // Test that the T.getMatrix() method produces the same
    // transformed vector
    for(int i=0;i<100;i++)
    {
        auto v = glm::linearRand( glm::vec3(-10), glm::vec3(10));

        auto M1 = matrix_multiply(T);
        auto M2 = sneaky(T);
        auto M3 = T.getMatrix();

        auto a = M1 * glm::vec4(v,1.0);
        auto b = M2 * glm::vec4(v,1.0);
        auto c = M3 * glm::vec4(v,1.0);

        auto d = T * v;

        REQUIRE( a.x == Catch::Approx(b.x));
        REQUIRE( a.y == Catch::Approx(b.y));
        REQUIRE( a.z == Catch::Approx(b.z));

        REQUIRE( c.x == Catch::Approx(b.x));
        REQUIRE( c.y == Catch::Approx(b.y));
        REQUIRE( c.z == Catch::Approx(b.z));

        REQUIRE( a.x == Catch::Approx(d.x).epsilon(0.1));
        REQUIRE( a.y == Catch::Approx(d.y).epsilon(0.1));
        REQUIRE( a.z == Catch::Approx(d.z).epsilon(0.1));
    }


}
