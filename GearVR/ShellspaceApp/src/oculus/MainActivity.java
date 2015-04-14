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

public class MainActivity extends VrActivity {
	public static final String TAG = "VrTemplate";

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
    }   
}
