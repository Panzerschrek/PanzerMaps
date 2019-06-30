#pragma once
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace PanzerMaps
{

// Simple logger. You can write messages to it.
// Not thread-safe.
class Log
{
public:
	enum class LogLevel
	{
		User,
		Info,
		Warning,
		FatalError,
	};

	template<class...Args>
	static void User(const Args&... args );

	template<class...Args>
	static void Info(const Args&... args );

	template<class...Args>
	static void Warning( const Args&... args );

	// Exits application.
	template<class...Args>
	static void FatalError( const Args&... args );

private:
	static inline void Print( std::ostringstream& ){}

	template<class Arg0, class... Args>
	static void Print( std::ostringstream& stream, const Arg0& arg0, const Args&... args );

	template<class... Args>
	static void PrinLine( LogLevel log_level, const Args&... args );

	static void ShowFatalMessageBox( const std::string& error_message );

private:
	static std::ofstream log_file_;
};

template<class...Args>
void Log::User(const Args&... args )
{
	PrinLine( LogLevel::User, args... );
}

template<class... Args>
void Log::Info(const Args&... args )
{
	PrinLine( LogLevel::Info, args... );
}

template<class... Args>
void Log::Warning( const Args&... args )
{
	PrinLine( LogLevel::Warning, args... );
}

template<class... Args>
void Log::FatalError( const Args&... args )
{
	std::ostringstream stream;
	Print( stream, args... );
	const std::string str= stream.str();

	std::cerr << str << std::endl;
	log_file_ << str << std::endl;
	ShowFatalMessageBox( str );

	std::exit(-1);
}

template<class Arg0, class... Args>
void Log::Print( std::ostringstream& stream, const Arg0& arg0, const Args&... args )
{
	stream << arg0;
	Print( stream, args... );
}

template<class... Args>
void Log::PrinLine( const LogLevel log_level, const Args&... args )
{
	(void)log_level; // TODO - use it
	std::ostringstream stream;
	Print( stream, args... );
	const std::string str= stream.str();

#ifdef __ANDROID__
	auto android_log_level= ANDROID_LOG_INFO;
	if( log_level == LogLevel::User || log_level == LogLevel::Info )
		android_log_level= ANDROID_LOG_INFO;
	else if( log_level == LogLevel::Warning )
		android_log_level= ANDROID_LOG_WARN;
	else if( log_level == LogLevel::FatalError )
		android_log_level= ANDROID_LOG_FATAL;

	__android_log_print( android_log_level, __FILE__, "%s", str.c_str() );
#else
	std::cout << str << std::endl;
	log_file_ << str << std::endl;
#endif
}

} // namespace PanzerMaps
