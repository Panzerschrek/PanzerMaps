#include <SDL_system.h>
#include "../../common/log.hpp"
#include "gps_service.hpp"

namespace PanzerMaps
{

GPSService::GPSService()
{
	Log::Info( "Start creating GPS service" );

	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	jobject sdl_activity= static_cast<jobject>(SDL_AndroidGetActivity());
	if( sdl_activity == nullptr )
	{
		Log::Warning( "Can not get SDL activity" );
		return;
	}

	gps_source_class_= jni_env->FindClass( "panzerschrek/panzermaps/app/GPSSource" );
	if( gps_source_class_ == nullptr )
	{
		Log::Warning( "Can not get \"GPSSource\" class" );
		jni_env->DeleteLocalRef( sdl_activity );
		return;
	}

	get_longitude_method_= jni_env->GetStaticMethodID( gps_source_class_, "GetLongitude","()D" );
	get_latitude_method_ = jni_env->GetStaticMethodID( gps_source_class_, "GetLatitude" ,"()D" );
	jmethodID enable_gps_source_method= jni_env->GetStaticMethodID( gps_source_class_, "Enable","(Landroid/app/Activity;)V" );

	if( get_longitude_method_ == nullptr )
	{
		Log::Warning( "Can not get \"GetLongitude\" method" );
		jni_env->DeleteLocalRef( sdl_activity );
		return;
	}
	if( get_latitude_method_  == nullptr )
	{
		Log::Warning( "Can not get \"GetLatitude\"  method" );
		jni_env->DeleteLocalRef( sdl_activity );
		return;
	}
	if( enable_gps_source_method == nullptr )
	{
		Log::Warning( "Can not get \"Enable\" method" );
		jni_env->DeleteLocalRef( sdl_activity );
		return;
	}

	jni_env->CallStaticObjectMethod( gps_source_class_, enable_gps_source_method, sdl_activity );

	jthrowable exception= jni_env->ExceptionOccurred();
	if( exception != nullptr )
	{
		jni_env->ExceptionDescribe();
		jni_env->ExceptionClear();
	}
	else
		Log::Info( "Enable GPSSource" );

	jni_env->DeleteLocalRef( sdl_activity );
}

GPSService::~GPSService()
{
	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	if( gps_source_class_ != nullptr )
	{
		jmethodID disable_gps_source_method = jni_env->GetStaticMethodID( gps_source_class_, "Disable","()V" );
		if( disable_gps_source_method == nullptr )
		{
			Log::Warning( "Can not get \"Enable\" method" );
			return;
		}

		jni_env->CallStaticObjectMethod( gps_source_class_, disable_gps_source_method );
		jthrowable exception= jni_env->ExceptionOccurred();
		if( exception != nullptr )
		{
			jni_env->ExceptionDescribe();
			jni_env->ExceptionClear();
		}
		else
			Log::Info( "Disable GPSSource" );
	}
}

void GPSService::Update()
{
	if( gps_source_class_ == nullptr )
		return;
	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	GeoPoint new_position;
	new_position.x= jni_env->CallStaticDoubleMethod( gps_source_class_, get_longitude_method_ );
	new_position.y= jni_env->CallStaticDoubleMethod( gps_source_class_, get_latitude_method_  );

	if( new_position != gps_position_ )
	{
		gps_position_= new_position;
		Log::Info( "Coordinates: ", new_position.x, " ", new_position.y );
	}
}

} // namespace PanzerMaps
