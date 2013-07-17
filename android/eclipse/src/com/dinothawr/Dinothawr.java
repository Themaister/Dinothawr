package com.dinothawr;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.provider.Settings;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.View;
import android.widget.Button;

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

		String cache = getApplicationInfo().dataDir;
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
		/*
		WindowManager wm = (WindowManager) getSystemService(Context.WINDOW_SERVICE);
		Display display = wm.getDefaultDisplay();
		return display.getRefreshRate();
		*/
		return 60.0f;
	}

	private void startRetroArch() {
		Intent intent = new Intent(this, NativeActivity.class);
		intent.putExtra("ROM", getApplicationInfo().dataDir + File.separator
				+ "dinothawr.game");
		intent.putExtra("LIBRETRO", getApplicationInfo().nativeLibraryDir
				+ "/libretro_dino.so");
		intent.putExtra("REFRESHRATE", Float.toString(getRefreshRate()));
		
		String dataDir = getApplicationInfo().dataDir + File.separator;
		
		ConfigFile conf = new ConfigFile();
		conf.setInt("audio_out_rate", getOptimalSamplingRate());
		conf.setString("input_overlay", dataDir + "overlay.cfg");
		conf.setInt("input_back_behavior", 0);
		conf.setDouble("video_aspect_ratio", -1.0);
		conf.setBoolean("video_font_enable", false);
		File rarchConfig = new File(dataDir, "retroarch.cfg");
		try {
			conf.write(rarchConfig);
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		if (rarchConfig.exists())
			intent.putExtra("CONFIGFILE", rarchConfig.getAbsolutePath());
		else
			intent.putExtra("CONFIGFILE", "");
		
		String current_ime = Settings.Secure.getString(getContentResolver(),
				Settings.Secure.DEFAULT_INPUT_METHOD);
		intent.putExtra("IME", current_ime);

		startActivity(intent);
	}

	private void startNative() {
		try {
			if (extractThread != null)
				extractThread.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		extractThread = null;
		startRetroArch();
	}
	
	private Thread extractThread = null;
	private boolean extracted = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.title);
		
		if (savedInstanceState != null)
			extracted = savedInstanceState.getBoolean("EXTRACTED", false);
		
		if (!extracted) {
			Log.i(TAG, "Starting asset extraction thread ...");
			extractThread = new Thread(new Runnable() {
				public void run() {
					try {
						extractAll();
					} catch (IOException e) {
						e.printStackTrace();
					}
				}
			});
			extractThread.start();
			extracted = true;
		}
		
		Button button = (Button)findViewById(R.id.button1);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				startNative();
			}
		});
		
		Log.i(TAG, "Optimal sampling rate = " + getOptimalSamplingRate());
	}
	
	@TargetApi(android.os.Build.VERSION_CODES.JELLY_BEAN_MR1)
	private int getLowLatencyOptimalSamplingRate() {
		AudioManager manager = (AudioManager)getApplicationContext().getSystemService(Context.AUDIO_SERVICE);
		return Integer.parseInt(manager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
	}

	private int getOptimalSamplingRate() {
		int ret;
		if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN_MR1)
			ret = getLowLatencyOptimalSamplingRate();
		else
			ret = AudioTrack.getNativeOutputSampleRate(AudioManager.STREAM_MUSIC);
		
		Log.i(TAG, "Using sampling rate: " + ret + " Hz");
		return ret;
	}
	
	@Override
	protected void onSaveInstanceState(Bundle bundle) {
		super.onSaveInstanceState(bundle);
		bundle.putBoolean("EXTRACTED", extracted);
	}
}
