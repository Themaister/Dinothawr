package com.dinothawr;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.os.Bundle;
import android.provider.Settings;
import android.app.Activity;
import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

public class Dinothawr extends Activity {
	static private final String TAG = "Dinothawr";

	private byte[] loadAsset(String asset) {
		try {
			String path = asset;
			InputStream stream = getAssets().open(path);
			int len = stream.available();
			byte[] buf = new byte[len];
			stream.read(buf, 0, len);
			return buf;
		} catch (IOException e) {
			return null;
		}
	}

	private void extractAssets(String folder, String cache) throws IOException {
		String[] assets = getAssets().list(folder);
		Log.i(TAG, "Found " + assets.length + " in folder: " + folder);
		for (String file : assets) {
			Log.i(TAG, "Found file: " + file);
			String asset_path = folder
					+ (folder.equals("") ? "" : File.separator) + file;
			byte[] buf = loadAsset(asset_path);
			if (buf == null)
				continue;

			int len = buf.length;

			String out_path = cache + File.separator + file;
			Log.i(TAG, asset_path + " => " + out_path);

			try {
				BufferedOutputStream writer = new BufferedOutputStream(
						new FileOutputStream(new File(out_path)));

				writer.write(buf, 0, len);
				writer.flush();
				writer.close();
			} catch (IOException e) {
				Log.i(TAG, "Failed to write to: " + out_path);
				throw e;
			}
		}
	}

	private void extractAll() throws IOException {
		String saves_folder = getFilesDir().getAbsolutePath();
		Log.i(TAG, "Saves folder: " + saves_folder);

		String cache = getCacheDir().getAbsolutePath();
		try {
			String[] dirs = new String[] { "", "assets", "assets/sfx",
					"assets/sfx", "assets/bg" };
			for (String dir : dirs) {
				File dirfile = new File(cache + File.separator + dir);
				if (!dirfile.exists()) {
					if (!dirfile.mkdirs())
						throw new IOException();
				}

				extractAssets(dir, cache + File.separator + dir);
			}
		} catch (IOException e) {
			Log.i(TAG, "Exception!");
		}
	}

	private float getRefreshRate() {
		WindowManager wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();
		return display.getRefreshRate();
	}

	private void startRetroArch() {
		Intent intent = new Intent(this, NativeActivity.class);
		intent.putExtra("ROM", getCacheDir() + File.separator
				+ "dinothawr.game");
		intent.putExtra("LIBRETRO", getApplicationInfo().nativeLibraryDir
				+ "/libretro_dino.so");
		intent.putExtra("REFRESHRATE", Float.toString(getRefreshRate()));
		
		String conf_path = getCacheDir() + File.separator + "retroarch.cfg";
		if (new File(conf_path).exists())
			intent.putExtra("CONFIGFILE", conf_path);
		else
			intent.putExtra("CONFIGFILE", "");
		
		String current_ime = Settings.Secure.getString(getContentResolver(),
				Settings.Secure.DEFAULT_INPUT_METHOD);
		intent.putExtra("IME", current_ime);

		startActivity(intent);
		finish();
	}

	private void startNative() {
		try {
			extractAll();
			startRetroArch();
		} catch (IOException e) {
			Log.e(TAG, "Failed to start Dinothawr! :(");
			e.printStackTrace();
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		startNative();
	}
}
