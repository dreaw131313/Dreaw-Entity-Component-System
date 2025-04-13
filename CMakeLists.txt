cmake_minimum_required(VERSION 3.15)

project(decs)

set(DEBUG_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/Debug")
set(RELEASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/Release")

option(DECS_DYNAMIC_RUNTIME ON)

if(DECS_DYNAMIC_RUNTIME)
    set(DECS_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    set(DECS_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(DECS_DIR ./decs/src)
file(GLOB_RECURSE DECS_SRC 
	"${DECS_DIR}/decs/*.cpp"
	"${DECS_DIR}/decs/*.h"
	)
	
add_library(decs STATIC ${DECS_SRC})
target_include_directories(decs PUBLIC ${DECS_DIR})

target_compile_features(decs PUBLIC cxx_std_20)  # Force C++20 for Jolt

set_target_properties(decs PROPERTIES DEBUG_POSTFIX "")
set_target_properties(decs PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${DEBUG_OUTPUT_DIR}/decs"
	ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${RELEASE_OUTPUT_DIR}/decs"
	
	LIBRARY_OUTPUT_DIRECTORY_DEBUG "${DEBUG_OUTPUT_DIR}/decs"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${RELEASE_OUTPUT_DIR}/decs" 
	
	RUNTIME_OUTPUT_DIRECTORY_DEBUG "${DEBUG_OUTPUT_DIR}/decs"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${RELEASE_OUTPUT_DIR}/decs"
)

set_property(TARGET decs PROPERTY MSVC_RUNTIME_LIBRARY "${DECS_MSVC_RUNTIME}")
