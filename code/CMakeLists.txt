cmake_minimum_required(VERSION 3.0)
project(quote CXX C)

set(CMAKE_MACOSX_RPATH 1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "The configuration to build" FORCE)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wno-unsupported-friend -pedantic -std=c++14")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/usr/local/lib/aws-lambda-runtime/cmake/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/${CMAKE_BUILD_TYPE}/lib")
# set (aws-lambda-runtime_DIR "${CMAKE_SOURCE_DIR}/cmake/")

option(BUILD_UNIT_TESTS "Build the quote unit tests" ON)

add_subdirectory(lib)
find_package(aws-lambda-runtime)
find_package(AWSSDK COMPONENTS s3)

include_directories("include" "/usr/local/include")
include_directories("include" "lib/cpp_redis/includes/")
include_directories("include" "lib/cpp_redis/tacopie/includes/")



add_subdirectory(src)
aws_lambda_package_target(quote)
aws_lambda_package_target(create_dictionary)
aws_lambda_package_target(create_quote)
aws_lambda_package_target(hello_cpp)
