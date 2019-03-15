cmake_minimum_required(VERSION 3.13)

project(structocol)

include(external/magic_get_submodule.cmake)

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

target_compile_features(structocol PUBLIC cxx_std_17)

target_link_libraries(structocol PUBLIC boost::pfr)
