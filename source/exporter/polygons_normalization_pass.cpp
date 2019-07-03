#include <algorithm>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "geometry_utils.hpp"
#include "polygons_normalization_pass.hpp"

namespace PanzerMaps
{

// Returns non-negative value for convex vertex of clockwise polygon.
static int64_t PolygonVertexCross( const MercatorPoint& p0, const MercatorPoint& p1, const MercatorPoint& p2 )
{
	const int32_t dx0= p1.x - p0.x;
	const int32_t dy0= p1.y - p0.y;
	const int32_t dx1= p2.x - p1.x;
	const int32_t dy1= p2.y - p1.y;

	return int64_t(dx1) * int64_t(dy0) - int64_t(dx0) * int64_t(dy1);
}

static bool VertexIsInisideClockwiseConvexPolygon( const MercatorPoint* const vertices, size_t vertex_count, const MercatorPoint& test_vertex )
{
	PM_ASSERT( vertex_count >= 3u );

	for( size_t i= 0u; i < vertex_count; ++i )
	{
		if(
			PolygonVertexCross(
				vertices[i],
				vertices[ (i+1u) % vertex_count ],
				test_vertex ) <= 0 )
			return false;
	}
	return true;
}

// Return 1 - have intersection, 0 - touches, -1 - no intersection.
static int32_t SegemntsIntersects(
	const MercatorPoint& e0v0, const MercatorPoint& e0v1,
	const MercatorPoint& e1v0, const MercatorPoint& e1v1 )
{
	const int32_t e0_dx= e0v1.x - e0v0.x;
	const int32_t e0_dy= e0v1.y - e0v0.y;

	const int32_t e1_dx= e1v1.x - e1v0.x;
	const int32_t e1_dy= e1v1.y - e1v0.y;

	const int64_t denom= int64_t(e1_dx) * int64_t(e0_dy) - int64_t(e0_dx) * int64_t(e1_dy);
	if( denom == 0 )
		return -1;
	const int64_t abs_denom= std::abs(denom);
	const int64_t denom_sign= denom > 0 ? +1 : -1;

	const int32_t dx0= e1v0.x - e0v0.x;
	const int32_t dy0= e1v0.y - e0v0.y;
	const int64_t s= ( -e0_dy * dx0 + e0_dx * dy0 ) * denom_sign;
	const int64_t t= ( +e1_dx * dy0 - e1_dy * dx0 ) * denom_sign;

	if( ( s == 0 || s == abs_denom ) && ( t >= 0 && t <= abs_denom ) )
		return 0;
	if( ( t == 0 || t == abs_denom ) && ( s >= 0 && s <= abs_denom ) )
		return 0;
	if( s > 0 && s < abs_denom && t > 0 && t < abs_denom )
		return 1;
	return -1;
}

static bool VertexOnEdge( const MercatorPoint& ev0, const MercatorPoint& ev1, const MercatorPoint& v )
{
	const int64_t dx0= v.x - ev0.x;
	const int64_t dy0= v.y - ev0.y;

	const int64_t dx1= v.x - ev1.x;
	const int64_t dy1= v.y - ev1.y;

	return dx0 * dy1 - dx1 * dy0 == 0 && dx0 * dx1 + dy0 * dy1 <= 0;

}

static std::vector< std::vector<MercatorPoint> > SplitPolygonIntNoncrossingParts( const std::vector<MercatorPoint>& vertices )
{
	for( size_t e0= 0u; e0 < vertices.size(); ++e0 )
	for( size_t e1= 0u; e1 < vertices.size(); ++e1 )
	{
		if( e0 == e1 ||
			( e0 + 1u ) % vertices.size() == e1 ||
			( e1 + 1u ) % vertices.size() == e0 ||
			( e0 + 1u ) % vertices.size() == ( e1 + 1u)  % vertices.size() )
			continue;

		const MercatorPoint& e0v0= vertices[e0];
		const MercatorPoint& e0v1= vertices[ (e0+1u) % vertices.size() ];

		const MercatorPoint& e1v0= vertices[e1];
		const MercatorPoint& e1v1= vertices[ (e1+1u) % vertices.size() ];

		const int32_t e0_dx= e0v1.x - e0v0.x;
		const int32_t e0_dy= e0v1.y - e0v0.y;

		const int32_t e1_dx= e1v1.x - e1v0.x;
		const int32_t e1_dy= e1v1.y - e1v0.y;

		const int64_t denom= int64_t(e1_dx) * int64_t(e0_dy) - int64_t(e0_dx) * int64_t(e1_dy);
		if( denom == 0 )
			continue;
		const int64_t abs_denom= std::abs(denom);
		const int64_t denom_sign= denom > 0 ? +1 : -1;

		const int32_t dx0= e1v0.x - e0v0.x;
		const int32_t dy0= e1v0.y - e0v0.y;
		const int64_t s= ( -e0_dy * dx0 + e0_dx * dy0 ) * denom_sign;
		const int64_t t= ( +e1_dx * dy0 - e1_dy * dx0 ) * denom_sign;

		if( s >= 0 && s <= abs_denom && t >= 0 && t <= abs_denom )
		{
			const MercatorPoint inersection_point{
				int32_t( e0v0.x + e0_dx * t / abs_denom ),
				int32_t( e0v0.y + e0_dy * t / abs_denom ) };

			std::vector<MercatorPoint> res0;
			const size_t res0_vertex_count= ( vertices.size() + e1 - e0 ) % vertices.size();
			PM_ASSERT( res0_vertex_count >= 2u );
			for( size_t i= 0u; i < res0_vertex_count; ++i )
				res0.push_back( vertices[ ( e0 + 1u + i ) % vertices.size() ] );
			if( res0.front() != inersection_point && res0.back() != inersection_point )
				res0.push_back( inersection_point );

			std::vector<MercatorPoint> res1;
			const size_t res1_vertex_count= ( vertices.size() + e0 - e1 ) % vertices.size();
			PM_ASSERT( res1_vertex_count >= 2u );
			for( size_t i= 0u; i < res1_vertex_count; ++i )
				res1.push_back( vertices[ ( e1 + 1u + i ) % vertices.size() ] );
			if( res1.front() != inersection_point && res1.back() != inersection_point )
				res1.push_back( inersection_point );

			std::vector< std::vector<MercatorPoint> > res_total;
			if( res0.size() >= 3u )
				res_total= SplitPolygonIntNoncrossingParts( res0 );

			if( res1.size() >= 3u )
			{
				std::vector< std::vector<MercatorPoint> > res1_splitted= SplitPolygonIntNoncrossingParts( res1 );
				for( std::vector<MercatorPoint>& split_result : res1_splitted )
					res_total.push_back( std::move( split_result ) );
			}

			return res_total;
		}
	}

	return { vertices };
}

// Input polygons must not be self-intersecting.
// Result parts are clockwise.
static std::vector< std::vector<MercatorPoint> > SplitPolygonIntoConvexParts( std::vector<MercatorPoint> vertices )
{
	PM_ASSERT( vertices.size() >= 3u );
	std::vector< std::vector<MercatorPoint> > result;

	const int64_t polygon_double_signed_area= CalculatePolygonDoubleSignedArea( vertices.data(), vertices.size() );
	if( polygon_double_signed_area == 0 )
		return { result };

	if( polygon_double_signed_area < 0 )
		std::reverse( vertices.begin(), vertices.end() ); // Make polygon clockwise.

	const auto is_split_vertex=
	[&]( const size_t vertex_index ) -> bool
	{
		const int64_t cross=
		PolygonVertexCross(
			vertices[ ( vertex_index + vertices.size() - 1u ) % vertices.size() ],
			vertices[ vertex_index % vertices.size() ],
			vertices[ ( vertex_index + 1u ) % vertices.size() ] );

		return cross < 0;
	};

	std::vector<MercatorPoint> triangle;
	while( vertices.size() > 3u )
	{
		bool have_split_vertices= false;
		for( size_t i= 0u; i < vertices.size(); ++i )
			have_split_vertices= have_split_vertices || is_split_vertex(i);
		if( !have_split_vertices )
		{
			// Polygon is convex, stop triangulation
			goto finish_triangulation;
		}

		for( size_t i= 0u; i < vertices.size(); ++i )
		{
			if( is_split_vertex(i) )
				continue;
			// Try create triangle with convex vertex.

			triangle.clear();
			triangle.push_back( vertices[ ( i + vertices.size() - 1u ) % vertices.size() ] );
			triangle.push_back( vertices[ i % vertices.size() ] );
			triangle.push_back( vertices[ ( i + 1u ) % vertices.size() ] );

			PM_ASSERT( CalculatePolygonDoubleSignedArea( triangle.data(), triangle.size() ) >= 0 );

			for( size_t j = 2u; j < vertices.size() - 1u; ++j )
			{
				const MercatorPoint& e0v0= vertices[ ( i + j      ) % vertices.size() ];
				const MercatorPoint& e0v1= vertices[ ( i + j + 1u ) % vertices.size() ];

				if( VertexIsInisideClockwiseConvexPolygon( triangle.data(), triangle.size(), e0v0 ) )
					goto select_next_vertex_fo_triangulation;

				const int32_t intersection_result= SegemntsIntersects( triangle[0u], triangle[2u], e0v0, e0v1 );
				if( intersection_result == -1 )
					continue;
				if( intersection_result == 1 )
					goto select_next_vertex_fo_triangulation;

				if( VertexOnEdge( triangle[0u], triangle[1u], e0v0 ) || VertexOnEdge( triangle[1u], triangle[2u], e0v0 ) )
					goto select_next_vertex_fo_triangulation;
			}

			result.push_back( triangle );
			vertices.erase( vertices.begin() + i );
			goto continue_triangulation;

			select_next_vertex_fo_triangulation:;
		}

		Log::Warning( "Broken polygon, split into convex parts failed. Vertex left: ", vertices.size() );
		result.push_back( vertices );
		return result;

		continue_triangulation:;
	}
	finish_triangulation:
	result.push_back( vertices );

	// After triangulation, merge ajusted polygons, is result polygon will be convex.
	// TODO - optimize it, make something, like O(n*log(n)), rather then O(n^3).
	while(true)
	{
		for( size_t p0= 0u; p0 < result.size(); ++p0 )
		for( size_t p1= 0u; p1 < result.size(); ++p1 )
		{
			if( p0 == p1 ) continue;
			const std::vector<MercatorPoint>& poly0= result[p0];
			const std::vector<MercatorPoint>& poly1= result[p1];
			const size_t p0_size_add= poly0.size() * 4u;
			const size_t p1_size_add= poly1.size() * 4u;

			size_t adjusted_v0= ~0u;
			size_t adjusted_v1= ~0u;
			for( size_t v0= 0u; v0 < poly0.size(); ++v0 )
			for( size_t v1= 0u; v1 < poly1.size(); ++v1 )
			{
				if( poly0[v0] == poly1[v1] )
				{
					adjusted_v0= v0;
					adjusted_v1= v1;
					goto have_adjusted_vertices;
				}
			}
			continue;

			have_adjusted_vertices:
			size_t adjusted_start0= adjusted_v0 + p0_size_add, adjusted_end0= adjusted_v0 + p0_size_add;
			size_t adjusted_start1= adjusted_v1 + p1_size_add, adjusted_end1= adjusted_v1 + p1_size_add;

			while( poly0[ ( adjusted_start0 + p0_size_add - 1u ) % poly0.size() ] ==
				   poly1[ ( adjusted_end1   + p1_size_add + 1u ) % poly1.size() ] &&
				   adjusted_end0 - adjusted_start0 < poly0.size() &&
				   adjusted_end1 - adjusted_start1 < poly1.size() )
			{
				--adjusted_start0;
				++adjusted_end1;
			}

			while( poly0[ ( adjusted_end0   + p0_size_add + 1u ) % poly0.size() ] ==
				   poly1[ ( adjusted_start1 + p1_size_add - 1u ) % poly1.size() ] &&
				   adjusted_end0 - adjusted_start0 < poly0.size() &&
				   adjusted_end1 - adjusted_start1 < poly1.size() )
			{
				++adjusted_end0;
				--adjusted_start1;
			}

			if( adjusted_start0 == adjusted_end0 )
				continue;// Have only one adjusted vertex.
			if( ( adjusted_end0 - adjusted_start0 ) % poly0.size() == 0u )
				continue;
			if( ( adjusted_end1 - adjusted_start1 ) % poly1.size() == 0u )
				continue;

			const int64_t cross0=
				PolygonVertexCross(
					poly0[ ( adjusted_start0 + p0_size_add - 1u ) % poly0.size() ],
					poly0[ ( adjusted_start0 + p0_size_add + 0u ) % poly0.size() ],
					poly1[ ( adjusted_end1   + p1_size_add + 1u ) % poly1.size() ] );
			const int64_t cross1=
				PolygonVertexCross(
					poly1[ ( adjusted_start1 + p1_size_add - 1u ) % poly1.size() ],
					poly1[ ( adjusted_start1 + p1_size_add + 0u ) % poly1.size() ],
					poly0[ ( adjusted_end0   + p0_size_add + 1u ) % poly0.size() ] );
			if( !( cross0 >= 0 && cross1 >= 0 ) )
				continue; // Result polygon will be non-convex.

			std::vector<MercatorPoint> poly_combined;

			const size_t poly0_vertex_count= poly0.size() - ( adjusted_end0 - adjusted_start0 );
			const size_t poly1_vertex_count= poly1.size() - ( adjusted_end1 - adjusted_start1 );
			for( size_t i= 0u; i < poly0_vertex_count; ++i )
				poly_combined.push_back( poly0[ ( adjusted_end0 + p0_size_add + i ) % poly0.size() ] );
			for( size_t i= 0u; i < poly1_vertex_count; ++i )
				poly_combined.push_back( poly1[ ( adjusted_end1 + p1_size_add + i ) % poly1.size() ] );
			PM_ASSERT( poly_combined.size() >= 3u );

			result[p0]= std::move( poly_combined );
			if( p1 + 1u < result.size() )
				result[p1]= std::move( result.back() );
			result.pop_back();

			goto next_iteration;
		}

		break;
		next_iteration:;
	}

	return result;
}

PolygonsNormalizationPassResult NormalizePolygons( const PhaseSortResult& in_data )
{
	PolygonsNormalizationPassResult result;
	result.min_point= in_data.min_point;
	result.max_point= in_data.max_point;
	result.start_point= in_data.start_point;
	result.coordinates_scale= in_data.coordinates_scale;
	result.zoom_level= in_data.zoom_level;
	result.meters_in_unit= in_data.meters_in_unit;

	result.point_objects.reserve( in_data.point_objects.size() );

	for( const BaseDataRepresentation::PointObject& in_object : in_data.point_objects )
	{
		BaseDataRepresentation::PointObject out_object;
		out_object.class_= in_object.class_;
		out_object.vertex_index= result.vertices.size();
		result.vertices.push_back( in_data.vertices[ in_object.vertex_index ] );
		result.point_objects.push_back( out_object );
	} // for point objects.

	for( const BaseDataRepresentation::LinearObject& in_object : in_data.linear_objects )
	{
		BaseDataRepresentation::LinearObject out_object;
		out_object.class_= in_object.class_;
		out_object.first_vertex_index= result.vertices.size();
		out_object.vertex_count= in_object.vertex_count;

		for( size_t v= 0u; v < in_object.vertex_count; ++v )
			result.vertices.push_back( in_data.vertices[ in_object.first_vertex_index + v ] );

		result.linear_objects.push_back( out_object );
	} // for point objects.

	for( const BaseDataRepresentation::ArealObject& in_object : in_data.areal_objects )
	{
		if( in_object.multipolygon != nullptr )
		{
		}
		else
		{
			std::vector<MercatorPoint> polygon_vertices;
			polygon_vertices.reserve( in_object.vertex_count );
			for( size_t v= in_object.first_vertex_index; v < in_object.first_vertex_index + in_object.vertex_count; ++v )
				polygon_vertices.push_back( in_data.vertices[v] );

			for( const std::vector<MercatorPoint>& noncrossing_polygon_part : SplitPolygonIntNoncrossingParts( polygon_vertices ) )
			{
				for( const std::vector<MercatorPoint>& convex_part : SplitPolygonIntoConvexParts( noncrossing_polygon_part ) )
				{
					PM_ASSERT( convex_part.size() >= 3u );

					BaseDataRepresentation::ArealObject out_object;
					out_object.class_= in_object.class_;
					out_object.first_vertex_index= result.vertices.size();
					out_object.vertex_count= convex_part.size();
					for( const MercatorPoint& vertex : convex_part )
						result.vertices.push_back(vertex);
					result.areal_objects.push_back( std::move(out_object) );
				}
			}
		}
	}

	Log::Info( "Polygons normalization pass: " );
	Log::Info( result.point_objects.size(), " point objects" );
	Log::Info( result.linear_objects.size(), " linear objects" );
	Log::Info( result.areal_objects.size(), " areal objects" );
	Log::Info( result.vertices.size(), " vertices" );
	Log::Info( "" );

	return result;
}

} // namespace PanzerMaps
