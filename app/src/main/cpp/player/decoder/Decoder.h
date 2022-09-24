/**
  ____        _             _____ _
 | __ ) _   _| |_ ___      |  ___| | _____      __
 |  _ \| | | | __/ _ \_____| |_  | |/ _ \ \ /\ / /
 | |_) | |_| | ||  __/_____|  _| | | (_) \ V  V /
 |____/ \__, |\__\___|     |_|   |_|\___/ \_/\_/
        |___/
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_DECODER_H
#define LEARNFFMPEG_DECODER_H

typedef void (*MessageCallback)(void *, int, float);

typedef long (*AVSyncCallback)(void *);

class Decoder {
public:
    virtual void start() = 0;

    virtual void pause() = 0;

    virtual void stop() = 0;

    virtual float getDuration() = 0;

    virtual void seekToPosition(float position) = 0;

    virtual float getCurrentPosition() = 0;

    virtual void setMessageCallback(void *context, MessageCallback callback) = 0;
};

#endif //LEARNFFMPEG_DECODER_H
