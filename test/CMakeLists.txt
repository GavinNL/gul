# Create a static library for Catch2s main so that we can reduce
# compiling time. Each unit test will link to this
cmake_minimum_required(VERSION 3.13)


################################################################################
# If the conan file exists, include it
################################################################################
if( EXISTS ${PROJECT_BINARY_DIR}/conanbuildinfo.cmake )
    include(${PROJECT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(NO_OUTPUT_DIRS TARGETS)
else()

    message("")
    message("")
    message("*************WARNING******************************")
    message("Could Not find ${PROJECT_BINARY_DIR}/conanbuildinfo.cmake.")
    message("Cannot build unit tests. ")
    message("Execute the following to build the unit tests")
    message("  cd ${PROJECT_BINARY_DIR}")
    message("  conan install ${PROJECT_SOURCE_DIR}")
    message("**************************************************")
    message("")
    message("")
    return()
endif()
################################################################################


add_library(${PROJECT_NAME}-catchmain STATIC ${CMAKE_CURRENT_SOURCE_DIR}/catch-main.cpp)
target_include_directories(${PROJECT_NAME}-catchmain PUBLIC third_party)
target_compile_features(${PROJECT_NAME}-catchmain PUBLIC cxx_std_17)
target_link_libraries( ${PROJECT_NAME}-catchmain PUBLIC   CONAN_PKG::catch2)
set_coverage_flags(${PROJECT_NAME}-catchmain)

get_filename_component(folder_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" folder_name ${folder_name})


enable_testing()


file(GLOB files "unit-*.cpp")
foreach(file ${files})

    # Replace any files named test_XXXXX with XXXX
    get_filename_component(file_basename ${file} NAME_WE)
    string(REGEX REPLACE "unit-([^$]+)" "\\1" testcase ${file_basename})

    set(FILE_SUFFIX_NAME  ${testcase} )
    set(EXE_NAME     unit-${FILE_SUFFIX_NAME} )
    set(TEST_NAME    test-${FILE_SUFFIX_NAME} )
    set(SRC_NAME     unit-${FILE_SUFFIX_NAME}.cpp )

    set(exeCmd  ${EXE_NAME} )
    set(envVar  "" )


    add_executable( ${EXE_NAME}  ${SRC_NAME} )

    target_link_libraries( ${EXE_NAME}
                                PUBLIC
                                    ${UNIT_TEST_LINK_TARGETS}
                                    ${LINKED_TARGETS})

    target_link_libraries( ${EXE_NAME}
                                PRIVATE
                                    ${PROJECT_NAME}-catchmain
                                    ${PROJECT_NAME}
                                    CONAN_PKG::glm
                                    CONAN_PKG::stb)


    add_test(  NAME    ${TEST_NAME}
               COMMAND ${exeCmd}
            )
    set_tests_properties(${TEST_NAME}
                            PROPERTIES
                                ENVIRONMENT
                                    "${envVar}")



    message("--------------------------------------------------")
    message("Unit Test Added: ${TEST_NAME}")
    message("--------------------------------------------------")
    message("Souce File       : ${SRC_NAME}")
    message("Working Directory: ${CMAKE_CURRENT_BINARY_DIR}")
    message("Env Variables    : ${envVar}")
    message("Command          : ${exeCmd}")
    #set_coverage_flags(${EXE_NAME})
    #set_project_warnings(${EXE_NAME})
endforeach()

return()
