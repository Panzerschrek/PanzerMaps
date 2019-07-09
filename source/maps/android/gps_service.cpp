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

	const jclass activity_class= jni_env->GetObjectClass( sdl_activity );
	if( activity_class == nullptr )
	{
		Log::Warning( "Can not get class of activity" );
		jni_env->DeleteLocalRef( sdl_activity );
		return;
	}

	jmethodID get_context_method = jni_env->GetStaticMethodID( activity_class, "getContext","()Landroid/content/Context;" );
	if( get_context_method == nullptr )
	{
		Log::Warning( "Can not get \"getContext\" method" );
		jni_env->DeleteLocalRef( sdl_activity );
		jni_env->DeleteLocalRef( activity_class );
		return;
	}

	context_= jni_env->CallStaticObjectMethod( activity_class, get_context_method );
	if( context_ == nullptr )
	{
		Log::Warning( "Context is null" );
		jni_env->DeleteLocalRef( sdl_activity );
		jni_env->DeleteLocalRef( activity_class );
		return;
	}
	Log::Info( "Get context" );
	jni_env->DeleteLocalRef( sdl_activity );
	jni_env->DeleteLocalRef( activity_class );

	gps_source_class_= jni_env->FindClass( "panzerschrek/panzermaps/app/GPSSource" );
	if( gps_source_class_ == nullptr )
	{
		Log::Warning( "Can not get \"GPSSource\" class" );
		return;
	}

	jmethodID enable_gps_source_method = jni_env->GetStaticMethodID( gps_source_class_, "Enable","(Landroid/content/Context;)V" );
	if( enable_gps_source_method == nullptr )
	{
		Log::Warning( "Can not get \"Enable\" method" );
		return;
	}

	jni_env->CallStaticObjectMethod( gps_source_class_, enable_gps_source_method, context_ );

	jthrowable exception= jni_env->ExceptionOccurred();
	if( exception != nullptr )
	{
		jni_env->ExceptionDescribe();
		jni_env->ExceptionClear();
	}
	else
		Log::Info( "Enable GPSSource" );
}

GPSService::~GPSService()
{
	JNIEnv* const jni_env= reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
	if( jni_env == nullptr )
	{
		Log::Warning( "Can not get JNIEnv" );
		return;
	}

	if( context_ != nullptr )
		jni_env->DeleteLocalRef( context_ );

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

GeoPoint GPSService::GetGeoPosition() const
{
	// TODO
	return GeoPoint{ 0.0, 0.0 };
}

} // namespace PanzerMaps
