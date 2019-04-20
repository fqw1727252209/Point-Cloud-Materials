#include "stdafx.h"
#include "included.h"
#include <wchar.h>


#define PI_F       (3.1415926f)

static const WCHAR* FACE_TEXT = LR"face_text(面部旋转:
    Picth: %03.03f
    Yaw: %03.03f
    Roll: %03.03f
面部表情:
    下巴张开: %03.03f
    撅起嘴巴: %03.03f
    下巴右滑: %03.03f
    嘴唇右拉: %03.03f
    嘴唇左拉: %03.03f
    嘴角左上: %03.03f
    嘴角右上: %03.03f
    嘴角左下: %03.03f
    嘴角右下: %03.03f
    鼓左脸颊: %03.03f
    鼓右脸颊: %03.03f
    闭着左眼: %03.03f
    闭着右眼: %03.03f
    右边皱眉: %03.03f
    左边皱眉: %03.03f
    下唇左下: %03.03f
    下唇右下: %03.03f

)face_text";


// ImageRender类构造函数
ImageRenderer::ImageRenderer(){
    ZeroMemory(&(this->data), sizeof(this->data));
	// 创建资源
	m_hrInit = CreateDeviceIndependentResources();
    // 创建缓冲区
    m_pColorRGBX = new RGBQUAD[IMAGE_WIDTH*IMAGE_HEIGHT];
    if (!m_pColorRGBX) m_hrInit = E_OUTOFMEMORY;
    m_timer.Start();
}


