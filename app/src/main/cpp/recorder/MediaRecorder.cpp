/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include "MediaRecorder.h"

MediaRecorder::MediaRecorder(const char *url, RecorderParam *param) {
//    LOGCATE("MediaRecorder::MediaRecorder url=%s", url);
    strcpy(outUrl, url);
    recorderParam = *param;
}

MediaRecorder::~MediaRecorder() {

}

int MediaRecorder::startRecord() {
//    LOGCATE("MediaRecorder::StartRecord");
    int result = 0;
    do {
        /* allocate the output media context */
        avformat_alloc_output_context2(&formatContext, NULL, NULL, outUrl);
        if (!formatContext) {
            LOGCATE("MediaRecorder::StartRecord Could not deduce output format from file extension: using MPEG.\n");
            avformat_alloc_output_context2(&formatContext, NULL, "mpeg", outUrl);
        }
        if (!formatContext) {
            result = -1;
            break;
        }

        outputFormat = formatContext->oformat;

        /* Add the audio and video streams using the default format codecs
         * and initialize the codecs. */
        if (outputFormat->video_codec != AV_CODEC_ID_NONE) {
            addStream(&videoOutputStream, formatContext, &videoCodec, outputFormat->video_codec);
            enableVideo = 1;
        }
        if (outputFormat->audio_codec != AV_CODEC_ID_NONE) {
            addStream(&audioOutputStream, formatContext, &audioCodec, outputFormat->audio_codec);
            enableAudio = 1;
        }

        /* Now that all the parameters are set, we can open the audio and
         * video codecs and allocate the necessary encode buffers. */
        if (enableVideo)
            openVideo(formatContext, videoCodec, &videoOutputStream);

        if (enableAudio)
            openAudio(formatContext, audioCodec, &audioOutputStream);

        av_dump_format(formatContext, 0, outUrl, 1);

        /* open the output file, if needed */
        if (!(outputFormat->flags & AVFMT_NOFILE)) {
            int ret = avio_open(&formatContext->pb, outUrl, AVIO_FLAG_WRITE);
            if (ret < 0) {
                LOGCATE("MediaRecorder::StartRecord Could not open '%s': %s", outUrl,
                        av_err2str(ret));
                result = -1;
                break;
            }
        }

        /* Write the stream header, if any. */
        result = avformat_write_header(formatContext, nullptr);
        if (result < 0) {
            LOGCATE("MediaRecorder::StartRecord Error occurred when opening output file: %s",
                    av_err2str(result));
            result = -1;
            break;
        }

    } while (false);

    if (result >= 0) {
//        if(m_EnableAudio)
//            m_pAudioThread = new thread(StartAudioEncodeThread, this);
//        if(m_EnableVideo)
//            m_pVideoThread = new thread(StartVideoEncodeThread, this);
        if (mediaThread == nullptr)
            mediaThread = new thread(startMediaEncodeThread, this);
    }

    return result;
}

int MediaRecorder::encodeFrame(AudioFrame *inputFrame) {
//    LOGCATE("MediaRecorder::OnFrame2Encode inputFrame->data=%p, inputFrame->dataSize=%d",
//            inputFrame->data, inputFrame->dataSize);
    if (isExit) return 0;
    AudioFrame *pAudioFrame = new AudioFrame(inputFrame->data, inputFrame->dataSize);
    audioFrameQueue.Push(pAudioFrame);
    return 0;
}

int MediaRecorder::encodeFrame(VideoFrame *inputFrame) {
    if (isExit) return 0;
//    LOGCATE("MediaRecorder::OnFrame2Encode [w,h,format]=[%d,%d,%d]", inputFrame->width,
//            inputFrame->height, inputFrame->format);
    VideoFrame *videoFrame = new VideoFrame();
    videoFrame->width = inputFrame->width;
    videoFrame->height = inputFrame->height;
    videoFrame->format = inputFrame->format;
    NativeImageUtil::AllocNativeImage(videoFrame);
    NativeImageUtil::CopyNativeImage(inputFrame, videoFrame);
    videoFrameQueue.Push(videoFrame);
    return 0;
}

