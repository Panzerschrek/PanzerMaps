#pragma once

namespace PanzerMaps
{

namespace DataFileDescription
{

// Coordinates - relative to "world" of specific data file.
using GlobalCoordType= uint32_t;

// Coordinates - ralative to chunk. Unit - specific for data file.
using ChunkCoordType= uint16_t;
struct ChunkVertex
{
	// if x == CoordType::max && y == CoordType::max - vertex is dummy "EndPrimitive" vertex
	ChunkCoordType x;
	ChunkCoordType y;
};

struct Chunk
{
	// All offsets - from start of chunk.

	using StyleIndex= uint8_t;

	struct PointObjectGroup
	{
		StyleIndex style_index;
		uint16_t first_vertex;
		uint16_t vertex_count;
	};

	struct LinearObjectGroup
	{
		StyleIndex style_index;
		uint16_t first_vertex;
		uint16_t vertex_count;
	};

	struct ArealObjectGroup
	{
		StyleIndex style_index;
		uint16_t first_vertex;
		uint16_t vertex_count;
	};

	// Bounding box.
	GlobalCoordType min_x;
	GlobalCoordType min_y;
	GlobalCoordType max_x;
	GlobalCoordType max_y;

	// Data sections.
	uint32_t point_object_groups_offset;
	uint32_t linear_object_groups_offset;
	uint32_t areal_object_groups_offset;
	uint32_t vertices_offset;

	uint16_t point_object_groups_count;
	uint16_t linear_object_groups_count;
	uint16_t areal_object_groups_count;
	uint16_t vertex_count;
};

struct DataFile
{
	// All offsets - from start of file.

	static constexpr const char* c_expected_header= "PanzerMaps-Data";
	static constexpr uint32_t c_expected_version= 1u;

	uint8_t header[16];
	uint32_t version;

	uint32_t chunks_offset;
	uint32_t chunk_count;
};

} // namespace DataFile

} // namespace namespace PanzerMaps
