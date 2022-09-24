/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_MASTER_GLCAMERARENDER_H
#define LEARNFFMPEG_MASTER_GLCAMERARENDER_H

#include <thread>
#include <ImageDef.h>
#include "VideoRender.h"
#include <GLES3/gl3.h>
#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include <vec2.hpp>
#include <render/BaseGLRender.h>
#include <vector>

using namespace glm;
using namespace std;

#define MATH_PI 3.1415926535897932384626433832802
#define TEXTURE_NUM 3

#define SHADER_INDEX_ORIGIN  0
#define SHADER_INDEX_DMESH   1
#define SHADER_INDEX_GHOST   2
#define SHADER_INDEX_CIRCLE  3
#define SHADER_INDEX_ASCII   4
#define SHADER_INDEX_SPLIT   5
#define SHADER_INDEX_MATTE   6
#define SHADER_INDEX_LUT_A   7
#define SHADER_INDEX_LUT_B   8
#define SHADER_INDEX_LUT_C   9
#define SHADER_INDEX_NE      10  //Negative effect

typedef void (*OnRenderFrameCallback)(void *, NativeImage *);

class GLCameraRender : public VideoRender, public BaseGLRender {
public:
    //初始化预览帧的宽高
    virtual void init(int videoWidth, int videoHeight, int *dstSize);

    //渲染一帧视频
    virtual void renderVideoFrame(NativeImage *pImage);

    virtual void unInit();

    //GLSurfaceView 的三个回调
    virtual void onSurfaceCreated();

    virtual void onSurfaceChanged(int w, int h);

    virtual void onDrawFrame();

    static GLCameraRender *getInstance();

    static void releaseInstance();

    //更新变换矩阵，Camera预览帧需要进行旋转
    virtual void updateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY);

    virtual void updateMVPMatrix(TransformMatrix *pTransformMatrix);

    virtual void setTouchLoc(float touchX, float touchY) {
        touchXY.x = touchX / screenSize.x;
        touchXY.y = touchY / screenSize.y;
    }

    //添加好滤镜之后，视频帧的回调，然后将带有滤镜的视频帧放入编码队列
    void setRenderCallback(void *ctx, OnRenderFrameCallback callback) {
        callbackContext = ctx;
        renderFrameCallback = callback;
    }

    //加载滤镜素材图像
    void setLUTImage(int index, NativeImage *pLUTImg);

    //加载 Java 层着色器脚本
    void setFragShaderStr(int index, char *pShaderStr, int strSize);

private:
    GLCameraRender();

    virtual ~GLCameraRender();

    bool createFrameBufferObj();

    void getRenderFrameFromFBO();

    //创建或更新滤镜素材纹理
    void updateExtTexture();

    static std::mutex m_Mutex;
    static GLCameraRender *instance;
    GLuint programID = GL_NONE;
    GLuint fboProgramID = GL_NONE;
    GLuint textureIds[TEXTURE_NUM];
    GLuint vaoId = GL_NONE;
    GLuint vboIds[3];
    GLuint fboTextureId = GL_NONE;
    GLuint fbo = GL_NONE;
    GLuint destFboTextureId = GL_NONE;
    GLuint destFboId = GL_NONE;
    NativeImage renderImage;
    glm::mat4 MVPMatrix;
    TransformMatrix transformMatrix;

    int frameIndex;
    vec2 touchXY;
    vec2 screenSize;

    OnRenderFrameCallback renderFrameCallback = nullptr;
    void *callbackContext = nullptr;

    //支持滑动选择滤镜功能
    volatile bool isShaderChanged = false;
    volatile bool extImageChanged = false;
    char *fragShaderBuffer = nullptr;
    NativeImage extImage;
    GLuint extTextureId = GL_NONE;
    int shaderIndex = 0;
    mutex shaderMutex;

};


#endif //LEARNFFMPEG_MASTER_GLCAMERARENDER_H
