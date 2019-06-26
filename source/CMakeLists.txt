cmake_minimum_required(VERSION 3.0)

project( PanzerMaps )

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DDEBUG )
	add_definitions( -DPM_DEBUG )
endif()
add_compile_options( -Wall -Wextra -Wconversion )

find_package( SDL2 REQUIRED )

add_subdirectory( tinyxml2 )
add_subdirectory( PanzerJson )

file( GLOB EXPORTER_SOURCES "exporter/*.hpp" "exporter/*.cpp" "common/*.hpp" "common/*.cpp" )

add_executable( Exporter ${EXPORTER_SOURCES} )
target_include_directories( Exporter PRIVATE PanzerJson/include )
target_link_libraries( Exporter PRIVATE tinyxml2 )
target_link_libraries( Exporter PRIVATE PanzerJsonLib )

file( GLOB MAPS_SOURCES
	"maps/*.hpp"
	"maps/*.cpp"
	"common/*.hpp"
	"common/*.cpp"
	"panzer_ogl_lib/*.hpp"
	"panzer_ogl_lib/*.h"
	"panzer_ogl_lib/*.cpp" )

add_executable( PanzerMaps ${MAPS_SOURCES} )
target_compile_definitions( PanzerMaps PRIVATE -DHAVE_SDL )
target_include_directories( PanzerMaps PRIVATE ${SDL2_INCLUDE_DIRS} )
target_link_libraries( PanzerMaps PRIVATE ${SDL2_LIBRARIES} )
target_link_libraries( PanzerMaps PRIVATE GL )
