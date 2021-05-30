#include <catch2/catch.hpp>

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wkeyword-macro"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#elif defined(_MSC_VER)

#endif


#define private public
#include <gul/Image.h>
#undef private
#include <iostream>

using namespace gul;

TEST_CASE("Accessing Pixels")
{
    gul::Image I;

    I.resize(10,10);

    REQUIRE( std::distance( &I.r(0,0) , &I.g(0,0 )) == 1 );
    REQUIRE( std::distance( &I.r(0,0) , &I.b(0,0 )) == 2 );
    REQUIRE( std::distance( &I.r(0,0) , &I.a(0,0 )) == 3 );

    WHEN("We make the first pixel equal to 0xAABBCCDD")
    {
        I(0,0,0) = 0xDD;
        I(0,0,1) = 0xCC;
        I(0,0,2) = 0xBB;
        I(0,0,3) = 0xAA;

        I(5,5,0) = 0x44;
        I(5,5,1) = 0x33;
        I(5,5,2) = 0x22;
        I(5,5,3) = 0x11;

        THEN("We can access each component")
        {
            REQUIRE( I.r(0,0) == 0xDD );
            REQUIRE( I.g(0,0) == 0xCC );
            REQUIRE( I.b(0,0) == 0xBB );
            REQUIRE( I.a(0,0) == 0xAA );

            REQUIRE( I.r(5,5) == 0x44 );
            REQUIRE( I.g(5,5) == 0x33 );
            REQUIRE( I.b(5,5) == 0x22 );
            REQUIRE( I.a(5,5) == 0x11 );
        }
    }
}

TEST_CASE("Constructors")
{
    gul::Image I;
    I.resize(10,10);
    I.r = 0xAA;
    I.g = 0xBB;
    I.b = 0xCC;
    I.a = 0xDD;

    WHEN("We copy construct")
    {
        gul::Image J(I);

        REQUIRE(I.getWidth() == J.getWidth());
        REQUIRE(I.getHeight() == J.getHeight());

        for(uint32_t v=0;v<10;v++)
        {
            for(uint32_t u=0;u<10;u++)
            {
                REQUIRE( J.r(u,v) == 0xAA );
                REQUIRE( J.g(u,v) == 0xBB );
                REQUIRE( J.b(u,v) == 0xCC );
                REQUIRE( J.a(u,v) == 0xDD );
            }
        }
    }
    WHEN("We copy")
    {
        gul::Image J;
        J.resize(10,10);

        J = I;
        REQUIRE(I.getWidth() == J.getWidth());
        REQUIRE(I.getHeight() == J.getHeight());

        for(uint32_t v=0;v<10;v++)
        {
            for(uint32_t u=0;u<10;u++)
            {
                REQUIRE( J.r(u,v) == 0xAA );
                REQUIRE( J.g(u,v) == 0xBB );
                REQUIRE( J.b(u,v) == 0xCC );
                REQUIRE( J.a(u,v) == 0xDD );
            }
        }
    }

    WHEN("We move")
    {
        gul::Image J;
        J.resize(10,10);

        gul::Image K(I);

        J = std::move(K);

        REQUIRE(I.getWidth() == J.getWidth());
        REQUIRE(I.getHeight() == J.getHeight());

        for(uint32_t v=0;v<10;v++)
        {
            for(uint32_t u=0;u<10;u++)
            {
                REQUIRE( J.r(u,v) == 0xAA );
                REQUIRE( J.g(u,v) == 0xBB );
                REQUIRE( J.b(u,v) == 0xCC );
                REQUIRE( J.a(u,v) == 0xDD );
            }
        }
    }

    WHEN("We move Construct")
    {
        gul::Image K(I);


        gul::Image J(std::move(K));

        REQUIRE(I.getWidth() == J.getWidth());
        REQUIRE(I.getHeight() == J.getHeight());

        for(uint32_t v=0;v<10;v++)
        {
            for(uint32_t u=0;u<10;u++)
            {
                REQUIRE( J.r(u,v) == 0xAA );
                REQUIRE( J.g(u,v) == 0xBB );
                REQUIRE( J.b(u,v) == 0xCC );
                REQUIRE( J.a(u,v) == 0xDD );
            }
        }
    }
}

