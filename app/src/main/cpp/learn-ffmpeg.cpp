/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include <cstdio>
#include <cstring>
#include <PlayerWrapper.h>
#include <render/video/VideoGLRender.h>
#include <render/video/VRGLRender.h>
#include <render/audio/OpenSLRender.h>
#include <MediaRecorderContext.h>
#include "util/LogUtil.h"
#include "jni.h"
#include "ASanTestCase.h"

extern "C" {
#include <libavcodec/version.h>
#include <libavcodec/avcodec.h>
#include <libavformat/version.h>
#include <libavutil/version.h>
#include <libavfilter/version.h>
#include <libswresample/version.h>
#include <libswscale/version.h>
};

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_byteflow_learnffmpeg_media_FFMediaPlayer
 * Method:    native_GetFFmpegVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1GetFFmpegVersion
        (JNIEnv *env, jclass cls) {
    char strBuffer[1024 * 4] = {0};
    strcat(strBuffer, "libavcodec : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVCODEC_VERSION));
    strcat(strBuffer, "\nlibavformat : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVFORMAT_VERSION));
    strcat(strBuffer, "\nlibavutil : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVUTIL_VERSION));
    strcat(strBuffer, "\nlibavfilter : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVFILTER_VERSION));
    strcat(strBuffer, "\nlibswresample : ");
    strcat(strBuffer, AV_STRINGIFY(LIBSWRESAMPLE_VERSION));
    strcat(strBuffer, "\nlibswscale : ");
    strcat(strBuffer, AV_STRINGIFY(LIBSWSCALE_VERSION));
    strcat(strBuffer, "\navcodec_configure : \n");
    strcat(strBuffer, avcodec_configuration());
    strcat(strBuffer, "\navcodec_license : ");
    strcat(strBuffer, avcodec_license());
//    LOGCATE("GetFFmpegVersion\n%s", strBuffer);

    //ASanTestCase::MainTest();

    return env->NewStringUTF(strBuffer);
}

/*
 * Class:     com_byteflow_learnffmpeg_media_FFMediaPlayer
 * Method:    native_Init
 * Signature: (JLjava/lang/String;Ljava/lang/Object;)J
 */
JNIEXPORT jlong JNICALL Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1Init
        (JNIEnv *env, jobject obj, jstring jurl, int playerType, jint renderType, jobject surface) {
    const char *url = env->GetStringUTFChars(jurl, nullptr);
    PlayerWrapper *player = new PlayerWrapper();
    player->init(env, obj, const_cast<char *>(url), playerType, renderType, surface);
    env->ReleaseStringUTFChars(jurl, url);
    return reinterpret_cast<jlong>(player);
}

/*
 * Class:     com_byteflow_learnffmpeg_media_FFMediaPlayer
 * Method:    native_Play
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1Play
        (JNIEnv *env, jobject obj, jlong player_handle) {
    if (player_handle != 0) {
        PlayerWrapper *pPlayerWrapper = reinterpret_cast<PlayerWrapper *>(player_handle);
        pPlayerWrapper->play();
    }

}

JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1SeekToPosition(JNIEnv *env, jobject thiz,
                                                                         jlong player_handle,
                                                                         jfloat position) {
    if (player_handle != 0) {
        PlayerWrapper *ffMediaPlayer = reinterpret_cast<PlayerWrapper *>(player_handle);
        ffMediaPlayer->seekToPosition(position);
    }
}

JNIEXPORT jlong JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1GetMediaParams(JNIEnv *env, jobject thiz,
                                                                         jlong player_handle,
                                                                         jint param_type) {
    long value = 0;
    if (player_handle != 0) {
        PlayerWrapper *ffMediaPlayer = reinterpret_cast<PlayerWrapper *>(player_handle);
        value = ffMediaPlayer->getMediaParams(param_type);
    }
    return value;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1SetMediaParams(JNIEnv *env, jobject thiz,
                                                                         jlong player_handle,
                                                                         jint param_type,
                                                                         jobject param) {
    if (player_handle != 0) {
        PlayerWrapper *ffMediaPlayer = reinterpret_cast<PlayerWrapper *>(player_handle);
        ffMediaPlayer->setMediaParams(param_type, param);
    }
}

/*
 * Class:     com_byteflow_learnffmpeg_media_FFMediaPlayer
 * Method:    native_Pause
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1Pause
        (JNIEnv *env, jobject obj, jlong player_handle) {
    if (player_handle != 0) {
        PlayerWrapper *ffMediaPlayer = reinterpret_cast<PlayerWrapper *>(player_handle);
        ffMediaPlayer->pause();
    }
}

/*
 * Class:     com_byteflow_learnffmpeg_media_FFMediaPlayer
 * Method:    native_Stop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1Stop
        (JNIEnv *env, jobject obj, jlong player_handle) {
    if (player_handle != 0) {
        PlayerWrapper *ffMediaPlayer = reinterpret_cast<PlayerWrapper *>(player_handle);
        ffMediaPlayer->stop();
    }
}

/*
 * Class:     com_byteflow_learnffmpeg_media_FFMediaPlayer
 * Method:    native_UnInit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1UnInit
        (JNIEnv *env, jobject obj, jlong player_handle) {
    if (player_handle != 0) {
        PlayerWrapper *ffMediaPlayer = reinterpret_cast<PlayerWrapper *>(player_handle);
        ffMediaPlayer->unInit();
        delete ffMediaPlayer;
    }
}

JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1OnSurfaceCreated(JNIEnv *env,
                                                                           jclass clazz,
                                                                           jint render_type) {
    switch (render_type) {
        case VIDEO_GL_RENDER:
            VideoGLRender::getInstance()->onSurfaceCreated();
            break;
        case AUDIO_GL_RENDER:
            AudioGLRender::getInstance()->onSurfaceCreated();
            break;
        case VR_3D_GL_RENDER:
            VRGLRender::getInstance()->onSurfaceCreated();
            break;
        default:
            break;
    }
}

JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1OnSurfaceChanged(JNIEnv *env,
                                                                           jclass clazz,
                                                                           jint render_type,
                                                                           jint width,
                                                                           jint height) {
    switch (render_type) {
        case VIDEO_GL_RENDER:
            VideoGLRender::getInstance()->onSurfaceChanged(width, height);
            break;
        case AUDIO_GL_RENDER:
            AudioGLRender::getInstance()->onSurfaceChanged(width, height);
            break;
        case VR_3D_GL_RENDER:
            VRGLRender::getInstance()->onSurfaceChanged(width, height);
            break;
        default:
            break;
    }
}

JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1OnDrawFrame(JNIEnv *env, jclass clazz,
                                                                      jint render_type) {
    switch (render_type) {
        case VIDEO_GL_RENDER:
            VideoGLRender::getInstance()->onDrawFrame();
            break;
        case AUDIO_GL_RENDER:
            AudioGLRender::getInstance()->onDrawFrame();
            break;
        case VR_3D_GL_RENDER:
            VRGLRender::getInstance()->onDrawFrame();
            break;
        default:
            break;
    }
}

JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1SetGesture(JNIEnv *env, jclass clazz,
                                                                     jint render_type,
                                                                     jfloat x_rotate_angle,
                                                                     jfloat y_rotate_angle,
                                                                     jfloat scale) {
    switch (render_type) {
        case VIDEO_GL_RENDER:
            VideoGLRender::getInstance()->updateMVPMatrix(x_rotate_angle, y_rotate_angle, scale,
                                                          scale);
            break;
        case AUDIO_GL_RENDER:
            AudioGLRender::getInstance()->updateMVPMatrix(x_rotate_angle, y_rotate_angle, scale,
                                                          scale);
            break;
        case VR_3D_GL_RENDER:
            VRGLRender::getInstance()->updateMVPMatrix(x_rotate_angle, y_rotate_angle, scale,
                                                       scale);
            break;
        default:
            break;
    }
}

JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_FFMediaPlayer_native_1SetTouchLoc(JNIEnv *env, jclass clazz,
                                                                      jint render_type,
                                                                      jfloat touch_x,
                                                                      jfloat touch_y) {
    switch (render_type) {
        case VIDEO_GL_RENDER:
            VideoGLRender::getInstance()->setTouchLoc(touch_x, touch_y);
            break;
        case AUDIO_GL_RENDER:
            AudioGLRender::getInstance()->setTouchLoc(touch_x, touch_y);
            break;
        case VR_3D_GL_RENDER:
            VRGLRender::getInstance()->setTouchLoc(touch_x, touch_y);
            break;
        default:
            break;
    }
}

#ifdef __cplusplus
}
#endif

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1CreateContext(JNIEnv *env,
                                                                               jobject thiz) {
    MediaRecorderContext::createContext(env, thiz);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1DestroyContext(JNIEnv *env,
                                                                                jobject thiz) {
    MediaRecorderContext::deleteContext(env, thiz);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1Init(JNIEnv *env, jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) return pContext->init();
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1UnInit(JNIEnv *env, jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) return pContext->unInit();
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1StartRecord(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jint recorder_type,
                                                                             jstring out_url,
                                                                             jint frame_width,
                                                                             jint frame_height,
                                                                             jlong video_bit_rate,
                                                                             jint fps) {
    const char *url = env->GetStringUTFChars(out_url, nullptr);
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    env->ReleaseStringUTFChars(out_url, url);
    if (pContext)
        return pContext->startRecord(recorder_type, url, frame_width, frame_height, video_bit_rate,
                                     fps);
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1OnAudioData(JNIEnv *env,
                                                                             jobject thiz,
                                                                             jbyteArray data,
                                                                             jint size) {
    int len = env->GetArrayLength(data);
    unsigned char *buf = new unsigned char[len];
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte *>(buf));
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->onAudioData(buf, len);
    delete[] buf;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1StopRecord(JNIEnv *env,
                                                                            jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) return pContext->stopRecord();
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1OnPreviewFrame(JNIEnv *env,
                                                                                jobject thiz,
                                                                                jint format,
                                                                                jbyteArray data,
                                                                                jint width,
                                                                                jint height) {
    int len = env->GetArrayLength(data);
    unsigned char *buf = new unsigned char[len];
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte *>(buf));
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->previewFrame(format, buf, width, height);
    delete[] buf;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1SetTransformMatrix(JNIEnv *env,
                                                                                    jobject thiz,
                                                                                    jfloat translate_x,
                                                                                    jfloat translate_y,
                                                                                    jfloat scale_x,
                                                                                    jfloat scale_y,
                                                                                    jint degree,
                                                                                    jint mirror) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext)
        pContext->setTransformMatrix(translate_x, translate_y, scale_x, scale_y, degree, mirror);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1OnSurfaceCreated(JNIEnv *env,
                                                                                  jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->onSurfaceCreated();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1OnSurfaceChanged(JNIEnv *env,
                                                                                  jobject thiz,
                                                                                  jint width,
                                                                                  jint height) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->onSurfaceChanged(width, height);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1OnDrawFrame(JNIEnv *env,
                                                                             jobject thiz) {
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->onDrawFrame();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1SetFilterData(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jint index,
                                                                               jint format,
                                                                               jint width,
                                                                               jint height,
                                                                               jbyteArray bytes) {
    int len = env->GetArrayLength(bytes);
    uint8_t *buf = new uint8_t[len];
    env->GetByteArrayRegion(bytes, 0, len, reinterpret_cast<jbyte *>(buf));
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->setLUTImage(index, format, width, height, buf);
    delete[] buf;
    env->DeleteLocalRef(bytes);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_byteflow_learnffmpeg_media_MediaRecorderContext_native_1SetFragShader(JNIEnv *env,
                                                                               jobject thiz,
                                                                               jint index,
                                                                               jstring str) {
    int length = env->GetStringUTFLength(str);
    const char *cStr = env->GetStringUTFChars(str, JNI_FALSE);
    char *buf = static_cast<char *>(malloc(length + 1));
    memcpy(buf, cStr, length + 1);
    MediaRecorderContext *pContext = MediaRecorderContext::getContext(env, thiz);
    if (pContext) pContext->setFragShader(index, buf, length + 1);
    free(buf);
    env->ReleaseStringUTFChars(str, cStr);
}