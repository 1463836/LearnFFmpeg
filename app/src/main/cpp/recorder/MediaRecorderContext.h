/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H
#define OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H

#include <cstdint>
#include <jni.h>
#include <SingleVideoRecorder.h>
#include "VideoGLRender.h"
#include "GLCameraRender.h"
#include "SingleAudioRecorder.h"
#include "MediaRecorder.h"

#define RECORDER_TYPE_SINGLE_VIDEO  0 //仅录制视频
#define RECORDER_TYPE_SINGLE_AUDIO  1 //仅录制音频
#define RECORDER_TYPE_AV            2 //同时录制音频和视频,打包成 MP4 文件

class MediaRecorderContext {
public:
    MediaRecorderContext();

    ~MediaRecorderContext();

    static void createContext(JNIEnv *env, jobject instance);

    static void storeContext(JNIEnv *env, jobject instance, MediaRecorderContext *pContext);

    static void deleteContext(JNIEnv *env, jobject instance);

    static MediaRecorderContext *getContext(JNIEnv *env, jobject instance);

    static void renderFrame(void *ctx, NativeImage *nativeImage);

    int init();

    int unInit();

    int startRecord(int recorderType, const char *outUrl, int frameWidth, int frameHeight,
                    long videoBitRate, int fps);

    void onAudioData(uint8_t *pData, int size);

    void previewFrame(int format, uint8_t *pBuffer, int width, int height);

    int stopRecord();

    void
    setTransformMatrix(float translateX, float translateY, float scaleX, float scaleY, int degree,
                       int mirror);

    //OpenGL callback
    void onSurfaceCreated();

    void onSurfaceChanged(int width, int height);

    void onDrawFrame();

    //加载滤镜素材图像
    void setLUTImage(int index, int format, int width, int height, uint8_t *pData);

    //加载着色器脚本
    void setFragShader(int index, char *pShaderStr, int strSize);

private:
    static jfieldID contextHandle;
    TransformMatrix transformMatrix;
    SingleVideoRecorder *singleVideoRecorder = nullptr;
    SingleAudioRecorder *singleAudioRecorder = nullptr;
    MediaRecorder *mediaRecorder = nullptr;
    mutex m_mutex;

};


#endif //OPENGLCAMERA2_BYTEFLOWRENDERCONTEXT_H
