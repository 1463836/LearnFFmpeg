/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#include <LogUtil.h>
#include <ImageDef.h>
#include "MediaRecorderContext.h"

jfieldID MediaRecorderContext::contextHandle = 0L;

MediaRecorderContext::MediaRecorderContext() {
    GLCameraRender::getInstance();
}

MediaRecorderContext::~MediaRecorderContext() {
    GLCameraRender::releaseInstance();
}

void MediaRecorderContext::createContext(JNIEnv *env, jobject instance) {
//    LOGCATE("MediaRecorderContext::createContext");
    MediaRecorderContext *pContext = new MediaRecorderContext();
    storeContext(env, instance, pContext);
}

void
MediaRecorderContext::storeContext(JNIEnv *env, jobject instance, MediaRecorderContext *pContext) {
//    LOGCATE("MediaRecorderContext::StoreContext");
    jclass cls = env->GetObjectClass(instance);
    if (cls == NULL) {
        LOGCATE("MediaRecorderContext::StoreContext cls == NULL");
        return;
    }

    contextHandle = env->GetFieldID(cls, "mNativeContextHandle", "J");
    if (contextHandle == NULL) {
        LOGCATE("MediaRecorderContext::StoreContext s_ContextHandle == NULL");
        return;
    }

    env->SetLongField(instance, contextHandle, reinterpret_cast<jlong>(pContext));

}


void MediaRecorderContext::deleteContext(JNIEnv *env, jobject instance) {
//    LOGCATE("MediaRecorderContext::DeleteContext");
    if (contextHandle == NULL) {
        LOGCATE("MediaRecorderContext::DeleteContext Could not find render context.");
        return;
    }

    MediaRecorderContext *pContext = reinterpret_cast<MediaRecorderContext *>(env->GetLongField(
            instance, contextHandle));
    if (pContext) {
        delete pContext;
    }
    env->SetLongField(instance, contextHandle, 0L);
}

MediaRecorderContext *MediaRecorderContext::getContext(JNIEnv *env, jobject instance) {
//    LOGCATE("MediaRecorderContext::GetContext");

    if (contextHandle == NULL) {
        LOGCATE("MediaRecorderContext::GetContext Could not find render context.");
        return NULL;
    }

    MediaRecorderContext *pContext = reinterpret_cast<MediaRecorderContext *>(env->GetLongField(
            instance, contextHandle));
    return pContext;
}

int MediaRecorderContext::init() {
    GLCameraRender::getInstance()->init(0, 0, nullptr);
    GLCameraRender::getInstance()->setRenderCallback(this, renderFrame);
    return 0;
}

int MediaRecorderContext::unInit() {
    GLCameraRender::getInstance()->unInit();

    return 0;
}

int
MediaRecorderContext::startRecord(int recorderType, const char *outUrl, int frameWidth,
                                  int frameHeight, long videoBitRate,
                                  int fps) {
    LOGCATE("MediaRecorderContext::StartRecord recorderType=%d, outUrl=%s, [w,h]=[%d,%d], videoBitRate=%ld, fps=%d",
            recorderType, outUrl, frameWidth, frameHeight, videoBitRate, fps);
    std::unique_lock<std::mutex> lock(m_mutex);
    switch (recorderType) {
        case RECORDER_TYPE_SINGLE_VIDEO:
            if (singleVideoRecorder == nullptr) {
                singleVideoRecorder = new SingleVideoRecorder(outUrl, frameHeight, frameWidth,
                                                              videoBitRate, fps);
                singleVideoRecorder->StartRecord();
            }
            break;
        case RECORDER_TYPE_SINGLE_AUDIO:
            if (singleAudioRecorder == nullptr) {
                singleAudioRecorder = new SingleAudioRecorder(outUrl, DEFAULT_SAMPLE_RATE,
                                                              AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
                singleAudioRecorder->StartRecord();
            }
            break;
        case RECORDER_TYPE_AV:
            if (mediaRecorder == nullptr) {
                RecorderParam param = {0};
                param.frameWidth = frameHeight;
                param.frameHeight = frameWidth;
                param.videoBitRate = videoBitRate;
                param.fps = fps;
                param.audioSampleRate = DEFAULT_SAMPLE_RATE;
                param.channelLayout = AV_CH_LAYOUT_STEREO;
                param.sampleFormat = AV_SAMPLE_FMT_S16;
                mediaRecorder = new MediaRecorder(outUrl, &param);
                mediaRecorder->startRecord();
            }
            break;
        default:
            break;
    }


    return 0;
}

int MediaRecorderContext::stopRecord() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (singleVideoRecorder != nullptr) {
        singleVideoRecorder->StopRecord();
        delete singleVideoRecorder;
        singleVideoRecorder = nullptr;
    }

    if (singleAudioRecorder != nullptr) {
        singleAudioRecorder->StopRecord();
        delete singleAudioRecorder;
        singleAudioRecorder = nullptr;
    }

    if (mediaRecorder != nullptr) {
        mediaRecorder->stopRecord();
        delete mediaRecorder;
        mediaRecorder = nullptr;
    }
    return 0;
}

