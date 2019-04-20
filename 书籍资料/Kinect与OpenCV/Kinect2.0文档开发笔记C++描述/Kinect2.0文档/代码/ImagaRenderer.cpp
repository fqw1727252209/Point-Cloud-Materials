#include "stdafx.h"
#include "included.h"

// ImageRender类构造函数
ImageRenderer::ImageRenderer() :m_hrInit(E_FAIL),
m_hwnd(NULL),
m_pD2DFactory(NULL),
m_pWICFactory(NULL),
m_pDWriteFactory(NULL),
m_pRenderTarget(NULL),
m_pTextFormatMain(NULL)
{
	// 创建资源
	m_hrInit = CreateDeviceIndependentResources();
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
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			30.f,
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
	SafeRelease(m_pRenderTarget);
	// 清空位图缓存
	for (BitmapCacheMap::iterator itr = m_mapBitmapCache.begin(); itr != m_mapBitmapCache.end(); ++itr){
		SafeRelease(itr->second);
	}
	m_mapBitmapCache.clear();
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
	hr = CreateDeviceResources();
	if (SUCCEEDED(hr)){
		// 开始
		m_pRenderTarget->BeginDraw();
		// 重置转换
		m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		// 清屏
		m_pRenderTarget->Clear(D2D1::ColorF(0x0066CCFF));
		// 正式刻画.........
		
		// 结束刻画
		hr = m_pRenderTarget->EndDraw();
		// 收到重建消息时，释放资源，等待下次自动创建
		if (hr == D2DERR_RECREATE_TARGET)
		{
			DiscardDeviceResources();
			hr = S_OK;
		}
	}
	return hr;
}