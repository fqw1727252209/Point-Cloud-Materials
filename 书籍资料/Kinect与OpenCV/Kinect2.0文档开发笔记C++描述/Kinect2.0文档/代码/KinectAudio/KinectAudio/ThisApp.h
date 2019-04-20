// ThisApp类 本程序的抽象

#pragma once

// ThisApp类
class ThisApp
{
public:
	// 构造函数
	ThisApp();
	// 析构函数
	~ThisApp();
    // 退出
    void Exit(){ SetEvent(m_hExit); m_threadAudio.join(); }
private:
	// 窗口过程函数
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    // 初始化Kinect
    HRESULT init_kinect();
    // 检查音频帧
    void check_audio_frame();
    // 音频线程
    static void AudioThread(ThisApp* pointer);
private:
    // Kinect v2 传感器
    IKinectSensor*          m_pKinect = nullptr;
    // 音频帧读取器
    IAudioBeamFrameReader*  m_pAudioBeamFrameReader = nullptr;
    // 音频临帧事件 不能用nullptr初始化 蛋疼
    WAITABLE_HANDLE         m_hAudioBeamFrameArrived = 0;
    // 退出事件
    HANDLE                  m_hExit = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
    // 音频缓冲区
    BYTE*                   m_pAudioBuffer = nullptr;
    // 当前写入进度
    UINT32                  m_uBufferUsed = 0;
    // 音频处理线程
    std::thread             m_threadAudio;
};