/**
 *
 * Created by 公众号：字节流动 on 2021/3/12.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */

//通过定义STB_IMAGE_IMPLEMENTATION，预处理器会修改头文件，让其只包含相关的函数定义源码，等于是将这个头文件变为一个.cpp 文件了,一个项目只需要定义一次
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLCameraRender.h"
#include <GLUtils.h>
#include <gtc/matrix_transform.hpp>
#include "stb_image_write.h"
#include "stb_image.h"

GLCameraRender *GLCameraRender::instance = nullptr;
std::mutex GLCameraRender::m_Mutex;

static char vShaderStr[] =
        "#version 300 es\n"
        "layout(location = 0) in vec4 a_position;\n"
        "layout(location = 1) in vec2 a_texCoord;\n"
        "uniform mat4 u_MVPMatrix;\n"
        "out vec2 v_texCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = u_MVPMatrix * a_position;\n"
        "    v_texCoord = a_texCoord;\n"
        "}";

static char fShaderStr[] =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec2 v_texCoord;\n"
        "layout(location = 0) out vec4 outColor;\n"
        "uniform sampler2D s_texture0;\n"
        "uniform sampler2D s_texture1;\n"
        "uniform sampler2D s_texture2;\n"
        "uniform int u_nImgType;// 1:RGBA, 2:NV21, 3:NV12, 4:I420\n"
        "\n"
        "void main()\n"
        "{\n"
        "\n"
        "    if(u_nImgType == 1) //RGBA\n"
        "    {\n"
        "        outColor = texture(s_texture0, v_texCoord);\n"
        "    }\n"
        "    else if(u_nImgType == 2) //NV21\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).a - 0.5;\n"
        "        yuv.z = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "        0.0, \t-0.344, \t1.770,\n"
        "        1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "\n"
        "    }\n"
        "    else if(u_nImgType == 3) //NV12\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        yuv.z = texture(s_texture1, v_texCoord).a - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "        0.0, \t-0.344, \t1.770,\n"
        "        1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "    }\n"
        "    else if(u_nImgType == 4) //I420\n"
        "    {\n"
        "        vec3 yuv;\n"
        "        yuv.x = texture(s_texture0, v_texCoord).r;\n"
        "        yuv.y = texture(s_texture1, v_texCoord).r - 0.5;\n"
        "        yuv.z = texture(s_texture2, v_texCoord).r - 0.5;\n"
        "        highp vec3 rgb = mat3(1.0,       1.0,     1.0,\n"
        "                              0.0, \t-0.344, \t1.770,\n"
        "                              1.403,  -0.714,     0.0) * yuv;\n"
        "        outColor = vec4(rgb, 1.0);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        outColor = vec4(1.0);\n"
        "    }\n"
        "}";

static GLfloat verticesCoords[] = {
        -1.0f, 1.0f, 0.0f,  // Position 0
        -1.0f, -1.0f, 0.0f,  // Position 1
        1.0f, -1.0f, 0.0f,  // Position 2
        1.0f, 1.0f, 0.0f,  // Position 3
};

//static GLfloat textureCoords[] = {
//        0.0f,  1.0f,        // TexCoord 0
//        0.0f,  0.0f,        // TexCoord 1
//        1.0f,  0.0f,        // TexCoord 2
//        1.0f,  1.0f         // TexCoord 3
//};
static GLfloat textureCoords[] = {
        0.0f, 0.0f,        // TexCoord 0
        0.0f, 1.0f,        // TexCoord 1
        1.0f, 1.0f,        // TexCoord 2
        1.0f, 0.0f         // TexCoord 3
};

static GLushort indices[] = {0, 1, 2, 0, 2, 3};

GLCameraRender::GLCameraRender() : VideoRender(VIDEO_RENDER_OPENGL) {

}

GLCameraRender::~GLCameraRender() {
    NativeImageUtil::FreeNativeImage(&renderImage);

}

