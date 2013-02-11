cmake_minimum_required(VERSION 2.6)
project(PATH_DECOMPOSED_TRIES)

find_package(Boost 1.42.0 COMPONENTS unit_test_framework iostreams system thread filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})

# Prevent MSVC from complaining about fopen, etc...
if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif(MSVC)

# Always create debugging information
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
endif(MSVC)
if (UNIX)
   set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) # Add debug info anyway
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE BOOL
  "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)

enable_testing()

# add the root directory to include path to make includes absolute
include_directories(${PATH_DECOMPOSED_TRIES_SOURCE_DIR})

# succinct subdirectory
add_subdirectory(succinct EXCLUDE_FROM_ALL)
# propagate compilation options from succinct
# XXX(ot): this is ugly, how can this be made better
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")	
   if (SUCCINCT_USE_LIBCXX) 
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif ()
endif ()

if (SUCCINCT_USE_POPCNT)
  if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
  endif ()
  # XXX(ot): what to do for MSVC?
endif ()


# tries
add_subdirectory(tries)  

# re-pair
add_subdirectory(repair)

# perftests
add_subdirectory(perftest)