TEST_CASE("Copying Channels")
{
    gul::Image I;

    I.resize(10,10);

    WHEN("Set a channel equal to a constant")
    {
        I.r = 0xAA;
        I.g = 0xBB;
        I.b = 0xCC;
        I.a = 0xDD;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 0xAA );
                    REQUIRE( I.g(u,v) == 0xBB );
                    REQUIRE( I.b(u,v) == 0xCC );
                    REQUIRE( I.a(u,v) == 0xDD );
                }
            }
        }
    }
    WHEN("Set a channel equal to a floating point constant")
    {
        I.r = 0.25f;
        I.g = 0.5f;
        I.b = 0.75f;
        I.a = 1.0f;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 63 );
                    REQUIRE( I.g(u,v) == 127 );
                    REQUIRE( I.b(u,v) == 191 );
                    REQUIRE( I.a(u,v) == 255 );
                }
            }
        }
    }
    WHEN("Copy one channel into another")
    {
        I.r = 0xAA;
        I.g = 0x00;
        I.b = 0x00;
        I.a = 0x00;

        I.g = I.r;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    //std::cout << u << ", " << v << ": " << std::hex << I(u,v) << std::endl;
                    REQUIRE( I.r(u,v) == 0xAA );
                    REQUIRE( I.g(u,v) == 0xAA );
                    REQUIRE( I.b(u,v) == 0x00 );
                    REQUIRE( I.a(u,v) == 0x00 );
                }
            }
        }
    }
}

TEST_CASE("Adding two channels")
{
    gul::Image I;

    I.resize(10,10);

    I.r = 0x10;
    I.g = 0x20;
    WHEN("Set a channel equal to a constant")
    {
        I.a = I.r + I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 0x10 );
                    REQUIRE( I.g(u,v) == 0x20 );

                    REQUIRE( I.a(u,v) == (0x10+0x20) );
                }
            }
        }
    }
}

TEST_CASE("Multiplying two channels treats each compoinent as a float  between 0-1")
{
    gul::Image I;

    I.resize(10,10);


    WHEN("We multiply by 255*255")
    {
        I.r = 255;
        I.g = 255;
        I.b = 0;

        I.a = I.r * I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 255 );
                    REQUIRE( I.g(u,v) == 255 );
                    REQUIRE( I.b(u,v) == 0 );
                    REQUIRE( I.a(u,v) == 255 );
                }
            }
        }
    }
    WHEN("We multiply by 255*0")
    {
        I.r = 255;
        I.g = 0;
        I.b = 0;

        I.a = I.r * I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 255 );
                    REQUIRE( I.g(u,v) == 0 );
                    REQUIRE( I.b(u,v) == 0 );
                    REQUIRE( I.a(u,v) == 0 );
                }
            }
        }
    }
    WHEN("We multiply by 255*127 == 127")
    {
        I.r = 255;
        I.g = 127;
        I.b = 0;

        I.a = I.r * I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 255 );
                    REQUIRE( I.g(u,v) == 127 );
                    REQUIRE( I.b(u,v) == 0 );
                    REQUIRE( I.a(u,v) == 127 );
                }
            }
        }
    }
    WHEN("We multiply by 127*127 == 63")
    {
        I.r = 127;
        I.g = 127;
        I.b = 0;

        I.a = I.r * I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.r(u,v) == 127 );
                    REQUIRE( I.g(u,v) == 127 );
                    REQUIRE( I.b(u,v) == 0 );
                    REQUIRE( I.a(u,v) == 63 );
                }
            }
        }
    }
}


TEST_CASE("AXBY on two channels")
{
    gul::Image I;

    I.resize(10,10);

    I.r = 0x08;
    I.g = 0x08;
    I.b = 255;

    WHEN("We perform some math")
    {
        I.a = 0.5f*I.r + 0.25f*I.g + 0.25f*I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 0x04+0x02+0x02);
                }
            }
        }
    }
    WHEN("We perform some math")
    {
        I.a = I.r*0.5f + 0.25f*I.g + 0.25f*I.g + 0.5f;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 0x04+0x02+0x02 + 127);
                }
            }
        }
    }
    WHEN("We perform some math")
    {
        I.a = I.r*0.5f + 0.5f + 0.25f*I.g + 0.25f*I.g;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 0x04+0x02+0x02 + 127);
                }
            }
        }
    }

}


