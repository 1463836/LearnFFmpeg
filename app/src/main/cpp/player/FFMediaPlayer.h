/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#ifndef LEARNFFMPEG_FFMEDIAPLAYER_H
#define LEARNFFMPEG_FFMEDIAPLAYER_H

#include <MediaPlayer.h>

class FFMediaPlayer : public MediaPlayer {
public:
    FFMediaPlayer() {};

    virtual ~FFMediaPlayer() {};

    virtual void init(JNIEnv *jniEnv, jobject obj, char *url, int renderType, jobject surface);

    virtual void unInit();

    virtual void play();

    virtual void pause();

    virtual void stop();

    virtual void seekToPosition(float position);

    virtual long getMediaParams(int paramType);

private:
    virtual JNIEnv *getJNIEnv(bool *isAttach);

    virtual jobject getJavaObj();

    virtual JavaVM *getJavaVM();

    static void PostMessage(void *context, int msgType, float msgCode);

    VideoDecoder *videoDecoder = nullptr;
    AudioDecoder *audioDecoder = nullptr;

    VideoRender *videoRender = nullptr;
    AudioRender *audioRender = nullptr;
};


#endif //LEARNFFMPEG_FFMEDIAPLAYER_H
