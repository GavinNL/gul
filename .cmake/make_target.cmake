########################################################################################
# This is an included file 
#
# This builds targets, either an executble or a library. This behaves similar to how 
# Rust creates modles. 
#
#  If src/lib.cpp exists, then all src files will be compiled into a library. 
#
#
#  If src/main.cpp exists. Then all src files will be compiled into a binary.
#
#
#  If include/ exists and src/ does not exist, then it will be compiled into a header-only
#  library
########################################################################################
get_filename_component(library_MODULE ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" library_MODULE ${library_MODULE})


include(GNUInstallDirs)



if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" AND NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src)

    # Header only library
    add_library( ${library_MODULE} INTERFACE )
    add_library( ${PROJECT_NAME}::${library_MODULE} ALIAS ${library_MODULE} )

    target_include_directories( ${library_MODULE}
                                INTERFACE
                                   "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
else()

    file(GLOB_RECURSE srcFiles ${CMAKE_CURRENT_SOURCE_DIR}/src/* )

    if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.cpp)
        # Library
        list(APPEND ${PROJECT_NAME}_ALL_LIBS ${subdir})
        add_library( ${library_MODULE} ${srcFiles} )
        add_library( ${PROJECT_NAME}::${library_MODULE} ALIAS ${library_MODULE} )

        target_link_libraries(${library_MODULE} PUBLIC  ${REQUIRED_PUBLIC_LIBS} )
        target_link_libraries(${library_MODULE} PRIVATE ${REQUIRED_PRIVATE_LIBS} )

        if( ${PROJECT_NAME}_PACKAGE_INCLUDE_LIBS )

            install(
                TARGETS
                   ${library_MODULE}
                LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
                INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            )

        endif()

    elseif( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
        # executable
        add_executable( ${library_MODULE} ${srcFiles} )

        target_compile_definitions(${library_MODULE} PRIVATE ${PRIVATE_DEFINITIONS})
        target_compile_definitions(${library_MODULE} PUBLIC  ${PUBLIC_DEFINITIONS})

        if(  ${PROJECT_NAME}_PACKAGE_INCLUDE_BINS )

            install(
                TARGETS
                   ${library_MODULE}
                LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
                INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            )

        endif()
    endif()



    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")

        target_include_directories( ${library_MODULE}
                                    PUBLIC
                                       "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    
    endif()

    if( ${PROJECT_NAME}_ENABLE_COVERAGE )
        target_link_libraries( ${library_MODULE}  PRIVATE   ${PROJECT_NAME}::coverage  )
    endif()

    if( ${PROJECT_NAME}_ENABLE_WARNINGS )
        target_link_libraries( ${library_MODULE}  PRIVATE   ${PROJECT_NAME}::warnings )
    endif()

endif()


if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" AND ${PROJECT_NAME}_PACKAGE_INCLUDE_HEADERS )
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
            DESTINATION ${CMAKE_INSTALL_PREFIX}
            FILES_MATCHING PATTERN "*"
    )
endif()



if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test")
    set(unitTestsList "")
    enable_testing()

    if(${PROJECT_NAME}_BUILD_UNIT_TESTS)
        add_subdirectory(test)
    endif()

endif()

message("*****************************************************")
message("LIBRARIES:")
message("*****************************************************")
message("   ${library_MODULE} ")
message("      PUBLIC_LINKED_TARGETS ")
foreach(item IN LISTS REQUIRED_PUBLIC_LIBS)
message("          ${item}")
endforeach()
message("      PRIVATE_LINKED_TARGETS ")
foreach(item IN LISTS REQUIRED_PRIVATE_LIBS)
message("          ${item}")
endforeach()
message("      Unit Tests ")
foreach(item IN LISTS unitTestsList)
message("          ${item}")
endforeach()
message("*****************************************************")
