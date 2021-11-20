# Gavin's Utility Library

[![pipeline status](https://gitlab.com/GavinNL/gul/badges/main/pipeline.svg)](https://gitlab.com/GavinNL/gul/-/commits/main) [![coverage report](https://gitlab.com/GavinNL/gul/badges/main/coverage.svg)](https://gitlab.com/GavinNL/gul/-/commits/main) [![Build status](https://ci.appveyor.com/api/projects/status/euex06777is1gixa/branch/main?svg=true)](https://ci.appveyor.com/project/GavinNL/gul/branch/main)

```bash

mkdir build
cd build
conan install ..

cmake ..
cmake --build .

ctest --output-on-failure

```

## Utils

## uri

## ResourceLocator


## Net

## socket.h

- Simple cross platform socket class


## Math

### Transform

- Non-matrix representation of a T * R * S matrix for 3D transformations

## Image.h

Requirements: None

```cpp

```

## MeshPrimitive.h

A class to hold Mesh data.


## Meta (gul/meta)

Some template meta-programming helpers

### has_destructor

Checks if `T` has a destructor. This can be used to check if class definitions exist:

```cpp

static_assert( gul::has_destructor< std::hash<MyClass> >::value, "T must be be hashable");

```
