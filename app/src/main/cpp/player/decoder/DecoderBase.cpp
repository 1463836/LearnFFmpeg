/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include "DecoderBase.h"
#include "LogUtil.h"
#include "../../util/LogUtil.h"

void DecoderBase::start() {
    if (m_Thread == nullptr) {
        startDecodingThread();
    } else {
        std::unique_lock<std::mutex> lock(m_Mutex);
        decoderState = STATE_DECODING;
        condition.notify_all();
    }
}

void DecoderBase::pause() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    decoderState = STATE_PAUSE;
}

void DecoderBase::stop() {
    LOGCATE("DecoderBase::Stop");
    std::unique_lock<std::mutex> lock(m_Mutex);
    decoderState = STATE_STOP;
    condition.notify_all();
}

void DecoderBase::seekToPosition(float position) {
    LOGCATE("DecoderBase::SeekToPosition position=%f", position);
    std::unique_lock<std::mutex> lock(m_Mutex);
    seekPosition = position;
    decoderState = STATE_DECODING;
    condition.notify_all();
}

float DecoderBase::getCurrentPosition() {
    //std::unique_lock<std::mutex> lock(m_Mutex);//读写保护
    //单位 ms
    return m_CurTimeStamp;
}

int DecoderBase::init(const char *url, AVMediaType mediaType) {
//    LOGCATE("DecoderBase::Init url=%s, mediaType=%d", url, mediaType);
    strcpy(this->url, url);
    this->mediaType = mediaType;
    return 0;
}

void DecoderBase::unInit() {
//    LOGCATE("DecoderBase::UnInit m_MediaType=%d", m_MediaType);
    if (m_Thread) {
        stop();
        m_Thread->join();
        delete m_Thread;
        m_Thread = nullptr;
    }
//    LOGCATE("DecoderBase::UnInit end, m_MediaType=%d", m_MediaType);
}

int DecoderBase::initFFDecoder() {
    int result = -1;
    do {
        //1.创建封装格式上下文
        formatContext = avformat_alloc_context();

        //2.打开文件
        if (avformat_open_input(&formatContext, url, NULL, NULL) != 0) {
            LOGCATE("DecoderBase::InitFFDecoder avformat_open_input fail.");
            break;
        }

        //3.获取音视频流信息
        if (avformat_find_stream_info(formatContext, NULL) < 0) {
            LOGCATE("DecoderBase::InitFFDecoder avformat_find_stream_info fail.");
            break;
        }

        //4.获取音视频流索引
        for (int i = 0; i < formatContext->nb_streams; i++) {
            if (formatContext->streams[i]->codecpar->codec_type == mediaType) {
                streamIndex = i;
                break;
            }
        }

        if (streamIndex == -1) {
            LOGCATE("DecoderBase::InitFFDecoder Fail to find stream index.");
            break;
        }
        //5.获取解码器参数
        AVCodecParameters *codecParameters = formatContext->streams[streamIndex]->codecpar;

        //6.获取解码器
        codec = avcodec_find_decoder(codecParameters->codec_id);
        if (codec == nullptr) {
            LOGCATE("DecoderBase::InitFFDecoder avcodec_find_decoder fail.");
            break;
        }

        //7.创建解码器上下文
        codecContext = avcodec_alloc_context3(codec);
        if (avcodec_parameters_to_context(codecContext, codecParameters) != 0) {
            LOGCATE("DecoderBase::InitFFDecoder avcodec_parameters_to_context fail.");
            break;
        }

        AVDictionary *dictionary = nullptr;
        av_dict_set(&dictionary, "buffer_size", "1024000", 0);
        av_dict_set(&dictionary, "stimeout", "20000000", 0);
        av_dict_set(&dictionary, "max_delay", "30000000", 0);
        av_dict_set(&dictionary, "rtsp_transport", "tcp", 0);

        //8.打开解码器
        result = avcodec_open2(codecContext, codec, &dictionary);
        if (result < 0) {
            LOGCATE("DecoderBase::InitFFDecoder avcodec_open2 fail. result=%d", result);
            break;
        }
        result = 0;

        duration = formatContext->duration / AV_TIME_BASE * 1000;//us to ms
        //创建 AVPacket 存放编码数据
        packet = av_packet_alloc();
        //创建 AVFrame 存放解码后的数据
        frame = av_frame_alloc();
    } while (false);

    if (result != 0 && msgContext && msgCallback)
        msgCallback(msgContext, MSG_DECODER_INIT_ERROR, 0);

    return result;
}

void DecoderBase::unInitDecoder() {
//    LOGCATE("DecoderBase::UnInitDecoder");
    if (frame != nullptr) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    if (packet != nullptr) {
        av_packet_free(&packet);
        packet = nullptr;
    }

    if (codecContext != nullptr) {
        avcodec_close(codecContext);
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
        codec = nullptr;
    }

    if (formatContext != nullptr) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }

}

void DecoderBase::startDecodingThread() {
    m_Thread = new thread(doAVDecoding, this);
}

