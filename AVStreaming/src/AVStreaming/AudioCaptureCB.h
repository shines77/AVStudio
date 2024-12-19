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

    // IUnknown �ӿڷ���
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

    // ISampleGrabberCB �ӿڷ���
    STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample)
    {
        // �����ﴦ���񵽵���Ƶ֡
        // SampleTime ��֡��ʱ���
        // pSample ����Ƶ֡����

        // ʾ������ȡ֡���ݲ�����
        BYTE * pBuffer = nullptr;
        pSample->GetPointer(&pBuffer);
        long bufferSize = pSample->GetSize();

        // ������������ pBuffer ���д���
        // ���籣�浽�ļ�����ʾ��������

        return S_OK;
    }

    STDMETHODIMP BufferCB(double SampleTime, BYTE * pBuffer, long BufferSize)
    {
        // ���������ĳЩ����»ᱻ���ã������� SampleCB
        // ����������ﴦ���񵽵���Ƶ֡����

        // ʾ��������֡����
        // pBuffer ����Ƶ֡����
        // BufferSize �����ݴ�С

        // ������������ pBuffer ���д���
        // ���籣�浽�ļ�����ʾ��������

        return S_OK;
    }

private:
    LONG m_refCount;
};