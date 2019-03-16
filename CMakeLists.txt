cmake_minimum_required(VERSION 3.13)

project(structocol)

include(external/magic_get_submodule.cmake)

option(STRUCTOCOL_BUILD_TESTING "Build tests for the structocol library" ON)
if(STRUCTOCOL_BUILD_TESTING)
	add_subdirectory(external/Catch2)
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/Catch2/contrib")
	include(Catch)
	enable_testing()
endif()

add_library(structocol
		src/structocol/dummy.cpp
		include/structocol/structocol.hpp
		include/structocol/protocol_handler.hpp
		include/structocol/serialization.hpp
		include/structocol/vector_buffer.hpp
		include/structocol/type_utilities.hpp
	)
add_library(structocol::structocol ALIAS structocol)

target_include_directories(structocol PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)

target_compile_features(structocol PUBLIC cxx_std_17)

target_link_libraries(structocol PUBLIC boost::pfr)

if(STRUCTOCOL_BUILD_TESTING)
	add_executable(structocol_unit_test
			src/main.test.cpp
			src/structocol/vector_buffer.test.cpp
			src/structocol/serialization.test.cpp
		)
	target_link_libraries(structocol_unit_test PUBLIC
			structocol::structocol
			Catch2::Catch2
		)
	catch_discover_tests(structocol_unit_test)
endif()
