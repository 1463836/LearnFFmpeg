//
// Created by 字节流动 on 2020/6/19.
//

#ifndef LEARNFFMPEG_VIDEORENDER_H
#define LEARNFFMPEG_VIDEORENDER_H

#define VIDEO_RENDER_OPENGL             0
#define VIDEO_RENDER_ANWINDOW           1
#define VIDEO_RENDER_3D_VR              2

#include "ImageDef.h"

class VideoRender {
public:
    VideoRender(int type) {
        renderType = type;
    }

    virtual ~VideoRender() {}

    virtual void init(int videoWidth, int videoHeight, int *dstSize) = 0;

    virtual void renderVideoFrame(NativeImage *pImage) = 0;

    virtual void unInit() = 0;

    int getRenderType() {
        return renderType;
    }

private:
    int renderType = VIDEO_RENDER_ANWINDOW;
};

#endif //LEARNFFMPEG_VIDEORENDER_H
