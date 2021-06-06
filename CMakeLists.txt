cmake_minimum_required(VERSION 3.8)

#########################################################
project(gul  VERSION 1.0.0)
#########################################################


get_directory_property( _is_sub_project PARENT_DIRECTORY)
if(_is_sub_project)

    option( ${PROJECT_NAME}_ENABLE_COVERAGE "Enable Coverage."       FALSE)
    option( ${PROJECT_NAME}_ENABLE_WARNINGS "Enable Strict Warnings" FALSE)
    option( ${PROJECT_NAME}_ENABLE_TESTING  "Enable Unit Tests"      FALSE)

else()

    option( ${PROJECT_NAME}_ENABLE_COVERAGE "Enable Coverage."       TRUE)
    option( ${PROJECT_NAME}_ENABLE_WARNINGS "Enable Strict Warnings" TRUE)
    option( ${PROJECT_NAME}_ENABLE_TESTING  "Enable Unit Tests"      TRUE)

    #########################################################
    include(cmake/StandardProjectSettings.cmake)
    include(cmake/CompilerWarnings.cmake)
    include(cmake/Coverage.cmake)
    #########################################################

    #########################################################
    # Other targets
    #########################################################
    add_custom_target( ${PROJECT_NAME}_other_files
                            SOURCES
                              # .travis/install.sh
                              # .travis/build.sh
                               README.md
                              # LICENSE
                               .gitlab-ci.yml
                              # .travis.yml
                               conanfile.txt
                               appveyor.yml
                               cmake/CompilerWarnings.cmake
                               cmake/Coverage.cmake
                    )
    #########################################################

endif()



#########################################################
# Create an interface library so that we can link to
# The proper configurations.
#
# Any target that wants to use gnl should link to
# ABC::ABC.  eg:
#
#  target_link_libraries(myexe ABC::ABC)
#
#########################################################
set(lib_name   ${PROJECT_NAME})
set(lib_alias  ${PROJECT_NAME}::${PROJECT_NAME})

add_library( ${lib_name} INTERFACE)
add_library( ${lib_alias} ALIAS ${lib_name} )

target_compile_features( ${lib_name}
                            INTERFACE
                                cxx_std_17
                        )
target_include_directories( ${lib_name}
                                INTERFACE
                                    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
                                    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated_headers>"
                           )

if( ${PROJECT_NAME}_ENABLE_COVERAGE )
    set_coverage_flags(${lib_name})
endif()
if( ${PROJECT_NAME}_ENABLE_WARNINGS )
    set_project_warnings(${lib_name})
endif()
#########################################################







if( _is_sub_project )
else()
    #########################################################
    enable_testing()
    add_subdirectory(test)
    #########################################################
endif()
