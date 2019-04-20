#include "stdafx.h"
#include "included.h"
#include <wchar.h>


static const float c_JointThickness = 3.0f;
static const float c_TrackedBoneThickness = 6.0f;
static const float c_InferredBoneThickness = 1.0f;
static const float c_HandSize = 30.0f;

// ImageRender类构造函数
ImageRenderer::ImageRenderer(){
	// 创建资源
	m_hrInit = CreateDeviceIndependentResources();
    m_timer.Start();
}


// 创建设备无关资源
HRESULT ImageRenderer::CreateDeviceIndependentResources(){
	HRESULT hr = S_OK;

	// 创建 Direct2D 工厂.
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

	if (SUCCEEDED(hr))
	{
		// 创建 WIC 工厂.
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			reinterpret_cast<void **>(&m_pWICFactory)
			);
	}

	if (SUCCEEDED(hr))
	{
		// 创建 DirectWrite 工厂.
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(m_pDWriteFactory),
			reinterpret_cast<IUnknown **>(&m_pDWriteFactory)
			);
	}

	if (SUCCEEDED(hr))
	{
		// 创建正文文本格式.
		hr = m_pDWriteFactory->CreateTextFormat(
			L"Microsoft YaHei",
			nullptr,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			60.f,
			L"", //locale
			&m_pTextFormatMain
			);
	}

	return hr;
}

// 从文件读取位图
HRESULT ImageRenderer::LoadBitmapFromFile(
	ID2D1RenderTarget *pRenderTarget,
	IWICImagingFactory *pIWICFactory,
	PCWSTR uri,
	UINT destinationWidth,
	UINT destinationHeight,
	ID2D1Bitmap **ppBitmap
	)
{
	IWICBitmapDecoder *pDecoder = NULL;
	IWICBitmapFrameDecode *pSource = NULL;
	IWICStream *pStream = NULL;
	IWICFormatConverter *pConverter = NULL;
	IWICBitmapScaler *pScaler = NULL;

	HRESULT hr = pIWICFactory->CreateDecoderFromFilename(
		uri,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
		);

	if (SUCCEEDED(hr))
	{
		hr = pDecoder->GetFrame(0, &pSource);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}


	if (SUCCEEDED(hr))
	{
		if (destinationWidth != 0 || destinationHeight != 0)
		{
			UINT originalWidth, originalHeight;
			hr = pSource->GetSize(&originalWidth, &originalHeight);
			if (SUCCEEDED(hr))
			{
				if (destinationWidth == 0)
				{
					FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
					destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
				}
				else if (destinationHeight == 0)
				{
					FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
					destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
				}

				hr = pIWICFactory->CreateBitmapScaler(&pScaler);
				if (SUCCEEDED(hr))
				{
					hr = pScaler->Initialize(
						pSource,
						destinationWidth,
						destinationHeight,
						WICBitmapInterpolationModeCubic
						);
				}
				if (SUCCEEDED(hr))
				{
					hr = pConverter->Initialize(
						pScaler,
						GUID_WICPixelFormat32bppPBGRA,
						WICBitmapDitherTypeNone,
						NULL,
						0.f,
						WICBitmapPaletteTypeMedianCut
						);
				}
			}
		}
		else
		{
			hr = pConverter->Initialize(
				pSource,
				GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone,
				NULL,
				0.f,
				WICBitmapPaletteTypeMedianCut
				);
		}
	}
	if (SUCCEEDED(hr))
	{
		hr = pRenderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			ppBitmap
			);
	}

	SafeRelease(pDecoder);
	SafeRelease(pSource);
	SafeRelease(pStream);
	SafeRelease(pConverter);
	SafeRelease(pScaler);

	return hr;
}

// 创建设备相关资源
HRESULT ImageRenderer::CreateDeviceResources()
{
	HRESULT hr = S_OK;
	if (!m_pRenderTarget)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
			);

		// 创建 Direct2D RenderTarget.
		hr = m_pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&m_pRenderTarget
			);

        // 创建白色笔刷
        if (SUCCEEDED(hr)){
            ID2D1SolidColorBrush* pSolidColorBrush = nullptr;
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pSolidColorBrush);
            m_pWhiteBrush = pSolidColorBrush;
        }

        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.27f, 0.75f, 0.27f), &m_pBrushJointTracked);
        }
       
        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
        }

        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 1.0f), &m_pBrushBoneTracked);
        }

        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 1.0f), &m_pBrushBoneInferred);
        }

        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.5f), &m_pBrushHandClosed);
        }

        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 0.5f), &m_pBrushHandOpen);
        }

        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 0.5f), &m_pBrushHandLasso);
        }
	}

	return hr;
}

