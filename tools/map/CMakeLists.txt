cmake_minimum_required(VERSION 3.5...3.14)
project (maptools CXX)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

if(NOT TARGET wzmaplib)
	# Not embedded in parent main WZ build system - must manually include maplib
	add_subdirectory("../../lib/wzmaplib" "lib/wzmaplib" EXCLUDE_FROM_ALL)
endif()

find_package(PNG 1.2 REQUIRED)

set (CMAKE_USE_PTHREADS_INIT TRUE)
find_package(Threads)

# set CMAKE_BUILD_TYPE to default
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif ()

# maptools
add_executable(maptools
				maptools.cpp pngsave.cpp pngsave.h)
set_target_properties(maptools
	PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
)
target_link_libraries(maptools PRIVATE wzmaplib PNG::PNG)
target_link_libraries(maptools PRIVATE nlohmann_json)
if (TARGET ZipIOProvider)
	target_link_libraries(maptools PRIVATE ZipIOProvider)
else()
	message(WARNING "ZipIOProvider is not available - please ensure libzip is installed. maptools will be compiled without direct support for .wz archives")
	target_compile_definitions(maptools PRIVATE "WZ_MAPTOOLS_DISABLE_ARCHIVE_SUPPORT")
endif()

if(MSVC)
	target_compile_definitions(maptools PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()
