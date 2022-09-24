/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

#ifndef LEARNFFMPEG_MASTER_GLRENDER_H
#define LEARNFFMPEG_MASTER_GLRENDER_H

#include <thread>
#include <ImageDef.h>
#include "VideoRender.h"
#include <GLES3/gl3.h>
#include <detail/type_mat.hpp>
#include <detail/type_mat4x4.hpp>
#include <vec2.hpp>
#include <render/BaseGLRender.h>

using namespace glm;

#define MATH_PI 3.1415926535897932384626433832802
#define TEXTURE_NUM 3

class VideoGLRender : public VideoRender, public BaseGLRender {
public:
    virtual void init(int videoWidth, int videoHeight, int *dstSize);

    virtual void renderVideoFrame(NativeImage *pImage);

    virtual void unInit();

    virtual void onSurfaceCreated();

    virtual void onSurfaceChanged(int w, int h);

    virtual void onDrawFrame();

    static VideoGLRender *getInstance();

    static void releaseInstance();

    virtual void updateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY);

    virtual void updateMVPMatrix(TransformMatrix *pTransformMatrix);

    virtual void setTouchLoc(float touchX, float touchY) {
        m_TouchXY.x = touchX / screenSize.x;
        m_TouchXY.y = touchY / screenSize.y;
    }

private:
    VideoGLRender();

    virtual ~VideoGLRender();

    static std::mutex m_Mutex;
    static VideoGLRender *instance;
    GLuint programObj = GL_NONE;
    GLuint textureIds[TEXTURE_NUM];
    GLuint vaoId;
    GLuint vboIds[3];
    NativeImage renderImage;
    glm::mat4 MVPMatrix;

    int frameIndex;
    vec2 m_TouchXY;
    vec2 screenSize;
};


#endif //LEARNFFMPEG_MASTER_GLRENDER_H
