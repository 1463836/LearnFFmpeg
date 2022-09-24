//
// Created by 公众号：字节流动 on 2019/7/10.
//

#ifndef NDK_OPENGLES_3_0_IMAGEDEF_H
#define NDK_OPENGLES_3_0_IMAGEDEF_H

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "stdio.h"
#include "sys/stat.h"
#include "stdint.h"
#include "LogUtil.h"

#define IMAGE_FORMAT_RGBA           0x01
#define IMAGE_FORMAT_NV21           0x02
#define IMAGE_FORMAT_NV12           0x03
#define IMAGE_FORMAT_I420           0x04

#define IMAGE_FORMAT_RGBA_EXT       "RGB32"
#define IMAGE_FORMAT_NV21_EXT       "NV21"
#define IMAGE_FORMAT_NV12_EXT       "NV12"
#define IMAGE_FORMAT_I420_EXT       "I420"

typedef struct _tag_NativeRectF {
    float left;
    float top;
    float right;
    float bottom;

    _tag_NativeRectF() {
        left = top = right = bottom = 0.0f;
    }
} RectF;

typedef struct _tag_NativeImage {
    int width;
    int height;
    int format;
    uint8_t *inData[3];
    int inSize[3];

    _tag_NativeImage() {
        width = 0;
        height = 0;
        format = 0;
        inData[0] = nullptr;
        inData[1] = nullptr;
        inData[2] = nullptr;
        inSize[0] = 0;
        inSize[1] = 0;
        inSize[2] = 0;
    }
} NativeImage;

class NativeImageUtil {
public:
    static void AllocNativeImage(NativeImage *nativeImage) {
        if (nativeImage->height == 0 || nativeImage->width == 0) return;

//        nativeImage.format = IMAGE_FORMAT_RGBA;
        switch (nativeImage->format) {
            case IMAGE_FORMAT_I420: {
                nativeImage->inData[0] = static_cast<uint8_t *>(malloc(
                        nativeImage->width * nativeImage->height * 1.5));
                nativeImage->inData[1] =
                        nativeImage->inData[0] + nativeImage->width * nativeImage->height;
                nativeImage->inData[2] =
                        nativeImage->inData[1] +
                        (nativeImage->width >> 1) * (nativeImage->height >> 1);
                nativeImage->inSize[0] = nativeImage->width;
                nativeImage->inSize[1] = nativeImage->width / 2;
                nativeImage->inSize[2] = nativeImage->width / 2;
            }
                break;
            case IMAGE_FORMAT_NV12:
            case IMAGE_FORMAT_NV21: {
                nativeImage->inData[0] = static_cast<uint8_t *>(malloc(
                        nativeImage->width * nativeImage->height * 1.5));
                nativeImage->inData[1] =
                        nativeImage->inData[0] + nativeImage->width * nativeImage->height;
                nativeImage->inSize[0] = nativeImage->width;
                nativeImage->inSize[1] = nativeImage->width;
                nativeImage->inSize[2] = 0;
            }
                break;
            case IMAGE_FORMAT_RGBA: {
                nativeImage->inData[0] = static_cast<uint8_t *>(malloc(
                        nativeImage->width * nativeImage->height * 4));
                nativeImage->inSize[0] = nativeImage->width * 4;
                nativeImage->inSize[1] = 0;
                nativeImage->inSize[2] = 0;
            }
                break;
            default:
                LOGCATE("NativeImageUtil::AllocNativeImage do not support the format. Format = %d",
                        nativeImage->format);
                break;
        }
    }

    static void FreeNativeImage(NativeImage *pImage) {
        if (pImage == nullptr || pImage->inData[0] == nullptr) return;

        free(pImage->inData[0]);
        pImage->inData[0] = nullptr;
        pImage->inData[1] = nullptr;
        pImage->inData[2] = nullptr;
    }

