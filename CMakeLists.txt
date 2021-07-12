cmake_minimum_required(VERSION 3.15)
project(DiskArray
	LANGUAGES CXX
	)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(DiskArray INTERFACE)
target_include_directories(DiskArray INTERFACE include/)
