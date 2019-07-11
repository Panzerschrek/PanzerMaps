#pragma once
#include <cstddef>
#include <type_traits>

namespace PanzerMaps
{

// Workaround for some versions of GCC.
struct EnumHasher
{
	template<class Enum>
	size_t operator()( const Enum enum_value ) const
	{
		static_assert( std::is_enum<Enum>::value, "Enum expected" );
		return static_cast<size_t>(enum_value);
	}
};

} // namespace PanzerMaps
