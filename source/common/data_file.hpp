#pragma once
#include <cstdint>

namespace PanzerMaps
{

namespace DataFileDescription
{

//
// All numbers are little-endian (as x86).
// All structs must have 4bytes alignment.
//

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
static_assert( sizeof(ChunkVertex) == 4u, "wrong size" );

bool operator==(const ChunkVertex& l, const ChunkVertex& r );
bool operator!=(const ChunkVertex& l, const ChunkVertex& r );

struct Chunk
{
	// All offsets - from start of chunk.

	using StyleIndex= uint8_t;

	struct PointObjectGroup
	{
		StyleIndex style_index;
		uint8_t padding[3u];
		uint16_t first_vertex;
		uint16_t vertex_count;
	};
	static_assert( sizeof(PointObjectGroup) == 8u, "wrong size" );

	struct LinearObjectGroup
	{
		StyleIndex style_index;
		uint8_t padding[1u];
		uint16_t z_level;
		uint16_t first_vertex;
		uint16_t vertex_count;
		// vertex with x= 65535 is break primitive vertex.
	};
	static_assert( sizeof(LinearObjectGroup) == 8u, "wrong size" );

	struct ArealObjectGroup
	{
		uint16_t first_vertex;
		uint16_t vertex_count;
		uint16_t z_level;
		uint8_t padding[2u];
		// vertex with x= 65535 is break primitive vertex.
		// "y" of this vertex is style index.
	};
	static_assert( sizeof(ArealObjectGroup) == 8u, "wrong size" );

	GlobalCoordType coord_start_x;
	GlobalCoordType coord_start_y;

	// Bounding box.
	GlobalCoordType min_x;
	GlobalCoordType min_y;
	GlobalCoordType max_x;
	GlobalCoordType max_y;

	uint16_t min_z_level;
	uint16_t max_z_level;

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
static_assert( sizeof(Chunk) == 52u, "wrong size" );


using ColorRGBA= unsigned char[4];
struct PointObjectStyle
{
	static constexpr const uint32_t c_icon_size_small= 16u;
	static constexpr const uint32_t c_icon_size_large= 32u;

	ColorRGBA icon_small[ c_icon_size_small * c_icon_size_small ];
	ColorRGBA icon_large[ c_icon_size_large * c_icon_size_large ];
};
static_assert( sizeof(PointObjectStyle) == 4u * ( 16u * 16u + 32u * 32u ), "wrong size" );

struct LinearObjectStyle
{
	ColorRGBA color;
	ColorRGBA color2;
	uint32_t width_mul_256; // Width of line, in units, multiplied by 256
	uint32_t dash_size_mul_256; // Dash size, in units, multiplied by 256

	uint16_t texture_width; // Non-zero, if line is textured
	uint16_t texture_height;
	uint32_t texture_data_offset;
};
static_assert( sizeof(LinearObjectStyle) == 24u, "wrong size" );

struct ArealObjectStyle
{
	ColorRGBA color;
};
static_assert( sizeof(ArealObjectStyle) == 4u, "wrong size" );

struct CommonStyle
{
	ColorRGBA background_color;

	// Copyright image - RGBA
	uint32_t copyright_image_offset;
	uint16_t copyright_image_width;
	uint16_t copyright_image_height;
};
static_assert( sizeof(CommonStyle) == 12u, "wrong size" );

struct PointStylesOrder
{
	Chunk::StyleIndex style_index;
	uint8_t padding[3u];
};
static_assert( sizeof(PointStylesOrder) == 4u, "wrong size" );

struct LinearStylesOrder
{
	Chunk::StyleIndex style_index;
	uint8_t padding[3u];
};
static_assert( sizeof(LinearStylesOrder) == 4u, "wrong size" );

struct ZoomLevel
{
	// Offsets - from data file start.
	uint32_t chunks_description_offset;
	uint32_t chunk_count;
	uint32_t zoom_level_log2;

	// Approximate unit size.
	float unit_size_m; // TODO - maybe use fixed?

	uint32_t point_styles_offset;
	uint32_t point_styles_count;
	uint32_t linear_styles_offset;
	uint32_t linear_styles_count;
	uint32_t areal_styles_offset;
	uint32_t areal_styles_count;

	uint32_t point_styles_order_offset;
	uint32_t point_styles_order_count;
	uint32_t linear_styles_order_offset;
	uint32_t linear_styles_order_count;
};
static_assert( sizeof(ZoomLevel) == 14u * 4u, "wrong size" );

struct DataFile
{
	struct ChunkDescription
	{
		uint32_t offset;
		uint32_t size;
	};

	enum class Projection : int32_t
	{
		Mercator,
		Stereographic,
		Albers,
	};

	// All offsets - from start of file.

	static constexpr const char c_expected_header[16]= "PanzerMaps-Data";
	static constexpr const uint32_t c_expected_version= 5u; // Change this each time, when DataFileDescripton structs changed.

	uint8_t header[16];
	uint32_t version;

	// Parameters of projection of data.
	Projection projection;
	double projection_min_lon, projection_min_lat;
	double projection_max_lon, projection_max_lat;
	// Parameters of linear transformation of data.
	GlobalCoordType min_x;
	GlobalCoordType min_y;
	GlobalCoordType max_x;
	GlobalCoordType max_y;
	GlobalCoordType unit_size;

	uint32_t zoom_levels_offset;
	uint32_t zoom_level_count;

	CommonStyle common_style;
};
static_assert( sizeof(DataFile) == 96u, "wrong size" );

} // namespace DataFile

} // namespace namespace PanzerMaps