// ImageRender析构函数
ImageRenderer::~ImageRenderer(){
	DiscardDeviceResources();
	SafeRelease(m_pD2DFactory);
	SafeRelease(m_pWICFactory);
	SafeRelease(m_pDWriteFactory);
	SafeRelease(m_pTextFormatMain);
}

// 丢弃设备相关资源
void ImageRenderer::DiscardDeviceResources(){
	// 清空位图缓存
	for (BitmapCacheMap::iterator itr = m_mapBitmapCache.begin(); itr != m_mapBitmapCache.end(); ++itr){
		SafeRelease(itr->second);
	}
    m_mapBitmapCache.clear();
    SafeRelease(m_pWhiteBrush);
    SafeRelease(m_pRenderTarget);
}


// 获取图片
// bitmapName	[in] : 文件名
// 返回: NULL表示失败 其余的则为位图的指针
ID2D1Bitmap* ImageRenderer::GetBitmap(std::wstring& bitmapName){
	ID2D1Bitmap* pBitmap;
	// 缓存中没有的话，从文件中读取
	BitmapCacheMap::iterator itr = m_mapBitmapCache.find(bitmapName);
	if (itr == m_mapBitmapCache.end()){
		// 读取成功的话
		if (SUCCEEDED(LoadBitmapFromFile(m_pRenderTarget, m_pWICFactory, bitmapName.c_str(), 0, 0, &pBitmap)))
			return m_mapBitmapCache[bitmapName] = pBitmap;
		else
			return m_mapBitmapCache[bitmapName] = NULL;
	}
	else
		return itr->second;
}
// 渲染图形图像
HRESULT ImageRenderer::OnRender(){
	HRESULT hr = S_OK;
    WCHAR buffer[1024];
    D2D1_RECT_F rect;
    // 常试创建资源
    hr = CreateDeviceResources();
	if (SUCCEEDED(hr)){
		// 开始
		m_pRenderTarget->BeginDraw();
		// 重置转换
        m_pRenderTarget->SetTransform(this->matrix);
		// 清屏
		m_pRenderTarget->Clear(D2D1::ColorF(0x0066CCFF));
		// 刻画骨骼
        DrawBody();
       // 复位显示FPS
        auto length = swprintf_s(buffer, 
            L"帧率: %2.2f\n放大率x: %2.2f\n放大率y: %2.2f",
            m_fFPS, this->matrix._11, this->matrix._22);
        auto size = m_pRenderTarget->GetSize();
        rect.left = 0.f; rect.right = size.width;
        rect.top = 0.f; rect.bottom = size.height;
        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->DrawText(buffer, length, m_pTextFormatMain, &rect, m_pWhiteBrush);
		// 结束刻画
		hr = m_pRenderTarget->EndDraw();
		// 收到重建消息时，释放资源，等待下次自动创建
		if (hr == D2DERR_RECREATE_TARGET) {
			DiscardDeviceResources();
			hr = S_OK;
		}
	}
	return hr;
}


// 设置骨骼数据
void ImageRenderer::SetBodyInfo(int i, BodyInfo* info){
    memcpy(m_bodyInfo + i, info, sizeof(BodyInfo));
    m_bodyInfo[i].ok = TRUE;
}


// 刻画骨骼
void ImageRenderer::DrawBody(){
    m_fFPS = 1000.f / m_timer.DeltaF_ms();
    m_timer.MovStartEnd();
    for (int i = 0; i < BODY_COUNT; ++i){
        if (!m_bodyInfo[i].ok) continue;
        m_bodyInfo[i].ok = FALSE;
        DrawBody(m_bodyInfo[i].joints, m_bodyInfo[i].jointPoints);
        DrawHand(m_bodyInfo[i].leftHandState, m_bodyInfo[i].jointPoints[JointType_HandLeft]);
        DrawHand(m_bodyInfo[i].rightHandState, m_bodyInfo[i].jointPoints[JointType_HandRight]);
    }
}



