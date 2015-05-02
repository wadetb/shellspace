package oculus;

import android.app.Application;
import android.util.Log;

// import org.acra.*;
// import org.acra.annotation.*;

// @ReportsCrashes(
//     httpMethod = org.acra.sender.HttpSender.Method.PUT,
//     reportType = org.acra.sender.HttpSender.Type.JSON,
//     formUri = "http://wadeb.com:5984/acra-shellspace/_design/acra-storage/_update/report",
//     formUriBasicAuthLogin = "shellspace",
//     formUriBasicAuthPassword = "yaSx-RSkh3-bi1f-ZAAEL"
// 	)
public class ShellspaceApp extends Application {
	public static final String TAG = "ShellspaceApp";
 
    @Override
    public void onCreate() {
		Log.d(TAG, "onCreate HIHIHI");
        super.onCreate();
        // ACRA.init(this); 
    }
}
