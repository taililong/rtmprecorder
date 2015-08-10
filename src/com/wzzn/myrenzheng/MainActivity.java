package com.wzzn.myrenzheng;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ShortBuffer;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.os.PowerManager;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;

public class MainActivity extends Activity implements OnClickListener {
	Taimedia taimedia;
	Taiplayer taiplayer;
	static final String LOG_TAG = "myrenzheng";
	private PowerManager.WakeLock mWakeLock;
	private String ffmpeg_link = "/mnt/sdcard/record2.flv";
	private String ffmpeg_get_link = "rtmp://123.56.85.167:1935/cert/cert01_ff";
	// /sdcard/aac001.aac rtmp://123.56.85.167:1935/cert/cert01_ff
	// rtmp://123.56.85.167:1935/xiangxiang/cert
	long startTime = 0;
	boolean recording = false;
	private int sampleAudioRateInHz = 44100;
	private int imageWidth = 320;
	private int imageHeight = 240;
	private int frameRate = 15;
	private Camera cameraDevice;
	private CameraView cameraView;
	private boolean isPreviewOn = false;
	final int RECORD_LENGTH = 0;
	long[] timestamps;
	ShortBuffer[] samples;
	int imagesIndex, samplesIndex;
	private int screenWidth, screenHeight;
	private Button startBtn;
	private AudioRecord audioRecord;
	private AudioRecordRunnable audioRecordRunnable;
	private Thread audioThread;
	private Thread playerThread;
	volatile boolean runAudioThread = true;
	private boolean playLoad = false;
	private boolean mediaLoad = false;
	static {
		System.loadLibrary("avcodec");
		System.loadLibrary("avformat");
		System.loadLibrary("avutil");
		System.loadLibrary("swscale");
		System.loadLibrary("swresample");
		System.loadLibrary("yuv");
		System.loadLibrary("Taimedia");
		System.loadLibrary("Taiplayer");

	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK,
				LOG_TAG);
		mWakeLock.acquire();

