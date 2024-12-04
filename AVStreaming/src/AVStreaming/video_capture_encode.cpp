
#include <windows.h>
#include <dshow.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#pragma comment(lib, "strmbase.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

// DirectShow捕捉视频的回调函数
class VideoCaptureCallback : public ISampleGrabberCB {
public:
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
            *ppv = static_cast<ISampleGrabberCB*>(this);
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP SampleCB(double SampleTime, IMediaSample* pSample) {
        return 0;
    }

    STDMETHODIMP BufferCB(double SampleTime, BYTE* pBuffer, long BufferLen) {
        // 在这里处理捕捉到的视频帧
        // 你可以将帧数据传递给FFmpeg进行编码
        return 0;
    }
};

// 初始化DirectShow捕捉
void InitializeDirectShowCapture(IBaseFilter** ppCapFilter) {
    HRESULT hr;
    IGraphBuilder* pGraph = NULL;
    ICaptureGraphBuilder2* pBuild = NULL;
    IMediaControl* pControl = NULL;
    ISampleGrabber* pGrabber = NULL;
    IBaseFilter* pGrabberFilter = NULL;
    VideoCaptureCallback* pCallback = new VideoCaptureCallback();

    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);
    if (FAILED(hr)) {
        std::cerr << "Failed to create Filter Graph" << std::endl;
        return;
    }

    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuild);
    if (FAILED(hr)) {
        std::cerr << "Failed to create Capture Graph Builder" << std::endl;
        return;
    }

    hr = pBuild->SetFiltergraph(pGraph);
    if (FAILED(hr)) {
        std::cerr << "Failed to set Filter Graph" << std::endl;
        return;
    }

    hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
    if (FAILED(hr)) {
        std::cerr << "Failed to get Media Control" << std::endl;
        return;
    }

    hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&pGrabberFilter);
    if (FAILED(hr)) {
        std::cerr << "Failed to create Sample Grabber" << std::endl;
        return;
    }

    hr = pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&pGrabber);
    if (FAILED(hr)) {
        std::cerr << "Failed to get Sample Grabber interface" << std::endl;
        return;
    }

    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;

    hr = pGrabber->SetMediaType(&mt);
    if (FAILED(hr)) {
        std::cerr << "Failed to set media type" << std::endl;
        return;
    }

    hr = pGraph->AddFilter(pGrabberFilter, L"Sample Grabber");
    if (FAILED(hr)) {
        std::cerr << "Failed to add Sample Grabber to graph" << std::endl;
        return;
    }

    hr = pGrabber->SetCallback(pCallback, 1);
    if (FAILED(hr)) {
        std::cerr << "Failed to set callback" << std::endl;
        return;
    }

    hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, *ppCapFilter, IID_ISampleGrabber, (void**)&pGrabber);
    if (FAILED(hr)) {
        std::cerr << "Failed to find capture interface" << std::endl;
        return;
    }

    hr = pControl->Run();
    if (FAILED(hr)) {
        std::cerr << "Failed to run graph" << std::endl;
        return;
    }
}

// 使用FFmpeg编码视频
void EncodeVideoFrame(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt, FILE* output_file) {
    int ret;

    // 发送帧到编码器
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        std::cerr << "Error sending a frame for encoding" << std::endl;
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            std::cerr << "Error during encoding" << std::endl;
            return;
        }

        // 将编码后的数据写入文件
        fwrite(pkt->data, 1, pkt->size, output_file);
        av_packet_unref(pkt);
    }
}

int main() {
    CoInitialize(NULL);

    IBaseFilter* pCapFilter = NULL;
    InitializeDirectShowCapture(&pCapFilter);

    // 初始化FFmpeg
    av_register_all();
    avcodec_register_all();

    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "H.264 codec not found" << std::endl;
        return -1;
    }

    AVCodecContext* c = avcodec_alloc_context3(codec);
    if (!c) {
        std::cerr << "Could not allocate video codec context" << std::endl;
        return -1;
    }

    c->bit_rate = 400000;
    c->width = 640;
    c->height = 480;
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_open2(c, codec, NULL) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate video frame" << std::endl;
        return -1;
    }

    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;

    int ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        std::cerr << "Could not allocate the video frame data" << std::endl;
        return -1;
    }

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Could not allocate packet" << std::endl;
        return -1;
    }

    FILE* output_file = fopen("output.h264", "wb");
    if (!output_file) {
        std::cerr << "Could not open output file" << std::endl;
        return -1;
    }

    // 模拟捕捉和编码过程
    for (int i = 0; i < 100; i++) {
        av_frame_make_writable(frame);

        // 假设我们从DirectShow捕捉到一帧数据
        // 这里我们简单地填充一些测试数据
        for (int y = 0; y < c->height; y++) {
            for (int x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        for (int y = 0; y < c->height / 2; y++) {
            for (int x = 0; x < c->width / 2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;

        // 编码帧
        EncodeVideoFrame(c, frame, pkt, output_file);
    }

    // 刷新编码器
    EncodeVideoFrame(c, NULL, pkt, output_file);

    fclose(output_file);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    CoUninitialize();
    return 0;
}