//0, 0, nullptr)
void GLCameraRender::init(int videoWidth, int videoHeight, int *dstSize) {
//    LOGCATE("GLCameraRender::InitRender video[w, h]=[%d, %d]", videoWidth, videoHeight);
    if (dstSize != nullptr) {
        dstSize[0] = videoWidth;
        dstSize[1] = videoHeight;
    }
    frameIndex = 0;
    updateMVPMatrix(0, 0, 1.0f, 1.0f);
}

void GLCameraRender::renderVideoFrame(NativeImage *pImage) {
//    LOGCATE("GLCameraRender::RenderVideoFrame pImage=%p", pImage);
    if (pImage == nullptr || pImage->inData[0] == nullptr)
        return;
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (pImage->width != renderImage.width || pImage->height != renderImage.height) {
        if (renderImage.inData[0] != nullptr) {
            NativeImageUtil::FreeNativeImage(&renderImage);
        }
        memset(&renderImage, 0, sizeof(NativeImage));
        renderImage.format = pImage->format;
        renderImage.width = pImage->width;
        renderImage.height = pImage->height;
        NativeImageUtil::AllocNativeImage(&renderImage);
    }

    NativeImageUtil::CopyNativeImage(pImage, &renderImage);
    //NativeImageUtil::DumpNativeImage(&m_RenderImage, "/sdcard", "camera");
}

void GLCameraRender::unInit() {
    NativeImageUtil::FreeNativeImage(&extImage);

    if (fragShaderBuffer != nullptr) {
        free(fragShaderBuffer);
        fragShaderBuffer = nullptr;
    }

}

void GLCameraRender::updateMVPMatrix(int angleX, int angleY, float scaleX, float scaleY) {
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);
    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    //glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(scaleX, scaleY, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    MVPMatrix = Projection * View * Model;

}

void GLCameraRender::updateMVPMatrix(TransformMatrix *pTransformMatrix) {
    //BaseGLRender::UpdateMVPMatrix(pTransformMatrix);
    transformMatrix = *pTransformMatrix;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * pTransformMatrix->angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * pTransformMatrix->angleY);

    float fFactorX = 1.0f;
    float fFactorY = 1.0f;

    if (pTransformMatrix->mirror == 1) {
        fFactorX = -1.0f;
    } else if (pTransformMatrix->mirror == 2) {
        fFactorY = -1.0f;
    }

    float fRotate = MATH_PI * pTransformMatrix->degree * 1.0f / 180;
    if (pTransformMatrix->mirror == 0) {
        if (pTransformMatrix->degree == 270) {
            fRotate = MATH_PI * 0.5;
        } else if (pTransformMatrix->degree == 180) {
            fRotate = MATH_PI;
        } else if (pTransformMatrix->degree == 90) {
            fRotate = MATH_PI * 1.5;
        }
    } else if (pTransformMatrix->mirror == 1) {
        if (pTransformMatrix->degree == 90) {
            fRotate = MATH_PI * 0.5;
        } else if (pTransformMatrix->degree == 180) {
            fRotate = MATH_PI;
        } else if (pTransformMatrix->degree == 270) {
            fRotate = MATH_PI * 1.5;
        }
    }

    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 1), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0) // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(fFactorX * pTransformMatrix->scaleX,
                                        fFactorY * pTransformMatrix->scaleY, 1.0f));
    Model = glm::rotate(Model, fRotate, glm::vec3(0.0f, 0.0f, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model,
                           glm::vec3(pTransformMatrix->translateX, pTransformMatrix->translateY,
                                     0.0f));

//    LOGCATE("GLCameraRender::UpdateMVPMatrix rotate %d,%.2f,%0.5f,%0.5f,%0.5f,%0.5f,",
//            pTransformMatrix->degree, fRotate,
//            pTransformMatrix->translateX, pTransformMatrix->translateY,
//            fFactorX * pTransformMatrix->scaleX, fFactorY * pTransformMatrix->scaleY);

    MVPMatrix = Projection * View * Model;
}

