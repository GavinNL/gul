#include <catch2/catch.hpp>
#include <iostream>
#include <thread>
#include <gul/net/curl.h>


SCENARIO("Test download file")
{
    gul::CURL curl;

    // for the gcc7 build its complaining
    curl.CURL_ADDITIONAL_FLAGS += " --insecure ";

    REQUIRE( gul::fs::exists( curl.CURL_PATH));

    auto path = curl.cache_file("https://ifconfig.me");

    auto path2 = curl.get("https://ifconfig.me");

    REQUIRE(path == path2);
    REQUIRE(gul::fs::exists(path2));
}

