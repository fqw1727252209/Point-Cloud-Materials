#include "stdafx.h"
#include "included.h"
#include <wchar.h>


// ImageRender类构造函数
ImageRenderer::ImageRenderer(){
	// 创建资源
	m_hrInit = CreateDeviceIndependentResources();
    // 创建缓冲区
    m_pColorRGBX = new RGBQUAD[IMAGE_D_WIDTH*IMAGE_D_HEIGHT];
    if (!m_pColorRGBX) m_hrInit = E_OUTOFMEMORY;
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

        // 创建刻画位图
        if (SUCCEEDED(hr)){
            D2D1_BITMAP_PROPERTIES prop = { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, 96.f, 96.f };
            hr = m_pRenderTarget->CreateBitmap(
                D2D1::SizeU(IMAGE_D_WIDTH, IMAGE_D_HEIGHT),
                prop,
                &m_pDrawBitmap
                );
        }

        // 创建白色笔刷
        if (SUCCEEDED(hr)){
            ID2D1SolidColorBrush* pSolidColorBrush = nullptr;
            hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pSolidColorBrush);
            m_pWhiteBrush = pSolidColorBrush;
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
    if (m_pColorRGBX){
        delete[] m_pColorRGBX;
        m_pColorRGBX = nullptr;
    }
}

// 丢弃设备相关资源
void ImageRenderer::DiscardDeviceResources(){
	// 清空位图缓存
    SafeRelease(m_pDrawBitmap);
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
		// 正式刻画.........
        m_pRenderTarget->DrawBitmap(m_pDrawBitmap);
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

// 写入数据
void ImageRenderer::WriteBitmapData(int width, int height){
    if (m_pDrawBitmap){
        D2D1_RECT_U rect = { 0, 0, width, height };
        m_pDrawBitmap->CopyFromMemory(&rect, m_pColorRGBX, width*sizeof(RGBQUAD));
    }
    m_fFPS = 1000.f / m_timer.DeltaF_ms();
    m_timer.MovStartEnd();
    OnRender();
}