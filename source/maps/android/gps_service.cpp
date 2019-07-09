#include <SDL_system.h>
#include "../../common/log.hpp"
#include "gps_service.hpp"

namespace PanzerMaps
{

static bool CheckClearException( JNIEnv* const jni_env ) // Returns true, if ok
{
	jthrowable exception= jni_env->ExceptionOccurred();
	if( exception != nullptr )
	{
		jni_env->ExceptionDescribe();
		jni_env->ExceptionClear();
		return false;
	}
	return true;
}

GPSService::GPSService()
{
	Log::Info( "Start creating GPS service" );

	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	gps_source_class_= jni_env->FindClass( "panzerschrek/panzermaps/app/GPSSource" );
	if( gps_source_class_ == nullptr )
	{
		Log::Warning( "Can not get \"GPSSource\" class" );
		return;
	}

	enable_method_ = jni_env->GetStaticMethodID( gps_source_class_, "Enable","(Landroid/app/Activity;)V" );
	disable_method_= jni_env->GetStaticMethodID( gps_source_class_, "Disable","()V" );
	get_longitude_method_= jni_env->GetStaticMethodID( gps_source_class_, "GetLongitude","()D" );
	get_latitude_method_ = jni_env->GetStaticMethodID( gps_source_class_, "GetLatitude" ,"()D" );
	if( enable_method_ == nullptr )
		Log::Warning( "Can not get \"Enable\" method" );
	if( disable_method_ == nullptr )
		Log::Warning( "Can not get \"Disable\" method" );
	if( get_longitude_method_ == nullptr )
		Log::Warning( "Can not get \"GetLongitude\" method" );
	if( get_latitude_method_  == nullptr )
		Log::Warning( "Can not get \"GetLatitude\"  method" );

	initialized_ok_=
		gps_source_class_ != nullptr &&
		enable_method_  != nullptr &&
		disable_method_ != nullptr &&
		get_longitude_method_ != nullptr &&
		get_latitude_method_  != nullptr;
}

void GPSService::SetEnabled( const bool enabled )
{
	if( !initialized_ok_ )
		return;

	if( enabled == enabled_ )
		return;

	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	if( enabled )
	{
		Log::Info( "Enable GPSService" );

		jobject sdl_activity= static_cast<jobject>(SDL_AndroidGetActivity());
		if( sdl_activity == nullptr )
		{
			Log::Warning( "Can not get SDL activity" );
			return;
		}

		jni_env->CallStaticObjectMethod( gps_source_class_, enable_method_, sdl_activity );
		jni_env->DeleteLocalRef( sdl_activity );
		if( CheckClearException( jni_env ) )
		{
			Log::Info( "Enable GPSService success" );
			enabled_= true;
		}
		else
			Log::Info( "Enable GPSService failed" );
	}
	else
	{
		Log::Info( "Disable GPSService" );

		jni_env->CallStaticObjectMethod( gps_source_class_, disable_method_ );
		if( CheckClearException( jni_env ) )
		{
			Log::Info( "Disable GPSService success" );
			enabled_= false;
		}
		else
			Log::Info( "Disable GPSService failed" );
	}
}

GPSService::~GPSService()
{
	SetEnabled(false);
}

void GPSService::Update()
{
	if( !initialized_ok_ )
		return;

	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	GeoPoint new_position;

	new_position.x= jni_env->CallStaticDoubleMethod( gps_source_class_, get_longitude_method_ );
	if( !CheckClearException( jni_env ) )
		return;

	new_position.y= jni_env->CallStaticDoubleMethod( gps_source_class_, get_latitude_method_  );
	if( !CheckClearException( jni_env ) )
		return;

	if( new_position != gps_position_ )
	{
		gps_position_= new_position;
		Log::Info( "Coordinates: ", new_position.x, " ", new_position.y );
	}
}

} // namespace PanzerMaps