int MediaRecorder::stopRecord() {
//    LOGCATE("MediaRecorder::StopRecord");
    isExit = true;
    if (audioThread != nullptr || videoThread != nullptr || mediaThread != nullptr) {

        if (audioThread != nullptr) {
            audioThread->join();
            delete audioThread;
            audioThread = nullptr;
        }

        if (videoThread != nullptr) {
            videoThread->join();
            delete videoThread;
            videoThread = nullptr;
        }

        if (mediaThread != nullptr) {
            mediaThread->join();
            delete mediaThread;
            mediaThread = nullptr;
        }

        while (!videoFrameQueue.Empty()) {
            VideoFrame *pImage = videoFrameQueue.Pop();
            NativeImageUtil::FreeNativeImage(pImage);
            delete pImage;
        }

        while (!audioFrameQueue.Empty()) {
            AudioFrame *pAudio = audioFrameQueue.Pop();
            delete pAudio;
        }

        int ret = av_write_trailer(formatContext);
//        LOGCATE("MediaRecorder::StopRecord while av_write_trailer %s",
//                av_err2str(ret));

        /* Close each codec. */
        if (enableVideo)
            closeStream(&videoOutputStream);
        if (enableAudio)
            closeStream(&audioOutputStream);

        if (!(outputFormat->flags & AVFMT_NOFILE))
            /* Close the output file. */
            avio_closep(&formatContext->pb);

        /* free the stream */
        avformat_free_context(formatContext);
    }
    return 0;
}

void MediaRecorder::startAudioEncodeThread(MediaRecorder *recorder) {
//    LOGCATE("MediaRecorder::StartAudioEncodeThread start");
    AVOutputStream *vOs = &recorder->videoOutputStream;
    AVOutputStream *aOs = &recorder->audioOutputStream;
    AVFormatContext *fmtCtx = recorder->formatContext;
    while (!aOs->encodeEnd) {
        double videoTimestamp = vOs->nextPts * av_q2d(vOs->codecContext->time_base);
        double audioTimestamp = aOs->nextPts * av_q2d(aOs->codecContext->time_base);
//        LOGCATE("MediaRecorder::StartAudioEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]",
//                videoTimestamp, audioTimestamp);
        if (av_compare_ts(vOs->nextPts, vOs->codecContext->time_base,
                          aOs->nextPts, aOs->codecContext->time_base) >= 0 || vOs->encodeEnd) {
//            LOGCATE("MediaRecorder::StartAudioEncodeThread start queueSize=%d",
//                    recorder->m_AudioFrameQueue.Size());
            //if(audioTimestamp >= videoTimestamp && vOs->m_EncodeEnd) aOs->m_EncodeEnd = vOs->m_EncodeEnd;
            aOs->encodeEnd = recorder->encodeAudioFrame(aOs);
        } else {
//            LOGCATE("MediaRecorder::StartAudioEncodeThread usleep");
            usleep(5 * 1000);
        }
    }
//    LOGCATE("MediaRecorder::StartAudioEncodeThread end");
}