    static void CopyNativeImage(NativeImage *pSrcImg, NativeImage *pDstImg) {
//        LOGCATE("NativeImageUtil::CopyNativeImage src[w,h,format]=[%d, %d, %d], dst[w,h,format]=[%d, %d, %d]",
//                pSrcImg->width, pSrcImg->height, pSrcImg->format, pDstImg->width, pDstImg->height,
//                pDstImg->format);
//        LOGCATE("NativeImageUtil::CopyNativeImage src[line0,line1,line2]=[%d, %d, %d], dst[line0,line1,line2]=[%d, %d, %d]",
//                pSrcImg->inSize[0], pSrcImg->inSize[1], pSrcImg->inSize[2],
//                pDstImg->inSize[0], pDstImg->inSize[1], pDstImg->inSize[2]);

        if (pSrcImg == nullptr || pSrcImg->inData[0] == nullptr) return;

        if (pSrcImg->format != pDstImg->format ||
            pSrcImg->width != pDstImg->width ||
            pSrcImg->height != pDstImg->height) {
            LOGCATE("NativeImageUtil::CopyNativeImage invalid params.");
            return;
        }

        if (pDstImg->inData[0] == nullptr) AllocNativeImage(pDstImg);

        switch (pSrcImg->format) {
            case IMAGE_FORMAT_I420: {
                // y plane
                if (pSrcImg->inSize[0] != pDstImg->inSize[0]) {
                    for (int i = 0; i < pSrcImg->height; ++i) {
                        memcpy(pDstImg->inData[0] + i * pDstImg->inSize[0],
                               pSrcImg->inData[0] + i * pSrcImg->inSize[0], pDstImg->width);
                    }
                } else {
                    memcpy(pDstImg->inData[0], pSrcImg->inData[0],
                           pDstImg->inSize[0] * pSrcImg->height);
                }

                // u plane
                if (pSrcImg->inSize[1] != pDstImg->inSize[1]) {
                    for (int i = 0; i < pSrcImg->height / 2; ++i) {
                        memcpy(pDstImg->inData[1] + i * pDstImg->inSize[1],
                               pSrcImg->inData[1] + i * pSrcImg->inSize[1], pDstImg->width / 2);
                    }
                } else {
                    memcpy(pDstImg->inData[1], pSrcImg->inData[1],
                           pDstImg->inSize[1] * pSrcImg->height / 2);
                }

                // v plane
                if (pSrcImg->inSize[2] != pDstImg->inSize[2]) {
                    for (int i = 0; i < pSrcImg->height / 2; ++i) {
                        memcpy(pDstImg->inData[2] + i * pDstImg->inSize[2],
                               pSrcImg->inData[2] + i * pSrcImg->inSize[2], pDstImg->width / 2);
                    }
                } else {
                    memcpy(pDstImg->inData[2], pSrcImg->inData[2],
                           pDstImg->inSize[2] * pSrcImg->height / 2);
                }
            }

                LOGCATE("in size %d %d %d ", pSrcImg->inSize[0], pSrcImg->inSize[1],
                        pSrcImg->inSize[2]);

                break;
            case IMAGE_FORMAT_NV21:
            case IMAGE_FORMAT_NV12: {
                // y plane
                if (pSrcImg->inSize[0] != pDstImg->inSize[0]) {
                    for (int i = 0; i < pSrcImg->height; ++i) {
                        memcpy(pDstImg->inData[0] + i * pDstImg->inSize[0],
                               pSrcImg->inData[0] + i * pSrcImg->inSize[0], pDstImg->width);
                    }
                } else {
                    memcpy(pDstImg->inData[0], pSrcImg->inData[0],
                           pDstImg->inSize[0] * pSrcImg->height);
                }

                // uv plane
                if (pSrcImg->inSize[1] != pDstImg->inSize[1]) {
                    for (int i = 0; i < pSrcImg->height / 2; ++i) {
                        memcpy(pDstImg->inData[1] + i * pDstImg->inSize[1],
                               pSrcImg->inData[1] + i * pSrcImg->inSize[1], pDstImg->width);
                    }
                } else {
                    memcpy(pDstImg->inData[1], pSrcImg->inData[1],
                           pDstImg->inSize[1] * pSrcImg->height / 2);
                }
            }
                break;
            case IMAGE_FORMAT_RGBA: {
                if (pSrcImg->inSize[0] != pDstImg->inSize[0]) {
                    for (int i = 0; i < pSrcImg->height; ++i) {
                        memcpy(pDstImg->inData[0] + i * pDstImg->inSize[0],
                               pSrcImg->inData[0] + i * pSrcImg->inSize[0], pDstImg->width * 4);
                    }
                } else {
                    memcpy(pDstImg->inData[0], pSrcImg->inData[0],
                           pSrcImg->inSize[0] * pSrcImg->height);
                }
            }
                break;
            default: {
                LOGCATE("NativeImageUtil::CopyNativeImage do not support the format. Format = %d",
                        pSrcImg->format);
            }
                break;
        }

    }

    static void DumpNativeImage(NativeImage *pSrcImg, const char *pPath, const char *pFileName) {
        if (pSrcImg == nullptr || pPath == nullptr || pFileName == nullptr) return;

        if (access(pPath, 0) == -1) {
            mkdir(pPath, 0666);
        }

        char imgPath[256] = {0};
        const char *pExt = nullptr;
        switch (pSrcImg->format) {
            case IMAGE_FORMAT_I420:
                pExt = IMAGE_FORMAT_I420_EXT;
                break;
            case IMAGE_FORMAT_NV12:
                pExt = IMAGE_FORMAT_NV12_EXT;
                break;
            case IMAGE_FORMAT_NV21:
                pExt = IMAGE_FORMAT_NV21_EXT;
                break;
            case IMAGE_FORMAT_RGBA:
                pExt = IMAGE_FORMAT_RGBA_EXT;
                break;
            default:
                pExt = "Default";
                break;
        }

        static int index = 0;
        sprintf(imgPath, "%s/IMG_%dx%d_%s_%d.%s", pPath, pSrcImg->width, pSrcImg->height, pFileName,
                index, pExt);

        FILE *fp = fopen(imgPath, "wb");

//        LOGCATE("DumpNativeImage fp=%p, file=%s", fp, imgPath);

        if (fp) {
            switch (pSrcImg->format) {
                case IMAGE_FORMAT_I420: {
                    fwrite(pSrcImg->inData[0],
                           static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
                    fwrite(pSrcImg->inData[1],
                           static_cast<size_t>((pSrcImg->width >> 1) * (pSrcImg->height >> 1)), 1,
                           fp);
                    fwrite(pSrcImg->inData[2],
                           static_cast<size_t>((pSrcImg->width >> 1) * (pSrcImg->height >> 1)), 1,
                           fp);
                    break;
                }
                case IMAGE_FORMAT_NV21:
                case IMAGE_FORMAT_NV12: {
                    fwrite(pSrcImg->inData[0],
                           static_cast<size_t>(pSrcImg->width * pSrcImg->height), 1, fp);
                    fwrite(pSrcImg->inData[1],
                           static_cast<size_t>(pSrcImg->width * (pSrcImg->height >> 1)), 1, fp);
                    break;
                }
                case IMAGE_FORMAT_RGBA: {
                    fwrite(pSrcImg->inData[0],
                           static_cast<size_t>(pSrcImg->width * pSrcImg->height * 4), 1, fp);
                    break;
                }
                default: {
                    LOGCATE("DumpNativeImage default");
                    break;
                }
            }

            fclose(fp);
            fp = NULL;
        }


    }
};


#endif //NDK_OPENGLES_3_0_IMAGEDEF_H