void MediaRecorderContext::onAudioData(uint8_t *pData, int size) {
//    LOGCATE("MediaRecorderContext::OnAudioData pData=%p, dataSize=%d", pData, size);
    AudioFrame audioFrame(pData, size, false);
    if (singleAudioRecorder != nullptr)
        singleAudioRecorder->OnFrame2Encode(&audioFrame);

    if (mediaRecorder != nullptr)
        mediaRecorder->encodeFrame(&audioFrame);
}

void MediaRecorderContext::previewFrame(int format, uint8_t *pBuffer, int width, int height) {
//    LOGCATE("MediaRecorderContext::UpdateFrame format=%d, width=%d, height=%d, pData=%p",
//            format, width, height, pBuffer);
    NativeImage nativeImage;
    nativeImage.format = format;
    nativeImage.width = width;
    nativeImage.height = height;
    nativeImage.inData[0] = pBuffer;

    switch (format) {
        case IMAGE_FORMAT_NV12:
        case IMAGE_FORMAT_NV21:
            nativeImage.inData[1] = nativeImage.inData[0] + width * height;
            nativeImage.inSize[0] = width;
            nativeImage.inSize[1] = width;
            break;
        case IMAGE_FORMAT_I420:
            nativeImage.inData[1] = nativeImage.inData[0] + width * height;
            nativeImage.inData[2] = nativeImage.inData[1] + width * height / 4;
            nativeImage.inSize[0] = width;
            nativeImage.inSize[1] = width / 2;
            nativeImage.inSize[2] = width / 2;
            break;
        default:
            break;
    }

//	std::unique_lock<std::mutex> lock(m_mutex);
//	if(m_pVideoRecorder!= nullptr) {
//        m_pVideoRecorder->OnFrame2Encode(&nativeImage);
//    }
//	lock.unlock();

    //NativeImageUtil::DumpNativeImage(&nativeImage, "/sdcard", "camera");
    GLCameraRender::getInstance()->renderVideoFrame(&nativeImage);
}

void MediaRecorderContext::setTransformMatrix(float translateX, float translateY, float scaleX,
                                              float scaleY, int degree, int mirror) {
    transformMatrix.translateX = translateX;
    transformMatrix.translateY = translateY;
    transformMatrix.scaleX = scaleX;
    transformMatrix.scaleY = scaleY;
    transformMatrix.degree = degree;
    transformMatrix.mirror = mirror;
    GLCameraRender::getInstance()->updateMVPMatrix(&transformMatrix);
}

void MediaRecorderContext::onSurfaceCreated() {
    GLCameraRender::getInstance()->onSurfaceCreated();
}

void MediaRecorderContext::onSurfaceChanged(int width, int height) {
    GLCameraRender::getInstance()->onSurfaceChanged(width, height);
}

void MediaRecorderContext::onDrawFrame() {
    GLCameraRender::getInstance()->onDrawFrame();
}

void MediaRecorderContext::renderFrame(void *ctx, NativeImage *nativeImage) {
//    LOGCATE("MediaRecorderContext::OnGLRenderFrame ctx=%p, pImage=%p", ctx, pImage);
    MediaRecorderContext *context = static_cast<MediaRecorderContext *>(ctx);
    std::unique_lock<std::mutex> lock(context->m_mutex);
    if (context->singleVideoRecorder != nullptr){
        context->singleVideoRecorder->OnFrame2Encode(nativeImage);
    }else{
//        LOGCATE("singleVideoRecorder==null");
    }


    if (context->mediaRecorder != nullptr)
        context->mediaRecorder->encodeFrame(nativeImage);
}

void
MediaRecorderContext::setLUTImage(int index, int format, int width, int height, uint8_t *pData) {
//    LOGCATE("MediaRecorderContext::SetLUTImage index=%d, format=%d, width=%d, height=%d, pData=%p",
//            index, format, width, height, pData);
    NativeImage nativeImage;
    nativeImage.format = format;
    nativeImage.width = width;
    nativeImage.height = height;
    nativeImage.inData[0] = pData;
    nativeImage.inSize[0] = width * 4; //RGBA

    GLCameraRender::getInstance()->setLUTImage(index, &nativeImage);
}

void MediaRecorderContext::setFragShader(int index, char *pShaderStr, int strSize) {
    GLCameraRender::getInstance()->setFragShaderStr(index, pShaderStr, strSize);
}