TEST_CASE("Adding number to Channel")
{
    gul::Image I;

    I.resize(10,10);

    I.r = 0;
    I.g = 0;
    I.b = 0;

    WHEN("We add a number to a channel")
    {
        I.a = I.r + 0.5f;

        THEN("All the values in that channel are increased by that value")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 127);
                    REQUIRE( I.r(u,v) == 0);
                    REQUIRE( I.g(u,v) == 0);
                    REQUIRE( I.b(u,v) == 0);
                }
            }
        }
    }
    WHEN("We add a number to a channel")
    {
        I.a = 0.5f + I.r;

        THEN("All the values in that channel are increased by that value")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 127);
                    REQUIRE( I.r(u,v) == 0);
                    REQUIRE( I.g(u,v) == 0);
                    REQUIRE( I.b(u,v) == 0);
                }
            }
        }
    }
}

TEST_CASE("Subtracting number from a Channel")
{
    gul::Image I;

    I.resize(10,10);

    I.r = 255;
    I.g = 0;
    I.b = 0;

    WHEN("We subtract a number from a channel")
    {
        I.a = I.r - 0.5f;
        uint8_t actual_value = 127;// - static_cast<uint8_t>(255.0f*0.5f);
        THEN("All the values in that channel are decreased by that value")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == actual_value);
                    REQUIRE( I.r(u,v) == 255);
                    REQUIRE( I.g(u,v) == 0);
                    REQUIRE( I.b(u,v) == 0);
                }
            }
        }
    }
    WHEN("We subtract a channel from a number")
    {
        I.g = 128;
        I.a = 1.0f - I.g;

        uint8_t actual_value = static_cast<uint8_t>( (1.0f - 128.0f/255.0f)*255 );
        THEN("All the values in that channel are increased by that value")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == actual_value);
                    REQUIRE( I.r(u,v) == 255);
                    REQUIRE( I.g(u,v) == 128);
                    REQUIRE( I.b(u,v) == 0);
                }
            }
        }
    }
}

TEST_CASE("Multiplying a number by a channel")
{
    gul::Image I;

    I.resize(10,10);

    I.r = 10;
    I.g = 0;
    I.b = 0;

    WHEN("We multi a number from a channel")
    {
        I.a = 2.0f*I.r;

        THEN("All the values in that channel are decreased by that value")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 20);
                    REQUIRE( I.r(u,v) == 10);
                    REQUIRE( I.g(u,v) == 0);
                    REQUIRE( I.b(u,v) == 0);
                }
            }
        }
    }
    WHEN("We multi a number from a channel")
    {
        I.a = I.r*2.0f;
        auto x = I.r*2.0f;

        THEN("All the values in that channel are decreased by that value")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 20);
                    REQUIRE( I.r(u,v) == 10);
                    REQUIRE( I.g(u,v) == 0);
                    REQUIRE( I.b(u,v) == 0);
                }
            }
        }
    }
}

TEST_CASE("Basic Math on two channels")
{
    gul::Image I;

    I.resize(10,10);

    I.r = 0;
    I.g = 100;
    I.b = 255;

    WHEN("We perform some math")
    {
        I.a = I.b - 1.0f;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 0);
                }
            }
        }
    }
    WHEN("We perform some math")
    {
        I.a = -1.0f + I.b;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 0);
                }
            }
        }
    }
    WHEN("We perform some math")
    {
        I.a = I.b * 0.5f;

        THEN("All the values in that channel becomes the same")
        {
            for(uint32_t v=0;v<10;v++)
            {
                for(uint32_t u=0;u<10;u++)
                {
                    REQUIRE( I.a(u,v) == 127);
                }
            }
        }
    }
}


TEST_CASE("Static functions")
{
    auto x = gul::Image::X(255,255);
    auto y = gul::Image::Y(255,255);

    THEN("All the values in that channel becomes the same")
    {
        for(uint32_t v=0;v<255;v++)
        {
            for(uint32_t u=0;u<255;u++)
            {
                REQUIRE( uint32_t(x(u,v)*255) == u);
                REQUIRE( uint32_t(y(u,v)*255) == v);
            }
        }
    }
}


TEST_CASE("Mix functions")
{
    gul::Image I;

    I.resize(10,10);

    THEN("All the values in that channel becomes the same")
    {
        I.r = 10;
        I.g = 20;

        I.a = mix( I.r, I.g, 0.5f);
        for(uint32_t v=0;v<10;v++)
        {
            for(uint32_t u=0;u<10;u++)
            {
                REQUIRE( I.a(u,v) == 15);
            }
        }
    }
    THEN("All the values in that channel becomes the same")
    {
        I.r = 10;
        I.g = 20;
        I.b = 128;
        I.a = mix( I.r, I.g, I.b);
        for(uint32_t v=0;v<10;v++)
        {
            for(uint32_t u=0;u<10;u++)
            {
                REQUIRE( I.a(u,v) == 15);
            }
        }
    }
}


