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


include(.cmake/extras.cmake)

option( ${PROJECT_NAME}_BUILD_UNIT_TESTS "Build the unit tests for this library"       TRUE)
option( ${PROJECT_NAME}_ENABLE_COVERAGE  "Enable Coverage. After build, execute: make coverage"       TRUE)
option( ${PROJECT_NAME}_ENABLE_WARNINGS  "Enable Strict Warnings"                                     TRUE)
option( ${PROJECT_NAME}_PACKAGE_INCLUDE_SOURCE "Include source files when building the package"      TRUE)

################################################################################
# Find all the packages we need
#
# Each module in the src/ folder can also define its own find_package 
# if they are meant to be private
#
################################################################################
#find_package(fmt REQUIRED)
#find_package(glm REQUIRED)
#find_package(glm REQUIRED)
################################################################################



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
    enable_testing()
    add_subdirectory(test)
endif()

#####################################################
## PACKAGE
#####################################################
SET(CPACK_PACKAGE_NAME          ${PROJECT_NAME})
SET(CPACK_PACKAGE_VENDOR        ${PROJECT_NAME})
SET(CPACK_PACKAGE_EXECUTABLES   ${PROJECT_NAME};${PROJECT_NAME})

SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${PROJECT_NAME}")
SET(CPACK_PACKAGE_VERSION              "${PROJECT_VERSION}")
SET(CPACK_PACKAGE_VERSION_MAJOR        "${PROJECT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR        "${PROJECT_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH        "${PROJECT_VERSION_PATCH}")
SET(CPACK_PACKAGE_CONTACT       "me")

INCLUDE(InstallRequiredSystemLibraries)

IF(WIN32 AND NOT UNIX)
  SET(CPACK_NSIS_INSTALLED_ICON_NAME   "bin\\\\${PROJECT_NAME}.exe")
  SET(CPACK_NSIS_DISPLAY_NAME          "${CPACK_PACKAGE_INSTALL_DIRECTORY}${PROJECT_NAME}")
  SET(CPACK_NSIS_MODIFY_PATH           ON)
ENDIF(WIN32 AND NOT UNIX)
#####################################################


INCLUDE(CPack)


return()
