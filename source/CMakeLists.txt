cmake_minimum_required(VERSION 3.6)

project( PanzerMaps )

set(CMAKE_CXX_STANDARD 11)

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DDEBUG )
	add_definitions( -DPM_DEBUG )
endif()
add_compile_options( -Wall -Wextra -Wconversion )

find_package( SDL2 REQUIRED )
find_package( PNG REQUIRED )

set( BUILD_SHARED_LIBS OFF ) # tinyxml2 builds as dynamic library, build as static instead.

add_subdirectory( tinyxml2 )
add_subdirectory( PanzerJson )

file( GLOB EXPORTER_SOURCES "exporter/*.hpp" "exporter/*.cpp" "common/*.hpp" "common/*.cpp" )

add_executable( Exporter ${EXPORTER_SOURCES} )
target_include_directories( Exporter PRIVATE PanzerJson/include )
target_include_directories( Exporter PRIVATE ${PNG_INCLUDE_DIRS} )
target_compile_definitions( Exporter PRIVATE ${PNG_DEFINITIONS} )
target_link_libraries( Exporter PRIVATE tinyxml2 )
target_link_libraries( Exporter PRIVATE PanzerJsonLib )
target_link_libraries( Exporter PRIVATE ${PNG_LIBRARIES} )

file( GLOB MAPS_SOURCES
	"maps/*.hpp"
	"maps/*.cpp"
	"common/*.hpp"
	"common/*.cpp"
	"panzer_ogl_lib/*.hpp"
	"panzer_ogl_lib/*.h"
	"panzer_ogl_lib/*.cpp" )

if( ANDROID )
	list( FILTER MAPS_SOURCES EXCLUDE REGEX "stub" )
else()
	list( FILTER MAPS_SOURCES EXCLUDE REGEX "android" )
endif()

add_executable( PanzerMaps ${MAPS_SOURCES} )
target_compile_definitions( PanzerMaps PRIVATE -DPM_HAVE_SDL )
target_include_directories( PanzerMaps PRIVATE ${SDL2_INCLUDE_DIRS} )
target_link_libraries( PanzerMaps PRIVATE ${SDL2_LIBRARIES} )
target_link_libraries( PanzerMaps PRIVATE GL )

if( ANDROID )
	target_compile_definitions( PanzerMaps PRIVATE -DPM_OPENGL_ES )
endif()
