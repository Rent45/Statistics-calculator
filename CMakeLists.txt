cmake_minimum_required(VERSION 3.16)
project(Final LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(Final main.cpp)

# point to your external_alglib folder (default relative, you can override with -DALGLIB_ROOT=...)
set(ALGLIB_ROOT "${CMAKE_SOURCE_DIR}/external_alglib" CACHE PATH "Path to ALGLIB")

get_filename_component(ALGLIB_ROOT "${ALGLIB_ROOT}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

file(GLOB ALGLIB_SOURCES "${ALGLIB_ROOT}/*.cpp")
message(STATUS "ALGLIB_SOURCES: ${ALGLIB_SOURCES}")

if(ALGLIB_SOURCES)
  add_library(alglib STATIC ${ALGLIB_SOURCES})
  target_include_directories(alglib PUBLIC "${ALGLIB_ROOT}")
  target_link_libraries(Final PRIVATE alglib)
else()
  message(WARNING "No ALGLIB .cpp found in ${ALGLIB_ROOT}; set -DALGLIB_ROOT or place files there")
endif()