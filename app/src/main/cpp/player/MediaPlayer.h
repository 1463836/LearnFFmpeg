/**
 *
 * Created by 公众号：字节流动 on 2021/12/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_MEDIAPLAYER_H
#define LEARNFFMPEG_MEDIAPLAYER_H

#include <jni.h>
#include <decoder/VideoDecoder.h>
#include <decoder/AudioDecoder.h>
#include <render/audio/AudioRender.h>

#define JAVA_PLAYER_EVENT_CALLBACK_API_NAME "playerEventCallback"

#define MEDIA_PARAM_VIDEO_WIDTH         0x0001
#define MEDIA_PARAM_VIDEO_HEIGHT        0x0002
#define MEDIA_PARAM_VIDEO_DURATION      0x0003

#define MEDIA_PARAM_ASSET_MANAGER       0x0020


class MediaPlayer {
public:
    MediaPlayer() {};

    virtual ~MediaPlayer() {};

    virtual void init(JNIEnv *jniEnv, jobject obj, char *url, int renderType, jobject surface) = 0;

    virtual void unInit() = 0;

    virtual void play() = 0;

    virtual void pause() = 0;

    virtual void stop() = 0;

    virtual void seekToPosition(float position) = 0;

    virtual long getMediaParams(int paramType) = 0;

    virtual void setMediaParams(int paramType, jobject obj) {}

    virtual JNIEnv *getJNIEnv(bool *isAttach) = 0;

    virtual jobject getJavaObj() = 0;

    virtual JavaVM *getJavaVM() = 0;

    JavaVM *javaVM = nullptr;
    jobject javaObj = nullptr;
};

#endif //LEARNFFMPEG_MEDIAPLAYER_H
