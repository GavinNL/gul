cmake_minimum_required(VERSION 3.10)

#get_filename_component(library_MODULE ${CMAKE_CURRENT_SOURCE_DIR} NAME)
#string(REPLACE " " "_" library_MODULE ${library_MODULE})



################################################################################
# Find all the packages we need
################################################################################
#find_package(Ktx REQUIRED)
#find_package(stb REQUIRED)

# Set which targets this library should be
# linking to
set(REQUIRED_PUBLIC_LIBS
            #${PROJECT_NAME}::${PROJECT_NAME}
    )

set(REQUIRED_PRIVATE_LIBS
            ""
   )

set(PRIVATE_DEFINITIONS
            ""
   )

set(PUBLIC_DEFINITIONS
            ""
   )
################################################################################


include(${CMAKE_SOURCE_DIR}/.cmake/make_target.cmake)


