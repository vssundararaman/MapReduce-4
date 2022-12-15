cmake_minimum_required(VERSION 3.24)
project(asio_deep)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl")

add_executable(asio_deep main.cpp
        headers/FileProcessorBase.hpp
        headers/MapperBase.hpp
        headers/ShufflerBase.hpp
        headers/ReducerBase.hpp
        )

target_link_libraries(asio_deep ${CMAKE_DL_LIBS})
