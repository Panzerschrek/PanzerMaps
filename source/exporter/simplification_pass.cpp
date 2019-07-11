#include <unordered_map>
#include "../common/assert.hpp"
#include "../common/enum_hasher.hpp"
#include "../common/log.hpp"
#include "simplification_pass.hpp"

namespace PanzerMaps
{

struct ArealObjectVertex
{
	ArealObjectClass class_= ArealObjectClass::None;
	size_t z_level= g_zero_z_level;
	ObjectsData::VertexTransformed vertex;
};

bool operator==( const ArealObjectVertex& l, const ArealObjectVertex& r )
{
	return l.class_ == r.class_ && l.z_level == r.z_level && l.vertex == r.vertex;
}

struct ArealObjectVertexHasher
{
	size_t operator()( const ArealObjectVertex& key ) const
	{
		return EnumHasher()(key.class_) ^ std::hash<size_t>()(key.z_level) ^ std::hash<int32_t>()(key.vertex.x) ^ std::hash<int32_t>()(key.vertex.y);
	}
};

using AdjustedVerticesMap= std::unordered_map< ArealObjectVertex, size_t, ArealObjectVertexHasher >;

static void SimplifyLine_r(
	const MercatorPoint* const start_vertex,
	const MercatorPoint* const   end_vertex,
	const int64_t square_simplification_distance,
	std::vector<MercatorPoint>& out_vertices )
{
	PM_ASSERT( end_vertex - start_vertex >= 1 );
	if( end_vertex - start_vertex == 1 )
	{
		out_vertices.push_back( *start_vertex );
		return;
	}

	bool simplification_ok= true;

	const int64_t edge_dx= end_vertex->x - start_vertex->x;
	const int64_t edge_dy= end_vertex->y - start_vertex->y;
	const int64_t edge_square_length= edge_dx * edge_dx + edge_dy * edge_dy;
	if( edge_square_length == 0 || // may be in case of loops.
		edge_square_length >= ( 1L << 40L ) ) // Prevent overflows - split very large lines into parts.
		simplification_ok= false;
	else
	{
		for( const MercatorPoint* v= start_vertex + 1; v < end_vertex; ++v )
		{
			const int64_t v_dx= v->x - start_vertex->x;
			const int64_t v_dy= v->y - start_vertex->y;
			const int64_t dot= edge_dx * v_dx + edge_dy * v_dy;

			const int64_t dist_vec_dx= v_dx - edge_dx * dot / edge_square_length;
			const int64_t dist_vec_dy= v_dy - edge_dy * dot / edge_square_length;
			const int64_t dist_vec_square_length= dist_vec_dx * dist_vec_dx + dist_vec_dy * dist_vec_dy;
			if( dist_vec_square_length > square_simplification_distance )
			{
				simplification_ok= false;
				break;
			}

			const int64_t angle_dot= ( v->x - (v-1)->x ) * ( (v+1)->x - v->x ) + ( v->y - (v-1)->y ) * ( (v+1)->y - v->y );
			if( angle_dot <= 0 )
			{
				// Do not simplify sharp corners.
				simplification_ok= false;
				break;
			}
		}
	}

	if( simplification_ok )
		out_vertices.push_back( *start_vertex );
	else
	{
		const MercatorPoint* const middle= start_vertex + ( end_vertex - start_vertex ) / 2;
		SimplifyLine_r( start_vertex, middle, square_simplification_distance, out_vertices );
		SimplifyLine_r( middle, end_vertex  , square_simplification_distance, out_vertices );
	}
}

static void SimplifyLine(
	const MercatorPoint* const vertices,
	const size_t vertex_count,
	const int32_t simplification_distance_units,
	std::vector<MercatorPoint>& out_vertices )
{
	PM_ASSERT( vertex_count >= 1u );

	if( vertex_count > 1u )
		SimplifyLine_r(
			vertices,
			vertices + vertex_count - 1u,
			simplification_distance_units * simplification_distance_units,
			out_vertices );

	out_vertices.push_back( vertices[ vertex_count - 1u ] );
}

static bool HaveAdjustedVertices(
	const AdjustedVerticesMap& adjusted_vertices_map,
	const MercatorPoint& vertex,
	const ArealObjectClass object_class,
	const size_t z_level )
{
	ArealObjectVertex key;
	key.class_= object_class;
	key.z_level= z_level;
	key.vertex= vertex;

	auto it= adjusted_vertices_map.find( key );
	if( it != adjusted_vertices_map.end() && it->second > 1u )
		return true;
	return false;
}

static void SimplifyPolygon_r(
	const MercatorPoint* const start_vertex,
	const MercatorPoint* const   end_vertex,
	const int64_t square_simplification_distance,
	const AdjustedVerticesMap& adjusted_vertices_map,
	const ArealObjectClass object_class,
	const size_t z_level,
	std::vector<MercatorPoint>& out_vertices )
{
	PM_ASSERT( end_vertex - start_vertex >= 1 );
	if( end_vertex - start_vertex == 1 )
	{
		out_vertices.push_back( *start_vertex );
		return;
	}

	bool simplification_ok= true;

	const int64_t edge_dx= end_vertex->x - start_vertex->x;
	const int64_t edge_dy= end_vertex->y - start_vertex->y;
	const int64_t edge_square_length= edge_dx * edge_dx + edge_dy * edge_dy;
	if( edge_square_length == 0 || // may be in case of loops.
		edge_square_length >= ( 1L << 40L ) ) // Prevent overflows - split very large lines into parts.
		simplification_ok= false;
	else
	{
		for( const MercatorPoint* v= start_vertex + 1; v < end_vertex; ++v )
		{
			const int64_t v_dx= v->x - start_vertex->x;
			const int64_t v_dy= v->y - start_vertex->y;
			const int64_t dot= edge_dx * v_dx + edge_dy * v_dy;

			const int64_t dist_vec_dx= v_dx - edge_dx * dot / edge_square_length;
			const int64_t dist_vec_dy= v_dy - edge_dy * dot / edge_square_length;
			const int64_t dist_vec_square_length= dist_vec_dx * dist_vec_dx + dist_vec_dy * dist_vec_dy;
			if( dist_vec_square_length > square_simplification_distance )
			{
				simplification_ok= false;
				break;
			}

			const int64_t angle_dot= ( v->x - (v-1)->x ) * ( (v+1)->x - v->x ) + ( v->y - (v-1)->y ) * ( (v+1)->y - v->y );
			if( angle_dot <= 0 )
			{
				// Do not simplify sharp corners.
				simplification_ok= false;
				break;
			}

			if( HaveAdjustedVertices( adjusted_vertices_map, *v, object_class, z_level ) )
			{
				// More, then one polygons with same class and z_level share same vertex.
				// Disable simplification for such vertices.
				simplification_ok= false;
				break;
			}
		}
	}
	if( simplification_ok )
		out_vertices.push_back( *start_vertex );
	else
	{
		const MercatorPoint* const middle= start_vertex + ( end_vertex - start_vertex ) / 2;
		SimplifyPolygon_r( start_vertex, middle, square_simplification_distance, adjusted_vertices_map, object_class, z_level, out_vertices );
		SimplifyPolygon_r( middle, end_vertex  , square_simplification_distance, adjusted_vertices_map, object_class, z_level, out_vertices );
	}
}

static void SimplifyPolygon(
	const MercatorPoint* const vertices,
	const size_t vertex_count,
	const int32_t simplification_distance_units,
	const AdjustedVerticesMap& adjusted_vertices_map,
	const ArealObjectClass object_class,
	const size_t z_level,
	std::vector<MercatorPoint>& out_vertices )
{
	PM_ASSERT( vertex_count >= 3u );

	const size_t first_vertex_index= out_vertices.size();

	SimplifyPolygon_r(
		vertices,
		vertices + vertex_count - 1u,
		simplification_distance_units * simplification_distance_units,
		adjusted_vertices_map,
		object_class,
		z_level,
		out_vertices );

	out_vertices.push_back( vertices[ vertex_count - 1u ] );

	if( out_vertices.size() - first_vertex_index <= 2u )
	{
		out_vertices.resize( first_vertex_index );
		return;
	}

	// Try remove back vertex, if it is near to front.
	if( !HaveAdjustedVertices( adjusted_vertices_map, out_vertices.back(), object_class, z_level ) )
	{
		const int64_t dx= out_vertices.back().x - out_vertices[ first_vertex_index ].x;
		const int64_t dy= out_vertices.back().y - out_vertices[ first_vertex_index ].y;
		const int64_t square_distance= dx * dx + dy * dy;
		if( square_distance <= simplification_distance_units * simplification_distance_units )
		{
			out_vertices.pop_back();
			if( out_vertices.size() - first_vertex_index <= 2u )
			{
				out_vertices.resize( first_vertex_index );
				return;
			}
		}
	}

	// Discard small pulygons.
	MercatorPoint min_point= out_vertices.back(), max_point= out_vertices.back();
	for( size_t i= first_vertex_index; i < out_vertices.size(); ++i )
	{
		min_point.x= std::min( min_point.x, out_vertices[i].x );
		min_point.y= std::min( min_point.y, out_vertices[i].y );
		max_point.x= std::max( max_point.x, out_vertices[i].x );
		max_point.y= std::max( max_point.y, out_vertices[i].y );
	}
	if( max_point.x - min_point.x <= simplification_distance_units ||
		max_point.y - min_point.y <= simplification_distance_units )
	{
		out_vertices.resize( first_vertex_index );
		return;
	}
}

void SimplificationPass( ObjectsData& data, const int32_t simplification_distance_units )
{
	std::vector<ObjectsData::LinearObject> result_linear_objects;
	std::vector<ObjectsData::VertexTransformed> result_linear_objects_vertices;

	std::vector<ObjectsData::ArealObject> result_areal_objects;
	std::vector<ObjectsData::VertexTransformed> result_areal_objects_vertices;

	const int32_t simplification_distance_corrected= std::max( 1, simplification_distance_units );

	// Simplify lines.
	for( const BaseDataRepresentation::LinearObject& in_object : data.linear_objects )
	{
		BaseDataRepresentation::LinearObject out_object;
		out_object.class_= in_object.class_;
		out_object.z_level= in_object.z_level;
		out_object.first_vertex_index= result_linear_objects_vertices.size();

		SimplifyLine(
			data.linear_objects_vertices.data() + in_object.first_vertex_index,
			in_object.vertex_count,
			simplification_distance_corrected,
			result_linear_objects_vertices );
		out_object.vertex_count= result_linear_objects_vertices.size() - out_object.first_vertex_index;

		PM_ASSERT( out_object.vertex_count >= 1u );
		if( in_object.vertex_count >= 2u )
			PM_ASSERT( out_object.vertex_count >= 2u );

		result_linear_objects.push_back( out_object );
	}

	// Count adjusted vertices of areal objects.
	AdjustedVerticesMap adjusted_areal_vertices_map;
	for( const BaseDataRepresentation::ArealObject& in_object : data.areal_objects )
	{
		const auto count_vertices=
		[&]( const size_t first_vertex, const size_t vertex_count )
		{
			for( size_t v= first_vertex; v < first_vertex + vertex_count; ++v )
			{
				ArealObjectVertex vertex;
				vertex.class_= in_object.class_;
				vertex.z_level= in_object.z_level;
				vertex.vertex= data.areal_objects_vertices[v];
				const auto it= adjusted_areal_vertices_map.find(vertex);
				if( it == adjusted_areal_vertices_map.end() )
					adjusted_areal_vertices_map[vertex]= 1u;
				else
					++(it->second);
			}
		};

		count_vertices( in_object.first_vertex_index, in_object.vertex_count );
		if( in_object.multipolygon != nullptr )
		{
			for( const BaseDataRepresentation::Multipolygon::Part& inner_ring : in_object.multipolygon->inner_rings )
				count_vertices( inner_ring.first_vertex_index, inner_ring.vertex_count );
			for( const BaseDataRepresentation::Multipolygon::Part& outer_ring : in_object.multipolygon->outer_rings )
				count_vertices( outer_ring.first_vertex_index, outer_ring.vertex_count );
		}
	}

	size_t vertices_by_count[256u] { 0u };
	for( const auto& map_value : adjusted_areal_vertices_map )
		if( map_value.second < 256u )
			++vertices_by_count[map_value.second];

	// Simplify polygon contours. Disable simplification for points, which are same for 2 or more polygons.
	for( const BaseDataRepresentation::ArealObject& in_object : data.areal_objects )
	{
		const auto transform_polygon=
		[&]( const size_t in_first_vertex, const size_t in_vertex_count, size_t& out_first_vertex, size_t& out_vertex_count )
		{
			out_first_vertex= result_areal_objects_vertices.size();

			SimplifyPolygon(
				data.areal_objects_vertices.data() + in_first_vertex,
				in_vertex_count,
				std::max( 1, simplification_distance_units ),
				adjusted_areal_vertices_map,
				in_object.class_,
				in_object.z_level,
				result_areal_objects_vertices );

			out_vertex_count= result_areal_objects_vertices.size() - out_first_vertex;
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
