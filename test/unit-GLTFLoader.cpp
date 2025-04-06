#include <catch2/catch_all.hpp>
#include <iostream>
#include <gul/GLTFLoader.h>

SCENARIO("No test")
{
    REQUIRE(true);
}

#if 0
SCENARIO("Stride Copy")
{
    auto filepath = gul::fs::path("/home/globo/Home-Projects/glTF-Sample-Models/2.0/Suzanne/glTF/Suzanne.gltf");

    std::ifstream in(filepath);
    REQUIRE(in);

    auto G = gul::loadGLTF(in, filepath.parent_path().native());

//    /home/globo/Home-Projects/glTF-Sample-Models/2.0/CesiumMan/glTF-Binary/CesiumMan.glb
}


SCENARIO("Binary loading")
{
    auto filepath = gul::fs::path("/home/globo/Home-Projects/glTF-Sample-Models/2.0/CesiumMan/glTF-Binary/CesiumMan.glb");
    //auto filepath = "/home/globo/Home-Projects/glTF-Sample-Models/2.0/CesiumMan/glTF-Binary/CesiumMan.glb";

    std::ifstream in(filepath);
    REQUIRE(in);

    auto G = gul::loadGLTF(in, filepath.parent_path().native());

//
}

#endif


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
