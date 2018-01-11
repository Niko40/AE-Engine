#pragma once

#include <filesystem>
#include <time.h>
#include <sstream>
#include <fstream>

#include "../BUILD_OPTIONS.h"
#include "../Platform.h"

#include "../Memory/MemoryTypes.h"
#include "../CppFileSystem/CppFileSystem.h"

namespace AE
{
namespace engine_internal
{
void WinMessageBox( String text, String caption );
}

enum class LogType : uint32_t
{
	LT_UNKNOWN		= 1 << 0,
	LT_INFO			= 1 << 1,
	LT_WARNING		= 1 << 2,
	LT_ERROR		= 1 << 3,
	LT_CRITICAL		= 1 << 4,

	LT_VULKAN		= 1 << 16,

	LT_OTHER		= 1 << 29,
	LT_INTERNAL		= 1 << 30,
};

LogType operator|( LogType type1, LogType type2 );
LogType operator&( LogType type1, LogType type2 );

class Logger
{
public:
	Logger( fsys::path log_file_path );
	~Logger();

	void	SetRecordedLogTypes( LogType types );

	template<typename T>
	void	LogInfo( T data ) {};
	template<typename T>
	void	LogWarning( T data ) {};
	template<typename T>
	void	LogError( T data ) {};
	template<typename T>
	void	LogCritical( T data ) {};
	template<typename T>
	void	LogVulkan( T data ) {};
	template<typename T>
	void	LogOther( T data ) {};

	template<typename T>
#if BUILD_DEBUG_LOG_FUNCTION_NAME_AND_LINE
	void Write( LogType log_type, T data, std::string function, int line )
#else
	void Write( LogType log_type, T data )
#endif
	{
		if( uint32_t( log_type & record_log_types ) || uint32_t( log_type & LogType::LT_INTERNAL ) || uint32_t( log_type & LogType::LT_CRITICAL ) ) {

			std::stringstream log;
			log.clear();

			auto time = std::time( nullptr );
			if( time != last_time ) {
				last_time = time;
				char ctime[ 32 ];
				ctime_s( ctime, sizeof( ctime ), &time );
				log << ctime;
			}
			log << "  -";

			switch( log_type ) {
			case LogType::LT_UNKNOWN:
				log << "UNKNOWN:  ";
				break;
			case LogType::LT_INFO:
				log << "INFO:     ";
				break;
			case LogType::LT_WARNING:
				log << "WARNING:  ";
				break;
			case LogType::LT_ERROR:
				log << "ERROR:    ";
				break;
			case LogType::LT_CRITICAL:
				log << "CRITICAL: ";
				break;
			case LogType::LT_VULKAN:
				break;
			case LogType::LT_OTHER:
				log << "OTHER:    ";
				break;
			case LogType::LT_INTERNAL:
				log << "OTHER:    ";
				break;
			default:
				log << "LOGERROR: ";
				break;
			}

#if BUILD_DEBUG_LOG_FUNCTION_NAME_AND_LINE
			log << function << "()" << " : " << line << " : ";
#endif
			log << data;

			std::ofstream file( file_path, std::ofstream::out | std::ofstream::app );
			file << log.str() << std::endl;
			file.flush();
			file.close();

			if( log_type == LogType::LT_CRITICAL ) {
				engine_internal::WinMessageBox( log.str().c_str(), "Critical error!" );
				std::exit( -1 );
			}
		}
	}

private:
	fsys::path				file_path				= "";

	LogType					record_log_types		= LogType::LT_UNKNOWN | LogType::LT_INFO | LogType::LT_WARNING | LogType::LT_ERROR | LogType::LT_CRITICAL | LogType::LT_VULKAN | LogType::LT_OTHER;

	time_t					last_time				= 0;
};

#if BUILD_DEBUG_LOG_ASSERT_INFO_LINES
#define assert_LogLineInfo ;assert( 0 )
#else
#define assert_LogLineInfo
#endif

#if BUILD_DEBUG_LOG_ASSERT_WARNING_LINES
#define assert_LogLineWarning ;assert( 0 )
#else
#define assert_LogLineWarning
#endif

#if BUILD_DEBUG_LOG_ASSERT_ERROR_LINES
#define assert_LogLineError ;assert( 0 )
#else
#define assert_LogLineError
#endif

#if BUILD_DEBUG_LOG_ASSERT_VULKAN_LINES
#define assert_LogLineVulkan ;assert( 0 )
#else
#define assert_LogLineVulkan
#endif

#if BUILD_DEBUG_LOG_ASSERT_OTHER_LINES
#define assert_LogLineOther ;assert( 0 )
#else
#define assert_LogLineOther
#endif

#if BUILD_DEBUG_LOG_FUNCTION_NAME_AND_LINE
#define LogInfo( message )			Write( AE::LogType::LT_INFO, message, __func__, __LINE__ ) assert_LogLineInfo
#define LogWarning( message )		Write( AE::LogType::LT_WARNING, message, __func__, __LINE__) assert_LogLineWarning
#define LogError( message )			Write( AE::LogType::LT_ERROR, message, __func__, __LINE__ ) assert_LogLineError
#define LogCritical( message )		Write( AE::LogType::LT_CRITICAL, message, __func__, __LINE__ ); assert( 0 )
#define LogVulkan( message )		Write( AE::LogType::LT_VULKAN, message, __func__, __LINE__ ) assert_LogLineVulkan
#define LogOther( message )			Write( AE::LogType::LT_OTHER, message, __func__, __LINE__ ) assert_LogLineOther
#else
#define LogInfo( message )			Write( AE::LogType::LT_INFO, message ) assert_LogLineInfo
#define LogWarning( message )		Write( AE::LogType::LT_WARNING, message ) assert_LogLineWarning
#define LogError( message )			Write( AE::LogType::LT_ERROR, message ) assert_LogLineError
#define LogCritical( message )		Write( AE::LogType::LT_CRITICAL, message ); assert( 0 )
#define LogVulkan( message )		Write( AE::LogType::LT_VULKAN, message ) assert_LogLineVulkan
#define LogOther( message )			Write( AE::LogType::LT_OTHER, message ) assert_LogLineOther
#endif

}
