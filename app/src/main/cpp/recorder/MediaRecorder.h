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

#ifndef LEARNFFMPEG_MEDIARECORDER_H
#define LEARNFFMPEG_MEDIARECORDER_H

#include <ImageDef.h>
#include <render/audio/AudioRender.h>
#include "ThreadSafeQueue.h"
#include "thread"

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

using namespace std;

typedef NativeImage VideoFrame;

class AVOutputStream {
public:
    AVOutputStream() {
        stream = nullptr;
        codecContext = nullptr;
        frame = nullptr;
        tmpFrame = nullptr;
        swrContext = nullptr;
        swsContext = nullptr;
        nextPts = 0;
        samplesCount = 0;
        encodeEnd = 0;
    }

    ~AVOutputStream() {}

public:
    AVStream *stream;
    AVCodecContext *codecContext;
    volatile int64_t nextPts;
    volatile int encodeEnd;
    int samplesCount;
    AVFrame *frame;
    AVFrame *tmpFrame;
    SwsContext *swsContext;
    SwrContext *swrContext;
};

struct RecorderParam {
    //video
    int frameWidth;
    int frameHeight;
    long videoBitRate;
    int fps;

    //audio
    int audioSampleRate;
    int channelLayout;
    int sampleFormat;
};

class MediaRecorder {
public:
    MediaRecorder(const char *url, RecorderParam *param);

    ~MediaRecorder();

    //开始录制
    int startRecord();

    //添加音频数据到音频队列
    int encodeFrame(AudioFrame *inputFrame);

    //添加视频数据到视频队列
    int encodeFrame(VideoFrame *inputFrame);

    //停止录制
    int stopRecord();

private:
    //启动音频编码线程
    static void startAudioEncodeThread(MediaRecorder *recorder);

    //启动视频编码线程
    static void startVideoEncodeThread(MediaRecorder *recorder);

    static void startMediaEncodeThread(MediaRecorder *recorder);

    //分配音频缓冲帧
    AVFrame *allocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate,
                             int nb_samples);

    //分配视频缓冲帧
    AVFrame *allocVideoFrame(AVPixelFormat pix_fmt, int width, int height);

    //写编码包到媒体文件
    int writePacket(AVFormatContext *avFormatContext, AVRational *time_base, AVStream *stream, AVPacket *pkt);

    //添加媒体流程
    void addStream(AVOutputStream *avOutputStream, AVFormatContext *formatContext, AVCodec **codec, AVCodecID codec_id);

    //打印 packet 信息
    void printfPacket(AVFormatContext *avFormatContext, AVPacket *pkt);

    //打开音频编码器
    int openAudio(AVFormatContext *formatContext, AVCodec *avCodec, AVOutputStream *avOutputStream);

    //打开视频编码器
    int openVideo(AVFormatContext *avFormatContext, AVCodec *avCodec, AVOutputStream *avOutputStream);

    //编码一帧音频
    int encodeAudioFrame(AVOutputStream *avOutputStream);

    //编码一帧视频
    int encodeVideoFrame(AVOutputStream *avOutputStream);

    //释放编码器上下文
    void closeStream(AVOutputStream *avOutputStream);

private:
    RecorderParam recorderParam = {0};
    AVOutputStream videoOutputStream;
    AVOutputStream audioOutputStream;
    char outUrl[1024] = {0};
    AVOutputFormat *outputFormat = nullptr;
    AVFormatContext *formatContext = nullptr;
    AVCodec *audioCodec = nullptr;
    AVCodec *videoCodec = nullptr;
    //视频帧队列
    ThreadSafeQueue<VideoFrame *>
            videoFrameQueue;
    //音频帧队列
    ThreadSafeQueue<AudioFrame *>
            audioFrameQueue;
    int enableVideo = 0;
    int enableAudio = 0;
    volatile bool isExit = false;
    //音频编码线程
    thread *audioThread = nullptr;
    //视频编码线程
    thread *videoThread = nullptr;
    thread *mediaThread = nullptr;
};


#endif //LEARNFFMPEG_MEDIARECORDER_H
