cmake_minimum_required(VERSION 3.0)

project( PanzerMaps )

find_package( SDL2 REQUIRED )

file( GLOB SOURCES "*.hpp" "*.cpp" )

add_executable( PanzerMaps ${SOURCES} )
target_include_directories( PanzerMaps PRIVATE ${SDL2_INCLUDE_DIRS} )
target_link_libraries( PanzerMaps PRIVATE ${SDL2_LIBRARIES} )
