#include <catch2/catch.hpp>
#include <iostream>
#include <fstream>
#include <gul/ResourceManager.h>

struct TextResource
{
    std::string data;
};
struct IntResource
{
    uint64_t data = 0;
};

SCENARIO("Test")
{
    using namespace gul;

    SingleResourceManager<TextResource> RM;

    const auto RES_URI = uri("file://" CMAKE_SOURCE_DIR "/README.md");

    auto rId = RM.findResource( RES_URI);

    REQUIRE( !rId->isLoaded() );
    REQUIRE( rId->getUri().toString() == RES_URI.toString());

    WHEN("We set the loader")
    {
        RM.setLoader([](uri const & _uri)
        {
            std::ifstream t(_uri.path);
            TextResource R;
            R.data = std::string((std::istreambuf_iterator<char>(t)),
                                  std::istreambuf_iterator<char>());
            return R;
        });

        THEN("We can load the resource")
        {
            rId->load();
            REQUIRE( rId->isLoaded() );
            REQUIRE( rId->get().data.size() > 0 );
        }
        THEN("We can load the resource")
        {
            REQUIRE( !rId->isLoaded());

            auto rId2 = RM.get(RES_URI);

            REQUIRE( rId2 == rId);
            REQUIRE( rId->isLoaded());
            REQUIRE( rId->get().data.size() > 0 );

        }
        THEN("We can load the resource from the handle")
        {
            auto & G = rId->get();
            REQUIRE( G.data.size() > 0);
        }
    }
}


SCENARIO("Resource Manager")
{
    using namespace gul;

    ResourceManager RM;

    RM.setLoader<TextResource>([](uri const & _uri)
    {
        std::ifstream t(_uri.path);
        TextResource R;
        R.data = std::string((std::istreambuf_iterator<char>(t)),
                              std::istreambuf_iterator<char>());
        return R;
    });
    RM.setLoader<IntResource>([](uri const & _uri)
    {
        std::ifstream t(_uri.path);
        IntResource R;
        std::hash<std::string> H;
        R.data = H(_uri.toString());
        return R;
    });
    const auto RES_URI = uri("file://" CMAKE_SOURCE_DIR "/README.md");

    auto rId = RM.findResource<TextResource>( RES_URI);

    REQUIRE( !rId->isLoaded() );
    REQUIRE( rId->getUri().toString() == RES_URI.toString());

    THEN("We can immediately get the resource and load it")
    {
        auto tr = RM.get<TextResource>(RES_URI);
        REQUIRE( tr->isLoaded());
        REQUIRE(tr->get().data.size() > 0);

        auto ir = RM.get<IntResource>(RES_URI);
        REQUIRE( ir->isLoaded() );
        REQUIRE(ir->get().data > 0);
    }
    WHEN("We set the loader")
    {
        THEN("We can load the resource")
        {
            rId->load();
            REQUIRE( rId->isLoaded() );
            REQUIRE( rId->get().data.size() > 0 );
        }
        THEN("We can load the resource from the handle")
        {
            auto & G = rId->get();
            REQUIRE( G.data.size() > 0);
        }
    }
}
