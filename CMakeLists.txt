cmake_minimum_required(VERSION 2.8)
project(AlloctorTests)

set(GENERIC_COMPILER_FLAGS "-Wall -pedantic -std=c++14 -mtune=native -fomit-frame-pointer")
IF (CMAKE_COMPILER_IS_GNUCXX)
    set(GENERIC_COMPILER_FLAGS  "${GENERIC_COMPILER_FLAGS} -Wold-style-cast -Woverloaded-virtual -flto")
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # This really need the -flto, but it is currently broken in clang
    set(GENERIC_COMPILER_FLAGS  "${GENERIC_COMPILER_FLAGS} -Wc++11-extensions")
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${GENERIC_COMPILER_FLAGS} -g -gdwarf-2 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${GENERIC_COMPILER_FLAGS} -Ofast -DNDEBUG")

find_package(Boost 1.60.0 COMPONENTS thread)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    set(Boost_USE_STATIC_LIBS ON)
endif()

include_directories(${BOOST_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

set(SOURCE_FILES
        performance.h
        fast_linear_allocator.h
        short_alloc.h
        main.cpp
        )
add_executable(AlloctorTests ${SOURCE_FILES})
target_link_libraries(AlloctorTests ${Boost_LIBRARIES})
