#include <catch2/catch.hpp>
#include <iostream>
#include <gul/uri.h>

using URI = gul::uri;

SCENARIO("regex test 1")
{
                     //        userinfo       host      port
                     //        ┌──┴───┐ ┌──────┴──────┐ ┌┴┐
    std::string str = "https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top";
                     //└─┬─┘   └───────────┬──────────────┘└───────┬───────┘ └───────────┬─────────────┘ └┬┘
                     //scheme          authority                  path                 query           fragment

    URI uri(str);

    REQUIRE( uri.scheme == "https");
    REQUIRE( uri.user == "john.doe");
    REQUIRE( uri.host == "www.example.com");
    REQUIRE( uri.password == "");
    REQUIRE( uri.port == "123");
    REQUIRE( uri.path == "/forum/questions/");
    REQUIRE( uri.query == "tag=networking&order=newest");
    REQUIRE( uri.fragment == "top");

    REQUIRE( uri.toString() == str);
}

SCENARIO("regex test 2")
{
    std::string str = "ldap://[2001:db8::7]/c=GB?objectClass?one";
                    // └┬─┘   └─────┬─────┘└─┬─┘ └──────┬──────┘
                    // scheme   authority   path      query
    URI uri(str);

    REQUIRE( uri.scheme == "ldap");
    REQUIRE( uri.user == "");
    REQUIRE( uri.host == "[2001:db8::7]");
    REQUIRE( uri.password == "");
    REQUIRE( uri.port == "");
    REQUIRE( uri.path == "/c=GB");
    REQUIRE( uri.query == "objectClass?one");
    REQUIRE( uri.fragment == "");

    REQUIRE( uri.toString() == str);

}

SCENARIO("regex test 3")
{
    std::string str = "urn:oasis:names:specification:docbook:dtd:xml:4.1.2";
                    // └┬┘ └──────────────────────┬──────────────────────┘
                    // scheme                    path
    URI uri(str);

    REQUIRE( uri.scheme == "urn");
    REQUIRE( uri.user == "");
    REQUIRE( uri.host == "");
    REQUIRE( uri.password == "");
    REQUIRE( uri.port == "");
    REQUIRE( uri.path == "oasis:names:specification:docbook:dtd:xml:4.1.2");
    REQUIRE( uri.query == "");
    REQUIRE( uri.fragment == "");

    REQUIRE( uri.toString() == str);
}

SCENARIO("regex test 4")
{
    std::string str = "data:application/octet-stream;base64,AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA";
                    // └┬┘ └──────────────────────┬───────────────────────────────────────┘
                    // scheme                    path
    URI uri(str);

    REQUIRE( uri.scheme == "data");
    REQUIRE( uri.user == "");
    REQUIRE( uri.host == "");
    REQUIRE( uri.password == "");
    REQUIRE( uri.port == "");
    REQUIRE( uri.path == "application/octet-stream;base64,AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA");
    REQUIRE( uri.query == "");
    REQUIRE( uri.fragment == "");

    REQUIRE( uri.toString() == str);
}


SCENARIO("regex test 5")
{
    std::string str = "file:/test.txt";
                    // └┬┘ └──────────────────────┬───────────────────────────────────────┘
                    // scheme                    path
    URI uri(str);

    REQUIRE( uri.scheme == "file");
    REQUIRE( uri.user == "");
    REQUIRE( uri.host == "");
    REQUIRE( uri.password == "");
    REQUIRE( uri.port == "");
    REQUIRE( uri.path == "/test.txt");
    REQUIRE( uri.query == "");
    REQUIRE( uri.fragment == "");

    REQUIRE( uri.toString() == str);
}


SCENARIO("regex test 6: ssh absolute path")
{
    std::string str = "ssh://username:password@localhost/home/user";
                    // └┬┘ └──────────────────────┬───────────────────────────────────────┘
                    // scheme                    path
    URI uri(str);

    REQUIRE( uri.scheme == "ssh");
    REQUIRE( uri.user == "username");
    REQUIRE( uri.password == "password");
    REQUIRE( uri.host == "localhost");
    REQUIRE( uri.port == "");
    REQUIRE( uri.path == "/home/user");
    REQUIRE( uri.query == "");
    REQUIRE( uri.fragment == "");

    REQUIRE( uri.toString() == str);
}

SCENARIO("regex test 7: testing paths and authority")
{
    {
        std::string str = "file:///this.txt";

        URI uri(str);

        REQUIRE( uri.scheme == "file");

        REQUIRE( uri.getAuthority() == "");

        REQUIRE( uri.user == "");
        REQUIRE( uri.password == "");
        REQUIRE( uri.host == "");
        REQUIRE( uri.port == "");
        REQUIRE( uri.path == "/this.txt");
        REQUIRE( uri.query == "");
        REQUIRE( uri.fragment == "");
    }

    // This is an invalid
    {
        std::string str = "file://this.txt";

        URI uri(str);

        REQUIRE( uri.scheme == "file");

        REQUIRE( uri.getAuthority() == "this.txt");

        REQUIRE( uri.user == "");
        REQUIRE( uri.password == "");
        REQUIRE( uri.host == "this.txt");
        REQUIRE( uri.port == "");
        REQUIRE( uri.path == "");
        REQUIRE( uri.query == "");
        REQUIRE( uri.fragment == "");
    }


}


SCENARIO("Data URI")
{
    //data:<mediatype>[;base64],<data>
    std::string str = "data:text/plain;base64,SGVsbG8sIFdvcmxkIQ==";

    URI uri(str);

    REQUIRE( uri.scheme == "data");

    REQUIRE( uri.path == "text/plain;base64,SGVsbG8sIFdvcmxkIQ==");

    THEN("We can get string views to parts of the path")
    {
        REQUIRE( uri.getMediaEncoding() == "text/plain;base64");
        REQUIRE( uri.getMediaType() == "base64");
        REQUIRE( uri.getMediaData() == "SGVsbG8sIFdvcmxkIQ==");
    }
}
