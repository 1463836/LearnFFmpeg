/**
 *
 * Created by 公众号：字节流动 on 2021/3/16.
 * https://github.com/githubhaohao/LearnFFmpeg
 * 最新文章首发于公众号：字节流动，有疑问或者技术交流可以添加微信 Byte-Flow ,领取视频教程, 拉你进技术交流群
 *
 * */


#include <LogUtil.h>
#include <unistd.h>
#include "OpenSLRender.h"

void OpenSLRender::Init() {
//    LOGCATE("OpenSLRender::Init");

    int result = -1;
    do {
        result = CreateEngine();
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::Init CreateEngine fail. result=%d", result);
            break;
        }

        result = CreateOutputMixer();
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::Init CreateOutputMixer fail. result=%d", result);
            break;
        }

        result = CreateAudioPlayer();
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::Init CreateAudioPlayer fail. result=%d", result);
            break;
        }

        m_thread = new std::thread(CreateSLWaitingThread, this);

    } while (false);

    if (result != SL_RESULT_SUCCESS) {
        LOGCATE("OpenSLRender::Init fail. result=%d", result);
        UnInit();
    }

}

void OpenSLRender::RenderAudioFrame(uint8_t *pData, int dataSize) {
//    LOGCATE("OpenSLRender::RenderAudioFrame pData=%p, dataSize=%d", pData, dataSize);
    if (audioPlayerInterface) {
        if (pData != nullptr && dataSize > 0) {

            //temp resolution, when queue size is too big.
            while (GetAudioFrameQueueSize() >= MAX_QUEUE_BUFFER_SIZE && !m_Exit) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }

            std::unique_lock<std::mutex> lock(m_Mutex);
            AudioFrame *audioFrame = new AudioFrame(pData, dataSize);
            m_AudioFrameQueue.push(audioFrame);
            m_Cond.notify_all();
            lock.unlock();
        }
    }

}

void OpenSLRender::UnInit() {
//    LOGCATE("OpenSLRender::UnInit");

    if (audioPlayerInterface) {
        (*audioPlayerInterface)->SetPlayState(audioPlayerInterface, SL_PLAYSTATE_STOPPED);
        audioPlayerInterface = nullptr;
    }

    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Exit = true;
    m_Cond.notify_all();
    lock.unlock();

    if (audioPlayerObj) {
        (*audioPlayerObj)->Destroy(audioPlayerObj);
        audioPlayerObj = nullptr;
        m_BufferQueue = nullptr;
    }

    if (outputMixObj) {
        (*outputMixObj)->Destroy(outputMixObj);
        outputMixObj = nullptr;
    }

    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineInterface = nullptr;
    }

    lock.lock();
    for (int i = 0; i < m_AudioFrameQueue.size(); ++i) {
        AudioFrame *audioFrame = m_AudioFrameQueue.front();
        m_AudioFrameQueue.pop();
        delete audioFrame;
    }
    lock.unlock();

    if (m_thread != nullptr) {
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }

    AudioGLRender::ReleaseInstance();

}

int OpenSLRender::CreateEngine() {
    SLresult result = SL_RESULT_SUCCESS;
    do {
        result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateEngine slCreateEngine fail. result=%d", result);
            break;
        }

        result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateEngine Realize fail. result=%d", result);
            break;
        }

        result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateEngine GetInterface fail. result=%d", result);
            break;
        }

    } while (false);
    return result;
}

int OpenSLRender::CreateOutputMixer() {
    SLresult result = SL_RESULT_SUCCESS;
    do {
        const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
        const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};

        result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObj, 1, mids, mreq);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateOutputMixer CreateOutputMix fail. result=%d", result);
            break;
        }

        result = (*outputMixObj)->Realize(outputMixObj, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateOutputMixer CreateOutputMix fail. result=%d", result);
            break;
        }

    } while (false);

    return result;
}

