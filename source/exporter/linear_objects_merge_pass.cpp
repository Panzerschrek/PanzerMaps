#include <unordered_map>
#include <unordered_set>
#include "../common/assert.hpp"
#include "../common/log.hpp"
#include "linear_objects_merge_pass.hpp"

namespace PanzerMaps
{

struct LinearObjectForMerge
{
	std::vector<ObjectsData::VertexTransformed> vertices;
	LinearObjectClass class_= LinearObjectClass::None;
	size_t z_level= g_zero_z_level;
};
using LinearObjectForMergePtr= std::shared_ptr<LinearObjectForMerge>;

struct LinearObjectKey
{
	LinearObjectClass class_= LinearObjectClass::None;
	size_t z_level= g_zero_z_level;
	ObjectsData::VertexTransformed vertex; // strart or finish.
};

bool operator==( const LinearObjectKey& l, const LinearObjectKey& r )
{
	return l.class_ == r.class_ && l.z_level == r.z_level && l.vertex == r.vertex;
}

struct LinearObjectKeyHasher
{
	size_t operator()( const LinearObjectKey& key ) const
	{
		return std::hash<LinearObjectClass>()(key.class_) ^ std::hash<size_t>()(key.z_level) ^ std::hash<int32_t>()(key.vertex.x) ^ std::hash<int32_t>()(key.vertex.y);
	}
};

using LinearObjectsMap= std::unordered_map< LinearObjectKey, LinearObjectForMergePtr, LinearObjectKeyHasher >;

static void EraseObjectFromMap( LinearObjectsMap& linear_objects_map, const LinearObjectForMergePtr& object )
{
	LinearObjectKey key_front, key_back;
	key_front.class_ = key_back.class_ = object->class_ ;
	key_front.z_level= key_back.z_level= object->z_level;
	key_front.vertex= object->vertices.front();
	key_back .vertex= object->vertices.back ();

	PM_ASSERT( linear_objects_map.count( key_front ) == 1 );
	PM_ASSERT( linear_objects_map.count( key_back  ) == 1 );

	linear_objects_map.erase( key_front );
	linear_objects_map.erase( key_back  );
}

static void PutObjectToMap( LinearObjectsMap& linear_objects_map, const LinearObjectForMergePtr& object )
{
	LinearObjectKey key_front, key_back;
	key_front.class_ = key_back.class_ = object->class_ ;
	key_front.z_level= key_back.z_level= object->z_level;
	key_front.vertex= object->vertices.front();
	key_back .vertex= object->vertices.back ();

	const auto front_it= linear_objects_map.find( key_front );
	const auto back_it = linear_objects_map.find( key_back  );
	if( front_it != linear_objects_map.end() )
	{
		LinearObjectForMergePtr out_object= front_it->second;
		PM_ASSERT( out_object != object );
		EraseObjectFromMap( linear_objects_map, out_object );

		if( key_front.vertex == out_object->vertices.front() )
			out_object->vertices.insert( out_object->vertices.begin(), object->vertices.rbegin(), object->vertices.rend() - 1 );
		else
		{
			PM_ASSERT( key_front.vertex == out_object->vertices.back() );
			out_object->vertices.insert( out_object->vertices.end(), object->vertices.begin() + 1, object->vertices.end() );
		}

		PutObjectToMap( linear_objects_map, out_object );
	}
	else if( back_it != linear_objects_map.end() )
	{
		LinearObjectForMergePtr out_object= back_it->second;
		PM_ASSERT( out_object != object );
		EraseObjectFromMap( linear_objects_map, out_object );

		if( key_back.vertex == out_object->vertices.front() )
			out_object->vertices.insert( out_object->vertices.begin(), object->vertices.begin(), object->vertices.end() - 1 );
		else
		{
			PM_ASSERT( key_back.vertex == out_object->vertices.back() );
			out_object->vertices.insert( out_object->vertices.end(), object->vertices.rbegin() + 1, object->vertices.rend() );
		}

		PutObjectToMap( linear_objects_map, out_object );
	}
	else
	{
		linear_objects_map[ key_front ]= object;
		linear_objects_map[ key_back  ]= object;
	}
}

void MergeLinearObjects( ObjectsData& data )
{
	LinearObjectsMap linear_objects_map;

	for( const ObjectsData::LinearObject& in_object : data.linear_objects )
	{
		std::vector<ObjectsData::VertexTransformed> vertices(
			data.linear_objects_vertices.data() + in_object.first_vertex_index,
			data.linear_objects_vertices.data() + in_object.first_vertex_index + in_object.vertex_count );

		const LinearObjectForMergePtr out_object= std::make_shared<LinearObjectForMerge>();
		out_object->class_= in_object.class_;
		out_object->z_level= in_object.z_level;
		out_object->vertices= std::move(vertices);
		PutObjectToMap( linear_objects_map, out_object );
	}

	std::unordered_set<LinearObjectForMergePtr> linear_objects_unique;
	for( const auto& map_value : linear_objects_map )
		linear_objects_unique.insert(map_value.second);

	data.linear_objects.clear();
	data.linear_objects_vertices.clear();

	for( const LinearObjectForMergePtr& in_object : linear_objects_unique )
	{
		ObjectsData::LinearObject out_object;
		out_object.class_= in_object->class_;
		out_object.z_level= in_object->z_level;
		out_object.first_vertex_index= data.linear_objects_vertices.size();
		out_object.vertex_count= in_object->vertices.size();
		data.linear_objects_vertices.insert( data.linear_objects_vertices.end(), in_object->vertices.begin(), in_object->vertices.end() );
		data.linear_objects.push_back(out_object);
	}


	Log::Info( "Linear objects merge pass: " );
	Log::Info( data.linear_objects.size(), " linear objects" );
	Log::Info( data.linear_objects_vertices.size(), " linear objects vertices" );
	Log::Info( "" );
}

} // namespace PanzerMaps
