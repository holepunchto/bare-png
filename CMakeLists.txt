cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(bare_png C)

fetch_package("github:madler/zlib@1.3.1" SOURCE_DIR zlib_dir)

set(ZLIB_INCLUDE_DIR ${zlib_dir})
set(ZLIB_LIBRARY $<TARGET_FILE:zlibstatic>)

add_library(ZLIB::ZLIB ALIAS zlibstatic)

set(SKIP_INSTALL_ALL ON)

fetch_package("github:pnggroup/libpng@1.6.48")

add_bare_module(bare_png)

target_sources(
  ${bare_png}
  PRIVATE
    binding.c
)

target_link_libraries(
  ${bare_png}
  PUBLIC
    png_static
)
