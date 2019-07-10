#include <algorithm>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "geometry_utils.hpp"
#include "phase_sort_pass.hpp"

namespace PanzerMaps
{

void SortByPhase( ObjectsData& data, const Styles::ZoomLevel& zoom_level )
{
	ObjectsData result;

	// Currently, point and linear object not splitted by phase.

	for( const PointObjectClass& object_class : zoom_level.point_classes_ordered )
	{
		for( const BaseDataRepresentation::PointObject& in_object : data.point_objects )
		{
			if( in_object.class_ != object_class )
				continue;

			BaseDataRepresentation::PointObject out_object;
			out_object.class_= in_object.class_;
			result.point_objects_vertices.push_back( data.point_objects_vertices[ &in_object - data.point_objects.data() ] );
			result.point_objects.push_back( out_object );
		}
	}

	std::unordered_map< LinearObjectClass, size_t > linear_classes_order;
	for( const LinearObjectClass& object_class : zoom_level.linear_classes_ordered )
	{
		linear_classes_order[ object_class ]= &object_class - zoom_level.linear_classes_ordered.data();

		for( const BaseDataRepresentation::LinearObject& in_object : data.linear_objects )
		{
			if( in_object.class_ != object_class )
				continue;

			BaseDataRepresentation::LinearObject out_object;
			out_object.class_= in_object.class_;
			out_object.z_level= in_object.z_level;
			out_object.first_vertex_index= result.linear_objects_vertices.size();
			out_object.vertex_count= in_object.vertex_count;

			for( size_t v= 0u; v < in_object.vertex_count; ++v )
				result.linear_objects_vertices.push_back( data.linear_objects_vertices[ in_object.first_vertex_index + v ] );

			result.linear_objects.push_back( out_object );
		}
	}

	// Sort lines by z_level.
	std::sort(
		result.linear_objects.begin(),
		result.linear_objects.end(),
		[&]( const BaseDataRepresentation::LinearObject& l, const BaseDataRepresentation::LinearObject& r )
		{
			if( l.z_level != r.z_level )
				return l.z_level < r.z_level;
			return linear_classes_order[l.class_] < linear_classes_order[r.class_];
		} );

	for( size_t z_level= 0u; z_level <= g_max_z_level; ++z_level )
	for( const Styles::ArealObjectPhase& phase : zoom_level.areal_object_phases )
	{
		std::vector< ObjectsData::ArealObject > areal_objects;
		for( const BaseDataRepresentation::ArealObject& in_object : data.areal_objects )
		{
			if( in_object.z_level != z_level || phase.classes.count(in_object.class_) == 0 )
				continue;

			BaseDataRepresentation::ArealObject out_object;
			out_object.class_= in_object.class_;
			out_object.z_level= in_object.z_level;

			if( in_object.multipolygon != nullptr )
			{
				out_object.multipolygon.reset( new BaseDataRepresentation::Multipolygon );
				out_object.first_vertex_index= out_object.vertex_count= 0u;

				for( const BaseDataRepresentation::Multipolygon::Part& inner_ring : in_object.multipolygon->inner_rings )
				{
					out_object.multipolygon->inner_rings.emplace_back();
					BaseDataRepresentation::Multipolygon::Part& out_ring= out_object.multipolygon->inner_rings.back();

					out_ring.first_vertex_index= result.areal_objects_vertices.size();
					out_ring.vertex_count= inner_ring.vertex_count;
					result.areal_objects_vertices.insert( result.areal_objects_vertices.end(), data.areal_objects_vertices.data() + inner_ring.first_vertex_index, data.areal_objects_vertices.data() + inner_ring.first_vertex_index + inner_ring.vertex_count );
				}
				for( const BaseDataRepresentation::Multipolygon::Part& outer_ring : in_object.multipolygon->outer_rings )
				{
					out_object.multipolygon->outer_rings.emplace_back();
					BaseDataRepresentation::Multipolygon::Part& out_ring= out_object.multipolygon->outer_rings.back();

					out_ring.first_vertex_index= result.areal_objects_vertices.size();
					out_ring.vertex_count= outer_ring.vertex_count;
					result.areal_objects_vertices.insert( result.areal_objects_vertices.end(), data.areal_objects_vertices.data() + outer_ring.first_vertex_index, data.areal_objects_vertices.data() + outer_ring.first_vertex_index + outer_ring.vertex_count );
				}
			}
			else
			{
				out_object.first_vertex_index= result.areal_objects_vertices.size();
				out_object.vertex_count= in_object.vertex_count;
				for( size_t v= 0u; v < in_object.vertex_count; ++v )
					result.areal_objects_vertices.push_back( data.areal_objects_vertices[ in_object.first_vertex_index + v ] );
			}
			areal_objects.push_back( std::move(out_object) );
		}

		const auto calculate_polyon_double_area=
		[&]( const  ObjectsData::ArealObject& polygon ) -> int64_t
		{
			if( polygon.multipolygon != nullptr )
			{
				int64_t accumulated_area= 0;
				// Area = total area of outer polygons - area of holes.
				for( const ObjectsData::Multipolygon::Part& outer_ring : polygon.multipolygon->outer_rings )
					accumulated_area+= std::abs( CalculatePolygonDoubleSignedArea( result.areal_objects_vertices.data() + outer_ring.first_vertex_index, outer_ring.vertex_count ) );

				for( const ObjectsData::Multipolygon::Part& inner_ring : polygon.multipolygon->inner_rings )
					accumulated_area-= std::abs( CalculatePolygonDoubleSignedArea( result.areal_objects_vertices.data() + inner_ring.first_vertex_index, inner_ring.vertex_count ) );
				return accumulated_area;
			}
			else
				return std::abs( CalculatePolygonDoubleSignedArea( result.areal_objects_vertices.data() + polygon.first_vertex_index, polygon.vertex_count ) );
		};

		// Sort by area in descent order.
		std::sort(
			areal_objects.begin(),
			areal_objects.end(),
			[&]( const ObjectsData::ArealObject& l, const ObjectsData::ArealObject& r )
			{
				return calculate_polyon_double_area(l) > calculate_polyon_double_area(r);
			} );

		for( auto& areal_object : areal_objects )
			result.areal_objects.push_back( std::move(areal_object) );
	}

	data.point_objects= std::move(result.point_objects);
	data.point_objects_vertices= std::move(result.point_objects_vertices);
	data.linear_objects= std::move(result.linear_objects);
	data.linear_objects_vertices= std::move(result.linear_objects_vertices);
	data.areal_objects= std::move(result.areal_objects);
	data.areal_objects_vertices= std::move(result.areal_objects_vertices);

	PM_ASSERT( data.point_objects.size() == data.point_objects_vertices.size() );

	Log::Info( "Phase sort pass: " );
	Log::Info( zoom_level.point_classes_ordered.size(), " point classes" );
	Log::Info( zoom_level.linear_classes_ordered.size(), " linear classes" );
	Log::Info( zoom_level.areal_object_phases.size(), " areal objects phases" );
	Log::Info( data.point_objects.size(), " point objects" );
	Log::Info( data.linear_objects.size(), " linear objects" );
	Log::Info( data.linear_objects_vertices.size(), " linear objects vertices" );
	Log::Info( data.areal_objects.size(), " areal objects" );
	Log::Info( data.areal_objects_vertices.size(), " areal objects vertices" );
	Log::Info( "" );
}

} // namespace PanzerMaps
