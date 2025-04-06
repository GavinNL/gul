cmake_minimum_required(VERSION 3.10)


################################################################################
# This Top-level project is for a single Top-level library
#
# A single library
# Unit tests for the library
# A set of executables which use this library
################################################################################



################################################################################
# Project name
#
#  many targets will be created:
#   myProject::warnings
#   myProject:
################################################################################
project( gul
         VERSION 1.0.0.0
         LANGUAGES CXX)

################################################################################


option( ${PROJECT_NAME}_BUILD_UNIT_TESTS "Build the unit tests for this library"                 TRUE)



################################################################################
# Top level INTERFACE library which can be used to 
################################################################################
set(outName              ${PROJECT_NAME})   # name of the library within the proje
                                            # Will create the target ${PROJECT_NAME}::${outName}

set(PublicLinkedTargets  "" )               # Any additional public targets you want to include
#-------------------------------------------------------------------------------
add_library( ${outName} INTERFACE )
add_library( ${PROJECT_NAME}::${outName}  ALIAS ${outName})

target_compile_features( ${outName}
                          INTERFACE
                              cxx_std_20)

target_compile_definitions( ${outName}
                              INTERFACE
                                CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
                                CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
                                )
target_include_directories( ${outName} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries( ${outName}  INTERFACE  ${PublicLinkedTargets})


include(GNUInstallDirs)

################################################################################


# enable testing on the root folder so that we can run ctest from it,
# otherwise we will have to CD into each sub folder to run the unit tests
if(${PROJECT_NAME}_BUILD_UNIT_TESTS)
    include(.cmake/extras.cmake)
    option( ${PROJECT_NAME}_ENABLE_COVERAGE  "Enable Coverage. After build, execute: make coverage"  TRUE)
    option( ${PROJECT_NAME}_ENABLE_WARNINGS  "Enable Strict Warnings"                                TRUE)
    enable_testing()
    add_subdirectory(test)
endif()


return()
