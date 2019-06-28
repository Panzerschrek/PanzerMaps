#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.hpp"
#include "memory_mapped_file.hpp"

namespace PanzerMaps
{

MemoryMappedFilePtr MemoryMappedFile::Create( const char* const file_name )
{
	// TODO - support Windows.
	const FileDescriptor file_descriptor= ::open( file_name, O_LARGEFILE );
	if( file_descriptor == -1 )
	{
		Log::Warning( "Error, opening file \"", file_name, "\". Error code: ", errno );
		return nullptr;
	}

	struct stat file_statistics;
	const int stat_result= ::fstat( file_descriptor, &file_statistics );
	if( stat_result != 0 )
	{
		Log::Warning( "Error, getting file size. Error code: ", errno );
		::close( file_descriptor );
		return nullptr;
	}

	const size_t file_size= file_statistics.st_size;

	const void* const data= ::mmap( nullptr, file_size, PROT_READ, MAP_SHARED, file_descriptor, 0u );
	if( data == nullptr )
	{
		Log::Warning( "Error, mapping file \"", file_name, "\". Error code: ", errno );
		::close( file_descriptor );
		return nullptr;
	}

	return MemoryMappedFilePtr( new MemoryMappedFile( data, file_size, file_descriptor ) );
}

MemoryMappedFile::MemoryMappedFile( const void* const data, const size_t size, const FileDescriptor file_descriptor )
	: data_(data), size_(size), file_descriptor_(file_descriptor)
{}

MemoryMappedFile::~MemoryMappedFile()
{
	::munmap( const_cast<void*>(data_), size_ );
	::close( file_descriptor_ );
}

} // namespace PanzerMaps
