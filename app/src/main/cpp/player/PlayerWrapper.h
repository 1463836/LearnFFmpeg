/**
 *
 * Created by 公众号：字节流动 on 2021/12/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_PLAYERWRAPPER_H
#define LEARNFFMPEG_PLAYERWRAPPER_H

#include <jni.h>
#include <HWCodecPlayer.h>
#include <FFMediaPlayer.h>

static const int FFMEDIA_PLAYER = 0;
static const int HWCODEC_PLAYER = 1;

class PlayerWrapper {
public:
    PlayerWrapper() {};

    virtual ~PlayerWrapper() {};

    void
    init(JNIEnv *jniEnv, jobject obj, char *url, int playerType, int renderType, jobject surface);

    void unInit();

    void play();

    void pause();

    void stop();

    void seekToPosition(float position);

    long getMediaParams(int paramType);

    void setMediaParams(int paramType, jobject obj);

private:
    MediaPlayer *mediaPlayer = nullptr;

};


#endif //LEARNFFMPEG_PLAYERWRAPPER_H