TEST_CASE("apply functions")
{
    gul::Image I;

    I.resize(255,255);


    THEN("All the values in that channel becomes the same")
    {
        I.r.apply(  [](float u, float v)
                    {
                        (void)v;
                        return u;
                    }
                 );
        I.g.apply(  [](float u, float v)
                    {
                        (void)u;
                        return v;
                    }
                 );
        I.b =  [](float u, float v)
                    {
                        (void)u;
                        return v;
                    };
        for(uint32_t v=0;v<255;v++)
        {
            for(uint32_t u=0;u<255;u++)
            {
                REQUIRE( I.r(u,v) == u);
                REQUIRE( I.g(u,v) == v);
                REQUIRE( I.b(u,v) == v);
            }
        }
    }
}


SCENARIO("Test 3-channel")
{
    gul::Image I(8,8,3);

    REQUIRE( I.size() == 8*8*3 );

    // +-+-+-+-+-+-+
    // |r|g|b|r|g|b|
    // +-+-+-+-+-+-+
    REQUIRE( std::distance( &I(0,0,0), &I(1,0,0) ) == 3 );
    REQUIRE( std::distance( &I(0,0,0), &I(1,0,1) ) == 4 );
    REQUIRE( std::distance( &I(0,0,0), &I(1,0,2) ) == 5 );


    REQUIRE( std::distance( &I(0,0,0), &I.r(1,0) ) == 3 );
    REQUIRE( std::distance( &I(0,0,0), &I.g(1,0) ) == 4 );
    REQUIRE( std::distance( &I(0,0,0), &I.b(1,0) ) == 5 );
    REQUIRE( std::distance( &I(0,0,0), &I.a(1,0) ) == 5 );
}

SCENARIO("Test 2-channel")
{
    gul::Image I(8,8,2);

    REQUIRE( I.size() == 8*8*2 );

    // +-+-+-+-+-+-+
    // |r|g|r|g|r|g|
    // +-+-+-+-+-+-+
    REQUIRE( std::distance( &I(0,0,0), &I(1,0,0) ) == 2 );
    REQUIRE( std::distance( &I(0,0,0), &I(1,0,1) ) == 3 );

    // blue and alpha both point to green
    REQUIRE( std::distance( &I(0,0,0), &I.r(1,0) ) == 2 );
    REQUIRE( std::distance( &I(0,0,0), &I.g(1,0) ) == 3 );
    REQUIRE( std::distance( &I(0,0,0), &I.b(1,0) ) == 3 );
    REQUIRE( std::distance( &I(0,0,0), &I.a(1,0) ) == 3 );
}

SCENARIO("Test 1-channel")
{
    gul::Image I(8,8,1);

    REQUIRE( I.size() == 8*8*1 );

    // +-+-+-+-+-+-+
    // |r|g|r|g|r|g|
    // +-+-+-+-+-+-+
    REQUIRE( std::distance( &I(0,0,0), &I(1,0,0) ) == 1 );
    REQUIRE( std::distance( &I(0,0,0), &I(2,0,0) ) == 2 );
    REQUIRE( std::distance( &I(0,0,0), &I(3,0,0) ) == 3 );

    // all point to the same channel (r)
    REQUIRE( std::distance( &I(0,0,0), &I.r(1,0) ) == 1 );
    REQUIRE( std::distance( &I(0,0,0), &I.g(1,0) ) == 1 );
    REQUIRE( std::distance( &I(0,0,0), &I.b(1,0) ) == 1 );
    REQUIRE( std::distance( &I(0,0,0), &I.a(1,0) ) == 1 );
}




SCENARIO("gul::ImageMM")
{
    gul::ImageMM MM;

    MM.resize(16,16);
    MM.allocateMipMaps();

    REQUIRE( MM.getLevelCount() == 4);
}

SCENARIO("gul::ImageArray")
{
    gul::ImageArray MM;

    MM.resize(16,16);
    MM.allocateMipMaps();

    REQUIRE( MM.getLevelCount() == 4);
}
