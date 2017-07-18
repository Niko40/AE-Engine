
#ifdef _WIN32
#include <Windows.h>
#endif

#include "../Memory/Memory.h"
#include "Logger.h"

namespace AE
{
void AE::engine_internal::WinMessageBox( String text, String caption )
{
#ifdef _WIN32
	MessageBox( NULL, text.c_str(), caption.c_str(), MB_OK );
#endif
}


Logger::Logger( fsys::path log_file_path )
{
	file_path		= log_file_path;
	fsys::remove( file_path );

	LogInfo( "Logger class created" );
}

Logger::~Logger()
{
	LogInfo( "Logger class destroyed" );
}

void Logger::SetRecordedLogTypes( LogType types )
{
	record_log_types		= types;

	uint32_t counter		= 0;
	std::string ret			= "LOG REPORT TYPES CHANGED TO: ";
	if( uint32_t( record_log_types & LogType::LT_UNKNOWN ) ) {
		ret += "UNKNOWN";
		counter++;
	}
	if( uint32_t( record_log_types & LogType::LT_INFO ) ) {
		if( counter ) ret += " | ";
		ret += "INFO";
		counter++;
	}
	if( uint32_t( record_log_types & LogType::LT_WARNING ) ) {
		if( counter ) ret += " | ";
		ret += "WARNING";
		counter++;
	}
	if( uint32_t( record_log_types & LogType::LT_ERROR ) ) {
		if( counter ) ret += " | ";
		ret += "ERROR";
		counter++;
	}
	if( uint32_t( record_log_types & LogType::LT_CRITICAL ) ) {
		if( counter ) ret += " | ";
		ret += "CRITICAL";
		counter++;
	}
	if( uint32_t( record_log_types & LogType::LT_VULKAN) ) {
		if( counter ) ret += " | ";
		ret += "VULKAN";
		counter++;
	}
	if( uint32_t( record_log_types & LogType::LT_OTHER ) ) {
		if( counter ) ret += " | ";
		ret += "OTHER";
		counter++;
	}

#if BUILD_DEBUG_LOG_FUNCTION_NAME_AND_LINE
	Write( LogType::LT_INTERNAL, ret, __func__, __LINE__ );
#else
	Write( LogType::LT_INTERNAL, ret );
#endif
}


LogType operator|( LogType type1, LogType type2 )
{
	return LogType( uint32_t( type1 ) | uint32_t( type2 ) );
}

LogType operator&( LogType type1, LogType type2 )
{
	return LogType( uint32_t( type1 ) & uint32_t( type2 ) );
}

}
