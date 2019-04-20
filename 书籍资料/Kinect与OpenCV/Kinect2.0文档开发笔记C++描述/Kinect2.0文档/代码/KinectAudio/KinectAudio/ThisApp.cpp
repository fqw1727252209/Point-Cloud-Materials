#include "stdafx.h"
#include "included.h"

#define TITLE L"Title"
#define WNDWIDTH 1024
#define WNDHEIGHT 768

#define lengthof(a) (sizeof(a)/sizeof(*a))

// ThisApp构造函数
ThisApp::ThisApp(){
    m_pAudioBuffer = new BYTE[64 * 1024 *1024];
    if (SUCCEEDED(init_kinect())){
        m_threadAudio.std::thread::~thread();
        m_threadAudio.std::thread::thread(AudioThread, this);
    }
}

// ThisApp析构函数
ThisApp::~ThisApp(){
    // 销毁事件
    if (m_hAudioBeamFrameArrived && m_pAudioBeamFrameReader){
        m_pAudioBeamFrameReader->UnsubscribeFrameArrived(m_hAudioBeamFrameArrived);
        m_hAudioBeamFrameArrived = 0;
    }
    // 释放AudioBeamFrameReader
    SafeRelease(m_pAudioBeamFrameReader);
    // 优雅地关闭Kinect
    if (m_pKinect){
        m_pKinect->Close();
    }
    SafeRelease(m_pKinect);
    if (m_hExit){
        ::CloseHandle(m_hExit);
        m_hExit = nullptr;
    }
    if (m_pAudioBuffer){
        // 写入磁盘
        FILE* file = nullptr;
        if (!fopen_s(&file, "temp.raw", "wb")){
            fwrite(m_pAudioBuffer, m_uBufferUsed, 1, file);
            fclose(file);
        }
        delete[] m_pAudioBuffer;
        m_pAudioBuffer = nullptr;
    }
}


// 音频线程
void ThisApp::AudioThread(ThisApp* pointer){
    // 先设置
    HANDLE events[] = { pointer->m_hExit, reinterpret_cast<HANDLE>(pointer->m_hAudioBeamFrameArrived)};
    bool exit = false;
    while (!exit) {
        switch (::WaitForMultipleObjects(lengthof(events), events, FALSE, INFINITE)){
        case WAIT_OBJECT_0 + 0:
            // 退出程序
            exit = true;
            break;
        case WAIT_OBJECT_0 + 1:
            // 音频帧到达
            pointer->check_audio_frame();
            break;
        }
    }
}

// 初始化Kinect
HRESULT ThisApp::init_kinect(){
    IAudioSource* pAudioSource = nullptr;
    // 查找当前默认Kinect
    HRESULT hr = ::GetDefaultKinectSensor(&m_pKinect);
    // 绅士地打开Kinect
    if (SUCCEEDED(hr)){
        hr = m_pKinect->Open();
    }
    // 获取音频源(AudioSource)
    if (SUCCEEDED(hr)){
        hr = m_pKinect->get_AudioSource(&pAudioSource);
    }
    // 再获取音频帧读取器
    if (SUCCEEDED(hr)){
        hr = pAudioSource->OpenReader(&m_pAudioBeamFrameReader);
    }
    // 注册临帧事件
    if (SUCCEEDED(hr)){
        m_pAudioBeamFrameReader->SubscribeFrameArrived(&m_hAudioBeamFrameArrived);
    }
    SafeRelease(pAudioSource);
    return hr;
}


// 检查音频帧
void ThisApp::check_audio_frame(){
    if (!m_pAudioBeamFrameReader) return;
    // 音频临帧事件参数
    IAudioBeamFrameArrivedEventArgs* pArgs = nullptr;
    // 音频帧引用
    IAudioBeamFrameReference* pABFrameRef = nullptr;
    // 音频帧链表
    IAudioBeamFrameList* pAudioBeamFrameList = nullptr;
    // 音频帧
    IAudioBeamFrame* pAudioBeamFrame = nullptr;
    // 副音频帧数量
    UINT32 subframe_count = 0;

    // 处理新的音频帧
    HRESULT hr = m_pAudioBeamFrameReader->GetFrameArrivedEventData(m_hAudioBeamFrameArrived, &pArgs);

    // 获取帧引用
    if (SUCCEEDED(hr)) {
        hr = pArgs->get_FrameReference(&pABFrameRef);
    }
    // 获取帧链表
    if (SUCCEEDED(hr)) {
        hr = pABFrameRef->AcquireBeamFrames(&pAudioBeamFrameList);
    }
    // 获取音频帧 目前音频帧链表只支持一个音频帧， 直接获取即可
    if (SUCCEEDED(hr))  {
        hr = pAudioBeamFrameList->OpenAudioBeamFrame(0, &pAudioBeamFrame);
    }
    // 副音频帧数量
    if (SUCCEEDED(hr))  {
        hr = pAudioBeamFrame->get_SubFrameCount(&subframe_count);
    }
    // 处理音频
    if (SUCCEEDED(hr) && subframe_count){
        for (UINT i = 0U; i < subframe_count; ++i){
            UINT count = 0; BYTE* data = nullptr;
            IAudioBeamSubFrame* pAudioBeamSubFrame = nullptr;
            // 获取副音频流
            hr = pAudioBeamFrame->GetSubFrame(i, &pAudioBeamSubFrame);
            // 获取音频流
            if (SUCCEEDED(hr)){
                hr = pAudioBeamSubFrame->AccessUnderlyingBuffer(&count, &data);
            }
            // 复制数据
            if (SUCCEEDED(hr)){
                memcpy(m_pAudioBuffer + m_uBufferUsed, data, count);
                m_uBufferUsed += count;
            }
            SafeRelease(pAudioBeamSubFrame);
        }
    }
    SafeRelease(pAudioBeamFrame);
    SafeRelease(pAudioBeamFrameList);
    SafeRelease(pABFrameRef);
    SafeRelease(pArgs);
}