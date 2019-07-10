#include "../common/log.hpp"
#include "simplification_pass.hpp"

namespace PanzerMaps
{

void SimplificationPass( ObjectsData& data, const int32_t simplification_distance_units )
{
	if( simplification_distance_units <= 0 )
		return;

	std::vector<ObjectsData::LinearObject> result_linear_objects;
	std::vector<ObjectsData::VertexTranspormed> result_linear_objects_vertices;

	std::vector<ObjectsData::ArealObject> result_areal_objects;
	std::vector<ObjectsData::VertexTranspormed> result_areal_objects_vertices;

	const int32_t simplification_suqare_distance= simplification_distance_units * simplification_distance_units;
	const auto vertices_near=
	[&]( const MercatorPoint& p0, const MercatorPoint& p1 ) -> bool
	{
		const int32_t dx= p1.x - p0.x;
		const int32_t dy= p1.y - p0.y;
		return dx * dx + dy * dy <= simplification_suqare_distance;
	};

	// TODO - improve simplification.
	// For linear objects remove collinear points.
	// For areal objects remove collinear point, but NOT remove points, which are same for multiple areal objects of same class and z level.

	// Remove equal adjusted vertices of linear objects.
	for( const BaseDataRepresentation::LinearObject& in_object : data.linear_objects )
	{
		BaseDataRepresentation::LinearObject out_object;
		out_object.class_= in_object.class_;
		out_object.z_level= in_object.z_level;
		out_object.first_vertex_index= result_linear_objects_vertices.size();
		out_object.vertex_count= 1u;
		result_linear_objects_vertices.push_back( data.linear_objects_vertices[ in_object.first_vertex_index ] );

		for( size_t v= in_object.first_vertex_index + 1u; v < in_object.first_vertex_index + in_object.vertex_count; ++v )
		{
			const ObjectsData::VertexTranspormed& vertex=data.linear_objects_vertices[ v ];
			if( !vertices_near( vertex, result_linear_objects_vertices.back() ) )
			{
				result_linear_objects_vertices.push_back( vertex );
				++out_object.vertex_count;
			}
			else if( vertex != result_linear_objects_vertices.back() && v == in_object.first_vertex_index + in_object.vertex_count - 1u )
			{
				result_linear_objects_vertices.push_back( vertex );
				++out_object.vertex_count;
			}
		}

		result_linear_objects.push_back( out_object );
	}

	// Remove equal adjusted vertices of areal objects. Remove too small areal objects.
	for( const BaseDataRepresentation::ArealObject& in_object : data.areal_objects )
	{
		const auto transform_polygon=
		[&]( const size_t in_first_vertex, const size_t in_vertex_count, size_t& out_first_vertex, size_t& out_vertex_count )
		{
			out_first_vertex= result_areal_objects_vertices.size();
			out_vertex_count= 1u;

			result_areal_objects_vertices.push_back( data.areal_objects_vertices[in_first_vertex] );

			for( size_t v= in_first_vertex + 1u; v < in_first_vertex + in_vertex_count; ++v )
			{
				const ObjectsData::VertexTranspormed& vertex= data.areal_objects_vertices[v];
				if( !vertices_near( vertex, result_areal_objects_vertices.back() ) )
				{
					result_areal_objects_vertices.push_back( vertex );
					++out_vertex_count;
				}
			}

			if( out_vertex_count >= 3u &&
				vertices_near( result_areal_objects_vertices[ out_first_vertex ], result_areal_objects_vertices[ out_first_vertex + out_vertex_count - 1u ] ) )
			{
				result_areal_objects_vertices.pop_back(); // Remove duplicated start and end vertex.
				--out_vertex_count;
			}
			if( out_vertex_count < 3u )
			{
				result_areal_objects_vertices.resize( result_areal_objects_vertices.size() - out_vertex_count ); // Polygon is too small.
				out_vertex_count= 0u;
			}
		};

		if( in_object.multipolygon != nullptr )
		{
			BaseDataRepresentation::Multipolygon out_multipolygon;
			for( const BaseDataRepresentation::Multipolygon::Part& inner_ring : in_object.multipolygon->inner_rings )
			{
				BaseDataRepresentation::Multipolygon::Part out_part;
				transform_polygon( inner_ring.first_vertex_index, inner_ring.vertex_count, out_part.first_vertex_index, out_part.vertex_count );
				if( out_part.vertex_count > 0u )
					out_multipolygon.inner_rings.push_back(out_part);
			}
			for( const BaseDataRepresentation::Multipolygon::Part& outer_ring : in_object.multipolygon->outer_rings )
			{
				BaseDataRepresentation::Multipolygon::Part out_part;
				transform_polygon( outer_ring.first_vertex_index, outer_ring.vertex_count, out_part.first_vertex_index, out_part.vertex_count );
				if( out_part.vertex_count > 0u )
					out_multipolygon.outer_rings.push_back(out_part);
			}

			if( !out_multipolygon.outer_rings.empty() )
			{
				BaseDataRepresentation::ArealObject out_object;
				out_object.class_= in_object.class_;
				out_object.z_level= in_object.z_level;
				out_object.first_vertex_index= out_object.vertex_count= 0u;
				out_object.multipolygon.reset( new BaseDataRepresentation::Multipolygon( std::move(out_multipolygon) ) );
				result_areal_objects.push_back( std::move(out_object) );
			}
		}
		else
		{
			BaseDataRepresentation::ArealObject out_object;
			out_object.class_= in_object.class_;
			out_object.z_level= in_object.z_level;

			transform_polygon( in_object.first_vertex_index, in_object.vertex_count, out_object.first_vertex_index, out_object.vertex_count );
			if( out_object.vertex_count > 0u )
				result_areal_objects.push_back( std::move(out_object) );
		}
	}

	data.linear_objects= std::move(result_linear_objects);
	data.linear_objects_vertices= std::move(result_linear_objects_vertices);
	data.areal_objects= std::move(result_areal_objects);
	data.areal_objects_vertices= std::move(result_areal_objects_vertices);

	Log::Info( "Simplification pass: " );
	Log::Info( "Simplification distance: ", data.coordinates_scale * simplification_distance_units );
	Log::Info( data.linear_objects.size(), " linear objects" );
	Log::Info( data.linear_objects_vertices.size(), " linear objects vertices" );
	Log::Info( data.areal_objects.size(), " areal objects" );
	Log::Info( data.areal_objects_vertices.size(), " areal objects vertices" );
	Log::Info( "" );
}

} // namespace PanzerMaps
