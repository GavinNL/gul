# Gavin's Utility Library

[![pipeline status](https://gitlab.com/GavinNL/gul/badges/master/pipeline.svg)](https://gitlab.com/GavinNL/gul/-/commits/master)

[![coverage report](https://gitlab.com/GavinNL/gul/badges/master/coverage.svg)](https://gitlab.com/GavinNL/gul/-/commits/master)

[![Build status](https://ci.appveyor.com/api/projects/status/euex06777is1gixa/branch/master?svg=true)](https://ci.appveyor.com/project/GavinNL/gul/branch/master)

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

Simple representation of a mesh.




## Meta (gul/meta)

Some template meta-programming helpers

### has_destructor<T>

Checks if `T` has a destructor. This can be used to check if class definitions exist:

```cpp

static_assert( gul::has_destructor< std::hash<MyClass> >::value, "T must be be hashable");

```