		initLayout();
	}

	private void initLayout() {
		Display display = ((WindowManager) getSystemService(Context.WINDOW_SERVICE))
				.getDefaultDisplay();
		screenWidth = display.getWidth();
		screenHeight = display.getHeight();
		LinearLayout.LayoutParams layoutParams = null;
		LayoutInflater myInflate = null;
		myInflate = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		LinearLayout mainLayout = new LinearLayout(this);
		mainLayout.setOrientation(LinearLayout.VERTICAL);
		setContentView(mainLayout);
		LinearLayout headLayout = (LinearLayout) myInflate.inflate(
				R.layout.rzhead, null);
		mainLayout.addView(headLayout);
		int cameraid = 0;
		final int cameraCount = Camera.getNumberOfCameras();
		Camera.CameraInfo info = new Camera.CameraInfo();
		for (int i = 0; i < cameraCount; i++) {
			Camera.getCameraInfo(i, info);
			if (Camera.CameraInfo.CAMERA_FACING_FRONT == info.facing) {
				cameraid = i;
			}
		}
		cameraDevice = Camera.open(cameraid);
		cameraDevice.setDisplayOrientation(90);
		Parameters params = cameraDevice.getParameters();

		// params.setPreviewSize(PreviewWidth, PreviewHeight);
		// params.setPictureSize(PreviewWidth, PreviewHeight);
		cameraDevice.setParameters(params);

		Log.i(LOG_TAG, "cameara open");
		cameraView = new CameraView(this, cameraDevice);
		layoutParams = new LinearLayout.LayoutParams(screenWidth,
				4 * screenWidth / 3);
		mainLayout.addView(cameraView, layoutParams);

		LinearLayout footLayout = (LinearLayout) myInflate.inflate(
				R.layout.rzfoot, null);
		mainLayout.addView(footLayout);

		startBtn = (Button) findViewById(R.id.startbtn);

		startBtn.setOnClickListener(this);

	}

	@Override
	public void onClick(View v) {
		if (!recording) {
			Log.v(LOG_TAG, "Start Button Pushed");
			startBtn.setText("连接中");
			startRecording();

		} else {
			if(playLoad == false || mediaLoad == false){
				return;
			}
			stopRecording();
			Log.v(LOG_TAG, "Stop Button Pushed");
			startBtn.setText("开始认证");

		}
	}

	public void startRecording() {
		String filename = ffmpeg_link;
		taimedia = new Taimedia();
		int ret = taimedia.ffmpegRecorder(filename, imageWidth, imageHeight,
				frameRate, sampleAudioRateInHz);
		if (ret < 0) {
			Log.v(LOG_TAG, "Stop Button Pushed");
			startBtn.setText("开始认证");
			return;
		}
		mediaLoad = true;
		Log.v(LOG_TAG, "Stop Button Pushed");
		startBtn.setText("结  束");
		 audioRecordRunnable = new AudioRecordRunnable();
		 audioThread = new Thread(audioRecordRunnable);
		 runAudioThread = true;
		 Log.d(LOG_TAG, "ffmpeg init: " + ret);
		 startTime = System.currentTimeMillis();
		recording = true;
		audioThread.start();

		playerThread = new Thread(new Runnable() {

			@Override
			public void run() {
				// TODO Auto-generated method stub
				taiplayer = new Taiplayer();
				int ret = taiplayer.ffmpegGrabber(ffmpeg_get_link);
				Log.v(LOG_TAG, "ret:" + ret);
				if (ret >= 0) {
					playLoad = true;
					taiplayer.playAudio();
				}
			}
		});
		//playerThread.start();
	}

	public void stopRecording() {
		runAudioThread = false;
		//taiplayer.stop();
		//taiplayer = null;
		taimedia.release();
		taimedia = null;
		try {
			if (audioThread != null) {
				audioThread.join();
			}
			if (playerThread != null) {
				playerThread.join();
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		audioRecordRunnable = null;
		audioThread = null;
		playerThread = null;
		playLoad= false;
		mediaLoad = false;
		if (recording) {
			recording = false;
			Log.v(LOG_TAG,
					"Finishing recording,  calling stop and release on recorder");

		}
	}

	class AudioRecordRunnable implements Runnable {

		@Override
		public void run() {
			android.os.Process
					.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

			// Audio
			int bufferSize;
			byte[] audioData;
			int bufferReadResult;

			bufferSize = AudioRecord
					.getMinBufferSize(sampleAudioRateInHz,
							AudioFormat.CHANNEL_IN_MONO,
							AudioFormat.ENCODING_PCM_16BIT);
			Log.d(LOG_TAG, "min bufferSize   :" + bufferSize);
			audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
					sampleAudioRateInHz, AudioFormat.CHANNEL_IN_MONO,
					AudioFormat.ENCODING_PCM_16BIT, bufferSize);
			// Log.v(LOG_TAG,"bufferSize : " + bufferSize);
			// audioData = ShortBuffer.allocate(bufferSize);
			bufferSize = 2048;
			Log.d(LOG_TAG, "audioRecord.startRecording( )");
			audioRecord.startRecording();
			audioData = new byte[bufferSize];
			/* ffmpeg_audio encoding loop */
			while (runAudioThread) {

				// Log.v(LOG_TAG,"recording? " + recording);
				bufferReadResult = audioRecord.read(audioData, 0, bufferSize);
				long t = System.currentTimeMillis();
				if (bufferReadResult > 0 && taimedia != null) {
					// Log.v(LOG_TAG, "timestamp"+t);
					synchronized (taimedia) {
						taimedia.recordAudio(audioData, t);
					}
				}
			}
			Log.v(LOG_TAG, "AudioThread Finished, release audioRecord");

			/* encoding finish, release recorder */
			if (audioRecord != null) {
				audioRecord.stop();
				audioRecord.release();
				audioRecord = null;
				Log.v(LOG_TAG, "audioRecord released");
			}
		}
	}

	class CameraView extends SurfaceView implements SurfaceHolder.Callback,
			PreviewCallback {

		private SurfaceHolder mHolder;
		private Camera mCamera;

		public CameraView(Context context, Camera camera) {
			super(context);
			Log.w("camera", "camera    view");
			mCamera = camera;
			mHolder = getHolder();
			mHolder.addCallback(CameraView.this);
			mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
			mCamera.setPreviewCallback(CameraView.this);
		}

		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			try {
				stopPreview();
				mCamera.setPreviewDisplay(holder);
			} catch (IOException exception) {
				mCamera.release();
				mCamera = null;
			}
		}

		public void surfaceChanged(SurfaceHolder holder, int format, int width,
				int height) {
			Log.v(LOG_TAG, "Setting   imageWidth: " + imageWidth
					+ " imageHeight:     " + imageHeight + " frameRate: "
					+ frameRate);
			Camera.Parameters camParams = mCamera.getParameters();
			int PreviewWidth = 360, PreviewHeight = 240;
			List<Size> pictureSizes = cameraDevice.getParameters()
					.getSupportedPictureSizes();
			Iterator<Camera.Size> itor = pictureSizes.iterator();
			while (itor.hasNext()) {
				Camera.Size cur = itor.next();
				if (cur.width >= 360 && cur.height >= 240) {
					PreviewWidth = cur.width;
					PreviewHeight = cur.height;
					Log.v(LOG_TAG, "支持的setPreviewSize width:" + PreviewWidth
							+ " PreviewHeight:" + PreviewHeight);
					// break;
				}
			}
			Log.v(LOG_TAG, "cameara width:  " + PreviewWidth + " heigth:"
					+ PreviewHeight);
			camParams.setPreviewSize(PreviewWidth, PreviewHeight);
			camParams.setPictureFormat(ImageFormat.NV21);
			// camParams.setPreviewFpsRange(frameRate-5, frameRate);
			Log.v(LOG_TAG,
					"Preview Framerate: " + camParams.getPreviewFrameRate()
							+ " format:" + camParams.getPreviewFormat());
			camParams.setPreviewFrameRate(frameRate);
			mCamera.setParameters(camParams);

			startPreview();
		}

		@Override
		public void surfaceDestroyed(SurfaceHolder holder) {
			try {
				mHolder.addCallback(null);
				mCamera.setPreviewCallback(null);
			} catch (RuntimeException e) {
				// The camera has probably just been released, ignore.
			}
		}

		public void startPreview() {
			if (!isPreviewOn && mCamera != null) {
				isPreviewOn = true;
				mCamera.startPreview();
			}
		}

		public void stopPreview() {
			if (isPreviewOn && mCamera != null) {
				isPreviewOn = false;
				mCamera.stopPreview();
			}
		}

		@Override
		public void onPreviewFrame(byte[] data, Camera camera) {
			// if (audioRecord == null
			// || audioRecord.getRecordingState() !=
			// AudioRecord.RECORDSTATE_RECORDING) {
			// startTime = System.currentTimeMillis();
			// return;
			// }
			if (data == null) {
				return;
			}
			long t = System.currentTimeMillis();
			if (taimedia != null && recording) {
				Camera.Parameters camParams = mCamera.getParameters();
				synchronized (taimedia) {
					taimedia.recordVideo(data,
							camParams.getPreviewSize().width,
							camParams.getPreviewSize().height, t);
				}
			}

		}
	}
}