void GLCameraRender::onSurfaceCreated() {
//    LOGCATE("GLCameraRender::OnSurfaceCreated");

    programID = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    fboProgramID = GLUtils::CreateProgram(vShaderStr, fShaderStr);
    if (!programID || !fboProgramID) {
        LOGCATE("GLCameraRender::OnSurfaceCreated create program fail");
        return;
    }

    glGenTextures(TEXTURE_NUM, textureIds);
    for (int i = 0; i < TEXTURE_NUM; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

    // Generate VBO Ids and load the VBOs with data
    glGenBuffers(3, vboIds);
    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Generate VAO Id
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[0]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *) 0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ARRAY_BUFFER, vboIds[1]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *) 0);
    glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIds[2]);

    glBindVertexArray(GL_NONE);

    touchXY = vec2(0.5f, 0.5f);
}

void GLCameraRender::onSurfaceChanged(int w, int h) {
//    LOGCATE("GLCameraRender::OnSurfaceChanged [w, h]=[%d, %d]", w, h);
    screenSize.x = w;
    screenSize.y = h;
    glViewport(0, 0, w, h);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
}

void GLCameraRender::onDrawFrame() {
    if (isShaderChanged) {
        unique_lock<mutex> lock(shaderMutex);
        GLUtils::DeleteProgram(fboProgramID);
        fboProgramID = GLUtils::CreateProgram(vShaderStr, fragShaderBuffer);
        isShaderChanged = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    if (programID == GL_NONE || renderImage.inData[0] == nullptr)
        return;
    if (fbo == GL_NONE && createFrameBufferObj()) {
        LOGCATE("GLCameraRender::OnDrawFrame CreateFrameBufferObj fail");
        return;
    }
//    LOGCATE("GLCameraRender::OnDrawFrame [w, h]=[%d, %d], format=%d screenSize x %d screenSize y %d", renderImage.width,
//            renderImage.height, renderImage.format,screenSize.x,screenSize.y);
    frameIndex++;

    updateExtTexture();

    std::unique_lock<std::mutex> lock(m_Mutex);
    // 渲染到 FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, renderImage.height, renderImage.width); //相机的宽和高反了
    glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(fboProgramID);
    // upload image data

    glBindTexture(GL_TEXTURE_2D, fboTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderImage.height, renderImage.width, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, destFboTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderImage.height, renderImage.width, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);

    switch (renderImage.format) {
        case IMAGE_FORMAT_RGBA:
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderImage.width, renderImage.height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, renderImage.inData[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        case IMAGE_FORMAT_NV21:
        case IMAGE_FORMAT_NV12:
            //upload Y plane data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, renderImage.width,
                         renderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         renderImage.inData[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update UV plane data
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, renderImage.width >> 1,
                         renderImage.height >> 1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                         renderImage.inData[1]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        case IMAGE_FORMAT_I420:
            //upload Y plane data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureIds[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, renderImage.width,
                         renderImage.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         renderImage.inData[0]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update U plane data
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureIds[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, renderImage.width >> 1,
                         renderImage.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         renderImage.inData[1]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);

            //update V plane data
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, textureIds[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, renderImage.width >> 1,
                         renderImage.height >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         renderImage.inData[2]);
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            break;
        default:
            break;
    }

    glBindVertexArray(vaoId);
    updateMVPMatrix(&transformMatrix);
    GLUtils::setMat4(fboProgramID, "u_MVPMatrix", MVPMatrix);
    for (int i = 0; i < TEXTURE_NUM; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        char samplerName[64] = {0};
        sprintf(samplerName, "s_texture%d", i);
        GLUtils::setInt(fboProgramID, samplerName, i);
    }
    float offset = (sin(frameIndex * MATH_PI / 40) + 1.0f) / 2.0f;
    GLUtils::setFloat(fboProgramID, "u_Offset", offset);
    GLUtils::setVec2(fboProgramID, "u_TexSize", vec2(renderImage.width, renderImage.height));
    GLUtils::setInt(fboProgramID, "u_nImgType", renderImage.format);

    switch (shaderIndex) {
        case SHADER_INDEX_ORIGIN:
            break;
        case SHADER_INDEX_DMESH:
            break;
        case SHADER_INDEX_GHOST:
            offset = frameIndex % 60 / 60.0f - 0.2f;
            if (offset < 0) offset = 0;
            GLUtils::setFloat(fboProgramID, "u_Offset", offset);
            break;
        case SHADER_INDEX_CIRCLE:
            break;
        case SHADER_INDEX_ASCII:
            glActiveTexture(GL_TEXTURE0 + TEXTURE_NUM);
            glBindTexture(GL_TEXTURE_2D, extTextureId);
            GLUtils::setInt(fboProgramID, "s_textureMapping", TEXTURE_NUM);
            GLUtils::setVec2(fboProgramID, "asciiTexSize",
                             vec2(extImage.width, extImage.height));
            break;
        case SHADER_INDEX_LUT_A:
        case SHADER_INDEX_LUT_B:
        case SHADER_INDEX_LUT_C:
            glActiveTexture(GL_TEXTURE0 + TEXTURE_NUM);
            glBindTexture(GL_TEXTURE_2D, extTextureId);
            GLUtils::setInt(fboProgramID, "s_LutTexture", TEXTURE_NUM);
            break;
        case SHADER_INDEX_NE:
            offset = (sin(frameIndex * MATH_PI / 60) + 1.0f) / 2.0f;
            GLUtils::setFloat(fboProgramID, "u_Offset", offset);
            break;
        default:
            break;
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *) 0);

    //再绘制一次，把方向倒过来
    glBindFramebuffer(GL_FRAMEBUFFER, destFboId);
    glViewport(0, 0, renderImage.height, renderImage.width); //相机的宽和高反了,
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(programID);
    glBindVertexArray(vaoId);

    updateMVPMatrix(0, 0, 1.0, 1.0);
    GLUtils::setMat4(programID, "u_MVPMatrix", MVPMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTextureId);
    GLUtils::setInt(programID, "s_texture0", 0);
    GLUtils::setInt(programID, "u_nImgType", IMAGE_FORMAT_RGBA);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *) 0);

    getRenderFrameFromFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    lock.unlock();

    // 渲染到屏幕
    glViewport(0, 0, screenSize.x, screenSize.y);

    glClear(GL_COLOR_BUFFER_BIT);

    updateMVPMatrix(0, 0, 1.0, 1.0);
    GLUtils::setMat4(programID, "u_MVPMatrix", MVPMatrix);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, destFboTextureId);
    GLUtils::setInt(programID, "s_texture0", 0);

    GLUtils::setInt(programID, "u_nImgType", IMAGE_FORMAT_RGBA);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *) 0);
}

