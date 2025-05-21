cmake_minimum_required(VERSION 3.31)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(bare_png C)

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
