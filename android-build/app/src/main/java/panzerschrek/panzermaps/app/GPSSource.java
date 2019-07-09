package panzerschrek.panzermaps.app;

import android.content.*;
import android.location.*;
import android.util.Log;

public class GPSSource
{
	public static void Enable( Context context )
	{
		location_manager_= (android.location.LocationManager) context.getSystemService( context.LOCATION_SERVICE );
		if( location_manager_ == null )
		{
			Log.w(TAG, "can not get location manager" );
			return;
		}
		Log.i(TAG, "get location manager" );

		location_listener_= new LocationListener()
		{
			@Override
			public void onLocationChanged(android.location.Location location) { last_location_= location; }

			@Override
			public void onStatusChanged(String provider, int status, android.os.Bundle extras) {}

			@Override
			public void onProviderEnabled(String provider) {}

			@Override
			public void onProviderDisabled(String provider) {}
		};

		location_manager_.requestLocationUpdates( LocationManager.GPS_PROVIDER, 2000, 5.0f, location_listener_ );
		Log.i(TAG, "subscribe to updates" );
	}

	public static void Disable()
	{
		if( location_manager_ != null )
		{
			location_manager_.removeUpdates(location_listener_);
		}
		location_listener_= null;
		location_listener_= null;
	}

	public static double GetLongitude()
	{
		if( last_location_ == null )
			return 1000.0;
		return last_location_.getLongitude();
	}

	public static double GetLatitude()
	{
		if( last_location_ == null )
			return 1000.0;
		return last_location_.getLatitude();
	}

	private static final String TAG = "PanzerMaps";

	static private android.location.LocationManager location_manager_;
	static private android.location.Location last_location_;
	static private LocationListener location_listener_;
}
