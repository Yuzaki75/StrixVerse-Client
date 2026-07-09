cmake_minimum_required(VERSION 3.20)

# Client core interface - only exposes include directory
add_library(client_core INTERFACE)

target_include_directories(client_core INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
)
