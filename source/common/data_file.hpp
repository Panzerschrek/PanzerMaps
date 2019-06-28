#pragma once
#include <cstdint>

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
	// if x == CoordType::max vertex is dummy "EndPrimitive" vertex. "y" of dummy vertex may be used for additional data.
	ChunkCoordType x;
	ChunkCoordType y;
};

bool operator==(const ChunkVertex& l, const ChunkVertex& r );
bool operator!=(const ChunkVertex& l, const ChunkVertex& r );

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
		// vertex with x= 65535 is break primitive vertex.
	};

	struct ArealObjectGroup
	{
		uint16_t first_vertex;
		uint16_t vertex_count;
		// vertex with x= 65535 is break primitive vertex.
		// "y" of this vertex is style index.
	};

	GlobalCoordType coord_start_x;
	GlobalCoordType coord_start_y;

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

using ColorRGBA= unsigned char[4];
struct PointObjectStyle
{
};

struct LinearObjectStyle
{
	ColorRGBA color;
	uint32_t width_mul_256; // Width of line, in units, multiplied by 256
};

struct ArealObjectStyle
{
	ColorRGBA color;
};

struct CommonStyle
{
	ColorRGBA background_color;
};

struct ZoomLevel
{
	// Offsets - from data file start.
	uint32_t chunks_description_offset;
	uint32_t chunk_count;
	uint32_t zoom_level_log2;

	uint32_t point_styles_offset;
	uint32_t point_styles_count;
	uint32_t linear_styles_offset;
	uint32_t linear_styles_count;
	uint32_t areal_styles_offset;
	uint32_t areal_styles_count;
};

struct DataFile
{
	struct ChunkDescription
	{
		uint32_t offset;
		uint32_t size;
	};

	// All offsets - from start of file.

	static constexpr const char c_expected_header[16]= "PanzerMaps-Data";
	static constexpr const uint32_t c_expected_version= 1u;

	uint8_t header[16];
	uint32_t version;

	uint32_t zoom_levels_offset;
	uint32_t zoom_level_count;

	CommonStyle common_style;
};

} // namespace DataFile

} // namespace namespace PanzerMaps
