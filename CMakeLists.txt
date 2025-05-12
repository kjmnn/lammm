cmake_minimum_required(VERSION 3.10.0)
project(Lammm VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(lammm_lib STATIC)
target_include_directories(lammm_lib PUBLIC src)
target_sources(lammm_lib PRIVATE
    src/Printer.cpp
    src/Types.cpp
    src/Parser.cpp
    src/Typer.cpp
    src/Interpreter.cpp
)

add_executable(test src/tests/Run.cpp)
target_include_directories(test PUBLIC src)
target_sources(test PRIVATE src/tests/Tests.cpp)
target_link_libraries(test lammm_lib)

add_executable(lammm src/Lammm.cpp)
target_include_directories(lammm PUBLIC src)
target_link_libraries(lammm lammm_lib)

