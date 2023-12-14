cmake_minimum_required(VERSION 3.15.3)
project(WebServer_0_3)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_STANDARD 11)

AUX_SOURCE_DIRECTORY(. SRC_LIST)

ADD_EXECUTABLE(server ${SRC_LIST})
target_link_libraries(server -lpthread)