void MediaRecorder::startVideoEncodeThread(MediaRecorder *recorder) {
//    LOGCATE("MediaRecorder::StartVideoEncodeThread start");
    AVOutputStream *videoOutputStream = &recorder->videoOutputStream;
    AVOutputStream *audioOutputStream = &recorder->audioOutputStream;
    while (!videoOutputStream->encodeEnd) {
        double videoTimestamp =
                videoOutputStream->nextPts * av_q2d(videoOutputStream->codecContext->time_base);
        double audioTimestamp =
                audioOutputStream->nextPts * av_q2d(audioOutputStream->codecContext->time_base);
//        LOGCATE("MediaRecorder::StartVideoEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]",
//                videoTimestamp, audioTimestamp);
        if (av_compare_ts(videoOutputStream->nextPts, videoOutputStream->codecContext->time_base,
                          audioOutputStream->nextPts, audioOutputStream->codecContext->time_base) <=
            0 || audioOutputStream->encodeEnd) {
//            LOGCATE("MediaRecorder::StartVideoEncodeThread start queueSize=%d",
//                    recorder->videoFrameQueue.Size());
            //视频和音频时间戳对齐，人对于声音比较敏感，防止出现视频声音播放结束画面还没结束的情况
            if (audioTimestamp <= videoTimestamp && audioOutputStream->encodeEnd)
                videoOutputStream->encodeEnd = audioOutputStream->encodeEnd;
            videoOutputStream->encodeEnd = recorder->encodeVideoFrame(videoOutputStream);
        } else {
//            LOGCATE("MediaRecorder::StartVideoEncodeThread start usleep");
            usleep(5 * 1000);
        }
    }
//    LOGCATE("MediaRecorder::StartVideoEncodeThread end");
}


int MediaRecorder::writePacket(AVFormatContext *avFormatContext, AVRational *time_base, AVStream *stream,
                               AVPacket *pkt) {
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, stream->time_base);
    pkt->stream_index = stream->index;

    /* Write the compressed frame to the media file. */
    printfPacket(avFormatContext, pkt);
    return av_interleaved_write_frame(avFormatContext, pkt);
}

void MediaRecorder::addStream(AVOutputStream *avOutputStream, AVFormatContext *formatContext,
                              AVCodec **codec,
                              AVCodecID codec_id) {
//    LOGCATE("MediaRecorder::AddStream");
    AVCodecContext *codecContext;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        LOGCATE("MediaRecorder::AddStream Could not find encoder for '%s'",
                avcodec_get_name(codec_id));
        return;
    }

    avOutputStream->stream = avformat_new_stream(formatContext, NULL);
    if (!avOutputStream->stream) {
        LOGCATE("MediaRecorder::AddStream Could not allocate stream");
        return;
    }
//    outputStream->stream->id = formatContext->nb_streams - 1;
    codecContext = avcodec_alloc_context3(*codec);
    if (!codecContext) {
        LOGCATE("MediaRecorder::AddStream Could not alloc an encoding context");
        return;
    }
    avOutputStream->codecContext = codecContext;

    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
//            AV_SAMPLE_FMT_FLTP
            codecContext->sample_fmt = (*codec)->sample_fmts ?
                                       (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            codecContext->bit_rate = 96000;
            codecContext->sample_rate = recorderParam.audioSampleRate;
            codecContext->channel_layout = recorderParam.channelLayout;
            codecContext->channels = av_get_channel_layout_nb_channels(
                    codecContext->channel_layout);
            avOutputStream->stream->time_base = (AVRational) {1, codecContext->sample_rate};

//            LOGCATE("MediaRecorder::AddStream AVMEDIA_TYPE_AUDIO AVCodecID=%d codecContext->sample_fmt %d codecContext->sample_rate %d codecContext->channel_layout %d  codecContext->channels %d",
//                    codec_id, codecContext->sample_fmt, codecContext->sample_rate,
//                    codecContext->channel_layout, codecContext->channels);
            break;

        case AVMEDIA_TYPE_VIDEO:
//            codecContext->codec_id = codec_id;
            codecContext->bit_rate = recorderParam.videoBitRate;
            /* Resolution must be a multiple of two. */
            codecContext->width = recorderParam.frameWidth;
            codecContext->height = recorderParam.frameHeight;
            /* timebase: This is the fundamental unit of time (in seconds) in terms
             * of which frame timestamps are represented. For fixed-fps content,
             * timebase should be 1/framerate and timestamp increments should be
             * identical to 1. */
            avOutputStream->stream->time_base = (AVRational) {1, recorderParam.fps};
            codecContext->time_base = avOutputStream->stream->time_base;

            codecContext->gop_size = 12; /* emit one intra frame every twelve frames at most */
            codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
//            if (codecContext->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
//                /* just for testing, we also add B-frames */
//                codecContext->max_b_frames = 2;
//            }
//            if (codecContext->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
//                /* Needed to avoid using macroblocks in which some coeffs overflow.
//                 * This does not happen with normal video, it just happens here as
//                 * the motion of the chroma plane does not match the luma plane. */
//                codecContext->mb_decision = 2;
//            }

            LOGCATE("MediaRecorder::AddStream AVMEDIA_TYPE_VIDEO AVCodecID=%d", codec_id);

            break;

        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER)
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void MediaRecorder::printfPacket(AVFormatContext *avFormatContext, AVPacket *pkt) {
    AVRational *time_base = &avFormatContext->streams[pkt->stream_index]->time_base;

//    LOGCATE("MediaRecorder::PrintfPacket pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
//            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
//            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
//            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
//            pkt->stream_index);
}

