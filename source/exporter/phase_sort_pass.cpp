#include <algorithm>
#include "../common/log.hpp"
#include "geometry_utils.hpp"
#include "phase_sort_pass.hpp"

namespace PanzerMaps
{

PhaseSortResult SortByPhase( const CoordinatesTransformationPassResult& in_data, const Styles::ZoomLevel& zoom_level )
{
	PhaseSortResult result;
	result.min_point= in_data.min_point;
	result.max_point= in_data.max_point;
	result.start_point= in_data.start_point;
	result.coordinates_scale= in_data.coordinates_scale;
	result.zoom_level= in_data.zoom_level;
	result.meters_in_unit= in_data.meters_in_unit;

	// Currently, point and linear object not splitted by phase.

	for( const PointObjectClass& object_class : zoom_level.point_classes_ordered )
	{
		for( const BaseDataRepresentation::PointObject& in_object : in_data.point_objects )
		{
			if( in_object.class_ != object_class )
				continue;

			BaseDataRepresentation::PointObject out_object;
			out_object.class_= in_object.class_;
			out_object.vertex_index= result.vertices.size();
			result.vertices.push_back( in_data.vertices[ in_object.vertex_index ] );
			result.point_objects.push_back( out_object );
		}
	}

	for( const LinearObjectClass& object_class : zoom_level.linear_classes_ordered )
	{
		for( const BaseDataRepresentation::LinearObject& in_object : in_data.linear_objects )
		{
			if( in_object.class_ != object_class )
				continue;

			BaseDataRepresentation::LinearObject out_object;
			out_object.class_= in_object.class_;
			out_object.first_vertex_index= result.vertices.size();
			out_object.vertex_count= in_object.vertex_count;

			for( size_t v= 0u; v < in_object.vertex_count; ++v )
				result.vertices.push_back( in_data.vertices[ in_object.first_vertex_index + v ] );

			result.linear_objects.push_back( out_object );
		}
	}

	for( const Styles::ArealObjectPhase& phase : zoom_level.areal_object_phases )
	{
		 std::vector< PhaseSortResult::ArealObject > areal_objects;
		for( const BaseDataRepresentation::ArealObject& in_object : in_data.areal_objects )
		{
			if( phase.classes.count(in_object.class_) == 0 )
				continue;

			BaseDataRepresentation::ArealObject out_object;
			out_object.class_= in_object.class_;
			out_object.first_vertex_index= result.vertices.size();
			out_object.vertex_count= in_object.vertex_count;

			for( size_t v= 0u; v < in_object.vertex_count; ++v )
				result.vertices.push_back( in_data.vertices[ in_object.first_vertex_index + v ] );

			areal_objects.push_back( std::move(out_object) );
		}

		// Sort by area in descent order.
		std::sort(
			areal_objects.begin(),
			areal_objects.end(),
			[&result]( const PhaseSortResult::ArealObject& l, const PhaseSortResult::ArealObject& r )
			{
				return
					std::abs( CalculatePolygonDoubleSignedArea( result.vertices.data() + l.first_vertex_index, l.vertex_count ) )
						>
					std::abs( CalculatePolygonDoubleSignedArea( result.vertices.data() + r.first_vertex_index, r.vertex_count ) );
			} );

		for( auto& areal_object : areal_objects )
			result.areal_objects.push_back( std::move(areal_object) );
	}

	Log::Info( "Phase sort pass: " );
	Log::Info( zoom_level.point_classes_ordered.size(), " point classes" );
	Log::Info( zoom_level.linear_classes_ordered.size(), " linear classes" );
	Log::Info( zoom_level.areal_object_phases.size(), " areal objects phases" );
	Log::Info( "" );

	return result;
}

} // namespace PanzerMaps
