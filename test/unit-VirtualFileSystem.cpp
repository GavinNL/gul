#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <catch2/catch.hpp>
#include <gul/VirtualFileSystem.h>


void tree(gul::VFS const & fs, gul::VFS::vfs_path_type root = "/", std::string prefix="")
{
    //std::cout << root.filename() << std::endl;
    fs.for_each(root, [&](auto const & v)
                {
                    std::cout << prefix << v << std::endl;
                    if(fs.is_directory(root / v))
                    {
                        tree(fs, root/v, prefix + "   ");
                    }
                });
}

SCENARIO("SDF")
{
    gul::VFS fs;

    fs.mount("/A", std::filesystem::path(CMAKE_SOURCE_DIR));
    fs.mkdir("/B");
    REQUIRE(fs.exists("/B"));
    REQUIRE(fs.exists("/A/test/CMakeLists.txt"));
    REQUIRE(fs.exists("/A"));

    //fs.print();
    //fs.list("/A");

}

SCENARIO("SDF2")
{
    gul::VFS fs;

    fs.mount("/src", std::filesystem::path(CMAKE_SOURCE_DIR));
    fs.mount("/src2", std::filesystem::path(CMAKE_SOURCE_DIR));

    {
        auto [mount, stem] = fs.splitMount("/src/cmake/Coverage.cmake");
        REQUIRE(mount == "/src");
        REQUIRE(stem == "cmake/Coverage.cmake");
    }
    {
        auto [mount, stem] = fs.splitMount("/src2/cmake/Coverage.cmake");
        REQUIRE(mount == "/src2");
        REQUIRE(stem == "cmake/Coverage.cmake");
    }

    REQUIRE(fs.exists("/"));
    REQUIRE(fs.exists("/src"));
    REQUIRE(fs.exists("/src/test"));
    REQUIRE(fs.exists("/src/test/CMakeLists.txt")); // exists on filesystem
    REQUIRE(fs.exists("/src/cmake"));
    REQUIRE(fs.exists("/src/cmake/Coverage.cmake"));

    REQUIRE(fs.is_directory("/"));
    REQUIRE(fs.is_directory("/src"));
    REQUIRE(fs.is_directory("/src/test"));

    fs.mkdir("/build");
    fs.mkdir("/test");
    fs.mkdir("/test/hello/world");

    fs.unmount("/src2");
    REQUIRE(!fs.exists("/src2"));
    REQUIRE(!fs.is_directory("/src2"));
    REQUIRE(!fs.is_file("/src2"));
}


SCENARIO("SDF3")
{
    gul::VFS fs;

    fs.mkdir("/res");
    fs.mount("/res/src" , std::filesystem::path(CMAKE_SOURCE_DIR)/"cmake");
    fs.mount("/res/src2", std::filesystem::path(CMAKE_SOURCE_DIR)/"test");

    {
        auto [mount, stem] = fs.splitMount("/res/src/Coverage.cmake");
        REQUIRE(mount == "/res/src");
        REQUIRE(stem == "Coverage.cmake");
    }
    {
        auto [mount, stem] = fs.splitMount("/res/src2/CMakeLists.txt");
        REQUIRE(mount == "/res/src2");
        REQUIRE(stem == "CMakeLists.txt");
    }

    REQUIRE(fs.exists("/"));
    REQUIRE(fs.exists("/res/src"));
    REQUIRE(fs.exists("/res/src/Coverage.cmake"));
    REQUIRE(fs.exists("/res/src2/CMakeLists.txt")); // exists on filesystem

    REQUIRE(fs.is_directory("/"));
    REQUIRE(fs.is_directory("/res/src"));
    REQUIRE(fs.is_directory("/res/src2"));
    //REQUIRE(fs.is_directory("/res/src/test"));

   // tree(fs);
}

SCENARIO("SDF4")
{
    gul::Mount M;
    gul::VFS fs;

    fs.mount("/res", {
                         std::filesystem::path(CMAKE_SOURCE_DIR)/"cmake"
                         ,std::filesystem::path(CMAKE_SOURCE_DIR)/"test"
                     });


    fs.list("/res/data");
    tree(fs);
//    M.hosts.push_back(std::filesystem::path(CMAKE_SOURCE_DIR)/"cmake");
//    M.hosts.push_back(std::filesystem::path(CMAKE_SOURCE_DIR)/"cmake");

  //  M.for_each([](auto v)
  //  {
  //      std::cout << v << std::endl;
  //  });

}