void DecoderBase::decodingLoop() {
//    LOGCATE("DecoderBase::DecodingLoop start, m_MediaType=%d", m_MediaType);
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        decoderState = STATE_DECODING;
        lock.unlock();
    }

    for (;;) {
        while (decoderState == STATE_PAUSE) {
            std::unique_lock<std::mutex> lock(m_Mutex);
//            LOGCATE("DecoderBase::DecodingLoop waiting, m_MediaType=%d", m_MediaType);
            condition.wait_for(lock, std::chrono::milliseconds(10));
            m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
        }

        if (decoderState == STATE_STOP) {
            break;
        }

        if (m_StartTimeStamp == -1)
            m_StartTimeStamp = GetSysCurrentTime();

        if (decodeOnePacket() != 0) {
            //解码结束，暂停解码器
            std::unique_lock<std::mutex> lock(m_Mutex);
            decoderState = STATE_PAUSE;
        }
    }
//    LOGCATE("DecoderBase::DecodingLoop end");
}

void DecoderBase::updateTimeStamp() {
//    LOGCATE("DecoderBase::UpdateTimeStamp");
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (frame->pkt_dts != AV_NOPTS_VALUE) {
        m_CurTimeStamp = frame->pkt_dts;
    } else if (frame->pts != AV_NOPTS_VALUE) {
        m_CurTimeStamp = frame->pts;
    } else {
        m_CurTimeStamp = 0;
    }

    m_CurTimeStamp = (int64_t) (
            (m_CurTimeStamp * av_q2d(formatContext->streams[streamIndex]->time_base)) * 1000);

    if (seekPosition > 0 && seekSuccess) {
        m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
        seekPosition = 0;
        seekSuccess = false;
    }
}

long DecoderBase::AVSync() {
//    LOGCATE("DecoderBase::AVSync");
    long curSysTime = GetSysCurrentTime();
    //基于系统时钟计算从开始播放流逝的时间
    long elapsedTime = curSysTime - m_StartTimeStamp;

    if (msgContext && msgCallback && mediaType == AVMEDIA_TYPE_AUDIO)
        msgCallback(msgContext, MSG_DECODING_TIME, m_CurTimeStamp * 1.0f / 1000);

    long delay = 0;

    //向系统时钟同步
    if (m_CurTimeStamp > elapsedTime) {
        //休眠时间
        auto sleepTime = static_cast<unsigned int>(m_CurTimeStamp - elapsedTime);//ms
        //限制休眠时间不能过长
        sleepTime = sleepTime > DELAY_THRESHOLD ? DELAY_THRESHOLD : sleepTime;
        av_usleep(sleepTime * 1000);
    }
    delay = elapsedTime - m_CurTimeStamp;

    return delay;
}

int DecoderBase::decodeOnePacket() {
//    LOGCATE("DecoderBase::DecodeOnePacket m_MediaType=%d", m_MediaType);
    if (seekPosition > 0) {
        //seek to frame
        int64_t seek_target = static_cast<int64_t>(seekPosition * 1000000);//微秒
        int64_t seek_min = INT64_MIN;
        int64_t seek_max = INT64_MAX;
        int seek_ret = avformat_seek_file(formatContext, -1, seek_min, seek_target, seek_max,
                                          0);
        if (seek_ret < 0) {
            seekSuccess = false;
//            LOGCATE("BaseDecoder::DecodeOneFrame error while seeking m_MediaType=%d", m_MediaType);
        } else {
            if (-1 != streamIndex) {
                avcodec_flush_buffers(codecContext);
            }
            clearCache();
            seekSuccess = true;
//            LOGCATE("BaseDecoder::DecodeOneFrame seekFrame pos=%f, m_MediaType=%d", m_SeekPosition,
//                    m_MediaType);
        }
    }
    int result = av_read_frame(formatContext, packet);
    while (result == 0) {
        if (packet->stream_index == streamIndex) {
//            UpdateTimeStamp(m_Packet);
//            if(AVSync() > DELAY_THRESHOLD && m_CurTimeStamp > DELAY_THRESHOLD)
//            {
//                result = 0;
//                goto __EXIT;
//            }

            if (avcodec_send_packet(codecContext, packet) == AVERROR_EOF) {
                //解码结束
                result = -1;
                goto __EXIT;
            }

            //一个 packet 包含多少 frame?
            int frameCount = 0;
            while (avcodec_receive_frame(codecContext, frame) == 0) {
                //更新时间戳
                updateTimeStamp();
                //同步
                AVSync();
                //渲染
//                LOGCATE("DecoderBase::DecodeOnePacket 000 m_MediaType=%d", m_MediaType);
                onFrameAvailable(frame);
//                LOGCATE("DecoderBase::DecodeOnePacket 0001 m_MediaType=%d", m_MediaType);
                frameCount++;
            }
//            LOGCATE("BaseDecoder::DecodeOneFrame frameCount=%d", frameCount);
            //判断一个 packet 是否解码完成
            if (frameCount > 0) {
                result = 0;
                goto __EXIT;
            }
        }
        av_packet_unref(packet);
        result = av_read_frame(formatContext, packet);
    }

    __EXIT:
    av_packet_unref(packet);
    return result;
}

void DecoderBase::doAVDecoding(DecoderBase *decoder) {
//    LOGCATE("DecoderBase::DoAVDecoding");
    do {
        if (decoder->initFFDecoder() != 0) {
            break;
        }
        decoder->onDecoderReady();
        decoder->decodingLoop();
    } while (false);

    decoder->unInitDecoder();
    decoder->onDecoderDone();

}


