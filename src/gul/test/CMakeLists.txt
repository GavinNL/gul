# Create a static library for Catch2s main so that we can reduce
# compiling time. Each unit test will link to this
cmake_minimum_required(VERSION 3.13)


find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(meshoptimizer REQUIRED)
find_package(stb REQUIRED)

set(UNIT_TEST_LINK_TARGETS ${PROJECT_NAME}::interface ${PROJECT_NAME}::${library_MODULE} ${PROJECT_NAME}::coverage 
glm::glm
stb::stb
nlohmann_json::nlohmann_json 
meshoptimizer::meshoptimizer
)

include(${CMAKE_SOURCE_DIR}/.cmake/make_tests.cmake)

