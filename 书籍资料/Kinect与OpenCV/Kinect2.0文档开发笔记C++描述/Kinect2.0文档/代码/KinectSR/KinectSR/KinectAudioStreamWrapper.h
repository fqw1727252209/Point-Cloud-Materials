#pragma once


// Kinect 音频流简单封装
class KinectAudioStreamWrapper : public IStream{
public:
    // 构造函数
    KinectAudioStreamWrapper(IStream *p32BitAudioStream);
    // 析构函数
    ~KinectAudioStreamWrapper();
    // 删除默认构造
    KinectAudioStreamWrapper() = delete;
    // 这是语音状态
    void SetSpeechState(BOOL state){ m_SpeechActive = state; }
    // IUnknown 方法 实现
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() {
        UINT ref = InterlockedDecrement(&m_cRef);
        if (ref == 0){
            delete this;
        }
        return ref;
    }
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        if (riid == IID_IUnknown) {
            AddRef();
            *ppv = (IUnknown*)this;
            return S_OK;
        }
        else if (riid == IID_IStream) {
            AddRef();
            *ppv = (IStream*)this;
            return S_OK;
        }
        else {
            return E_NOINTERFACE;
        }
    }
    // IStream 方法
    STDMETHODIMP Read(void *, ULONG, ULONG *);
    STDMETHODIMP Write(const void *, ULONG, ULONG *);
    STDMETHODIMP Seek(LARGE_INTEGER, DWORD, ULARGE_INTEGER *);
    STDMETHODIMP SetSize(ULARGE_INTEGER);
    STDMETHODIMP CopyTo(IStream *, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *);
    STDMETHODIMP Commit(DWORD);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    STDMETHODIMP Stat(STATSTG *, DWORD);
    STDMETHODIMP Clone(IStream **);
private:
    // 引用计数
    UINT                    m_cRef = 1;
    // 浮点缓冲区
    float*                  m_pFloatBuffer = nullptr;
    // 缓冲区大小
    UINT                    m_uFloatBuferSize = 0;
    // 封装对象
    IStream*                m_p32BitAudio;
    // 语音状态 使用BOOL保证数据对齐
    BOOL                    m_SpeechActive = FALSE;
};