// 创建设备无关资源
HRESULT ImageRenderer::CreateDeviceIndependentResources(){
	HRESULT hr = S_OK;
	// 创建 Direct2D 工厂.
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    // 创建 WIC 工厂.
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            reinterpret_cast<void **>(&m_pWICFactory)
            );
    }
    // 创建 DirectWrite 工厂.
	if (SUCCEEDED(hr)){
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(m_pDWriteFactory),
			reinterpret_cast<IUnknown **>(&m_pDWriteFactory)
			);
	}
    // 创建正文文本格式.
	if (SUCCEEDED(hr)) {
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
    // 创建显示文本格式.
    if (SUCCEEDED(hr))  {
        hr = m_pDWriteFactory->CreateTextFormat(
            L"Microsoft YaHei",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            40.f,
            L"", //locale
            &m_pTextFormatShow
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

        // 创建刻画位图
        if (SUCCEEDED(hr)){
            D2D1_BITMAP_PROPERTIES prop = { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, 96.f, 96.f };
            hr = m_pRenderTarget->CreateBitmap(
                D2D1::SizeU(IMAGE_WIDTH, IMAGE_HEIGHT),
                prop,
                &m_pDrawBitmap
                );
        }

        // 创建白色笔刷
        if (SUCCEEDED(hr)){
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pWhiteBrush);
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
    SafeRelease(m_pTextFormatShow);
    if (m_pColorRGBX){
        delete[] m_pColorRGBX;
        m_pColorRGBX = nullptr;
    }
}

// 丢弃设备相关资源
void ImageRenderer::DiscardDeviceResources(){
	// 清空位图缓存
    SafeRelease(m_pDrawBitmap);
    SafeRelease(m_pWhiteBrush);
    SafeRelease(m_pRenderTarget);
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
		// 正式刻画.........
        m_pRenderTarget->DrawBitmap(m_pDrawBitmap);
        // 刻画信息
        this->draw_facedata();
        // 复位显示FPS
        auto length = swprintf_s(buffer, 
            L"帧率: %2.2f\n放大率x: %2.2f\n放大率y: %2.2f",
            m_fFPS, this->matrix._11, this->matrix._22);
        auto size = m_pRenderTarget->GetSize();
        rect.left = 0.f; rect.right = size.width;
        rect.top = 0.f; rect.bottom = size.height;

        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->DrawText(buffer, length, m_pTextFormatMain, &rect, m_pWhiteBrush);
        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Scale(0.9f, 0.9f, D2D1::Point2F(1000.f, 0.f)));
        rect = { 0.f, 0.f, static_cast<float>(IMAGE_WIDTH), 50.f };
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

// 写入数据
void ImageRenderer::WriteBitmapData(RGBQUAD* data, int width, int height){
    if (m_pDrawBitmap){
        D2D1_RECT_U rect = { 0, 0, width, height };
        m_pDrawBitmap->CopyFromMemory(&rect, data, width*sizeof(RGBQUAD));
    }
    m_fFPS = 1000.f / m_timer.DeltaF_ms();
    m_timer.MovStartEnd();
    OnRender();
}



// 刻画面部数据
void ImageRenderer::draw_facedata(){
    // 跟踪状态 ?
    if (!this->data.tracked) return;
    // 面部外框
    D2D1_RECT_F rect;
    rect.left = static_cast<float>(this->data.face_box.Left);
    rect.top = static_cast<float>(this->data.face_box.Top);
    rect.right = static_cast<float>(this->data.face_box.Right);
    rect.bottom = static_cast<float>(this->data.face_box.Bottom);
    m_pRenderTarget->DrawRectangle(rect, m_pWhiteBrush, 5.f);
    // 头部旋转中心
    D2D1_ELLIPSE ellipse = { this->data.head_pivot_1, 20.f, 10.f };
    m_pRenderTarget->FillEllipse(ellipse, m_pWhiteBrush);
    // 旋转四元数
    WCHAR buffer[1024];
    float picth, yaw, roll;
    ExtractFaceRotationInDegrees(&this->data.face_rotation, &picth, &yaw, &roll);
    auto length = swprintf_s(buffer, FACE_TEXT, picth, yaw, roll,
        this->data.au[FaceShapeAnimations_JawOpen],
        this->data.au[FaceShapeAnimations_LipPucker],
        this->data.au[FaceShapeAnimations_JawSlideRight],
        this->data.au[FaceShapeAnimations_LipStretcherRight],
        this->data.au[FaceShapeAnimations_LipStretcherLeft],
        this->data.au[FaceShapeAnimations_LipCornerPullerLeft],
        this->data.au[FaceShapeAnimations_LipCornerPullerRight],
        this->data.au[FaceShapeAnimations_LipCornerDepressorLeft],
        this->data.au[FaceShapeAnimations_LipCornerDepressorRight],
        this->data.au[FaceShapeAnimations_LeftcheekPuff],
        this->data.au[FaceShapeAnimations_RightcheekPuff],
        this->data.au[FaceShapeAnimations_LefteyeClosed],
        this->data.au[FaceShapeAnimations_RighteyeClosed],
        this->data.au[FaceShapeAnimations_RighteyebrowLowerer],
        this->data.au[FaceShapeAnimations_LefteyebrowLowerer],
        this->data.au[FaceShapeAnimations_LowerlipDepressorLeft],
        this->data.au[FaceShapeAnimations_LowerlipDepressorRight]
        );
    // 刻画
    rect.left = rect.right + 10.f;
    rect.top -= 100.f;
    rect.right = static_cast<float>(IMAGE_WIDTH);
    rect.bottom = static_cast<float>(IMAGE_HEIGHT);
    m_pRenderTarget->DrawTextW(
        buffer,
        length,
        m_pTextFormatShow,
        rect,
        m_pWhiteBrush
        );
}




// 旋转四元数 转换成 旋转角度
void ImageRenderer::ExtractFaceRotationInDegrees(const Vector4* pQuaternion, float* pPitch, float* pYaw, float* pRoll){
#if 1
#define x (pQuaternion->x)
#define y (pQuaternion->y)
#define z (pQuaternion->z)
#define w (pQuaternion->w)

    // convert face rotation quaternion to Euler angles in degrees		
    //double dPitch, dYaw, dRoll;
    *pPitch = atan2f(2.f * (y * z + w * x), w * w - x * x - y * y + z * z) / PI_F * 180.f;
    *pYaw = asinf(2.f * (w * y - x * z)) / PI_F * 180.f;
    *pRoll = atan2f(2.f * (x * y + w * z), w * w + x * x - y * y - z * z) / PI_F * 180.f;

#undef x
#undef y
#undef z
#undef w

#else

    double x = pQuaternion->x;
    double y = pQuaternion->y;
    double z = pQuaternion->z;
    double w = pQuaternion->w;

    double dPitch, dYaw, dRoll;
    dPitch = atan2(2 * (y * z + w * x), w * w - x * x - y * y + z * z) / M_PI * 180.0;
    dYaw = asin(2 * (w * y - x * z)) / M_PI * 180.0;
    dRoll = atan2(2 * (x * y + w * z), w * w + x * x - y * y - z * z) / M_PI * 180.0;

    double increment = 5;
    *pPitch = float(static_cast<int>((dPitch + increment / 2.0 * (dPitch > 0 ? 1.0 : -1.0)) / increment) * static_cast<int>(increment));
    *pYaw = float(static_cast<int>((dYaw + increment / 2.0 * (dYaw > 0 ? 1.0 : -1.0)) / increment) * static_cast<int>(increment));
    *pRoll = float(static_cast<int>((dRoll + increment / 2.0 * (dRoll > 0 ? 1.0 : -1.0)) / increment) * static_cast<int>(increment));

#endif
}