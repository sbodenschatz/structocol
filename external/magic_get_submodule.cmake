cmake_minimum_required(VERSION 3.13)

include_guard()

add_library(pfr INTERFACE)
add_library(boost::pfr ALIAS pfr)
target_include_directories(pfr INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/magic_get/include>
		$<INSTALL_INTERFACE:include>
	)

install(
		TARGETS pfr
		EXPORT magic_get_targets
	)
install(
		DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/magic_get/include/
		DESTINATION include
	)

export(
		EXPORT magic_get_targets
		FILE "${CMAKE_CURRENT_BINARY_DIR}/magic_get_targets.cmake"
	)

install(
		EXPORT magic_get_targets
		FILE magic_get_targets.cmake
		NAMESPACE boost::
		DESTINATION lib/cmake
	)