// 
void ImageRenderer::DrawBody(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints){

    DrawBone(pJoints, pJointPoints, JointType_Head, JointType_Neck);
    DrawBone(pJoints, pJointPoints, JointType_Neck, JointType_SpineShoulder);
    DrawBone(pJoints, pJointPoints, JointType_SpineShoulder, JointType_SpineMid);
    DrawBone(pJoints, pJointPoints, JointType_SpineMid, JointType_SpineBase);
    DrawBone(pJoints, pJointPoints, JointType_SpineShoulder, JointType_ShoulderRight);
    DrawBone(pJoints, pJointPoints, JointType_SpineShoulder, JointType_ShoulderLeft);
    DrawBone(pJoints, pJointPoints, JointType_SpineBase, JointType_HipRight);
    DrawBone(pJoints, pJointPoints, JointType_SpineBase, JointType_HipLeft);
 
    DrawBone(pJoints, pJointPoints, JointType_ShoulderRight, JointType_ElbowRight);
    DrawBone(pJoints, pJointPoints, JointType_ElbowRight, JointType_WristRight);
    DrawBone(pJoints, pJointPoints, JointType_WristRight, JointType_HandRight);
    DrawBone(pJoints, pJointPoints, JointType_HandRight, JointType_HandTipRight);
    DrawBone(pJoints, pJointPoints, JointType_WristRight, JointType_ThumbRight);

    DrawBone(pJoints, pJointPoints, JointType_ShoulderLeft, JointType_ElbowLeft);
    DrawBone(pJoints, pJointPoints, JointType_ElbowLeft, JointType_WristLeft);
    DrawBone(pJoints, pJointPoints, JointType_WristLeft, JointType_HandLeft);
    DrawBone(pJoints, pJointPoints, JointType_HandLeft, JointType_HandTipLeft);
    DrawBone(pJoints, pJointPoints, JointType_WristLeft, JointType_ThumbLeft);

    DrawBone(pJoints, pJointPoints, JointType_HipRight, JointType_KneeRight);
    DrawBone(pJoints, pJointPoints, JointType_KneeRight, JointType_AnkleRight);
    DrawBone(pJoints, pJointPoints, JointType_AnkleRight, JointType_FootRight);

    DrawBone(pJoints, pJointPoints, JointType_HipLeft, JointType_KneeLeft);
    DrawBone(pJoints, pJointPoints, JointType_KneeLeft, JointType_AnkleLeft);
    DrawBone(pJoints, pJointPoints, JointType_AnkleLeft, JointType_FootLeft);

    for (int i = 0; i < JointType_Count; ++i)
    {
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(pJointPoints[i], c_JointThickness, c_JointThickness);

        if (pJoints[i].TrackingState == TrackingState_Inferred)
        {
            m_pRenderTarget->FillEllipse(ellipse, m_pBrushJointInferred);
        }
        else if (pJoints[i].TrackingState == TrackingState_Tracked)
        {
            m_pRenderTarget->FillEllipse(ellipse, m_pBrushJointTracked);
        }
    }
}

//
void ImageRenderer::DrawBone(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints, JointType joint0, JointType joint1)
{
    TrackingState joint0State = pJoints[joint0].TrackingState;
    TrackingState joint1State = pJoints[joint1].TrackingState;

    if ((joint0State == TrackingState_NotTracked) || (joint1State == TrackingState_NotTracked))
    {
        return;
    }

    if ((joint0State == TrackingState_Inferred) && (joint1State == TrackingState_Inferred))
    {
        return;
    }

    if ((joint0State == TrackingState_Tracked) && (joint1State == TrackingState_Tracked))
    {
        m_pRenderTarget->DrawLine(pJointPoints[joint0], pJointPoints[joint1], m_pBrushBoneTracked, c_TrackedBoneThickness);
    }
    else
    {
        m_pRenderTarget->DrawLine(pJointPoints[joint0], pJointPoints[joint1], m_pBrushBoneInferred, c_InferredBoneThickness);
    }
}

//
void ImageRenderer::DrawHand(HandState handState, const D2D1_POINT_2F& handPosition)
{
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(handPosition, c_HandSize, c_HandSize);

    switch (handState)
    {
    case HandState_Closed:
        m_pRenderTarget->FillEllipse(ellipse, m_pBrushHandClosed);
        break;

    case HandState_Open:
        m_pRenderTarget->FillEllipse(ellipse, m_pBrushHandOpen);
        break;

    case HandState_Lasso:
        m_pRenderTarget->FillEllipse(ellipse, m_pBrushHandLasso);
        break;
    }
}