AVFrame *MediaRecorder::allocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout,
                                        int sample_rate, int nb_samples) {
//    LOGCATE("MediaRecorder::AllocAudioFrame");
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        LOGCATE("MediaRecorder::AllocAudioFrame Error allocating an audio frame");
        return nullptr;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            LOGCATE("MediaRecorder::AllocAudioFrame Error allocating an audio buffer");
            return nullptr;
        }
    }

    return frame;
}

int MediaRecorder::openAudio(AVFormatContext *formatContext, AVCodec *avCodec,
                             AVOutputStream *avOutputStream) {
//    LOGCATE("MediaRecorder::OpenAudio");
    AVCodecContext *codecContext;
    int nb_samples;
    int ret;
    codecContext = avOutputStream->codecContext;
    /* open it */
    ret = avcodec_open2(codecContext, avCodec, nullptr);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenAudio Could not open audio codec: %s", av_err2str(ret));
        return -1;
    }

    if (codecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = codecContext->frame_size;

    avOutputStream->frame = allocAudioFrame(codecContext->sample_fmt, codecContext->channel_layout,
                                            codecContext->sample_rate, nb_samples);

    avOutputStream->tmpFrame = av_frame_alloc();

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(avOutputStream->stream->codecpar, codecContext);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenAudio Could not copy the stream parameters");
        return -1;
    }

    /* create resampler context */
    avOutputStream->swrContext = swr_alloc();
    if (!avOutputStream->swrContext) {
        LOGCATE("MediaRecorder::OpenAudio Could not allocate resampler context");
        return -1;
    }

    /* set options */
    av_opt_set_int(avOutputStream->swrContext, "in_channel_count", codecContext->channels, 0);
    av_opt_set_int(avOutputStream->swrContext, "in_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(avOutputStream->swrContext, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(avOutputStream->swrContext, "out_channel_count", codecContext->channels, 0);
    av_opt_set_int(avOutputStream->swrContext, "out_sample_rate", codecContext->sample_rate, 0);
    av_opt_set_sample_fmt(avOutputStream->swrContext, "out_sample_fmt", codecContext->sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(avOutputStream->swrContext)) < 0) {
        LOGCATE("MediaRecorder::OpenAudio Failed to initialize the resampling context");
        return -1;
    }
    return 0;
}

int MediaRecorder::encodeAudioFrame(AVOutputStream *avOutputStream) {
//    LOGCATE("MediaRecorder::EncodeAudioFrame");
    int result = 0;
    AVCodecContext *codecContext;
    AVPacket packet = {0}; // data and size must be 0;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    av_init_packet(&packet);
    codecContext = avOutputStream->codecContext;

    while (audioFrameQueue.Empty() && !isExit) {
        usleep(10 * 1000);
    }

    AudioFrame *audioFrame = audioFrameQueue.Pop();
    frame = avOutputStream->tmpFrame;
    if (audioFrame) {
        frame->data[0] = audioFrame->data;
        frame->nb_samples = audioFrame->dataSize / 4;
        frame->pts = avOutputStream->nextPts;
        avOutputStream->nextPts += frame->nb_samples;
    }

    if ((audioFrameQueue.Empty() && isExit) || avOutputStream->encodeEnd) frame = nullptr;

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(avOutputStream->swrContext, codecContext->sample_rate) +
                frame->nb_samples,
                codecContext->sample_rate, codecContext->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(avOutputStream->frame);
        if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame Error while av_frame_make_writable");
            result = 1;
            goto EXIT;
        }

        /* convert to destination format */
        ret = swr_convert(avOutputStream->swrContext,
                          avOutputStream->frame->data, dst_nb_samples,
                          (const uint8_t **) frame->data, frame->nb_samples);