int OpenSLRender::CreateAudioPlayer() {
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//format type
            (SLuint32) 2,//channel count
            SL_SAMPLINGRATE_44_1,//44100hz
            SL_PCMSAMPLEFORMAT_FIXED_16,// bits per sample
            SL_PCMSAMPLEFORMAT_FIXED_16,// container size
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,// channel mask
            SL_BYTEORDER_LITTLEENDIAN // endianness
    };
    SLDataSource slDataSource = {&android_queue, &pcm};

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObj};
    SLDataSink slDataSink = {&outputMix, nullptr};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    SLresult result;

    do {

        result = (*engineInterface)->CreateAudioPlayer(engineInterface, &audioPlayerObj,
                                                       &slDataSource, &slDataSink, 3, ids, req);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateAudioPlayer CreateAudioPlayer fail. result=%d", result);
            break;
        }

        result = (*audioPlayerObj)->Realize(audioPlayerObj, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateAudioPlayer Realize fail. result=%d", result);
            break;
        }

        result = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_PLAY,
                                                 &audioPlayerInterface);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateAudioPlayer GetInterface fail. result=%d", result);
            break;
        }

        result = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_BUFFERQUEUE,
                                                 &m_BufferQueue);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateAudioPlayer GetInterface fail. result=%d", result);
            break;
        }

        result = (*m_BufferQueue)->RegisterCallback(m_BufferQueue, AudioPlayerCallback, this);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateAudioPlayer RegisterCallback fail. result=%d", result);
            break;
        }

        result = (*audioPlayerObj)->GetInterface(audioPlayerObj, SL_IID_VOLUME,
                                                 &audioPlayerVolumeInterface);
        if (result != SL_RESULT_SUCCESS) {
            LOGCATE("OpenSLRender::CreateAudioPlayer GetInterface fail. result=%d", result);
            break;
        }

    } while (false);

    return result;
}

void OpenSLRender::StartRender() {

    while (GetAudioFrameQueueSize() < MAX_QUEUE_BUFFER_SIZE && !m_Exit) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Cond.wait_for(lock, std::chrono::milliseconds(10));
        //m_Cond.wait(lock);
        lock.unlock();
    }

    (*audioPlayerInterface)->SetPlayState(audioPlayerInterface, SL_PLAYSTATE_PLAYING);
    AudioPlayerCallback(m_BufferQueue, this);
}

void OpenSLRender::HandleAudioFrameQueue() {
//    LOGCATE("OpenSLRender::HandleAudioFrameQueue QueueSize=%lu", m_AudioFrameQueue.size());
    if (audioPlayerInterface == nullptr) return;

    while (GetAudioFrameQueueSize() < MAX_QUEUE_BUFFER_SIZE && !m_Exit) {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_Cond.wait_for(lock, std::chrono::milliseconds(10));
    }

    std::unique_lock<std::mutex> lock(m_Mutex);
    AudioFrame *audioFrame = m_AudioFrameQueue.front();
    if (nullptr != audioFrame && audioPlayerInterface) {
        SLresult result = (*m_BufferQueue)->Enqueue(m_BufferQueue, audioFrame->data,
                                                    (SLuint32) audioFrame->dataSize);
        if (result == SL_RESULT_SUCCESS) {
            AudioGLRender::getInstance()->UpdateAudioFrame(audioFrame);
            m_AudioFrameQueue.pop();
            delete audioFrame;
        }

    }
    lock.unlock();

}

void OpenSLRender::CreateSLWaitingThread(OpenSLRender *openSlRender) {
    openSlRender->StartRender();
}

void OpenSLRender::AudioPlayerCallback(SLAndroidSimpleBufferQueueItf bufferQueue, void *context) {
    OpenSLRender *openSlRender = static_cast<OpenSLRender *>(context);
    openSlRender->HandleAudioFrameQueue();
}

int OpenSLRender::GetAudioFrameQueueSize() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    return m_AudioFrameQueue.size();
}

void OpenSLRender::ClearAudioCache() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    for (int i = 0; i < m_AudioFrameQueue.size(); ++i) {
        AudioFrame *audioFrame = m_AudioFrameQueue.front();
        m_AudioFrameQueue.pop();
        delete audioFrame;
    }

}
