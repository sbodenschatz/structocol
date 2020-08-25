cmake_minimum_required(VERSION 3.13)

project(structocol)

if(NOT TARGET boost::pfr)
	include(external/magic_get_submodule.cmake)
endif()

option(STRUCTOCOL_BUILD_TESTING "Build tests for the structocol library" ON)
if(STRUCTOCOL_BUILD_TESTING)
	if(NOT TARGET Catch2::Catch2)
		add_subdirectory(external/Catch2)
		list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/Catch2/contrib")
	endif()
	include(Catch)
	enable_testing()
endif()

add_library(structocol_check_build
		src/structocol/dummy.cpp
		include/structocol/structocol.hpp
		include/structocol/protocol_handler.hpp
		include/structocol/serialization.hpp
		include/structocol/vector_buffer.hpp
		include/structocol/type_utilities.hpp
		include/structocol/stream_buffer.hpp
		include/structocol/multiplexing.hpp	
		include/structocol/buffers_ring.hpp
		include/structocol/recycling_buffers_queue.hpp
		include/structocol/stdio_buffer.hpp
		include/structocol/exceptions.hpp
	)
add_library(structocol INTERFACE)
add_library(structocol::structocol ALIAS structocol)
target_include_directories(structocol INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)
target_compile_features(structocol INTERFACE cxx_std_20)
target_link_libraries(structocol INTERFACE boost::pfr)
target_compile_options(structocol_check_build PUBLIC
		$<$<CXX_COMPILER_ID:MSVC>:/MP /W4 /WX /bigobj>
		$<$<CXX_COMPILER_ID:GNU>: -Wall -Wextra -Werror $<$<PLATFORM_ID:Windows>:-Wa,-mbig-obj>>
		$<$<CXX_COMPILER_ID:Clang>: -Wall -Wextra -Werror>
	)
target_link_libraries(structocol_check_build PUBLIC structocol)

option(STRUCTOCOL_FORCE_DISABLE_ASIO_SUPPORT "Disable optional support for Boost.Asio even if boost could be found." OFF)
if(NOT STRUCTOCOL_FORCE_DISABLE_ASIO_SUPPORT)
	find_package(Boost)
	if(TARGET Boost::boost)
		target_link_libraries(structocol INTERFACE Boost::boost)
		target_compile_definitions(structocol INTERFACE STRUCTOCOL_ENABLE_ASIO_SUPPORT)
		if(WIN32)
			target_compile_definitions(structocol_check_build PUBLIC _WIN32_WINNT=0x0601)
		endif()
	endif()
endif()

if(STRUCTOCOL_BUILD_TESTING)
	add_executable(structocol_unit_tests
			src/main.test.cpp
			src/structocol/vector_buffer.test.cpp
			src/structocol/serialization.test.cpp
			src/structocol/serialized_size.test.cpp
			src/structocol/protocol_handler.test.cpp
			src/structocol/stdio_buffer.test.cpp
			src/structocol/buffers_queuing.test.cpp
		)
	target_link_libraries(structocol_unit_tests PUBLIC
			structocol_check_build
			Catch2::Catch2
		)
	catch_discover_tests(structocol_unit_tests)
endif()
