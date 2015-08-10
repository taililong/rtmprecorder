package com.wzzn.myrenzheng;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

public class Taiplayer {
	AudioDriver _track;

	// 初始化 记录功能
	public native int ffmpegGrabber(String filename);

	private native byte[] getAudio();

	private native int release();
	
	public int stop(){
		try {
			_track.release();
			_track = null;
			release();
		} catch (Exception e) {
			e.printStackTrace();
		}
		
		
		return 0;
	}

	// 获取素材 并播放
	public int playAudio() {
		_track = new AudioDriver(44100);

		Log.v(MainActivity.LOG_TAG, "pre get audio");
		new Thread(new Runnable() {

			@Override
			public void run() {
				byte[] audioData = new byte[8192];
				while (true) {
					if(_track == null){
						break;
					}
					audioData = getAudio();
					try{
						_track.writeSample(audioData);
					}catch(Exception e){
						e.printStackTrace();
					}
				}
			}
		}).start();

		Log.v(MainActivity.LOG_TAG, "no !!!!!!get audio");

		return 0;
	}

	public class AudioDriver {
		AudioTrack track;
		short[] buffer = new short[2048];

		public AudioDriver(int sampleRate) {
			// Log.v(MainActivity.TAG, "sampleRate:" + sampleRate + " channels:"
			// + channels);
			int minSize = AudioTrack.getMinBufferSize(sampleRate,
					AudioFormat.CHANNEL_OUT_STEREO,
					AudioFormat.ENCODING_PCM_16BIT);
			Log.v(MainActivity.LOG_TAG, "minSize:" + minSize);
			track = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
					AudioFormat.CHANNEL_OUT_STEREO,
					AudioFormat.ENCODING_PCM_16BIT, minSize,
					AudioTrack.MODE_STREAM);
			track.play();
		}
		public void writeSample(byte[] samples) {
			if(track != null){
				try {
					track.write(samples, 0, samples.length);
				} catch (Exception e) {
					e.printStackTrace();
				}
				
			}
		}
		public void writeSamples(float[] samples) {
			Log.v(MainActivity.LOG_TAG, " !!!!!!get audio" + samples.length);
			fillBuffer(samples);

			track.write(buffer, 0, samples.length);
		}

		private void fillBuffer(float[] samples) {
			// if (buffer.length < samples.length) {
			buffer = new short[samples.length];
			// }
			for (int i = 0; i < samples.length; i++) {
				buffer[i] = (short) (samples[i] * Short.MAX_VALUE);
			}
		}

		public void release() {
			track.stop();
			track.release();
			track = null;
		}
	}
}
