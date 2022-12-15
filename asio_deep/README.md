## Build

g++ -std=c++17 -o asio_server main.cpp headers/FileProcessorBase.hpp headers/MapperBase.hpp headers/ReducerBase.hpp headers/ShufflerBase.hpp -ldl