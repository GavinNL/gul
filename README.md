# Gavin's Utility Library

[![pipeline status](https://gitlab.com/GavinNL/gul/badges/main/pipeline.svg)](https://gitlab.com/GavinNL/gul/-/commits/main) [![coverage report](https://gitlab.com/GavinNL/gul/badges/main/coverage.svg)](https://gitlab.com/GavinNL/gul/-/commits/main) [![Build status](https://ci.appveyor.com/api/projects/status/euex06777is1gixa/branch/main?svg=true)](https://ci.appveyor.com/project/GavinNL/gul/branch/main)

This is a collection of single-header classes which I have developed over the years for various uses. 

The following classes only require the STL library as a dependency

* **gul::uri** - a Universal Resource Identifer class.
* **gul::thread_pool** - A thread pool class.
* **gul::writer_preferred_shared_mutex** - like std::shared_mutex but writers are given priority
* **gul::Image** - A class used to manipulate image data with per-channel operators, 
  eg: `img.r = img.g + img.b`
* **gul::MeshPrimitive** - A class to hold mesh data and load OBJ models
* **gul::ResourceLocator** - Works similar to the PATH variable. Provide a list of directories and search for files within them
* **gul::ResourceManager** - A management system for loading resources.

The following classes require the GLM math library

* **gul::Transform** - a non-matrix representation of the translate-rotate-scale transform
* **gul::Frustum** - a frustum class used for culling
* **gul::Octree** - an octree class for geometry culling


## Using This Library

To use this library, simply add it as a submodule and link it with your binary.

```cmake
add_subdirectory(third_party/gul)

target_link_libraries( myApp PUBLIC gul::gul)
```


## Build Unit Tests
 
The unit tests use conan to download any third-party dependences (glm and catch2). 

```bash
mkdir build
cd build
conan install .. -s compiler.libcxx=libstdc++11

cmake ..
cmake --build .

ctest --output-on-failure
```


