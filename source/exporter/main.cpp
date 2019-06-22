#include <tinyxml2.h>

int main()
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile( "maps_src/small_place.osm" );
	doc.Print();
}
