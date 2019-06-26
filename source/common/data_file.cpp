#include "data_file.hpp"

namespace PanzerMaps
{

namespace DataFileDescription
{

bool operator==(const ChunkVertex& l, const ChunkVertex& r )
{
	return l.x == r.x && l.y == r.y;
}

bool operator!=(const ChunkVertex& l, const ChunkVertex& r )
{
	return !( l == r );
}

constexpr const char DataFile::c_expected_header[16u];
constexpr const uint32_t DataFile::c_expected_version;

} // namespace DataFile

} // namespace namespace PanzerMaps
