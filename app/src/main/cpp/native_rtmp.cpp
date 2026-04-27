#include <jni.h>
#include <string>
#include <vector>
#include <cstdint>

#include <rtmp.h>
#include <android/log.h>

#define LOG_TAG "rtmp-native"
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)

std::vector<uint8_t> buildAVCDecoderRecord(const std::vector<uint8_t>& sps, const std::vector<uint8_t>& pps) {
    std::vector<uint8_t> config;

    config.push_back(1);

    config.push_back(sps[1]);
    config.push_back(sps[2]);
    config.push_back(sps[3]);

    config.push_back(0xFF);

    config.push_back(0xE0 | 1);

    config.push_back((uint8_t)(sps.size() >> 8));
    config.push_back((uint8_t)(sps.size() & 0xFF));

    config.insert(config.end(), sps.begin(), sps.end());

    config.push_back((uint8_t)1);

    config.push_back((uint8_t)(pps.size() >> 8));
    config.push_back((uint8_t)(pps.size() & 0xFF));

    config.insert(config.end(), pps.begin(), pps.end());

    return config;
}

void sendAVCConfig(RTMP *rtmp, const std::vector<uint8_t>& config) {
    RTMPPacket packet;
    RTMPPacket_Reset(&packet);
    RTMPPacket_Alloc(&packet, 5 + config.size());

    uint8_t* body = (uint8_t*)packet.m_body;
    body[0] = 0x17;
    body[1] = 0x00;
    body[2] = 0;
    body[3] = 0;
    body[4] = 0;

    memcpy(body + 5, config.data(), config.size());

    packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet.m_nBodySize = 5 + config.size();
    packet.m_nChannel = 0x04;
    packet.m_nTimeStamp = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;

    RTMP_SendPacket(rtmp, &packet, TRUE);
    RTMPPacket_Free(&packet);
}

bool sendAacConfig(RTMP *rtmp, const std::vector<uint8_t>& config, uint8_t audioFormat) {
    if (config.size() < 2) return false;

    std::vector<uint8_t> audioData;
    audioData.push_back(0x00);
    audioData.insert(audioData.end(), config.begin(), config.end());

    int bodyLen = 2 + config.size();

    RTMPPacket packet;
    RTMPPacket_Alloc(&packet, bodyLen);
    RTMPPacket_Reset(&packet);
    packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_nInfoField2 = rtmp->m_stream_id;

    packet.m_nBodySize = bodyLen;
    char* body = packet.m_body;
    body[0] = audioFormat;
    memcpy(body + 1, audioData.data(), audioData.size());

    bool success = RTMP_SendPacket(rtmp, &packet, TRUE);

    RTMPPacket_Free(&packet);
    return success;
}

bool annexbToAvcc(const std::vector<uint8_t>& annexb, std::vector<uint8_t>& avcc_out) {
    avcc_out.clear();

    if (annexb.empty()) return true;

    std::vector<std::pair<size_t, size_t>> nalu_ranges;

    size_t i = 0;
    const size_t len = annexb.size();

    while (i < len) {
        while (i < len && annexb[i] == 0) {
            ++i;
        }

        if (i >= len) break;

        size_t sc_pos = i;
        size_t sc_len = 0;

        bool found = false;
        if (i >= 2 && annexb[i-2] == 0 && annexb[i-1] == 0 && annexb[i] == 1) {
            sc_pos = i - 2;
            sc_len = 3;
            found = true;
        } else if (i >= 3 && annexb[i-3] == 0 && annexb[i-2] == 0 && annexb[i-1] == 0 && annexb[i] == 1) {
            sc_pos = i - 3;
            sc_len = 4;
            found = true;
        }

        if (!found) {
            ++i;
            continue;
        }

        size_t nalu_start = sc_pos + sc_len;
        if (nalu_start >= len) break;

        size_t j = nalu_start;
        while (j < len) {
            if (j + 2 < len && annexb[j] == 0 && annexb[j+1] == 0 && annexb[j+2] == 1) {
                break;
            }
            if (j + 3 < len && annexb[j] == 0 && annexb[j+1] == 0 && annexb[j+2] == 0 && annexb[j+3] == 1) {
                break;
            }
            ++j;
        }

        size_t nalu_end = j;
        size_t nalu_size = nalu_end - nalu_start;

        if (nalu_size > 0) {
            nalu_ranges.emplace_back(nalu_start, nalu_end);
        }

        i = j;
    }

    avcc_out.reserve(4 * nalu_ranges.size() + len);

    for (const auto& [start, end] : nalu_ranges) {
        size_t nalu_len = end - start;
        if (nalu_len > UINT32_MAX) {
            continue;
        }

        uint32_t len32 = static_cast<uint32_t>(nalu_len);
        avcc_out.push_back((len32 >> 24) & 0xFF);
        avcc_out.push_back((len32 >> 16) & 0xFF);
        avcc_out.push_back((len32 >> 8) & 0xFF);
        avcc_out.push_back(len32 & 0xFF);

        avcc_out.insert(avcc_out.end(), annexb.begin() + start, annexb.begin() + end);
    }

    return !avcc_out.empty();
}

