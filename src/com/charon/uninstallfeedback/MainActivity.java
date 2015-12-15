package com.charon.uninstallfeedback;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;

public class MainActivity extends Activity {
	private static final String TAG = "@@@";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		String packageDir = "/data/data/" + getPackageName();
		initUninstallFeedback(packageDir, Build.VERSION.SDK_INT);
	}

	private native void initUninstallFeedback(String packagePath, int sdkVersion);

	static {
		System.loadLibrary("uninstall_feedback");
	}
}
