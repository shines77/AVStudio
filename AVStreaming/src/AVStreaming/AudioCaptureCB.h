#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <dshow.h>

class AudioCaptureCB : public ISampleGrabberCB
{
public:
    AudioCaptureCB() : m_refCount(0) {}
    virtual ~AudioCaptureCB() {}

    // IUnknown 接口方法
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
            *ppv = static_cast<ISampleGrabberCB *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ULONG refCount = InterlockedDecrement(&m_refCount);
        if (refCount == 0) {
            delete this;
        }
        return refCount;
    }

    // ISampleGrabberCB 接口方法
    STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample)
    {
        // 在这里处理捕获到的视频帧
        // SampleTime 是帧的时间戳
        // pSample 是视频帧数据

        // 示例：获取帧数据并处理
        BYTE * pBuffer = nullptr;
        pSample->GetPointer(&pBuffer);
        long bufferSize = pSample->GetSize();

        // 你可以在这里对 pBuffer 进行处理
        // 例如保存到文件、显示、分析等

        return S_OK;
    }

    STDMETHODIMP BufferCB(double SampleTime, BYTE * pBuffer, long BufferSize)
    {
        // 这个方法在某些情况下会被调用，而不是 SampleCB
        // 你可以在这里处理捕获到的视频帧数据

        // 示例：处理帧数据
        // pBuffer 是视频帧数据
        // BufferSize 是数据大小

        // 你可以在这里对 pBuffer 进行处理
        // 例如保存到文件、显示、分析等

        return S_OK;
    }

private:
    LONG m_refCount;
};