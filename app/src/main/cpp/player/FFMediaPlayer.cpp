/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include <render/video/NativeRender.h>
#include <render/audio/OpenSLRender.h>
#include <render/video/VideoGLRender.h>
#include <render/video/VRGLRender.h>
#include "FFMediaPlayer.h"

void
FFMediaPlayer::init(JNIEnv *jniEnv, jobject obj, char *url, int videoRenderType, jobject surface) {
    jniEnv->GetJavaVM(&javaVM);
    javaObj = jniEnv->NewGlobalRef(obj);

    videoDecoder = new VideoDecoder(url);
    audioDecoder = new AudioDecoder(url);

    if (videoRenderType == VIDEO_RENDER_OPENGL) {
        videoDecoder->setVideoRender(VideoGLRender::getInstance());
    } else if (videoRenderType == VIDEO_RENDER_ANWINDOW) {
        videoRender = new NativeRender(jniEnv, surface);
        videoDecoder->setVideoRender(videoRender);
    } else if (videoRenderType == VIDEO_RENDER_3D_VR) {
        videoDecoder->setVideoRender(VRGLRender::getInstance());
    }

    audioRender = new OpenSLRender();
    audioDecoder->SetAudioRender(audioRender);

    videoDecoder->setMessageCallback(this, PostMessage);
    audioDecoder->setMessageCallback(this, PostMessage);
}

void FFMediaPlayer::unInit() {
//    LOGCATE("FFMediaPlayer::UnInit");
    if (videoDecoder) {
        delete videoDecoder;
        videoDecoder = nullptr;
    }

    if (videoRender) {
        delete videoRender;
        videoRender = nullptr;
    }

    if (audioDecoder) {
        delete audioDecoder;
        audioDecoder = nullptr;
    }

    if (audioRender) {
        delete audioRender;
        audioRender = nullptr;
    }

    VideoGLRender::releaseInstance();

    bool isAttach = false;
    getJNIEnv(&isAttach)->DeleteGlobalRef(javaObj);
    if (isAttach)
        getJavaVM()->DetachCurrentThread();

}

void FFMediaPlayer::play() {
//    LOGCATE("FFMediaPlayer::Play");
    if (videoDecoder)
        videoDecoder->start();

    if (audioDecoder)
        audioDecoder->start();
}

void FFMediaPlayer::pause() {
//    LOGCATE("FFMediaPlayer::Pause");
    if (videoDecoder)
        videoDecoder->pause();

    if (audioDecoder)
        audioDecoder->pause();

}

void FFMediaPlayer::stop() {
//    LOGCATE("FFMediaPlayer::Stop");
    if (videoDecoder)
        videoDecoder->stop();

    if (audioDecoder)
        audioDecoder->stop();
}

void FFMediaPlayer::seekToPosition(float position) {
//    LOGCATE("FFMediaPlayer::SeekToPosition position=%f", position);
    if (videoDecoder)
        videoDecoder->seekToPosition(position);

    if (audioDecoder)
        audioDecoder->seekToPosition(position);

}

long FFMediaPlayer::getMediaParams(int paramType) {
//    LOGCATE("FFMediaPlayer::GetMediaParams paramType=%d", paramType);
    long value = 0;
//    paramType=0
    switch (paramType) {
        case MEDIA_PARAM_VIDEO_WIDTH:
            value = videoDecoder != nullptr ? videoDecoder->getVideoWidth() : 0;
            break;
        case MEDIA_PARAM_VIDEO_HEIGHT:
            value = videoDecoder != nullptr ? videoDecoder->getVideoHeight() : 0;
            break;
        case MEDIA_PARAM_VIDEO_DURATION:
            value = videoDecoder != nullptr ? videoDecoder->getDuration() : 0;
            break;
    }
    return value;
}

JNIEnv *FFMediaPlayer::getJNIEnv(bool *isAttach) {
    JNIEnv *env;
    int status;
    if (nullptr == javaVM) {
        LOGCATE("FFMediaPlayer::GetJNIEnv m_JavaVM == nullptr");
        return nullptr;
    }
    *isAttach = false;
    status = javaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
    if (status != JNI_OK) {
        status = javaVM->AttachCurrentThread(&env, nullptr);
        if (status != JNI_OK) {
            LOGCATE("FFMediaPlayer::GetJNIEnv failed to attach current thread");
            return nullptr;
        }
        *isAttach = true;
    }
    return env;
}

jobject FFMediaPlayer::getJavaObj() {
    return javaObj;
}

JavaVM *FFMediaPlayer::getJavaVM() {
    return javaVM;
}

void FFMediaPlayer::PostMessage(void *context, int msgType, float msgCode) {
    if (context != nullptr) {
        FFMediaPlayer *player = static_cast<FFMediaPlayer *>(context);
        bool isAttach = false;
        JNIEnv *env = player->getJNIEnv(&isAttach);
//        LOGCATE("FFMediaPlayer::PostMessage env=%p", env);
        if (env == nullptr)
            return;
        jobject javaObj = player->getJavaObj();
        jmethodID mid = env->GetMethodID(env->GetObjectClass(javaObj),
                                         JAVA_PLAYER_EVENT_CALLBACK_API_NAME, "(IF)V");
        env->CallVoidMethod(javaObj, mid, msgType, msgCode);
        if (isAttach)
            player->getJavaVM()->DetachCurrentThread();

    }
}


