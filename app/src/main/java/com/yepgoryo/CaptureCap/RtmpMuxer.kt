package com.yepgoryo.CaptureCap

import android.content.Context
import android.media.MediaCodec
import android.media.MediaFormat
import android.os.Handler
import android.util.Log
import java.io.IOException
import java.nio.ByteBuffer
import java.util.concurrent.TimeoutException

class RtmpMuxer(
    private val context: Context,
    private val rtmpUrl: String,
    private var audioSampleRate: Int,
    private var audioChannels: Int
) {

    private val TAG = "RtmpMuxer"

    private external fun nativeCreate(): Long
    private external fun nativeConnect(jContext: Long, url: String): Boolean
    private external fun nativeSetAVCConfig(jContext: Long, jSps: ByteArray, jPps: ByteArray)
    private external fun nativeSetAACConfig(jContext: Long, asc: ByteArray, sampleRate: Int, channelsCount: Int)
    private external fun nativeIsTimedOut(ctxPtr: Long): Boolean
    private external fun nativeWriteVideo(jContext: Long, data: ByteArray, timestampMs: Long, isKeyFrame: Boolean): Boolean
    private external fun nativeWriteAudio(jContext: Long, data: ByteArray, timestampMs: Long): Boolean
    private external fun nativeDisconnect(jContext: Long)
    private external fun nativeDestroy(jContext: Long)

    private var mNativeContext: Long = 0
    private var mStarted: Boolean = false
    private var mVideoTrackIndex: Int = -1
    private var mAudioTrackIndex: Int = -1

    private var videoSps: ByteArray? = null

    private var videoPps: ByteArray? = null

    private var audioAsc: ByteArray? = null
    private var frameHandler: Handler? = null
    private var frameRunnable: Runnable? = null

    init {
        System.loadLibrary("rtmp-jni")

        mNativeContext = nativeCreate()
        if (mNativeContext == 0L) {
            throw RuntimeException("Failed to allocate RTMP context")
        }
    }
    fun ByteArray.hex() = joinToString(" ") { "%02X".format(it) }
    fun setAVCConfig(sps: ByteArray, pps: ByteArray) {
        videoSps = sps
        videoPps = pps

        Log.d("RtmpMuxer", "videoSps: ${videoSps!!.hex()}")
        Log.d("RtmpMuxer", "videoPps: ${videoPps!!.hex()}")
    }

    fun setAACConfig(asc: ByteArray) {
        audioAsc = asc

        Log.d("RtmpMuxer", "audioAsc: ${audioAsc!!.hex()}")
    }

    fun addTrack(format: MediaFormat): Int {
        if (mStarted) {
            throw IllegalStateException("Cannot add tracks after start()")
        }

        val mime = format.getString(MediaFormat.KEY_MIME) ?: ""
        return when {
            mime.startsWith("video/avc") -> {
                if (mVideoTrackIndex >= 0) throw IllegalStateException("Video track already added")
                mVideoTrackIndex = 0
                Log.d(TAG, "Added video track (index=$mVideoTrackIndex)")
                mVideoTrackIndex
            }
            mime.startsWith("audio/mp4a-latm") -> {
                if (mAudioTrackIndex >= 0) throw IllegalStateException("Audio track already added")
                mAudioTrackIndex = 1
                Log.d(TAG, "Added audio track (index=$mAudioTrackIndex)")
                mAudioTrackIndex
            }
            else -> throw IllegalArgumentException("Unsupported MIME type: $mime")
        }
    }

    fun start() {
        if (mStarted) return
        if (mVideoTrackIndex < 0 && mAudioTrackIndex < 0) {
            throw IllegalStateException("No tracks added")
        }

        if (!nativeConnect(mNativeContext, rtmpUrl)) {
            nativeDestroy(mNativeContext)
            mNativeContext = 0
            throw IOException(context.getString(R.string.error_connecting_stream))
        }

        if (mVideoTrackIndex != -1) {
            nativeSetAVCConfig(mNativeContext, videoSps!!, videoPps!!)
        }

        if (mAudioTrackIndex != -1) {
            nativeSetAACConfig(mNativeContext, audioAsc!!, audioSampleRate, audioChannels)
        }

        mStarted = true

        Log.d(TAG, "RTMP muxer started")
    }

    private fun ByteBuffer.takeBytes(): ByteArray {
        val size = remaining()
        val bytes = ByteArray(size)
        get(bytes)
        return bytes
    }

    fun writeSampleData(trackIndex: Int, buffer: ByteBuffer, info: MediaCodec.BufferInfo) {
        if (!mStarted) throw IllegalStateException("Muxer not started")
        if (buffer.remaining() <= 0) return

        val data = buffer.takeBytes()
        val timestampMs = info.presentationTimeUs / 1000L

        if (nativeIsTimedOut(mNativeContext)) {
            throw TimeoutException(context.getString(R.string.error_connecting_timed_out))
        }

        when (trackIndex) {
            mVideoTrackIndex -> {
                val isKeyFrame = info.flags and MediaCodec.BUFFER_FLAG_KEY_FRAME != 0
                val isConfig = info.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0

                if (!isConfig) {
                    if (!nativeWriteVideo(mNativeContext, data, timestampMs, isKeyFrame)) {
                        Log.e(TAG, "Failed to write video frame (pts=$timestampMs ms)")
                    }
                }
            }
            mAudioTrackIndex -> {
                if (!nativeWriteAudio(mNativeContext, data, timestampMs)) {
                    Log.e(TAG, "Failed to write audio sample (pts=$timestampMs ms)")
                }
            }
            else -> throw IllegalArgumentException("Unknown track index: $trackIndex")
        }
    }

    fun stop() {
        if (!mStarted) return
        mStarted = false
        frameHandler?.removeCallbacks(frameRunnable!!)
        nativeDisconnect(mNativeContext)
    }

    fun release() {
        frameHandler?.removeCallbacks(frameRunnable!!)
        if (mNativeContext != 0L) {
            nativeDestroy(mNativeContext)
            mNativeContext = 0
        }
    }

    protected fun finalize() {
        try {
            if (mStarted) stop()
        } finally {
            release()
        }
    }

}