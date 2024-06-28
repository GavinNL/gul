
find_package(Catch2 REQUIRED)


set(TESTCASE_PREFIX  "test-${library_MODULE}")

get_filename_component(folder_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" folder_name ${folder_name})

enable_testing()

message("*****************************************************")
message("UNIT TESTS:")
message("*****************************************************")
# Find all files named unit-*.cpp
file(GLOB files "unit-*.cpp")
foreach(file ${files})

    get_filename_component(file_basename ${file} NAME_WE)
    string(REGEX REPLACE "unit-([^$]+)" "${TESTCASE_PREFIX}-\\1" testcase ${file_basename})

    string(REGEX REPLACE "unit-([^$]+)" "unit-\\1" exe_name ${file_basename})

    #message("New File: ${file} Test case: ${testcase} Exe name: ${exe_name}")


    set(UNIT_EXE_NAME  ${PROJECT_NAME}-${library_MODULE}-${exe_name} )
    set(UNIT_TEST_NAME ${TESTCASE_PREFIX}-${exe_name} )

    add_executable( ${UNIT_EXE_NAME} ${file} )

    #target_compile_features( ${UNIT_EXE_NAME}
    #                            PUBLIC
    #                                "")

    target_compile_definitions(${UNIT_EXE_NAME} PRIVATE
                                    CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
                                    CMAKE_CURRENT_LIST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                                    CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
                                    CMAKE_CURRENT_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

    target_link_libraries( ${UNIT_EXE_NAME} PUBLIC 
                                                Catch2::Catch2WithMain
                                                ${UNIT_TEST_LINK_TARGETS} )
    add_test( NAME ${UNIT_TEST_NAME}
              COMMAND ${UNIT_EXE_NAME}
    )

    list(APPEND unitTestsList ${UNIT_EXE_NAME})

    #message("${UNIT_EXE_NAME} ")
endforeach()
