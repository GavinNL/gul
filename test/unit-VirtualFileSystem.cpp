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
                        tree(fs, root/v, prefix + "     ");
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
    REQUIRE(fs.exists("/src"));
    REQUIRE(fs.exists("/src/test/CMakeLists.txt")); // exists on filesystem
    REQUIRE(fs.exists("/src/cmake/Coverage.cmake"));

    REQUIRE(fs.is_directory("/"));
    REQUIRE(fs.is_directory("/src"));
    REQUIRE(fs.is_directory("/src/test"));

    auto [mount, stem] = fs.splitMount("/src/cmake/Coverage.cmake");
    REQUIRE(mount == "/src");
    REQUIRE(stem == "cmake/Coverage.cmake");

    fs.mkdir("/build");
    fs.mkdir("/test");
    fs.mkdir("/test/hello/world");

}

#if 0
SCENARIO("SDF")
{

    namespace FS = std::filesystem;
    REQUIRE(!gul::VirtualFileSystem::is_base_of("/root/A", "/root3/A/b/c/d"));
    REQUIRE(gul::VirtualFileSystem::is_base_of("/root/A", "/root/A/b/c/d"));
}

void tree(gul::VirtualFileSystem const & fs, gul::VirtualFileSystem::vfs_path_type root = "/", std::string prefix="")
{
    //std::cout << root.filename() << std::endl;
    fs.for_each(root, [&](auto const & v)
    {
        std::cout << prefix << v << std::endl;
        if(fs.is_directory(root / v))
        {
            tree(fs, root/v, prefix + "  ");
        }
    });
}

SCENARIO("test")
{
    gul::VirtualFileSystem fs;


    namespace FS = std::filesystem;

    FS::remove_all(FS::path(CMAKE_CURRENT_BINARY_DIR) / "mnt");
    auto A = FS::path(CMAKE_CURRENT_BINARY_DIR) / "mnt" /"A";
    auto B = FS::path(CMAKE_CURRENT_BINARY_DIR) / "mnt" /"B";

    FS::create_directories( A / "1" / "a");
    FS::create_directories( A / "2" / "b");
    FS::create_directories( A / "3" / "c");
    FS::create_directories( B / "4" / "d");
    FS::create_directories( B / "5" / "e");
    FS::create_directories( B / "6" / "f");

    REQUIRE_NOTHROW(fs.mount("/", {}));
    REQUIRE(fs.exists("/"));

    REQUIRE_NOTHROW(fs.mount("/A", gul::HostPath{A}));


    WHEN("We try to mount a folder thats already mounted we get an error")
    {
        REQUIRE_THROWS(fs.mount("/", gul::HostPath{B}));
    }

    fs.mount("/B", gul::HostPath{B});

    REQUIRE( fs.exists("/B/4"));

    fs.mount("/A/7", gul::HostPath{B});

    fs.listDir("/");
    fs.listDir("/A");
    fs.listDir("/B");
    fs.listDir("/B/4");

}

#if 0
SCENARIO("Union")
{
    gul::VirtualFileSystem fs;

    gul::HostPath H;
    H.pathOnHost.push_back(std::filesystem::path(CMAKE_SOURCE_DIR) / "test");
    H.pathOnHost.push_back(std::filesystem::path(CMAKE_SOURCE_DIR) / "cmake");
    fs.mount("/", {});
    fs.mount("/U", H);

    //fs.print();

    REQUIRE(fs.is_directory("/"));
    REQUIRE(fs.is_directory("/U"));

    REQUIRE(fs.is_mount("/"));
    REQUIRE(fs.is_mount("/U"));

    REQUIRE(fs.exists("/U/Coverage.cmake"));
    REQUIRE(fs.exists("/U/unit-Transform.cpp"));


    REQUIRE(fs.is_regular_file("/U/Coverage.cmake"));
    REQUIRE(!fs.is_directory("/U/Coverate.cmake"));
}


SCENARIO("Union - SameFile")
{
    gul::VirtualFileSystem fs;

    gul::HostPath H;

    // lower directory
    H.pathOnHost.push_back(std::filesystem::path(CMAKE_SOURCE_DIR));

    // upper directory
    H.pathOnHost.push_back(std::filesystem::path(CMAKE_SOURCE_DIR) / "test");

    fs.mount("/", {});
    fs.mount("/U", H);

    //fs.print();

    REQUIRE(fs.exists("/U/CMakeLists.txt"));
    REQUIRE(fs.host_path("/U/CMakeLists.txt") == std::filesystem::path(CMAKE_SOURCE_DIR) / "test" / "CMakeLists.txt");
    tree(fs);
}


#endif
#endif
