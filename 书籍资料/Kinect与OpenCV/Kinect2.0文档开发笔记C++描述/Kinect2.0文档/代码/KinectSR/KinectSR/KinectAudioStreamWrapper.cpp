#include "stdafx.h"
#include "included.h"


// KinectAudioStreamWrapper 构造函数
KinectAudioStreamWrapper::KinectAudioStreamWrapper(IStream *p32BitAudio) :m_p32BitAudio(p32BitAudio){
    // 增加计数
    if (m_p32BitAudio){
        m_p32BitAudio->AddRef();
    }
}


// 析构函数
KinectAudioStreamWrapper::~KinectAudioStreamWrapper(){
    SafeRelease(m_p32BitAudio);
    if (m_pFloatBuffer){
        delete[] m_pFloatBuffer;
        m_pFloatBuffer = nullptr;
    }
}




// IStream Read方法的实现
STDMETHODIMP KinectAudioStreamWrapper::Read(void *pBuffer, ULONG cbBuffer, ULONG *pcbRead){
    // 参数检查
    if (!pBuffer || !pcbRead) return E_INVALIDARG;
    // 在读取前未使用 m_SpeechActive 返回S_OK
    if (!m_SpeechActive){
        *pcbRead = cbBuffer;
        return S_OK;
    }
    HRESULT hr = S_OK;
    // 目标是将浮点编码转换成16位PCM编码
    INT16* const p16Buffer = reinterpret_cast<INT16*>(pBuffer);
    // 长度倍数
    const int multiple = sizeof(float) / sizeof(INT16);
    // 检查缓冲区释放足够
    auto float_buffer_size = cbBuffer / multiple;
    if (float_buffer_size > m_uFloatBuferSize){
        // 不够就重新申请内存
        m_uFloatBuferSize = float_buffer_size;
        if (m_pFloatBuffer) delete[]m_pFloatBuffer;
        m_pFloatBuffer = new float[m_uFloatBuferSize];
    }
    // 缓冲区写入进度 字节为单位
    BYTE* pWriteProgress = reinterpret_cast<BYTE*>(m_pFloatBuffer);
    // 目前读取量
    ULONG bytesRead = 0;
    // 需要读取量
    ULONG bytesNeed = cbBuffer * multiple;
    // 循环读取
    while (true){
        // 已经不需要语音的情况下
        if (!m_SpeechActive){
            *pcbRead = cbBuffer;
            hr = S_OK;
            break;
        }
        // 从包装对象获取数据
        hr = m_p32BitAudio->Read(pWriteProgress, bytesNeed, &bytesRead);
        bytesNeed -= bytesRead;
        pWriteProgress += bytesRead;
        // 检查是否足够
        if (!bytesNeed){
            *pcbRead = cbBuffer;
            break;
        }
        // 不然就睡一个时间片的时间
        Sleep(20);
    }
    // 数据处理 float -> 16bit PCM
    if (!bytesNeed){
        for (UINT i = 0; i < cbBuffer / multiple; i++) {
            float sample = m_pFloatBuffer[i];
            // 区间保证
            //sample = max(min(sample, 1.f), -1.f);
            if (sample > 1.f) sample = 1.f;
            if (sample < -1.f) sample = -1.f;
            // 数据转换
            float sampleScaled = sample * (float)SHRT_MAX;
            p16Buffer[i] = (sampleScaled > 0.f) ? (INT16)(sampleScaled + 0.5f) : (INT16)(sampleScaled - 0.5f);
        }
    }
    return hr;
}

// 其他不需要支持的方法实现

STDMETHODIMP KinectAudioStreamWrapper::Write(const void *, ULONG, ULONG *)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::Seek(LARGE_INTEGER /* dlibMove */, DWORD /* dwOrigin */, ULARGE_INTEGER * /* plibNewPosition */)
{
    // Seek在语音识别中是个比较关键的函数 Kinect目前不支持 但是防止失败返回S_OK
    return S_OK;
}

STDMETHODIMP KinectAudioStreamWrapper::SetSize(ULARGE_INTEGER)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::Commit(DWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::Stat(STATSTG *, DWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP KinectAudioStreamWrapper::Clone(IStream **)
{
    return E_NOTIMPL;
}
