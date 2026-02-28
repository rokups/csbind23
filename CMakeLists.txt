cmake_minimum_required(VERSION 3.21)

project(csbind23 VERSION 0.1.0 LANGUAGES CXX)

include(${PROJECT_SOURCE_DIR}/cmake/Csbind23Managed.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CSBIND23_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
set(CSBIND23_LIB_DIR "${CMAKE_BINARY_DIR}/lib")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CSBIND23_BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CSBIND23_BIN_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CSBIND23_LIB_DIR}")

if (CMAKE_CONFIGURATION_TYPES)
	foreach(config ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER "${config}" config_upper)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CSBIND23_BIN_DIR}/${config}")
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${CSBIND23_BIN_DIR}/${config}")
		set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${CSBIND23_LIB_DIR}/${config}")
	endforeach()
endif()

csbind23_generate_msbuild_props()

add_subdirectory(src)
add_subdirectory(tests)

csbind23_add_managed_aggregate_targets()
