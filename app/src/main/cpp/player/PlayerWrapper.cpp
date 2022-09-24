/**
 *
 * Created by 公众号：字节流动 on 2021/12/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "PlayerWrapper.h"

void PlayerWrapper::init(JNIEnv *jniEnv, jobject obj, char *url, int playerType, int renderType,
                         jobject surface) {
    switch (playerType) {
        case FFMEDIA_PLAYER:
            mediaPlayer = new FFMediaPlayer();
            break;
        case HWCODEC_PLAYER:
            mediaPlayer = new HWCodecPlayer();
            break;
        default:
            break;
    }

    if (mediaPlayer)
        mediaPlayer->init(jniEnv, obj, url, renderType, surface);
}

void PlayerWrapper::unInit() {
    if (mediaPlayer) {
        mediaPlayer->unInit();
        delete mediaPlayer;
        mediaPlayer = nullptr;
    }
}

void PlayerWrapper::play() {
    if (mediaPlayer) {
        mediaPlayer->play();
    }
}

void PlayerWrapper::pause() {
    if (mediaPlayer) {
        mediaPlayer->pause();
    }
}

void PlayerWrapper::stop() {
    if (mediaPlayer) {
        mediaPlayer->stop();
    }
}

void PlayerWrapper::seekToPosition(float position) {
    if (mediaPlayer) {
        mediaPlayer->seekToPosition(position);
    }

}

long PlayerWrapper::getMediaParams(int paramType) {
    if (mediaPlayer) {
        return mediaPlayer->getMediaParams(paramType);
    }

    return 0;
}

void PlayerWrapper::setMediaParams(int paramType, jobject obj) {
    if (mediaPlayer) {
        mediaPlayer->setMediaParams(paramType, obj);
    }

}
