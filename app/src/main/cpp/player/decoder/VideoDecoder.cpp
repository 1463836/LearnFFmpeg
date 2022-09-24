/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include "VideoDecoder.h"

void VideoDecoder::onDecoderReady() {
//    LOGCATE("VideoDecoder::OnDecoderReady");
    videoWidth = getCodecContext()->width;
    videoHeight = getCodecContext()->height;

    if (msgContext && msgCallback)
        msgCallback(msgContext, MSG_DECODER_READY, 0);

    if (videoRender != nullptr) {
        int dstSize[2] = {0};
        videoRender->init(videoWidth, videoHeight, dstSize);
        renderWidth = dstSize[0];
        renderHeight = dstSize[1];

        if (videoRender->getRenderType() == VIDEO_RENDER_ANWINDOW) {
            int fps = 25;
            long videoBitRate = renderWidth * renderHeight * fps * 0.2;
            videoRecorder = new SingleVideoRecorder("/sdcard/learnffmpeg_output.mp4",
                                                    renderWidth, renderHeight, videoBitRate,
                                                    fps);
            videoRecorder->StartRecord();
        }

        RGBAFrame = av_frame_alloc();
        int bufferSize = av_image_get_buffer_size(DST_PIXEL_FORMAT, renderWidth, renderHeight,
                                                  1);
        frameBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
        av_image_fill_arrays(RGBAFrame->data, RGBAFrame->linesize,
                             frameBuffer, DST_PIXEL_FORMAT, renderWidth, renderHeight, 1);

        swsContext = sws_getContext(videoWidth, videoHeight, getCodecContext()->pix_fmt,
                                    renderWidth, renderHeight, DST_PIXEL_FORMAT,
                                    SWS_FAST_BILINEAR, NULL, NULL, NULL);
    } else {
        LOGCATE("VideoDecoder::OnDecoderReady m_VideoRender == null");
    }
}

void VideoDecoder::onDecoderDone() {
//    LOGCATE("VideoDecoder::OnDecoderDone");

    if (msgContext && msgCallback)
        msgCallback(msgContext, MSG_DECODER_DONE, 0);

    if (videoRender)
        videoRender->unInit();

    if (RGBAFrame != nullptr) {
        av_frame_free(&RGBAFrame);
        RGBAFrame = nullptr;
    }

    if (frameBuffer != nullptr) {
        free(frameBuffer);
        frameBuffer = nullptr;
    }

    if (swsContext != nullptr) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }

    if (videoRecorder != nullptr) {
        videoRecorder->StopRecord();
        delete videoRecorder;
        videoRecorder = nullptr;
    }

}

void VideoDecoder::onFrameAvailable(AVFrame *frame) {
//    LOGCATE("VideoDecoder::OnFrameAvailable frame=%p", frame);
    if (videoRender != nullptr && frame != nullptr) {
        NativeImage image;
//        LOGCATE("VideoDecoder::OnFrameAvailable frame[w,h]=[%d, %d],format=%d,[line0,line1,line2]=[%d, %d, %d]",
//                frame->width, frame->height, GetCodecContext()->pix_fmt, frame->linesize[0],
//                frame->linesize[1], frame->linesize[2]);
        if (videoRender->getRenderType() == VIDEO_RENDER_ANWINDOW) {
            sws_scale(swsContext, frame->data, frame->linesize, 0,
                      videoHeight, RGBAFrame->data, RGBAFrame->linesize);

            image.format = IMAGE_FORMAT_RGBA;
            image.width = renderWidth;
            image.height = renderHeight;
            image.inData[0] = RGBAFrame->data[0];
            image.inSize[0] = image.width * 4;
        } else if (getCodecContext()->pix_fmt == AV_PIX_FMT_YUV420P ||
                getCodecContext()->pix_fmt == AV_PIX_FMT_YUVJ420P) {
            image.format = IMAGE_FORMAT_I420;
            image.width = frame->width;
            image.height = frame->height;
            image.inSize[0] = frame->linesize[0];
            image.inSize[1] = frame->linesize[1];
            image.inSize[2] = frame->linesize[2];
            image.inData[0] = frame->data[0];
            image.inData[1] = frame->data[1];
            image.inData[2] = frame->data[2];
            if (frame->data[0] && frame->data[1] && !frame->data[2] &&
                frame->linesize[0] == frame->linesize[1] && frame->linesize[2] == 0) {
                // on some android device, output of h264 mediacodec decoder is NV12 兼容某些设备可能出现的格式不匹配问题
                image.format = IMAGE_FORMAT_NV12;
            }
        } else if (getCodecContext()->pix_fmt == AV_PIX_FMT_NV12) {
            image.format = IMAGE_FORMAT_NV12;
            image.width = frame->width;
            image.height = frame->height;
            image.inSize[0] = frame->linesize[0];
            image.inSize[1] = frame->linesize[1];
            image.inData[0] = frame->data[0];
            image.inData[1] = frame->data[1];
        } else if (getCodecContext()->pix_fmt == AV_PIX_FMT_NV21) {
            image.format = IMAGE_FORMAT_NV21;
            image.width = frame->width;
            image.height = frame->height;
            image.inSize[0] = frame->linesize[0];
            image.inSize[1] = frame->linesize[1];
            image.inData[0] = frame->data[0];
            image.inData[1] = frame->data[1];
        } else if (getCodecContext()->pix_fmt == AV_PIX_FMT_RGBA) {
            image.format = IMAGE_FORMAT_RGBA;
            image.width = frame->width;
            image.height = frame->height;
            image.inSize[0] = frame->linesize[0];
            image.inData[0] = frame->data[0];
        } else {
            sws_scale(swsContext, frame->data, frame->linesize, 0,
                      videoHeight, RGBAFrame->data, RGBAFrame->linesize);
            image.format = IMAGE_FORMAT_RGBA;
            image.width = renderWidth;
            image.height = renderHeight;
            image.inData[0] = RGBAFrame->data[0];
            image.inSize[0] = image.width * 4;
        }

        videoRender->renderVideoFrame(&image);

        if (videoRecorder != nullptr) {
            videoRecorder->OnFrame2Encode(&image);
        }
    }

    if (msgContext && msgCallback)
        msgCallback(msgContext, MSG_REQUEST_RENDER, 0);
}