//        LOGCATE("MediaRecorder::EncodeAudioFrame dst_nb_samples=%d, frame->nb_samples=%d",
//                dst_nb_samples, frame->nb_samples);
        if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame Error while converting");
            result = 1;
            goto EXIT;
        }
        frame = avOutputStream->frame;

        frame->pts = av_rescale_q(avOutputStream->samplesCount,
                                  (AVRational) {1, codecContext->sample_rate},
                                  codecContext->time_base);
        avOutputStream->samplesCount += dst_nb_samples;
    }

    ret = avcodec_send_frame(codecContext, frame);
    if (ret == AVERROR_EOF) {
        result = 1;
        goto EXIT;
    } else if (ret < 0) {
        LOGCATE("MediaRecorder::EncodeAudioFrame audio avcodec_send_frame fail. ret=%s",
                av_err2str(ret));
        result = 0;
        goto EXIT;
    }

    while (!ret) {
        ret = avcodec_receive_packet(codecContext, &packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            result = 0;
            goto EXIT;
        } else if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame audio avcodec_receive_packet fail. ret=%s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
//        LOGCATE("MediaRecorder::EncodeAudioFrame pkt pts=%ld, size=%d", pkt.pts, pkt.size);
        int result = writePacket(formatContext, &codecContext->time_base, avOutputStream->stream,
                                 &packet);
        if (result < 0) {
            LOGCATE("MediaRecorder::EncodeAudioFrame audio Error while writing audio frame: %s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
    }

    EXIT:
    if (audioFrame) delete audioFrame;
    return result;
}

AVFrame *MediaRecorder::allocVideoFrame(AVPixelFormat pix_fmt, int width, int height) {
//    LOGCATE("MediaRecorder::AllocVideoFrame");
    AVFrame *frame;
    int ret;

    frame = av_frame_alloc();
    if (!frame)
        return nullptr;

    frame->format = pix_fmt;
    frame->width = width;
    frame->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(frame, 1);
    if (ret < 0) {
        LOGCATE("MediaRecorder::AllocVideoFrame Could not allocate frame data.");
        return nullptr;
    }

    return frame;
}

int MediaRecorder::openVideo(AVFormatContext *avFormatContext, AVCodec *avCodec,
                             AVOutputStream *avOutputStream) {
//    LOGCATE("MediaRecorder::OpenVideo");
    int ret;
    AVCodecContext *codecContext = avOutputStream->codecContext;

    /* open the codec */
    ret = avcodec_open2(codecContext, avCodec, nullptr);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenVideo Could not open video codec: %s", av_err2str(ret));
        return -1;
    }

    /* allocate and init a re-usable frame */
    avOutputStream->frame = allocVideoFrame(codecContext->pix_fmt, codecContext->width,
                                            codecContext->height);
    avOutputStream->tmpFrame = av_frame_alloc();
    if (!avOutputStream->frame) {
        LOGCATE("MediaRecorder::OpenVideo Could not allocate video frame");
        return -1;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(avOutputStream->stream->codecpar, codecContext);
    if (ret < 0) {
        LOGCATE("MediaRecorder::OpenVideo Could not copy the stream parameters");
        return -1;
    }
    return 0;
}

int MediaRecorder::encodeVideoFrame(AVOutputStream *avOutputStream) {
//    LOGCATE("MediaRecorder::EncodeVideoFrame");
    int result = 0;
    int ret;
    AVCodecContext *codecContext;
    AVFrame *frame;
    AVPacket pkt = {0};

    codecContext = avOutputStream->codecContext;

    av_init_packet(&pkt);

    while (videoFrameQueue.Empty() && !isExit) {

        usleep(10 * 1000);
    }

    frame = avOutputStream->tmpFrame;
    AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
    VideoFrame *videoFrame = videoFrameQueue.Pop();
    if (videoFrame) {
        frame->data[0] = videoFrame->inData[0];
        frame->data[1] = videoFrame->inData[1];
        frame->data[2] = videoFrame->inData[2];
        frame->linesize[0] = videoFrame->inSize[0];
        frame->linesize[1] = videoFrame->inSize[1];
        frame->linesize[2] = videoFrame->inSize[2];
        frame->width = videoFrame->width;
        frame->height = videoFrame->height;
        switch (videoFrame->format) {
            case IMAGE_FORMAT_RGBA:
                srcPixFmt = AV_PIX_FMT_RGBA;
                break;
            case IMAGE_FORMAT_NV21:
                srcPixFmt = AV_PIX_FMT_NV21;
                break;
            case IMAGE_FORMAT_NV12:
                srcPixFmt = AV_PIX_FMT_NV12;
                break;
            case IMAGE_FORMAT_I420:
                srcPixFmt = AV_PIX_FMT_YUV420P;
                break;
            default:
                LOGCATE("MediaRecorder::EncodeVideoFrame unSupport format pImage->format=%d",
                        videoFrame->format);
                break;
        }
    }

    if ((videoFrameQueue.Empty() && isExit) || avOutputStream->encodeEnd) frame = nullptr;

    if (frame != nullptr) {
        /* when we pass a frame to the encoder, it may keep a reference to it
        * internally; make sure we do not overwrite it here */
        if (av_frame_make_writable(avOutputStream->frame) < 0) {
            result = 1;
            goto EXIT;
        }


        if (srcPixFmt != AV_PIX_FMT_YUV420P) {
            /* as we only generate a YUV420P picture, we must convert it
             * to the codec pixel format if needed */
            //AV_PIX_FMT_YUV420P
            if (!avOutputStream->swsContext) {
                avOutputStream->swsContext = sws_getContext(codecContext->width,
                                                            codecContext->height,
                                                            srcPixFmt,//AV_PIX_FMT_RGBA
                                                            codecContext->width,
                                                            codecContext->height,
                                                            codecContext->pix_fmt,//AV_PIX_FMT_YUV420P
                                                            SWS_FAST_BILINEAR, nullptr, nullptr,
                                                            nullptr);
                if (!avOutputStream->swsContext) {
                    LOGCATE("MediaRecorder::EncodeVideoFrame Could not initialize the conversion context\n");
                    result = 1;
                    goto EXIT;
                }
            }
            sws_scale(avOutputStream->swsContext, (const uint8_t *const *) frame->data,
                      frame->linesize, 0, codecContext->height, avOutputStream->frame->data,
                      avOutputStream->frame->linesize);

            LOGCATE("%s w %d h %d srcPixFmt %d,codecContext->pix_fmt %d src lineSize %d size %d dest lineSize %d size %d ",
                     __FUNCTION__,codecContext->width,
                    codecContext->height, srcPixFmt,
                    codecContext->pix_fmt, frame->linesize[0],
                    strlen((const char *) frame->data[0]), avOutputStream->frame->linesize[0],
                    strlen((const char *) avOutputStream->frame->data[0]));
        }
        avOutputStream->frame->pts = avOutputStream->nextPts++;
        frame = avOutputStream->frame;
    }

//    if(frame) {
//        NativeImage i420;
//        i420.format = IMAGE_FORMAT_I420;
//        i420.width = frame->width;
//        i420.height = frame->height;
//        i420.ppPlane[0] = frame->data[0];
//        i420.ppPlane[1] = frame->data[1];
//        i420.ppPlane[2] = frame->data[2];
//        i420.pLineSize[0] = frame->linesize[0];
//        i420.pLineSize[1] = frame->linesize[1];
//        i420.pLineSize[2] = frame->linesize[2];
//        NativeImageUtil::DumpNativeImage(&i420, "/sdcard/DCIM", "media");
//    }

    /* encode the image */
    ret = avcodec_send_frame(codecContext, frame);
    if (ret == AVERROR_EOF) {
        result = 1;
        goto EXIT;
    } else if (ret < 0) {
        LOGCATE("MediaRecorder::EncodeVideoFrame video avcodec_send_frame fail. ret=%s",
                av_err2str(ret));
        result = 0;
        goto EXIT;
    }

    while (!ret) {
        ret = avcodec_receive_packet(codecContext, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            result = 0;
            goto EXIT;
        } else if (ret < 0) {
            LOGCATE("MediaRecorder::EncodeVideoFrame video avcodec_receive_packet fail. ret=%s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
//        LOGCATE("MediaRecorder::EncodeVideoFrame video pkt pts=%ld, size=%d", pkt.pts, pkt.size);
        int result = writePacket(formatContext, &codecContext->time_base, avOutputStream->stream,
                                 &pkt);
        if (result < 0) {
            LOGCATE("MediaRecorder::EncodeVideoFrame video Error while writing audio frame: %s",
                    av_err2str(ret));
            result = 0;
            goto EXIT;
        }
    }

    EXIT:
    NativeImageUtil::FreeNativeImage(videoFrame);
    if (videoFrame) delete videoFrame;
    return result;
}

void MediaRecorder::closeStream(AVOutputStream *avOutputStream) {
    avcodec_free_context(&avOutputStream->codecContext);
    av_frame_free(&avOutputStream->frame);
    sws_freeContext(avOutputStream->swsContext);
    swr_free(&avOutputStream->swrContext);
    if (avOutputStream->tmpFrame != nullptr) {
        av_free(avOutputStream->tmpFrame);
        avOutputStream->tmpFrame = nullptr;
    }

}

void MediaRecorder::startMediaEncodeThread(MediaRecorder *recorder) {
//    LOGCATE("MediaRecorder::StartMediaEncodeThread start");
    AVOutputStream *videoOutputStream = &recorder->videoOutputStream;
    AVOutputStream *audioOutputStream = &recorder->audioOutputStream;
    while (!videoOutputStream->encodeEnd || !audioOutputStream->encodeEnd) {
        double videoTimestamp =
                videoOutputStream->nextPts * av_q2d(videoOutputStream->codecContext->time_base);
        double audioTimestamp =
                audioOutputStream->nextPts * av_q2d(audioOutputStream->codecContext->time_base);
//        LOGCATE("MediaRecorder::StartVideoEncodeThread [videoTimestamp, audioTimestamp]=[%lf, %lf]",
//                videoTimestamp, audioTimestamp);
        if (!videoOutputStream->encodeEnd &&
            (audioOutputStream->encodeEnd ||
             av_compare_ts(videoOutputStream->nextPts, videoOutputStream->codecContext->time_base,
                           audioOutputStream->nextPts,
                           audioOutputStream->codecContext->time_base) <= 0)) {
            //视频和音频时间戳对齐，人对于声音比较敏感，防止出现视频声音播放结束画面还没结束的情况
            if (audioTimestamp <= videoTimestamp && audioOutputStream->encodeEnd)
                videoOutputStream->encodeEnd = 1;
            videoOutputStream->encodeEnd = recorder->encodeVideoFrame(videoOutputStream);
        } else {
            audioOutputStream->encodeEnd = recorder->encodeAudioFrame(audioOutputStream);
        }
    }
//    LOGCATE("MediaRecorder::StartVideoEncodeThread end");
}
