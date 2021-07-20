cmake_minimum_required(VERSION 3.14)
project(diskarray_example CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(cmake/CPM.cmake)

# Prefer to specify specific tag using "@v1.2" etc. or commit hash using "#commithash"
# CPMAddPackage("gh:adi-g15/DiskArray#main")
# add_subdirectory(..)
include_directories(../include)

find_package(Protobuf REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/bigobject.proto)
include_directories(${CMAKE_BINARY_DIR})    # Required, since this location contains the "myobject.pb.h" header

add_executable(${PROJECT_NAME} main.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(${PROJECT_NAME} PUBLIC protobuf)
