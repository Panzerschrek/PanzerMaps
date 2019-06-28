#include "../common/assert.hpp"
#include "geometry_utils.hpp"

namespace PanzerMaps
{

int64_t CalculatePolygonDoubleSignedArea( const MercatorPoint* const vertices, size_t vertex_count )
{
	PM_ASSERT( vertex_count >= 3u );

	int64_t result= 0;

	result+= int64_t(vertices[0u].x) * int64_t(vertices[vertex_count-1u].y) - int64_t(vertices[vertex_count-1u].x) * int64_t(vertices[0u].y);
	for( size_t i= 1u; i < vertex_count; ++i )
		result+= int64_t(vertices[i].x) * int64_t(vertices[i-1u].y) - int64_t(vertices[i-1u].x) * int64_t(vertices[i].y);

	return result;
}

} // namespace PanzerMaps

