cmake_minimum_required(VERSION 3.13)

project(structocol)

add_library(structocol
		src/structocol/dummy.cpp
		include/structocol/structocol.hpp
		include/structocol/protocol_handler.hpp
		include/structocol/serialization.hpp
		include/structocol/vector_buffer.hpp
		include/structocol/type_utilities.hpp
	)

target_include_directories(structocol PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)
