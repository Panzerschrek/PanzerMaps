cmake_minimum_required(VERSION 3.0)

project( PanzerMaps )

find_package( SDL2 REQUIRED )

add_subdirectory( tinyxml2 )

file( GLOB EXPORTER_SOURCES "exporter/*.hpp" "exporter/*.cpp" )

add_executable( Exporter ${EXPORTER_SOURCES} )
target_link_libraries( Exporter PRIVATE tinyxml2 )

file( GLOB MAPS_SOURCES "maps/*.hpp" "maps/*.cpp" )
add_executable( PanzerMaps ${MAPS_SOURCES} )
target_include_directories( PanzerMaps PRIVATE ${SDL2_INCLUDE_DIRS} )
target_link_libraries( PanzerMaps PRIVATE ${SDL2_LIBRARIES} )
