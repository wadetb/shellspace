/************************************************************************************

Filename    :   MainActivity.java
Content     :   
Created     :   
Authors     :   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/
package oculus;

import android.os.Bundle;
import android.util.Log;

import com.oculusvr.vrlib.VrActivity;

// import org.acra.*;
// import org.acra.annotation.*;

import com.crittercism.app.Crittercism;
import com.crittercism.app.CrittercismConfig;

public class MainActivity extends VrActivity {
	public static final String TAG = "Shellspace";

	public static int a = 1;
	public static int b = 0;

	/** Load jni .so on initialization */
	static {
		Log.d(TAG, "LoadLibrary");
		System.loadLibrary("ovrapp");
	}

    public static native long nativeSetAppInterface( VrActivity act );

    @Override
    protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		appPtr = nativeSetAppInterface( this );
		CrittercismConfig config = new CrittercismConfig();
        config.setNdkCrashReportingEnabled(true);
		Crittercism.initialize(getApplicationContext(), "5543097b7365f84f7d3d707b", config);
		// int c = a / b;
    }
}