extern "C" {

struct RtmpContext {
    RTMP *rtmp;
    std::vector<uint8_t> sps_video;
    std::vector<uint8_t> pps_video;
    std::vector<uint8_t> asc_audio;
    uint8_t format_audio;
};

JNIEXPORT void JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeSetAACConfig(JNIEnv *env, jclass clazz, jlong jContext, jbyteArray asc, jint sampleRate, jint channelsCount) {
    RtmpContext* ctx = (RtmpContext*)jContext;
    if (!ctx || !ctx->rtmp) return;

    static const uint8_t SOUND_FORMAT_AAC = 10;

    int bitDepth = 16;

    int soundRate;
    if (sampleRate <= 5500) soundRate = 0;
    else if (sampleRate <= 11000) soundRate = 1;
    else if (sampleRate <= 22000) soundRate = 2;
    else soundRate = 3;

    int soundSize = (bitDepth == 16) ? 1 : 0;
    int soundType = (channelsCount == 2) ? 1 : 0;

    ctx->format_audio = (SOUND_FORMAT_AAC << 4) | (soundRate << 2) | (soundSize << 1) | soundType;

    jsize ascLen = env->GetArrayLength(asc);
    jbyte* ascData = env->GetByteArrayElements(asc, nullptr);
    if (!ascData) return;

    if (ascLen < 2) {
        env->ReleaseByteArrayElements(asc, ascData, JNI_ABORT);
        LOGE("ASC is too short");
        return;
    }

    ctx->asc_audio.assign(ascData, ascData + ascLen);

    sendAacConfig(ctx->rtmp, ctx->asc_audio, ctx->format_audio);

    env->ReleaseByteArrayElements(asc, ascData, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeSetAVCConfig(JNIEnv *env, jclass clazz, jlong jContext, jbyteArray jSps, jbyteArray jPps) {
    RtmpContext* ctx = (RtmpContext*)jContext;
    if (!ctx || !ctx->rtmp) return;

    jsize spsLen = env->GetArrayLength(jSps);
    jbyte* spsData = env->GetByteArrayElements(jSps, nullptr);
    if (!spsData) return;

    if (spsLen < 5) {
        LOGE("SPS is too short");
        env->ReleaseByteArrayElements(jSps, spsData, JNI_ABORT);
        return;
    }

    ctx->sps_video.assign(spsData + 4, spsData + spsLen);

    jsize ppsLen = env->GetArrayLength(jPps);
    jbyte* ppsData = env->GetByteArrayElements(jPps, nullptr);
    if (!ppsData) return;

    ctx->pps_video.assign(ppsData + 4, ppsData + ppsLen);

    std::vector<uint8_t> config = buildAVCDecoderRecord(ctx->sps_video, ctx->pps_video);

    sendAVCConfig(ctx->rtmp, config);

    env->ReleaseByteArrayElements(jSps, spsData, JNI_ABORT);
    env->ReleaseByteArrayElements(jPps, ppsData, JNI_ABORT);
}

JNIEXPORT jlong JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeCreate(JNIEnv *env, jclass clazz) {
    RTMP *rtmp = RTMP_Alloc();
    if (!rtmp) return 0;

    RTMP_Init(rtmp);
    RTMP_SetBufferMS(rtmp, 5 * 1000);

    RtmpContext *ctx = new RtmpContext();
    ctx->rtmp = rtmp;

    return (jlong)ctx;
}

JNIEXPORT jboolean JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeConnect(JNIEnv *env, jclass clazz, jlong jContext, jstring jUrl) {
    RtmpContext *ctx = (RtmpContext *)jContext;
    if (!ctx || !ctx->rtmp) return JNI_FALSE;

    const char *url = env->GetStringUTFChars(jUrl, nullptr);
    if (!url) return JNI_FALSE;

    int ret = RTMP_SetupURL(ctx->rtmp, (char*)url);
    if (!ret) {
        LOGE("Failed setting up URL");
        env->ReleaseStringUTFChars(jUrl, url);
        return JNI_FALSE;
    }

    RTMP_EnableWrite(ctx->rtmp);

    ret = RTMP_Connect(ctx->rtmp, nullptr);
    if (!ret) {
        env->ReleaseStringUTFChars(jUrl, url);
        return JNI_FALSE;
    }

    ret = RTMP_ConnectStream(ctx->rtmp, 0);
    if (!ret) {
        LOGE("Could not connectStream");
        return JNI_FALSE;
    }

    env->ReleaseStringUTFChars(jUrl, url);
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeIsTimedOut(JNIEnv *env, jclass clazz, jlong jContext) {
    RtmpContext *ctx = (RtmpContext *)jContext;
    if (!ctx || !ctx->rtmp) return JNI_FALSE;

    return (!RTMP_IsConnected(ctx->rtmp) || RTMP_IsTimedout(ctx->rtmp));
}

JNIEXPORT jboolean JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeWriteVideo(JNIEnv *env, jclass clazz, jlong jContext, jbyteArray jData, jlong jTimestampMs, jboolean isKeyFrame) {
    RtmpContext *ctx = (RtmpContext *)jContext;
    if (!ctx || !ctx->rtmp) return JNI_FALSE;

    if (!RTMP_IsConnected(ctx->rtmp) || RTMP_IsTimedout(ctx->rtmp)) {
        LOGE("RTMP not connected yet — skipping packet");
        return JNI_FALSE;
    }

    jsize len = env->GetArrayLength(jData);
    jbyte *bytes = env->GetByteArrayElements(jData, nullptr);

    if (!bytes || len <= 0) {
        env->ReleaseByteArrayElements(jData, bytes, JNI_ABORT);
        return JNI_FALSE;
    }

    std::vector<uint8_t> annexb(len);
    std::memcpy(annexb.data(), bytes, len);

    std::vector<uint8_t> avcc;

    if (!annexbToAvcc(annexb, avcc)) {
        LOGE("Failed to convert Annex-B to AVCC");
        env->ReleaseByteArrayElements(jData, bytes, JNI_ABORT);
        return JNI_FALSE;
    }

    RTMPPacket packet = {};
    RTMPPacket_Alloc(&packet, 5 + avcc.size());
    RTMPPacket_Reset(&packet);

    uint8_t frameType = isKeyFrame ? 1 : 2;

    packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet.m_nChannel = 0x04;
    packet.m_nTimeStamp = (uint32_t)jTimestampMs;
    packet.m_hasAbsTimestamp = 1;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;

    char *body = packet.m_body;

    body[0] = (frameType << 4) | 0x07;
    body[1] = 1;
    body[2] = 0;
    body[3] = 0;
    body[4] = 0;
    memcpy(body + 5, avcc.data(), avcc.size());

    packet.m_nBodySize = 5 + avcc.size();

    int ret = RTMP_SendPacket(ctx->rtmp, &packet, TRUE);
    RTMPPacket_Free(&packet);

    env->ReleaseByteArrayElements(jData, bytes, JNI_ABORT);
    return ret ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeWriteAudio(JNIEnv *env, jclass clazz, jlong jContext, jbyteArray jData, jlong jTimestampMs) {
    RtmpContext* ctx = (RtmpContext*)jContext;
    if (!ctx || !ctx->rtmp) return JNI_FALSE;

    jsize len = env->GetArrayLength(jData);
    jbyte *data = env->GetByteArrayElements(jData, nullptr);
    if (!data) return JNI_FALSE;

    std::vector<uint8_t> frameData;
    frameData.push_back(0x01);
    frameData.insert(frameData.end(), data, data + len);

    int bodyLen = 2 + frameData.size();

    RTMPPacket packet = {};
    RTMPPacket_Alloc(&packet, bodyLen);
    RTMPPacket_Reset(&packet);

    packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet.m_nChannel = 0x05;
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;

    packet.m_nTimeStamp = (uint32_t)jTimestampMs;
    packet.m_hasAbsTimestamp = 1;
    packet.m_nBodySize = bodyLen;

    char* body = packet.m_body;

    body[0] = ctx->format_audio;
    memcpy(body + 1, frameData.data(), frameData.size());

    int ret = RTMP_SendPacket(ctx->rtmp, &packet, TRUE);
    RTMPPacket_Free(&packet);

    env->ReleaseByteArrayElements(jData, data, JNI_ABORT);
    return ret ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeDisconnect(JNIEnv *env, jclass clazz, jlong jContext) {
    RtmpContext *ctx = (RtmpContext *)jContext;
    if (ctx && ctx->rtmp) {
        RTMP_Close(ctx->rtmp);
    }
}

JNIEXPORT void JNICALL Java_com_yepgoryo_CaptureCap_RtmpMuxer_nativeDestroy(JNIEnv *env, jclass clazz, jlong jContext) {
    RtmpContext *ctx = (RtmpContext *)jContext;
    if (ctx) {
        if (ctx->rtmp) {
            RTMP_Free(ctx->rtmp);
        }
        delete ctx;
    }
}

}