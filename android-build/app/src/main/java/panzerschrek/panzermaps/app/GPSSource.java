package panzerschrek.panzermaps.app;

import android.app.*;
import android.content.*;
import android.location.*;
import android.util.Log;

public class GPSSource
{
	public static void Enable( Activity activity )
	{
		if( activity == null )
		{
			Log.w( TAG, "null activity" );
			return;
		}
		activity_= activity;
		activity.runOnUiThread(
			new Runnable()
			{
				public void run()
				{
					try // Catch all exceptions on UiThread. We do not want to ruin it.
					{
						Activity activity= activity_;
						if( activity == null )
						{
							return;
						}

						LocationManager location_manager= (LocationManager) activity.getSystemService( activity.LOCATION_SERVICE );
						if( location_manager == null )
						{
							Log.w( TAG, "can not get location manager" );
							return;
						}
						location_manager_= location_manager;
						Log.i( TAG, "get location manager" );

						String provider= LocationManager.GPS_PROVIDER;

						last_location_= location_manager.getLastKnownLocation( provider );
						Log.i( TAG, "get last known location" );

						LocationListener location_listener= new LocationListener()
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

						location_manager.requestLocationUpdates( provider, 2000, 5.0f, location_listener );
						location_listener_= location_listener;
						Log.i( TAG, "subscribe to updates" );
					}
					catch( Exception e )
					{
						Log.w( TAG, e.getMessage() );
					}
				}
			} );
	}

	public static void Disable()
	{
		Activity activity= activity_;
		if( activity == null )
		{
			return;
		}
		activity.runOnUiThread(
			new Runnable()
			{
				public void run()
				{
					try // Catch all exceptions on UiThread. We do not want to ruin it.
					{
						LocationManager location_manager= location_manager_;
						LocationListener location_listener= location_listener_;
						if( location_manager != null && location_listener != null )
						{
							location_manager.removeUpdates(location_listener);
						}
						activity_= null;
						location_manager_= null;
						location_listener_= null;
						last_location_= null;
					}
					catch( Exception e )
					{
						Log.w( TAG, e.getMessage() );
					}
				}
			} );
	}

	public static double GetLongitude()
	{
		Location location= last_location_;
		if( location == null )
			return c_invalid_coordinate;
		return location.getLongitude();
	}

	public static double GetLatitude()
	{
		Location location= last_location_;
		if( location == null )
			return c_invalid_coordinate;
		return location.getLatitude();
	}

	private static final String TAG = "PanzerMaps";
	public static final double c_invalid_coordinate= 1000.0;

	// All variables are volatile, because we can access them from different threads.
	static private volatile Activity activity_;
	static private volatile LocationManager location_manager_;
	static private volatile LocationListener location_listener_;
	static private volatile Location last_location_;
}
