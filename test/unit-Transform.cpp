#include <catch2/catch.hpp>
#include <gul/math/Transform.h>


SCENARIO("test")
{
    gul::Transform T;

    auto M = T.getMatrix();

    REQUIRE( M[0][0] == Approx(1.0f));
    REQUIRE( M[1][1] == Approx(1.0f));
    REQUIRE( M[2][2] == Approx(1.0f));
    REQUIRE( M[3][3] == Approx(1.0f));
}
