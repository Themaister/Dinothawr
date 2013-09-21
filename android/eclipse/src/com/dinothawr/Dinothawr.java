package com.dinothawr;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.NativeActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

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

	private boolean isTablet() {
		return (getResources().getConfiguration().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_LARGE;
	}
	
	private boolean isGamepadLet() {
		// If we're a "gamepadlet", don't display questions about overlay.
		final String[] devices = { "SHIELD", "GAMEMID_BT", "OUYA Console",
				"R800x", };

		for (String dev : devices)
			if (android.os.Build.MODEL.equals(dev))
				return true;
		return false;
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
		return 60.0f; // Doesn't really matter what the real refresh rate is. Audio is threaded.
	}

	private void runControlDialog() {
		final SharedPreferences prefs = PreferenceManager
				.getDefaultSharedPreferences(getBaseContext());
		final Context ctx = this;
		
		final AlertDialog alert = new AlertDialog.Builder(this)
				.setTitle("Thanks for playing Dinothawr!")
				.setMessage(
						"This is your first time playing Dinothawr.\n\nThis game is designed with gamepads in mind. If you only have a touchscreen available, you can use an on-screen overlay to control the game.\n\nWould you like to enable overlays now?\n\nYou can always change this option later under 'Options' menu.")
				.setNeutralButton("Controls",
						new DialogInterface.OnClickListener() {
							@Override
							public void onClick(DialogInterface dialog,
									int which) {
							}
						})
				.setPositiveButton("Yes",
						new DialogInterface.OnClickListener() {
							@Override
							public void onClick(DialogInterface dialog,
									int which) {
								SharedPreferences.Editor edit = prefs.edit();
								edit.putBoolean("overlay_enable", true);
								edit.commit();
							}
						})
				.setNegativeButton("No", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						SharedPreferences.Editor edit = prefs.edit();
						edit.putBoolean("overlay_enable", false);
						edit.commit();
					}
				}).create();
		alert.setOnShowListener(new DialogInterface.OnShowListener() {
			@Override
			public void onShow(DialogInterface dialog) {
				Button neutral = alert.getButton(AlertDialog.BUTTON_NEUTRAL);
				neutral.setOnClickListener(new View.OnClickListener() {
					@Override
					public void onClick(View v) {
						Intent intent = new Intent(ctx, Controls.class);
						startActivity(intent);
					}
				});
			}
		});
		alert.show();
	}
	
	private void runPopupControls() {
		SharedPreferences prefs = PreferenceManager
				.getDefaultSharedPreferences(getBaseContext());
		
		if (isGamepadLet()) {
			prefs.edit().putBoolean("overlay_enable", false).commit();
			return;
		}

		boolean demoMode = prefs.getBoolean("demo_mode", false);
		boolean firstTime = prefs.getBoolean("first_time", true);

		if (demoMode || firstTime)
			runControlDialog();
		
		SharedPreferences.Editor edit = prefs.edit();
		edit.putBoolean("first_time", false);
		edit.commit();
	}

	private void startRetroArch() {
		Intent intent = new Intent(this, NativeActivity.class);
		intent.putExtra("ROM", getApplicationInfo().dataDir + File.separator
				+ "dinothawr.game");
		intent.putExtra("LIBRETRO", getApplicationInfo().nativeLibraryDir
				+ "/libretro_dino.so");
		intent.putExtra("REFRESHRATE", Float.toString(getRefreshRate()));

		String dataDir = getApplicationInfo().dataDir + File.separator;
		String overlay = isTablet() ? "overlay_big.cfg" : "overlay.cfg";

		ConfigFile conf = new ConfigFile();
		conf.setInt("audio_out_rate", getOptimalSamplingRate());
		conf.setInt("input_back_behavior", 0);
		conf.setDouble("video_aspect_ratio", -1.0);
		conf.setBoolean("video_font_enable", false);
		File rarchConfig = new File(dataDir, "retroarch.cfg");
		
		SharedPreferences prefs = PreferenceManager
				.getDefaultSharedPreferences(getBaseContext());
		
		conf.setString("input_overlay", prefs.getBoolean("overlay_enable", true) ? dataDir + overlay : "");
		conf.setDouble("input_overlay_opacity", 0.5);
		
		boolean pixelPurist = prefs.getBoolean("pixel_purist", false);
		boolean audioEnable = prefs.getBoolean("enable_audio", true);
		if (pixelPurist) {
			conf.setBoolean("video_scale_integer", true);
			conf.setBoolean("video_smooth", false);
		} else {
			conf.setBoolean("video_scale_integer", false);
			conf.setBoolean("video_smooth", true);
		}
		
		conf.setBoolean("audio_enable", audioEnable);
		
		String shader = prefs.getString("shader", "");
		if (shader != null && !shader.isEmpty()) {
			conf.setBoolean("video_shader_enable", true);
			conf.setString("video_shader", getApplicationInfo().dataDir + File.separator + shader);
		} else
			conf.setBoolean("video_shader_enable", false);
		
		try {
			conf.write(rarchConfig);
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		if (prefs.getBoolean("demo_mode", false)) {
			File save = new File(getApplicationInfo().dataDir, "dinothawr.srm");
			if (save.exists()) {
				Log.i(TAG, "Demo mode, deleting save file.");
				save.delete();
			}
		}
		
		if (rarchConfig.exists())
			intent.putExtra("CONFIGFILE", rarchConfig.getAbsolutePath());
		else
			intent.putExtra("CONFIGFILE", "");

		String current_ime = Settings.Secure.getString(getContentResolver(),
				Settings.Secure.DEFAULT_INPUT_METHOD);
		intent.putExtra("IME", current_ime);
		
		
		conf = new ConfigFile();
		conf.setString("dino_timer", prefs.getBoolean("time_reference", false) ? "enabled" : "disabled");
		try {
			conf.write(new File(dataDir, ".retroarch-core-options.cfg"));
		} catch (IOException e) {
			e.printStackTrace();
		}

		startActivity(intent);
	}

	private void startNative() {
		startRetroArch();
	}

	private void setupCallbacks() {
		final Context ctx = this;
		
		Button button = (Button) findViewById(R.id.start_button);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				startNative();
			}
		});
		
		button = (Button) findViewById(R.id.controls_button);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intent = new Intent(ctx, Controls.class);
				startActivity(intent);
			}
		});
		
		button = (Button) findViewById(R.id.instruction_button);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intent = new Intent(ctx, Instructions.class);
				startActivity(intent);
			}
		});

		button = (Button) findViewById(R.id.quit_button);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});

		button = (Button) findViewById(R.id.options_button);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				Intent intent = new Intent(ctx, SettingsActivity.class);
				startActivity(intent);
			}
		});

		button = (Button) findViewById(R.id.credits_button);
		button.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				LayoutInflater inflater = getLayoutInflater();
				View dialog = inflater.inflate(R.layout.credits, null);

				TextView link = (TextView) dialog
						.findViewById(R.id.retroarch_link);
				link.setText(Html.fromHtml(getString(R.string.retroarch)));
				link.setMovementMethod(LinkMovementMethod.getInstance());

				link = (TextView) dialog.findViewById(R.id.libvorbis_link);
				link.setText(Html.fromHtml(getString(R.string.libvorbis)));
				link.setMovementMethod(LinkMovementMethod.getInstance());
				
				link = (TextView) dialog.findViewById(R.id.pugixml_link);
				link.setText(Html.fromHtml(getString(R.string.pugixml)));
				link.setMovementMethod(LinkMovementMethod.getInstance());

				AlertDialog.Builder builder = new AlertDialog.Builder(ctx)
						.setTitle("Credits").setView(dialog);
				builder.show();
			}
		});
	}

	private Thread extractThread = null;
	private boolean extracted = false;
	private boolean dialog = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.title);

		if (savedInstanceState != null) {
			extracted = savedInstanceState.getBoolean("EXTRACTED", false);
			dialog = savedInstanceState.getBoolean("DIALOG", false);
		}

		if (!extracted) {
			final Dialog dialog = new Dialog(this);
			dialog.setCancelable(false);
			dialog.setContentView(R.layout.assets);
			dialog.setTitle("Asset extraction");
			
			Log.i(TAG, "Starting asset extraction thread ...");
			extractThread = new Thread(new Runnable() {
				public void run() {
					try {
						extractAll();
					} catch (IOException e) {
						e.printStackTrace();
					}
					dialog.dismiss();
				}
			});
			extractThread.start();
			extracted = true;
			
			dialog.show();
		}

		setupCallbacks();
		Log.i(TAG, "Optimal sampling rate = " + getOptimalSamplingRate());
		
		if (!dialog)
			runPopupControls();
	}

	@TargetApi(17)
	private int getLowLatencyOptimalSamplingRate() {
		AudioManager manager = (AudioManager) getApplicationContext()
				.getSystemService(Context.AUDIO_SERVICE);
		return Integer.parseInt(manager
				.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
	}

	private int getOptimalSamplingRate() {
		int ret;
		if (android.os.Build.VERSION.SDK_INT >= 17)
			ret = getLowLatencyOptimalSamplingRate();
		else
			ret = AudioTrack
					.getNativeOutputSampleRate(AudioManager.STREAM_MUSIC);

		Log.i(TAG, "Using sampling rate: " + ret + " Hz");
		return ret;
	}

	@Override
	protected void onSaveInstanceState(Bundle bundle) {
		super.onSaveInstanceState(bundle);
		bundle.putBoolean("EXTRACTED", extracted);
		bundle.putBoolean("DIALOG", true);
	}
}
