package com.wzzn.myrenzheng;

import java.nio.ShortBuffer;

import android.R.integer;



public class Taimedia {
	public native boolean ffmpegInit();
    public native boolean ffmpegUninit();
    public native int  ffmpegGetavcodecversion();
    //初始化 记录功能
    public native int ffmpegRecorder(String filename,int imageWidth,int imageHeight,int frameRate,int sampleAudioRateInHz);
    //记录视频
    public native int recordVideo(byte[] data,int pwidth,int pheight,long ptime);
    //记录音频
    public native int recordAudio(byte[] data,long ptime);
    //关闭，并释放内存
    public native int release();
}