GLCameraRender *GLCameraRender::getInstance() {
    if (instance == nullptr) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (instance == nullptr) {
            instance = new GLCameraRender();
        }
    }
    return instance;
}

void GLCameraRender::releaseInstance() {
    if (instance != nullptr) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (instance != nullptr) {
            delete instance;
            instance = nullptr;
        }

    }
}

bool GLCameraRender::createFrameBufferObj() {
    // 创建并初始化 FBO 纹理
    if (fboTextureId == GL_NONE) {
        glGenTextures(1, &fboTextureId);
        glBindTexture(GL_TEXTURE_2D, fboTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

    if (destFboTextureId == GL_NONE) {
        glGenTextures(1, &destFboTextureId);
        glBindTexture(GL_TEXTURE_2D, destFboTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }

    // 创建并初始化 FBO
    if (fbo == GL_NONE) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glBindTexture(GL_TEXTURE_2D, fboTextureId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               fboTextureId, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderImage.height, renderImage.width, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
//            LOGCATE("GLCameraRender::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
            if (fboTextureId != GL_NONE) {
                glDeleteTextures(1, &fboTextureId);
                fboTextureId = GL_NONE;
            }

            if (fbo != GL_NONE) {
                glDeleteFramebuffers(1, &fbo);
                fbo = GL_NONE;
            }
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
            return false;
        }
    }

    if (destFboId == GL_NONE) {
        glGenFramebuffers(1, &destFboId);
        glBindFramebuffer(GL_FRAMEBUFFER, destFboId);
        glBindTexture(GL_TEXTURE_2D, destFboTextureId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               destFboTextureId, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, renderImage.height, renderImage.width, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
//            LOGCATE("GLCameraRender::CreateFrameBufferObj glCheckFramebufferStatus status != GL_FRAMEBUFFER_COMPLETE");
            if (destFboTextureId != GL_NONE) {
                glDeleteTextures(1, &destFboTextureId);
                destFboTextureId = GL_NONE;
            }

            if (destFboId != GL_NONE) {
                glDeleteFramebuffers(1, &destFboId);
                destFboId = GL_NONE;
            }
            glBindTexture(GL_TEXTURE_2D, GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
            return false;
        }
    }

    glBindTexture(GL_TEXTURE_2D, GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
    return true;
}

static int index = 0;

void GLCameraRender::getRenderFrameFromFBO() {
//    LOGCATE("GLCameraRender::GetRenderFrameFromFBO m_RenderFrameCallback=%p",
//            renderFrameCallback);
    if (renderFrameCallback != nullptr) {
        uint8_t *pBuffer = new uint8_t[renderImage.width * renderImage.height * 4];
        NativeImage nativeImage = renderImage;
        nativeImage.format = IMAGE_FORMAT_RGBA;
        nativeImage.width = renderImage.height;
        nativeImage.height = renderImage.width;
        nativeImage.inSize[0] = nativeImage.width * 4;
        nativeImage.inData[0] = pBuffer;
        glReadPixels(0, 0, nativeImage.width, nativeImage.height, GL_RGBA, GL_UNSIGNED_BYTE,
                     pBuffer);
        renderFrameCallback(callbackContext, &nativeImage);

//        LOGCATE("GetRenderFrameFromFBO nativeImage.width %d nativeImage.height %d", nativeImage.width, nativeImage.height);

//        char path[256];
//        sprintf(path, "/sdcard/capture%d.png", ((index++) % 6));
//        int ret =  stbi_write_jpg(path, nativeImage.width, nativeImage.height, 3,pBuffer,0);
//        int ret = stbi_write_png(path, nativeImage.width, nativeImage.height, 4,
//                                 pBuffer, nativeImage.width * 4);
//        LOGCATE("write %s ret %d width %d.height %d ", path, ret, nativeImage.width,
//                nativeImage.height);
        delete[]pBuffer;
    }
}

void GLCameraRender::setFragShaderStr(int index, char *pShaderStr, int strSize) {
//    LOGCATE("GLByteFlowRender::LoadFragShaderScript pShaderStr = %p, shaderIndex=%d", pShaderStr,
//            index);
    if (shaderIndex != index) {
        unique_lock<mutex> lock(shaderMutex);
        if (fragShaderBuffer != nullptr) {
            free(fragShaderBuffer);
            fragShaderBuffer = nullptr;
        }
        shaderIndex = index;
        fragShaderBuffer = static_cast<char *>(malloc(strSize));
        memcpy(fragShaderBuffer, pShaderStr, strSize);
        isShaderChanged = true;
    }

}

void GLCameraRender::setLUTImage(int index, NativeImage *pLUTImg) {
//    LOGCATE("GLCameraRender::SetLUTImage pImage = %p, index=%d", pLUTImg->inData[0],
//            index);
    unique_lock<mutex> lock(m_Mutex);
    NativeImageUtil::FreeNativeImage(&extImage);
    extImage.width = pLUTImg->width;
    extImage.height = pLUTImg->height;
    extImage.format = pLUTImg->format;
    NativeImageUtil::AllocNativeImage(&extImage);
    NativeImageUtil::CopyNativeImage(pLUTImg, &extImage);
    extImageChanged = true;
}

void GLCameraRender::updateExtTexture() {
//    LOGCATE("GLCameraRender::UpdateExtTexture");
    if (extImageChanged && extImage.inData[0] != nullptr) {
        if (extTextureId != GL_NONE) {
            glDeleteTextures(1, &extTextureId);
        }
        glGenTextures(1, &extTextureId);
        glBindTexture(GL_TEXTURE_2D, extTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, extImage.width, extImage.height, 0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE, extImage.inData[0]);
        extImageChanged = false;
    }
}



